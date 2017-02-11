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
#include "src/ui/chartwrapper.h"
#include "src/ui/widgets/chartview.h"
#include "src/core/dataclass.h"
#include "src/core/AbstractModel.h"

#include <QtCore/QTimer>
#include <QtCore/QWeakPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QBuffer>
#include <QtCore/QVector>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableView>

#include <QDrag>
#include <QPrinter>
#include <QPrintPreviewDialog>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>

#include <iostream>

#include "chartwidget.h"


 void LineSeries::setColor(const QColor &color) 
 { 
     QPen pen = QtCharts::QLineSeries::pen();
//      pen.setStyle(Qt::DashDotLine);
     pen.setWidth(2);
     pen.setColor(color);
     setPen(pen);     
}

void LineSeries::ShowLine(int state)
{
    if(state == Qt::Unchecked)
        setVisible(false);
    else if(state == Qt::Checked)
        setVisible(true);   
}

ChartWidget::ChartWidget() 
{
    
    m_signalchart = new QtCharts::QChart;
    m_signalchart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    
    m_errorchart = new QtCharts::QChart;
    m_errorchart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    
    m_signalview = new ChartView(m_signalchart);
    m_signalview->setYAxis("Shift [ppm]");
    m_errorview = new ChartView(m_errorchart);
    m_errorview->setYAxis("Error [ppm]");
    m_x_scale = new QComboBox;
    connect(m_x_scale, SIGNAL(currentIndexChanged(QString)), m_signalview, SLOT(setXAxis(QString)));
    connect(m_x_scale, SIGNAL(currentIndexChanged(QString)), m_errorview, SLOT(setXAxis(QString)));    
    m_x_scale->addItems(QStringList()  << tr("c(Guest)")<< tr("c(Host)") << tr("Ratio c(Host/Guest)")<< tr("Ratio c(Guest/Host)"));
    m_x_scale->setCurrentIndex(3);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_signalview,1, 0);
    layout->addWidget(m_errorview, 2, 0);
    layout->addWidget(m_x_scale, 3, 0);
    
    m_signalchart->setTheme(QtCharts::QChart::ChartThemeBlueCerulean);
    m_errorchart->setTheme(QtCharts::QChart::ChartThemeBlueCerulean);
    
    setLayout(layout);
    max_shift = 0;
    min_shift = 0;
}

ChartWidget::~ChartWidget()
{
}


void ChartWidget::setRawData(const QPointer<DataClass> rawdata) 
{
    m_rawdata = rawdata;
    
    ChartWrapper::PlotMode j = (ChartWrapper::PlotMode)(m_x_scale->currentIndex() + 1) ;
    m_data_mapper = new ChartWrapper(this);
    m_data_mapper->setPlotMode(j);
    m_data_mapper->setDataTable(m_rawdata->SignalModel());
    m_data_mapper->setData(m_rawdata);
    for(int i = 0; i < m_rawdata->SignalCount(); ++i)
    {
        QtCharts::QVXYModelMapper * signal= m_data_mapper->DataMapper(i);
        QtCharts::QScatterSeries *signal_series = new QtCharts::QScatterSeries;
        signal->setSeries(signal_series);
        signal_series->setName("Signal " + QString::number(i + 1));
        m_signalview->addSeries(signal_series, true);
        QPair<qreal, qreal > minmax = Series2MinMax(signal_series);
        min_shift = minmax.first;
        max_shift = minmax.second;
    }
    
    m_signalview->formatAxis();
    
}



Charts ChartWidget::addModel(QSharedPointer<AbstractTitrationModel > model)
{
    m_models << model;
    connect(model.data(), SIGNAL(Recalculated()), this, SLOT(Repaint()));
    ChartWrapper::PlotMode j = (ChartWrapper::PlotMode)(m_x_scale->currentIndex() + 1) ;
    ChartWrapper *signal_wrapper = new ChartWrapper(this);
    signal_wrapper->setPlotMode(j);
    signal_wrapper->setDataTable(model->ModelTable());
    signal_wrapper->setData(model.data());
    
    ChartWrapper *error_wrapper = new ChartWrapper(this);
    error_wrapper->setPlotMode(j);
    error_wrapper->setDataTable(model->ErrorTable());
    error_wrapper->setData(model.data());
    
    for(int i = 0; i < model->SignalCount(); ++i)
    {
        
        if(model->Type() != 3)
        {
            QtCharts::QVXYModelMapper * mapper = signal_wrapper->DataMapper(i);
            LineSeries *model_series = new LineSeries;
            mapper->setSeries(model_series);
            model_series->setName("Signal " + QString::number(i + 1));
            model_series->setColor(m_data_mapper->color(i));
            connect(m_data_mapper->DataMapper(i)->series(), SIGNAL(colorChanged(QColor)), model_series, SLOT(setColor(QColor)));
            m_signalview->addSeries(model_series, true);
        }
        if(model->Type() != 3)
        {
            QtCharts::QVXYModelMapper * error= error_wrapper->DataMapper(i);
            LineSeries *error_series = new LineSeries;
            error->setSeries(error_series);
            error_series->setName("Signal " + QString::number(i + 1));
            error_series->setColor(m_data_mapper->color(i));
            connect(m_data_mapper->DataMapper(i)->series(), SIGNAL(colorChanged(QColor)), error_series, SLOT(setColor(QColor)));
            m_errorview->addSeries(error_series, true);
        }
        
    }
    Repaint(); 
    Charts charts;
    charts.error_wrapper = error_wrapper;
    charts.signal_wrapper = signal_wrapper;
    charts.data_wrapper = m_data_mapper;
    return charts;
}

void ChartWidget::Repaint()
{         
    if(m_plot_mode != (ChartWrapper::PlotMode)(m_x_scale->currentIndex() + 1))
    {
        m_plot_mode = (ChartWrapper::PlotMode)(m_x_scale->currentIndex() + 1);
        m_data_mapper->setPlotMode(m_plot_mode);
    }    
    QVector<int > trash;
//     for(int i= 0; i < m_models.size(); ++i)
//     {
//         if(!m_models[i].isNull())
//         {
//             m_models[i].data()->setPlotMode(m_plot_mode);
//             m_models[i].data()->UpdatePlotModels();
//         }else
//             trash << i;
//     }
//     for(int i = 0; i < trash.size(); ++i)
//         m_models.remove(trash[i]);

    formatAxis();
}

void ChartWidget::formatAxis()
{
    QTimer::singleShot(1,m_signalview, SLOT(formatAxis()));
    QTimer::singleShot(1,m_errorview, SLOT(formatAxis()));
}


void ChartWidget::updateUI()
{
    QtCharts::QChart::ChartTheme theme = (QtCharts::QChart::ChartTheme) m_themebox->itemData(m_themebox->currentIndex()).toInt();
        
    m_signalchart->setTheme(theme);
    m_errorchart->setTheme(theme);
    
     for(int i = 0; i < m_rawdata->SignalCount(); ++i)
         m_data_mapper->DataMapper(i)->series()->setColor(m_data_mapper->color(i));

}

QPair<qreal, qreal > ChartWidget::Series2MinMax(const QtCharts::QXYSeries *series)
{
    QPair<qreal, qreal > values(0,0);
    QVector<QPointF> points = series->pointsVector();
    if(points.isEmpty())
        return values;
    for(int i = 0; i < points.size(); ++i)
    {
        values.first = qMin(values.first, points[i].ry());
        values.second = qMax(values.second, points[i].ry());
    }
    
    return values;
}

#include "chartwidget.moc"
