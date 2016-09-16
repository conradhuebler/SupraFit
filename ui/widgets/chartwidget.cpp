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
#include <QPushButton>
#include <QTableView>
#include "chartwidget.h"
ChartWidget::ChartWidget() : m_y_max_chart(0), m_y_max_error(0), m_x_max_chart(0), m_x_max_error(0), m_y_min_error(0), m_y_min_chart(10), m_themebox(createThemeBox())
{

    m_chart = new QtCharts::QChart;
        m_chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    m_errorview = new QtCharts::QChart;
        m_errorview->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    m_chartwidget = new QtCharts::QChartView(m_chart);
    m_errorchart = new QtCharts::QChartView(m_errorview);
    m_x_scale = new QComboBox;
    m_x_scale->addItems(QStringList() << tr("Host") << tr("Guest") << tr("Ratio"));
    m_x_scale->setCurrentIndex(2);
    m_error_axis = QSharedPointer<QtCharts::QLineSeries> (new QtCharts::QLineSeries, &QObject::deleteLater);
        m_error_axis->setColor(Qt::black);
        m_errorview->addSeries(m_error_axis.data());
    
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_themebox, 0, 0);
    layout->addWidget(m_chartwidget,1, 0);
    layout->addWidget(m_errorchart, 2, 0);
    layout->addWidget(m_x_scale, 3, 0);
//     connect(m_x_scale, SIGNAL(currentIndexChanged(QString)), this, SLOT(Repaint()));
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
            signal_series->setColor(m_rawdata->color(i));
             addSeries(signal_series);
            if(!signal_series->pointsVector().isEmpty())
                for(int i = 0; i < signal_series->pointsVector().size(); ++i)
                {
                    if(signal_series->pointsVector()[i].y() > m_y_max_chart)
                        m_y_max_chart = signal_series->pointsVector()[i].y();
                    if(signal_series->pointsVector()[i].y() < m_y_min_chart)
                        m_y_min_chart = signal_series->pointsVector()[i].y();
                }

        }
        formatAxis();
        
}
void ChartWidget::addModel(const QPointer<AbstractTitrationModel > model)
{
    m_models << model;
    model->UpdatePlotModels();
   connect(model, SIGNAL(Recalculated()), this, SLOT(Repaint()));

    AbstractTitrationModel::PlotMode j = (AbstractTitrationModel::PlotMode)(m_x_scale->currentIndex() + 1) ;
    model->setPlotMode(j);
    for(int i = 0; i < model->SignalCount(); ++i)
        {

            if(model->Type() != 3)
            {
            QtCharts::QVXYModelMapper * mapper = model->ModelMapper(i);
            QtCharts::QLineSeries *series = new QtCharts::QLineSeries;
            series->setColor(model->color(i));
                mapper->setSeries(series);
             addLineSeries(series);
            }
            if(model->Type() != 3)
            {
            QtCharts::QVXYModelMapper * error= model->ErrorMapper(i);
            QtCharts::QLineSeries *error_series = new QtCharts::QLineSeries;
                error->setSeries(error_series);
                error_series->setColor(model->color(i));
                addErrorSeries(error_series);
            }
    
        }

        connect(model, SIGNAL(Recalculated()), this, SLOT(Repaint()));
        Repaint(); 

}

void ChartWidget::Repaint()
{         
     m_y_max_chart = 0;
     m_y_max_error = 0;
     m_x_max_chart = 0;
     m_x_max_error = 0;
     m_y_min_chart = 10;
     m_y_min_error = 0;
     formatAxis();
     formatErrorAxis();

}


void ChartWidget::addSeries( QtCharts::QScatterSeries *series, const QString &str)
{
    if(series->pointsVector().isEmpty())
        return;
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
    if(series->pointsVector().isEmpty())
        return;
        
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
        
    if(series->pointsVector().isEmpty())
        return;
    
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
    foreach(const QtCharts::QAbstractSeries *series_abstract, m_chart->series())
    {
        const QtCharts::QXYSeries * series = qobject_cast<const QtCharts::QXYSeries *>(series_abstract);
        if(!series->pointsVector().isEmpty())
            for(int i = 0; i < series->pointsVector().size(); ++i)
            {
                if(series->pointsVector()[i].y() > m_y_max_chart)
                    m_y_max_chart = series->pointsVector()[i].y();
                if(series->pointsVector()[i].y() < m_y_min_chart)
                    m_y_min_chart = series->pointsVector()[i].y();
            }
    }
    m_chart->createDefaultAxes();
    QtCharts::QValueAxis *x_axis = qobject_cast<QtCharts::QValueAxis *>( m_chart->axisX());
    if(m_x_max_chart > 1)
    {
        x_axis->setMax(int(m_x_max_chart)+1);
        x_axis->setTickCount(int(m_x_max_chart)+2);
    }else if(m_x_max_chart)
        x_axis->setMax(m_x_max_chart+m_x_max_chart*0.01);
    if(!m_x_max_chart)
        return;
    x_axis->setMin(0);
    
    QtCharts::QValueAxis *y_axis = qobject_cast<QtCharts::QValueAxis *>( m_chart->axisY());
    y_axis->setMax(int(m_y_max_chart) + 1);
    y_axis->setTickCount(int(m_y_max_chart - m_y_min_chart) + 2);
    y_axis->setMin(int(m_y_min_chart));
    
    
}
void ChartWidget::formatErrorAxis()
{
    foreach(const QtCharts::QAbstractSeries *series_abstract, m_errorview->series())
    {
        const QtCharts::QXYSeries * series = qobject_cast<const QtCharts::QXYSeries *>(series_abstract);
        if(!series->pointsVector().isEmpty())
            for(int i = 0; i < series->pointsVector().size(); ++i)
            {
                if(series->pointsVector()[i].y() > m_y_max_error)
                    m_y_max_error = series->pointsVector()[i].y();
                if(series->pointsVector()[i].y() < m_y_min_error)
                    m_y_min_error = series->pointsVector()[i].y();
            }
    }
    m_errorview->createDefaultAxes();
    QtCharts::QValueAxis *x_axis = new QtCharts::QValueAxis(qobject_cast<QtCharts::QValueAxis *>( m_chart->axisX()));
    m_errorview->setAxisX(x_axis);
    
    QtCharts::QValueAxis *y_axis = qobject_cast<QtCharts::QValueAxis *>( m_errorview->axisY());
    y_axis->setMin(1.1*m_y_min_error);
    y_axis->setMax(1.1*m_y_max_error);
    qDebug() << 1.1*m_y_min_error <<"min_error" << 1.1*m_y_max_error << "max_error";
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
        
            m_chart->setTheme(theme);
            m_errorview->setTheme(theme);
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


#include "chartwidget.moc"
