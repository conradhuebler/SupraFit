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

class MonteCarloStatistics;
class AbstractTitrationModel;
class QPushButton;
class QDoubleSpinBox;

class ChartView;

class MCResultsWidget : public QWidget
{
    Q_OBJECT
    
public:
    MCResultsWidget(QPointer<MonteCarloStatistics > statistics, QSharedPointer<AbstractTitrationModel> model, QWidget *parent = 0);
    ~MCResultsWidget();
    
private:
    QPointer<MonteCarloStatistics> m_statistics;
    QSharedPointer< AbstractTitrationModel > m_model;
    
    QPushButton *m_switch, *m_save;
    QDoubleSpinBox *m_error;
    QPointer<ChartView > m_histgram, m_contour;
    QVector<QColor> m_colors;
    void setUi();
    
    QtCharts::QAreaSeries *AreaSeries(const QJsonObject &confidence, const QColor &color, double max) const;
    void ClearArea(); 
    QPointer<ChartView > MakeHistogram(QString &confidence);
    QPointer<ChartView > MakeContour();
    
private slots:
    void ChangeConfidence();
    void ExportResults();
    void SwitchView();
    
};

#endif // MCRESULTSWIDGET_H
