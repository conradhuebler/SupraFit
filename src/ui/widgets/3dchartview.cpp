/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/QSurface3DSeries>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>

#include "3dchartview.h"

_3DChartViewPrivate::_3DChartViewPrivate() 
{
    
    
}

_3DChartViewPrivate::~_3DChartViewPrivate()
{

}


_3DChartView::_3DChartView(QWidget *parent) : QWidget(parent), d(new _3DChartViewPrivate()), has_series(false), Max_Z(0), Max_X(0), Max_Y(0), Min_Z(0), Min_X(0), Min_Y(0)
{
    layer = QWidget::createWindowContainer(d, this);
    m_config = new QPushButton(tr("Config"));
    m_config->setFlat(true);
        m_config->setIcon(QIcon::fromTheme("applications-system"));
        m_config->setMaximumWidth(100);
        m_config->setStyleSheet("QPushButton {background-color: #A3C1DA; color: black;}");
    QGridLayout *layout = new QGridLayout;
    
    layout->addWidget(layer, 0, 0, 1, 5);//, Qt::AlignHCenter);
    layout->addWidget(m_config, 0, 4, Qt::AlignTop);
    setLayout(layout);
}

_3DChartView::~_3DChartView()
{
//     delete m_series;
}


void _3DChartView::setData(const QtDataVisualization::QSurfaceDataArray& data)
{
    if(has_series)
        d->removeSeries(m_series);
    m_data = new QtDataVisualization::QSurfaceDataArray(data);
    m_series = new QtDataVisualization::QSurface3DSeries;
    m_series->dataProxy()->resetArray(m_data);
    m_series->setDrawMode(QtDataVisualization::QSurface3DSeries::DrawSurfaceAndWireframe);
    d->addSeries(m_series);   
    has_series = true;
    CreateChart();
    ApplayRanges();
}

void _3DChartView::CreateChart()
{
        d->axisX()->setLabels(QStringList() << "xaxis");
        d->axisX()->setLabelFormat("%.2f");
        d->axisZ()->setLabelFormat("%.2f");
        d->axisX()->setLabelAutoRotation(30);
        d->axisY()->setLabelAutoRotation(90);
        d->axisZ()->setLabelAutoRotation(30);
        QLinearGradient gr;
        gr.setColorAt(0.0, Qt::black);
        gr.setColorAt(0.33, Qt::blue);
        gr.setColorAt(0.67, Qt::red);
        gr.setColorAt(1.0, Qt::yellow);
        m_series->setBaseGradient(gr);
        m_series->setColorStyle(QtDataVisualization::Q3DTheme::ColorStyleRangeGradient);
}

void _3DChartView::ApplayRanges()
{
    if(!has_series)
        return;
    d->axisX()->setRange(Min_X, Max_X);
    d->axisY()->setRange(Min_Z, Max_Z);
    d->axisZ()->setRange(Min_Y, Max_Y);
}


#include "3dchartview.moc"
