/*
 * DialogMemory - remembered geometry, splitters and last values of the import dialogs
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 *
 * Claude Generated (UX package 2a of the import dialogs).
 */

#include <cmath>

#include <QtCore/QEvent>
#include <QtCore/QSettings>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QWidget>

#include "dialogmemory.h"

DialogMemory::DialogMemory(QWidget* dialog, const QString& name)
    : QObject(dialog)
    , m_dialog(dialog)
    , m_name(name)
{
    if (dialog)
        dialog->installEventFilter(this);
}

QString DialogMemory::key(const QString& entry) const
{
    return QStringLiteral("dialogs/%1/%2").arg(m_name, entry);
}

QVariant DialogMemory::value(const QString& entry, const QVariant& fallback) const
{
    return QSettings().value(key(entry), fallback);
}

void DialogMemory::setValue(const QString& entry, const QVariant& value)
{
    QSettings().setValue(key(entry), value);
}

bool DialogMemory::contains(const QString& entry) const
{
    return QSettings().contains(key(entry));
}

bool DialogMemory::fitsAScreen(const QRect& geometry)
{
    if (!geometry.isValid())
        return false;
    // the top strip carries the title bar: the user must be able to grab it
    const QRect strip(geometry.topLeft(), QSize(qMin(100, geometry.width()), qMin(30, geometry.height())));
    for (const QScreen* screen : QGuiApplication::screens()) {
        if (screen->availableGeometry().contains(strip))
            return true;
    }
    return false;
}

bool DialogMemory::restoreGeometry()
{
    // kept as a plain rectangle: QWidget::restoreGeometry would fit the window into the screen it finds
    // and so change a size the check below is meant to judge
    if (!m_dialog)
        return false;
    const QVariant stored = value(QStringLiteral("geometry"));
    if (stored.typeId() != QMetaType::QRect)
        return false;
    const QRect placed = stored.toRect();
    const QSize minimum = m_dialog->minimumSize();
    if (!placed.isValid() || placed.width() < minimum.width() || placed.height() < minimum.height())
        return false; // squeezed below what the dialog needs: the default size
    if (!fitsAScreen(placed))
        return false; // on a screen that is gone: the default place
    m_dialog->setGeometry(placed);
    return true;
}

void DialogMemory::addSplitter(QSplitter* splitter, const QString& id)
{
    if (!splitter)
        return;
    m_splitters << qMakePair(QPointer<QSplitter>(splitter), id);
    const QByteArray state = value(QStringLiteral("splitter/%1").arg(id)).toByteArray();
    if (!state.isEmpty())
        splitter->restoreState(state); // refuses a state of another layout
}

void DialogMemory::save()
{
    if (!m_dialog)
        return;
    if (m_shown)
        setValue(QStringLiteral("geometry"), m_dialog->geometry());
    for (const auto& entry : m_splitters)
        if (entry.first)
            setValue(QStringLiteral("splitter/%1").arg(entry.second), entry.first->saveState());
}

bool DialogMemory::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_dialog) {
        if (event->type() == QEvent::Show)
            m_shown = true;
        else if (event->type() == QEvent::Hide && m_shown)
            save();
    }
    return QObject::eventFilter(watched, event);
}

bool DialogMemory::readDouble(const QString& entry, double min, double max, double* out) const
{
    const QVariant stored = value(entry);
    if (!stored.isValid())
        return false;
    bool ok = false;
    const double v = stored.toDouble(&ok);
    if (!ok || !std::isfinite(v) || v < min || v > max)
        return false;
    *out = v;
    return true;
}

bool DialogMemory::readInt(const QString& entry, int min, int max, int* out) const
{
    const QVariant stored = value(entry);
    if (!stored.isValid())
        return false;
    bool ok = false;
    const int v = stored.toInt(&ok);
    if (!ok || v < min || v > max)
        return false;
    *out = v;
    return true;
}

bool DialogMemory::readBool(const QString& entry, bool* out) const
{
    const QVariant stored = value(entry);
    if (!stored.isValid())
        return false;
    if (stored.typeId() == QMetaType::Bool) {
        *out = stored.toBool();
        return true;
    }
    const QString text = stored.toString().trimmed().toLower();
    if (text == QLatin1String("true") || text == QLatin1String("1")) {
        *out = true;
        return true;
    }
    if (text == QLatin1String("false") || text == QLatin1String("0")) {
        *out = false;
        return true;
    }
    return false;
}

bool DialogMemory::restore(QDoubleSpinBox* box, const QString& entry) const
{
    double v = 0;
    if (!box || !readDouble(entry, box->minimum(), box->maximum(), &v))
        return false;
    box->setValue(v);
    return true;
}

bool DialogMemory::restore(QSpinBox* box, const QString& entry) const
{
    int v = 0;
    if (!box || !readInt(entry, box->minimum(), box->maximum(), &v))
        return false;
    box->setValue(v);
    return true;
}

bool DialogMemory::restore(QComboBox* box, const QString& entry) const
{
    int v = 0;
    if (!box || box->count() == 0 || !readInt(entry, 0, box->count() - 1, &v))
        return false;
    box->setCurrentIndex(v);
    return true;
}

bool DialogMemory::restore(QCheckBox* box, const QString& entry) const
{
    bool v = false;
    if (!box || !readBool(entry, &v))
        return false;
    box->setChecked(v);
    return true;
}
