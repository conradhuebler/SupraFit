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
#include "src/ui/guitools/instance.h"
#include "src/ui/guitools/waiter.h"

#include "src/core/toolset.h"
#include "src/global.h"

#include "thermogramwidget.h"

static int baseline_step_size = 20;

ThermogramWidget::ThermogramWidget(QWidget* parent)
    : QWidget(parent)
{
    CreateSeries();
    setUi();
}

void ThermogramWidget::setUi()
{
    m_thermogram = new ChartView;
    connect(m_thermogram, &ChartView::LastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    connect(m_thermogram, &ChartView::AddRect, this, &ThermogramWidget::AddRectanglePeak);

    m_thermogram->setVerticalLineEnabled(true);
    //m_thermogram->setSelectionStrategie(2);

    connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, m_thermogram, &ChartView::ConfigurationChanged);
    m_thermogram->setModal(true);
    m_thermogram->setMinimumSize(600, 450);

    m_table = new QTableWidget;
    connect(m_table, &QTableWidget::doubleClicked, this, QOverload<const QModelIndex&>::of(&ThermogramWidget::PeakDoubleClicked));

    m_peak_rule_list = new QTableWidget;
    m_peak_rule_list->setRowCount(1);
    m_peak_rule_list->setColumnCount(2);
    m_peak_rule_list->setContextMenuPolicy(Qt::ActionsContextMenu);
    QAction* action = new QAction("Add Rule");
    action->setIcon(Icon("list-add"));
    connect(action, &QAction::triggered, m_peak_rule_list, [this]() {
        int rows = m_peak_rule_list->rowCount();
        m_peak_rule_list->setRowCount(m_peak_rule_list->rowCount() + 1);
        QTableWidgetItem* item = new QTableWidgetItem(QString::number(m_peaks_start->value()));
        m_peak_rule_list->setItem(rows, 0, item);
        item = new QTableWidgetItem(QString::number(m_peaks_time->value()));
        m_peak_rule_list->setItem(rows, 1, item);
        m_current_peaks_rule = rows;
    });
    m_peak_rule_list->addAction(action);

    action = new QAction("Remove (last) Rule");
    connect(action, &QAction::triggered, m_peak_rule_list, [this]() {
        if (m_peak_rule_list->rowCount() > 1) {
            //int rows = m_peak_rule_list->rowCount() -2;
            m_peak_rule_list->setRowCount(m_peak_rule_list->rowCount() - 1);
            m_current_peaks_rule = 0;
        }
    });
    m_peak_rule_list->addAction(action);

    connect(m_peak_rule_list, &QTableWidget::doubleClicked, this, QOverload<const QModelIndex&>::of(&ThermogramWidget::PeakRuleDoubleClicked));

    QStringList header = QStringList() << "start"
                                       << "interval";
    m_peak_rule_list->setHorizontalHeaderLabels(header);

    QTabWidget* peaks_tab = new QTabWidget;
    peaks_tab->addTab(m_table, tr("Peak List"));
    peaks_tab->addTab(m_peak_rule_list, tr("Peak-Pick Rules"));

    QHBoxLayout* hlayout = new QHBoxLayout;

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->tr("thermogram_splitter");
    m_splitter->addWidget(m_thermogram);
    m_splitter->addWidget(peaks_tab);
    hlayout->addWidget(m_splitter);

    QWidget* chart = new QWidget;

    chart->setLayout(hlayout);

    QWidget* baseline = new QWidget;

    QGridLayout* baselayout = new QGridLayout;

    m_baseline_type = new QComboBox;
    QStringList options = QStringList() << tr("none") << tr("offset") << tr("constant") << tr("polynomial");
    m_baseline_type->addItems(options);
    m_baseline_type->setCurrentText("polynomial");
    connect(m_baseline_type, &QComboBox::currentTextChanged, this, &ThermogramWidget::UpdateBaseLine);

    m_fit_type = new QComboBox;
    options = QStringList() << tr("all") << tr("peaks") << tr("cutoff");
    m_fit_type->addItems(options);
    m_fit_type->setCurrentText("peaks");
    m_fit = "peaks";
    connect(m_fit_type, &QComboBox::currentTextChanged, this, &ThermogramWidget::UpdateFit);

    baselayout->addWidget(new QLabel(tr("Baseline corretion"), 0, 0));
    baselayout->addWidget(m_baseline_type, 0, 1);
    baselayout->addWidget(new QLabel(tr("Baseline fit type:")), 0, 2);
    baselayout->addWidget(m_fit_type, 0, 3);

    m_poly_slow = new QCheckBox(tr("Slow Fit"));
    m_poly_slow->setChecked(true);
    baselayout->addWidget(m_poly_slow, 0, 4);

    m_limits = new QCheckBox(tr("Show Fit limit"));
    /*
    baselayout->addWidget(m_limits, 0, 5);
    connect(m_limits, &QCheckBox::stateChanged, this, [this](int state) {
        m_upper->setVisible(state);
        m_lower->setVisible(state);
        UpdateLimits();
    });
    */
    m_smooth = new QCheckBox(tr("Smooth"));
    connect(m_smooth, &QCheckBox::stateChanged, this, &ThermogramWidget::UpdatePlot);
    connect(m_smooth, &QCheckBox::stateChanged, this, &ThermogramWidget::Update);

    m_coeffs = new QSpinBox;
    m_coeffs->setMinimum(1);
    m_coeffs->setMaximum(120);
    m_coeffs->setValue(3);
    connect(m_coeffs, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThermogramWidget::FitBaseLine);

    m_constant = new QLineEdit(QString::number(m_offset));
    connect(m_constant, &QLineEdit::textChanged, this, &ThermogramWidget::UpdateLimits);

    m_filter = new QSpinBox;
    m_filter->setMinimum(1);
    m_filter->setMaximum(12);
    m_filter->setValue(3);
    connect(m_filter, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThermogramWidget::UpdatePlot);
    connect(m_filter, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThermogramWidget::Update);

    baselayout->addWidget(new QLabel(tr("Constant offset")), 1, 0);
    baselayout->addWidget(m_constant, 1, 1);
    baselayout->addWidget(new QLabel(tr("Coefficents:")), 1, 2);
    baselayout->addWidget(m_coeffs, 1, 3);
    /*
    baselayout->addWidget(m_smooth, 1, 4);
    baselayout->addWidget(m_filter, 1, 5);
    */

    m_stdev = new QLineEdit;
    connect(m_stdev, &QLineEdit::textChanged, this, &ThermogramWidget::UpdateLimits);

    m_mult = new QLineEdit("1");
    connect(m_mult, &QLineEdit::textChanged, this, &ThermogramWidget::UpdateLimits);

    /*
    baselayout->addWidget(new QLabel(tr("Stddev")), 2, 0);
    baselayout->addWidget(m_stdev, 2, 1);
    baselayout->addWidget(new QLabel(tr("Multiplier")), 2, 2);
    baselayout->addWidget(m_mult, 2, 3);
    */
    m_fit_button = new QPushButton(tr("Refit Baseline"));
    // baselayout->addWidget(m_fit_button, 2, 4);
    connect(m_fit_button, &QPushButton::clicked, this, &ThermogramWidget::FitBaseLine);

    m_baseline_polynom = new QPlainTextEdit;
    m_baseline_polynom->setReadOnly(true);

    QPalette p = m_baseline_polynom->palette();
    p.setColor(QPalette::Active, QPalette::Base, Qt::lightGray);
    p.setColor(QPalette::Inactive, QPalette::Base, Qt::lightGray);
    p.setColor(QPalette::Inactive, QPalette::Text, Qt::black);
    p.setColor(QPalette::Inactive, QPalette::Window, Qt::black);
    m_baseline_polynom->setPalette(p);

    baselayout->addWidget(m_baseline_polynom, 3, 0, 1, 4);
    baseline->setLayout(baselayout);

    m_full_spec = new QRadioButton(tr("Full Spectrum"));
    m_peak_wise = new QRadioButton(tr("Peak-wise"));

    connect(m_full_spec, &QRadioButton::toggled, this, [this]() {
        m_fit_type->setDisabled(!m_full_spec->isChecked());
        m_coeffs->setDisabled(!m_full_spec->isChecked());
        m_constant->setDisabled(!m_full_spec->isChecked());
        FitBaseLine();
    });

    QHBoxLayout* base_input = new QHBoxLayout;
    base_input->addWidget(m_full_spec);
    base_input->addWidget(m_peak_wise);
    base_input->addWidget(m_fit_button);
    m_peak_wise->setChecked(true);
    m_full_spec->setChecked(false);

    m_const_offset = new QDoubleSpinBox;
    m_const_offset->setMinimum(-1e5);
    m_const_offset->setMaximum(1e5);
    m_const_offset->setValue(0);
    m_const_offset->setDecimals(7);
    connect(m_const_offset, &QDoubleSpinBox::editingFinished, this, &ThermogramWidget::Update);

    m_peaks_start = new QDoubleSpinBox;
    m_peaks_start->setMinimum(0);
    m_peaks_start->setMaximum(1e8);
    connect(m_peaks_start, &QDoubleSpinBox::editingFinished, this, &ThermogramWidget::Divide2Peaks);

    m_get_peaks_start = new QPushButton(tr("Select"));
    connect(m_get_peaks_start, &QPushButton::clicked, this, [this]() {
        m_get_time_from_thermogram = 1;
        connect(m_thermogram, &ChartView::PointDoubleClicked, this, &ThermogramWidget::PointDoubleClicked);
    });

    m_peaks_end = new QDoubleSpinBox;
    m_peaks_end->setMinimum(0);
    m_peaks_end->setMaximum(1e8);
    connect(m_peaks_end, &QDoubleSpinBox::editingFinished, this, &ThermogramWidget::Divide2Peaks);

    m_get_peaks_end = new QPushButton(tr("Select"));
    connect(m_get_peaks_end, &QPushButton::clicked, this, [this]() {
        m_get_time_from_thermogram = 2;
        connect(m_thermogram, &ChartView::PointDoubleClicked, this, &ThermogramWidget::PointDoubleClicked);
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

    m_calibration_label = new QLabel(tr("Calibration (0)"));

    m_peak_apply = new QPushButton(tr("Apply and Update"));
    connect(m_peak_apply, &QPushButton::clicked, this, &ThermogramWidget::UpdatePeaks);

    m_get_peaks_range = new QPushButton(tr("Select Peak Range"));
    connect(m_get_peaks_range, &QPushButton::clicked, this, [this]() {
        m_thermogram->setSelectStrategy(SelectStrategy::S_Horizontal);
    });

    m_peak_count = new QSpinBox;
    m_peak_count->setRange(1, 1e9);

    m_peak_sensitivity = new QSpinBox;
    m_peak_sensitivity->setRange(1, 10);
    m_peak_sensitivity->setValue(3);

    m_auto_pick = new QPushButton(tr("Auto Picking"));
    connect(m_auto_pick, &QPushButton::clicked, this, &ThermogramWidget::PickPeaks);

    QHBoxLayout* peak_layout = new QHBoxLayout;
    /*
    peak_layout->addWidget(new QLabel(tr("Remove constant")));
    peak_layout->addWidget(m_const_offset);
    peak_layout->addWidget(m_peak_sensitivity);
    peak_layout->addWidget(m_auto_pick);
    peak_layout->addWidget(new QLabel(tr("Peak Count")));
    peak_layout->addWidget(m_peak_count);
    */

    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->addWidget(new QLabel(tr("Thermogram starts (s)")));
    vlayout->addWidget(m_get_peaks_start);
    vlayout->addWidget(m_peaks_start);
    peak_layout->addLayout(vlayout);

    vlayout = new QVBoxLayout;
    vlayout->addWidget(new QLabel(tr("Thermogram ends (s)")));
    vlayout->addWidget(m_get_peaks_end);
    vlayout->addWidget(m_peaks_end);
    peak_layout->addLayout(vlayout);

    vlayout = new QVBoxLayout;
    vlayout->addWidget(new QLabel(tr("Time between injects (s)")));
    vlayout->addWidget(m_peaks_time);
    vlayout->addWidget(m_get_peaks_range);
    peak_layout->addLayout(vlayout);

    vlayout = new QVBoxLayout;
    vlayout->addWidget(m_calibration_label);
    vlayout->addWidget(m_calibration_start);
    vlayout->addWidget(m_calibration_heat);
    peak_layout->addLayout(vlayout);

    peak_layout->addWidget(m_peak_apply);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addLayout(peak_layout);
    layout->addWidget(chart);
    layout->addLayout(base_input);
    layout->addWidget(baseline);

    setLayout(layout);

    m_fit_type->setDisabled(true);
    m_coeffs->setDisabled(true);
    m_constant->setDisabled(true);

    m_stdev->setEnabled(false);
    m_mult->setEnabled(false);

    QSettings settings;
    settings.beginGroup("thermogram");
    m_peaks_start->setValue(settings.value("peaks_start", 60).toDouble());
    m_peaks_time->setValue(settings.value("peaks_time", 150).toDouble());
    m_splitter->restoreState(settings.value("splitterSizes").toByteArray());
    QTableWidgetItem* item = new QTableWidgetItem(QString::number(m_peaks_start->value()));
    m_peak_rule_list->setItem(m_current_peaks_rule, 0, item);
    item = new QTableWidgetItem(QString::number(m_peaks_time->value()));
    m_peak_rule_list->setItem(m_current_peaks_rule, 1, item);
}

ThermogramWidget::~ThermogramWidget()
{
    QSettings settings;
    settings.beginGroup("thermogram");
    settings.setValue("peaks_start", m_peaks_start->value());
    settings.setValue("peaks_time", m_peaks_time->value());
    settings.setValue("splitterSizes", m_splitter->saveState());
}

void ThermogramWidget::setThermogram(PeakPick::spectrum* spec, qreal offset)
{
    m_spec = *spec;
    m_frequency = spec->Step();
    if (offset == 0.0)
        m_offset = m_spec.Mean();
    else
        m_offset = offset;

    m_spectrum = true;
    m_baseline.baselines.push_back(Vector(1));
    m_baseline.baselines[0](0) = m_offset;

    PeakPick::spectrum sign = PeakPick::spectrum(m_spec);
    sign.InvertSgn();

    std::vector<PeakPick::Peak> peaks = PeakPick::PickPeaks(&sign, 0, qPow(2, 1));
    std::vector<PeakPick::Peak> max_peak = PeakPick::PickPeaks(&m_spec, 0, qPow(2, 1));

    m_peak_count->setValue(peaks.size() + max_peak.size());

    m_peaks_start->setMaximum(m_spec.XMax());
    m_peaks_end->setMaximum(m_spec.XMax() - m_calibration_start->value() * m_spec.Step());

    m_peaks_start->setValue(m_spec.XMin());
    m_peaks_end->setValue(m_spec.XMax() - m_calibration_start->value() * m_spec.Step());

    UpdatePlot();
    m_stdev->setText(QString::number(spec->StdDev()));
    m_constant->setText(QString::number(spec->Mean()));
}

void ThermogramWidget::setPeakList(const std::vector<PeakPick::Peak>& peak_list)
{
    m_peak_list = peak_list;
    if (m_peak_list.size()) {
        m_peaks_start->setValue(m_peak_list[0].start * m_spec.Step() - m_spec.Step());
        //m_peaks_time->setValue(m_peak_list[0].end * m_spec.Step() - m_peak_list[0].start * m_spec.Step() + m_spec.Step());
        m_peaks_end->setValue(m_peak_list[m_peak_list.size() - 1].end * m_spec.Step());
    }
    m_peak_count->setValue(m_peak_list.size());
    Update();
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

        /*QSpinBox* box = new QSpinBox(m_table);
        box->setMaximum(1e9);
        box->setMinimum(-1e9);
        box->setValue(m_spec.X(m_peak_list[j].start));*/
        /*connect(box, &QSpinBox::editingFinished, this, [this, j, box]() {
            PeakChanged(j, 1, box->value() / m_frequency); //FIXME hack
        });*/
        //m_table->setCellWidget(j, 1, box);
        m_table->setItem(j, 1, newItem);

        newItem = new QTableWidgetItem(QString::number(m_spec.X(m_peak_list[j].end)));
        newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        /*box = new QSpinBox(m_table);
        box->setMaximum(1e9);
        box->setMinimum(-1e9);
        box->setValue(m_spec.X(m_peak_list[j].end));*/
        /*connect(box, &QSpinBox::editingFinished, this, [this, j, box]() {
            PeakChanged(j, 2, box->value() / m_frequency); //FIXME hack
        });*/
        //m_table->setCellWidget(j, 2, box);
        m_table->setItem(j, 2, newItem);
    }
    QStringList header = QStringList() << "heat\n[raw]"
                                       << "peak start\n[s]"
                                       << "peak end\n[s]";
    m_table->setHorizontalHeaderLabels(header);
    m_table->resizeColumnsToContents();

    if (m_baseline.baselines.size() == 1) {
        QString string("<html><p>f(x) = ");
        for (unsigned int i = 0; i < m_baseline.baselines[0].size(); ++i) {
            if (i != 0)
                string += " (" + QString::number(m_baseline.baselines[0](i)) + ") x<sup>" + QString::number(i) + "</sup> +";
            else
                string += QString::number(m_baseline.baselines[0](i)) + " +";
            if (i % 5 == 0 && i != 0 && (i < m_baseline.baselines[0].size() - 1)) {
                string += "</p><p>     = ";
            }
        }
        string.chop(1);
        string += "</p></html>";

        m_baseline_polynom->clear();
        m_baseline_polynom->appendHtml(string);
    } else
        m_baseline_polynom->clear();
}

void ThermogramWidget::UpdatePlot()
{
    fromSpectrum(&m_spec, m_thermogram_series);
    m_thermogram->addSeries(m_thermogram_series);
}

void ThermogramWidget::UpdateLimits()
{
    if (!m_limits->isChecked())
        return;

    qreal constant;

    if (m_baseline_type->currentText() == "constant")
        constant = m_constant->text().toDouble();
    else
        constant = m_offset;
    qreal stdev = m_stdev->text().toDouble();
    qreal mult = m_mult->text().toDouble();
    Vector coeff = Vector(1);
    coeff(0) = constant + (mult * stdev);
    fromPolynomial(coeff, m_upper);
    m_thermogram->addSeries(m_upper);

    coeff(0) = constant - (mult * stdev);
    fromPolynomial(coeff, m_lower);
    m_thermogram->addSeries(m_lower);
}

void ThermogramWidget::PickPeaks()
{
    if (!m_spectrum)
        return;

    PeakPick::spectrum sign = PeakPick::spectrum(m_spec);
    sign.InvertSgn();

    std::vector<PeakPick::Peak> peaks = PeakPick::PickPeaks(&sign, 0, qPow(10, m_peak_sensitivity->value()), m_spec.XtoIndex(m_peaks_start->value()), m_spec.XtoIndex(m_peaks_end->value()));
    for (unsigned int i = 0; i < peaks.size(); ++i) {
        int pos = PeakPick::FindMinimum(&m_spec, peaks[i]);
        peaks[i].max = pos;
        PeakPick::IntegrateNumerical(&m_spec, peaks[i]);
    }

    std::vector<PeakPick::Peak> max_peak = PeakPick::PickPeaks(&m_spec, 0, qPow(10, m_peak_sensitivity->value()), m_spec.XtoIndex(m_peaks_start->value()), m_spec.XtoIndex(m_peaks_end->value()));

    for (int i = 0; i < int(max_peak.size()); ++i) {
        int pos = PeakPick::FindMaximum(&m_spec, max_peak[i]);
        max_peak[i].max = pos;
        PeakPick::IntegrateNumerical(&m_spec, max_peak[i]);
    }
    peaks.insert(peaks.end(), max_peak.begin(), max_peak.end());
    setPeakList(peaks);
}

void ThermogramWidget::fromSpectrum(const PeakPick::spectrum* original, LineSeries* series)
{
    PeakPick::spectrum* spectrum = new PeakPick::spectrum(original);
    if (m_smooth->isChecked())
        SmoothFunction(spectrum, m_filter->value());

    series->clear();
    for (unsigned int i = 1; i <= spectrum->size(); i++) {
        series->append(QPointF(spectrum->X(i), spectrum->Y(i)));
    }
}

void ThermogramWidget::fromPolynomial(const Vector& coeff, LineSeries* series)
{
    series->clear();
    for (unsigned int i = 1; i <= m_spec.size(); i++)
        series->append(QPointF(m_spec.X(i), PeakPick::Polynomial(m_spec.X(i), coeff)));
}

void ThermogramWidget::clear()
{
    m_peak_list.clear();
    m_spectrum = false;
    m_table->clear();
    m_thermogram_series->clear();
    m_baseline_series->clear();
    m_upper->clear();
    m_lower->clear();
    m_left->clear();
    m_right->clear();
    m_base_grids->clear();
}

void ThermogramWidget::Update()
{
    qint64 t0 = QDateTime::currentMSecsSinceEpoch();
    PeakPick::spectrum* spectrum = new PeakPick::spectrum(m_spec);
    if (m_smooth->isChecked())
        SmoothFunction(spectrum, m_filter->value());
    Integrate(&m_peak_list, spectrum);
    //qDebug() << QDateTime::currentMSecsSinceEpoch() - t0 << " for integration" << this;
    t0 = QDateTime::currentMSecsSinceEpoch();
    delete spectrum;
    UpdateTable();
    //qDebug() << QDateTime::currentMSecsSinceEpoch() - t0 << " for remaining part" << this;
}

void ThermogramWidget::Integrate(std::vector<PeakPick::Peak>* peaks, const PeakPick::spectrum* original)
{
    CalibrateSystem();

    Waiter wait;
    m_base_grids->clear();
    m_baseline_series->clear();
    m_integrals_raw.clear();
    Vector baseline;
    if (m_baseline.baselines.size() > 0)
        baseline = m_baseline.baselines[0];

    for (int i = 0; i < int(peaks->size()); ++i) {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        if (peaks->size() == m_baseline.baselines.size() && m_baseline.x_grid_points.size() > 0) {
            baseline = m_baseline.baselines[i];
            for (int j = 0; j < int(m_baseline.x_grid_points[i].size()); ++j) {
                m_base_grids->append(m_baseline.x_grid_points[i][j], m_baseline.y_grid_points[i][j]);
            }
        }
        (*peaks)[i].max = ((*peaks)[i].end + (*peaks)[i].start) / 2.0;
        PeakPick::IntegrateNumerical(original, (*peaks)[i], baseline);
        m_integrals_raw << (*peaks)[i].integ_num;

        for (int j = (*peaks)[i].start; j <= int((*peaks)[i].end); j += baseline_step_size)
            m_baseline_series->append(QPointF(m_spec.X(j + 1), PeakPick::Polynomial(m_spec.X(j + 1), baseline)));
    }

    if (m_baseline.x_grid_points.size() == 1) {
        for (int j = 0; j < int(m_baseline.x_grid_points[0].size()); ++j) {
            m_base_grids->append(m_baseline.x_grid_points[0][j], m_baseline.y_grid_points[0][j]);
        }
    }
    ApplyCalibration();
    m_base_grids->setMarkerSize(8);
    m_thermogram->addSeries(m_base_grids);
    m_thermogram->addSeries(m_baseline_series);
}

void ThermogramWidget::ApplyCalibration()
{
    for (int i = 0; i < int(m_integrals_raw.size()); ++i) {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        if (m_calibration_heat->value() != 0)
            (m_peak_list)[i].integ_num = m_integrals_raw[i] * m_calibration_heat->value() / m_calibration_peak.integ_num;

        (m_peak_list)[i].integ_num -= m_const_offset->value();
    }
    UpdateTable();
    emit Offset();
}

void ThermogramWidget::UpdateBaseLine(const QString& str)
{
    //if (str == m_base || m_block)
    //return;
    //m_thermogram->zoo
    Vector coeff;

    if (str == "none") {
        coeff = Vector(1);
        coeff(0) = 0;
        m_limits->setChecked(false);

        m_fit_type->setDisabled(true);
        m_coeffs->setDisabled(true);
        m_constant->setDisabled(true);

        m_stdev->setEnabled(false);
        m_mult->setEnabled(false);

    } else if (str == "offset") {
        coeff = Vector(1);
        coeff(0) = m_offset;

        m_fit_type->setDisabled(true);
        m_coeffs->setDisabled(true);
        m_constant->setDisabled(true);

        m_stdev->setEnabled(false);
        m_mult->setEnabled(false);

    } else if (str == "constant") {
        coeff = Vector(1);

        m_fit_type->setDisabled(true);
        m_coeffs->setDisabled(true);
        m_constant->setDisabled(false);

        m_stdev->setEnabled(false);
        m_mult->setEnabled(false);

        coeff(0) = m_constant->text().toDouble();
    } else if (str == "polynomial") {
        coeff = Vector(m_coeffs->value());
        for (int i = 0; i < coeff.size(); ++i)
            coeff(i) = 1;

        m_fit_type->setDisabled(false);
        m_coeffs->setDisabled(false);
        m_constant->setDisabled(false);
        m_stdev->setEnabled(true);
        m_mult->setEnabled(true);
    }
    m_baseline.baselines.clear();
    m_baseline.baselines.push_back(coeff);
    FitBaseLine();
    UpdatePlot();

    emit IntegrationChanged();

    m_base = str;
}

void ThermogramWidget::UpdateFit(const QString& str)
{
    //if (str == m_fit)
    //    return;

    FitBaseLine();
    m_fit = str;
}

void ThermogramWidget::FitBaseLine()
{
    if (m_baseline_type->currentText() != "polynomial")
        return;
    Waiter wait;

    PeakPick::spectrum* spectrum = new PeakPick::spectrum(m_spec);
    if (m_smooth->isChecked())
        SmoothFunction(spectrum, m_filter->value());

    PeakPick::BaseLine baseline(spectrum);
    if (m_full_spec->isChecked())
        baseline.setBaseLineRange(PeakPick::BaseLine::BLR::FullSpectrum);
    else if (m_peak_wise->isChecked())
        baseline.setBaseLineRange(PeakPick::BaseLine::BLR::PeakWise);
    else
        baseline.setBaseLineRange(PeakPick::BaseLine::BLR::FullSpectrum);

    baseline.setNoCoeffs(m_coeffs->value());

    qreal constant = m_constant->text().toDouble();

    qreal stdev = m_stdev->text().toDouble();
    qreal mult = m_mult->text().toDouble();

    if (m_fit == "cutoff") {
        baseline.setLower(constant - (mult * stdev));
        baseline.setUpper(constant + (mult * stdev));
    } else if (m_fit == "peaks") {
        baseline.setPeaks(&m_peak_list);
    } else if (m_fit == "all") {
        baseline.setLower(0);
        baseline.setUpper(0);
    }

    if (m_poly_slow->isChecked())
        baseline.setPolynomFit(PeakPick::BaseLine::Polynom::Slow);
    else
        baseline.setPolynomFit(PeakPick::BaseLine::Polynom::Fast);

    baseline.setInitialBaseLine(m_initial_baseline);

    /* QThread *thread = QThread::create([this, baseline]{ */ m_baseline = baseline.Fit(); /*});*/

    if (m_baseline.baselines.size() == 1) {
        m_initial_baseline = m_baseline.baselines[0];
        m_coeffs->setValue(m_initial_baseline.size());
    }

    delete spectrum;
    Update();
    //emit IntegrationChanged();
}

void ThermogramWidget::PeakRuleDoubleClicked(const QModelIndex& index)
{
    int peak = index.row();
    m_current_peaks_rule = peak;
    QTableWidgetItem* item = m_peak_rule_list->item(index.row(), 0);
    m_peaks_start->setValue(item->data(Qt::DisplayRole).toDouble());
    // PeakDoubleClicked(peak);
}

void ThermogramWidget::PeakDoubleClicked(const QModelIndex& index)
{
    int peak = index.row();
    return;
    PeakDoubleClicked(peak);
}

void ThermogramWidget::PeakDoubleClicked(int peak)
{
    int lower = peak, upper = peak;

    if (peak > 1)
        lower = peak - 1;

    if (peak < m_peak_list.size() - 1)
        upper = peak + 1;

    qreal ymax = m_thermogram->YMaxRange();
    qreal ymin = m_thermogram->YMinRange();

    qreal xmin;
    qreal xmax;

    xmin = m_spec.X(m_peak_list[peak].start);
    xmax = m_spec.X(m_peak_list[peak].end);

    m_left->clear();
    m_left->append(xmin, ymin);
    m_left->append(xmin, ymax);
    m_thermogram->addSeries(m_left);

    m_right->clear();
    m_right->append(xmax, ymin);
    m_right->append(xmax, ymax);
    m_thermogram->addSeries(m_right);

    xmin = m_spec.X(m_peak_list[lower].start);
    xmax = m_spec.X(m_peak_list[upper].end);

    m_thermogram->setXRange(xmin, xmax);
    m_thermogram->setYRange(ymin, ymax);
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
    m_baseline_type->setCurrentText(fit["baseline"].toString());
    m_fit_type->setCurrentText(fit["baseline_fit"].toString());
    m_coeffs->setValue(fit["coeffs"].toInt());
    m_const_offset->setValue(fit["constants"].toDouble());
    m_stdev->setText(fit["stddev"].toString());
    m_mult->setText(fit["multiplier"].toString());
    if (fit.contains("smooth")) {
        m_smooth->setChecked(true);
        m_filter->setValue(fit["SV"].toInt());
    }
    if (fit["peakwise"].toBool() == true)
        m_peak_wise->setChecked(true);
    else
        m_full_spec->setChecked(true);

    if (fit.contains("slow")) {
        m_poly_slow->setChecked(fit["slow"].toBool());
    } else
        m_poly_slow->setChecked(false);
    m_peaks_start->setValue(fit["start_time"].toDouble());
    m_peaks_time->setValue(fit["peak_time"].toDouble());
    m_peaks_end->setValue(fit["end_time"].toDouble());
    m_block = false;
    UpdatePeaks();
}

QJsonObject ThermogramWidget::Fit() const
{
    QJsonObject fit;
    fit["baseline"] = m_baseline_type->currentText();
    fit["baseline_fit"] = m_fit_type->currentText();
    fit["coeffs"] = m_coeffs->value();
    fit["constants"] = m_const_offset->value();
    fit["stddev"] = m_stdev->text();
    fit["multiplier"] = m_mult->text();
    if (m_smooth->isChecked()) {
        fit["smooth"] = "SG";
        fit["SV"] = m_filter->value();
    }
    fit["peakwise"] = m_peak_wise->isChecked();
    fit["slow"] = m_poly_slow->isChecked();
    fit["frequency"] = m_frequency;
    fit["start_time"] = m_peaks_start->value();
    fit["end_time"] = m_peaks_end->value();
    fit["peak_time"] = m_peaks_time->value();
    return fit;
}

void ThermogramWidget::CreateSeries()
{
    m_thermogram_series = new LineSeries;
    m_baseline_series = new LineSeries;
    //m_baseline_series->setDashDotLine(true);
    m_upper = new LineSeries;
    m_lower = new LineSeries;
    m_left = new LineSeries;
    m_right = new LineSeries;
    //m_calibration_line = new LineSeries;
    m_base_grids = new ScatterSeries;
    //m_calibration_grid = new ScatterSeries;
}

void ThermogramWidget::UpdatePeaks()
{
    qint64 t0 = QDateTime::currentMSecsSinceEpoch();
    qreal offset;
    int off = 0;
    //double start = m_peaks_start->value();
    int rules_size = m_peak_rule_list->rowCount();
    double start = m_peak_rule_list->item(0, 0)->data(Qt::UserRole).toDouble();
    double end = m_peaks_end->value();
    m_peak_list.clear();
    PeakPick::Peak peak;

    if (m_filetype == FileType::ITC) {
        for (unsigned int i = 1; i <= m_spec.size(); ++i) {
            if (m_spec.X(i) < start) {
                off++;
                offset += m_spec.Y(i);
            } else {
                qreal mod = m_spec.X(i) - int(start) - m_spec.X(1);
                // qDebug() << mod << m_peaks_time->value();
                if (std::fmod(mod, m_peaks_time->value()) == 0) {
                    qDebug() << m_peaks_time->value() << mod / m_peaks_time->value();
                    if (!(qFuzzyCompare(mod / m_peaks_time->value(), 0))) {
                        peak.end = i - 1;
                        m_peak_list.push_back(peak);
                    }
                    peak = PeakPick::Peak();
                    peak.start = i;
                }
            }
        }
    } else {
        for (int j = 0; j < rules_size; ++j) {
            QTableWidgetItem* item = (m_peak_rule_list->item(j, 0));
            double index_start = m_spec.XtoIndex(item->data(Qt::DisplayRole).toDouble()); //m_spec.XtoIndex(m_peak_rule_list->item(j, 0)->data(Qt::UserRole).toDouble());
            item = m_peak_rule_list->item(j, 1);
            double timestep = m_spec.XtoIndex(item->data(Qt::DisplayRole).toDouble()); //m_spec.XtoIndex(m_peak_rule_list->item(j, 1)->data(Qt::UserRole).toDouble());
            double index_end;

            if (j == rules_size - 1)
                index_end = end;
            else
                index_end = m_peak_rule_list->item(j + 1, 0)->data(Qt::DisplayRole).toDouble();

            qDebug() << index_start << timestep << index_end;

            for (int i = index_start; i + (timestep)-1 < m_spec.XtoIndex(index_end); i += (timestep)) {
                peak = PeakPick::Peak();
                peak.start = i;
                peak.end = i + (timestep)-1;
                m_peak_list.push_back(peak);
            }
        }

        /*
        for (int i = index_start; i + (m_peaks_time->value()) / m_spec.Step() - 1 < m_spec.XtoIndex(end); i += (m_peaks_time->value()) / m_spec.Step()) {
            peak = PeakPick::Peak();
            peak.start = i;
            peak.end = i + (m_peaks_time->value()) / m_spec.Step() - 1;
            m_peak_list.push_back(peak);
        }*/
    }

    m_offset = offset / double(off);
    m_baseline.baselines.push_back(Vector(1));
    m_baseline.baselines[0](0) = m_offset;
    // qDebug() << QDateTime::currentMSecsSinceEpoch() - t0 << "for generating";
    t0 = QDateTime::currentMSecsSinceEpoch();

    FitBaseLine();
    // qDebug() << QDateTime::currentMSecsSinceEpoch() - t0 << "for baseline";
    t0 = QDateTime::currentMSecsSinceEpoch();
    Update();
    // qDebug() << QDateTime::currentMSecsSinceEpoch() - t0 << "to update the reminder";

    emit PeaksChanged();
    // emit IntegrationChanged();
}

void ThermogramWidget::AddRectanglePeak(const QPointF& point1, const QPointF& point2)
{
    m_thermogram->setSelectStrategy(SelectStrategy::S_None);
    qDebug() << point1 << point2;
    qDebug() << point2.x() - point1.x();
}

void ThermogramWidget::PointDoubleClicked(const QPointF& point)
{
    disconnect(m_thermogram, &ChartView::PointDoubleClicked, this, &ThermogramWidget::PointDoubleClicked);
    if (m_get_time_from_thermogram == 1) {
        m_peaks_start->setValue(point.x());
        m_peak_rule_list->item(m_current_peaks_rule, 0)->setData(Qt::DisplayRole, point.x());
    } else if (m_get_time_from_thermogram == 2)
        m_peaks_end->setValue(point.x());
    Divide2Peaks();
    m_get_time_from_thermogram = 0;
}

void ThermogramWidget::Divide2Peaks()
{
    return;
    if (m_peaks_start->value() < m_peaks_end->value()) {
        std::vector<PeakPick::Peak> peaks = PeakPick::Divide2Peaks(&m_spec, m_peaks_start->value(), m_peak_count->value(), m_peaks_end->value());
        setPeakList(peaks);
    }
}

void ThermogramWidget::CalibrateSystem()
{
    m_calibration_peak.start = m_spec.XtoIndex(m_spec.XMax() - m_calibration_start->value() * m_spec.Step());
    m_calibration_peak.end = m_spec.XtoIndex(m_spec.XMax());

    std::vector<PeakPick::Peak> list;
    list.push_back(m_calibration_peak);

    PeakPick::spectrum* spectrum = new PeakPick::spectrum(m_spec);

    PeakPick::BaseLine baseline(spectrum);

    baseline.setBaseLineRange(PeakPick::BaseLine::BLR::PeakWise);
    baseline.setNoCoeffs(2);
    baseline.setPeaks(&list);
    baseline.setInitialBaseLine(Vector(0));

    PeakPick::BaseLineResult baseline_result = baseline.Fit();
    // std::cout << baseline_result.baselines[0] << std::endl;
    /*
    m_calibration_grid->clear();
    for(int i = 0; i < baseline_result.baselines[0].size(); ++i)
        m_calibration_grid->append(m_baseline.x_grid_points[0][j], m_baseline.y_grid_points[0][j]);

        m_baseline_series->append(QPointF(m_spec.X(j + 1), PeakPick::Polynomial(m_spec.X(j + 1), baseline)));
    m_calibration_line->clear(),
    */
    m_calibration_peak.integ_num = PeakPick::IntegrateNumerical(spectrum, m_calibration_peak.start, m_calibration_peak.end, baseline_result.baselines[0]);
    // qDebug() << m_calibration_peak.integ_num<< m_calibration_peak.start<< m_calibration_peak.end;
    delete spectrum;
    m_calibration_label->setText(tr("Calibration (%1)").arg(m_calibration_peak.integ_num));
}
