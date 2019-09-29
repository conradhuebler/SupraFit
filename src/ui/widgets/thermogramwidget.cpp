/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QAction>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
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
#include "src/core/toolset.h"

#include "src/global.h"

#include "thermogramwidget.h"

static int baseline_step_size = 50;

ThermogramWidget::ThermogramWidget(QWidget* parent)
    : QWidget(parent)
{
    setUi();
    CreateSeries();
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
    connect(m_convert_rules, &QPushButton::clicked, this, &ThermogramWidget::ConvertRules);

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

    QHBoxLayout* hlayout = new QHBoxLayout;

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->tr("thermogram_splitter");
    m_splitter->addWidget(m_thermogram);
    m_splitter->addWidget(peaks_tab);
    hlayout->addWidget(m_splitter);

    QWidget* chart = new QWidget;

    chart->setLayout(hlayout);
    m_const_offset = new QDoubleSpinBox;
    m_const_offset->setMinimum(-1e5);
    m_const_offset->setMaximum(1e5);
    m_const_offset->setValue(0);
    m_const_offset->setDecimals(7);

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
            setGuideText(QString("You are now in <i>Thermogram start selection mode</i>. <b>Double click</b> with the <b>left</b> mouse in the chart to define, where the peak starts. Click on the <em>Click to Undo Button</em> to decline the selection. Zooming is still possible via <b>single left click</b>."));
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
            setGuideText(QString("You are now in <i>Thermogram end selection mode</i>. <b>Double click</b> with the <b>left</b> mouse in the chart to define, where the peak starts. Click on the <em>Click to Undo Button</em> to decline the selection. Zooming is still possible via <b>single left click</b>."));
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
    m_calibration_start->setValue(qApp->instance()->property("calibration_start").toDouble());
    connect(m_calibration_start, qOverload<double>(&QDoubleSpinBox::valueChanged), m_calibration_start, [this](double value) {
        qApp->instance()->setProperty("calibration_start", value);
        m_peaks_end->setMaximum(m_spec.XMax() - value * m_spec.Step());
        m_peaks_end->setValue(m_spec.XMax() - value * m_spec.Step());
        CalibrateSystem();
        ApplyCalibration();
    });
    m_calibration_start->setMaximumWidth(100);
    m_calibration_start->setToolTip(tr("Set the duration in seconds of the calibration peak. The calibration peak is considered to be last peak in the thermogram. Leave zero, if the calorimeter does not use calibration peaks"));

    m_calibration_heat = new QDoubleSpinBox;
    m_calibration_heat->setMinimum(-1e8);
    m_calibration_heat->setMaximum(1e8);
    m_calibration_heat->setDecimals(5);
    m_calibration_heat->setSingleStep(1e-2);
    m_calibration_heat->setValue(qApp->instance()->property("calibration_heat").toDouble());
    connect(m_calibration_heat, qOverload<double>(&QDoubleSpinBox::valueChanged), m_calibration_heat, [this](double value) {
        qApp->instance()->setProperty("calibration_heat", value);
        CalibrateSystem();
        ApplyCalibration();
    });
    m_calibration_heat->setMaximumWidth(100);
    m_calibration_heat->setToolTip(tr("Set the heat in J, that were used to for the calibration. Leave zero if no calibration peak was used. If the value is not zero, the scaling factor may be redundant. Please keep that in mind."));

    m_calibration_label = new QLabel(tr("<h4>Calibration (0)</h4>"));

    m_integration_range = new QComboBox;
    m_integration_range->addItems(m_Peak_Cut_Options);
    m_integration_range->setMaximumWidth(100);

    m_peak_apply = new QPushButton(tr("Apply and Update"));
    m_peak_apply->setIcon(Icon("dialog-ok-apply"));
    connect(m_peak_apply, &QPushButton::clicked, this, [this]() {
        UpdatePeaks();
        CutAllLimits();
    });
    m_peak_apply->setMaximumWidth(150);
    m_peak_apply->setToolTip(tr("Click generate peaks, fit baseline and integrate the peaks"));

    m_integration_range_threshold = new QDoubleSpinBox;
    m_integration_range_threshold->setMinimum(0);
    m_integration_range_threshold->setMaximum(1e5);
    m_integration_range_threshold->setDecimals(10);
    m_integration_range_threshold->setValue(0);
    m_integration_range_threshold->setMaximumWidth(150);
    m_integration_range_threshold->setToolTip(tr("Define a threshold for Peak Range Reduction"));

    m_iterations = new QSpinBox;
    m_iterations->setMinimum(1);
    m_iterations->setMaximum(1e3);
    m_iterations->setValue(15);
    m_iterations->setToolTip(tr("Define the maximum number of iteration for the Peak Range Reduction Cycle. In some case, this value should be 1."));

    m_direction = new QCheckBox(tr("Before"));

    m_overshot = new QSpinBox;
    m_overshot->setRange(1, 200);
    m_overshot->setValue(1);

    m_gradient = new QDoubleSpinBox;
    m_gradient->setDecimals(5);
    m_gradient->setRange(-1, 1);
    m_gradient->setValue(-1);

    QHBoxLayout* peak_layout = new QHBoxLayout;
    QVBoxLayout* vlayout = new QVBoxLayout;

    vlayout->addWidget(new QLabel(tr("Remove constant")));
    vlayout->addWidget(m_const_offset);
    peak_layout->addLayout(vlayout);

    QHBoxLayout* triplett = new QHBoxLayout;

    vlayout = new QVBoxLayout;
    triplett->addWidget(new QLabel(tr("Thermogram\nstart time [s]")));
    vlayout->addWidget(m_get_peaks_start);
    vlayout->addWidget(m_peaks_start);
    triplett->addLayout(vlayout);
    peak_layout->addLayout(triplett);

    triplett = new QHBoxLayout;
    triplett->addWidget(new QLabel(tr("Thermogram\nend time [s]")));
    vlayout = new QVBoxLayout;
    vlayout->addWidget(m_get_peaks_end);
    vlayout->addWidget(m_peaks_end);
    triplett->addLayout(vlayout);
    peak_layout->addLayout(triplett);

    triplett = new QHBoxLayout;
    triplett->addWidget(new QLabel(tr("Time between\ninjections [s]")));
    triplett->addWidget(m_peaks_time);
    peak_layout->addLayout(triplett);

    vlayout = new QVBoxLayout;
    vlayout->addWidget(m_calibration_label);

    triplett = new QHBoxLayout;
    triplett->addWidget(new QLabel("Time start [s]"));
    triplett->addWidget(m_calibration_start);
    vlayout->addLayout(triplett);

    triplett = new QHBoxLayout;
    triplett->addWidget(new QLabel("Heat [raw]"));
    triplett->addWidget(m_calibration_heat);
    vlayout->addLayout(triplett);
    peak_layout->addLayout(vlayout);

    vlayout = new QVBoxLayout;
    QHBoxLayout* iterlayout = new QHBoxLayout;

    iterlayout->addWidget(m_integration_range_threshold);
    iterlayout->addWidget(new QLabel(tr("Iterations")));
    iterlayout->addWidget(m_iterations);
    if (qApp->instance()->property("advanced_ui").toBool()) {
        iterlayout->addWidget(m_direction);
        iterlayout->addWidget(m_overshot);
        iterlayout->addWidget(m_gradient);
    }
    vlayout->addLayout(iterlayout);

    iterlayout = new QHBoxLayout;
    iterlayout->addWidget(new QLabel(tr("Method")));
    iterlayout->addWidget(m_integration_range);
    vlayout->addLayout(iterlayout);

    vlayout->addWidget(m_peak_apply);
    peak_layout->addLayout(vlayout);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addLayout(peak_layout);
    layout->addWidget(m_guide_label);
    layout->addWidget(chart);

    setLayout(layout);

    QSettings settings;
    settings.beginGroup("thermogram");
    m_peaks_start->setValue(settings.value("peaks_start", 60).toDouble());
    m_peaks_time->setValue(settings.value("peaks_time", 150).toDouble());
    m_splitter->restoreState(settings.value("splitterSizes").toByteArray());
    m_iterations->setValue(settings.value("iterations", 15).toInt());
    // m_integration_range->setCurrentText(settings.value("integration_range", m_Peak_Cut_Options[0]).toString());
    QTableWidgetItem* item = new QTableWidgetItem(QString::number(m_peaks_start->value()));
    m_peak_rule_list->setItem(m_current_peaks_rule, 0, item);
    item = new QTableWidgetItem(QString::number(m_peaks_time->value()));
    m_peak_rule_list->setItem(m_current_peaks_rule, 1, item);

    connect(m_integration_range, &QComboBox::currentTextChanged, this, &ThermogramWidget::CutAllLimits);

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
}

ThermogramWidget::~ThermogramWidget()
{
    QSettings settings;
    settings.beginGroup("thermogram");
    settings.setValue("splitterSizes", m_splitter->saveState());
    settings.setValue("iterations", m_iterations->value());
    settings.setValue("integration_range", m_integration_range->currentText());
}

void ThermogramWidget::setThermogram(PeakPick::spectrum* spec, qreal offset)
{
    m_spec = *spec;
    m_frequency = spec->Step();
    if (offset == 0.0)
        m_offset = m_spec.Mean();
    else
        m_offset = offset;

    if (m_filetype == FileType::ITC) {
        const QSignalBlocker blocker_a(m_calibration_start);
        const QSignalBlocker blocker_b(m_calibration_heat);

        m_calibration_start->setValue(0);
        m_calibration_heat->setValue(0);
    }

    m_baseline.baselines.push_back(Vector(1));
    m_baseline.baselines[0](0) = m_offset;

    PeakPick::spectrum sign = PeakPick::spectrum(m_spec);
    sign.InvertSgn();

    std::vector<PeakPick::Peak> peaks = PeakPick::PickPeaks(&sign, 0, qPow(2, 1));
    std::vector<PeakPick::Peak> max_peak = PeakPick::PickPeaks(&m_spec, 0, qPow(2, 1));

    if (m_spec.size() == 0)
        return;

    m_spectrum = true;

    m_peaks_start->setMaximum(m_spec.XMax());
    m_peaks_end->setMaximum(m_spec.XMax() - m_calibration_start->value() * m_spec.Step());

    m_peaks_start->setValue(m_spec.XMin());
    m_peaks_end->setValue(m_spec.XMax() - m_calibration_start->value() * m_spec.Step());

    UpdatePlot();
}

void ThermogramWidget::setPeakList(const std::vector<PeakPick::Peak>& peak_list)
{
    const QSignalBlocker blocker_a(m_peaks_time);
    const QSignalBlocker blocker_b(m_peaks_start);

    double time_pred = 0;
    m_peak_list = peak_list;
    m_peak_rule_list->clear();
    int row = 0;
    for (int i = 0; i < m_peak_list.size(); ++i) {
        double time = m_peak_list[i].end * m_spec.Step() - m_peak_list[i].start * m_spec.Step() + m_spec.Step();
        double start = m_peak_list[i].start * m_spec.Step() - m_spec.Step();
        if (!qFuzzyCompare(time_pred, time)) {
            m_peaks_start->setValue(start);
            m_peaks_time->setValue(time);
            m_peaks_end->setValue(m_peak_list[m_peak_list.size() - 1].end * m_spec.Step());
            m_peak_rule_list->setRowCount(row + 1);
            PeakRule* item = new PeakRule(QString::number(start));
            m_peak_rule_list->setItem(row, 0, item);
            item = new PeakRule(QString::number(time));
            m_peak_rule_list->setItem(row, 1, item);
            row++;
        }
        time_pred = time;
    }
    UpdatePeaks();
    CutAllLimits();
}

void ThermogramWidget::UpdateTable()
{
    m_table->clear();
    m_table->setRowCount(m_peak_list.size());
    m_table->setColumnCount(3);
    for (unsigned int j = 0; j < m_peak_list.size(); ++j) {
        QTableWidgetItem* newItem;

        newItem = new QTableWidgetItem(QString::number(m_peak_list[j].integ_num));
        newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_table->setItem(j, 0, newItem);

        newItem = new QTableWidgetItem(QString::number(m_spec.X(m_peak_list[j].start)));
        newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_table->setItem(j, 1, newItem);

        newItem = new QTableWidgetItem(QString::number(m_spec.X(m_peak_list[j].end)));
        newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        m_table->setItem(j, 2, newItem);
    }
    QStringList header = QStringList() << "Heat\n[raw]"
                                       << "Peak Start\n[s]"
                                       << "Peak End\n[s]";
    m_table->setHorizontalHeaderLabels(header);
    m_table->resizeColumnsToContents();
}

void ThermogramWidget::UpdatePlot()
{
    fromSpectrum(&m_spec, m_thermogram_series);
    double lineWidth = qApp->instance()->property("lineWidth").toDouble() / 10.0;
    m_thermogram->addSeries(m_thermogram_series);
    m_thermogram_series->setSize(lineWidth);
    m_thermogram->setXAxis("time [s]");
    m_thermogram->setYAxis("q [raw/s]");
}

void ThermogramWidget::fromSpectrum(const PeakPick::spectrum* original, LineSeries* series)
{
    PeakPick::spectrum* spectrum = new PeakPick::spectrum(original);

    series->clear();
    for (unsigned int i = 0; i < spectrum->x().size(); i++) {
        series->append(QPointF(spectrum->x()[i], spectrum->y()[i]));
    }
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
    Integrate(&m_peak_list, &m_spec);
    UpdateTable();
    UpdateSeries();
}

void ThermogramWidget::Integrate(std::vector<PeakPick::Peak>* peaks, const PeakPick::spectrum* original)
{
    CalibrateSystem();

    Waiter wait;

    m_integrals_raw.clear();

    Vector baseline;
    QVector<qreal> difference_signal_baseline, tmp;
    qreal sum_difference_signal_baseline = 0;
    if (m_baseline.baselines.size() > 0)
        baseline = m_baseline.baselines[0];

    for (int i = 0; i < int(peaks->size()); ++i) {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        if (peaks->size() == m_baseline.baselines.size()) // && m_baseline.x_grid_points.size() > 0)
        {
            baseline = m_baseline.baselines[i];
        }
        PeakPick::IntegrateNumerical(original, (*peaks)[i], baseline);
        m_integrals_raw << (*peaks)[i].integ_num;

        for (int j = (*peaks)[i].int_start; j < int((*peaks)[i].int_end) - 1; j++) {
            sum_difference_signal_baseline += qAbs(PeakPick::Polynomial(m_spec.X(j), baseline) - (m_spec.Y(j)));
            difference_signal_baseline << qAbs(PeakPick::Polynomial(m_spec.X(j), baseline) - (m_spec.Y(j)));
        }
    }
    qreal stdev = Stddev(difference_signal_baseline, 0, sum_difference_signal_baseline / double(difference_signal_baseline.size()));

    int counter = 0;
    double sum = 0;

    for (int i = 0; i < difference_signal_baseline.size(); ++i) {
        tmp << difference_signal_baseline[i] * (difference_signal_baseline[i] < stdev);
        counter += (difference_signal_baseline[i] < stdev);
        sum += difference_signal_baseline[i] * (difference_signal_baseline[i] < stdev);
    }

    qreal stdev2 = Stddev(tmp, 0, sum / double(counter));

    if (m_initial_threshold < 1e-12)
        m_initial_threshold = stdev2 / 10.0;

    m_integration_range_threshold->setValue(stdev2 / 10.0);

    ApplyCalibration();
}

void ThermogramWidget::UpdateSeries()
{
    CalibrateSystem();

    Waiter wait;
    m_base_grids->clear();
    m_base_grids->hide();
    m_baseline_series->clear();
    m_baseline_series->hide();
    m_baseline_ignored_series->clear();
    m_baseline_ignored_series->hide();
    Vector baseline;

    int steps = 1;
    if (m_baseline.baselines.size() > 0)
        baseline = m_baseline.baselines[0];

    for (int i = 0; i < int(m_peak_list.size()); ++i) {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        if (m_peak_list.size() == m_baseline.baselines.size() && m_baseline.x_grid_points.size() > 0) {
            baseline = m_baseline.baselines[i];
            for (int j = 0; j < int(m_baseline.x_grid_points[i].size()); ++j) {
                m_base_grids->append(m_baseline.x_grid_points[i][j], m_baseline.y_grid_points[i][j]);
            }
        }

        steps = (m_peak_list[i].end - m_peak_list[i].start) / baseline_step_size + 1;

        for (int j = m_peak_list[i].int_start; j < int(m_peak_list[i].int_end) - 1; j += steps)
            m_baseline_series->append(QPointF(m_spec.X(j), PeakPick::Polynomial(m_spec.X(j), baseline)));

        for (int j = m_peak_list[i].int_end; j < int(m_peak_list[i].end) - 1; j += steps)
            m_baseline_ignored_series->append(QPointF(m_spec.X(j), PeakPick::Polynomial(m_spec.X(j), baseline)));

        m_baseline_series->append(QPointF(m_spec.X(int(m_peak_list[i].int_end) - 1), PeakPick::Polynomial(m_spec.X(int(m_peak_list[i].int_end) - 1), baseline)));
        m_baseline_ignored_series->append(QPointF(m_spec.X(int(m_peak_list[i].end) - 1), PeakPick::Polynomial(m_spec.X(int(m_peak_list[i].end) - 1), baseline)));
    }

    if (m_baseline.x_grid_points.size() == 1) {
        for (int j = 0; j < int(m_baseline.x_grid_points[0].size()); j += steps) {
            m_base_grids->append(m_baseline.x_grid_points[0][j], m_baseline.y_grid_points[0][j]);
        }
    }
    m_base_grids->show();
    m_baseline_series->show();
    m_baseline_ignored_series->show();
    m_base_grids->setMarkerSize(8);
    m_thermogram->addSeries(m_base_grids);
    m_thermogram->addSeries(m_baseline_series);
    m_thermogram->addSeries(m_baseline_ignored_series);
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

void ThermogramWidget::UpdateFit(const QString& str)
{
    FitBaseLine();
    m_fit = str;
}

void ThermogramWidget::FitBaseLine()
{
    PeakPick::BaseLine baseline(&m_spec);
    baseline.setBaseLineRange(PeakPick::BaseLine::BLR::PeakWise);
    baseline.setPolynomFit(PeakPick::BaseLine::Polynom::Slow);
    baseline.setNoCoeffs(2);
    baseline.setPeaks(&m_peak_list);
    m_baseline = baseline.Fit();

    if (m_baseline.baselines.size() == 1) {
        m_initial_baseline = m_baseline.baselines[0];
        m_coeffs->setValue(m_initial_baseline.size());
    }
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

    xmin_0 = m_spec.X(m_peak_list[lower].start);
    xmax_0 = m_spec.X(m_peak_list[upper].end);

    xmin = m_spec.X(m_peak_list[peak].start);
    xmax = m_spec.X(m_peak_list[peak].end);

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

    Update();
    PeakDoubleClicked(row);
}

void ThermogramWidget::setFit(const QJsonObject& fit)
{
    m_block = true;
    QSignalBlocker time(m_peaks_time);
    QSignalBlocker start(m_peaks_start);

    if (fit.contains("thermogram") && m_spectrum == false) {
        Vector x, y;
        x = ToolSet::String2DoubleEigVec(fit["thermogram"].toObject()["x"].toString());
        y = ToolSet::String2DoubleEigVec(fit["thermogram"].toObject()["y"].toString());
        m_spec.setSpectrum(x, y);

        m_spectrum = true;
        m_frequency = m_spec.Step();

        const QSignalBlocker blocker_a(m_calibration_start);
        const QSignalBlocker blocker_b(m_calibration_heat);

        m_calibration_start->setValue(fit["calibration_start"].toDouble());
        m_calibration_heat->setValue(fit["calibration_heat"].toDouble());

        m_peaks_start->setMaximum(m_spec.XMax());
        m_peaks_end->setMaximum(m_spec.XMax() - m_calibration_start->value() * m_spec.Step());

        m_peaks_start->setValue(m_spec.XMin());
        m_peaks_end->setValue(m_spec.XMax() - m_calibration_start->value() * m_spec.Step());

        m_guide_label->setText(QString("<font color='red'>The raw data files are not in place. I will use the stored thermogram.</font>"));

        UpdatePlot();
    }

    m_const_offset->setValue(fit["constants"].toDouble());
    m_integration_range_threshold->setValue(fit["integration_range_threshold"].toDouble());
    m_integration_range->setCurrentText(fit["integration_range"].toString());
    m_initial_threshold = fit["integration_range"].toDouble();

    m_peaks_start->setValue(fit["start_time"].toDouble());
    m_peaks_time->setValue(fit["peak_time"].toDouble());
    m_peaks_end->setValue(fit["end_time"].toDouble());
    m_iterations->setValue(fit["iter"].toInt());

    QList<QPointF> points = ToolSet::String2Points(fit["rules_list"].toString());
    m_peak_rule_list->setRowCount(points.size());
    for (int i = 0; i < m_peak_rule_list->rowCount(); ++i) {
        QTableWidgetItem* item = new QTableWidgetItem(QString::number(points[i].x()));
        m_peak_rule_list->setItem(i, 0, item);

        item = new QTableWidgetItem(QString::number(points[i].y()));
        m_peak_rule_list->setItem(i, 1, item);
    }
    UpdatePeaks();
    points = ToolSet::String2Points(fit["peak_int_ranges"].toString());
    for (int i = 0; i < m_peak_list.size(); ++i) {
        m_peak_list[i].int_start = points[i].x();
        m_peak_list[i].int_end = points[i].y();
    }
    FitBaseLine();
    Update();
    m_block = false;
}

QJsonObject ThermogramWidget::Fit() const
{
    QJsonObject fit;

    fit["constants"] = m_const_offset->value();
    fit["frequency"] = m_frequency;
    fit["start_time"] = m_peaks_start->value();
    fit["end_time"] = m_peaks_end->value();
    fit["peak_time"] = m_peaks_time->value();
    fit["calibration_start"] = m_calibration_start->value();
    fit["calibration_heat"] = m_calibration_heat->value();

    QList<QPointF> points;
    for (int i = 0; i < m_peak_rule_list->rowCount(); ++i)
        points << QPointF(m_peak_rule_list->item(i, 0)->data(Qt::DisplayRole).toDouble(), m_peak_rule_list->item(i, 1)->data(Qt::DisplayRole).toDouble());
    fit["rules_list"] = ToolSet::Points2String(points);
    points.clear();

    for (int i = 0; i < m_peak_list.size(); ++i)
        points << QPointF(m_peak_list[i].int_start, m_peak_list[i].int_end);
    fit["peak_int_ranges"] = ToolSet::Points2String(points);
    fit["integration_range"] = m_integration_range->currentText();
    fit["integration_range_threshold"] = m_integration_range_threshold->value();
    fit["iter"] = m_last_iteration_max;

    QJsonObject thermo;
    thermo["x"] = ToolSet::DoubleList2String(m_spec.x());
    thermo["y"] = ToolSet::DoubleList2String(m_spec.y());
    fit["thermogram"] = thermo;

    return fit;
}

void ThermogramWidget::CreateSeries()
{
    m_thermogram_series = new LineSeries;
    m_baseline_series = new ScatterSeries;
    m_baseline_series->setMarkerSize(3.5);
    m_baseline_series->setBorderColor(QColor(Qt::green).lighter());

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
}

void ThermogramWidget::UpdatePeaks()
{
    if (m_spec.Step() == 0)
        return;

    qreal offset = 0;
    int off = 1;
    int rules_size = m_peak_rule_list->rowCount();
    double end = m_peaks_end->value();
    m_peak_list.clear();
    PeakPick::Peak peak;


    for (int j = 0; j < rules_size; ++j) {
        QTableWidgetItem* item = (m_peak_rule_list->item(j, 0));
        double index_start = m_spec.XtoIndex(item->data(Qt::DisplayRole).toDouble());
        item = m_peak_rule_list->item(j, 1);
        double timestep = item->data(Qt::DisplayRole).toDouble() / m_spec.Step();
        if (timestep <= 0) {
            m_guide_label->setText(QString("<font color='red'>Sorry, but Peak rule %1 contains a zero as peak time. That means, if I did not stop right here, you would be waiting for Godot</font>").arg(j + 1));
            return;
        }
        double index_end;

        if (j == rules_size - 1)
            index_end = end;
        else
            index_end = m_peak_rule_list->item(j + 1, 0)->data(Qt::DisplayRole).toDouble();

        for (int i = index_start; i + (timestep)-1 < m_spec.XtoIndex(index_end); i += (timestep)) {
            peak = PeakPick::Peak();
            peak.setPeakStart(i);
            peak.setPeakEnd(i + (timestep)-1);
            peak.max = PeakPick::FindMaximum(&m_spec, peak);
            peak.min = PeakPick::FindMinimum(&m_spec, peak);

            m_peak_list.push_back(peak);
        }
        }

        m_offset = offset / double(off);
        m_baseline.baselines.push_back(Vector(1));
        m_baseline.baselines[0](0) = m_offset;

        ResetGuideLabel();
}

void ThermogramWidget::AddRectanglePeak(const QPointF& point1, const QPointF& point2)
{
    if (!m_peak_edit_mode)
        return;

    m_thermogram->setSelectStrategy(SelectStrategy::S_None);
    m_thermogram->setZoomStrategy(ZoomStrategy::Z_Rectangular);

    m_peak_start_line->hide();
    m_peak_end_line->hide();

    m_peak_list[m_current_peak].int_start = m_spec.XtoIndex(point1.x());
    m_peak_list[m_current_peak].int_end = m_spec.XtoIndex(point2.x());
    m_peak_edit_mode = false;
    FitBaseLine();
    Update();
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

void ThermogramWidget::CalibrateSystem()
{
    /* avoid asking an empty spectrum */
    if (m_spec.Step() == 0)
        return;

    m_calibration_peak.setPeakStart(m_spec.size() - 1 - m_calibration_start->value() * m_spec.Step());
    m_calibration_peak.setPeakEnd(m_spec.size() - 1);

    std::vector<PeakPick::Peak> list;
    list.push_back(m_calibration_peak);

    PeakPick::spectrum* spectrum = new PeakPick::spectrum(m_spec);

    PeakPick::BaseLine baseline(spectrum);

    baseline.setBaseLineRange(PeakPick::BaseLine::BLR::PeakWise);
    baseline.setNoCoeffs(2);
    baseline.setPeaks(&list);
    baseline.setInitialBaseLine(Vector(0));

    PeakPick::BaseLineResult baseline_result = baseline.Fit();

    m_calibration_peak.integ_num = PeakPick::IntegrateNumerical(spectrum, m_calibration_peak.start, m_calibration_peak.end, baseline_result.baselines[0]);
    delete spectrum;
    m_calibration_label->setText(tr("<h4>Calibration (%1)</h4>").arg(m_calibration_peak.integ_num));
}

bool ThermogramWidget::CutAllLimits()
{
    Integrate(&m_peak_list, &m_spec);

    double threshold = 0;
    double old_threshold = m_integration_range_threshold->value();
    int maxiter = 1;
    int direction = -1 * m_direction->isChecked();
    int overshot = m_overshot->value();
    /* Zero/Threshold Cutting */
    if (m_integration_range->currentText() == m_Peak_Cut_Options[0]) {
        for (int i = 0; i < m_peak_list.size(); ++i) {
            m_peak_list[i].int_start = m_peak_list[i].start;
            m_peak_list[i].int_end = m_peak_list[i].end;
        }
        FitBaseLine();
        m_integration_range_threshold->setValue(m_initial_threshold);
        Integrate(&m_peak_list, &m_spec);
        Update();
        return false;
    } else if (m_integration_range->currentText() == m_Peak_Cut_Options[2]) { /* Threshold cutting */
        threshold = m_integration_range_threshold->value();
        maxiter = m_iterations->value();
    } else { /* Zero Cutting */
        for (int i = 0; i < m_peak_list.size(); ++i) {
            m_peak_list[i].int_start = m_peak_list[i].start;
            m_peak_list[i].int_end = m_peak_list[i].end;
        }
        FitBaseLine();
        Integrate(&m_peak_list, &m_spec);
        m_integration_range_threshold->setValue(m_initial_threshold);
    }
    Vector baseline;
    if (m_baseline.baselines.size() > 0)
        baseline = m_baseline.baselines[0];
    int x = 0;

    for (x = 0; x < maxiter; ++x) {
        for (int i = 0; i < m_peak_list.size(); ++i) {
            if (m_peak_list.size() == m_baseline.baselines.size()) // && m_baseline.x_grid_points.size() > 0)
                baseline = m_baseline.baselines[i];

            qreal start_end = (m_spec.Y(m_peak_list[i].start) + m_spec.Y(m_peak_list[i].end)) * 0.5;

            int start_position = m_peak_list[i].max;
            if (qAbs(qAbs(m_spec.Y(m_peak_list[i].min) - start_end)) > qAbs(qAbs(m_spec.Y(m_peak_list[i].max)) - start_end))
                start_position = m_peak_list[i].min;

            PeakPick::ResizeIntegrationRange(&m_spec, &m_peak_list[i], baseline, start_position, threshold, overshot, direction);
        }
        FitBaseLine();
        Integrate(&m_peak_list, &m_spec);
        if (qAbs(old_threshold - m_integration_range_threshold->value()) < 1e-10)
            break;
        old_threshold = m_integration_range_threshold->value();
    }
    m_last_iteration_max = x;
    FitBaseLine();
    Update();
    setGuideText(QString("Adaption of the baseline took %1 cycles").arg(m_last_iteration_max));

    return true;
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

void ThermogramWidget::ConvertRules()
{
    m_peak_rule_list->clear();
    m_peak_rule_list->setRowCount(m_peak_list.size());

    QStringList header = QStringList() << "Start Time\n[s]"
                                       << "Peak Duration\n[s]";
    m_peak_rule_list->setHorizontalHeaderLabels(header);

    for (int i = 0; i < m_peak_list.size(); ++i) {
        PeakRule* item = new PeakRule(QString::number((m_peak_list[i].start + 1) * m_spec.Step() + m_spec.XMin()));
        m_peak_rule_list->setItem(i, 0, item);
        item = new PeakRule(QString::number((m_peak_list[i].end - m_peak_list[i].start + 1) * m_spec.Step()));
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
        }
    }
    m_peak_rule_list->sortByColumn(0, Qt::AscendingOrder);
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
        QTableWidgetItem* item = (m_peak_rule_list->item(i, 0));
        stream << item->data(Qt::DisplayRole).toString() << "\t";
        item = m_peak_rule_list->item(i, 1);
        stream << item->data(Qt::DisplayRole).toString();
        if (i < m_peak_rule_list->rowCount() - 1)
            stream << "\n";
    }
}
