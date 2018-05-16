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
#include "src/ui/widgets/thermogramwidget.h"

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
    m_experiment = new ThermogramWidget(this);
    m_dilution = new ThermogramWidget(this);

    QGridLayout* layout = new QGridLayout;

    m_exp_button = new QPushButton(tr("Load Experiment"));
    connect(m_exp_button, &QPushButton::clicked, this, &Thermogram::setExperiment);
    m_dil_button = new QPushButton(tr("Load Dilution"));
    connect(m_dil_button, &QPushButton::clicked, this, &Thermogram::setDilution);

    m_refit = new QPushButton(tr("Update"));
    connect(m_refit, &QPushButton::clicked, this, &Thermogram::UpdateData);

    m_exp_file = new QLineEdit;
    m_exp_file->setClearButtonEnabled(true);
    connect(m_exp_file, &QLineEdit::textChanged, this, &Thermogram::clearExperiment);

    m_dil_file = new QLineEdit;
    m_dil_file->setClearButtonEnabled(true);
    connect(m_dil_file, &QLineEdit::textChanged, this, &Thermogram::clearDilution);

    m_scale = new QLineEdit(tr("4.184"));
    connect(m_scale, &QLineEdit::textChanged, m_scale, [this]() {
        bool ok;
        qreal scale = m_scale->text().toDouble(&ok);
        if (ok) {
            m_experiment->setScale(scale);
            m_dilution->setScale(scale);
        }
    });
    m_injct = new QLineEdit;

    m_exp_base = new QLineEdit;
    m_dil_base = new QLineEdit;

    m_remove_offset = new QCheckBox(tr("Remove Offset"));
    connect(m_remove_offset, &QCheckBox::stateChanged, this, &Thermogram::UpdateData);

    m_message = new QLabel("Inject Volume will be taken from ITC file (if available)!");
    m_offset = new QLabel(tr("No offset"));
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

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(m_remove_offset);
    hlayout->addWidget(m_offset);
    hlayout->addWidget(new QLabel(tr("cal->J")));
    hlayout->addWidget(m_scale);
    hlayout->addWidget(new QLabel(tr("Experiment Heat:")));
    hlayout->addWidget(m_exp_base);
    hlayout->addWidget(new QLabel(tr("Dilution Heat:")));
    hlayout->addWidget(m_dil_base);
    hlayout->addWidget(m_refit);

    layout->addLayout(hlayout, 2, 0, 1, 4);

    m_mainwidget = new QTabWidget;

    m_table = new QTableWidget;
    m_table->setFixedWidth(250);
    m_data_series = new ScatterSeries;
    m_data = new QtCharts::QChart();
    m_data_view = new ChartView(m_data);
    m_data_view->setModal(true);
    m_data_view->setMinimumSize(400, 300);

    QWidget* widget = new QWidget;
    hlayout = new QHBoxLayout;
    widget->setLayout(hlayout);
    hlayout->addWidget(m_data_view);
    hlayout->addWidget(m_table);
    m_mainwidget->addTab(widget, tr("Data Table"));

    m_mainwidget->addTab(m_experiment, tr("Experiment"));
    m_mainwidget->addTab(m_dilution, tr("Dilution"));

    layout->addWidget(m_mainwidget, 4, 0, 1, 4);

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(m_buttonbox, 5, 0, 1, 4);
    m_buttonbox->setDisabled(m_injct->text().isEmpty());

    connect(m_experiment, &ThermogramWidget::IntegrationChanged, this, &Thermogram::UpdateData);
    //connect(m_dilution, &ThermogramWidget::IntegrationChanged, this, &Thermogram::UpdateData);

    setLayout(layout);
}

PeakPick::spectrum Thermogram::LoadITCFile(const QString& filename, std::vector<PeakPick::Peak>* peaks, qreal& offset)
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
    qreal last_x = 0, freq = 2.0;
    offset = 0;
    PeakPick::Peak floating_peak;
    int i_offset = 0;
    for (const QString& str : filecontent) {
        if (str.contains("$") || str.contains("#") || str.contains("?") || str.contains("%")) {
            if (str.contains("%")) {
            }
        } else if (str.contains("@")) {
            skip = (str.contains("@0"));
            if (skip)
                continue;
            start_peak = true;
            floating_peak.end = last_x;
            if (last_x && floating_peak.start)
                peaks->push_back(floating_peak);
            m_inject << str.split(",")[1].toDouble();
        } else {
            QStringList elements = str.simplified().split(",");
            if (elements.size() > 2) {
                if (skip) {
                    offset += elements[1].toDouble();
                    i_offset++;
                    last_x = elements[0].toDouble() / freq;
                }
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

    floating_peak.end = last_x;
    peaks->push_back(floating_peak);
    offset /= double(i_offset);
    x = Vector::Map(&entries_x[0], entries_x.size());
    y = Vector::Map(&entries_y[0], entries_y.size());

    PeakPick::spectrum original(y, x[0], x[x.size() - 1]);
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
    m_heat_offset = 0;
    QFileInfo info(filename);
    PeakPick::spectrum original;
    if (info.suffix() == "itc") {
        qreal offset = 0;
        original = LoadITCFile(filename, &m_exp_peaks, offset);
        m_exp_base->setText(QString::number(offset));
        m_experiment->setThermogram(&original, offset);
        m_experiment->setPeakList(m_exp_peaks);
    } else {
        original = LoadXYFile(filename);
        m_experiment->setThermogram(&original);
        m_experiment->PickPeaks();
    }
    m_buttonbox->setEnabled(m_injection);
    m_exp_therm = original;

    m_raw = m_experiment->PeakList();
    m_exp_file->setText(filename);

    m_content = QString();
    UpdateData();
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
    m_data_series->clear();
    m_table->clear();
    m_table->setRowCount(m_exp_peaks.size());
    m_table->setColumnCount(2);
    m_heat.clear();
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
        m_raw << m_exp_peaks[j].integ_num;
        qreal dil = 0;
        if (j < m_dil_heat.size())
            dil = m_dil_heat[j];
        m_heat << m_exp_peaks[j].integ_num - dil;
        newItem = new QTableWidgetItem(QString::number(PeakAt(j)));
        m_table->setItem(j, 1, newItem);
        m_data_series->append(QPointF(j, PeakAt(j)));
    }
    m_data_view->addSeries(m_data_series);
}

void Thermogram::setDilution()
{
    m_dil_heat.clear();
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir());
    if (filename.isEmpty())
        return;

    setLastDir(filename);

    QFileInfo info(filename);
    PeakPick::spectrum original;
    if (info.suffix() == "itc") {
        qreal offset = 0;
        original = LoadITCFile(filename, &m_dil_peaks, offset);
        m_dilution->setThermogram(&original, offset);
        m_dilution->setPeakList(m_exp_peaks);
        m_dil_base->setText(QString::number(offset));
    } else {
        original = LoadXYFile(filename);
        m_dilution->setThermogram(&original);
        m_dilution->PickPeaks();
    }
    m_dil_therm = original;
    m_dil_file->setText(filename);
    m_dil_heat = m_dilution->PeakList();
    UpdateData();
    m_mainwidget->setCurrentIndex(2);
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

        m_content += string + "\t" + QString::number(PeakAt(j)) + "\n";
    }
    return m_content;
}

void Thermogram::clearExperiment()
{
    if (m_exp_file->text().isEmpty()) {
        m_experiment->clear();
        UpdateData();
    }
}

void Thermogram::clearDilution()
{
    if (m_dil_file->text().isEmpty()) {
        m_dilution->clear();
        UpdateData();
    }
}

void Thermogram::UpdateData()
{
    m_experiment->Update();
    m_dilution->Update();
    m_raw = m_experiment->PeakList();
    m_dil_heat = m_dilution->PeakList();
    m_offset->setText(QString::number((m_heat_offset + m_dil_offset) * m_scale->text().toDouble()) + " = Heat: " + QString::number(m_heat_offset * m_scale->text().toDouble()) + "+ Dilution:" + QString::number(m_dil_offset * m_scale->text().toDouble()) + "  ");
    UpdateTable();
}

qreal Thermogram::PeakAt(int i)
{
    qreal dilution = 0;
    if (i < m_dil_heat.size())
        dilution = m_dil_heat[i];
    return (m_raw[i] - dilution - (m_heat_offset + m_dil_offset) * m_remove_offset->isChecked()) * m_scale->text().toDouble();
}
