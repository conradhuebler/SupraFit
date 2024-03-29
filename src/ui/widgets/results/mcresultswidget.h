/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>

#include <QtWidgets/QWidget>

#include <QtCharts/QAreaSeries>

#include "src/global.h"

#include "resultswidget.h"

class ChartWrapper;
class MonteCarloStatistics;
class AbstractModel;
class QPushButton;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class ChartView;
class ListChart;
class LineSeries;

class QJsonObject;

class MCResultsWidget : public QWidget {
    Q_OBJECT

public:
    MCResultsWidget(const QJsonObject& data, QSharedPointer<AbstractModel> model, ChartWrapper* wrapper, const QList<QJsonObject>& models = QList<QJsonObject>());
    ~MCResultsWidget();

    void setModels(const QList<QJsonObject>& models) { m_models = models; }
    inline bool hasData() const { return has_boxplot || has_scatter || has_histogram; }
    void setUi();

private:
    QPushButton *m_switch, *m_save;
    QDoubleSpinBox* m_error;
    QPointer<ListChart> m_histgram, m_box, m_series_chart;
    QPointer<QWidget> m_scatter;
    QVector<QColor> m_colors;
    QVector<QAreaSeries*> m_area_series;

    void UpdateBoxes();
    void setAreaColor(int index, const QColor& color);

    QAreaSeries* AreaSeries(const QColor& color) const;
    QSpinBox* m_bins;
    QPointer<ListChart> MakeHistogram();
    QPointer<QWidget> MakeScatter();
    QPointer<ListChart> MakeBoxPlot();
    QPointer<ListChart> MakeSeriesChart();

    QList<QJsonObject> m_box_object;
    QList<QJsonObject> m_models;
    bool has_histogram, has_scatter, has_boxplot;
    SupraFit::Method m_type;

    ChartWrapper* m_wrapper;
    QWeakPointer<AbstractModel> m_model;
    QJsonObject m_data;

    QHash<LineSeries*, QVector<qreal>> m_linked_data;

private slots:
    void ExportResults();
    void GenerateConfidence(double error);
    void GenerateHistogram();

signals:
    void ConfidenceUpdated(QJsonObject data);
};
