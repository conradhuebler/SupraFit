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
#include "src/capabilities/montecarlostatistics.h"
#include "src/core/AbstractModel.h"
#include "src/core/toolset.h"
#include "src/ui/guitools/waiter.h"
#include "src/ui/widgets/chartview.h"
#include "src/ui/widgets/statisticwidget.h"

#include <QtCore/QPointer>

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>

#include "mcresultswidget.h"

MCResultsWidget::MCResultsWidget(QPointer<MonteCarloStatistics > statistics, QSharedPointer<AbstractModel> model) //: ResultsWidget(statistics, model,  parent) 
{
    m_statistics = statistics;
    m_model = model;
    if(m_statistics)
        setUi();
}


MCResultsWidget::~MCResultsWidget()
{

}

QWidget * MCResultsWidget::ChartWidget()
{
    QWidget *widget = new QWidget;
    QWidget *resultwidget = new QWidget;
    QGridLayout *layout = new QGridLayout;
    resultwidget->setLayout(layout);

    m_histgram = MakeHistogram();
    layout->addWidget(m_histgram, 0, 0, 1, 7);
    if(m_model->GlobalParameterSize() == 2)
    {
        m_contour = MakeContour();
        layout->addWidget(m_contour, 0, 0, 1, 7);
        m_contour->hide();
        m_switch = new QPushButton(tr("Switch Plots"));
        connect(m_switch, SIGNAL(clicked()), this, SLOT(SwitchView()));
    }
    m_save = new QPushButton(tr("Export Results"));
    connect(m_save, SIGNAL(clicked()), this, SLOT(ExportResults()));
    m_error = new QDoubleSpinBox;
    m_error->setValue(95);
    m_error->setSingleStep(0.5);
    m_error->setSuffix(tr("%"));
    m_error->setMaximum(100);
    connect(m_error, SIGNAL(valueChanged(double)), m_statistics, SLOT(AnalyseData(double)));
    connect(m_statistics, SIGNAL(AnalyseFinished()), this, SLOT(UpdateConfidence()));
    layout->addWidget(new QLabel(tr("Confidence Intervall")), 1, 0);
    layout->addWidget(m_error, 1, 1);
    layout->addWidget(m_save, 1, 2);
    if(m_model->GlobalParameterSize() == 2)
        layout->addWidget(m_switch, 1, 3);
    
    widget->setLayout(layout);
    UpdateConfidence();
    return widget;
}

QPointer<ChartView> MCResultsWidget::MakeContour()
{
    QtCharts::QChart *chart_ellipsoid = new QtCharts::QChart; 
    QPointer<ChartView > view = new ChartView(chart_ellipsoid);
    
    QList<QPointF > data = ToolSet::fromModelsList(m_statistics->Models(), "globalParameter");
    QWidget *resultwidget_ellipsoid = new QWidget;
    QGridLayout *layout_ellipsoid = new QGridLayout;
    resultwidget_ellipsoid->setLayout(layout_ellipsoid);
    
    chart_ellipsoid->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    
    layout_ellipsoid->addWidget(view, 0, 0, 1, 7);
    QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries(this);
    xy_series->append(data);
    xy_series->setMarkerSize(8);
    view->addSeries(xy_series);
    view->setXAxis(m_model->GlobalParameterName(0));
    view->setYAxis(m_model->GlobalParameterName(1));
    return view;
}

QPointer<ChartView> MCResultsWidget::MakeHistogram()
{
    QtCharts::QChart *chart = new QtCharts::QChart;
    
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    QPointer<ChartView > view = new ChartView(chart);
    view->setXAxis("constant");
    view->setYAxis("rate");
    view->setMinimumSize(300,400);
    bool formated = false;
    
    QList<QList<QPointF > >series = m_statistics->Series();
    QList<QJsonObject > constant_results = m_statistics->Results();
    
    WriteConfidence(constant_results);
    
    for(int i = 0; i < series.size(); ++i)
    {
        QtCharts::QLineSeries *xy_series = new QtCharts::QLineSeries(this);
        xy_series->append(series[i]);
        view->addSeries(xy_series);
        if(!formated)
            view->formatAxis();
        formated = true;

        QtCharts::QLineSeries *current_constant= new QtCharts::QLineSeries();
        *current_constant << QPointF(m_model->GlobalParameter(i), 0) << QPointF(m_model->GlobalParameter(i), view->YMax());
        current_constant->setColor(xy_series->color());
        current_constant->setName( m_model->GlobalParameterName(i));
        view->addSeries(current_constant, true);
        
        QJsonObject confidenceObject = constant_results[i]["confidence"].toObject();
        if(view)
        {
            QtCharts::QAreaSeries *area_series = AreaSeries(xy_series->color());
            view->addSeries(area_series);
            m_area_series << area_series;
        } 
        m_colors << xy_series->color();
    } 
    return view;
}

void MCResultsWidget::WriteConfidence(const QList<QJsonObject > &constant_results)
{ 
    QString confidence;
    QJsonObject controller = constant_results.first()["controller"].toObject();
    confidence += "MC Steps: " + QString::number(controller["steps"].toInt()) + "\t";
    if(controller["bootstrap"].toBool())
        confidence += "Bootstrapped ";
    else
        confidence += "Variance = " + QString::number(controller["variance"].toDouble()) + " ";
    
    if(controller["original"].toBool())
        confidence += "operated on original data\n";
    else
        confidence += "operated on modelled data\n";
    
    for(int i = 0; i < constant_results.size(); ++i)
    {
        m_model->setMCStatistic(constant_results[i], i);
        if(i < m_model->GlobalParameterSize())
        {
            QJsonObject confidenceObject = constant_results[i]["confidence"].toObject();
            confidence  += Print::TextFromConfidence(constant_results[i], m_model.data()) + "\n";
        }
    }
    m_confidence_label->setText(confidence);
}


void MCResultsWidget::UpdateConfidence()
{    
    QList<QList<QPointF > >series = m_statistics->Series();
    QList<QJsonObject > constant_results = m_statistics->Results();
    
    for(int i = 0; i < constant_results.size(); ++i)
    {
        m_model->setMCStatistic(constant_results[i], i);
    }

    WriteConfidence(constant_results);
    UpdateBoxes(series, constant_results);
}

void MCResultsWidget::UpdateBoxes(const QList<QList<QPointF > > &series, const QList<QJsonObject > &constant_results)
{    
    for(int i = 0; i < series.size(); ++i)
    {
        double max = m_histgram->YMax()/2;
        QJsonObject confidenceObject = constant_results[i]["confidence"].toObject();

        if(m_histgram)
        {
            QtCharts::QAreaSeries *area_series = m_area_series[i];
            QtCharts::QLineSeries *series1 = area_series->lowerSeries();
            QtCharts::QLineSeries *series2 = area_series->upperSeries();
            
            series1->clear();
            series2->clear();
            
            *series1 << QPointF(confidenceObject["lower"].toVariant().toDouble(), 0) << QPointF(confidenceObject["lower"].toVariant().toDouble(), max);
            *series2 << QPointF(confidenceObject["upper"].toVariant().toDouble(), 0) << QPointF(confidenceObject["upper"].toVariant().toDouble(), max);
            
            area_series->setLowerSeries(series1);
            area_series->setUpperSeries(series2);
            area_series->setName(m_model->GlobalParameterName(i));
        }
    }
}


void MCResultsWidget::ExportResults()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(str.isEmpty())
        return;
    Waiter wait;
    setLastDir(str);
    m_statistics->ExportResults(str);
}


QtCharts::QAreaSeries * MCResultsWidget::AreaSeries(const QColor &color) const
{
    QtCharts::QLineSeries *series1 = new QtCharts::QLineSeries();
    QtCharts::QLineSeries *series2 = new QtCharts::QLineSeries();
    QtCharts::QAreaSeries *area_series = new QtCharts::QAreaSeries(series1, series2);
    QPen pen(0x059605);
    pen.setWidth(3);
    area_series->setPen(pen);
    
    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
    gradient.setColorAt(0.0, color);
    gradient.setColorAt(1.0, 0x26f626);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    area_series->setBrush(gradient);
    area_series->setOpacity(0.4);
    return area_series;
}

void MCResultsWidget::SwitchView()
{
    bool histogram = m_histgram->isHidden();
    m_histgram->setHidden(!histogram);
    m_contour->setHidden(histogram);
}
