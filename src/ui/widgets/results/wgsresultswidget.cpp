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
#include "src/capabilities/weakenedgridsearch.h"
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
#include "wgsresultswidget.h"


WGSResultsWidget::WGSResultsWidget(QPointer<AbstractSearchClass> statistics, QSharedPointer<AbstractModel> model, QWidget* parent)//: QWidget(parent), m_model(model), m_statistics(statistics)
{
    m_statistics = statistics;
    m_model = model;
    if(!m_statistics)
        throw 1;
    setUi();
}

WGSResultsWidget::~WGSResultsWidget()
{

}

QWidget * WGSResultsWidget::ChartWidget()
{
    QWidget *widget = new QWidget;
    QGridLayout *layout = new QGridLayout;

    if(qobject_cast<WeakenedGridSearch *>(m_statistics))
        layout->addWidget(CVPlot());
    else
        layout->addWidget(MoCoPlot());
    
    widget->setLayout(layout);
    return widget;
}

void WGSResultsWidget::WriteConfidence(const QList<QJsonObject > &constant_results)
{
    QString text;
    QJsonObject controller = constant_results.first()["controller"].toObject();
    if(constant_results.first()["method"].toString() == "model comparison")
    {
        text += "<h3>Model Comparison</h3";
        text += "<p><b>Monte Carlo Steps </b>: " + QString::number(controller["steps"].toInt()) + "</p>";
        text += "<p><b>Area of the ellipse</b> is " + QString::number(constant_results.first()["moco_area"].toDouble()) + "</p>";
    }
    else
    {
        text += "<h3>Weakened Grid Search</h3>";
        text += "<p>Maxsteps</b> : " + QString::number(controller["steps"].toInt()) + "</p>"; 
        text += "<p>Increment for each step is " + QString::number(controller["increment"].toDouble()) + "</p>";
    }
            
    text += "<p><b>Fisher Statistic</b> ";
    if(controller["fisher"].toBool())
        text += "was used. F-value is " + QString::number(controller["f-value"].toDouble());
    else
        text += "was not used";
    text += "</p>";
    text += "The maximal error is " + QString::number(controller["maxerror"].toDouble());
        
    for(int i = 0; i < constant_results.size(); ++i)
    {
        if(constant_results[i].contains("moco_area"))
            m_model->setMoCoStatistic(constant_results[i], i);
        else
            m_model->setCVStatistic(constant_results[i], i);
        if(i < m_model->GlobalParameterSize())
        {
            QJsonObject confidenceObject = constant_results[i]["confidence"].toObject();
            text  += Print::TextFromConfidence(constant_results[i], m_model.data()) + "\n";
        }
    }
    m_confidence_label->setText(text);
}

ChartView * WGSResultsWidget::CVPlot()
{
    QList<QJsonObject > constant_results = m_statistics->Results();
    QList<QList<QPointF > >series = m_statistics->Series();
    QtCharts::QChart *chart = new QtCharts::QChart;
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart);
    view->setXAxis("constant");
    view->setYAxis("Sum of Squares");
    WriteConfidence(constant_results);
    for(int i = 0; i < constant_results.size(); ++i)
    {
        QtCharts::QLineSeries *xy_series = new QtCharts::QLineSeries;
        xy_series->append(series[i]);
        view->addSeries(xy_series);
        
        LineSeries *current_constant= new LineSeries;
        *current_constant << QPointF(m_model->Constant(i), m_model->SumofSquares()) << QPointF(m_model->Constant(i), m_model->SumofSquares()*1.1);
        current_constant->setColor(xy_series->color());
        current_constant->setName("K<sub>" + m_model->ConstantNames()[i].remove(":") + "</sub>");
        view->addSeries(current_constant, true);
    }
    return view;
}

ChartView *  WGSResultsWidget::MoCoPlot()
{
    QList<QJsonObject > constant_results = m_statistics->Results();
    WriteConfidence(constant_results);
    QList<QList<QPointF > >series = m_statistics->Series();
    
    QtCharts::QChart *chart = new QtCharts::QChart;
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart);
    QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries;
    xy_series->append(ToolSet::fromModelsList(m_statistics->Models()));
    xy_series->setMarkerSize(6);
    xy_series->setName("MC Results");
    view->addSeries(xy_series, true);
    int i = 0;
    for(const QList<QPointF> &serie : qAsConst(series))
    {
        LineSeries *xy_serie = new LineSeries;
        xy_serie->append(serie);
        xy_serie->setName("K<sub>" + m_model->ConstantNames()[i].remove(":") + "</sub>");
        view->addSeries(xy_serie, true);
        ++i;
    }
    view->setXAxis("K<sub>" + m_model->ConstantNames()[0].remove(":") + "</sub>");
    view->setYAxis("K<sub>" + m_model->ConstantNames()[1].remove(":") + "</sub>");
    return view;
}
