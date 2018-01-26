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

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include "wgsresultswidget.h"


WGSResultsWidget::WGSResultsWidget(const QJsonObject &data, QSharedPointer<AbstractModel> model, QWidget* parent) //: m_data(data)
{
    m_data = data;
    m_model = model;
    has_data = false;
    setUi();
}

WGSResultsWidget::~WGSResultsWidget()
{

}

QWidget * WGSResultsWidget::ChartWidget()
{
    QWidget *widget = new QWidget;
    QGridLayout *layout = new QGridLayout;

     if(m_data["controller"].toObject()["method"].toInt() == SupraFit::Statistic::WeakenedGridSearch)
         layout->addWidget(WGPlot());
     else
        layout->addWidget(MoCoPlot());
    
    widget->setLayout(layout);
    return widget;
}

void WGSResultsWidget::WriteConfidence(const QJsonObject  &result)
{
    /*
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
        m_model->UpdateStatistic(result);
        if(i < m_model->GlobalParameterSize())
        {
            QJsonObject confidenceObject = constant_results[i]["confidence"].toObject();
            text  += Print::TextFromConfidence(constant_results[i], m_model.data()) + "\n";
        }
    }
    m_confidence_label->setText(text);*/
}

ChartView * WGSResultsWidget::WGPlot()
{
    QtCharts::QChart *chart = new QtCharts::QChart;
    if(qApp->instance()->property("chartanimation").toBool())
        chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    chart->setTheme((QtCharts::QChart::ChartTheme) qApp->instance()->property("charttheme").toInt());
    ChartView *view = new ChartView(chart);
    view->setXAxis("constant");
    view->setYAxis("Sum of Squares");
    WriteConfidence(m_data);
    for(int i = 0; i < m_data.count() - 1; ++i)
    {
        QJsonObject data = m_data[QString::number(i)].toObject();
        if(data.isEmpty())
            continue;
        
        QtCharts::QLineSeries *xy_series = new QtCharts::QLineSeries;
        QList<qreal > x = ToolSet::String2DoubleList( data["data"].toObject()["x"].toString() );
        QList<qreal > y = ToolSet::String2DoubleList( data["data"].toObject()["y"].toString() );
        for(int j = 0; j < x.size(); ++j)
            xy_series->append(QPointF(x[j], y[j]));
        if(x.size())
            has_data = true;
        view->addSeries(xy_series);
        
        LineSeries *current_constant= new LineSeries;
        *current_constant << QPointF(m_model->GlobalParameter(i), m_model->SumofSquares()) << QPointF(m_model->GlobalParameter(i), m_model->SumofSquares()*1.1);
        current_constant->setColor(xy_series->color());
        current_constant->setName(m_model->GlobalParameterName(i));
        view->addSeries(current_constant, true);
    }
    return view;
}

ChartView *  WGSResultsWidget::MoCoPlot()
{
    WriteConfidence(m_data);
    
    QtCharts::QChart *chart = new QtCharts::QChart;
    if(qApp->instance()->property("chartanimation").toBool())
        chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    chart->setTheme((QtCharts::QChart::ChartTheme) qApp->instance()->property("charttheme").toInt());
    ChartView *view = new ChartView(chart);
    QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries;
    QList<qreal > x = ToolSet::String2DoubleList( m_data["data"].toObject()["global_0"].toString() );
    QList<qreal > y = ToolSet::String2DoubleList( m_data["data"].toObject()["global_1"].toString() );
    
    for(int j = 0; j < x.size(); ++j)
            xy_series->append(QPointF(x[j], y[j]));
    
    if(x.size())
            has_data = true;
    
    xy_series->setMarkerSize(6);
    xy_series->setName("MC Results");
    view->addSeries(xy_series, true);
    
    QList<QList<QPointF > >series;
    
    QJsonObject box = m_data["box"].toObject();
        series << ToolSet::String2Points( box["0"].toString() );
        series << ToolSet::String2Points( box["1"].toString() );
    int i = 0;
    
    for(const QList<QPointF> &serie : qAsConst(series))
    {
        LineSeries *xy_serie = new LineSeries;
        xy_serie->append(serie);
        xy_serie->setName(m_model->GlobalParameterName(i));
        view->addSeries(xy_serie, true);
        ++i;
    }
    
    view->setXAxis(m_model->GlobalParameterName(0));
    view->setYAxis(m_model->GlobalParameterName(1));
    return view;
}
