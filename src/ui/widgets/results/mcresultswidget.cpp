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
#include "src/capabilities/montecarlostatistics.h"
#include "src/core/AbstractModel.h"
#include "src/core/toolset.h"
#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/waiter.h"
#include "src/ui/widgets/chartview.h"
#include "src/ui/widgets/listchart.h"
#include "src/ui/widgets/statisticwidget.h"

#include <QtCore/QPointer>

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>

#include <QtCharts/QAreaSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBoxPlotSeries>
#include <QtCharts/QBoxSet>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>

#include "mcresultswidget.h"

MCResultsWidget::MCResultsWidget(const QList<QJsonObject > &data, QSharedPointer<AbstractModel> model, ChartWrapper *wrapper, const QList<QJsonObject > &models, Type type) : m_data(data), m_type(type)
{
    m_model = model;
    m_models = models;
    m_wrapper = wrapper;
    has_boxplot = false;
    has_histogram = false;
    has_contour = false;
    setUi();
    GenerateConfidence(95);
}


MCResultsWidget::~MCResultsWidget()
{

}

QWidget * MCResultsWidget::ChartWidget()
{
    QWidget *widget = new QWidget;
    QTabWidget *tabs = new QTabWidget;
    tabs->setTabPosition(QTabWidget::South);
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(tabs, 0, 0, 1, 7);

    m_histgram = MakeHistogram();
    m_box = MakeBoxPlot();
    if(has_histogram)
        tabs->addTab(m_histgram, tr("Histogram"));
    
    if(has_boxplot)
        tabs->addTab(m_box, tr("Boxplot"));
    
    if(m_model->GlobalParameterSize() == 2 && m_models.size() && has_contour)
    {
        m_contour = MakeContour();
        tabs->addTab(m_contour, tr("Contour Plot"));
    }
    m_save = new QPushButton(tr("Export Results"));
    connect(m_save, SIGNAL(clicked()), this, SLOT(ExportResults()));
    
    m_error = new QDoubleSpinBox;
    m_error->setValue(95);
    m_error->setSingleStep(0.5);
    m_error->setSuffix(tr("%"));
    m_error->setMaximum(100);
    connect(m_error, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MCResultsWidget::GenerateConfidence);
    
    layout->addWidget(new QLabel(tr("Confidence Intervall")), 1, 0);
    layout->addWidget(m_error, 1, 1);
    
    if(m_models.size())
        layout->addWidget(m_save, 1, 2);
    
    widget->setLayout(layout);
    
    return widget;
}

QPointer<ListChart> MCResultsWidget::MakeHistogram()
{
    QPointer<ListChart> view = new ListChart;
    view->setXAxis("parameter");
    view->setYAxis("relative rate");
    if(qApp->instance()->property("chartanimation").toBool())
        view->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    view->setTheme((QtCharts::QChart::ChartTheme) qApp->instance()->property("charttheme").toInt());
    view->setMinimumSize(300,400);
    bool formated = false;
    
    for(int i = 0; i < m_data.size(); ++i)
    {
        QString name = m_data[i]["name"].toString();
        qreal x_0 = m_data[i]["value"].toDouble();
        QVector<qreal> list = ToolSet::String2DoubleVec(m_data[i]["data"].toObject()["raw"].toString());
        QVector<QPair<qreal, qreal> > histogram = ToolSet::List2Histogram(list,500);
        LineSeries *xy_series = new LineSeries;
        if(m_data[i]["type"] == "Local Parameter")
        {
            if(!m_data[i].contains("index"))
                continue;
            int index =  m_data[i]["index"].toString().split("|")[1].toInt();
            xy_series->setColor(m_wrapper->Series(index)->color());
            connect(m_wrapper->Series(index), &QtCharts::QXYSeries::colorChanged, xy_series, &LineSeries::setColor);
            connect(m_wrapper->Series(index), &QtCharts::QXYSeries::colorChanged, [i, view]( const QColor &color ) { view->setColor(i, color); }); 
            connect(m_wrapper->Series(index), &QtCharts::QXYSeries::colorChanged, [index, this]( const QColor &color ) { this->setAreaColor(index, color); });
        }else
            xy_series->setColor(ChartWrapper::ColorCode(i));
        
        for(int j = 0; j < histogram.size(); ++j)
        {
            xy_series->append(QPointF(histogram[j].first, histogram[j].second));       
        }
        if(histogram.size())
             has_histogram = true;
        view->addSeries(xy_series, i, xy_series->color(), name);
        view->setColor(i, xy_series->color());
        if(!formated)
            view->formatAxis();
        formated = true;
        
        LineSeries *current_constant = new LineSeries;
        connect(xy_series, &QtCharts::QXYSeries::colorChanged, current_constant, &LineSeries::setColor);
        *current_constant << QPointF(x_0, 0) << QPointF(x_0, 1.25);
        current_constant->setColor(xy_series->color());
        current_constant->setName( name);
        view->addSeries(current_constant, i, xy_series->color(), name);
        
        QJsonObject confidenceObject = m_data[i]["confidence"].toObject();
        if(view)
        {
            QtCharts::QAreaSeries *area_series = AreaSeries(xy_series->color());
            view->addSeries(area_series, i, area_series->color(), name);
            m_area_series << area_series;
        } 
        m_colors << xy_series->color();
        
    }

    return view;
}

 QPointer<ListChart > MCResultsWidget::MakeBoxPlot()
{
    ListChart *boxplot = new ListChart;
    double min = 10, max = 0;
    if(qApp->instance()->property("chartanimation").toBool())
        boxplot->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    boxplot->setTheme((QtCharts::QChart::ChartTheme) qApp->instance()->property("charttheme").toInt());
    for(int i = 0; i < m_data.size(); ++i)
    {
        QJsonObject data = m_data[i];

        has_boxplot = true;
        
        SupraFit::BoxWhisker bw = ToolSet::Object2Whisker( data["boxplot"].toObject() );
        min = qMin(bw.lower_whisker, min);
        max = qMax(bw.upper_whisker, max);
        BoxPlotSeries *series = new BoxPlotSeries(bw);
        series->setName(m_data[i]["name"].toString());
 
        if(m_data[i]["type"] == "Local Parameter")
        {
            if(!m_data[i].contains("index"))
                continue;
            int index =  m_data[i]["index"].toString().split("|")[1].toInt();
            series->setBrush(m_wrapper->Series(index)->color());
            connect(m_wrapper->Series(index), &QtCharts::QXYSeries::colorChanged, series, &BoxPlotSeries::setColor);
            connect(m_wrapper->Series(index), &QtCharts::QXYSeries::colorChanged, [i, boxplot]( const QColor &color ) { boxplot->setColor(i, color); }); 
        }
        boxplot->addSeries(series, i, series->color(), m_data[i]["name"].toString());  
        boxplot->setColor(i, series->color());
        m_box_object << ToolSet::Box2Object(bw);
    }

    if(has_boxplot)
    {
        QtCharts::QValueAxis *y_axis = qobject_cast<QtCharts::QValueAxis *>( boxplot->Chart()->axisY());
        y_axis->setMin(min*0.99);
        y_axis->setMax(max*1.01);
    }
    return boxplot;
}


QPointer<ChartView> MCResultsWidget::MakeContour()
{
    QtCharts::QChart *chart_ellipsoid = new QtCharts::QChart; 
    QPointer<ChartView > view = new ChartView(chart_ellipsoid);
    if(qApp->instance()->property("chartanimation").toBool())
        chart_ellipsoid->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    chart_ellipsoid->setTheme((QtCharts::QChart::ChartTheme) qApp->instance()->property("charttheme").toInt());
    QList<QPointF > data = ToolSet::fromModelsList(m_models, "globalParameter");
    if(data.size())
        has_contour = true;
    QWidget *resultwidget_ellipsoid = new QWidget;
    QGridLayout *layout_ellipsoid = new QGridLayout;
    resultwidget_ellipsoid->setLayout(layout_ellipsoid);

    layout_ellipsoid->addWidget(view, 0, 0, 1, 7);
    QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries(this);
    xy_series->append(data);
    xy_series->setMarkerSize(8);
    view->addSeries(xy_series, false);
    view->setXAxis(m_model->GlobalParameterName(0));
    view->setYAxis(m_model->GlobalParameterName(1));
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
        if(m_type == MonteCarlo)
            m_model->setMCStatistic(constant_results[i], i);

        QJsonObject confidenceObject = constant_results[i]["confidence"].toObject();
        confidence  += Print::TextFromConfidence(constant_results[i], m_model.data()) + "\n";
    }
    m_confidence_label->setText(confidence);
}

void MCResultsWidget::UpdateBoxes()
{    
     for(int i = 0; i < m_data.size(); ++i)
    {
        QJsonObject data = m_data[i];
        QJsonObject confidenceObject = data["confidence"].toObject();
        if(m_histgram)
        {
            QtCharts::QAreaSeries *area_series = m_area_series[i];
            QtCharts::QLineSeries *series1 = area_series->lowerSeries();
            QtCharts::QLineSeries *series2 = area_series->upperSeries();
            
            series1->clear();
            series2->clear();
            
            *series1 << QPointF(confidenceObject["lower"].toVariant().toDouble(), 0) << QPointF(confidenceObject["lower"].toVariant().toDouble(), 0.66);
            *series2 << QPointF(confidenceObject["upper"].toVariant().toDouble(), 0) << QPointF(confidenceObject["upper"].toVariant().toDouble(), 0.66);
            
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
    ToolSet::ExportResults(str, m_models);
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

void MCResultsWidget::setAreaColor(int index, const QColor &color)
{
    if(index >= m_area_series.size())
        return;
    QtCharts::QAreaSeries *area_series = m_area_series[index];
    
    QPen pen(0x059605);
    pen.setWidth(3);
    area_series->setPen(pen);
    
    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
    gradient.setColorAt(0.0, color);
    gradient.setColorAt(1.0, 0x26f626);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    area_series->setBrush(gradient);
    area_series->setOpacity(0.4);
}


void MCResultsWidget::GenerateConfidence(double error)
{    
    if(m_type == CrossValidation)
        error = 0;
    for(int i = 0; i < m_data.size(); ++i)
    {
        QList<qreal> list = ToolSet::String2DoubleList( m_data[i]["data"].toObject()["raw"].toString() );
        SupraFit::ConfidenceBar bar = ToolSet::Confidence(list, 100-error);
        QJsonObject confidence;
        confidence["lower"] = bar.lower;
        confidence["upper"] = bar.upper;
        m_data[i]["confidence"] = confidence;
        m_data[i]["error"] = error;
    }
    UpdateBoxes();
    WriteConfidence(m_data);
}

#include "mcresultswidget.moc"
