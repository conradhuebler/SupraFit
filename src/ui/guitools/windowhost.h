/*
 * Window host: shows a dialog as a window of its own and remembers where the user put it.
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtCore/QSettings>

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <QtWidgets/QWidget>

/*! \brief Shows dialogs that stay windows (scan, detailed information, a result taken out of the pane) the
 * same way every time: the first time centred on the main window, afterwards where the user left it, at the
 * size the user gave it. Without this every dialog opened wherever the toolkit put it (relative to a pane that
 * may sit in a hidden tab) and had forgotten its size. The geometry is kept under the settings group
 * "windows", one key per kind of window (not per model: ten models must not open ten differently placed
 * scan windows). Claude Generated. */
namespace WindowHost {

/*! \brief Remembers the geometry of the window it is installed on: written when the window is moved, resized
 * or hidden while it is on screen. */
class GeometryKeeper : public QObject {
public:
    GeometryKeeper(QWidget* window, const QString& key)
        : QObject(window)
        , m_window(window)
        , m_key(key)
    {
        setObjectName(QStringLiteral("windowhost_keeper"));
        window->installEventFilter(this);
    }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_window && (event->type() == QEvent::Move || event->type() == QEvent::Resize || event->type() == QEvent::Hide)) {
            // an unshown window has no geometry worth keeping (construction, restoring)
            if (m_window->isVisible() || event->type() == QEvent::Hide)
                if (m_window->isWindow() && !m_restoring)
                    QSettings().setValue(QStringLiteral("windows/%1").arg(m_key), m_window->saveGeometry());
        }
        return QObject::eventFilter(watched, event);
    }

    void setRestoring(bool restoring) { m_restoring = restoring; }

private:
    QWidget* m_window;
    QString m_key;
    bool m_restoring = false;
};

/*! \brief Show \p window as a window under \p key. \p anchor (any widget of the main window) decides where it
 * opens the first time; an already open window is only brought to the front. */
inline void Show(QWidget* window, const QString& key, QWidget* anchor = nullptr)
{
    if (!window)
        return;
    if (!window->isWindow())
        window->setWindowFlag(Qt::Window, true);

    if (!window->isVisible()) {
        // (a class without Q_OBJECT cannot be found by type: the object name does it)
        GeometryKeeper* keeper = static_cast<GeometryKeeper*>(window->findChild<QObject*>(QStringLiteral("windowhost_keeper"), Qt::FindDirectChildrenOnly));
        if (!keeper)
            keeper = new GeometryKeeper(window, key);
        keeper->setRestoring(true);

        const QByteArray saved = QSettings().value(QStringLiteral("windows/%1").arg(key)).toByteArray();
        bool placed = !saved.isEmpty() && window->restoreGeometry(saved);
        if (placed) { // a saved place on a screen that is gone is no place
            bool onScreen = false;
            for (const QScreen* screen : QGuiApplication::screens())
                if (screen->availableGeometry().intersects(window->frameGeometry()))
                    onScreen = true;
            placed = onScreen;
        }
        if (!placed) {
            QWidget* top = anchor ? anchor->window() : nullptr;
            const QScreen* screen = top && top->screen() ? top->screen() : QGuiApplication::primaryScreen();
            const QRect available = screen ? screen->availableGeometry() : QRect(0, 0, 1280, 800);
            QSize size = window->size();
            size = size.boundedTo(available.size() * 9 / 10);
            window->resize(size);
            const QPoint centre = (top && top->isVisible()) ? top->frameGeometry().center() : available.center();
            QPoint corner = centre - QPoint(size.width() / 2, size.height() / 2);
            corner.setX(qBound(available.left(), corner.x(), qMax(available.left(), available.right() - size.width())));
            corner.setY(qBound(available.top(), corner.y(), qMax(available.top(), available.bottom() - size.height())));
            window->move(corner);
        }
        keeper->setRestoring(false);
    }
    window->show();
    window->raise();
    window->activateWindow();
}

}
