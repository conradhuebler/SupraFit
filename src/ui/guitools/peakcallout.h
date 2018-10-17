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

#pragma once

#include <QtCore/QPointer>
#include <QtGui/QFont>

#include <QtWidgets/QGraphicsItem>

#include <QtCharts/QChartGlobal>
#include <QtCharts/QChartView>

class QGraphicsSceneMouseEvent;

class PeakCallOut : public QGraphicsTextItem {
public:
    PeakCallOut(QPointer<QtCharts::QChart> parent);

    void setText(const QString& text, const QPointF& point);
    void setAnchor(QPointF point);
    // void updateGeometry();

    virtual QRectF boundingRect() const override;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    inline void setSeries(const QPointer<QtCharts::QAbstractSeries> serie) { m_serie = serie; }

public slots:
    void setColor(const QColor& color);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QString m_text, m_htmltext;
    QRectF m_textRect;
    QRectF m_rect;
    QPointF m_anchor, m_text_position;
    QPointer<QtCharts::QChart> m_chart;
    bool flip = false;
    QColor m_color;

    void Update();
    QPointer<QtCharts::QAbstractSeries> m_serie;
};
