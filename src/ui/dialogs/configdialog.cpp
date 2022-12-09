/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/widgets/buttons/scientificbox.h"
#include "src/ui/widgets/optimizerwidget.h"

#include <QtCharts/QChart>

#include <QtCore/QThread>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>


#include "configdialog.h"

OptimizerDialog::OptimizerDialog(QJsonObject config, QWidget* parent)
    : QDialog(parent)
    , m_opt_config(config)
{
    setUi();
}

OptimizerDialog::~OptimizerDialog()
{
}

void OptimizerDialog::setUi()
{
    QVBoxLayout* mainlayout = new QVBoxLayout;

    setLayout(mainlayout);

    m_mainwidget = new QTabWidget;
    mainlayout->addWidget(m_mainwidget);

    createOptimTab();

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainlayout->addWidget(m_buttons);
}

void OptimizerDialog::createOptimTab()
{
    QVBoxLayout* layout = new QVBoxLayout;
    m_opt_widget = new OptimizerWidget(m_opt_config, this);
    m_opt_widget->setLayout(layout);

    m_mainwidget->addTab(m_opt_widget, tr("Optimizer Settings"));
}

ConfigDialog::ConfigDialog(QJsonObject config, QWidget* parent)
    : m_opt_config(config)
    , QDialog(parent)
{
    m_dirlevel = qApp->instance()->property("dirlevel").toInt();
    setUi();
    setWindowTitle("Configure");
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::setUi()
{
    QVBoxLayout* mainlayout = new QVBoxLayout;

    setLayout(mainlayout);

    m_mainwidget = new QTabWidget;
    mainlayout->addWidget(m_mainwidget);

    createGeneralTab();
    createSaveTab();
    createChartTab();
    createStandardCalTab();
    createOptimTab();

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok
        | QDialogButtonBox::Cancel);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainlayout->addWidget(m_buttons);
    setMinimumSize(600, 400);
}

void ConfigDialog::createGeneralTab()
{
    QVBoxLayout* layout = new QVBoxLayout;
    QWidget* generalTab = new QWidget;
    generalTab->setLayout(layout);

    m_threads = new QSpinBox;
    m_threads->setMaximum(2 * QThread::idealThreadCount());
    m_threads->setValue(qApp->instance()->property("threads").toInt());
    m_threads->setMinimum(1);

    QHBoxLayout* h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("Threads:")));
    h_layout->addWidget(m_threads);
    layout->addLayout(h_layout);

    m_model_element_cols = new QSpinBox;
    m_model_element_cols->setMaximum(1e6);
    m_model_element_cols->setValue(qApp->instance()->property("ModelParameterColums").toInt());
    m_model_element_cols->setMinimum(1);

    h_layout = new QHBoxLayout;
    m_ScriptTimout = new QSpinBox;
    m_ScriptTimout->setMinimum(-1);
    m_ScriptTimout->setMaximum(1e6);
    m_ScriptTimout->setValue(qApp->instance()->property("ScriptTimeout").toInt());
    h_layout->addWidget(new QLabel(tr("Timeout for Script Model (mscs):")));
    h_layout->addWidget(m_ScriptTimout);
    layout->addLayout(h_layout);

    h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("Columns for Model Parameter:")));
    h_layout->addWidget(m_model_element_cols);
    layout->addLayout(h_layout);

    layout->addWidget(new QLabel(tr("Set directory behavior to:")));

    m_current_dir = new QRadioButton(tr("Current Directory, where Application was started"), generalTab);
    if (m_dirlevel == 0)
        m_current_dir->setChecked(true);
    layout->addWidget(m_current_dir);

    m_last_dir = new QRadioButton(tr("Last Directory"), generalTab);
    if (m_dirlevel == 1)
        m_last_dir->setChecked(true);
    layout->addWidget(m_last_dir);

    m_working_dir = new QRadioButton(tr("Set a working Directory"), generalTab);
    if (m_dirlevel == 2)
        m_working_dir->setChecked(true);
    layout->addWidget(m_working_dir);

    m_working = new QLineEdit;
    m_working->setClearButtonEnabled(true);
    m_working->setText(qApp->instance()->property("workingdir").toString());
    m_select_working = new QPushButton(this);
    m_select_working->setText(tr("Select"));
    connect(m_select_working, SIGNAL(clicked()), this, SLOT(SelectWorkingDir()));

    h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("Working Directory:")));
    h_layout->addWidget(m_working);
    h_layout->addWidget(m_select_working);
    layout->addLayout(h_layout);

    m_auto_thermo_dialog = new QCheckBox(tr("Automatic open Thermogram Dialog"));
    m_auto_thermo_dialog->setToolTip(tr("Open automatically the thermogram dialog, if the loaded or imported data look like a thermogram."));
    m_auto_thermo_dialog->setChecked(qApp->instance()->property("auto_thermo_dialog").toBool());
    layout->addWidget(m_auto_thermo_dialog);

    m_thermogram_guideline = new QCheckBox(tr("Show guidelines in Thermogram Dialog"));
    m_thermogram_guideline->setToolTip(tr("If checked, some guidelines in the Thermogram will be shown. Errors will shown regardless this option."));
    m_thermogram_guideline->setChecked(qApp->instance()->property("thermogram_guidelines").toBool());
    layout->addWidget(m_thermogram_guideline);

    m_ColorFullSearch = new QCheckBox(tr("Colorfull global search table"));
    m_ColorFullSearch->setToolTip(tr("Indicate fully optimised models in global search with light green backgroud color, not-fully optimised models with light yellow background color and invalid models with a light red background color."));
    m_ColorFullSearch->setChecked(qApp->instance()->property("ColorFullSearch").toBool());
    layout->addWidget(m_ColorFullSearch);

    m_advanced_ui = new QCheckBox(tr("Show advanced options and simulation tools in GUI."));
    m_advanced_ui->setToolTip(tr("If checked, some more advanced options and tools are available via Graphical User Interface. To apply, please restart the SupraFit."));
    m_advanced_ui->setChecked(qApp->instance()->property("advanced_ui").toBool());
    layout->addWidget(m_advanced_ui);

    m_tooltips = new QCheckBox(tr("Show Tooltips as quick help on selected widgets."));
    m_tooltips->setToolTip(tr("If this checkbox were not checked, you would not see this tooltip.\nAnd now for something complete different:\nIf three witches had three watches, which witch would watch which watch?"));
    m_tooltips->setChecked(qApp->instance()->property("tooltips").toBool());
    layout->addWidget(m_tooltips);

    m_initialise_random = new QCheckBox(tr("Initialise Simulation with random numbers."));
    m_initialise_random->setToolTip(tr("If a new model is added to a simulation set, all parameter will be initialised as random numbers."));
    m_initialise_random->setChecked(qApp->instance()->property("InitialiseRandom").toBool());
    layout->addWidget(m_initialise_random);

    m_save_on_exit = new QCheckBox(tr("Automatic save data on exit"));
    m_save_on_exit->setToolTip(tr("Save data automatically on exit. The file will be called 'projectname'.autosave#.json, where # is a natural number. No older files will be overwritten."));
    m_save_on_exit->setChecked(qApp->instance()->property("save_on_exit").toBool());
    //layout->addWidget(m_save_on_exit);

    m_unsafe_copy = new QCheckBox(tr("Unsafe Copy of Models"));
    m_unsafe_copy->setToolTip(tr("If this is enabled, on can copy the model parameter of any model into another via Drag 'n' Drop. If disabled, only Models of the same type are compatible. - Please, don't enable without purpose. SupraFit may behave inappropriately and crash ...."));
    m_unsafe_copy->setChecked(qApp->instance()->property("UnsafeCopy").toBool());
    layout->addWidget(m_unsafe_copy);

    m_ask_on_exit = new QCheckBox(tr("Confirm quit of SupraFit"));
    m_ask_on_exit->setToolTip(tr("Confirm quit of SupraFit"));
    m_ask_on_exit->setChecked(qApp->instance()->property("ask_on_exit").toBool());
    layout->addWidget(m_ask_on_exit);

    m_mainwidget->addTab(generalTab, tr("General Settings"));
}

void ConfigDialog::createSaveTab()
{
    QVBoxLayout* layout = new QVBoxLayout;
    QWidget* generalTab = new QWidget;
    generalTab->setLayout(layout);

    m_StoreRawData = new QCheckBox(tr("Store raw data (thermograms etc) in SupraFit Project files."));
    m_StoreRawData->setChecked(qApp->instance()->property("StoreRawData").toBool());
    m_StoreRawData->setToolTip(tr("Store the original data within the SupraFit project. This may increase the file size and impact the save/write performance negativly or even crash the file for Qt < 5.15 due to the json bug."));
    layout->addWidget(m_StoreRawData);

    m_StoreFileName = new QCheckBox(tr("Store the filename, where the raw data were imported from, in SupraFit Project files."));
    m_StoreFileName->setChecked(qApp->instance()->property("StoreFileName").toBool());
    layout->addWidget(m_StoreFileName);

    layout->addWidget(new QLabel(tr("The above options are checked as default. Additionally, the absolute path\ncan be stored supplementary.This may be an infringement\nof your privacy, if you share the project file.")));

    m_StoreAbsolutePath = new QCheckBox(tr("Store the absolute path of the file containing the raw data."));
    m_StoreAbsolutePath->setChecked(qApp->instance()->property("StoreAbsolutePath").toBool());
    m_StoreAbsolutePath->setToolTip(tr("You can store the absolute path of the file with the raw data. If the project files gets moved around without the original data,\nSupraFit still knows where to look after the data. However, if you share your files, other people will know sth. about the file system structure on the original machine."));
    connect(m_StoreFileName, &QCheckBox::stateChanged, m_StoreAbsolutePath, &QCheckBox::setEnabled);
    m_StoreAbsolutePath->setEnabled(m_StoreFileName->isChecked());
    layout->addWidget(m_StoreAbsolutePath);

    /* We will not provide hash and recursive files right now, but later */

    m_StoreFileHash = new QCheckBox(tr("Store the md5 hash of the file containing the raw data."));
    m_StoreFileHash->setChecked(qApp->instance()->property("StoreFileHash").toBool());
    m_StoreFileHash->setToolTip(tr("Some safety stuff, the ensure the original raw data file."));
    connect(m_StoreFileName, &QCheckBox::stateChanged, m_StoreFileHash, &QCheckBox::setEnabled);
    m_StoreFileHash->setEnabled(m_StoreFileName->isChecked());
    // layout->addWidget(m_StoreFileHash);

    m_FindFileRecursive = new QCheckBox(tr("Look recursivly for the missing raw file in subdirectories."));
    m_FindFileRecursive->setChecked(qApp->instance()->property("FindFileRecursive").toBool());
    m_FindFileRecursive->setToolTip(tr("If SupraFit does not find the file specified via the filename and stored absolute path or the current directory, SupraFit can look for the correct file (name and hash) in subdirectories."));

    // layout->addWidget(m_FindFileRecursive);

    m_mainwidget->addTab(generalTab, tr("File Save Settings"));
}

void ConfigDialog::createStandardCalTab()
{
    QVBoxLayout* layout = new QVBoxLayout;
    QWidget* calcTab = new QWidget;
    calcTab->setLayout(layout);

    m_auto_confidence = new QCheckBox(tr("Automatic Confidence Calculation using Simplified Model Comparison."));
    m_auto_confidence->setChecked(qApp->instance()->property("auto_confidence").toBool());
    layout->addWidget(m_auto_confidence);

    m_series_confidence = new QCheckBox(tr("Include Series in Automatic Confidence Calculation"));
    m_series_confidence->setChecked(qApp->instance()->property("series_confidence").toBool());
    m_series_confidence->setEnabled(qApp->instance()->property("auto_confidence").toBool());
    layout->addWidget(m_series_confidence);

    connect(m_auto_confidence, &QCheckBox::stateChanged, m_series_confidence, &QCheckBox::setEnabled);

    m_p_value = new QDoubleSpinBox;
    m_p_value->setMaximum(100);
    m_p_value->setMinimum(0);
    m_p_value->setSingleStep(1E-2);
    m_p_value->setValue(95);
    m_p_value->setDecimals(2);
    m_p_value->setSuffix("%");
    m_p_value->setValue(qApp->instance()->property("p_value").toDouble());

    QHBoxLayout* h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("Confidence Interval")));
    h_layout->addWidget(m_p_value);
    layout->addLayout(h_layout);

    m_FastConfidenceSteps = new QSpinBox;
    m_FastConfidenceSteps->setMinimum(1);
    m_FastConfidenceSteps->setMaximum(100000);
    m_FastConfidenceSteps->setValue(qApp->instance()->property("FastConfidenceSteps").toInt());

    h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("Maximal Steps")));
    h_layout->addWidget(m_FastConfidenceSteps);

    layout->addLayout(h_layout);

    m_FastConfidenceScaling = new QSpinBox;
    m_FastConfidenceScaling->setMinimum(-10);
    m_FastConfidenceScaling->setMaximum(1);
    m_FastConfidenceScaling->setValue(qApp->instance()->property("FastConfidenceScaling").toInt());

    h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("Single Steps Length")));
    h_layout->addWidget(m_FastConfidenceScaling);

    layout->addLayout(h_layout);

    m_HistBins = new QSpinBox;
    m_HistBins->setMinimum(10);
    m_HistBins->setMaximum(100000);
    m_HistBins->setValue(qApp->instance()->property("EntropyBins").toInt());

    m_overwrite_bins = new QCheckBox(tr("Overwrite stored bin number"));
    m_overwrite_bins->setChecked(qApp->instance()->property("OverwriteBins").toBool());

    m_FullShannon = new QCheckBox(tr("Calculate full Shannon entropy!"));
    m_FullShannon->setChecked(qApp->instance()->property("FullShannon").toBool());
    m_FullShannon->setToolTip(tr("Calculate Shannon entropy including the discretisation term. Not recommended, as the ordering of appropriate parameters and models is reversed."));

    h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("# bins for Shannon Entropy Calculation")));
    h_layout->addWidget(m_HistBins);
    h_layout->addWidget(m_overwrite_bins);

    layout->addLayout(h_layout);
    layout->addWidget(m_FullShannon);

    m_mainwidget->addTab(calcTab, tr("Standard Calculation"));
}

void ConfigDialog::createOptimTab()
{
    QVBoxLayout* layout = new QVBoxLayout;
    m_opt_widget = new OptimizerWidget(m_opt_config, this);
    m_opt_widget->setLayout(layout);

    m_mainwidget->addTab(m_opt_widget, tr("Optimizer Settings"));
}

void ConfigDialog::createChartTab()
{
    QVBoxLayout* layout = new QVBoxLayout;
    QWidget* chartTab = new QWidget;
    chartTab->setLayout(layout);

    m_MaxSeriesPoints = new QSpinBox;
    m_MaxSeriesPoints->setRange(0, 1e25);
    m_MaxSeriesPoints->setValue(qApp->instance()->property("MaxSeriesPoints").toInt());

    layout->addWidget(new QLabel(tr("General Chart Settings:")));
    QHBoxLayout* hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(tr("Maximal number of visualised points per series.")));
    hbox->addWidget(m_MaxSeriesPoints);

    layout->addWidget(new QLabel(tr("Feedback effects in Charts:")));

    m_PointFeedback = new QCheckBox(tr("Highlight #th Point on hovering"));
    m_PointFeedback->setChecked(qApp->instance()->property("PointFeedback").toBool());
    connect(m_PointFeedback, &QCheckBox::stateChanged, this, [this]() {
        m_ModuloPointFeedback->setEnabled(m_PointFeedback->isChecked());
    });
#pragma message("Restore or clean up before release")
    m_ModuloPointFeedback = new QSpinBox;
    m_ModuloPointFeedback->setValue(qApp->instance()->property("ModuloPointFeedback").toInt());
    m_ModuloPointFeedback->setEnabled(m_PointFeedback->isChecked());

    // hbox = new QHBoxLayout;
    // hbox->addWidget(m_PointFeedback);
    // hbox->addWidget(m_ModuloPointFeedback);
    layout->addLayout(hbox);

    m_MarkerPointFeedbackSize = new QDoubleSpinBox;
    m_MarkerPointFeedbackSize->setValue(qApp->instance()->property("MarkerPointFeedback").toDouble());
    m_MarkerPointFeedbackSize->setRange(-100, 100);

    hbox = new QHBoxLayout;
    // hbox->addWidget(new QLabel(tr("Increase size of Points")));
    // hbox->addWidget(m_MarkerPointFeedbackSize);
    layout->addLayout(hbox);

    layout->addWidget(new QLabel(tr("Configure Chart Export Settings:")));

    m_markerSize = new QDoubleSpinBox;
    m_markerSize->setMinimum(0);
    m_markerSize->setMaximum(30);
    m_markerSize->setValue(qApp->instance()->property("markerSize").toDouble());

    hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(tr("Define marker size for exported charts:")));
    hbox->addWidget(m_markerSize);
    layout->addLayout(hbox);

    m_lineWidth = new QDoubleSpinBox;
    m_lineWidth->setMinimum(0);
    m_lineWidth->setMaximum(30);
    m_lineWidth->setValue(qApp->instance()->property("lineWidth").toDouble());

    hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(tr("Define line width for exported charts:")));
    hbox->addWidget(m_lineWidth);
    layout->addLayout(hbox);

    m_XScale = new QSpinBox;
    m_XScale->setMinimum(0);
    m_XScale->setMaximum(1e5);
    m_XScale->setValue(qApp->instance()->property("xSize").toInt());

    m_YScale = new QSpinBox;
    m_YScale->setMinimum(0);
    m_YScale->setMaximum(1e5);
    m_YScale->setValue(qApp->instance()->property("ySize").toInt());

    hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(tr("Size of Chart x-Axis :")));
    hbox->addWidget(m_XScale);
    layout->addLayout(hbox);

    hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(tr("Size of Chart y-Axis :")));
    hbox->addWidget(m_YScale);
    layout->addLayout(hbox);

    m_chartScaling = new QDoubleSpinBox;
    m_chartScaling->setMinimum(0);
    m_chartScaling->setMaximum(30);
    m_chartScaling->setValue(qApp->instance()->property("chartScaling").toDouble());

    hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(tr("Define the chart scaling ration :")));
    hbox->addWidget(m_chartScaling);
    layout->addLayout(hbox);

    m_transparentChart = new QCheckBox;
    m_transparentChart->setText(tr("Transparent Charts"));
    m_transparentChart->setChecked(qApp->instance()->property("transparentChart").toBool());
    layout->addWidget(m_transparentChart);

    m_cropedChart = new QCheckBox;
    m_cropedChart->setText(tr("Remove Transparent Border from Charts"));
    m_cropedChart->setChecked(qApp->instance()->property("cropedChart").toBool());

    m_RemoveGrid = new QCheckBox;
    m_RemoveGrid->setText(tr("Remove Grid in exported Charts"));
    m_RemoveGrid->setChecked(qApp->instance()->property("noGrid").toBool());
    layout->addWidget(m_RemoveGrid);

    m_EmphAxis = new QCheckBox;
    m_EmphAxis->setText(tr("Stronger Axis"));
    m_EmphAxis->setChecked(qApp->instance()->property("empAxis").toBool());
    layout->addWidget(m_EmphAxis);

    layout->addWidget(m_cropedChart);

    connect(m_transparentChart, &QCheckBox::stateChanged, m_transparentChart, [this](bool state) {
        m_cropedChart->setEnabled(state);
    });
    m_cropedChart->setEnabled(m_transparentChart->isChecked());

    m_mainwidget->addTab(chartTab, tr("Chart Settings"));
}

void ConfigDialog::SelectWorkingDir()
{
    QString filename = QFileDialog::getExistingDirectory(this, "Choose Working Directory", qApp->instance()->property("workingdir").toString());
    if (filename.isEmpty())
        return;

    m_working->setText(filename);
}

void ConfigDialog::accept()
{
    if (m_current_dir->isChecked())
        m_dirlevel = 0;
    else if (m_last_dir->isChecked())
        m_dirlevel = 1;
    else if (m_working_dir->isChecked())
        m_dirlevel = 2;

    qApp->instance()->setProperty("threads", m_threads->value());
    qApp->instance()->setProperty("ScriptTimeout", m_ScriptTimout->value());

    qApp->instance()->setProperty("ModelParameterColums", m_model_element_cols->value());

    qApp->instance()->setProperty("workingdir", m_working->text());
    qApp->instance()->setProperty("dirlevel", m_dirlevel);
    qApp->instance()->setProperty("auto_confidence", m_auto_confidence->isChecked());
    qApp->instance()->setProperty("series_confidence", m_series_confidence->isChecked());
    qApp->instance()->setProperty("p_value", m_p_value->value());
    qApp->instance()->setProperty("ask_on_exit", m_ask_on_exit->isChecked());
    qApp->instance()->setProperty("ColorFullSearch", m_ColorFullSearch->isChecked());
    qApp->instance()->setProperty("save_on_exit", m_save_on_exit->isChecked());
    qApp->instance()->setProperty("tooltips", m_tooltips->isChecked());
    qApp->instance()->setProperty("markerSize", m_markerSize->value());
    qApp->instance()->setProperty("lineWidth", m_lineWidth->value());
    qApp->instance()->setProperty("xSize", m_XScale->value());
    qApp->instance()->setProperty("ySize", m_YScale->value());
    qApp->instance()->setProperty("transparentChart", m_transparentChart->isChecked());
    qApp->instance()->setProperty("cropedChart", m_cropedChart->isChecked());
    qApp->instance()->setProperty("noGrid", m_RemoveGrid->isChecked());
    qApp->instance()->setProperty("empAxis", m_EmphAxis->isChecked());
    qApp->instance()->setProperty("chartScaling", m_chartScaling->value());
    qApp->instance()->setProperty("EntropyBins", m_HistBins->value());
    qApp->instance()->setProperty("FastConfidenceScaling", m_FastConfidenceScaling->value());
    qApp->instance()->setProperty("FastConfidenceSteps", m_FastConfidenceSteps->value());
    qApp->instance()->setProperty("auto_thermo_dialog", m_auto_thermo_dialog->isChecked());
    qApp->instance()->setProperty("thermogram_guidelines", m_thermogram_guideline->isChecked());
    qApp->instance()->setProperty("advanced_ui", m_advanced_ui->isChecked());
    qApp->instance()->setProperty("UnsafeCopy", m_unsafe_copy->isChecked());
    qApp->instance()->setProperty("OverwriteBins", m_overwrite_bins->isChecked());
    qApp->instance()->setProperty("FullShannon", m_FullShannon->isChecked());
    qApp->instance()->setProperty("InitialiseRandom", m_initialise_random->isChecked());

    /* Chart Feedback Stuff */
    qApp->instance()->setProperty("PointFeedback", m_PointFeedback->isChecked());
    qApp->instance()->setProperty("ModuloPointFeedback", m_ModuloPointFeedback->value());
    qApp->instance()->setProperty("MarkerPointFeedback", m_MarkerPointFeedbackSize->value());
    qApp->instance()->setProperty("MaxSeriesPoints", m_MaxSeriesPoints->value());

    /* File Save Stuff */
    qApp->instance()->setProperty("StoreRawData", m_StoreRawData->isChecked());
    qApp->instance()->setProperty("StoreFileName", m_StoreFileName->isChecked());
    qApp->instance()->setProperty("StoreAbsolutePath", m_StoreAbsolutePath->isChecked());
    qApp->instance()->setProperty("StoreFileHash", m_StoreFileHash->isChecked());
    qApp->instance()->setProperty("FindFileRecursive", m_FindFileRecursive->isChecked());

    QDialog::accept();
}

#include "configdialog.moc"
