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

#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>

#include <QtCharts/QChart>

#include "libpeakpick/peakpick.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/widgets/chartview.h"

#include "src/core/toolset.h"
#include "src/global.h"

#include "thermogram.h"

typedef Eigen::VectorXd Vector;

Thermogram::Thermogram()
{
    setUi();
}

void Thermogram::setUi()
{
    QGridLayout* layout = new QGridLayout;

    m_exp_button = new QPushButton(tr("Load Experiment"));
    connect(m_exp_button, &QPushButton::clicked, this, &Thermogram::setExperiment);
    m_dil_button = new QPushButton(tr("Load Dilution"));
    connect(m_dil_button, &QPushButton::clicked, this, &Thermogram::setDilution);

    m_exp_file = new QLineEdit;
    m_dil_file = new QLineEdit;
    m_injct = new QLineEdit;
    connect(m_injct, &QLineEdit::textChanged, this, &Thermogram::UpdateInject);

    layout->addWidget(m_exp_button, 0, 0);
    layout->addWidget(m_exp_file, 0, 1);
    layout->addWidget(m_dil_button, 0, 2);
    layout->addWidget(m_dil_file, 0, 3);

    layout->addWidget(new QLabel(tr("Inject Volume")), 1, 0);
    layout->addWidget(m_injct, 1, 1);

    m_mainwidget = new QTabWidget;

    m_table = new QTableWidget;
    m_table->setFixedWidth(250);
    m_thermogram = new ScatterSeries;
    m_therm = new QtCharts::QChart();
    m_thermogram_view = new ChartView(m_therm);
    m_thermogram_view->setMinimumSize(800, 600);

    QWidget* widget = new QWidget;
    QHBoxLayout* hlayout = new QHBoxLayout;
    widget->setLayout(hlayout);
    hlayout->addWidget(m_thermogram_view);
    hlayout->addWidget(m_table);
    m_mainwidget->addTab(widget, tr("Data Table"));

    m_experiment = new QtCharts::QChart();
    m_experiment_view = new ChartView(m_experiment);
    m_experiment_view->setMinimumSize(800, 600);

    m_exp_table = new QTableWidget;
    m_exp_table->setFixedWidth(250);

    widget = new QWidget;
    hlayout = new QHBoxLayout;

    hlayout->addWidget(m_experiment_view);
    hlayout->addWidget(m_exp_table);
    widget->setLayout(hlayout);

    m_mainwidget->addTab(widget, tr("Experiment"));

    m_dilution = new QtCharts::QChart();
    m_dilution_view = new ChartView(m_dilution);
    m_dilution_view->setMinimumSize(800, 600);

    m_dil_table = new QTableWidget;
    m_dil_table->setFixedWidth(250);

    widget = new QWidget;
    hlayout = new QHBoxLayout;
    hlayout->addWidget(m_dilution_view);
    hlayout->addWidget(m_dil_table);
    widget->setLayout(hlayout);

    m_mainwidget->addTab(widget, tr("Dilution"));

    layout->addWidget(m_mainwidget, 2, 0, 1, 4);

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    ;
    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(m_buttonbox, 3, 0, 1, 4);
    m_buttonbox->setDisabled(m_injct->text().isEmpty());

    setLayout(layout);
}

void Thermogram::setExperiment()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir());
    if (filename.isEmpty()) {
        qDebug() << "no filename set " << filename;
        return;
    }
    setLastDir(filename);
    QPair<Vector, Vector> experiment = ToolSet::LoadXYFile(filename);
    if (!(experiment.first.size() && experiment.second.size())) {
        qDebug() << "size dont fit";
        return;
    }
    m_exp_file->setText(filename);

    PeakPick::spectrum original = PeakPick::spectrum(experiment.second, experiment.first[0], experiment.first[experiment.first.size() - 1]);
    m_experiment_view->ClearChart();
    original.center();
    m_exp_peaks = PickPeaks(original, m_exp_table);

    m_thermogram->clear();
    m_heat.clear();
    m_raw.clear();
    m_content = QString();
    m_table->clear();
    m_table->setRowCount(m_exp_peaks.size());
    m_table->setColumnCount(2);
    for (int j = 0; j < m_exp_peaks.size(); ++j) {
        QTableWidgetItem* newItem;
        newItem = new QTableWidgetItem(m_injct->text());
        m_table->setItem(j, 0, newItem);
        m_heat << m_exp_peaks[j].integ_num;
        m_raw << m_exp_peaks[j].integ_num;
        newItem = new QTableWidgetItem(QString::number(m_heat[j]));
        m_table->setItem(j, 1, newItem);
        m_thermogram->append(QPointF(j, m_heat[j]));
        m_content += m_injct->text() + "\t" + QString::number(m_heat[j]) + "\n";
    }
    m_thermogram_view->addSeries(m_thermogram);

    LineSeries* series = fromSpectrum(original);
    m_experiment_view->addSeries(series);
    m_mainwidget->setCurrentIndex(1);
}

void Thermogram::setDilution()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir());
    if (filename.isEmpty())
        return;

    setLastDir(filename);

    QPair<Vector, Vector> dilution = ToolSet::LoadXYFile(filename);
    if (!(dilution.first.size() && dilution.second.size()))
        return;
    m_dil_file->setText(filename);
    PeakPick::spectrum original = PeakPick::spectrum(dilution.second, dilution.first[0], dilution.first[dilution.first.size() - 1]);
    original.center();
    m_dilution_view->ClearChart();

    m_dil_peaks = PickPeaks(original, m_dil_table);
    m_content = QString();
    m_thermogram->clear();
    if (m_dil_peaks.size() == m_exp_peaks.size()) {
        for (int j = 0; j < m_dil_peaks.size(); ++j) {
            QTableWidgetItem* newItem;
            newItem = new QTableWidgetItem(m_injct->text());
            m_table->setItem(j, 0, newItem);
            m_heat[j] = m_raw[j] - m_dil_peaks[j].integ_num;
            newItem = new QTableWidgetItem(QString::number(m_heat[j]));
            m_table->setItem(j, 1, newItem);
            m_thermogram->append(QPointF(j, m_heat[j]));
            m_content += m_injct->text() + "\t" + QString::number(m_heat[j]) + "\n";
        }
        m_thermogram_view->addSeries(m_thermogram);
    }

    LineSeries* series = fromSpectrum(original);
    m_dilution_view->addSeries(series);
    m_mainwidget->setCurrentIndex(2);
}

LineSeries* Thermogram::fromSpectrum(const PeakPick::spectrum original)
{
    LineSeries* series = new LineSeries;
    for (int i = 0; i < original.size(); i++)
        series->append(QPointF(original.X(i), original.Y(i)));

    return series;
}

std::vector<PeakPick::Peak> Thermogram::PickPeaks(const PeakPick::spectrum spectrum, QTableWidget* widget)
{
    PeakPick::spectrum sign = spectrum;
    sign.InvertSgn();

    std::vector<PeakPick::Peak> peaks = PeakPick::PickPeaks(&sign, 0, qPow(2, 1));
    for (int i = 0; i < peaks.size(); ++i) {
        int pos = PeakPick::FindMinimum(&spectrum, peaks[i]);
        peaks[i].max = pos;
        PeakPick::IntegrateNumerical(&spectrum, peaks[i]);
    }

    std::vector<PeakPick::Peak> max_peak = PeakPick::PickPeaks(&spectrum, 0, qPow(2, 1));

    for (int i = 0; i < max_peak.size(); ++i) {
        int pos = PeakPick::FindMaximum(&spectrum, max_peak[i]);
        max_peak[i].max = pos;
        PeakPick::IntegrateNumerical(&spectrum, max_peak[i]);
    }
    peaks.insert(peaks.end(), max_peak.begin(), max_peak.end());

    widget->clear();
    widget->setRowCount(peaks.size());
    widget->setColumnCount(2);
    for (int j = 0; j < peaks.size(); ++j) {
        int pos = peaks[j].max;
        QTableWidgetItem* newItem;
        newItem = new QTableWidgetItem(QString::number(spectrum.X(pos)));
        widget->setItem(j, 0, newItem);
        newItem = new QTableWidgetItem(QString::number(peaks[j].integ_num));
        widget->setItem(j, 1, newItem);
    }
    return peaks;
}

void Thermogram::UpdateInject()
{
    m_buttonbox->setDisabled(m_injct->text().isEmpty());
    m_content = QString();
    if (!m_table->rowCount())
        return;
    for (int j = 0; j < m_table->rowCount(); ++j) {
        QTableWidgetItem* newItem;
        newItem = new QTableWidgetItem(m_injct->text());
        m_table->setItem(j, 0, newItem);
        m_content += m_injct->text() + "\t" + QString::number(m_heat[j]) + "\n";
    }
}

QString Thermogram::Content()
{
    m_content = QString();

    for (int j = 0; j < m_table->rowCount(); ++j) {
        m_content += m_injct->text() + "\t" + QString::number(m_heat[j]) + "\n";
    }
    return m_content;
}
