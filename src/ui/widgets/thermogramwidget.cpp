/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2021 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <Eigen/Dense>

#include <QtCore/QDateTime>
#include <QtCore/QSettings>
#include <QtCore/QtMath>

#include <QtGui/QAction>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableWidget>

#include <QtCharts/QChart>

#include "libpeakpick/analyse.h"
#include "libpeakpick/baseline.h"
#include "libpeakpick/peakpick.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/guitools.h"
#include "src/ui/guitools/waiter.h"

#include "src/core/instance.h"
#include "src/core/libmath.h"
#include "src/core/thermogramhandler.h"
#include "src/core/toolset.h"

#include "src/global.h"

#include "thermogramwidget.h"

ThermogramWidget::ThermogramWidget(QPointer<ThermogramHandler> thermogram, QWidget* parent)
    : QWidget(parent)
    , m_stored_thermogram(thermogram)
{
    setUi();
    CreateSeries();
    InitialiseChart();
}

void ThermogramWidget::setUi()
{
    m_guide_label = new QLabel;
    if (qApp->instance()->property("thermogram_guidelines").toBool()) {
        m_guide_label->setFixedHeight(70);
        m_guide_label->setWordWrap(true);
        m_guide_label->setMaximumWidth(1100);
        m_guide_label->setToolTip(tr("Hey, I am like Clippit (german: Karl Klammer). If you do not want to see me, just disable me in the general config (=> [General Settings] => [Show guidelines in Thermogram Dialog])!"));
    } else
        m_guide_label->setFixedHeight(30);

    m_guide_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_thermogram = new ChartView;
    m_thermogram->setAutoScaleStrategy(AutoScaleStrategy::QtNiceNumbers);

    connect(m_thermogram, &ChartView::LastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    connect(m_thermogram, &ChartView::AddRect, this, &ThermogramWidget::AddRectanglePeak);
    connect(m_thermogram, &ChartView::scaleUp, this, &ThermogramWidget::scaleUp);
    connect(m_thermogram, &ChartView::scaleDown, this, &ThermogramWidget::scaleDown);

    connect(m_thermogram, &ChartView::EscapeSelectMode, this, [this]() {
        m_thermogram->setSelectStrategy(SelectStrategy::S_None);
        m_thermogram->setZoomStrategy(ZoomStrategy::Z_Rectangular);

        m_peak_start_line->hide();
        m_peak_end_line->hide();
        m_peak_edit_mode = false;
        ResetGuideLabel();
    });

    connect(m_thermogram, &ChartView::RightKey, this, [this]() {
        if (m_peak_edit_mode)
            PeakDoubleClicked(m_current_peak + 1);
    });

    connect(m_thermogram, &ChartView::LeftKey, this, [this]() {
        if (m_peak_edit_mode)
            PeakDoubleClicked(m_current_peak - 1);
    });

    m_thermogram->setVerticalLineEnabled(true);

    connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, m_thermogram, &ChartView::ConfigurationChanged);
    m_thermogram->setModal(true);
    m_thermogram->setMinimumSize(600, 450);

    m_table = new QTableWidget;
    connect(m_table, &QTableWidget::doubleClicked, this, QOverload<const QModelIndex&>::of(&ThermogramWidget::PeakDoubleClicked));
    m_table->setToolTip(tr("This table hold all peaks, consisting of the integral, start time and end time. Double click on a peak will toggle the <i>Peak Integration mode</i>. Double click of within the thermogram selects the appropriate peak in this table (if any peaks already exists"));

    m_peak_rule_list = new QTableWidget;
    PeakRule* prototype = new PeakRule;
    m_peak_rule_list->setItemPrototype(prototype);
    m_peak_rule_list->setRowCount(1);
    m_peak_rule_list->setColumnCount(2);
    m_peak_rule_list->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_peak_rule_list->setToolTip(tr("This table holds the rules (start time and peak duration) to generate the peak list. One rule will be repeated until the next start time (in the next peak rule) is due. This rule will then be applied. Incomplete peaks will be ignored. If there is the last peak incomplete, add the starting point as rule and reduce the peak duration."));

    QAction* action = new QAction("Add Rule");
    action->setIcon(Icon("list-add"));
    connect(action, &QAction::triggered, m_peak_rule_list, [this]() {
        int rows = m_peak_rule_list->rowCount();
        m_peak_rule_list->setRowCount(m_peak_rule_list->rowCount() + 1);
        PeakRule* item = new PeakRule(QString::number(m_peaks_start->value()));
        m_peak_rule_list->setItem(rows, 0, item);
        item = new PeakRule(QString::number(m_peaks_time->value()));
        m_peak_rule_list->setItem(rows, 1, item);
        m_peak_rule_list->sortByColumn(0, Qt::AscendingOrder);
        m_current_peaks_rule = m_peak_rule_list->row(item);
        m_rules_imported = false;
    });
    m_peak_rule_list->addAction(action);

    action = new QAction("Remove Rule");
    action->setIcon(Icon("trash-empty"));
    connect(action, &QAction::triggered, m_peak_rule_list, [this]() {
        if (m_peak_rule_list->rowCount() > 1) {
            int peak = m_peak_rule_list->currentRow();
            if (peak < m_peak_rule_list->rowCount())
                m_peak_rule_list->removeRow(peak);
            if (m_current_peaks_rule && peak > m_current_peaks_rule)
                m_current_peaks_rule--;
            m_rules_imported = false;
        }
    });
    m_peak_rule_list->addAction(action);

    connect(m_peak_rule_list, &QTableWidget::clicked, this, QOverload<const QModelIndex&>::of(&ThermogramWidget::PeakRuleDoubleClicked));
    connect(m_peak_rule_list, &QTableWidget::itemChanged, m_peak_rule_list, [this]() {
        m_peak_rule_list->sortByColumn(0, Qt::AscendingOrder);
    });

    QStringList header = QStringList() << "Start Time\n[s]"
                                       << "Peak Duration\n[s]";
    m_peak_rule_list->setHorizontalHeaderLabels(header);

    m_convert_rules = new QPushButton();
    m_convert_rules->setFlat(true);
    m_convert_rules->setToolTip(tr("Convert the current peak list into peak rules."));
    m_convert_rules->setIcon(Icon("document-import"));
    connect(m_convert_rules, &QPushButton::clicked, m_stored_thermogram, &ThermogramHandler::ConvertRules);

    m_load_rules = new QPushButton;
    m_load_rules->setFlat(true);
    m_load_rules->setToolTip(tr("Load a peak rule list from file."));
    m_load_rules->setIcon(Icon("document-open"));
    connect(m_load_rules, &QPushButton::clicked, this, &ThermogramWidget::LoadRules);

    m_write_rules = new QPushButton;
    m_write_rules->setFlat(true);
    m_write_rules->setToolTip(tr("Save the current peak rules to file."));
    m_write_rules->setIcon(Icon("document-save-as"));
    connect(m_write_rules, &QPushButton::clicked, this, &ThermogramWidget::WriteRules);

    m_clear_rules = new QPushButton;
    m_clear_rules->setFlat(true);
    m_clear_rules->setToolTip(tr("Remove all but the first Peak Rule."));
    m_clear_rules->setIcon(Icon("document-close"));
    connect(m_clear_rules, &QPushButton::clicked, this, [this]() {
        if (m_peak_rule_list->rowCount() == 1)
            return;

        QMessageBox question(QMessageBox::Question, tr("Clear Rules"), tr("Do you really want to clear all Peak Rules ( execept the last one)?"), QMessageBox::Yes | QMessageBox::No, this);
        if (question.exec() == QMessageBox::No)
            return;

        while (m_peak_rule_list->rowCount() > 1) {
            int peak = m_peak_rule_list->rowCount() - 1;
            if (peak < m_peak_rule_list->rowCount())
                m_peak_rule_list->removeRow(peak);
            if (m_current_peaks_rule && peak > m_current_peaks_rule)
                m_current_peaks_rule--;
        }
        m_rules_imported = false;

    });

    QGridLayout* grid = new QGridLayout;
    grid->addWidget(m_convert_rules, 0, 0);
    grid->addWidget(m_load_rules, 0, 1);
    grid->addWidget(m_write_rules, 0, 2);
    grid->addWidget(m_clear_rules, 0, 3);
    grid->addWidget(m_peak_rule_list, 1, 0, 1, 4);

    QWidget* widget = new QWidget;
    widget->setLayout(grid);

    QTabWidget* peaks_tab = new QTabWidget;
    peaks_tab->addTab(m_table, tr("Peak List"));
    peaks_tab->addTab(widget, tr("Peak  Rules"));

    QHBoxLayout* chartlayout = new QHBoxLayout;

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->tr("thermogram_splitter");
    m_splitter->addWidget(m_thermogram);
    m_splitter->addWidget(peaks_tab);
    chartlayout->addWidget(m_splitter);

    m_const_offset = new QDoubleSpinBox;
    m_const_offset->setMinimum(-1e5);
    m_const_offset->setMaximum(1e5);
    m_const_offset->setValue(0);
    m_const_offset->setDecimals(7);
    m_const_offset->setMaximumWidth(100);

    connect(m_const_offset, &QDoubleSpinBox::editingFinished, this, &ThermogramWidget::Update);
    connect(m_const_offset, &QDoubleSpinBox::editingFinished, this, &ThermogramWidget::ResetGuideLabel);

    connect(m_const_offset, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() {
        setGuideText("Press [Enter] to apply the current value which will be removed from your integrals. Please keep in mind, that the dimension of the value should be the same as within this tabulator - thus please use the raw unit observed from your calorimeter and not converted units, for example from cal into J.");
    });

    m_peaks_start = new QDoubleSpinBox;
    m_peaks_start->setMinimum(0);
    m_peaks_start->setMaximum(1e8);

    m_get_peaks_start = new QPushButton(tr("Click to Select"));
    m_get_peaks_start->setIcon(Icon("edit-select"));
    connect(m_thermogram, &ChartView::PointDoubleClicked, this, &ThermogramWidget::PointDoubleClicked);

    connect(m_get_peaks_start, &QPushButton::clicked, this, [this]() {
        if (!m_get_time_from_thermogram) {
            m_get_time_from_thermogram = 1;
            QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
            m_get_peaks_start->setText("Click to Undo");
            setGuideText(QString("You are now in <i>Thermogram start selection mode</i>. <b>Double click</b> with the <b>left</b> mouse in the chart to define, when the peak starts. Click on the <em>Click to Undo Button</em> to decline the selection. Zooming is still possible via <b>single left click</b>."));
        } else if (m_get_time_from_thermogram == 1) {
            m_get_time_from_thermogram = 0;
            m_get_peaks_start->setText("Click to Select");
            ResetGuideLabel();
            QApplication::restoreOverrideCursor();
        }

    });

    m_peaks_end = new QDoubleSpinBox;
    m_peaks_end->setMinimum(0);
    m_peaks_end->setMaximum(1e8);

    m_get_peaks_end = new QPushButton(tr("Click to Select"));
    m_get_peaks_end->setIcon(Icon("edit-select"));
    connect(m_get_peaks_end, &QPushButton::clicked, this, [this]() {
        if (!m_get_time_from_thermogram) {
            m_get_time_from_thermogram = 2;
            QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
            m_get_peaks_end->setText("Click to Undo");
            setGuideText(QString("You are now in <i>Thermogram end selection mode</i>. <b>Double click</b> with the <b>left</b> mouse in the chart to define, when the peaks end. Click on the <em>Click to Undo Button</em> to decline the selection. Zooming is still possible via <b>single left click</b>."));
        } else if (m_get_time_from_thermogram == 2) {
            m_get_time_from_thermogram = 0;
            m_get_peaks_end->setText("Click to Select");
            ResetGuideLabel();
            QApplication::restoreOverrideCursor();
        }
    });

    m_peaks_time = new QDoubleSpinBox;
    m_peaks_time->setMinimum(0);
    m_peaks_time->setMaximum(1e8);

    m_calibration_start = new QDoubleSpinBox;
    m_calibration_start->setMinimum(0);
    m_calibration_start->setMaximum(1e8);
    //m_calibration_start->setMaximumWidth(80);
    //m_calibration_start->setValue(qApp->instance()->property("calibration_start").toDouble());
    connect(m_calibration_start, qOverload<double>(&QDoubleSpinBox::valueChanged), m_calibration_start, [this](double value) {
        qApp->instance()->setProperty("calibration_start", value);
        m_peaks_end->setMaximum(m_stored_thermogram->Spectrum()->XMax() - value);
        m_peaks_end->setValue(m_stored_thermogram->Spectrum()->XMax() - value);
    });

    m_get_calibration_start = new QPushButton(tr("Click to Select"));
    m_get_calibration_start->setIcon(Icon("edit-select"));
    connect(m_thermogram, &ChartView::PointDoubleClicked, this, &ThermogramWidget::PointDoubleClicked);

    m_get_calibration_start->setMaximumWidth(100);
    m_get_calibration_start->setToolTip(tr("Click and select the starting time for the calibration from the thermogram."));

    connect(m_get_calibration_start, &QPushButton::clicked, this, [this]() {
        if (!m_get_time_from_thermogram) {
            m_calibration_start_int = 1;
            QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
            m_get_calibration_start->setText("Click to Undo");
            setGuideText(QString("You are now in <i>Calibartion start time selection mode</i>. <b>Double click</b> with the <b>left</b> mouse in the chart to define, when the calibration starts. Click on the <em>Click to Undo Button</em> to decline the selection. Zooming is still possible via <b>single left click</b>."));
        } else if (m_get_time_from_thermogram == 1) {
            m_calibration_start_int = 0;
            m_get_calibration_start->setText("Click to Select");
            ResetGuideLabel();
            QApplication::restoreOverrideCursor();
        }
    });

    m_calibration_heat = new QDoubleSpinBox;
    m_calibration_heat->setMinimum(-1e8);
    m_calibration_heat->setMaximum(1e8);
    m_calibration_heat->setDecimals(5);
    m_calibration_heat->setSingleStep(1e-2);
    m_calibration_heat->setMaximumWidth(80);
    //m_calibration_heat->setValue(qApp->instance()->property("calibration_heat").toDouble());
    connect(m_calibration_heat, qOverload<double>(&QDoubleSpinBox::valueChanged), m_calibration_heat, [this](double value) {
        qApp->instance()->setProperty("calibration_heat", value);
        m_stored_thermogram->setCalibrationHeat(value);
    });
    m_calibration_heat->setMaximumWidth(100);
    m_calibration_heat->setToolTip(tr("Set the heat in J, that were used to for the calibration. Leave zero if no calibration peak was used. If the value is not zero, the scaling factor may be redundant. Please keep that in mind."));

    /* m_calibration_label = new QLabel(tr("<h4>Calibration (0)</h4>")); */

    m_averaged = new QCheckBox(tr("Average"));
    m_averaged->setChecked(true);
    m_averaged->setEnabled(false);
    connect(m_averaged, &QCheckBox::stateChanged, this, [this](bool state) {
        m_stored_thermogram->setAveraged(state);
        Waiter wait;
        // QSignalBlocker block(m_stored_thermogram);
        UpdatePeaks();
        //m_stored_thermogram->IntegrateThermogram();
        UpdateBaseLine();
        UpdateTable();
    });

    m_iterations = new QSpinBox;
    m_iterations->setMinimum(1);
    m_iterations->setMaximum(1e3);
    m_iterations->setValue(15);
    m_iterations->setToolTip(tr("Define the maximum number of iteration for the Peak Range Reduction Cycle. In some case, this value should be 1."));
    m_iterations->setEnabled(false);

    m_integration_range = new QComboBox;
    m_integration_range->addItems(m_Peak_Cut_Options);
    m_integration_range->setMaximumWidth(100);
    connect(m_integration_range, &QComboBox::currentTextChanged, this, [this](const QString& str) {
        m_averaged->setEnabled(str == m_Peak_Cut_Options[1]);
        m_iterations->setEnabled(str == m_Peak_Cut_Options[2]);

        Waiter wait;
        //QSignalBlocker block(m_stored_thermogram);
        UpdatePeaks();
        //m_stored_thermogram->IntegrateThermogram();
        UpdateBaseLine();
        UpdateTable();
    });

    m_peak_apply = new QPushButton(tr("Apply and Update"));
    m_peak_apply->setIcon(Icon("dialog-ok-apply"));
    connect(m_peak_apply, &QPushButton::clicked, this, [this]() {
        UpdatePeaks();
    });
    m_peak_apply->setMaximumWidth(150);
    m_peak_apply->setToolTip(tr("Click generate peaks, fit baseline and integrate the peaks!"));

    m_scaling = new QComboBox;
    m_scaling->addItem(QString::number(cal2joule));
    m_scaling->addItem("1");
    m_scaling->setEditable(true);
    m_scaling->setMaximumWidth(100);

    connect(m_scaling, &QComboBox::currentTextChanged, m_scaling, [this]() {
        m_ScalingFactor = m_scaling->currentText().toDouble();
        LoadDefaultThermogram();
        m_stored_thermogram->ApplyScaling();
    });
    m_ScalingFactor = m_scaling->currentText().toDouble();

    m_integration_range_threshold = new QDoubleSpinBox;
    m_integration_range_threshold->setMinimum(0);
    m_integration_range_threshold->setMaximum(1e5);
    m_integration_range_threshold->setDecimals(10);
    m_integration_range_threshold->setValue(0);
    m_integration_range_threshold->setMaximumWidth(150);
    m_integration_range_threshold->setToolTip(tr("Define a threshold for Peak Range Reduction"));

    m_direction = new QCheckBox(tr("Before"));

    m_overshot = new QSpinBox;
    m_overshot->setRange(1, 200);
    m_overshot->setValue(1);

    m_gradient = new QDoubleSpinBox;
    m_gradient->setDecimals(5);
    m_gradient->setRange(-1, 1);
    m_gradient->setValue(-1);

    int maxwidth = 200;
    int maxheight = 120;
    QHBoxLayout* peak_layout = new QHBoxLayout;
    QVBoxLayout* vlayout = new QVBoxLayout;

    QGroupBox* group = new QGroupBox;

    group->setTitle(tr("Start time [s]"));
    group->setMaximumWidth(maxwidth);
    group->setMaximumHeight(maxheight);
    vlayout->addWidget(m_get_peaks_start);
    vlayout->addWidget(m_peaks_start);
    group->setLayout(vlayout);
    peak_layout->addWidget(group);

    group = new QGroupBox;
    group->setTitle(tr("End time [s]"));
    group->setMaximumWidth(maxwidth);
    group->setMaximumHeight(maxheight);

    vlayout = new QVBoxLayout;
    vlayout->addWidget(m_get_peaks_end);
    vlayout->addWidget(m_peaks_end);
    group->setLayout(vlayout);
    peak_layout->addWidget(group);

    group = new QGroupBox;
    group->setTitle(tr("Inject Time [s]"));
    group->setMaximumWidth(maxwidth);
    group->setMaximumHeight(maxheight);

    vlayout = new QVBoxLayout;
    vlayout->addWidget(m_peaks_time);
    vlayout->addWidget(m_peak_apply);
    group->setLayout(vlayout);
    peak_layout->addWidget(group);

    group = new QGroupBox;
    group->setTitle(tr("Peak Ranges"));
    group->setMaximumWidth(maxwidth);
    group->setMaximumHeight(maxheight);

    QHBoxLayout* triplett = new QHBoxLayout;
    vlayout = new QVBoxLayout;
    triplett->addWidget(m_integration_range);
    triplett->addWidget(m_averaged);
    vlayout->addLayout(triplett);

    triplett = new QHBoxLayout;
    triplett->addWidget(new QLabel(tr("Iterations")));
    triplett->addWidget(m_iterations);
    vlayout->addLayout(triplett);

    group->setLayout(vlayout);
    peak_layout->addWidget(group);

    group = new QGroupBox;
    group->setTitle(tr("Calibration"));
    group->setMaximumWidth(maxwidth);
    group->setMaximumHeight(maxheight);

    connect(m_stored_thermogram, &ThermogramHandler::CalibrationChanged, m_stored_thermogram, [group, this]() {
        qDebug() << m_stored_thermogram->Calibration();
        group->setTitle(tr("Calibration (%1)").arg(m_stored_thermogram->Calibration()));
    });

    vlayout = new QVBoxLayout;

    triplett = new QHBoxLayout;
    //triplett->addWidget(new QLabel("Time start [s]"));
    triplett->addWidget(m_get_calibration_start);
    triplett->addWidget(m_calibration_start);
    vlayout->addLayout(triplett);

    triplett = new QHBoxLayout;
    triplett->addWidget(new QLabel("Heat [raw]"));
    triplett->addWidget(m_calibration_heat);
    vlayout->addLayout(triplett);
    group->setLayout(vlayout);
    peak_layout->addWidget(group);

    vlayout = new QVBoxLayout;
    group = new QGroupBox;

    group->setTitle(tr("Scaling"));
    group->setMaximumWidth(maxwidth);
    group->setMaximumHeight(maxheight);

    triplett = new QHBoxLayout;
    triplett->addWidget((new QLabel(tr("Scaling"))));
    triplett->addWidget(m_scaling);
    vlayout->addLayout(triplett);

    triplett = new QHBoxLayout;
    triplett->addWidget((new QLabel(tr("Offset"))));
    triplett->addWidget(m_const_offset);
    vlayout->addLayout(triplett);

    group->setLayout(vlayout);
    peak_layout->addWidget(group);

    if (qApp->instance()->property("advanced_ui").toBool()) {
        group = new QGroupBox;
        group->setTitle(tr("Advanced stuff"));
        group->setMaximumWidth(maxwidth);
        group->setMaximumHeight(maxheight);

        vlayout = new QVBoxLayout;
        triplett = new QHBoxLayout;

        triplett->addWidget(m_integration_range_threshold);

        triplett->addWidget(m_direction);
        vlayout->addLayout(triplett);
        triplett = new QHBoxLayout;

        triplett->addWidget(m_overshot);
        triplett->addWidget(m_gradient);
        vlayout->addLayout(triplett);

        group->setLayout(vlayout);
        peak_layout->addWidget(group);
    }
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addLayout(peak_layout);
    layout->addWidget(m_guide_label);
    layout->addLayout(chartlayout);

    setLayout(layout);

    QSettings settings;
    settings.beginGroup("thermogram");

    m_peaks_start->setValue(settings.value("peaks_start", 60).toDouble());
    m_peaks_time->setValue(settings.value("peaks_time", 150).toDouble());
    m_splitter->restoreState(settings.value("splitterSizes").toByteArray());
    m_iterations->setValue(settings.value("iterations", 15).toInt());
    // m_integration_range->setCurrentText(settings.value("integration_range", m_Peak_Cut_Options[0]).toString());
    PeakRule* item = new PeakRule(QString::number(m_peaks_start->value()));
    m_peak_rule_list->setItem(m_current_peaks_rule, 0, item);
    item = new PeakRule(QString::number(m_peaks_time->value()));
    m_peak_rule_list->setItem(m_current_peaks_rule, 1, item);


    connect(m_peaks_start, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        m_peak_rule_list->item(m_current_peaks_rule, 0)->setData(Qt::DisplayRole, value);
        QSettings settings;
        settings.beginGroup("thermogram");
        settings.setValue("peaks_start", value);
    });

    connect(m_peaks_time, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        m_peak_rule_list->item(m_current_peaks_rule, 1)->setData(Qt::DisplayRole, value);
        QSettings settings;
        settings.beginGroup("thermogram");
        settings.setValue("peaks_time", value);
    });

    connect(m_iterations, qOverload<int>(&QSpinBox::valueChanged), m_stored_thermogram, [this]() {
        Waiter wait;
        m_stored_thermogram->ResetThreshold();
        UpdatePeaks();
        UpdateBaseLine();
        UpdateTable();
    });

    connect(m_stored_thermogram, &ThermogramHandler::ThermogramInitialised, this, &ThermogramWidget::LoadDefault);
    connect(m_stored_thermogram, &ThermogramHandler::ThermogramChanged, this, &ThermogramWidget::Update);
    connect(m_stored_thermogram, &ThermogramHandler::PeakRulesChanged, this, &ThermogramWidget::UpdateRules);
    connect(m_stored_thermogram, &ThermogramHandler::BaseLineChanged, this, &ThermogramWidget::UpdateBaseLine);
}

ThermogramWidget::~ThermogramWidget()
{
    QSettings settings;
    settings.beginGroup("thermogram");
    settings.setValue("splitterSizes", m_splitter->saveState());
    settings.setValue("iterations", m_iterations->value());
    settings.setValue("integration_range", m_integration_range->currentText());
}

void ThermogramWidget::LoadDefaultThermogram()
{
    QJsonObject thermo;
    thermo["CalibrationStart"] = m_CalibrationStart;
    thermo["CalibrationHeat"] = m_CalibrationHeat;
    thermo["PeakDuration"] = m_PeakDuration;
    thermo["PeakCount"] = m_PeakCount;
    thermo["ScalingFactor"] = m_ScalingFactor;
    m_stored_thermogram->setThermogramParameter(thermo);
}

void ThermogramWidget::LoadDefault()
{
    QJsonObject thermo;
    thermo["ScalingFactor"] = m_ScalingFactor;
    m_stored_thermogram->UpdateParameter(thermo);
    Update();
}

void ThermogramWidget::UpdateTable()
{
    const QVector<PeakPick::Peak>* peaks = m_stored_thermogram->Peaks();

    m_table->clear();
    m_table->setRowCount(peaks->size());
    m_table->setColumnCount(5);

    for (unsigned int j = 0; j < peaks->size(); ++j) {
        QTableWidgetItem* newItem;

        newItem = new QTableWidgetItem(QString::number(peaks->at(j).integ_num));
        newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_table->setItem(j, 0, newItem);

        newItem = new QTableWidgetItem(QString::number(m_stored_thermogram->Spectrum()->X(peaks->at(j).start)));
        newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_table->setItem(j, 1, newItem);

        newItem = new QTableWidgetItem(QString::number(m_stored_thermogram->Spectrum()->X(peaks->at(j).end)));
        newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        m_table->setItem(j, 2, newItem);

        newItem = new QTableWidgetItem(QString::number(m_stored_thermogram->Spectrum()->X(peaks->at(j).int_start)));
        newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        m_table->setItem(j, 3, newItem);

        newItem = new QTableWidgetItem(QString::number(m_stored_thermogram->Spectrum()->X(peaks->at(j).int_end)));
        newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        m_table->setItem(j, 4, newItem);
    }
    QStringList header = QStringList() << "Heat\n[raw]"
                                       << "Peak Start\n[s]"
                                       << "Peak End\n[s]"
                                       << QString("%1 Start\n[s]").arg(Unicode_Integral)
                                       << QString("%1 End\n[s]").arg(Unicode_Integral);
    m_table->setHorizontalHeaderLabels(header);
    m_table->resizeColumnsToContents();
}

void ThermogramWidget::InitialiseChart()
{
    double lineWidth = qApp->instance()->property("lineWidth").toDouble() / 10.0;
    m_thermogram->addSeries(m_thermogram_series);
    m_thermogram_series->setName("Thermogram");
    m_thermogram_series->setSize(lineWidth);
    m_thermogram->setXAxis("time [s]");
    m_thermogram->setYAxis("q [raw/s]");
    m_thermogram->addSeries(m_optional_series);
    m_optional_series->setSize(lineWidth);
}

void ThermogramWidget::addOptionalSeries(const QList<QPointF>& series, const QString& name)
{
    m_optional_series->clear();

    if (series.size() == 0) {
        m_optional_series->hide();
        return;
    }

    m_optional_series->clear();
    m_optional_series->append(series);
    m_optional_series->setName(name);
    m_optional_series->setColor(QColor(Qt::red).darker(200));

    m_optional_series->show();
}

void ThermogramWidget::clear()
{
    m_peak_list.clear();
    m_spectrum = false;
    m_table->clear();
    m_thermogram_series->clear();
    m_baseline_series->clear();
    m_baseline_ignored_series->clear();
    m_upper->clear();
    m_lower->clear();
    m_base_grids->clear();
}

void ThermogramWidget::Update()
{
    m_peak_list = QVector<PeakPick::Peak>(*m_stored_thermogram->Peaks());
    m_peaks_end->setValue(m_stored_thermogram->ThermogramEnd() - m_calibration_start->value());
    m_peaks_start->setValue(m_stored_thermogram->ThermogramBegin());
    qDebug() << m_stored_thermogram->ThermogramBegin();
    UpdateTable();
    UpdateSeries();
    UpdateRules();
}


void ThermogramWidget::UpdateBaseLine()
{
    Waiter wait;

    m_base_grids->clear();
    m_baseline_series->clear();
    m_baseline_ignored_series->clear();

    m_base_grids->append(m_stored_thermogram->BaselineGrid());
    m_thermogram->addSeries(m_base_grids);

    m_baseline_series->append(m_stored_thermogram->BaselineSeries());
    m_thermogram->addSeries(m_baseline_series);
    m_baseline_ignored_series->append(m_stored_thermogram->BaselineIgnored());
    m_thermogram->addSeries(m_baseline_ignored_series);

    m_base_grids->setMarkerSize(8);
}

void ThermogramWidget::UpdateSeries()
{
    Waiter wait;
    m_thermogram_series->clear();
    m_thermogram_series->append(m_stored_thermogram->ThermogramSeries());
}

void ThermogramWidget::ApplyCalibration()
{
    for (int i = 0; i < int(m_integrals_raw.size()); ++i) {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        if (m_calibration_heat->value() != 0)
            (m_peak_list)[i].integ_num = m_integrals_raw[i] * m_calibration_heat->value() / m_calibration_peak.integ_num;

        (m_peak_list)[i].integ_num -= m_const_offset->value();
    }
    emit CalibrationChanged(m_calibration_heat->value());
}


void ThermogramWidget::PeakRuleDoubleClicked(const QModelIndex& index)
{
    int peak = index.row();
    PeakRuleDoubleClicked(m_peak_rule_list->item(index.row(), 0), peak);
}

void ThermogramWidget::PeakRuleDoubleClicked(const QTableWidgetItem* item, int peak)
{
    m_current_peaks_rule = peak;
    m_peaks_start->setValue(item->data(Qt::DisplayRole).toDouble());
    m_peak_rule_list->sortByColumn(0, Qt::AscendingOrder);
}

void ThermogramWidget::PeakDoubleClicked(const QModelIndex& index)
{
    int peak = index.row();
    PeakDoubleClicked(peak);
}

void ThermogramWidget::PeakDoubleClicked(int peak)
{
    if (peak < 0 || peak >= m_peak_list.size())
        return;

    m_peak_edit_mode = true;

    int lower = peak, upper = peak;

    if (peak == 0)
        lower = peak;
    else if (peak == 1)
        lower = peak - 1;
    else
        lower = peak - 2;

    if (peak == m_peak_list.size() - 1)
        upper = peak;
    else if (peak == m_peak_list.size() - 2)
        upper = peak + 1;
    else
        upper = peak + 2;

    m_current_peak = peak;

    qreal ymax = m_thermogram->YMaxRange();
    qreal ymin = m_thermogram->YMinRange();

    qreal xmin, xmin_0;
    qreal xmax, xmax_0;

    xmin_0 = m_stored_thermogram->Spectrum()->X(m_peak_list[lower].start);
    xmax_0 = m_stored_thermogram->Spectrum()->X(m_peak_list[upper].end);

    xmin = m_stored_thermogram->Spectrum()->X(m_peak_list[peak].start);
    xmax = m_stored_thermogram->Spectrum()->X(m_peak_list[peak].end);

    m_peak_start_line->clear();
    m_peak_start_line->append(xmin, ymin);
    m_peak_start_line->append(xmin, ymax);

    m_peak_start_line->show();

    m_peak_end_line->clear();

    m_peak_end_line->append(xmax, ymin);
    m_peak_end_line->append(xmax, ymax);

    m_peak_end_line->show();

    m_thermogram->addSeries(m_peak_end_line);
    m_thermogram->addSeries(m_peak_start_line);

    m_thermogram->setXRange(xmin_0, xmax_0);
    m_thermogram->setYRange(ymin, ymax);
    m_thermogram->setSelectStrategy(SelectStrategy::S_Horizontal);
    m_thermogram->setSelectBox(QPointF(xmin, ymax), QPointF(xmax, ymin));
    setGuideText(QString("You are now in <i>Peak Integration mode</i>. Click [ESC] to leave to mode. Sometimes you might have to activate the chart widget by <b>clicking</b> with the <b>left mouse button</b> right before [ESC]. The peak integration range can be reduced by clicking the <b>right mouse button</b> within the black separation lines - or reset to the whole peak <b>clicking</b> with the <b>right mouse button</b> out of the area between the separation borders. Use [LEFT] or [RIGHT] arrow on your keyboard to navigate through the peaks or <b>double-click</b> on a different peak in the table. Zooming with the <b>mouse wheel</b> is possible."));
}

void ThermogramWidget::PeakChanged(int row, int column, int value)
{
    if (column == 1)
        m_peak_list[row].start = value;
    else if (column == 2)
        m_peak_list[row].end = value;
    m_stored_thermogram->setPeakList(m_peak_list);

    Update();
    PeakDoubleClicked(row);
}

void ThermogramWidget::CreateSeries()
{
    m_thermogram_series = new LineSeries;
    m_thermogram_series->setUseOpenGL(false);
    m_baseline_series = new ScatterSeries;
    m_baseline_series->setMarkerSize(3.5);
    m_baseline_series->setColor(QColor(Qt::green).lighter());
    m_baseline_series->setBorderColor(QColor(Qt::green).lighter());
    m_baseline_series->setName("Base line");

    m_baseline_ignored_series = new ScatterSeries;
    m_baseline_ignored_series->setMarkerSize(2.5);
    m_baseline_ignored_series->setBorderColor(QColor(Qt::red).lighter(200));

    m_upper = new LineSeries;
    m_lower = new LineSeries;
    m_peak_start_line = new LineSeries;
    QPen pen;
    pen.setWidth(2);
    pen.setColor(Qt::black);
    m_peak_start_line->setPen(pen);
    m_peak_start_line->show();
    m_peak_end_line = new LineSeries;
    m_peak_end_line->setPen(pen);
    m_peak_end_line->show();

    m_base_grids = new ScatterSeries;
    m_optional_series = new LineSeries;
}

void ThermogramWidget::ApplyParameter()
{
    m_stored_thermogram->setCurrentCutOption(m_integration_range->currentText());
    m_stored_thermogram->setInitialThreshold(m_integration_range_threshold->value());
    m_stored_thermogram->setMaximalIterations(m_iterations->value());
    m_stored_thermogram->setOvershotCounter(m_overshot->value());
    m_stored_thermogram->setCutBefore(m_direction->isChecked());
}

void ThermogramWidget::UpdatePeaks()
{
    Waiter wait;
    QVector<QPointF> peak_rules;

    for (int i = 0; i < m_peak_rule_list->rowCount(); ++i) {
        PeakRule* item = dynamic_cast<PeakRule*>((m_peak_rule_list->item(i, 0)));
        double start = item->data(Qt::DisplayRole).toDouble();
        item = dynamic_cast<PeakRule*>(m_peak_rule_list->item(i, 1));
        double time = item->data(Qt::DisplayRole).toDouble();
        peak_rules << QPointF(start, time);
    }
    m_stored_thermogram->setCalibrationHeat(m_calibration_heat->value());
    m_stored_thermogram->setCalibrationStart(m_calibration_start->value());
    ApplyParameter();

    m_stored_thermogram->setThermogramEnd(m_peaks_end->value());
    m_stored_thermogram->setPeakRules(peak_rules);
    m_stored_thermogram->UpdatePeaks();

    m_stored_thermogram->IntegrateThermogram();
}

void ThermogramWidget::AddRectanglePeak(const QPointF& point1, const QPointF& point2)
{
    if (!m_peak_edit_mode)
        return;

    m_thermogram->setSelectStrategy(SelectStrategy::S_None);
    m_thermogram->setZoomStrategy(ZoomStrategy::Z_Rectangular);

    m_peak_start_line->hide();
    m_peak_end_line->hide();

    m_peak_list[m_current_peak].int_start = m_stored_thermogram->Spectrum()->XtoIndex(point1.x());
    m_peak_list[m_current_peak].int_end = m_stored_thermogram->Spectrum()->XtoIndex(point2.x());
    m_stored_thermogram->setPeakList(m_peak_list);
    m_stored_thermogram->FitBaseLine();
    m_stored_thermogram->IntegrateThermogram();
    m_peak_edit_mode = false;
    ResetGuideLabel();
}

void ThermogramWidget::PointDoubleClicked(const QPointF& point)
{
    QApplication::restoreOverrideCursor();

    qreal x = point.x();

    if (m_get_time_from_thermogram == 1) {
        if (m_rules_imported) {
            qreal diff = m_peak_rule_list->item(m_current_peaks_rule, 1)->data(Qt::DisplayRole).toInt() - (x - m_peaks_start->value());
            if (diff > m_peaks_time->value() || diff < 0) {
                m_guide_label->setText(tr("<font color='red'>I will not apply this value, since you are leaving the peak boundaries. If this was intentend, you can manually edit the 'Start Time': %1 and the 'Peak Duration' %2 in the table!.</font>").arg(x).arg(diff));
                m_get_peaks_end->setText("Click to Select");
                m_get_time_from_thermogram = 0;
                return;
            }
            m_peak_rule_list->item(m_current_peaks_rule, 1)->setData(Qt::DisplayRole, diff);
        }
        m_peaks_start->setValue(x);
        m_peak_rule_list->item(m_current_peaks_rule, 0)->setData(Qt::DisplayRole, x);
        m_get_peaks_start->setText("Click to Select");
    } else if (m_get_time_from_thermogram == 2) {
        m_get_peaks_end->setText("Click to Select");
        m_peaks_end->setValue(x);
    } else if (m_calibration_start_int == 1) {
        m_calibration_start->setValue(m_stored_thermogram->Spectrum()->XMax() - x);
        m_CalibrationStart = x;
        qApp->instance()->setProperty("calibration_start", x);
        m_peaks_end->setMaximum(x);
        m_peaks_end->setValue(x);
        m_calibration_start_int = 0;
    } else {
        for (int i = 0; i < m_table->rowCount(); ++i) {
            double start = m_table->item(i, 1)->data(Qt::DisplayRole).toDouble();
            double end = m_table->item(i, 2)->data(Qt::DisplayRole).toDouble();

            if (start < x && x < end) {
                m_table->setCurrentCell(i, 0, QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
                break;
            }
        }
        if (m_rules_imported) {
            for (int i = 0; i < m_peak_rule_list->rowCount(); ++i) {
                double start = m_peak_rule_list->item(i, 0)->data(Qt::DisplayRole).toDouble();
                double end = start + m_peak_rule_list->item(i, 1)->data(Qt::DisplayRole).toDouble();

                if (start < x && x < end) {
                    m_peak_rule_list->setCurrentCell(i, 0, QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
                    PeakRuleDoubleClicked(m_peak_rule_list->item(i, 0), i);
                    break;
                }
            }
        }
    }
    ResetGuideLabel();
    m_get_time_from_thermogram = 0;
}

void ThermogramWidget::scaleUp()
{
    qreal position = m_thermogram->Chart()->mapToValue(m_thermogram->currentMousePosition()).y();
    qreal max = m_thermogram->YMaxRange();
    qreal min = m_thermogram->YMinRange();
    qreal next = (max - min) * 0.1;

    qreal ratio = (position - min) / (max - min);

    if (position > max)
        m_thermogram->setYRange(min, max - next);
    else if (position < min)
        m_thermogram->setYRange(min - next, max);
    else
        m_thermogram->setYRange(min + (ratio)*next, max - (1 - ratio) * next);
}

void ThermogramWidget::scaleDown()
{
    qreal position = m_thermogram->Chart()->mapToValue(m_thermogram->currentMousePosition()).y();
    qreal max = m_thermogram->YMaxRange();
    qreal min = m_thermogram->YMinRange();
    qreal next = (max - min) * 0.1;

    qreal ratio = (position - min) / (max - min);

    if (position > max)
        m_thermogram->setYRange(min, max + next);
    else if (position < min)
        m_thermogram->setYRange(min + next, max);
    else
        m_thermogram->setYRange(min - (ratio)*next, max + (1 - ratio) * next);
}

void ThermogramWidget::setGuideText(const QString& str)
{
    if (qApp->instance()->property("thermogram_guidelines").toBool())
        m_guide_label->setText(str);
    else
        m_guide_label->clear();
}

void ThermogramWidget::ResetGuideLabel()
{
    if (qApp->instance()->property("thermogram_guidelines").toBool())
        m_guide_label->setText("You are in standard mode. In addition to the SupraFit chart interaction, the <b>mouse wheel</b> can be used to zoom in and out, where the ratio of zooming depends on the actual positon of the curser in the chart. See the SupraFit Handbook for details. And <b>double click</b> in the chart selects the peak in the table.\nRegarding the units in this tabulator: They are all in the units observed by your calorimeter. If they are originally for example in cal, please mind the scaling factor to convert them into J. If they are already in J, since you are using a calibration peak, set the scaling factor to 1 (this may have been done automatically, but always check).");
    else
        m_guide_label->clear();
}

void ThermogramWidget::UpdateRules()
{
    QVector<QPointF> peak_rules = m_stored_thermogram->PeakRules();

    if (!peak_rules.size())
        return;

    m_peak_rule_list->clear();
    m_peak_rule_list->setRowCount(peak_rules.size());

    QStringList header = QStringList() << "Start Time\n[s]"
                                       << "Peak Duration\n[s]";
    m_peak_rule_list->setHorizontalHeaderLabels(header);

    for (int i = 0; i < peak_rules.size(); ++i) {
        PeakRule* item = new PeakRule(QString::number(peak_rules[i].x()));
        m_peak_rule_list->setItem(i, 0, item);
        item = new PeakRule(QString::number(peak_rules[i].y()));
        m_peak_rule_list->setItem(i, 1, item);
    }
    m_peak_rule_list->sortByColumn(0, Qt::AscendingOrder);
    m_rules_imported = true;
}

void ThermogramWidget::LoadRules()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir());
    if (filename.isEmpty())
        return;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return;

    m_peak_rule_list->clear();
    m_peak_rule_list->setRowCount(1);
    QStringList header = QStringList() << "Start Time\n[s]"
                                       << "Peak Duration\n[s]";
    m_peak_rule_list->setHorizontalHeaderLabels(header);

    QStringList blob = QString(file.readAll()).split("\n");
    QVector<QPointF> peak_rules;
    int rows = 0;
    for (const QString& str : qAsConst(blob)) {
        if (str.isEmpty() || str.isNull())
            continue;

        QStringList line = str.simplified().split(" ");
        if (line.size() == 2 && !str.contains("#")) {
            m_peak_rule_list->setRowCount(rows + 1);
            PeakRule* item = new PeakRule(line[0]);
            m_peak_rule_list->setItem(rows, 0, item);

            item = new PeakRule(line[1]);
            m_peak_rule_list->setItem(rows, 1, item);
            rows++;
            peak_rules << QPointF(line[0].toDouble(), line[1].toDouble());
        }
    }
    m_peak_rule_list->sortByColumn(0, Qt::AscendingOrder);
    m_stored_thermogram->setPeakRules(peak_rules);
    m_rules_imported = false;
}

void ThermogramWidget::WriteRules()
{
    QString filename = QFileDialog::getSaveFileName(this, "Select file", getDir());
    if (filename.isEmpty())
        return;

    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
        return;

    QTextStream stream(&file);
    stream << QString("#Start Time") + "\t" + " Peak Duration " + "\n";
    stream << QString("#[s]") + "\t" + " [s] " + "\n";

    for (int i = 0; i < m_peak_rule_list->rowCount(); ++i) {
        PeakRule* item = dynamic_cast<PeakRule*>((m_peak_rule_list->item(i, 0)));
        stream << item->data(Qt::DisplayRole).toString() << "\t";
        item = dynamic_cast<PeakRule*>(m_peak_rule_list->item(i, 1));
        stream << item->data(Qt::DisplayRole).toString();
        if (i < m_peak_rule_list->rowCount() - 1)
            stream << "\n";
    }
}
