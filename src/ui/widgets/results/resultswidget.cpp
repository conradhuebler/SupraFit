/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/AbstractModel.h"
#include "src/core/toolset.h"
#include "src/ui/guitools/chartwrapper.h"

#include "src/ui/widgets/chartview.h"

#include "src/ui/widgets/listchart.h"
#include "src/ui/widgets/statisticwidget.h"
#include "src/ui/widgets/results/mcresultswidget.h"

#include <QtCore/QPointer>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QWidget>

#include "resultswidget.h"


ResultsWidget::ResultsWidget(const QJsonObject &data, QSharedPointer<AbstractModel> model, ChartWrapper *wrapper, const QList<QJsonObject > &models)
{
    m_data = data;
    m_model = model;
    m_models = models;
    m_wrapper = wrapper;
    
    setUi();
    resize(1024,600);
}

ResultsWidget::~ResultsWidget()
{

}

void ResultsWidget::setUi()
{
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    
    QGridLayout *layout = new QGridLayout;

    m_confidence_label = new QLabel();
    m_confidence_label->setTextFormat(Qt::RichText);
    m_confidence_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    
    switch(m_data["controller"].toObject()["method"].toInt()){
        case  SupraFit::Statistic::MonteCarlo:
             m_widget = MonteCarloWidget();
             setObjectName("Monte Carlo Simulation for " + m_model->Name());
        break;
        
        case  SupraFit::Statistic::ModelComparison:
            m_widget = ModelComparisonWidget();
            setObjectName("Model Comparison Confidence for " + m_model->Name());
        break;
        
        case  SupraFit::Statistic::WeakenedGridSearch:
            m_widget = GridSearchWidget();
            setObjectName("Weakend Grid Search Confidence for " + m_model->Name());
        break;
        
        case  SupraFit::Statistic::Reduction:
            m_widget = ReductionWidget();
            setObjectName("Reduction Analysis for " + m_model->Name());
        break;
        
        case  SupraFit::Statistic::CrossValidation:
            m_widget = MonteCarloWidget();
            setObjectName("Cross Validation Estimation for " + m_model->Name());
        break;
        
        default:
            m_widget = new QWidget;
        break;
    }
    
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidget(m_confidence_label);
    scroll->setWidgetResizable(true);
    
    splitter->addWidget(m_widget);
    splitter->addWidget(scroll);
    layout->addWidget(splitter, 0, 0);
    setLayout(layout);
    WriteConfidence(m_data);
}

QWidget * ResultsWidget::MonteCarloWidget()
{
    MCResultsWidget *widget = new MCResultsWidget(m_data, m_model, m_wrapper, m_models);
    connect(widget, &MCResultsWidget::ConfidenceUpdated, this, &ResultsWidget::WriteConfidence);
    widget->setUi();
    return widget;
}


QWidget * ResultsWidget::ReductionWidget()
{
    QPointer<ListChart> view = new ListChart;
    
    QVector<qreal> x = ToolSet::String2DoubleVec(m_data["controller"].toObject()["x"].toString());
    for(int i = 0; i < m_data.count() - 1; ++i)
    {
        QJsonObject data = m_data[QString::number(i)].toObject();
        if(data.isEmpty())
            continue;
        
        QString name = data["name"].toString();
        LineSeries *serie = new LineSeries;
        serie->setSize(4);
        QList< QPointF > series;
        QVector<qreal> list = ToolSet::String2DoubleVec(data["data"].toObject()["raw"].toString());
        for(int i = 0; i < list.size(); ++i)
            series << QPointF(x[i], list[i]);
        if(series.isEmpty())
            continue;
        
        QColor color;
        int index = 0, jndex = 0;
        if(data["type"].toString() == "Global Parameter")
            color = ChartWrapper::ColorCode(i);
        else
        {
            if(data.contains("index"))
            {   
                QStringList lindex = data["index"].toString().split("|");
                index =  lindex[1].toInt();
                jndex = lindex[0].toInt();
                color = m_wrapper->Series(index)->color();
            }
        }
        serie->append(series);
        serie->setName( name );
        serie->setColor(color);
        view->addSeries(serie, i, color, name);
        view->setColor(i,  color);
            
        serie = new LineSeries;
        serie->setDashDotLine(true);
        qreal value = 0;
        if(data["type"].toString() == "Global Parameter")
           value = m_model->GlobalParameter(i);
        else
           value = m_model->LocalParameter(jndex,index);
        
        serie->append(QPointF(series.last().x(), value));
        serie->append(QPointF(series.first().x(), value));
        serie->setColor(color);
        view->addSeries(serie, i, color, name);
        view->setColor(i,  color);
        if(data["type"].toString() != "Global Parameter")
            view->HideSeries(i);
    }
    return view;
}

QWidget * ResultsWidget::ModelComparisonWidget()
{    
    QJsonObject controller = m_data["controller"].toObject();
    ChartView *view = new ChartView;
    QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries;
    
    QList<qreal > x = ToolSet::String2DoubleList( controller["data"].toObject()["global_0"].toString() );
    QList<qreal > y = ToolSet::String2DoubleList( controller["data"].toObject()["global_1"].toString() );

    for(int j = 0; j < x.size(); ++j)
            xy_series->append(QPointF(x[j], y[j]));
    
    xy_series->setMarkerSize(6);
    xy_series->setName("MC Results");
    view->addSeries(xy_series, true);
    
    QList<QList<QPointF > >series;
    
    QJsonObject box = controller["box"].toObject();
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

QWidget * ResultsWidget::GridSearchWidget()
{
    ChartView *view = new ChartView;
    view->setXAxis("constant");
    view->setYAxis("Sum of Squares");

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
        view->addSeries(xy_series);
        
        LineSeries *current_constant= new LineSeries;
        *current_constant << QPointF(m_model->GlobalParameter(i), m_model->SumofSquares()) << QPointF(m_model->GlobalParameter(i), m_model->SumofSquares()*1.1);
        current_constant->setColor(xy_series->color());
        current_constant->setName(m_model->GlobalParameterName(i));
        view->addSeries(current_constant, true);
    }
    return view;
}

QWidget * ResultsWidget::SearchWidget()
{
    QWidget *widget = new QWidget;
    return widget;
}

void ResultsWidget::WriteConfidence(const QJsonObject &data)
{
    QString text;
    m_data = data;
    QJsonObject controller = m_data["controller"].toObject();
    for(int i = 0; i < m_data.count() - 1; ++i)
    {
        QJsonObject data = m_data[QString::number(i)].toObject();
        if(data.isEmpty())
            continue;
        text += Print::TextFromConfidence(data, m_model.data(), controller);
    }
    m_confidence_label->setText(text);
}


#include "resultswidget.moc"
