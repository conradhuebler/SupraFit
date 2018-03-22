/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/dataclass.h"
#include "src/core/AbstractModel.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/instance.h"
#include "src/ui/widgets/chartview.h"

#include <QtCore/QTimer>
#include <QtCore/QWeakPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QBuffer>
#include <QtCore/QVector>

#include <QtWidgets/QAction>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableView>

#include <QDrag>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QApplication>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>

#include <iostream>

#include "chartwidget.h"


ChartDockTitleBar::ChartDockTitleBar()
{    
    m_tools = new QPushButton(tr("Tools and Chart Settings"));
    m_tools->setFlat(true);
    m_tools->setIcon(QIcon::fromTheme("applications-system"));
    QMenu *toolsmenu = new QMenu;

    m_flip = new QAction(tr("Flip View"));
    connect(m_flip, &QAction::toggled, this, &ChartDockTitleBar::ChartFlip);
    
    toolsmenu->addAction(m_flip);

    toolsmenu->addSeparator();
    
    m_animation = new QAction(tr("Animation"));
    m_animation->setCheckable(true);
    m_animation->setChecked(qApp->instance()->property("chartanimation").toBool());
    
    connect(m_animation, &QAction::toggled, this, &ChartDockTitleBar::AnimationChanged);
    toolsmenu->addAction(m_animation);
    
    m_theme = new QMenu("Chart Theme");
    
    QAction *light = new QAction(tr("Light"));
    light->setData(QtCharts::QChart::ChartThemeLight);
    m_theme->addAction(light);
    
    QAction *blue = new QAction(tr("Blue Cerulean"));
    blue->setData( QtCharts::QChart::ChartThemeBlueCerulean);
    m_theme->addAction(blue);
    
    QAction *dark = new QAction(tr("Dark"));
    dark->setData( QtCharts::QChart::ChartThemeDark);
    m_theme->addAction(dark);
    
    QAction *brown = new QAction(tr("Brown Sand"));
    brown->setData( QtCharts::QChart::ChartThemeBrownSand);
    m_theme->addAction(brown);
    
    QAction *bluencs = new QAction(tr("Blue NCS"));
    bluencs->setData( QtCharts::QChart::ChartThemeBlueNcs);
    m_theme->addAction(bluencs);

    QAction *high = new QAction(tr("High Contrast"));
    high->setData( QtCharts::QChart::ChartThemeHighContrast);
    m_theme->addAction(high);
    
    QAction *icy = new QAction(tr("Blue Icy"));
    icy->setData( QtCharts::QChart::ChartThemeBlueIcy);
    m_theme->addAction(icy);
    
    QAction *qt = new QAction(tr("Qt"));
    qt->setData( QtCharts::QChart::ChartThemeQt);
    m_theme->addAction(qt);

    toolsmenu->addMenu(m_theme);
    
    m_size = new QMenu("Chart Size");

    QAction *tiny = new QAction("Tiny");
    tiny->setData(512);
    m_size->addAction(tiny);

    QAction *small = new QAction("Small");
    small->setData(650);
    m_size->addAction(small);

    QAction *medium = new QAction("Medium");
    medium->setData(850);
    m_size->addAction(medium);

    QAction *max = new QAction("Maximum");
    max->setData(1024);
    m_size->addAction(max);

    QAction *any = new QAction("Any");
    any->setData(16777215);
    m_size->addAction(any);

    toolsmenu->addMenu(m_size);

    m_tools->setMenu(toolsmenu);
    
    m_hide = new QPushButton;
    m_hide->setFlat(true);
    m_hide->setIcon(QIcon::fromTheme("tab-close"));
    
    
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(new QLabel("Charts"));
    layout->addStretch();
    layout->addWidget(m_tools);
    layout->addWidget(m_hide);
    setLayout(layout);
    
    connect(m_hide, &QPushButton::clicked, this, &ChartDockTitleBar::close);
    connect(m_theme, &QMenu::triggered, this, &ChartDockTitleBar::ThemeChange);
    connect(m_size, &QMenu::triggered, [this](QAction *action)
    {
        emit setSize(action->data().toInt());
    });

}


void ChartDockTitleBar::ThemeChange(QAction *action)
{
    QtCharts::QChart::ChartTheme  theme = QtCharts::QChart::ChartTheme (action->data().toInt());
    emit ThemeChanged(theme);
}


ChartWidget::ChartWidget() : m_TitleBarWidget(new ChartDockTitleBar)
{
    
    m_signalchart = new QtCharts::QChart;
    m_errorchart = new QtCharts::QChart;

    m_signalview = new ChartView(m_signalchart, true);
    m_errorview = new ChartView(m_errorchart, true);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_signalview,1, 0);
    layout->addWidget(m_errorview, 2, 0);
    
    m_signalchart->setTheme((QtCharts::QChart::ChartTheme) qApp->instance()->property("charttheme").toInt());
    m_errorchart->setTheme((QtCharts::QChart::ChartTheme) qApp->instance()->property("charttheme").toInt());
    
    restartAnimation();
    setLayout(layout);
    max_shift = 0;
    min_shift = 0;
    
    connect(m_TitleBarWidget, &ChartDockTitleBar::ThemeChanged, this, &ChartWidget::updateTheme);
    connect(m_TitleBarWidget, &ChartDockTitleBar::AnimationChanged, this, &ChartWidget::setAnimation);
    connect(m_TitleBarWidget, &ChartDockTitleBar::setSize, this, [this](int size)
    {
        setMinimumWidth(size);
        setMaximumWidth(size);
    });

    setMinimumWidth(512);
    setMaximumWidth(1024);
}

ChartWidget::~ChartWidget()
{
}


QSharedPointer<ChartWrapper > ChartWidget::setRawData(QSharedPointer<DataClass> rawdata) 
{
    m_rawdata = rawdata;
    
    m_data_mapper = QSharedPointer<ChartWrapper>(new ChartWrapper(false, this), &QObject::deleteLater);
    m_data_mapper->setDataTable(m_rawdata.data()->DependentModel());
    m_data_mapper->setData(m_rawdata);
    connect(m_data_mapper.data(), SIGNAL(stopAnimiation()), this, SLOT(stopAnimiation()));
    connect(m_data_mapper.data(), SIGNAL(restartAnimation()), this, SLOT(restartAnimation()));
    for(int i = 0; i < m_data_mapper->SeriesSize(); ++i)
    {
        ScatterSeries *signal_series = (qobject_cast<ScatterSeries *>(m_data_mapper->Series(i)));
        m_data_mapper->setSeries(signal_series, i);
        m_signalview->addSeries(signal_series);
    }
    
    m_signalview->formatAxis();

    m_signalview->setTitle("<h4>Model</h4>");
    m_errorview->setTitle("<h4>Errors</h4>");

    return m_data_mapper;
}

Charts ChartWidget::addModel(QSharedPointer<AbstractModel > model)
{
    m_models << model;
    connect(model.data(), SIGNAL(Recalculated()), this, SLOT(Repaint()));
    ChartWrapper *signal_wrapper = new ChartWrapper(false, this);
    connect(m_data_mapper.data(), SIGNAL(ModelChanged()), signal_wrapper, SLOT(UpdateModel()));
    connect(m_data_mapper.data(), SIGNAL(ShowSeries(int)), signal_wrapper, SLOT(showSeries(int)));
    signal_wrapper->setDataTable(model->ModelTable());
    m_data_mapper->TransformModel(model);
    signal_wrapper->setData(model);
    
    ChartWrapper *error_wrapper = new ChartWrapper(false, this);
    connect(m_data_mapper.data(), SIGNAL(ModelChanged()), error_wrapper, SLOT(UpdateModel()));
    connect(m_data_mapper.data(), SIGNAL(ShowSeries(int)), error_wrapper, SLOT(showSeries(int)));
    error_wrapper->setDataTable(model->ErrorTable());
    error_wrapper->setData(model);
    
    for(int i = 0; i <m_data_mapper->SeriesSize(); ++i)
    {
        if(model->Type() != 3)
        {
            LineSeries *model_series = (qobject_cast<LineSeries *>(signal_wrapper->Series(i)));
            signal_wrapper->setSeries(model_series, i);
            connect(m_data_mapper->Series(i), SIGNAL(NameChanged(QString)), model_series, SLOT(setName(QString)));
            connect(m_data_mapper->Series(i), SIGNAL(visibleChanged(int)), model_series, SLOT(ShowLine(int)));
            model_series->setName(m_data_mapper.data()->Series(i)->name());
            model_series->setColor(m_data_mapper->color(i));
            connect(m_data_mapper->Series(i), SIGNAL(colorChanged(QColor)), model_series, SLOT(setColor(QColor)));
            m_signalview->addSeries(model_series);
        }
        if(model->Type() != 3)
        {
            LineSeries *error_series = (qobject_cast<LineSeries *>(error_wrapper->Series(i)));
            error_wrapper->setSeries(error_series, i);
            error_series->setName(m_data_mapper.data()->Series(i)->name());
            error_series->setColor(m_data_mapper->color(i));
            connect(m_data_mapper->Series(i), SIGNAL(colorChanged(QColor)), error_series, SLOT(setColor(QColor)));
            connect(m_data_mapper->Series(i), SIGNAL(visibleChanged(int)), error_series, SLOT(ShowLine(int)));
            m_errorview->addSeries(error_series);
        }
    }

    Charts charts;
    charts.error_wrapper = error_wrapper;
    charts.signal_wrapper = signal_wrapper;
    charts.data_wrapper = m_data_mapper.data();
    connect(model.data(), &AbstractModel::Recalculated, model.data(), [model, this] ()
    {
        m_signal_x = model->XLabel();
        m_signal_y = model->YLabel();
        m_error_x = model->XLabel();
        m_error_y ="Error " +  model->YLabel();
    });

    m_signal_x = model->XLabel();
    m_signal_y = model->YLabel();
    m_error_x = model->XLabel();
    m_error_y ="Error " +  model->YLabel();

    Repaint();

    return charts;
}

void ChartWidget::Repaint()
{         
    if(!m_data_mapper)
        return;

    if(m_models.size())
    {
        m_signalview->setXAxis(m_signal_x);
        m_errorview->setXAxis(m_error_x);

        m_signalview->setYAxis(m_signal_y);
        m_errorview->setYAxis(m_error_y);
    }
    m_data_mapper->UpdateModel();

    formatAxis();
}

void ChartWidget::formatAxis()
{
    QTimer::singleShot(1,m_signalview, SLOT(formatAxis()));
    QTimer::singleShot(1,m_errorview, SLOT(formatAxis()));
}


void ChartWidget::updateTheme(QtCharts::QChart::ChartTheme  theme)
{  
    qApp->instance()->setProperty("charttheme", theme);
    emit Instance::GlobalInstance()->ConfigurationChanged();
}

void ChartWidget::setAnimation(bool animation)
{
    qApp->instance()->setProperty("chartanimation", animation);
    emit Instance::GlobalInstance()->ConfigurationChanged();
}

void ChartWidget::updateUI()
{
    for(int i = 0; i < m_rawdata.data()->SeriesCount(); ++i)
        m_data_mapper->Series(i)->setColor(m_data_mapper->color(i));
}

void ChartWidget::stopAnimiation()
{
    m_signalchart->setAnimationOptions(QtCharts::QChart::NoAnimation);
    m_errorchart->setAnimationOptions(QtCharts::QChart::NoAnimation);
}

void ChartWidget::restartAnimation()
{
    if(qApp->instance()->property("chartanimation").toBool())
    {
        m_signalchart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
        m_errorchart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    }
}

#include "chartwidget.moc"
