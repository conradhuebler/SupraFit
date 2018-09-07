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
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
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
    connect(m_experiment, &ThermogramWidget::IntegrationChanged, this, [this]() {
        this->m_exp_peaks = this->m_experiment->Peaks();
        this->UpdateTable();
    });

    m_dilution = new ThermogramWidget(this);
    connect(m_dilution, &ThermogramWidget::IntegrationChanged, this, [this]() {
        this->m_dil_peaks = this->m_dilution->Peaks();
        this->UpdateTable();
    });

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

    m_scale = new QComboBox;
    m_scale->addItem(QString::number(cal2joule));
    m_scale->addItem("1");
    m_scale->setEditable(true);
    connect(m_scale, &QComboBox::currentTextChanged, m_scale, [this]() {
        bool ok;
        qreal scale = m_scale->currentText().toDouble(&ok);
        if (ok)
            this->UpdateTable();
    });
    m_injct = new QLineEdit;

    m_exp_base = new QLineEdit;
    m_dil_base = new QLineEdit;

    m_freq = new QDoubleSpinBox;
    m_freq->setValue(1.0);
    m_freq->setStyleSheet("background-color: green");
    connect(m_freq, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](qreal freq) {
        m_forceStep = true;
        this->setDilutionFile(m_dil_file->text());
        this->setExperimentFile(m_exp_file->text());
        this->m_dilution->setFrequency(freq);
        this->m_experiment->setFrequency(freq);
    });

    m_message = new QLabel("Inject Volume will be taken from ITC file (if available)!");
    m_offset = new QLabel(tr("No offset"));

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
    hlayout->addWidget(new QLabel(tr("Freq:")));
    hlayout->addWidget(m_freq);
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

    m_thm_series = new ScatterSeries;
    m_thm_series->setName(tr("Thermogram"));
    m_raw_series = new ScatterSeries;
    m_raw_series->setName(tr("ITC Data"));
    m_dil_series = new ScatterSeries;
    m_dil_series->setName(tr("Dilution Data"));

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

    connect(m_experiment, &ThermogramWidget::IntegrationChanged, this, &Thermogram::UpdateData);
    connect(m_dilution, &ThermogramWidget::IntegrationChanged, this, &Thermogram::UpdateData);

    setLayout(layout);
}

PeakPick::spectrum Thermogram::LoadITCFile(QString& filename, std::vector<PeakPick::Peak>* peaks, qreal& offset)
{
    peaks->clear();
    m_forceInject = true;
    m_injection = true;
    qreal freq = 0;
    PeakPick::spectrum original = ToolSet::LoadITCFile(filename, peaks, offset, freq, m_inject);
    QSignalBlocker block(m_freq);
    m_freq->setValue(freq);
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

void Thermogram::setExperimentFile(QString filename)
{
    m_heat_offset = 0;
    QFileInfo info(filename);
    PeakPick::spectrum original;
    if (info.suffix() == "itc") {
        qreal offset = 0;
        try {
            original = LoadITCFile(filename, &m_exp_peaks, offset);
        } catch (int error) {
            if (error == 404) {
                m_exp_file->setStyleSheet("background-color: " + excluded());
                qDebug() << "file no found";
                return;
            }
            if (error == 101) {
                m_exp_file->setStyleSheet("background-color: " + excluded());
                qDebug() << "no thermogram found";
                return;
            }
        }
        m_exp_file->setText(filename);
        m_exp_file->setStyleSheet("background-color: " + included());
        m_exp_base->setText(QString::number(offset));
        m_experiment->setThermogram(&original, offset);
        m_experiment->setPeakList(m_exp_peaks);
        m_exp_peaks = m_experiment->Peaks();
    } else {
        original = LoadXYFile(filename);
        QSignalBlocker block(m_freq);
        m_freq->setValue(original.Step());
        m_experiment->setThermogram(&original);
    }
    m_exp_therm = original;

    m_exp_file->setText(filename);
    emit m_experiment->IntegrationChanged();
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
    m_content.clear();

    m_thm_series->clear();
    m_raw_series->clear();
    m_dil_series->clear();

    m_raw.clear();
    m_heat.clear();
    m_dil_heat.clear();

    m_table->clear();
    m_table->setRowCount(m_exp_peaks.size());
    m_table->setColumnCount(4);

    for (unsigned int j = 0; j < m_exp_peaks.size(); ++j) {
        QTableWidgetItem* newItem;
        if (j < m_inject.size()) {
            if (m_forceInject && !m_injct->text().isEmpty())
                newItem = new QTableWidgetItem(m_injct->text());
            else
                newItem = new QTableWidgetItem(QString::number(m_inject[j]));
        } else {
            newItem = new QTableWidgetItem(m_injct->text());
        }
        m_content += newItem->data(Qt::DisplayRole).toString() + "\t";
        m_table->setItem(j, 0, newItem);

        m_raw << m_exp_peaks[j].integ_num;
        newItem = new QTableWidgetItem(QString::number(m_raw.last()));
        m_raw_series->append(QPointF(j, m_raw.last()));

        m_table->setItem(j, 1, newItem);

        qreal dil = 0;
        if (j < m_dil_peaks.size()) {
            m_dil_heat << m_dil_peaks[j].integ_num;
            dil = m_dil_heat.last();
        }
        newItem = new QTableWidgetItem(QString::number(dil));
        m_table->setItem(j, 2, newItem);
        m_dil_series->append(QPointF(j, dil));

        newItem = new QTableWidgetItem(QString::number(PeakAt(j)));
        m_content += newItem->data(Qt::DisplayRole).toString() + "\n";
        m_table->setItem(j, 3, newItem);

        m_thm_series->append(QPointF(j, PeakAt(j)));
    }
    m_table->resizeColumnsToContents();

    m_data_view->addSeries(m_thm_series);
    m_data_view->addSeries(m_raw_series);
    if (m_dil_peaks.size())
        m_data_view->addSeries(m_dil_series);
}

void Thermogram::setDilution()
{
    m_dil_heat.clear();
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir());
    if (filename.isEmpty())
        return;
    setLastDir(filename);

    setDilutionFile(filename);
}

void Thermogram::setDilutionFile(QString filename)
{
    setLastDir(filename);

    QFileInfo info(filename);
    PeakPick::spectrum original;
    if (info.suffix() == "itc" || info.suffix() == "ITC") {
        qreal offset = 0;
        try {
            original = LoadITCFile(filename, &m_dil_peaks, offset);
        } catch (int error) {
            if (error == 404) {
                m_dil_file->setStyleSheet("background-color: " + excluded());
                qDebug() << "file no found";
                return;
            }
            if (error == 101) {
                m_dil_file->setStyleSheet("background-color: " + excluded());
                qDebug() << "no thermogram found";
                return;
            }
        }
        m_dil_file->setText(filename);
        m_dil_file->setStyleSheet("background-color: " + included());
        m_dilution->setThermogram(&original, offset);
        m_dilution->setPeakList(m_exp_peaks);
        m_dil_base->setText(QString::number(offset));
        m_dil_peaks = m_dilution->Peaks();
    } else {
        original = LoadXYFile(filename);
        m_dilution->setThermogram(&original);
        m_dilution->PickPeaks();
    }
    m_dil_therm = original;
    m_dil_file->setText(filename);

    emit m_dilution->IntegrationChanged();
    UpdateData();
    m_mainwidget->setCurrentIndex(2);
}


void Thermogram::clearExperiment()
{
    if (m_exp_file->text().isEmpty()) {
        m_experiment->clear();
        m_exp_peaks.clear();
        UpdateData();
        m_exp_file->setStyleSheet("background-color: white");
    }
}

void Thermogram::clearDilution()
{
    if (m_dil_file->text().isEmpty()) {
        m_dilution->clear();
        m_dil_peaks.clear();
        UpdateData();
        m_dil_file->setStyleSheet("background-color: white");
    }
}

void Thermogram::UpdateData()
{
    m_experiment->Update();
    m_dilution->Update();
    //m_offset->setText(QString::number((m_heat_offset + m_dil_offset) * m_scale->currentText().toDouble()) + " = Heat: " + QString::number(m_heat_offset * m_scale->currentText().toDouble()) + "+ Dilution:" + QString::number(m_dil_offset * m_scale->currentText().toDouble()) + "  ");
    UpdateTable();
}

qreal Thermogram::PeakAt(int i)
{
    qreal dilution = 0;
    if (i < m_dil_heat.size())
        dilution = m_dil_heat[i];
    return (m_raw[i] - dilution) * m_scale->currentText().toDouble();
}

QJsonObject Thermogram::Raw() const
{
    QJsonObject raw, block;

    block["fit"] = m_experiment->Fit();
    block["file"] = m_exp_file->text();
    raw["experiment"] = block;

    if (!m_dil_file->text().isEmpty()) {
        block["fit"] = m_dilution->Fit();
        block["file"] = m_dil_file->text();
        raw["dilution"] = block;
    }

    return raw;
}

QString Thermogram::ProjectName() const
{
    QFileInfo file(m_exp_file->text());
    return file.baseName();
}

void Thermogram::setExperimentFit(const QJsonObject& json)
{
    m_experiment->setFit(json);
}

void Thermogram::setDilutionFit(const QJsonObject& json)
{
    m_dilution->setFit(json);
}
