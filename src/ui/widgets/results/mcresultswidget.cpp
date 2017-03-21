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

#include "src/capabilities/montecarlostatistics.h"
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

#include "mcresultswidget.h"

MCResultsWidget::MCResultsWidget(QPointer<MonteCarloStatistics > statistics, QSharedPointer<AbstractTitrationModel> model, QWidget* parent) : QWidget(parent), m_model(model), m_statistics(statistics)
{
    if(m_statistics)
        setUi();
}


MCResultsWidget::~MCResultsWidget()
{
    if(m_statistics)
        delete m_statistics;
}

void MCResultsWidget::setUi()
{
    QList<QList<QPointF > >series = m_statistics->getSeries();
    QList<QJsonObject > constant_results = m_statistics->getResult();
    QWidget *resultwidget = new QWidget;
    QGridLayout *layout = new QGridLayout;
    resultwidget->setLayout(layout);
    
    QtCharts::QChart *chart = new QtCharts::QChart;
    
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart);
    layout->addWidget(view, 0, 0, 1, 7);
    bool formated = false;
    for(int i = 0; i < constant_results.size(); ++i)
        m_model->setMCStatistic(constant_results[i], i);
    for(int i = 0; i < series.size(); ++i)
    {
        QtCharts::QLineSeries *xy_series = new QtCharts::QLineSeries(this);
        xy_series->append(series[i]);
        view->addSeries(xy_series);
        if(!formated)
            view->formatAxis();
        formated = true;
        
        
        QtCharts::QLineSeries *current_constant= new QtCharts::QLineSeries();
        *current_constant << QPointF(m_model->Constant(i), 0) << QPointF(m_model->Constant(i), view->YMax());
        current_constant->setColor(xy_series->color());
        view->addSeries(current_constant);
        QtCharts::QLineSeries *series1 = new QtCharts::QLineSeries();
        QtCharts::QLineSeries *series2 = new QtCharts::QLineSeries();
        
        QJsonObject confidence = constant_results[i]["confidence"].toObject();
        *series1 << QPointF(confidence["lower_5"].toVariant().toDouble(), 0) << QPointF(confidence["lower_5"].toVariant().toDouble(), view->YMax());
        *series2 << QPointF(confidence["upper_5"].toVariant().toDouble(), 0) << QPointF(confidence["upper_5"].toVariant().toDouble(), view->YMax());
        QtCharts::QAreaSeries *area_series = new QtCharts::QAreaSeries(series1, series2);
        QPen pen(0x059605);
        pen.setWidth(3);
        area_series->setPen(pen);
        
        QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
        gradient.setColorAt(0.0, xy_series->color());
        gradient.setColorAt(1.0, 0x26f626);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        area_series->setBrush(gradient);
        area_series->setOpacity(0.4);
        view->addSeries(area_series);
        
        QString text;
        if(i == 0)
        {
            text += "MC Steps: " + QString::number(constant_results[i]["controller"].toObject()["steps"].toInt()) + "\t";
            if(constant_results[i]["controller"].toObject()["bootstrap"].toBool())
                text += "Bootstrapped ";
            else
                text += "Variance = " + QString::number(constant_results[i]["controller"].toObject()["variance"].toDouble()) + " ";
            
            if(constant_results[i]["controller"].toObject()["original"].toBool())
                text += "operated on original data\n";
            else
                text += "operated on modelled data\n";
        }
        text  += StatisticWidget::TextFromConfidence(constant_results[i]) + "\n";
        QLabel *label = new QLabel(text);
        label->setTextFormat(Qt::RichText);
        layout->addWidget(label, i + 1, 0);
        
    }
    setLayout(layout);
}

void MCResultsWidget::ShowContour()
{
    QList<QPointF > data = ToolSet::fromModelsList(m_statistics->Models());
    QWidget *resultwidget_ellipsoid = new QWidget;
    QGridLayout *layout_ellipsoid = new QGridLayout;
    resultwidget_ellipsoid->setLayout(layout_ellipsoid);
    QtCharts::QChart *chart_ellipsoid = new QtCharts::QChart;
    chart_ellipsoid->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart_ellipsoid);
    layout_ellipsoid->addWidget(view, 0, 0, 1, 7);
    QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries(this);
    xy_series->append(data);
    xy_series->setMarkerSize(8);
    view->addSeries(xy_series);
//     m_statistic_result->setWidget(resultwidget_ellipsoid, "Monte Carlo Simulation for " + m_model->Name());
}

void MCResultsWidget::ChangeConfidence()
{
}

