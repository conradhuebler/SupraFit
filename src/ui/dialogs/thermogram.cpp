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

#include <QtCore/QSettings>

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
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableWidget>

#include <QtCharts/QChart>

#include "libpeakpick/peakpick.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/instance.h"
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
    connect(m_dilution, &ThermogramWidget::CalibrationChanged, this, [this](double val) {
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

    m_UseParameter = new QCheckBox(tr("Use Parameter"));
    connect(m_UseParameter, &QCheckBox::stateChanged, this, [this]() {
        this->m_ParameterUsed = this->m_UseParameter->isChecked();
    });

    m_CellVolume = new QLineEdit;
    //m_CellVolume->setMaximumWidth(100);

    connect(m_CellVolume, &QLineEdit::textEdited, m_CellVolume, [this](const QString& str) {
        this->m_systemparameter[QString::number(AbstractItcModel::CellVolume)] = str;
        this->m_UseParameter->setChecked(true);
    });

    m_CellConcentration = new QLineEdit;
    //m_CellConcentration->setMaximumWidth(100);

    connect(m_CellConcentration, &QLineEdit::textEdited, m_CellConcentration, [this](const QString& str) {
        this->m_systemparameter[QString::number(AbstractItcModel::CellConcentration)] = str;
        this->m_UseParameter->setChecked(true);
    });

    m_SyringeConcentration = new QLineEdit;
    //m_SyringeConcentration->setMaximumWidth(100);

    connect(m_SyringeConcentration, &QLineEdit::textEdited, m_SyringeConcentration, [this](const QString& str) {
        this->m_systemparameter[QString::number(AbstractItcModel::SyringeConcentration)] = str;
        this->m_UseParameter->setChecked(true);
    });

    m_Temperature = new QLineEdit;
    //m_Temperature->setMaximumWidth(100);

    connect(m_Temperature, &QLineEdit::textEdited, m_Temperature, [this](const QString& str) {
        this->m_systemparameter[QString::number(AbstractItcModel::Temperature)] = str;
        this->m_UseParameter->setChecked(true);
    });

    m_constantVolume = new QCheckBox(tr("Constant Volume"));
    m_constantVolume->setChecked(true);
    connect(m_constantVolume, &QCheckBox::stateChanged, m_constantVolume, [this]() {
        this->m_systemparameter[QString::number(AbstractItcModel::Reservoir)] = this->m_constantVolume->isChecked();
    });

    m_dil_file = new QLineEdit;
    m_dil_file->setClearButtonEnabled(true);
    connect(m_dil_file, &QLineEdit::textChanged, this, &Thermogram::clearDilution);

    m_scale = new QComboBox;
    m_scale->addItem(QString::number(cal2joule));
    m_scale->addItem("1");
    m_scale->setEditable(true);
    connect(m_scale, &QComboBox::currentTextChanged, m_scale, [this]() {
        this->UpdateTable();
    });
    m_injct = new QLineEdit;
    //m_injct->setMaximumWidth(100);

    m_exp_base = new QLineEdit;
    //m_exp_base->setMaximumWidth(100);

    m_dil_base = new QLineEdit;
    //m_dil_base->setMaximumWidth(100);

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

    QHBoxLayout* hlayout = new QHBoxLayout;

    hlayout->addWidget(m_exp_button);
    hlayout->addWidget(m_exp_file);
    hlayout->addWidget(m_UseParameter);
    hlayout->addWidget(m_dil_button);
    hlayout->addWidget(m_dil_file);
    layout->addLayout(hlayout, 0, 0, 1, 4);

    hlayout = new QHBoxLayout;
    hlayout->addWidget(new QLabel(tr("Cell Volume")));
    hlayout->addWidget(m_CellVolume);

    hlayout->addWidget(new QLabel(tr("Cell Concentration")));
    hlayout->addWidget(m_CellConcentration);

    hlayout->addWidget(new QLabel(tr("Syringe Concentration")));
    hlayout->addWidget(m_SyringeConcentration);

    hlayout->addWidget(new QLabel(tr("Temperatur")));
    hlayout->addWidget(m_Temperature);
    hlayout->addWidget(m_constantVolume);

    layout->addLayout(hlayout, 1, 0, 1, 4);

    layout->addWidget(new QLabel(tr("Inject Volume")), 2, 0);
    layout->addWidget(m_injct, 2, 1);
    layout->addWidget(m_message, 2, 2, 1, 2);

    hlayout = new QHBoxLayout;
    hlayout->addWidget(new QLabel(tr("Freq:")));
    hlayout->addWidget(m_freq);
    hlayout->addWidget(new QLabel(tr("cal->J")));
    hlayout->addWidget(m_scale);
    hlayout->addWidget(new QLabel(tr("Experiment Heat:")));
    hlayout->addWidget(m_exp_base);
    hlayout->addWidget(new QLabel(tr("Dilution Heat:")));
    hlayout->addWidget(m_dil_base);
    hlayout->addWidget(m_refit);

    layout->addLayout(hlayout, 3, 0, 1, 4);

    m_mainwidget = new QTabWidget;

    m_table = new QTableWidget;
    //m_table->setFixedWidth(250);

    m_thm_series = new ScatterSeries;
    m_thm_series->setName(tr("Thermogram"));
    m_raw_series = new ScatterSeries;
    m_raw_series->setName(tr("ITC Data"));
    m_dil_series = new ScatterSeries;
    m_dil_series->setName(tr("Dilution Data"));

    m_data_view = new ChartView;
    m_data_view->setModal(true);
    m_data_view->setMinimumSize(400, 300);
    connect(m_data_view, &ChartView::LastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });

    m_data_view->setAutoScaleStrategy(AutoScaleStrategy::QtNiceNumbers);

    m_export_data = new QPushButton(tr("Export Table"));
    connect(m_export_data, &QPushButton::clicked, this, &Thermogram::ExportData);

    m_splitter = new QSplitter(Qt::Horizontal);
    QWidget* table_holder = new QWidget;
    QVBoxLayout* vlayout = new QVBoxLayout;

    vlayout->addWidget(m_export_data);
    vlayout->addWidget(m_table);
    table_holder->setLayout(vlayout);

    m_splitter->addWidget(m_data_view);
    m_splitter->addWidget(table_holder);

    hlayout = new QHBoxLayout;
    hlayout->addWidget(m_splitter);

    QWidget* widget = new QWidget;
    widget->setLayout(hlayout);
    m_mainwidget->addTab(widget, tr("Data Table"));

    m_mainwidget->addTab(m_experiment, tr("Experiment"));
    m_mainwidget->addTab(m_dilution, tr("Dilution"));

    layout->addWidget(m_mainwidget, 4, 0, 1, 4);

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(m_buttonbox, 5, 0, 1, 4);

    connect(m_experiment, &ThermogramWidget::IntegrationChanged, this, &Thermogram::UpdateExpTable);
    connect(m_dilution, &ThermogramWidget::IntegrationChanged, this, &Thermogram::UpdateDilTable);

    setLayout(layout);

    QSettings settings;
    settings.beginGroup("thermogram_dialog");
    m_splitter->restoreState(settings.value("splitterSizes").toByteArray());

    connect(m_experiment, &ThermogramWidget::CalibrationChanged, this, [this](double val) {
        if (val == 0)
            m_scale->setCurrentText(QString::number(cal2joule));
        else
            m_scale->setCurrentText("1");
        this->m_exp_peaks = this->m_experiment->Peaks();
        this->UpdateTable();
    });
}

Thermogram::~Thermogram()
{
    QSettings settings;
    settings.beginGroup("thermogram_dialog");
    settings.setValue("splitterSizes", m_splitter->saveState());
}

PeakPick::spectrum Thermogram::LoadITCFile(QString& filename, std::vector<PeakPick::Peak>* peaks, qreal& offset)
{
    peaks->clear();
    m_forceInject = true;
    m_injection = true;
    qreal freq = 0;
    QPair<PeakPick::spectrum, QJsonObject> pair = ToolSet::LoadITCFile(filename, peaks, offset, freq, m_inject);
    PeakPick::spectrum original = pair.first;
    m_systemparameter = pair.second;

    m_Temperature->setText(m_systemparameter[QString::number(AbstractItcModel::Temperature)].toString());
    m_CellConcentration->setText(m_systemparameter[QString::number(AbstractItcModel::CellConcentration)].toString());
    m_SyringeConcentration->setText(m_systemparameter[QString::number(AbstractItcModel::SyringeConcentration)].toString());
    m_CellVolume->setText(m_systemparameter[QString::number(AbstractItcModel::CellVolume)].toString());

    m_UseParameter->setChecked(m_systemparameter.size() != 0);
    QSignalBlocker block(m_freq);
    m_freq->setValue(freq);
    return original;
}

void Thermogram::setScaling(const QString& str)
{
    m_scale->setCurrentText(str);
}

PeakPick::spectrum Thermogram::LoadXYFile(const QString& filename)
{
    QPair<Vector, Vector> experiment = ToolSet::LoadXYFile(filename);
    if (!(experiment.first.size() && experiment.second.size())) {
        qDebug() << "size dont fit";
        return PeakPick::spectrum();
    }
    return PeakPick::spectrum(experiment.first, experiment.second);
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
        m_experiment->setFileType(ThermogramWidget::FileType::ITC);
        m_experiment->setThermogram(&original, offset);
        m_experiment->setPeakList(m_exp_peaks);
        m_exp_peaks = m_experiment->Peaks();
    } else {
        original = LoadXYFile(filename);
        QSignalBlocker block(m_freq);
        m_freq->setValue(original.Step());
        m_experiment->setFileType(ThermogramWidget::FileType::RAW);
        m_experiment->setThermogram(&original);
    }
    m_exp_therm = original;

    m_exp_file->setText(filename);
    emit m_experiment->IntegrationChanged();
    //UpdateData();
    m_mainwidget->setCurrentIndex(1);
}

void Thermogram::setExperiment()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir(), tr("Supported files (*.txt *.dat *.itc *.ITC);;All files (*.*)"));
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
    m_all_rows.clear();
    m_thm_series->clear();
    m_raw_series->clear();
    m_dil_series->clear();

    m_raw.clear();
    m_heat.clear();
    m_dil_heat.clear();

    m_table->clear();
    m_table->setRowCount(m_exp_peaks.size());
    m_table->setColumnCount(4);
    QChar mu = QChar(956);
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
        m_all_rows += newItem->data(Qt::DisplayRole).toString() + "\t";
        m_table->setItem(j, 0, newItem);

        m_raw << m_exp_peaks[j].integ_num;
        newItem = new QTableWidgetItem(QString::number(m_raw.last()));
        m_all_rows += newItem->data(Qt::DisplayRole).toString() + "\t";
        newItem->setBackgroundColor(m_raw_series->color().lighter());
        m_raw_series->append(QPointF(j, m_raw.last()));

        m_table->setItem(j, 1, newItem);

        qreal dil = 0;
        if (j < m_dil_peaks.size()) {
            m_dil_heat << m_dil_peaks[j].integ_num;
            dil = m_dil_heat.last();
        }
        newItem = new QTableWidgetItem(QString::number(dil));
        m_all_rows += newItem->data(Qt::DisplayRole).toString() + "\t";

        if (m_dil_peaks.size())
            newItem->setBackground(m_dil_series->color().lighter());
        m_table->setItem(j, 2, newItem);
        m_dil_series->append(QPointF(j, dil));

        newItem = new QTableWidgetItem(QString::number(PeakAt(j)));
        m_all_rows += newItem->data(Qt::DisplayRole).toString() + "\n";
        m_content += newItem->data(Qt::DisplayRole).toString() + "\n";
        newItem->setBackgroundColor(m_thm_series->color().lighter());
        m_table->setItem(j, 3, newItem);

        m_thm_series->append(QPointF(j, PeakAt(j)));
    }

    QStringList header = QStringList() << QString("Volume\n[%1L]").arg(mu) << " exp. heat \n[raw]"
                                       << "dil. heat \n[raw]"
                                       << "joined heat \n[J]";
    m_table->setHorizontalHeaderLabels(header);
    m_table->resizeColumnsToContents();

    m_data_view->addSeries(m_thm_series);
    m_data_view->addSeries(m_raw_series);

    if (m_dil_peaks.size())
        m_data_view->addSeries(m_dil_series);

    m_data_view->setXAxis("Inject Number");
    m_data_view->setYAxis("Heat q");
}

QString Thermogram::Content() const
{
    QString content("");

    for (int i = 0; i < m_table->rowCount(); ++i) {
        content += m_table->item(i, 0)->data(Qt::DisplayRole).toString() + "\t";
        content += m_table->item(i, 3)->data(Qt::DisplayRole).toString() + "\t";
        content += "\n";
    }

    return content;
}

void Thermogram::setDilution()
{
    m_dil_heat.clear();
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir(), tr("Supported files (*.txt *.dat *.itc *.ITC);;All files (*.*)"));
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
        m_dilution->setFileType(ThermogramWidget::FileType::ITC);
        m_dilution->setThermogram(&original, offset);
        m_dilution->setPeakList(m_exp_peaks);
        m_dil_base->setText(QString::number(offset));
        m_dil_peaks = m_dilution->Peaks();
    } else {
        original = LoadXYFile(filename);
        m_dilution->setFileType(ThermogramWidget::FileType::RAW);
        m_dilution->setThermogram(&original);
        //        m_dilution->PickPeaks();
    }
    m_dil_therm = original;
    m_dil_file->setText(filename);

    emit m_dilution->IntegrationChanged();
    //UpdateData();
    m_mainwidget->setCurrentIndex(2);
}


void Thermogram::clearExperiment()
{
    if (m_exp_file->text().isEmpty()) {
        m_experiment->clear();
        m_exp_peaks.clear();
        UpdateData();
        m_exp_file->setStyleSheet("background-color: white");
    } else {
        setExperimentFile(m_exp_file->text());
    }
}

void Thermogram::clearDilution()
{
    if (m_dil_file->text().isEmpty()) {
        m_dilution->clear();
        m_dil_peaks.clear();
        UpdateData();
        m_dil_file->setStyleSheet("background-color: white");
    } else {
        setDilutionFile(m_dil_file->text());
    }
}

void Thermogram::UpdateExpTable()
{
    m_experiment->Update();
    UpdateTable();
}

void Thermogram::UpdateDilTable()
{
    m_dilution->Update();
    UpdateTable();
}
void Thermogram::UpdateData()
{
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
    raw["scaling"] = m_scale->currentText();
    raw["injectvolume"] = m_injct->text();
    return raw;
}

void Thermogram::setRaw(const QJsonObject& object)
{
    m_injct->setText(object["injectvolume"].toString());
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

void Thermogram::ExportData()
{
    QString filename = QFileDialog::getSaveFileName(this, "Select file", getDir());
    if (filename.isEmpty())
        return;

    setLastDir(filename);

    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite))
        return;

    QChar mu = QChar(956);

    QTextStream stream(&file);
    stream << QString("#Volume") + "\t" + " exp. heat " + "\t" + "dil. heat" + "\t" + "joined heat" + "\n";
    stream << QString("#[%1L]").arg(mu) + "\t" + "[raw]" + "\t" + "[raw]" + "\t" + "[J]" + "\n";

    stream << m_all_rows;
}
