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
#include <QtCharts/QValueAxis>

#include <QtGui/qwidget.h>
#include <QtCore/QPointer>
#include <QtCore/QVector>
class AbstractTitrationModel;
class QComboBox;
class QPushButton;
class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    ChartWidget();
    ~ChartWidget();
    void setRawData(const QPointer<DataClass> rawdata); 
    
    

public slots:
    void addSeries(QtCharts::QScatterSeries *series, const QString& str = "Signal");
    void addLineSeries(const QPointer< QtCharts::QLineSeries >  &series, const QString& str = "Signal");
    void addErrorSeries(const QPointer< QtCharts::QLineSeries >  &series, const QString& str = "Signal");
    void addModel(const QPointer< AbstractTitrationModel > model);
private:
    void formatAxis();
    void formatErrorAxis();
    QPointer<QComboBox > createThemeBox() const;
   
    QPointer<QComboBox > m_x_scale, m_themebox;
    QPointer<QtCharts::QChartView > m_chartwidget, m_errorchart;
    QPointer<QtCharts::QChart > m_chart, m_errorview;
    QPointer<QtCharts::QValueAxis > m_x_chart, m_y_chart, m_x_error, m_y_error;
    QVector< QPointer<AbstractTitrationModel > > m_models;
    QPointer<DataClass > m_rawdata;
    QVector< QVector <int > > m_titration_curve, m_model_curve, m_error_curve;
    qreal m_y_max_chart, m_y_min_error, m_y_max_error, m_x_max_chart, m_x_max_error, m_y_min_chart;
    QSharedPointer<QtCharts::QLineSeries > m_error_axis;
    void Paint();
private slots:
    void Repaint();
    void updateUI();
};

#endif // CHARTWIDGET_H
