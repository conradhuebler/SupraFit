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

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
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
    
    QWidget *resultwidget = new QWidget;
    QGridLayout *layout = new QGridLayout;
    resultwidget->setLayout(layout);
    
    QtCharts::QChart *chart = new QtCharts::QChart;
    
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    QString confidence;
    m_histgram = MakeHistogram(confidence);
    layout->addWidget(m_histgram, 0, 0, 1, 7);
    m_contour = MakeContour();
    layout->addWidget(m_contour, 0, 0, 1, 7);
    m_contour->hide();
    
    m_switch = new QPushButton(tr("Switch Plots"));
    connect(m_switch, SIGNAL(clicked()), this, SLOT(SwitchView()));
    m_save = new QPushButton(tr("Export Results"));
    m_error = new QDoubleSpinBox;
    m_error->setValue(5);
    m_error->setSingleStep(0.5);
    m_error->setSuffix(tr("%"));
    connect(m_error, SIGNAL(valueChanged(double)), this, SLOT(ChangeConfidence()));
    layout->addWidget(new QLabel(tr("Confidence for Error")), 1, 0);
    layout->addWidget(m_error, 1, 1);
    layout->addWidget(m_save, 1, 2);
    if(m_model->ConstantSize() == 2)
        layout->addWidget(m_switch, 1, 3);
    
    QLabel *label = new QLabel(confidence);
    label->setTextFormat(Qt::RichText);
    layout->addWidget(label,2, 0, 1, 3);
    setLayout(layout);
}

QPointer<ChartView> MCResultsWidget::MakeContour()
{
    QtCharts::QChart *chart_ellipsoid = new QtCharts::QChart; 
    QPointer<ChartView > view = new ChartView(chart_ellipsoid);
    
    QList<QPointF > data = ToolSet::fromModelsList(m_statistics->Models());
    QWidget *resultwidget_ellipsoid = new QWidget;
    QGridLayout *layout_ellipsoid = new QGridLayout;
    resultwidget_ellipsoid->setLayout(layout_ellipsoid);
    
    chart_ellipsoid->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    
    layout_ellipsoid->addWidget(view, 0, 0, 1, 7);
    QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries(this);
    xy_series->append(data);
    xy_series->setMarkerSize(8);
    view->addSeries(xy_series);
    return view;
}

QPointer<ChartView> MCResultsWidget::MakeHistogram(QString &confidence)
{
    QtCharts::QChart *chart = new QtCharts::QChart;
    
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    QPointer<ChartView > view = new ChartView(chart);
    view->setMinimumSize(300,400);
    bool formated = false;
    
    QList<QList<QPointF > >series = m_statistics->getSeries();
    QList<QJsonObject > constant_results = m_statistics->getResult();
    
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
        
        QJsonObject confidenceObject = constant_results[i]["confidence"].toObject();
        double max = view->YMax();
        if(view)
            view->addSeries(AreaSeries(confidenceObject, xy_series->color(), max));
        
        
        if(i == 0)
        {
            confidence += "MC Steps: " + QString::number(constant_results[i]["controller"].toObject()["steps"].toInt()) + "\t";
            if(constant_results[i]["controller"].toObject()["bootstrap"].toBool())
                confidence += "Bootstrapped ";
            else
                confidence += "Variance = " + QString::number(constant_results[i]["controller"].toObject()["variance"].toDouble()) + " ";
            
            if(constant_results[i]["controller"].toObject()["original"].toBool())
                confidence += "operated on original data\n";
            else
                confidence += "operated on modelled data\n";
        }
        confidence  += StatisticWidget::TextFromConfidence(constant_results[i]) + "\n";
        m_colors << xy_series->color();
    }   
    return view;
}


void MCResultsWidget::ChangeConfidence()
{
    qreal error = m_error->value();
    m_statistics->AnalyseData(error);
    
    QList<QList<QPointF > >series = m_statistics->getSeries();
    QList<QJsonObject > constant_results = m_statistics->getResult();
    
    for(int i = 0; i < constant_results.size(); ++i)
        m_model->setMCStatistic(constant_results[i], i);
    ClearArea();
    for(int i = 0; i < series.size(); ++i)
    {
        double max = m_histgram->YMax();
         QJsonObject confidenceObject = constant_results[i]["confidence"].toObject();
        if(m_histgram)
            m_histgram->addSeries(AreaSeries(confidenceObject, m_colors[i], max));
    }
}

void MCResultsWidget::ExportResults()
{
    
}


QtCharts::QAreaSeries * MCResultsWidget::AreaSeries(const QJsonObject &confidence, const QColor &color, double max) const
{
    QtCharts::QLineSeries *series1 = new QtCharts::QLineSeries();
    QtCharts::QLineSeries *series2 = new QtCharts::QLineSeries();
    
    *series1 << QPointF(confidence["lower"].toVariant().toDouble(), 0) << QPointF(confidence["lower"].toVariant().toDouble(), max);
    *series2 << QPointF(confidence["upper"].toVariant().toDouble(), 0) << QPointF(confidence["upper"].toVariant().toDouble(), max);
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

void MCResultsWidget::ClearArea()
{
    for(QtCharts::QAbstractSeries *series: m_histgram->series())
    {
        QPointer<QtCharts::QAreaSeries > serie = qobject_cast<QtCharts::QAreaSeries * >(series);
        if(serie)
        {
            m_histgram->removeSeries(serie);
            if(serie)
                delete serie;
        }
    }
}

void MCResultsWidget::SwitchView()
{
    bool histogram = m_histgram->isHidden();
    m_histgram->setHidden(!histogram);
    m_contour->setHidden(histogram);
}
