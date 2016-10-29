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

#include "ui/widgets/chartview.h"
#include "core/data/dataclass.h"
#include "core/data/modelclass.h"
#include <QDrag>
#include <QBuffer>
#include <QVector>
#include <QtWidgets/QComboBox>
#include <QtCharts/QChart>
// #include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QLegendMarker>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QGridLayout>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>
#include <QPushButton>
#include <QTableView>
#include "chartwidget.h"


ChartWidget::ChartWidget() : m_themebox(createThemeBox())
{
    
    m_signalchart = new QtCharts::QChart;
    m_signalchart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    
    m_errorchart = new QtCharts::QChart;
    m_errorchart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    
    m_signalview = new ChartView(m_signalchart);
    m_errorview = new ChartView(m_errorchart);
    
    m_x_scale = new QComboBox;
    m_x_scale->addItems(QStringList()  << tr("c(Guest)")<< tr("c(Host)") << tr("Ratio c(Host/Guest)")<< tr("Ratio c(Guest/Host)"));
    m_x_scale->setCurrentIndex(3);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_themebox, 0, 0);
    layout->addWidget(m_signalview,1, 0);
    layout->addWidget(m_errorview, 2, 0);
    layout->addWidget(m_x_scale, 3, 0);
    
    connect(m_x_scale, SIGNAL(currentIndexChanged(QString)), this, SLOT(Repaint()));
    connect(m_themebox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateUI()));
    setLayout(layout);
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
        signal_series->setColor(m_rawdata->color(i));
        addSeries(signal_series);
    }
    
    formatAxis();
    
}



void ChartWidget::addModel(const QPointer<AbstractTitrationModel > model)
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
            QtCharts::QLineSeries *series = new QtCharts::QLineSeries;
            series->setName("Signal " + QString::number(i + 1));
            series->setColor(model->color(i));
            mapper->setSeries(series);
            addLineSeries(series);
        }
        if(model->Type() != 3)
        {
            QtCharts::QVXYModelMapper * error= model->ErrorMapper(i);
            QtCharts::QLineSeries *error_series = new QtCharts::QLineSeries;
            error->setSeries(error_series);
            error_series->setName("Signal " + QString::number(i + 1));
            error_series->setColor(model->color(i));
            addErrorSeries(error_series);
        }
        
    }
    connect(model, SIGNAL(Recalculated()), this, SLOT(Repaint()));
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
    for(int i = 0; i < trash.size(); ++i)
        m_models.remove(trash[i]);

    formatAxis();
    formatErrorAxis();
    
}


void ChartWidget::addSeries( QtCharts::QScatterSeries *series, const QString &str)
{
    if(series->pointsVector().isEmpty())
        return;
    
    if(!m_signalchart->series().contains(series))
        m_signalchart->addSeries(series);
    m_signalchart->setTitle(str);
    
    
//     m_signalview->setRenderHint(QPainter::Antialiasing, true);
//     m_chartwidget->chart()->legend()->setAlignment(Qt::AlignRight);
}

void ChartWidget::addLineSeries(const QPointer< QtCharts::QLineSeries > &series, const QString& str)
{
    if(series->pointsVector().isEmpty())
        return;
    
    
    if(!m_signalchart->series().contains(series))
    {
        m_signalchart->addSeries(series);
        m_signalchart->legend()->markers(series).first()->setVisible(false);
    }
    m_signalchart->setTitle(str);
    
//     m_signalview->setRenderHint(QPainter::Antialiasing, true);
//     m_chartwidget->chart()->legend()->setAlignment(Qt::AlignRight);
}
void ChartWidget::addErrorSeries(const QPointer< QtCharts::QLineSeries > &series, const QString& str)
{
    
    if(series->pointsVector().isEmpty())
        return;
    
    if(!m_errorchart->series().contains(series))
        m_errorchart->addSeries(series);
    m_errorchart->setTitle(str);
//     m_errorview->setRenderHint(QPainter::Antialiasing, true);     
}

void ChartWidget::formatAxis()
{
    if(m_signalchart->series().isEmpty())
        return;
    m_signalchart->createDefaultAxes();
    
    
    QtCharts::QValueAxis *y_axis = qobject_cast<QtCharts::QValueAxis *>( m_signalchart->axisY());
    y_axis->applyNiceNumbers();
    y_axis->setTitleText("Shift [ppm]");

    QtCharts::QValueAxis *x_axis = qobject_cast<QtCharts::QValueAxis *>( m_signalchart->axisX());
    x_axis->applyNiceNumbers();
    x_axis->setTitleText(m_x_scale->currentText());

}
void ChartWidget::formatErrorAxis()
{
    
    if(m_errorchart->series().isEmpty())
        return;
    m_errorchart->createDefaultAxes();

    QtCharts::QValueAxis *y_axis = qobject_cast<QtCharts::QValueAxis *>( m_errorchart->axisY());
    
    y_axis->applyNiceNumbers();
    y_axis->setTitleText("Error [ppm]");
    
    QtCharts::QValueAxis *x_axis = qobject_cast<QtCharts::QValueAxis *>( m_errorchart->axisX());
    x_axis->applyNiceNumbers();

    x_axis->setTitleText(m_x_scale->currentText());

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
    
    //     if (m_chart->theme() != theme) {
    
    m_signalchart->setTheme(theme);
    m_errorchart->setTheme(theme);
    QPalette pal = window()->palette();
    if (theme == QtCharts::QChart::ChartThemeLight) {
        pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
    } else if (theme == QtCharts::QChart::ChartThemeDark) {
        pal.setColor(QPalette::Window, QRgb(0x121218));
        pal.setColor(QPalette::WindowText, QRgb(0xd6d6d6));
    } else if (theme == QtCharts::QChart::ChartThemeBlueCerulean) {
        pal.setColor(QPalette::Window, QRgb(0x40434a));
        pal.setColor(QPalette::WindowText, QRgb(0xd6d6d6));
    } else if (theme == QtCharts::QChart::ChartThemeBrownSand) {
        pal.setColor(QPalette::Window, QRgb(0x9e8965));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
    } else if (theme == QtCharts::QChart::ChartThemeBlueNcs) {
        pal.setColor(QPalette::Window, QRgb(0x018bba));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
    } else if (theme == QtCharts::QChart::ChartThemeHighContrast) {
        pal.setColor(QPalette::Window, QRgb(0xffab03));
        pal.setColor(QPalette::WindowText, QRgb(0x181818));
    } else if (theme == QtCharts::QChart::ChartThemeBlueIcy) {
        pal.setColor(QPalette::Window, QRgb(0xcee7f0));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
    } else {
        pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
    }
    window()->setPalette(pal);
    //     }
}

void ChartWidget::setActiveSignals( QVector<int> active_signals)
{
    qDebug() << active_signals;
    if(active_signals.size() < m_signalchart->series().size()  && active_signals.size() <= m_errorchart->series().size())
    {
        for(int i = 0; i < active_signals.size(); ++i)
        {
            m_signalchart->series()[i]->setVisible(active_signals[i]);
            m_signalchart->series()[i + active_signals.size()]->setVisible(active_signals[i]);
            m_errorchart->series()[i]->setVisible(active_signals[i]);
        }
        Repaint();
    }
}



#include "chartwidget.moc"
