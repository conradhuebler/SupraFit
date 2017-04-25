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
#include "src/capabilities/abstractsearchclass.h"
#include "src/capabilities/continuousvariation.h"
#include "src/core/AbstractModel.h"
#include "src/core/toolset.h"

#include "src/ui/guitools/chartwrapper.h"

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


CVResultsWidget::CVResultsWidget(QPointer<AbstractSearchClass> statistics, QSharedPointer<AbstractTitrationModel> model, QWidget* parent): QWidget(parent), m_model(model), m_statistics(statistics)
{
    if(!m_statistics)
        throw 1;
    setUi();
}

CVResultsWidget::~CVResultsWidget()
{
    if(m_statistics)
        delete m_statistics;
}

void CVResultsWidget::setUi()
{
    QGridLayout *layout = new QGridLayout;

    m_confidence_label = new QLabel();
    m_confidence_label->setTextFormat(Qt::RichText);
    layout->addWidget(m_confidence_label, 1, 0);
    
    if(qobject_cast<ContinuousVariation *>(m_statistics))
        m_view = CVPlot();
    else
        m_view = EllipsoidalPlot();
    
    layout->addWidget(m_view, 0, 0);
    setLayout(layout);
}

void CVResultsWidget::WriteConfidence(const QList<QJsonObject > &constant_results)
{
    QString text;
    
    QJsonObject controller = constant_results.first()["controller"].toObject();
    text += "Maxsteps: " + QString::number(controller["steps"].toInt()) + "\t";
    text += "Increment = " + QString::number(controller["increment"].toDouble()) + "\t";
    text += "Max Error = " + QString::number(controller["maxerror"].toDouble()) + "\n";
    
    for(int i = 0; i < constant_results.size(); ++i)
    {
        if(constant_results[i].contains("moco_area"))
            m_model->setMoCoStatistic(constant_results[i], i);
        else
            m_model->setCVStatistic(constant_results[i], i);
        if(i < m_model->ConstantSize())
        {
            QJsonObject confidenceObject = constant_results[i]["confidence"].toObject();
            text  += StatisticWidget::TextFromConfidence(constant_results[i]) + "\n";
        }
    }
    m_confidence_label->setText(text);
}

ChartView * CVResultsWidget::CVPlot()
{
    QList<QJsonObject > constant_results = m_statistics->Results();
    QList<QList<QPointF > >series = m_statistics->Series();
    QtCharts::QChart *chart = new QtCharts::QChart;
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart);
    WriteConfidence(constant_results);
    for(int i = 0; i < constant_results.size(); ++i)
    {
        QtCharts::QLineSeries *xy_series = new QtCharts::QLineSeries(this);
        xy_series->append(series[i]);
        view->addSeries(xy_series);
        
        QtCharts::QLineSeries *current_constant= new QtCharts::QLineSeries();
        *current_constant << QPointF(m_model->Constant(i), m_model->SumofSquares()) << QPointF(m_model->Constant(i), m_model->SumofSquares()*1.1);
        current_constant->setColor(xy_series->color());
        view->addSeries(current_constant);
    }
    return view;
}

ChartView *  CVResultsWidget::EllipsoidalPlot()
{
    QList<QJsonObject > constant_results = m_statistics->Results();
    WriteConfidence(constant_results);
    QList<QList<QPointF > >series = m_statistics->Series();
    
    QtCharts::QChart *chart = new QtCharts::QChart;
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart);
    QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries(this);
    xy_series->append(ToolSet::fromModelsList(m_statistics->Models()));
    xy_series->setMarkerSize(7);
    view->addSeries(xy_series);
    for(const QList<QPointF> &serie : qAsConst(series))
    {
        LineSeries *xy_serie = new LineSeries;
        xy_serie->append(serie);
        view->addSeries(xy_serie);
    }
    return view;
}
