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
 * 
 */

#pragma once

#include <QtCore/QPointer>

#include <QtWidgets/QWidget>

#include <QtCharts/QAreaSeries>

#include "resultswidget.h"

class ChartWrapper;
class MonteCarloStatistics;
class AbstractModel;
class QPushButton;
class QDoubleSpinBox;
class QLabel;
class ChartView;
class ListChart;

class MCResultsWidget : public ResultsWidget
{
    Q_OBJECT
    
public:
    
    enum Type {
        MonteCarlo = 1,
        CrossValidation = 2
    };
    
    MCResultsWidget(const QList<QJsonObject > &data, QSharedPointer< AbstractModel > model, ChartWrapper *wrapper, const QList<QJsonObject > &models = QList<QJsonObject>(), Type type = MonteCarlo);
    ~MCResultsWidget();
    
    void setModels(const QList<QJsonObject > &models) { m_models = models; }
    inline bool hasData() const { return has_boxplot || has_contour || has_histogram; }
    

    
private:
    QPushButton *m_switch, *m_save;
    QDoubleSpinBox *m_error;
    QPointer<ListChart > m_histgram, m_box;
    QPointer<ChartView> m_contour;
    QVector<QColor> m_colors;
    QVector<QtCharts::QAreaSeries * > m_area_series;
    
    virtual QWidget * ChartWidget() override;
    void WriteConfidence(const QList<QJsonObject > &constant_results) override;
    void UpdateBoxes();
    
    QtCharts::QAreaSeries *AreaSeries(const QColor &color) const;
    QPointer<ListChart > MakeHistogram();
    QPointer<ChartView > MakeContour();
    QPointer<ListChart > MakeBoxPlot();
    QList<QJsonObject> m_box_object;
    QList<QJsonObject > m_data, m_models;
    bool has_histogram, has_contour, has_boxplot;
    Type m_type;
    
    ChartWrapper *m_wrapper;

private slots:
    void ExportResults();
    void GenerateConfidence(double error);
    
};
