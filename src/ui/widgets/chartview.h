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

#pragma once

#include "src/ui/dialogs/chartconfig.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QtCore/QPointer>

class QPushButton;
class QChart;

class PeakCallOut;

struct ChartConfig;
struct PgfPlotConfig {
    QString colordefinition;
    QString plots;
    QStringList table;
};

class ChartViewPrivate : public QtCharts::QChartView {
    Q_OBJECT
public:
    inline ChartViewPrivate(QWidget* parent = Q_NULLPTR)
        : QtCharts::QChartView(parent)
    {
        setRubberBand(QChartView::RectangleRubberBand);
    }
    inline ChartViewPrivate(QtCharts::QChart* chart, QWidget* parent = Q_NULLPTR)
        : QtCharts::QChartView(parent)
    {
        setChart(chart);
        setAcceptDrops(true);
        setRenderHint(QPainter::Antialiasing, true);
        setRubberBand(QChartView::RectangleRubberBand);
        /*QFont font = chart->titleFont();
        font.setBold(true);
        font.setPointSize(12);
        chart->setTitleFont(font);*/
    }
    inline ~ChartViewPrivate() {}

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
};

class ChartView : public QWidget {
    Q_OBJECT
public:
    ChartView(QtCharts::QChart* chart, bool latex_supported = false);
    ChartView();
    ~ChartView();
    void addSeries(QPointer<QtCharts::QAbstractSeries>, bool callout = false);
    qreal YMax() const { return m_ymax; }
    inline void removeSeries(QtCharts::QAbstractSeries* series) { m_chart->removeSeries(series); }
    inline QList<QtCharts::QAbstractSeries*> series() const { return m_chart->series(); }

    QtCharts::QLineSeries* addLinearSeries(qreal m, qreal n, qreal min, qreal max);
    void ClearChart();
    inline void setModal(bool modal) { m_chartconfigdialog.setModal(modal); }

    inline qreal YMaxRange() const
    {
        if (m_hasAxis)
            return m_YAxis->max();
        else
            return 0;
    }
    inline qreal YMinRange() const
    {
        if (m_hasAxis)
            return m_YAxis->min();
        else
            return 0;
    }

    inline qreal XMaxRange() const
    {
        if (m_hasAxis)
            return m_XAxis->max();
        else
            return 0;
    }
    inline qreal XMinRange() const
    {
        if (m_hasAxis)
            return m_XAxis->min();
        else
            return 0;
    }

    inline void setXRange(qreal xmin, qreal xmax)
    {
        if (m_hasAxis) {
            m_XAxis->setMin(xmin);
            m_XAxis->setMax(xmax);
        }
    }

    inline void setYRange(qreal ymin, qreal ymax)
    {
        if (m_hasAxis) {
            m_YAxis->setMin(ymin);
            m_YAxis->setMax(ymax);
        }
    }

    inline void setName(const QString& name)
    {
        m_name = name;
        ReadSettings();
    }
public slots:
    void formatAxis();

    void setXAxis(const QString& str)
    {
        m_x_axis = str;
        emit AxisChanged();
    }
    void setYAxis(const QString& str)
    {
        m_y_axis = str;
        emit AxisChanged();
    }
    void setTitle(const QString& str);

private:
    QAction* m_lock_action;
    ChartViewPrivate* m_chart_private;
    QPointer<QtCharts::QChart> m_chart;
    QPushButton* m_config;
    void setUi();
    bool has_legend, connected, m_hasAxis = false;
    QString m_x_axis, m_y_axis;
    ChartConfig getChartConfig() const;
    PgfPlotConfig getScatterTable() const;
    PgfPlotConfig getLineTable() const;
    QString Color2RGB(const QColor& color) const;
    void WriteTable(const QString& str);
    ChartConfigDialog m_chartconfigdialog;
    bool m_pending, m_lock_scaling, m_latex_supported, m_modal = true;
    qreal m_ymax, m_ymin, m_xmin, m_xmax;
    QVector<QPointer<QtCharts::QAbstractSeries>> m_series;
    QVector<PeakCallOut*> m_peak_anno;

    ChartConfig m_last_config;
    void WriteSettings(const ChartConfig& chartconfig);
    ChartConfig ReadSettings();
    QString m_name;
    QPointer<QtCharts::QValueAxis> m_XAxis, m_YAxis;
private slots:
    void PlotSettings();
    void PrintPlot();
    void ExportLatex();
    //     void ExportGnuplot();
    void ExportPNG();
    void setChartConfig(const ChartConfig& chartconfig);
    void forceformatAxis();
    void ConfigurationChanged();

signals:
    void AxisChanged();
    void ChartCleared();
};
