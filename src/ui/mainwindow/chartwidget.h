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

#include "src/ui/guitools/chartwrapper.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QLineSeries>

#include <QtCharts/QValueAxis>

#include <QtWidgets/QWidget>
#include <QtCore/QPointer>
#include <QtCore/QVector>
#include "src/ui/widgets/chartview.h"

class AbstractModel;
class QComboBox;
class QPushButton;
class QChartView;
class ChartWrapper;

struct Charts{
    ChartWrapper *error_wrapper;
    ChartWrapper *signal_wrapper;
    ChartWrapper *data_wrapper;
};

class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    ChartWidget();
    ~ChartWidget();
    QSharedPointer<ChartWrapper > setRawData(QSharedPointer<DataClass> rawdata); 
    Charts addModel(QSharedPointer< AbstractModel > model);
    
private:
    qreal max_shift, min_shift;
       
    QPointer<QComboBox > m_x_scale, m_themebox;
    QPointer<ChartView > m_signalview, m_errorview;
    QPointer<QtCharts::QChart > m_signalchart, m_errorchart;
    QPointer<QtCharts::QValueAxis > m_x_chart, m_y_chart, m_x_error, m_y_error;
    QVector< QWeakPointer<AbstractModel > > m_models;
    QWeakPointer<DataClass > m_rawdata;
    QVector< QVector <int > > m_titration_curve, m_model_curve, m_error_curve;
    QPair<qreal, qreal > Series2MinMax(const QtCharts::QXYSeries *series);
    void Paint();
    ChartWrapper::PlotMode m_plot_mode;
    QSharedPointer<ChartWrapper > m_data_mapper;
    
private slots:
    void formatAxis();
    void Repaint();
    void updateUI();
    void stopAnimiation();
    void restartAnimation();
};

#endif // CHARTWIDGET_H
