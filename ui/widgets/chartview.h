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

#ifndef CHARTVIEW_H
#define CHARTVIEW_H
#include <QtCharts/QChartView>

class QPushButton;
class QChart;

class ChartViewPrivate : public QtCharts::QChartView
{
  Q_OBJECT
public:
    ChartViewPrivate(QWidget *parent = Q_NULLPTR) : QtCharts::QChartView(parent) {}
    ChartViewPrivate(QtCharts::QChart *chart, QWidget *parent = Q_NULLPTR) : QtCharts::QChartView(parent)  {setChart(chart); setAcceptDrops(true); setRenderHint(QPainter::Antialiasing, true);}
    ~ChartViewPrivate(){ };
        
protected:
    void mousePressEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void keyPressEvent(QKeyEvent *event);
private:
    
};


class ChartView : public QWidget
{
    Q_OBJECT
public:
    ChartView(QtCharts::QChart *chart);
private:
    ChartViewPrivate *m_chart_private;
    QPushButton *m_config;
    void setUi();
    
private slots:
    void PlotSettings();
    void PrintPlot();
    void ExportLatex();
    void ExportGnuplot();
//     QVector<Q
};

#endif // CHARTVIEW_H
