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

#include "src/ui/dialogs/chartconfig.h"

#include <QtCharts/QValueAxis>
#include <QtCharts/QChartView>
#include <QtCore/QPointer>
class QPushButton;
class QChart;

struct ChartConfig;


class ChartViewPrivate : public QtCharts::QChartView
{
  Q_OBJECT
public:
    inline ChartViewPrivate(QWidget *parent = Q_NULLPTR) : QtCharts::QChartView(parent) {setRubberBand(QChartView::RectangleRubberBand);}
    inline ChartViewPrivate(QtCharts::QChart *chart, QWidget *parent = Q_NULLPTR) : QtCharts::QChartView(parent)  {setChart(chart); setAcceptDrops(true); setRenderHint(QPainter::Antialiasing, true);setRubberBand(QChartView::RectangleRubberBand);}
    inline ~ChartViewPrivate(){ };
        
protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent * event) override;
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
    ChartView();
    inline ~ChartView() { }
    void addSeries( QtCharts::QAbstractSeries* series, bool legend = false );
    
public slots:
    void formatAxis();
    
    void setXAxis(const QString &str) { m_x_axis = str; emit AxisChanged(); }
    void setYAxis(const QString &str) { m_y_axis = str; emit AxisChanged(); };
    
private:
    ChartViewPrivate *m_chart_private;
    QPointer< QtCharts::QChart > m_chart;
    QPushButton *m_config;
    void setUi();
    bool has_legend, connected;
    QString m_x_axis, m_y_axis;
    ChartConfig getChartConfig() const;
    
    ChartConfigDialog m_chartconfigdialog;
    bool m_pending, m_lock_scaling;
    qreal ceil(qreal value);
    qreal floor(qreal value);
    qreal scale(qreal value, qreal &pow);
    qreal scale(qreal value);
    
    
private slots:
    void PlotSettings();
    void PrintPlot();
    void ExportLatex();
    void ExportGnuplot();
    void setChartConfig(const ChartConfig &chartconfig);
    void forceformatAxis();
    
signals:
    void AxisChanged();
};

#endif // CHARTVIEW_H
