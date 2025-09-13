/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2022  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/instance.h"

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

    m_theme = new QMenu(tr("Chart Theme"));

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

    m_tools->setMenu(toolsmenu);

    m_hide = new QPushButton;
    m_hide->setFlat(true);
    m_hide->setIcon(QIcon::fromTheme("tab-close"));

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(new QLabel(tr("Charts")));
    layout->addStretch();
    layout->addWidget(m_tools);
    layout->addWidget(m_hide);
    setLayout(layout);

    connect(m_hide, &QPushButton::clicked, this, &ChartDockTitleBar::close);
    connect(m_theme, &QMenu::triggered, this, &ChartDockTitleBar::ThemeChange);
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
    connect(m_signalview, &ChartView::lastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    Instance::GlobalInstance()->MakeChartConnections(m_signalview);

    m_errorview = new ChartView;
    m_errorview->setName("errorview");
    connect(m_errorview, &ChartView::lastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    Instance::GlobalInstance()->MakeChartConnections(m_errorview);

    QGridLayout* layout = new QGridLayout;
    layout->addWidget(m_signalview, 1, 0);
    layout->addWidget(m_errorview, 2, 0);

    m_signalview->chart()->setTheme((QChart::ChartTheme)qApp->instance()->property("charttheme").toInt());
    m_errorview->chart()->setTheme((QChart::ChartTheme)qApp->instance()->property("charttheme").toInt());

    restartAnimation();
    setLayout(layout);
    max_shift = 0;
    min_shift = 0;

    connect(m_TitleBarWidget, &ChartDockTitleBar::ThemeChanged, this, &ChartWidget::updateTheme);
    connect(m_TitleBarWidget, &ChartDockTitleBar::AnimationChanged, this, &ChartWidget::setAnimation);

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
    m_data_mapper->setDataTable(m_rawdata.toStrongRef().data()->DependentModel());

    m_data_mapper->setData(m_rawdata);
    connect(m_data_mapper.data(), SIGNAL(stopAnimiation()), this, SLOT(stopAnimiation()));
    connect(m_data_mapper.data(), SIGNAL(restartAnimation()), this, SLOT(restartAnimation()));
    for (int i = 0; i < m_data_mapper->SeriesSize(); ++i) {
        ScatterSeries* signal_series = (qobject_cast<ScatterSeries*>(m_data_mapper->Series(i)));
        signal_series->setName(m_rawdata.toStrongRef().data()->DependentModel()->header()[i]);
        m_data_mapper->setSeries(signal_series, i);
        m_signalview->addSeries(signal_series);
    }
    if (m_data_mapper->SeriesSize())
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
    // Claude Generated - Add comprehensive debug output to identify crash location
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Starting addModel for" << (model ? model->Name() : "NULL");
#endif
    
    if (!model) {
        qWarning() << "❌ ChartWidget::addModel: Model is null";
        return Charts{};
    }
    
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Checking m_data_mapper" << (m_data_mapper ? "OK" : "NULL");
#endif
    if (!m_data_mapper) {
        qWarning() << "❌ ChartWidget::addModel: m_data_mapper is null";
        return Charts{};
    }
    
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Calling TransformModel";
#endif
    m_data_mapper->TransformModel(model);
#ifdef DEBUG_ON
    qDebug() << "✅ DEBUG ChartWidget::addModel: TransformModel completed";
#endif
    
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Getting lineWidth";
#endif
    double lineWidth = qApp->instance()->property("lineWidth").toDouble() / 10.0;
#ifdef DEBUG_ON
    qDebug() << "✅ DEBUG ChartWidget::addModel: lineWidth =" << lineWidth;
#endif

#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Setting up connections and wrappers";
#endif
    m_empty = false;
    connect(model.data(), SIGNAL(Recalculated()), this, SLOT(Repaint()));
    
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Creating signal_wrapper";
#endif
    QSharedPointer<ChartWrapper> signal_wrapper = QSharedPointer<ChartWrapper>(new ChartWrapper(this), &QObject::deleteLater);
#ifdef DEBUG_ON
    qDebug() << "✅ DEBUG ChartWidget::addModel: signal_wrapper created";
#endif
    
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Setting up signal_wrapper connections";
#endif
    connect(m_data_mapper.data(), SIGNAL(ModelChanged()), signal_wrapper.data(), SLOT(UpdateModel()));
    connect(m_data_mapper.data(), SIGNAL(ShowSeries(int)), signal_wrapper.data(), SLOT(showSeries(int)));
    
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Getting ModelTable";
#endif
    DataTable* modelTable = model->ModelTable();
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: ModelTable is" << (modelTable ? "valid" : "NULL");
#endif
    signal_wrapper->setDataTable(modelTable);
    
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Setting data on signal_wrapper";
#endif
    signal_wrapper->setData(model);
#ifdef DEBUG_ON
    qDebug() << "✅ DEBUG ChartWidget::addModel: signal_wrapper setup complete";
#endif

#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Creating error_wrapper";
#endif
    QSharedPointer<ChartWrapper> error_wrapper = QSharedPointer<ChartWrapper>(new ChartWrapper(this), &QObject::deleteLater);
#ifdef DEBUG_ON
    qDebug() << "✅ DEBUG ChartWidget::addModel: error_wrapper created";
#endif
    
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Setting up error_wrapper connections";
#endif
    connect(m_data_mapper.data(), SIGNAL(ModelChanged()), error_wrapper.data(), SLOT(UpdateModel()));
    connect(m_data_mapper.data(), SIGNAL(ShowSeries(int)), error_wrapper.data(), SLOT(showSeries(int)));
    
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Getting ErrorTable";
#endif
    DataTable* errorTable = model->ErrorTable();
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: ErrorTable is" << (errorTable ? "valid" : "NULL");
#endif
    error_wrapper->setDataTable(errorTable);
    
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Setting data on error_wrapper";
#endif
    error_wrapper->setData(model);
#ifdef DEBUG_ON
    qDebug() << "✅ DEBUG ChartWidget::addModel: error_wrapper setup complete";
#endif


#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG ChartWidget::addModel: Starting series loop, SeriesCount:" << model->SeriesCount();
#endif
    for (int i = 0; i < model->SeriesCount(); ++i) {
#ifdef DEBUG_ON
        qDebug() << "🔍 DEBUG ChartWidget::addModel: Processing series" << i;
        
        qDebug() << "🔍 DEBUG ChartWidget::addModel: Getting series from signal_wrapper";
#endif
        QXYSeries* base_series = signal_wrapper->Series(i);

        // Claude Generated: Dynamic series type detection and handling
        if (LineSeries* line_series = qobject_cast<LineSeries*>(base_series)) {
#ifdef DEBUG_ON
            qDebug() << "🔍 DEBUG ChartWidget::addModel: Detected LineSeries" << i;
#endif
            signal_wrapper->setSeries(line_series, i);

            // LineSeries doesn't have name/visibility change signals, use direct slot calls
            line_series->setName(m_data_mapper.data()->Series(i)->name());
            line_series->setColor(m_data_mapper.data()->Series(i)->color());
            connect(m_data_mapper->Series(i), SIGNAL(colorChanged(QColor)), line_series, SLOT(setColor(QColor)));
            line_series->setLineWidth(lineWidth);
            line_series->setVisible(true); // Claude Generated: Ensure LineSeries is initially visible
            m_signalview->addSeries(line_series);

        } else if (ScatterSeries* scatter_series = qobject_cast<ScatterSeries*>(base_series)) {
#ifdef DEBUG_ON
            qDebug() << "🔍 DEBUG ChartWidget::addModel: Detected ScatterSeries" << i;
#endif
            signal_wrapper->setSeries(scatter_series, i);

            // ScatterSeries has the correct signals: nameChanged and visibilityChanged
            connect(m_data_mapper->Series(i), SIGNAL(nameChanged(QString)), scatter_series, SLOT(setName(QString)));
            connect(m_data_mapper->Series(i), SIGNAL(visibilityChanged(int)), scatter_series, SLOT(showLine(int)));
            scatter_series->setName(m_data_mapper.data()->Series(i)->name());
            scatter_series->setColor(m_data_mapper.data()->Series(i)->color());
            connect(m_data_mapper->Series(i), SIGNAL(colorChanged(QColor)), scatter_series, SLOT(setColor(QColor)));
            m_signalview->addSeries(scatter_series);

        } else {
#ifdef DEBUG_ON
            qDebug() << "⚠️  WARNING ChartWidget::addModel: Unknown series type for series" << i;
            if (base_series) {
                qDebug() << "🔍 DEBUG ChartWidget::addModel: Series type:" << base_series->metaObject()->className();
#endif
                signal_wrapper->setSeries(base_series, i);
                m_signalview->addSeries(base_series);
            }
        }

        // Claude Generated: Handle error series with same type detection logic
        if (model->Type() != DataClassPrivate::DataType::Simulation) {
            QXYSeries* base_error_series = error_wrapper->Series(i);

            if (LineSeries* error_line_series = qobject_cast<LineSeries*>(base_error_series)) {
#ifdef DEBUG_ON
                qDebug() << "🔍 DEBUG ChartWidget::addModel: Detected LineSeries for error" << i;
#endif
                error_wrapper->setSeries(error_line_series, i);
                error_line_series->setName(m_data_mapper.data()->Series(i)->name());
                error_line_series->setColor(m_data_mapper.data()->Series(i)->color());
                connect(m_data_mapper->Series(i), SIGNAL(colorChanged(QColor)), error_line_series, SLOT(setColor(QColor)));
                error_line_series->setLineWidth(lineWidth);
                error_line_series->setVisible(true); // Claude Generated: Ensure error LineSeries is initially visible
                m_errorview->addSeries(error_line_series);

            } else if (ScatterSeries* error_scatter_series = qobject_cast<ScatterSeries*>(base_error_series)) {
#ifdef DEBUG_ON
                qDebug() << "🔍 DEBUG ChartWidget::addModel: Detected ScatterSeries for error" << i;
#endif
                error_wrapper->setSeries(error_scatter_series, i);
                connect(m_data_mapper->Series(i), SIGNAL(visibilityChanged(int)), error_scatter_series, SLOT(showLine(int)));
                error_scatter_series->setName(m_data_mapper.data()->Series(i)->name());
                error_scatter_series->setColor(m_data_mapper.data()->Series(i)->color());
                connect(m_data_mapper->Series(i), SIGNAL(colorChanged(QColor)), error_scatter_series, SLOT(setColor(QColor)));
                m_errorview->addSeries(error_scatter_series);

            } else {
#ifdef DEBUG_ON
                qDebug() << "⚠️  WARNING ChartWidget::addModel: Unknown error series type for series" << i;
                if (base_error_series) {
                    qDebug() << "🔍 DEBUG ChartWidget::addModel: Error series type:" << base_error_series->metaObject()->className();
#endif
                    error_wrapper->setSeries(base_error_series, i);
                    m_errorview->addSeries(base_error_series);
                }
            }
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
    m_signalview->chart()->setAnimationOptions(QChart::NoAnimation);
    m_errorview->chart()->setAnimationOptions(QChart::NoAnimation);
}

void ChartWidget::restartAnimation()
{
    if (qApp->instance()->property("chartanimation").toBool()) {
        m_signalview->chart()->setAnimationOptions(QChart::SeriesAnimations);
        m_errorview->chart()->setAnimationOptions(QChart::SeriesAnimations);
    }
}

#include "chartwidget.moc"
