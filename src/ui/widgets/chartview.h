/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QtCore/QPointer>
class QPushButton;
class QChart;

struct ChartConfig;
struct PgfPlotConfig
{
    QString colordefinition;
    QString plots;
    QStringList table;
};

class ChartViewPrivate : public QtCharts::QChartView
{
  Q_OBJECT
public:
    inline ChartViewPrivate(QWidget *parent = Q_NULLPTR) : QtCharts::QChartView(parent) {setRubberBand(QChartView::RectangleRubberBand);}
    inline ChartViewPrivate(QtCharts::QChart *chart, QWidget *parent = Q_NULLPTR) : QtCharts::QChartView(parent)  {setChart(chart); setAcceptDrops(true); setRenderHint(QPainter::Antialiasing, true);setRubberBand(QChartView::RectangleRubberBand);}
    inline ~ChartViewPrivate(){ }
    
protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent * event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
private:

};


class ChartView : public QWidget
{
    Q_OBJECT
public:
    ChartView(QtCharts::QChart *chart, bool latex_supported = false);
    ChartView();
    inline ~ChartView() { }
    void addSeries( QPointer<QtCharts::QAbstractSeries>);
    qreal YMax() const { return m_ymax; }
    inline void removeSeries(QtCharts::QAbstractSeries *series) { m_chart->removeSeries(series); }
    inline QList<QtCharts::QAbstractSeries *> series() const { return m_chart->series(); }
    
    QtCharts::QLineSeries *addLinearSeries(qreal m, qreal n, qreal min, qreal max);
    void ClearChart();
    
public slots:
    void formatAxis();
    
    void setXAxis(const QString &str) { m_x_axis = str; emit AxisChanged(); }
    void setYAxis(const QString &str) { m_y_axis = str; emit AxisChanged(); }
    
private:
    ChartViewPrivate *m_chart_private;
    QPointer< QtCharts::QChart > m_chart;
    QPushButton *m_config;
    void setUi();
    bool has_legend, connected;
    QString m_x_axis, m_y_axis;
    ChartConfig getChartConfig() const;
    PgfPlotConfig getScatterTable() const;
    PgfPlotConfig getLineTable() const;
    QString Color2RGB(const QColor &color) const;
    void WriteTable(const QString &str);
    ChartConfigDialog m_chartconfigdialog;
    bool m_pending, m_lock_scaling, m_latex_supported;   
    qreal m_ymax, m_ymin, m_xmin, m_xmax;

private slots:
    void PlotSettings();
    void PrintPlot();
    void ExportLatex();
//     void ExportGnuplot();
    void ExportPNG();
    void setChartConfig(const ChartConfig &chartconfig);
    void forceformatAxis();
    void ConfigurationChanged();
    
signals:
    void AxisChanged();
    void ChartCleared();
};

#endif // CHARTVIEW_H
