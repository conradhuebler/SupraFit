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

#include "src/capabilities/continuousvariation.h"
#include "src/core/AbstractModel.h"
#include "src/core/toolset.h"
#include "src/ui/widgets/chartview.h"
#include "src/ui/widgets/statisticwidget.h"

#include <QtCore/QPointer>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include "cvresultswidget.h"


CVResultsWidget::CVResultsWidget(QPointer<ContinuousVariation> statistics, QSharedPointer<AbstractTitrationModel> model, QWidget* parent): QWidget(parent), m_model(model), m_statistics(statistics)
{
    if(m_statistics)
        setUi();
}

CVResultsWidget::~CVResultsWidget()
{
    if(m_statistics)
        delete m_statistics;
}

void CVResultsWidget::setUi()
{
    CVPlot();
}


void CVResultsWidget::CVPlot()
{
    QList<QJsonObject > constant_results = m_statistics->Results();
    QList<QList<QPointF > >series = m_statistics->Series();
    QWidget *resultwidget = new QWidget;
    QGridLayout *layout = new QGridLayout;
    resultwidget->setLayout(layout);
    
    QtCharts::QChart *chart = new QtCharts::QChart;
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart);
    layout->addWidget(view, 0, 0, 1, 7);
    for(int i = 0; i < constant_results.size(); ++i)
    {
        QtCharts::QLineSeries *xy_series = new QtCharts::QLineSeries(this);
        xy_series->append(series[i]);
        view->addSeries(xy_series);
        m_model->setCVStatistic(constant_results[i], i);
        
        QtCharts::QLineSeries *current_constant= new QtCharts::QLineSeries();
        *current_constant << QPointF(m_model->Constant(i), m_model->SumofSquares()) << QPointF(m_model->Constant(i), m_model->SumofSquares()*1.1);
        current_constant->setColor(xy_series->color());
        view->addSeries(current_constant);
        
        QString text;
        if(i == 0)
        {
            text += "Maxsteps: " + QString::number(constant_results[i]["controller"].toObject()["steps"].toInt()) + "\t";
            text += "Increment = " + QString::number(constant_results[i]["controller"].toObject()["increment"].toDouble()) + "\t";
            text += "Max Error = " + QString::number(constant_results[i]["controller"].toObject()["maxerror"].toDouble()) + "\n";
        }
        text  += StatisticWidget::TextFromConfidence(constant_results[i]) + "\n";
        QLabel *label = new QLabel(text);
        label->setTextFormat(Qt::RichText);
        layout->addWidget(label, i + 1, 0);
    }
    setLayout(layout);
}


void CVResultsWidget::Ellipsoidal()
{
    QList<QJsonObject > constant_results = m_statistics->Results();
    QList<QList<QPointF > >series = m_statistics->Series();
    QWidget *resultwidget = new QWidget;
    QGridLayout *layout = new QGridLayout;
    resultwidget->setLayout(layout);
    
    QtCharts::QChart *chart = new QtCharts::QChart;
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart);
    layout->addWidget(view, 0, 0, 1, 7);
    QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries(this);
    xy_series->append(ToolSet::fromModelsList(m_statistics->Models()));
    xy_series->setMarkerSize(8);
    view->addSeries(xy_series);
}


void CVResultsWidget::ChangeConfidence()
{
}
