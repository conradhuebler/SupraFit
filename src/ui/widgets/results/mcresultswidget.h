/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#ifndef MCRESULTSWIDGET_H
#define MCRESULTSWIDGET_H

#include <QtCore/QPointer>

#include <QtWidgets/QWidget>

#include <QtCharts/QAreaSeries>

#include "resultswidget.h"

class MonteCarloStatistics;
class AbstractModel;
class QPushButton;
class QDoubleSpinBox;
class QLabel;
class ChartView;

class MCResultsWidget : public ResultsWidget
{
    Q_OBJECT
    
public:
    MCResultsWidget(const QList<QJsonObject > &data, QSharedPointer< AbstractModel > model, const QList<QJsonObject > &models = QList<QJsonObject>());
    ~MCResultsWidget();
    
    void setModels(const QList<QJsonObject > &models) { m_models = models; }
    
private:
    QPushButton *m_switch, *m_save;
    QDoubleSpinBox *m_error;
    QPointer<ChartView > m_histgram, m_contour;
    QPointer<QtCharts::QChartView > m_box;
    QVector<QColor> m_colors;
    QVector<QtCharts::QAreaSeries * > m_area_series;
    
    virtual QWidget * ChartWidget() override;
    void WriteConfidence(const QList<QJsonObject > &constant_results) override;
    void UpdateBoxes();
    
    QtCharts::QAreaSeries *AreaSeries(const QColor &color) const;
    QPointer<ChartView > MakeHistogram();
    QPointer<ChartView > MakeContour();
    QPointer<QtCharts::QChartView > MakeBoxPlot();
    QList<QJsonObject> m_box_object;
    QList<QJsonObject > m_data, m_models;
    
private slots:
    void ExportResults();
    void GenerateConfidence(double error);
    
};

#endif // MCRESULTSWIDGET_H
