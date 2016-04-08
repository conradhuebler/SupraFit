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

#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QLineSeries>

#include <QtGui/qwidget.h>
#include <QtCore/QPointer>


class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    ChartWidget();
    ~ChartWidget();
    void clearPlot();
    void clearErrrorPlot();
public slots:
    void setChart(const QPointer<QtCharts::QChart > chart);
    void addSeries(QPointer< QtCharts::QScatterSeries >  series, const QString& str = "Signal");
    void addLineSeries(QPointer< QtCharts::QLineSeries >  series, const QString& str = "Signal");
    void addErrorSeries(QPointer< QtCharts::QLineSeries >  series, const QString& str = "Signal");
    
private:
    QPointer<QtCharts::QChartView > m_chartwidget, m_errorchart;
    QPointer<QtCharts::QChart > m_chart, m_errorview;
};

#endif // CHARTWIDGET_H
