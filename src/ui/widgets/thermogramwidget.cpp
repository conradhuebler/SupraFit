/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <Eigen/Dense>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableWidget>

#include <QtCharts/QChart>

#include "libpeakpick/baseline.h"
#include "libpeakpick/peakpick.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/widgets/chartview.h"

#include "src/core/toolset.h"
#include "src/global.h"

#include "thermogramwidget.h"

ThermogramWidget::ThermogramWidget(QWidget* parent)
    : QWidget(parent)
{
    m_thermogram_series = new LineSeries;
    m_baseline_series = new LineSeries;
    m_upper = new LineSeries;
    m_lower = new LineSeries;
    m_left = new LineSeries;
    m_right = new LineSeries;

    setUi();
}

void ThermogramWidget::setUi()
{
    m_data = new QtCharts::QChart();
    m_thermogram = new ChartView(m_data);
    m_thermogram->setModal(true);
    m_thermogram->setMinimumSize(800, 600);

    m_table = new QTableWidget;
    m_table->setFixedWidth(250);
    connect(m_table, &QTableWidget::doubleClicked, this, QOverload<const QModelIndex&>::of(&ThermogramWidget::PeakDoubleClicked));

    QHBoxLayout* hlayout = new QHBoxLayout;

    hlayout->addWidget(m_thermogram);
    hlayout->addWidget(m_table);

    QWidget* chart = new QWidget;

    chart->setLayout(hlayout);

    QWidget* baseline = new QWidget;

    QGridLayout* baselayout = new QGridLayout;

    m_baseline_type = new QComboBox;
    QStringList options = QStringList() << tr("none") << tr("offset") << tr("constant") << tr("polynomial");
    m_baseline_type->addItems(options);
    m_baseline_type->setCurrentText("offset");
    connect(m_baseline_type, &QComboBox::currentTextChanged, this, &ThermogramWidget::UpdateBaseLine);

    m_fit_type = new QComboBox;
    options = QStringList() << tr("ignore") << tr("simple") << tr("peaks") << tr("cutoff") << tr("iterative cutoff");
    m_fit_type->addItems(options);
    connect(m_fit_type, &QComboBox::currentTextChanged, this, &ThermogramWidget::UpdateFit);

    baselayout->addWidget(new QLabel(tr("Baseline corretion"), 0, 0));
    baselayout->addWidget(m_baseline_type, 0, 1);
    baselayout->addWidget(new QLabel(tr("Baseline fit type:")), 0, 2);
    baselayout->addWidget(m_fit_type, 0, 3);

    m_limits = new QCheckBox(tr("Show Fit limit"));
    baselayout->addWidget(m_limits, 0, 4);
    connect(m_limits, &QCheckBox::stateChanged, this, [this](int state) {
        m_upper->setVisible(state);
        m_lower->setVisible(state);
        UpdateLimits();
    });

    m_smooth = new QCheckBox(tr("Smooth"));
    connect(m_smooth, &QCheckBox::stateChanged, this, &ThermogramWidget::UpdatePlot);
    connect(m_smooth, &QCheckBox::stateChanged, this, &ThermogramWidget::Update);

    m_degree = new QSpinBox;
    m_degree->setMinimum(2);
    m_degree->setMaximum(120);
    m_degree->setValue(3);
    connect(m_degree, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThermogramWidget::FitBaseLine);

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
    baselayout->addWidget(new QLabel(tr("Polynomial degree")), 1, 2);
    baselayout->addWidget(m_degree, 1, 3);
    baselayout->addWidget(m_smooth, 1, 4);
    baselayout->addWidget(m_filter, 1, 5);

    m_stdev = new QLineEdit;
    connect(m_stdev, &QLineEdit::textChanged, this, &ThermogramWidget::UpdateLimits);

    m_mult = new QLineEdit("1");
    connect(m_mult, &QLineEdit::textChanged, this, &ThermogramWidget::UpdateLimits);

    baselayout->addWidget(new QLabel(tr("Stddev")), 2, 0);
    baselayout->addWidget(m_stdev, 2, 1);
    baselayout->addWidget(new QLabel(tr("Multiplier")), 2, 2);
    baselayout->addWidget(m_mult, 2, 3);

    m_fit_button = new QPushButton(tr("Refit Baseline"));
    baselayout->addWidget(m_fit_button, 2, 4);
    connect(m_fit_button, &QPushButton::clicked, this, &ThermogramWidget::FitBaseLine);

    baseline->setLayout(baselayout);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(chart);
    layout->addWidget(baseline);

    setLayout(layout);
}

void ThermogramWidget::setThermogram(PeakPick::spectrum* spec, qreal offset)
{
    m_spec = *spec;
    if (offset == 0)
        m_offset = m_spec.Mean();
    else
        m_offset = offset;

    m_spectrum = true;
    m_baseline = Vector(1);
    m_baseline(0) = m_offset;

    UpdatePlot();
    m_stdev->setText(QString::number(spec->StdDev()));
    m_constant->setText(QString::number(spec->Mean()));
}

void ThermogramWidget::UpdateTable()
{
    m_table->clear();
    m_table->setRowCount(m_peak_list.size());
    m_table->setColumnCount(3);
    for (int j = 0; j < m_peak_list.size(); ++j) {
        QTableWidgetItem* newItem;

        newItem = new QTableWidgetItem(QString::number(m_peak_list[j].integ_num * m_scale));
        m_table->setItem(j, 0, newItem);

        newItem = new QTableWidgetItem(QString::number(m_spec.X(m_peak_list[j].start)));
        QSpinBox* box = new QSpinBox(m_table);
        box->setMaximum(1e9);
        box->setMinimum(-1e9);
        box->setValue(m_spec.X(m_peak_list[j].start));
        connect(box, &QSpinBox::editingFinished, this, [this, j, box]() {
            PeakChanged(j, 1, box->value() / 2); //FIXME hack
        });
        m_table->setCellWidget(j, 1, box);
        m_table->setItem(j, 1, newItem);

        newItem = new QTableWidgetItem(QString::number(m_spec.X(m_peak_list[j].end)));
        box = new QSpinBox(m_table);
        box->setMaximum(1e9);
        box->setMinimum(-1e9);
        box->setValue(m_spec.X(m_peak_list[j].end));
        connect(box, &QSpinBox::editingFinished, this, [this, j, box]() {
            PeakChanged(j, 2, box->value() / 2); //FIXME hack
        });
        m_table->setCellWidget(j, 2, box);
        m_table->setItem(j, 2, newItem);
    }
}

void ThermogramWidget::UpdatePlot()
{
    fromSpectrum(&m_spec, m_thermogram_series);
    m_thermogram->addSeries(m_thermogram_series);
    UpdateBase();
}

void ThermogramWidget::UpdateBase()
{
    fromPolynomial(m_baseline, m_baseline_series);
    m_thermogram->addSeries(m_baseline_series);
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

    std::vector<PeakPick::Peak> peaks = PeakPick::PickPeaks(&sign, 0, qPow(2, 1));
    for (int i = 0; i < peaks.size(); ++i) {
        int pos = PeakPick::FindMinimum(&m_spec, peaks[i]);
        peaks[i].max = pos;
        PeakPick::IntegrateNumerical(&m_spec, peaks[i]);
    }

    std::vector<PeakPick::Peak> max_peak = PeakPick::PickPeaks(&m_spec, 0, qPow(2, 1));

    for (int i = 0; i < max_peak.size(); ++i) {
        int pos = PeakPick::FindMaximum(&m_spec, max_peak[i]);
        max_peak[i].max = pos;
        PeakPick::IntegrateNumerical(&m_spec, max_peak[i]);
    }
    peaks.insert(peaks.end(), max_peak.begin(), max_peak.end());

    m_table->clear();
    m_table->setRowCount(peaks.size());
    m_table->setColumnCount(2);
    for (int j = 0; j < peaks.size(); ++j) {
        int pos = peaks[j].max;
        QTableWidgetItem* newItem;
        newItem = new QTableWidgetItem(QString::number(m_spec.X(pos)));
        m_table->setItem(j, 0, newItem);
        newItem = new QTableWidgetItem(QString::number(peaks[j].integ_num));
        m_table->setItem(j, 1, newItem);
    }
    m_peak_list = peaks;
    UpdateTable();
}

void ThermogramWidget::fromSpectrum(const PeakPick::spectrum* original, LineSeries* series)
{
    PeakPick::spectrum* spectrum = new PeakPick::spectrum(original);
    if (m_smooth->isChecked())
        SmoothFunction(spectrum, m_filter->value());

    series->clear();
    for (int i = 0; i < spectrum->size(); i++)
        series->append(QPointF(spectrum->X(i), spectrum->Y(i) * m_scale));
}

void ThermogramWidget::fromPolynomial(const Vector& coeff, LineSeries* series)
{
    std::cout << coeff << std::endl;
    series->clear();
    for (int i = 0; i < m_spec.size(); i++)
        series->append(QPointF(m_spec.X(i), PeakPick::Polynomial(m_spec.X(i), coeff) * m_scale));
}

void ThermogramWidget::clear()
{
    m_peak_list.clear();
    m_spectrum = false;
    m_table->clear();
    m_thermogram->ClearChart();
}

void ThermogramWidget::Update()
{
    PeakPick::spectrum* spectrum = new PeakPick::spectrum(m_spec);
    if (m_smooth->isChecked())
        SmoothFunction(spectrum, m_filter->value());
    Integrate(&m_peak_list, spectrum);
    delete spectrum;
    UpdateTable();
}

void ThermogramWidget::Integrate(std::vector<PeakPick::Peak>* peaks, const PeakPick::spectrum* original)
{
    m_peaks.clear();
    for (int i = 0; i < peaks->size(); ++i) {
        int min = PeakPick::FindMinimum(original, (*peaks)[i]);
        int max = PeakPick::FindMaximum(original, (*peaks)[i]);
        (*peaks)[i].max = ((*peaks)[i].end + (*peaks)[i].start) / 2.0;
        PeakPick::IntegrateNumerical(original, (*peaks)[i], m_baseline);
        m_peaks << (*peaks)[i].integ_num;
    }
}

void ThermogramWidget::UpdateBaseLine(const QString& str)
{
    if (str == m_base)
        return;

    Vector coeff;

    if (str == "none") {
        coeff = Vector(1);
        coeff(0) = 0;
        m_limits->setChecked(false);

    } else if (str == "offset") {
        coeff = Vector(1);
        coeff(0) = m_offset;
    } else if (str == "constant") {
        coeff = Vector(1);
        coeff(0) = m_constant->text().toDouble();
    } else if (str == "polynomial") {
        coeff = Vector(m_degree->value());
        for (int i = 0; i < coeff.size(); ++i)
            coeff(i) = 1;
    }
    m_baseline = coeff;
    UpdatePlot();
    emit IntegrationChanged();

    m_base = str;
}

void ThermogramWidget::UpdateFit(const QString& str)
{
    if (str == m_fit)
        return;

    FitBaseLine();
    m_fit = str;
}

void ThermogramWidget::FitBaseLine()
{
    if (m_fit == "ignore") {
        m_baseline = Vector(1);
        if (m_baseline_type->currentText() == "constant")
            m_baseline(0) = m_constant->text().toDouble();
        else
            m_baseline(0) = m_offset;
        UpdateBase();
        return;
    }

    PeakPick::spectrum* spectrum = new PeakPick::spectrum(m_spec);
    if (m_smooth->isChecked())
        SmoothFunction(spectrum, m_filter->value());

    PeakPick::BaseLine baseline(spectrum);
    baseline.setDegree(m_degree->value());

    qreal constant;

    if (m_baseline_type->currentText() == "constant")
        constant = m_constant->text().toDouble();
    else
        constant = m_offset;

    qreal stdev = m_stdev->text().toDouble();
    qreal mult = m_mult->text().toDouble();

    if (m_fit == "cutoff") {
        baseline.setLower(constant - (mult * stdev));
        baseline.setUpper(constant + (mult * stdev));
    } else if (m_fit == "peaks") {
        baseline.setPeaks(&m_peak_list);
    }
    m_baseline = baseline.Fit();
    UpdateBase();
    delete spectrum;

    emit IntegrationChanged();
}

void ThermogramWidget::PeakDoubleClicked(const QModelIndex& index)
{
    int peak = index.row();
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
