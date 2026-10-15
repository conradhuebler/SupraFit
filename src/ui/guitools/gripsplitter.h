/*
 * Splitter with a visible grip on its handles.
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
 *
 * Claude Generated (UI redesign WP8).
 */

#pragma once

#include <QtGui/QPainter>

#include <QtWidgets/QSplitter>
#include <QtWidgets/QSplitterHandle>

/*! \brief Splitter handle that draws a short grip bar in the middle (palette colours, highlighted
 * while the mouse is over it), so the handle is recognisable as draggable in light and dark mode. */
class GripSplitterHandle : public QSplitterHandle {
public:
    GripSplitterHandle(Qt::Orientation orientation, QSplitter* parent)
        : QSplitterHandle(orientation, parent)
    {
        setAttribute(Qt::WA_Hover);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const bool hover = underMouse();
        QColor colour = hover ? palette().color(QPalette::Highlight) : palette().color(QPalette::WindowText);
        if (!hover)
            colour.setAlpha(90);
        painter.setPen(Qt::NoPen);
        painter.setBrush(colour);
        const int length = 48, thickness = 4;
        QRectF grip = orientation() == Qt::Vertical
            ? QRectF((width() - length) / 2.0, (height() - thickness) / 2.0, length, thickness)
            : QRectF((width() - thickness) / 2.0, (height() - length) / 2.0, thickness, length);
        painter.drawRoundedRect(grip, 2, 2);
    }
    bool event(QEvent* event) override
    {
        if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverLeave)
            update();
        return QSplitterHandle::event(event);
    }
};

/*! \brief QSplitter whose handles show a grip (see GripSplitterHandle) and are a bit wider. */
class GripSplitter : public QSplitter {
public:
    explicit GripSplitter(Qt::Orientation orientation, QWidget* parent = nullptr)
        : QSplitter(orientation, parent)
    {
        setHandleWidth(10);
    }

protected:
    QSplitterHandle* createHandle() override { return new GripSplitterHandle(orientation(), this); }
};
