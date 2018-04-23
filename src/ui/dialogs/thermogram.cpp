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
    setModal(true);
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

    m_message = new QLabel("Inject Volume will be taken from ITC file (if available)!");

    connect(m_injct, &QLineEdit::textChanged, this, &Thermogram::UpdateInject);
    connect(m_injct, &QLineEdit::returnPressed, m_injct, [this]() {
        if (m_forceInject) {
            m_message->setText("Inject Volume will be taken from ITC file (if available)!");
            m_forceInject = false;
        } else {
            m_message->setText("Inject Volume will NOT be taken from ITC file!");
            m_forceInject = true;
        }
        UpdateTable();
    });

    layout->addWidget(m_exp_button, 0, 0);
    layout->addWidget(m_exp_file, 0, 1);
    layout->addWidget(m_dil_button, 0, 2);
    layout->addWidget(m_dil_file, 0, 3);

    layout->addWidget(new QLabel(tr("Inject Volume")), 1, 0);
    layout->addWidget(m_injct, 1, 1);
    layout->addWidget(m_message, 1, 2, 1, 2);

    m_mainwidget = new QTabWidget;

    m_table = new QTableWidget;
    m_table->setFixedWidth(250);
    m_thermogram = new ScatterSeries;
    m_therm = new QtCharts::QChart();
    m_thermogram_view = new ChartView(m_therm);
    m_thermogram_view->setModal(true);
    m_thermogram_view->setMinimumSize(800, 600);

    QWidget* widget = new QWidget;
    QHBoxLayout* hlayout = new QHBoxLayout;
    widget->setLayout(hlayout);
    hlayout->addWidget(m_thermogram_view);
    hlayout->addWidget(m_table);
    m_mainwidget->addTab(widget, tr("Data Table"));

    m_experiment = new QtCharts::QChart();
    m_experiment_view = new ChartView(m_experiment);
    m_experiment_view->setModal(true);
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
    m_dilution_view->setModal(true);
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

    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(m_buttonbox, 3, 0, 1, 4);
    m_buttonbox->setDisabled(m_injct->text().isEmpty());

    setLayout(layout);
}

PeakPick::spectrum Thermogram::LoadITCFile(const QString& filename, std::vector<PeakPick::Peak>* peaks)
{
    peaks->clear();
    m_forceInject = true;
    m_injection = true;
    Vector x, y;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << file.errorString();
    }

    QStringList filecontent = QString(file.readAll()).split("\n");

    std::vector<double> entries_x, entries_y;
    bool start_peak = false, skip = false;
    qreal last_x = 0, offset = 0, freq = 2.0;
    PeakPick::Peak floating_peak;
    int i_offset = 0;
    for (const QString& str : filecontent) {
        if (str.contains("$") || str.contains("#") || str.contains("?") || str.contains("%")) {
            if (str.contains("%")) {
                i_offset++;
                if (i_offset == 3)
                    offset = QString(str).remove("%").remove(" ").toDouble();
            }
        } else if (str.contains("@")) {
            skip = (str.contains("@0"));
            if (skip)
                continue;
            start_peak = true;
            floating_peak.end = last_x;
            if (last_x)
                peaks->push_back(floating_peak);
            m_inject << str.split(",")[1].toDouble();

        } else {
            if (skip)
                continue;
            QStringList elements = str.simplified().split(",");
            if (elements.size() == 7) {
                entries_x.push_back(elements[0].toDouble());
                entries_y.push_back(elements[1].toDouble());
                if (start_peak) {
                    floating_peak.start = elements[0].toDouble() / freq;
                    start_peak = false;
                }
                last_x = elements[0].toDouble() / freq;
            }
        }
    }

    //floating_peak.end = last_x;
    //peaks->push_back(floating_peak);

    x = Vector::Map(&entries_x[0], entries_x.size());
    y = Vector::Map(&entries_y[0], entries_y.size());

    PeakPick::spectrum original(y, x[0], x[x.size() - 1]);

    for (int i = 0; i < peaks->size(); ++i) {
        int min = PeakPick::FindMinimum(&original, (*peaks)[i]);
        int max = PeakPick::FindMaximum(&original, (*peaks)[i]);
        qDebug() << i << (*peaks)[i].start << (*peaks)[i].end << ((*peaks)[i].end + (*peaks)[i].start) / 2.0;
        ;
        (*peaks)[i].max = ((*peaks)[i].end + (*peaks)[i].start) / 2.0;
        PeakPick::IntegrateNumerical(&original, (*peaks)[i], offset);
    }
    return original;
}

PeakPick::spectrum Thermogram::LoadXYFile(const QString& filename)
{
    QPair<Vector, Vector> experiment = ToolSet::LoadXYFile(filename);
    if (!(experiment.first.size() && experiment.second.size())) {
        qDebug() << "size dont fit";
        return PeakPick::spectrum();
    }

    return PeakPick::spectrum(experiment.second, experiment.first[0], experiment.first[experiment.first.size() - 1]);
}

void Thermogram::setExperimentFile(const QString& filename)
{
    QFileInfo info(filename);
    PeakPick::spectrum original;
    if (info.suffix() == "itc") {
        original = LoadITCFile(filename, &m_exp_peaks);
        m_exp_table->clear();
        m_exp_table->setRowCount(m_exp_peaks.size());
        m_exp_table->setColumnCount(2);
        for (int j = 0; j < m_exp_peaks.size(); ++j) {
            int pos = m_exp_peaks[j].max;
            QTableWidgetItem* newItem;
            newItem = new QTableWidgetItem(QString::number(original.X(pos)));
            m_exp_table->setItem(j, 0, newItem);
            newItem = new QTableWidgetItem(QString::number(m_exp_peaks[j].integ_num));
            m_exp_table->setItem(j, 1, newItem);
        }
    } else {
        original = LoadXYFile(filename);
        original.center();
        m_exp_peaks = PickPeaks(original, m_exp_table);
    }
    m_buttonbox->setEnabled(m_injection);

    m_exp_file->setText(filename);

    m_experiment_view->ClearChart();

    m_heat.clear();
    m_raw.clear();
    m_content = QString();
    UpdateTable();
    m_thermogram_view->addSeries(m_thermogram);

    LineSeries* series = fromSpectrum(original);
    m_experiment_view->addSeries(series);
    m_mainwidget->setCurrentIndex(1);
}

void Thermogram::setExperiment()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir());
    if (filename.isEmpty()) {
        qDebug() << "no filename set " << filename;
        return;
    }
    setLastDir(filename);
    setExperimentFile(filename);
}

void Thermogram::UpdateTable()
{
    m_thermogram->clear();
    m_table->clear();
    m_table->setRowCount(m_exp_peaks.size());
    m_table->setColumnCount(2);
    for (int j = 0; j < m_exp_peaks.size(); ++j) {
        QTableWidgetItem* newItem;
        QString string;
        if (j < m_inject.size() || m_forceInject) {
            string = QString::number(m_inject[j]);
            newItem = new QTableWidgetItem(QString::number(m_inject[j]));
        } else {
            string = m_injct->text();
            newItem = new QTableWidgetItem(m_injct->text());
        }
        m_table->setItem(j, 0, newItem);
        m_heat << m_exp_peaks[j].integ_num;
        m_raw << m_exp_peaks[j].integ_num;
        newItem = new QTableWidgetItem(QString::number(m_heat[j]));
        m_table->setItem(j, 1, newItem);
        m_thermogram->append(QPointF(j, m_heat[j]));
        m_content += string + "\t" + QString::number(m_heat[j]) + "\n";
    }
}

void Thermogram::setDilution()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir());
    if (filename.isEmpty())
        return;

    setLastDir(filename);

    QFileInfo info(filename);
    PeakPick::spectrum original;
    if (info.suffix() == "itc") {
        original = LoadITCFile(filename, &m_dil_peaks);
        m_dil_table->clear();
        m_dil_table->setRowCount(m_dil_peaks.size());
        m_dil_table->setColumnCount(2);
        for (int j = 0; j < m_dil_peaks.size(); ++j) {
            int pos = m_dil_peaks[j].max;
            QTableWidgetItem* newItem;
            newItem = new QTableWidgetItem(QString::number(original.X(pos)));
            m_dil_table->setItem(j, 0, newItem);
            newItem = new QTableWidgetItem(QString::number(m_dil_peaks[j].integ_num));
            m_dil_table->setItem(j, 1, newItem);
        }
    } else {
        original = LoadXYFile(filename);
        original.center();
        m_dil_peaks = PickPeaks(original, m_dil_table);
    }

    m_dil_file->setText(filename);

    m_dilution_view->ClearChart();

    m_content = QString();
    m_thermogram->clear();
    if (m_dil_peaks.size() == m_exp_peaks.size()) {
        for (int j = 0; j < m_dil_peaks.size(); ++j) {
            QTableWidgetItem* newItem;
            QString string;
            if (j < m_inject.size() || m_forceInject) {
                string = QString::number(m_inject[j]);
                newItem = new QTableWidgetItem(QString::number(m_inject[j]));
            } else {
                string = m_injct->text();
                newItem = new QTableWidgetItem(m_injct->text());
            }
            m_table->setItem(j, 0, newItem);
            m_heat[j] = m_raw[j] - m_dil_peaks[j].integ_num;
            newItem = new QTableWidgetItem(QString::number(m_heat[j]));
            m_table->setItem(j, 1, newItem);
            m_thermogram->append(QPointF(j, m_heat[j]));
            m_content += string + "\t" + QString::number(m_heat[j]) + "\n";
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
    m_buttonbox->setEnabled(m_injection || !m_injct->text().isEmpty());

    m_content = QString();
    if (!m_table->rowCount())
        return;
    for (int j = 0; j < m_table->rowCount(); ++j) {
        QTableWidgetItem* newItem;
        QString string;
        if (j < m_inject.size() || m_forceInject) {
            string = QString::number(m_inject[j]);
            newItem = new QTableWidgetItem(QString::number(m_inject[j]));
        } else {
            string = m_injct->text();
            newItem = new QTableWidgetItem(m_injct->text());
        }
        m_table->setItem(j, 0, newItem);

        m_content += string + "\t" + QString::number(m_heat[j]) + "\n";
    }
}

QString Thermogram::Content()
{
    m_content = QString();

    for (int j = 0; j < m_table->rowCount(); ++j) {
        QString string;
        if (j < m_inject.size() || m_forceInject)
            string = QString::number(m_inject[j]);
        else
            string = m_injct->text();

        m_content += string + "\t" + QString::number(m_heat[j]) + "\n";
    }
    return m_content;
}
