/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <charts.h>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

#include "src/core/instance.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/guitools.h"

#include <QtCharts/QCategoryAxis>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QtCore/QBuffer>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>

#include <QtGui/QAction>
#include <QtGui/QDrag>

#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtPrintSupport/QPrinter>

#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableView>

#include <iostream>

#include "chartwidget.h"

ChartDockTitleBar::ChartDockTitleBar()
{
    m_tools = new QPushButton(tr("Tools and Chart Settings"));
    m_tools->setFlat(true);
    m_tools->setIcon(QIcon::fromTheme("applications-system"));
    QMenu* toolsmenu = new QMenu(this);

    toolsmenu->addSeparator();

    m_animation = new QAction(tr("Animation"), this);
    m_animation->setCheckable(true);
    m_animation->setChecked(qApp->instance()->property("chartanimation").toBool());

    connect(m_animation, &QAction::toggled, this, &ChartDockTitleBar::AnimationChanged);
    toolsmenu->addAction(m_animation);

    m_theme = new QMenu("Chart Theme");

    QAction* light = new QAction(tr("Light"), this);
    light->setData(QChart::ChartThemeLight);
    m_theme->addAction(light);

    QAction* blue = new QAction(tr("Blue Cerulean"), this);
    blue->setData(QChart::ChartThemeBlueCerulean);
    m_theme->addAction(blue);

    QAction* dark = new QAction(tr("Dark"), this);
    dark->setData(QChart::ChartThemeDark);
    m_theme->addAction(dark);

    QAction* brown = new QAction(tr("Brown Sand"), this);
    brown->setData(QChart::ChartThemeBrownSand);
    m_theme->addAction(brown);

    QAction* bluencs = new QAction(tr("Blue NCS"), this);
    bluencs->setData(QChart::ChartThemeBlueNcs);
    m_theme->addAction(bluencs);

    QAction* high = new QAction(tr("High Contrast"), this);
    high->setData(QChart::ChartThemeHighContrast);
    m_theme->addAction(high);

    QAction* icy = new QAction(tr("Blue Icy"), this);
    icy->setData(QChart::ChartThemeBlueIcy);
    m_theme->addAction(icy);

    QAction* qt = new QAction(tr("Qt"), this);
    qt->setData(QChart::ChartThemeQt);
    m_theme->addAction(qt);

    toolsmenu->addMenu(m_theme);

    m_size = new QMenu("Chart Size", this);

    QAction* tiny = new QAction("Tiny", this);
    tiny->setData(512);
    m_size->addAction(tiny);

    QAction* small = new QAction("Small", this);
    small->setData(650);
    m_size->addAction(small);

    QAction* medium = new QAction("Medium", this);
    medium->setData(850);
    m_size->addAction(medium);

    QAction* max = new QAction("Maximum", this);
    max->setData(1024);
    m_size->addAction(max);

    QAction* any = new QAction("Any", this);
    any->setData(16777215);
    m_size->addAction(any);

    toolsmenu->addMenu(m_size);

    m_tools->setMenu(toolsmenu);

    m_hide = new QPushButton;
    m_hide->setFlat(true);
    m_hide->setIcon(QIcon::fromTheme("tab-close"));

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(new QLabel("Charts"));
    layout->addStretch();
    layout->addWidget(m_tools);
    layout->addWidget(m_hide);
    setLayout(layout);

    connect(m_hide, &QPushButton::clicked, this, &ChartDockTitleBar::close);
    connect(m_theme, &QMenu::triggered, this, &ChartDockTitleBar::ThemeChange);
    connect(m_size, &QMenu::triggered, [this](QAction* action) {
        emit setSize(action->data().toInt());
    });
}

void ChartDockTitleBar::ThemeChange(QAction* action)
{
    QChart::ChartTheme theme = QChart::ChartTheme(action->data().toInt());
    emit ThemeChanged(theme);
}

ChartWidget::ChartWidget()
    : m_TitleBarWidget(new ChartDockTitleBar)
{
    m_signalview = new ChartView;
    m_signalview->setName("signalview");
    connect(m_signalview, &ChartView::LastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, m_signalview, &ChartView::ApplyConfigurationChange);

    m_errorview = new ChartView;
    m_errorview->setName("errorview");
    connect(m_errorview, &ChartView::LastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, m_errorview, &ChartView::ApplyConfigurationChange);

    QGridLayout* layout = new QGridLayout;
    layout->addWidget(m_signalview, 1, 0);
    layout->addWidget(m_errorview, 2, 0);

    m_signalview->Chart()->setTheme((QChart::ChartTheme)qApp->instance()->property("charttheme").toInt());
    m_errorview->Chart()->setTheme((QChart::ChartTheme)qApp->instance()->property("charttheme").toInt());

    restartAnimation();
    setLayout(layout);
    max_shift = 0;
    min_shift = 0;

    connect(m_TitleBarWidget, &ChartDockTitleBar::ThemeChanged, this, &ChartWidget::updateTheme);
    connect(m_TitleBarWidget, &ChartDockTitleBar::AnimationChanged, this, &ChartWidget::setAnimation);
    connect(m_TitleBarWidget, &ChartDockTitleBar::setSize, this, [this](int size) {
        setMinimumWidth(size);
        setMaximumWidth(size);
    });

    //setMinimumWidth(512);
    //setMaximumWidth(1024);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

ChartWidget::~ChartWidget()
{
    m_data_mapper.clear();
    m_rawdata.clear();
}

QSharedPointer<ChartWrapper> ChartWidget::setRawData(QSharedPointer<DataClass> rawdata)
{
    m_rawdata = rawdata;

    m_data_mapper = QSharedPointer<ChartWrapper>(new ChartWrapper(this), &QObject::deleteLater);
    qDebug() << m_rawdata.toStrongRef().data()->DependentModel();
    m_data_mapper->setDataTable(m_rawdata.toStrongRef().data()->DependentModel());
    /*
    connect(rawdata->Info(), &DataClassPrivateObject::Update, this, [this](){
        m_data_mapper->setDataTable(m_rawdata.toStrongRef().data()->DependentModel());
    });
    */
    m_data_mapper->setData(m_rawdata);
    connect(m_data_mapper.data(), SIGNAL(stopAnimiation()), this, SLOT(stopAnimiation()));
    connect(m_data_mapper.data(), SIGNAL(restartAnimation()), this, SLOT(restartAnimation()));
    for (int i = 0; i < m_data_mapper->SeriesSize(); ++i) {
        ScatterSeries* signal_series = (qobject_cast<ScatterSeries*>(m_data_mapper->Series(i)));
        signal_series->setName(m_rawdata.toStrongRef().data()->DependentModel()->header()[i]);
        m_data_mapper->setSeries(signal_series, i);
        m_signalview->addSeries(signal_series);
    }
    m_recent_color = m_data_mapper->Series(m_data_mapper->SeriesSize() - 1)->color();

    m_signalview->formatAxis();

    m_signalview->setTitle("Model");
    m_errorview->setTitle("Errors");

    return m_data_mapper;
}

QSharedPointer<ChartWrapper> ChartWidget::setRawWrapper(const QWeakPointer<ChartWrapper>& wrapper)
{
    m_data_mapper = QSharedPointer<ChartWrapper>(new ChartWrapper(this), &QObject::deleteLater);

    if (wrapper)
        m_data_mapper->addWrapper(wrapper);

    for (int i = 0; i < m_data_mapper->SeriesSize(); ++i) {
        ScatterSeries* signal_series = (qobject_cast<ScatterSeries*>(m_data_mapper->Series(i)));
        m_data_mapper->setSeries(signal_series, i);
        m_signalview->addSeries(signal_series);
    }

    connect(m_data_mapper.data(), &ChartWrapper::SeriesAdded, m_data_mapper.data(), [this](int series) {
        m_signalview->addSeries(qobject_cast<ScatterSeries*>(m_data_mapper.data()->Series(series)));
    });
    if (m_data_mapper->SeriesSize())
        m_recent_color = m_data_mapper->Series(m_data_mapper->SeriesSize() - 1)->color();
    else
        m_recent_color = QColor();

    m_signalview->formatAxis();

    m_signalview->setTitle("Model");
    m_errorview->setTitle("Errors");

    return m_data_mapper;
}

Charts ChartWidget::addModel(QSharedPointer<AbstractModel> model)
{
    m_data_mapper->TransformModel(model);
    double lineWidth = qApp->instance()->property("lineWidth").toDouble() / 10.0;

    m_empty = false;
    connect(model.data(), SIGNAL(Recalculated()), this, SLOT(Repaint()));
    QSharedPointer<ChartWrapper> signal_wrapper = QSharedPointer<ChartWrapper>(new ChartWrapper(this), &QObject::deleteLater);
    connect(m_data_mapper.data(), SIGNAL(ModelChanged()), signal_wrapper.data(), SLOT(UpdateModel()));
    connect(m_data_mapper.data(), SIGNAL(ShowSeries(int)), signal_wrapper.data(), SLOT(showSeries(int)));
    signal_wrapper->setDataTable(model->ModelTable());
    signal_wrapper->setData(model);

    QSharedPointer<ChartWrapper> error_wrapper = QSharedPointer<ChartWrapper>(new ChartWrapper(this), &QObject::deleteLater);
    connect(m_data_mapper.data(), SIGNAL(ModelChanged()), error_wrapper.data(), SLOT(UpdateModel()));
    connect(m_data_mapper.data(), SIGNAL(ShowSeries(int)), error_wrapper.data(), SLOT(showSeries(int)));
    error_wrapper->setDataTable(model->ErrorTable());
    error_wrapper->setData(model);

    /*
    connect(model->Info(), &DataClassPrivateObject::Update, this, [model, signal_wrapper, error_wrapper](){
        signal_wrapper->setDataTable(model->ModelTable());
        error_wrapper->setDataTable(model->ErrorTable());
    });*/

    for (int i = 0; i < model->SeriesCount(); ++i) {
        //if (model->Type() != 3) {
        LineSeries* model_series = (qobject_cast<LineSeries*>(signal_wrapper->Series(i)));
        signal_wrapper->setSeries(model_series, i);
        connect(m_data_mapper->Series(i), SIGNAL(NameChanged(QString)), model_series, SLOT(setName(QString)));
        connect(m_data_mapper->Series(i), SIGNAL(visibleChanged(int)), model_series, SLOT(ShowLine(int)));
        model_series->setName(m_data_mapper.data()->Series(i)->name());
        model_series->setColor(m_data_mapper.data()->Series(i)->color());
        connect(m_data_mapper->Series(i), SIGNAL(colorChanged(QColor)), model_series, SLOT(setColor(QColor)));
        model_series->setSize(lineWidth);
        m_signalview->addSeries(model_series);
        //}
        if (model->Type() != DataClassPrivate::DataType::Simulation) {
            LineSeries* error_series = (qobject_cast<LineSeries*>(error_wrapper->Series(i)));
            error_wrapper->setSeries(error_series, i);
            error_series->setName(m_data_mapper.data()->Series(i)->name());
            error_series->setColor(m_data_mapper.data()->Series(i)->color());
            connect(m_data_mapper->Series(i), SIGNAL(colorChanged(QColor)), error_series, SLOT(setColor(QColor)));
            connect(m_data_mapper->Series(i), SIGNAL(visibleChanged(int)), error_series, SLOT(ShowLine(int)));
            error_series->setSize(lineWidth);
            m_errorview->addSeries(error_series);
        }
    }

    m_recent_color = m_data_mapper->Series(m_data_mapper->SeriesSize() - 1)->color();

    Charts charts;
    charts.error_wrapper = error_wrapper;
    charts.signal_wrapper = signal_wrapper;
    charts.data_wrapper = m_data_mapper;
    connect(model.data(), &AbstractModel::Recalculated, model.data(), [model, this]() {
        m_signal_x = model->XLabel();
        m_signal_y = model->YLabel();
        m_error_x = model->XLabel();
        m_error_y = tr("%1 (y<sub>calc</sub> - y<sub>exp</sub>)").arg(model->YLabel());
        Repaint();
    });

    m_signal_x = model->XLabel();
    m_signal_y = model->YLabel();
    m_error_x = model->XLabel();
    m_error_y = tr("%1 (y<sub>calc</sub> - y<sub>exp</sub>)").arg(model->YLabel());

    Repaint();

    return charts;
}

void ChartWidget::Repaint()
{
    if (!m_data_mapper)
        return;

    if (!m_empty) {
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
    QTimer::singleShot(1, m_signalview, SLOT(formatAxis()));
    QTimer::singleShot(1, m_errorview, SLOT(formatAxis()));
}

void ChartWidget::updateTheme(QChart::ChartTheme theme)
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
    if (m_rawdata) {
        for (int i = 0; i < m_rawdata.toStrongRef().data()->SeriesCount(); ++i)
            m_data_mapper->Series(i)->setColor(m_data_mapper->color(i));
    }
}

void ChartWidget::stopAnimiation()
{
    m_signalview->Chart()->setAnimationOptions(QChart::NoAnimation);
    m_errorview->Chart()->setAnimationOptions(QChart::NoAnimation);
}

void ChartWidget::restartAnimation()
{
    if (qApp->instance()->property("chartanimation").toBool()) {
        m_signalview->Chart()->setAnimationOptions(QChart::SeriesAnimations);
        m_errorview->Chart()->setAnimationOptions(QChart::SeriesAnimations);
    }
}

#include "chartwidget.moc"
