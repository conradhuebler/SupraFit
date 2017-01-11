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

#include "src/ui/widgets/chartview.h"
#include "src/core/dataclass.h"
#include "src/core/AbstractModel.h"
#include <QDrag>
#include <QBuffer>
#include <QVector>
#include <QtWidgets/QComboBox>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QGridLayout>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>
#include <QPushButton>
#include <QTableView>
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

ChartWidget::ChartWidget() : m_themebox(createThemeBox())
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
//     layout->addWidget(m_themebox, 0, 0);
    layout->addWidget(m_signalview,1, 0);
    layout->addWidget(m_errorview, 2, 0);
    layout->addWidget(m_x_scale, 3, 0);
    
    m_signalchart->setTheme(QtCharts::QChart::ChartThemeBlueCerulean);
    m_errorchart->setTheme(QtCharts::QChart::ChartThemeBlueCerulean);
    
    connect(m_themebox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateUI()));
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
    
    m_rawdata->PlotModel();
    AbstractTitrationModel::PlotMode j = (AbstractTitrationModel::PlotMode)(m_x_scale->currentIndex() + 1) ;
    m_rawdata->setPlotMode(j);
    for(int i = 0; i < m_rawdata->SignalCount(); ++i)
    {
        QtCharts::QVXYModelMapper * signal= m_rawdata->DataMapper(i);
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



void ChartWidget::addModel(QPointer<AbstractTitrationModel > model)
{
    m_models << model;
    model->UpdatePlotModels();
    connect(model, SIGNAL(Recalculated()), this, SLOT(Repaint()));
    connect(model, SIGNAL(ActiveSignalsChanged(QVector<int>)), this, SLOT(setActiveSignals(QVector<int>)));
    AbstractTitrationModel::PlotMode j = (AbstractTitrationModel::PlotMode)(m_x_scale->currentIndex() + 1) ;
    model->setPlotMode(j);
    for(int i = 0; i < model->SignalCount(); ++i)
    {
        
        if(model->Type() != 3)
        {
            QtCharts::QVXYModelMapper * mapper = model->ModelMapper(i);
            LineSeries *model_series = new LineSeries;
            mapper->setSeries(model_series);
            model_series->setName("Signal " + QString::number(i + 1));
            model_series->setColor(m_rawdata->color(i));
            connect(m_rawdata->DataMapper(i)->series(), SIGNAL(colorChanged(QColor)), model_series, SLOT(setColor(QColor)));
            m_signalview->addSeries(model_series, true);
        }
        if(model->Type() != 3)
        {
            QtCharts::QVXYModelMapper * error= model->ErrorMapper(i);
            LineSeries *error_series = new LineSeries;
            error->setSeries(error_series);
            error_series->setName("Signal " + QString::number(i + 1));
            error_series->setColor(m_rawdata->color(i));
            connect(m_rawdata->DataMapper(i)->series(), SIGNAL(colorChanged(QColor)), error_series, SLOT(setColor(QColor)));
            m_errorview->addSeries(error_series, true);
        }
        
    }
    connect(model, SIGNAL(Recalculated()), this, SLOT(formatAxis()));
//     connect(model, SIGNAL(Recalculated()), m_errorview, SLOT(formatAxis()));
    Repaint(); 
    
}

void ChartWidget::Repaint()
{         
    AbstractTitrationModel::PlotMode j = (AbstractTitrationModel::PlotMode)(m_x_scale->currentIndex() + 1) ;
    if(m_rawdata)
        m_rawdata->setPlotMode(j);
      m_rawdata->PlotModel();  
    QVector<int > trash;
    for(int i= 0; i < m_models.size(); ++i)
    {
        if(m_models[i])
        {
            m_models[i]->setPlotMode(j);
            m_models[i]->UpdatePlotModels();
        }else
            trash << i;
    }
//     for(int i = 0; i < trash.size(); ++i)
//         m_models.remove(trash[i]);

    formatAxis();
}

void ChartWidget::formatAxis()
{
    m_signalview->formatAxis();
    m_errorview->formatAxis();      
}


QPointer<QComboBox > ChartWidget::createThemeBox() const
{
    // settings layout
    QPointer<QComboBox >themeComboBox = new QComboBox();
    themeComboBox->addItem("Light", QtCharts::QChart::ChartThemeLight);
    themeComboBox->addItem("Blue Cerulean", QtCharts::QChart::ChartThemeBlueCerulean);
    themeComboBox->addItem("Dark", QtCharts::QChart::ChartThemeDark);
    themeComboBox->addItem("Brown Sand", QtCharts::QChart::ChartThemeBrownSand);
    themeComboBox->addItem("Blue NCS", QtCharts::QChart::ChartThemeBlueNcs);
    themeComboBox->addItem("High Contrast", QtCharts::QChart::ChartThemeHighContrast);
    themeComboBox->addItem("Blue Icy", QtCharts::QChart::ChartThemeBlueIcy);
    themeComboBox->addItem("Qt", QtCharts::QChart::ChartThemeQt);
    return themeComboBox;
}

void ChartWidget::updateUI()
{
    QtCharts::QChart::ChartTheme theme = (QtCharts::QChart::ChartTheme) m_themebox->itemData(m_themebox->currentIndex()).toInt();
        
    m_signalchart->setTheme(theme);
    m_errorchart->setTheme(theme);
    
     for(int i = 0; i < m_rawdata->SignalCount(); ++i)
         m_rawdata->DataMapper(i)->series()->setColor(m_rawdata->color(i));

}

void ChartWidget::setActiveSignals( QVector<int> active_signals)
{
    qDebug() << active_signals;
/*    if(active_signals.size() < m_signalchart->series().size()  && active_signals.size() <= m_errorchart->series().size())
    {
        for(int i = 0; i < active_signals.size(); ++i)
        {
            m_signalchart->series()[i]->setVisible(active_signals[i]);
            m_signalchart->series()[i + active_signals.size()]->setVisible(active_signals[i]);
            m_errorchart->series()[i]->setVisible(active_signals[i]);
        }
        Repaint();
    }*/
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
