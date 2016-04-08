/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QGridLayout>

#include "chartwidget.h"

ChartWidget::ChartWidget()
{
    m_chart = new QtCharts::QChart;
    m_errorview = new QtCharts::QChart;
    m_chartwidget = new QtCharts::QChartView(m_chart);
    m_errorchart = new QtCharts::QChartView(m_errorview);
//     m_chartwidget->addChart(m_errorview);
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_chartwidget,0, 0);
    layout->addWidget(m_errorchart, 1, 0);
    setLayout(layout);
}

ChartWidget::~ChartWidget()
{
}

void ChartWidget::setChart(const QPointer< QtCharts::QChart > chart)
{
//      m_chart = chart; 
//      m_chartwidget->addChart(m_chart);
}

void ChartWidget::addSeries(QPointer< QtCharts::QScatterSeries > series, const QString& str)
{
    m_chart->addSeries(series);
    m_chart->setTitle(str);
    m_chart->createDefaultAxes();
    
//     foreach (QtCharts::QChartView *chart, m_chartwidget)
//         chart->setRenderHint(QPainter::Antialiasing, true);
    m_chartwidget->setRenderHint(QPainter::Antialiasing, true);
    m_chartwidget->chart()->legend()->setAlignment(Qt::AlignRight);
}

void ChartWidget::addLineSeries(QPointer< QtCharts::QLineSeries > series, const QString& str)
{
    
    m_chart->addSeries(series);
     m_chart->setTitle(str);
     m_chart->createDefaultAxes();
//     
// //     foreach (QtCharts::QChartView *chart, m_chartwidget)
// //         chart->setRenderHint(QPainter::Antialiasing, true);
     m_chartwidget->setRenderHint(QPainter::Antialiasing, true);
     m_chartwidget->chart()->legend()->setAlignment(Qt::AlignRight);
}
void ChartWidget::addErrorSeries(QPointer< QtCharts::QLineSeries > series, const QString& str)
{
     

   m_errorview->addSeries(series);
     m_errorview->setTitle(str);
     m_errorview->createDefaultAxes(); 
}

void ChartWidget::clearErrrorPlot()
{
m_errorview->removeAllSeries();
}


void ChartWidget::clearPlot()
{
    m_chart->removeAllSeries();
    
}


#include "chartwidget.moc"
