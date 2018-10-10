/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QDebug>
#include <QtCore/QPointer>

#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QTextDocument>

#include <QtWidgets/QGraphicsSceneMouseEvent>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>

#include "peakcallout.h"

PeakCallOut::PeakCallOut(QPointer<QtCharts::QChart> chart)
    : QGraphicsTextItem(chart)
    , m_chart(chart)
{
}

QRectF PeakCallOut::boundingRect() const
{
    QPointF anchor = mapFromParent(m_chart->mapToPosition(m_anchor));
    QRectF rect;
    rect.setLeft(qMin(m_rect.left(), anchor.x()));
    rect.setRight(qMax(m_rect.right(), anchor.x()));
    rect.setTop(qMin(m_rect.top(), anchor.y()));
    rect.setBottom(qMax(m_rect.bottom(), anchor.y()));
    return rect;
}

void PeakCallOut::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    int width = 40;
    int height = -100;
    if (!flip) {
        QFontMetrics fm = painter->fontMetrics();
        width = fm.width(m_text) - 54;
        height = -50;
    }
    qreal x = m_chart->mapToPosition(m_anchor).x() - width / 4;
    qreal y = m_chart->mapToPosition(m_anchor).y() - height - 120;
    setPos(x, y);

    QGraphicsTextItem::paint(painter, option, widget);
}

void PeakCallOut::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    event->setAccepted(true);
}

void PeakCallOut::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        setPos(mapToParent(event->pos() - event->buttonDownPos(Qt::LeftButton)));
        event->setAccepted(true);
    } else {
        event->setAccepted(false);
    }
}

void PeakCallOut::setText(const QString& text, const QPointF& point)
{
    m_anchor = point;
    m_text = tr("<h4>%1</h4>").arg(text);
    setHtml(m_text);
    QFontMetrics metrics(m_font);
    QTextDocument doc;
    doc.setHtml(m_text);
    m_textRect = metrics.boundingRect(QRect(0, 0, 250, 250), Qt::AlignLeft, m_text);
    prepareGeometryChange();
    m_rect = m_textRect.adjusted(0, 0, 0, 0);
    if (doc.size().width() > 60) {
        setRotation(-90);
        flip = true;
    }
}

void PeakCallOut::setAnchor(QPointF point)
{
    m_anchor = point;
}
