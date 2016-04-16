/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "core/data/dataclass.h"
#include "core/data/modelclass.h"
#include <QVector>
#include <QtWidgets/QComboBox>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QGridLayout>
#include <QtCharts/QValueAxis>
#include "chartwidget.h"
ChartWidget::ChartWidget() : m_y_max_chart(0), m_y_max_error(0), m_x_max_chart(0), m_x_max_error(0), m_y_min_chart(10)
{
    m_chart = new QtCharts::QChart;
    m_errorview = new QtCharts::QChart;
    m_chartwidget = new QtCharts::QChartView(m_chart);
    m_errorchart = new QtCharts::QChartView(m_errorview);
    m_x_scale = new QComboBox;
    m_x_scale->addItems(QStringList() << tr("Host") << tr("Guest") << tr("Ratio"));
    m_x_scale->setCurrentIndex(2);
    m_error_axis = QSharedPointer<QtCharts::QLineSeries> (new QtCharts::QLineSeries, &QObject::deleteLater);
        m_error_axis->setColor(Qt::black);
        m_errorview->addSeries(m_error_axis.data());
    
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_chartwidget,0, 0);
    layout->addWidget(m_errorchart, 1, 0);
    layout->addWidget(m_x_scale, 2, 0);
    connect(m_x_scale, SIGNAL(currentIndexChanged(QString)), this, SLOT(Repaint()));
    setLayout(layout);
}

ChartWidget::~ChartWidget()
{
}

void ChartWidget::addModel(const QPointer<AbstractTitrationModel > model)
{
    m_models << model;
    connect(model, SIGNAL(Recalculated()), this, SLOT(Repaint()));
    Repaint();
}

void ChartWidget::Repaint()
{
        
    m_y_max_chart = 0;
    m_y_min_chart = 10;
        
    m_y_max_error = 0;
    m_y_min_error = 10;

    AbstractTitrationModel::PlotMode j = (AbstractTitrationModel::PlotMode)(m_x_scale->currentIndex() + 1) ;
    foreach(QPointer<AbstractTitrationModel > model, m_models)
    {
        model->setPlotMode(j);
        
        for(int i = 0; i < model->Size(); ++i)
        {
            addSeries(model->SignalSeries(i).data());
            addLineSeries(model->ModelSeries(i).data());
            addErrorSeries(model->ErrorSeries(i).data());
        }
    }
    
        formatAxis();
        formatErrorAxis();

}


void ChartWidget::addSeries(const QPointer< QtCharts::QScatterSeries > &series, const QString &str)
{
    for(int i = 0; i < series->pointsVector().size(); ++i)
    {
        if(series->pointsVector()[i].y() > m_y_max_chart)
            m_y_max_chart = series->pointsVector()[i].y();
        if(series->pointsVector()[i].y() < m_y_min_chart)
            m_y_min_chart = series->pointsVector()[i].y();
    }
    m_x_max_chart = series->pointsVector().last().x();
            
    m_error_axis.data()->clear();
    m_error_axis->append(0,0);
    m_error_axis->append(m_x_max_chart, 0);
    
    if(!m_chart->series().contains(series))
        m_chart->addSeries(series);
    m_chart->setTitle(str);
    
    
    m_chartwidget->setRenderHint(QPainter::Antialiasing, true);
    m_chartwidget->chart()->legend()->setAlignment(Qt::AlignRight);
}

void ChartWidget::addLineSeries(const QPointer< QtCharts::QLineSeries > &series, const QString& str)
{
    for(int i = 0; i < series->pointsVector().size(); ++i)
    {
        if(series->pointsVector()[i].y() > m_y_max_chart)
            m_y_max_chart = series->pointsVector()[i].y();
                
        if(series->pointsVector()[i].y() < m_y_min_chart)
            m_y_min_chart = series->pointsVector()[i].y();
    }
    m_x_max_chart = series->pointsVector().last().x();
    if(!m_chart->series().contains(series))
        m_chart->addSeries(series);
    m_chart->setTitle(str);
    
    m_chartwidget->setRenderHint(QPainter::Antialiasing, true);
    m_chartwidget->chart()->legend()->setAlignment(Qt::AlignRight);
}
void ChartWidget::addErrorSeries(const QPointer< QtCharts::QLineSeries > &series, const QString& str)
{
    for(int i = 0; i < series->pointsVector().size(); ++i)
    {
        if(series->pointsVector()[i].y() > m_y_max_error)
            m_y_max_error = series->pointsVector()[i].y();
        if(series->pointsVector()[i].y() < m_y_min_error)
            m_y_min_error = series->pointsVector()[i].y();
    }
    if(!m_errorview->series().contains(series))
        m_errorview->addSeries(series);
    m_errorview->setTitle(str);
    m_errorchart->setRenderHint(QPainter::Antialiasing, true);     
}

void ChartWidget::formatAxis()
{
    m_chart->createDefaultAxes();
    QtCharts::QValueAxis *x_axis = qobject_cast<QtCharts::QValueAxis *>( m_chart->axisX());
    if(m_x_max_chart > 1)
    {
        x_axis->setMax(int(m_x_max_chart)+1);
        x_axis->setTickCount(int(m_x_max_chart)+2);
    }else
        x_axis->setMax(m_x_max_chart+m_x_max_chart*0.01);

    x_axis->setMin(0);
    
    QtCharts::QValueAxis *y_axis = qobject_cast<QtCharts::QValueAxis *>( m_chart->axisY());
    y_axis->setMax(int(m_y_max_chart) + 1);
    y_axis->setTickCount(int(m_y_max_chart - m_y_min_chart) + 2);
    y_axis->setMin(int(m_y_min_chart));
    
    
}
void ChartWidget::formatErrorAxis()
{
    m_errorview->createDefaultAxes();
    QtCharts::QValueAxis *x_axis = qobject_cast<QtCharts::QValueAxis *>( m_errorview->axisX());
    if(m_x_max_chart > 1)
    {
        x_axis->setTickCount(2*(int(m_x_max_chart))+3);
        x_axis->setMax(int(m_x_max_chart)+1);
    }else
        x_axis->setMax(m_x_max_chart+m_x_max_chart*0.01);

    x_axis->setMin(0);
    
    QtCharts::QValueAxis *y_axis = qobject_cast<QtCharts::QValueAxis *>( m_errorview->axisY());
    y_axis->setMin(1.1*m_y_min_error);
    y_axis->setMax(1.1*m_y_max_error);
}

#include "chartwidget.moc"
