/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#pragma once

#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QLineSeries>

#include <QtWidgets/QDialog>

class QPushButton;
class QSpinBox;
class QTextEdit;

class ChartWrapper;
class DataClass;
class ListChart;

/**
 * @todo write docs
 */
class RegressionAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    RegressionAnalysisDialog(QWeakPointer<DataClass > data,  QWeakPointer<ChartWrapper> wrapper, QWidget *parent = 0);
    ~RegressionAnalysisDialog();
    void UpdatePlots();
    
private:
    void setUI();
    QPushButton *m_fit;
    QSpinBox *m_functions;
    QWeakPointer<DataClass > m_data;
    QWeakPointer<ChartWrapper> m_wrapper;
    ListChart *m_chart;
    QList<QPointer<QtCharts::QScatterSeries > > m_series;
    QMultiHash<int, QtCharts::QLineSeries *> m_linear_series;
    QTextEdit *m_output;
    
private slots:
    void FitFunctions();
};
