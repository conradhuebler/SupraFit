/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/models/titrations/AbstractItcModel.h" // AbstractItcModel enum (was transitive via models.h)

#include <Eigen/Dense>

#include <QtCore/QCryptographicHash>
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
#include "src/ui/guitools/guitools.h"
#include "src/ui/widgets/thermogramwidget.h"

#include "src/core/itcprocessor.h"
#include "src/core/thermogramhandler.h"
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
    // The ITC state (both thermograms + the experiment-minus-dilution join + injection volumes)
    // is owned by the core, GUI-independent ItcProcessor; the dialog is a view/controller over it.
    // The two handler members are non-owning pointers to the processor's handlers, which the
    // ThermogramWidgets edit interactively. Claude Generated
    m_processor = new ItcProcessor(this);
    m_experiment_thermogram = m_processor->experiment();
    m_dilution_thermogram = m_processor->dilution();

    m_experiment = new ThermogramWidget(m_experiment_thermogram, this);
    m_dilution = new ThermogramWidget(m_dilution_thermogram, this);

    /* One connection instead of one per handler: the processor's constructor already recomputes the
     * net heat whenever either handler re-integrates, so resultChanged() fires once per change and
     * always after the join is consistent. Listening to the handlers directly meant the dialog could
     * render before the processor had re-joined.
     *
     * UpdateData (resolve volumes, then render), not UpdateTable directly: re-integration can change
     * the peak count, so the volume vector has to be brought back to length before it is shown. The
     * resolve emits nothing, so this does not re-enter. Claude Generated */
    connect(m_processor, &ItcProcessor::resultChanged, this, &Thermogram::UpdateData);

    // Either combo drives the shared cal->J factor through the processor; both then show it.
    connect(m_experiment, &ThermogramWidget::ScalingFactorChanged, this, &Thermogram::setScalingFactor);
    connect(m_dilution, &ThermogramWidget::ScalingFactorChanged, this, &Thermogram::setScalingFactor);
    setScalingFactor(cal2joule); // seed the processor and both combos

    QGridLayout* layout = new QGridLayout;

    m_exp_button = new QPushButton(tr("Load Experiment"));
    connect(m_exp_button, &QPushButton::clicked, this, &Thermogram::setExperiment);

    m_dil_button = new QPushButton(tr("Load Dilution"));
    connect(m_dil_button, &QPushButton::clicked, this, &Thermogram::setDilution);

    m_showDilution = new QCheckBox(tr("Show Dilution"));
    m_showDilution->setEnabled(false);

    // m_refit = new QPushButton(tr("Update"));
    // connect(m_refit, &QPushButton::clicked, this, &Thermogram::UpdateData);

    m_exp_file = new QLineEdit;
    m_exp_file->setClearButtonEnabled(true);
    connect(m_exp_file, &QLineEdit::textEdited, this, &Thermogram::clearExperiment);

    m_UseParameter = new QCheckBox(tr("Use Parameter"));
    connect(m_UseParameter, &QCheckBox::stateChanged, this, [this]() {
        this->m_ParameterUsed = this->m_UseParameter->isChecked();
    });

    m_CellVolume = new QLineEdit;
    //m_CellVolume->setMaximumWidth(100);

    connect(m_CellVolume, &QLineEdit::textEdited, m_CellVolume, [this](QString str) {
        this->m_systemparameter[QString::number(AbstractItcModel::CellVolume)] = str.replace(",", ".");
        this->m_UseParameter->setChecked(true);
    });

    m_CellConcentration = new QLineEdit;
    //m_CellConcentration->setMaximumWidth(100);

    connect(m_CellConcentration, &QLineEdit::textEdited, m_CellConcentration, [this](QString str) {
        this->m_systemparameter[QString::number(AbstractItcModel::CellConcentration)] = str.replace(",", ".");
        this->m_UseParameter->setChecked(true);
    });

    m_SyringeConcentration = new QLineEdit;
    //m_SyringeConcentration->setMaximumWidth(100);

    connect(m_SyringeConcentration, &QLineEdit::textEdited, m_SyringeConcentration, [this](QString str) {
        this->m_systemparameter[QString::number(AbstractItcModel::SyringeConcentration)] = str.replace(",", ".");
        this->m_UseParameter->setChecked(true);
    });

    m_Temperature = new QLineEdit;
    //m_Temperature->setMaximumWidth(100);

    connect(m_Temperature, &QLineEdit::textEdited, m_Temperature, [this](QString str) {
        this->m_systemparameter[QString::number(AbstractItcModel::Temperature)] = str.replace(",", ".");
        this->m_UseParameter->setChecked(true);
    });

    m_constantVolume = new QCheckBox(tr("Constant Volume"));
    m_constantVolume->setChecked(true);
    connect(m_constantVolume, &QCheckBox::stateChanged, m_constantVolume, [this]() {
        this->m_systemparameter[QString::number(AbstractItcModel::Reservoir)] = this->m_constantVolume->isChecked();
    });

    m_dil_file = new QLineEdit;
    m_dil_file->setClearButtonEnabled(true);
    connect(m_dil_file, &QLineEdit::textEdited, this, &Thermogram::clearDilution);

    /*
    m_scale = new QComboBox;
    m_scale->addItem(QString::number(cal2joule));
    m_scale->addItem("1");
    m_scale->setEditable(true);
    connect(m_scale, &QComboBox::currentTextChanged, m_scale, [this]() {
        this->UpdateTable();
    });
    */
    m_injct = new QLineEdit;
    //m_injct->setMaximumWidth(100);

    //m_exp_base = new QLineEdit;
    //m_exp_base->setMaximumWidth(100);

    //m_dil_base = new QLineEdit;
    //m_dil_base->setMaximumWidth(100);

    /*
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
    m_freq->setReadOnly(true);
*/
    m_message = new QLabel("Inject Volume will be taken from ITC file (if available)!");
    m_offset = new QLabel(tr("No offset"));

    // An explicit switch for "use the field's value for every injection", instead of the old
    // per-keystroke toggle whose value depended on how many characters had been typed. The field is
    // only editable while the box is checked. Claude Generated
    m_uniformInject = new QCheckBox(tr("Uniform volume for all injections"));
    m_uniformInject->setToolTip(tr("Apply the volume on the left to every injection instead of the "
                                   "per-injection volumes from the .itc file. Loading a file resets this."));
    m_injct->setEnabled(false);

    connect(m_uniformInject, &QCheckBox::toggled, this, [this](bool on) {
        m_injct->setEnabled(on);
        UpdateData();
    });
    connect(m_injct, &QLineEdit::textChanged, this, [this]() {
        UpdateData();
    });

    QHBoxLayout* hlayout = new QHBoxLayout;

    hlayout->addWidget(m_exp_button);
    hlayout->addWidget(m_exp_file);
    hlayout->addWidget(m_UseParameter);
    hlayout->addWidget(m_showDilution);
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
    layout->addWidget(m_uniformInject, 2, 2);
    layout->addWidget(m_message, 2, 3);

    /*
    hlayout = new QHBoxLayout;
    hlayout->addWidget(new QLabel(tr("Freq:")));
    hlayout->addWidget(m_freq);
    hlayout->addWidget(new QLabel(tr("cal->J")));
    //hlayout->addWidget(m_scale);
    hlayout->addWidget(new QLabel(tr("Experiment Heat:")));
    hlayout->addWidget(m_exp_base);
    hlayout->addWidget(new QLabel(tr("Dilution Heat:")));
    hlayout->addWidget(m_dil_base);
    hlayout->addWidget(m_refit);

    layout->addLayout(hlayout, 3, 0, 1, 4);
*/
    m_mainwidget = new QTabWidget;

    m_table = new QTableWidget;
    //m_table->setFixedWidth(250);

    // Claude Generated: allow manual per-injection volume edits (column 0). The value goes to the
    // processor, which owns the volume vector, so a later UpdateTable() rebuild keeps it and it is
    // stored with the project; useful when the titration step size varies. Guarded by
    // m_updating_table so programmatic fills don't recurse.
    connect(m_table, &QTableWidget::cellChanged, this, [this](int row, int column) {
        if (m_updating_table || column != 0 || row < 0)
            return;
        QTableWidgetItem* item = m_table->item(row, column);
        if (!item)
            return;
        bool ok = false;
        const qreal value = item->data(Qt::DisplayRole).toString().replace(",", ".").toDouble(&ok);
        if (!ok)
            return;
        m_processor->setInjectionVolume(row, value);
        // A manual per-point edit is a per-injection choice, so leave uniform mode - otherwise the
        // next resolve would broadcast the field's value straight back over it.
        m_uniformInject->setChecked(false);
    });

    m_thm_series = new ScatterSeries;
    m_thm_series->setName(tr("Thermogram"));
    m_raw_series = new ScatterSeries;
    m_raw_series->setName(tr("ITC Data"));
    m_dil_series = new ScatterSeries;
    m_dil_series->setName(tr("Dilution Data"));

    m_data_view = new ChartView;
    m_data_view->setModal(true);
    m_data_view->setMinimumSize(400, 300);
    connect(m_data_view, &ChartView::lastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });

    m_data_view->setAutoScaleStrategy(AutoScaleStrategy::QtNiceNumbers);

    m_import_row = new QPushButton;
    m_import_row->setFlat(true);
    m_import_row->setToolTip(tr("Import file containg the first column (eg. injections)."));
    m_import_row->setIcon(Icon("document-import"));
    connect(m_import_row, &QPushButton::clicked, this, &Thermogram::ImportRow);

    m_export_data = new QPushButton;
    m_export_data->setFlat(true);
    m_export_data->setToolTip(tr("Export current integration as plain or dh file."));
    m_export_data->setIcon(Icon("document-save-as"));
    connect(m_export_data, &QPushButton::clicked, this, &Thermogram::ExportData);

    m_splitter = new QSplitter(Qt::Horizontal);
    QWidget* table_holder = new QWidget;
    QVBoxLayout* vlayout = new QVBoxLayout;

    hlayout = new QHBoxLayout;
    hlayout->addWidget(m_import_row);
    hlayout->addWidget(m_export_data);

    vlayout->addLayout(hlayout);
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

    // The dialog table is refreshed via each handler's ThermogramChanged signal (connected in
    // setUi); the widget's IntegrationChanged/CalibrationChanged signals were never emitted after the
    // move-to-core, so the old connections here were dead and have been removed. Claude Generated

    m_experiment->setDisabled(true);
    m_dilution->setDisabled(true);

    setLayout(layout);

    QSettings settings;
    settings.beginGroup("thermogram_dialog");
    m_splitter->restoreState(settings.value("splitterSizes").toByteArray());

    connect(m_showDilution, &QCheckBox::stateChanged, this, [this]() {
        if (!m_showDilution->isChecked())
            m_experiment->addOptionalSeries(QList<QPointF>(), "");
        else
            m_experiment->addOptionalSeries(m_dilution_thermogram->ThermogramSeries(), "Dilution");
    });

    setWindowTitle(tr("Thermogram Dialog"));

    m_data_view->addSeries(m_thm_series);
    m_data_view->addSeries(m_raw_series);
}

Thermogram::~Thermogram()
{
    QSettings settings;
    settings.beginGroup("thermogram_dialog");
    settings.setValue("splitterSizes", m_splitter->saveState());
}

QPair<PeakPick::spectrum, QJsonObject> Thermogram::LoadITCFile(QString& filename, std::vector<PeakPick::Peak>* peaks, qreal& offset, QVector<qreal>& inject)
{
    qreal freq = 0;
    // QSignalBlocker block(m_freq);
    // m_freq->setValue(freq);
    return ToolSet::LoadITCFile(filename, peaks, offset, freq, inject);
}

void Thermogram::ApplySystemParameter(const QJsonObject& parameter)
{
    m_systemparameter = parameter;

    m_Temperature->setText(m_systemparameter[QString::number(AbstractItcModel::Temperature)].toString());
    m_CellConcentration->setText(m_systemparameter[QString::number(AbstractItcModel::CellConcentration)].toString());
    m_SyringeConcentration->setText(m_systemparameter[QString::number(AbstractItcModel::SyringeConcentration)].toString());
    m_CellVolume->setText(m_systemparameter[QString::number(AbstractItcModel::CellVolume)].toString());

    m_UseParameter->setChecked(m_systemparameter.size() != 0);
}

void Thermogram::setScalingFactor(qreal factor)
{
    // The processor sets and re-scales both handlers (emitting resultChanged -> re-render); then
    // both combos are synced to show the shared value. The processor's setter is idempotent and each
    // widget blocks its own combo, so this cannot loop between the two widgets.
    m_processor->setScalingFactor(factor);
    m_experiment->setScalingFactor(factor);
    m_dilution->setScalingFactor(factor);
}

void Thermogram::setScaling(const QString& str)
{
    bool ok = false;
    const qreal factor = QString(str).replace(",", ".").toDouble(&ok);
    if (ok)
        setScalingFactor(factor);
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

    filename = ToolSet::FindFile(filename, m_root_dir, QString(), qApp->instance()->property("FindFileRecursive").toBool());
    if (filename.isEmpty()) {
        m_exp_file->setStyleSheet("background-color: " + excluded());
        qDebug() << "no thermogram found";
        return;
    }
    PeakPick::spectrum original;
    QFileInfo info(filename);

    if (info.suffix() == "itc") {
        qreal offset = 0;
        QVector<qreal> inject;
        try {
            QPair<PeakPick::spectrum, QJsonObject> pair = LoadITCFile(filename, &m_exp_peaks, offset, inject);
            original = pair.first;
            // The experiment defines the titration: its @-lines are the injection protocol, and its
            // cell/syringe concentrations are the system parameters.
            m_processor->setInjectionVolumes(inject);
            ApplySystemParameter(pair.second);

            // The file's per-injection volumes are now authoritative; leave uniform mode so they are
            // not overwritten, and give the operator a recovery path (a reload always wins).
            m_uniformInject->setChecked(false);
            QSignalBlocker block(m_injct);
            if (inject.size())
                m_injct->setText(QString::number(inject.last()));
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
        //m_exp_base->setText(QString::number(offset));
        m_experiment->setFileType(ThermogramWidget::FileType::ITC);
        m_experiment_thermogram->setThermogram(original);
        m_experiment_thermogram->setPeakList(m_exp_peaks);
        //m_experiment->setThermogram(&original, offset);
        //m_experiment->setPeakList(m_exp_peaks);
        //m_exp_peaks = m_experiment->Peaks();
    } else {
        original = LoadXYFile(filename);
        // QSignalBlocker block(m_freq);
        // m_freq->setValue(original.Step());
        m_experiment_thermogram->setThermogram(original);
        //m_experiment->setFileType(ThermogramWidget::FileType::RAW);
        //m_experiment->setThermogram(&original);
    }
    m_experiment_thermogram->Initialise();
    m_experiment_thermogram->IntegrateThermogram();

    m_exp_therm = original;
    m_experiment->setEnabled(true);

    m_exp_file->setText(filename);
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

QVector<QVector<qreal>> Thermogram::ResultRows() const
{
    // ResolveInjectionVolumes() has already made the vector as long as the peak list, so a row past
    // its end means no volume is known - shown as 0, not invented.
    const QVector<qreal> volumes = m_processor->injectionVolumes();
    const QVector<qreal> net = m_processor->netHeat();
    const QVector<qreal> raw_exp = m_processor->experiment()->Integrals();
    // Only show a dilution column for a dilution that is actually being subtracted.
    const QVector<qreal> raw_dil = m_processor->dilutionEnabled() ? m_processor->dilution()->Integrals() : QVector<qreal>();

    QVector<QVector<qreal>> rows;
    rows.reserve(net.size());
    for (int j = 0; j < net.size(); ++j) {
        rows << (QVector<qreal>() << (j < volumes.size() ? volumes[j] : 0.0)
                                  << (j < raw_exp.size() ? raw_exp[j] : 0.0)
                                  << (j < raw_dil.size() ? raw_dil[j] : 0.0)
                                  << net[j]);
    }
    return rows;
}

void Thermogram::UpdateTable()
{
    m_updating_table = true; // suppress the cellChanged handler while we repopulate the table
    m_thm_series->clear();
    m_raw_series->clear();
    m_dil_series->clear();
    m_table->clear();

    const QVector<QVector<qreal>> rows = ResultRows();
    if (rows.isEmpty()) {
        m_updating_table = false;
        return;
    }

    const bool dilution = m_processor->dilutionEnabled();
    m_table->setRowCount(rows.size());
    m_table->setColumnCount(4);
    QChar mu = QChar(956);

    for (int j = 0; j < rows.size(); ++j) {
        QTableWidgetItem* newItem = new QTableWidgetItem(QString::number(rows[j][0]));
        m_table->setItem(j, 0, newItem);

        newItem = new QTableWidgetItem(QString::number(rows[j][1]));
        newItem->background().setColor(m_raw_series->color().lighter());
        newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable); // only the volume column is editable
        m_table->setItem(j, 1, newItem);
        m_raw_series->append(QPointF(j + 1, rows[j][1]));

        newItem = new QTableWidgetItem(QString::number(rows[j][2]));
        if (dilution)
            newItem->setBackground(m_dil_series->color().lighter());
        newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(j, 2, newItem);
        m_dil_series->append(QPointF(j + 1, rows[j][2]));

        newItem = new QTableWidgetItem(QString::number(rows[j][3]));
        newItem->background().setColor(m_thm_series->color().lighter());
        newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(j, 3, newItem);
        m_thm_series->append(QPointF(j + 1, rows[j][3]));
    }

    QStringList header = QStringList() << QString("Volume\n[%1L]").arg(mu)
                                       << "exp. heat\n[raw]"
                                       << "dil. heat\n[raw]"
                                       << "joined heat\n[J]";
    m_table->setHorizontalHeaderLabels(header);
    if (QTableWidgetItem* volHeader = m_table->horizontalHeaderItem(0))
        volHeader->setToolTip(tr("Injection volume in %1L — double-click a cell to edit a single addition point (e.g. when the titration step size varies).").arg(mu));
    m_table->resizeColumnsToContents();

    if (dilution)
        m_data_view->addSeries(m_dil_series);

    m_data_view->setXAxis("Inject Number");
    m_data_view->setYAxis("Heat q");
    m_export_data->setEnabled(m_table->rowCount() && m_table->columnCount());
    m_updating_table = false;

    UpdateMessage(rows.size());
}

void Thermogram::UpdateMessage(int injections)
{
    QString message;
    if (m_uniformInject->isChecked())
        message = tr("Uniform volume %1 %2L applied to all %3 injections.")
                      .arg(m_injct->text())
                      .arg(QChar(956))
                      .arg(injections);
    else
        message = tr("Injection volumes taken from the ITC file (%1 injections).").arg(injections);

    // The dilution/experiment length mismatch used to be silent: shorter dilutions just subtracted
    // zero past their last peak.
    if (m_processor->dilutionEnabled()) {
        const int dil = m_processor->dilution()->Integrals().size();
        if (dil < injections)
            message += tr(" Dilution covers only %1 of %2 injections; the rest are treated as zero "
                          "dilution heat.")
                           .arg(dil)
                           .arg(injections);
    }
    m_message->setText(message);
}

QString Thermogram::Content() const
{
    /* Columns 0 and 3 of the rendered result: what the model is given is by construction what the
     * table shows. Previously this scraped the QTableWidget back out of the view, dereferencing
     * item(i, 0) unguarded and passing on a comma decimal from a manually edited cell that
     * FileHandler would then misparse.
     *
     * Renders ResultRows() rather than m_processor->resultTable(): the two are now equal (the volume
     * vector is resolved to the peak count before every render), but ResultRows() reads the same
     * resolved vector the table does without depending on that resolve having run. Claude Generated */
    QString content;
    for (const QVector<qreal>& row : ResultRows()) {
        content += QString::number(row[0]) + "\t";
        content += QString::number(row[3]) + "\t";
        content += "\n";
    }
    return content;
}

void Thermogram::setDilution()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir(), tr("Supported files (*.txt *.dat *.itc *.ITC);;All files (*.*)"));
    if (filename.isEmpty())
        return;
    setLastDir(filename);

    setDilutionFile(filename);
}

void Thermogram::setDilutionFile(QString filename)
{
    setLastDir(filename);

    filename = ToolSet::FindFile(filename, m_root_dir, QString(), qApp->instance()->property("FindFileRecursive").toBool());
    if (filename.isEmpty()) {
        m_exp_file->setStyleSheet("background-color: " + excluded());
        qDebug() << "no thermogram found";
        m_processor->setDilutionEnabled(false); // nothing loaded: nothing to subtract
        return;
    }

    QFileInfo info(filename);
    PeakPick::spectrum original;
    if (info.suffix() == "itc" || info.suffix() == "ITC") {
        qreal offset = 0;
        /* Both outputs are deliberately dropped. A dilution run has its own @-line volumes and its
         * own cell/syringe concentrations, but the titration is defined by the experiment: adopting
         * either from here used to append the volumes onto the experiment's and overwrite its
         * concentrations in the fields. Claude Generated */
        QVector<qreal> dilution_inject;
        try {
            original = LoadITCFile(filename, &m_dil_peaks, offset, dilution_inject).first;
        } catch (int error) {
            m_processor->setDilutionEnabled(false); // the load failed: do not subtract a stale one
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
        m_dilution_thermogram->setThermogram(original);
        m_dilution_thermogram->setPeakList(m_dil_peaks); // was m_exp_peaks (copy-paste bug): dilution must use its own peaks
        m_showDilution->setEnabled(true);
        //m_dilution->setThermogram(&original, offset);
        //m_dilution->setPeakList(m_exp_peaks);
        //m_dil_base->setText(QString::number(offset));
        //m_dil_peaks = m_dilution->Peaks();
    } else {
        original = LoadXYFile(filename);
        m_dilution->setFileType(ThermogramWidget::FileType::RAW);
        m_dilution_thermogram->setThermogram(original);

        //m_dilution->setThermogram(&original);
    }
    m_dilution_thermogram->Initialise();
    m_dilution_thermogram->IntegrateThermogram();
    m_dilution->setEnabled(true);

    // Now that the dilution is integrated, let it into the join - this is what makes the processor
    // subtract it at all. Enabling it re-joins, so the table follows without further prompting.
    m_processor->setDilutionEnabled(true);

    m_dil_therm = original;
    m_dil_file->setText(filename);
    m_mainwidget->setCurrentIndex(2);
}


void Thermogram::clearExperiment()
{
    if (m_exp_file->text().isEmpty()) {
        m_experiment->clear();
        m_exp_peaks.clear();
        UpdateData();
        m_exp_file->setStyleSheet("background-color: white");
        m_experiment->setDisabled(true);
    } else {
        setExperimentFile(m_exp_file->text());
    }
}

void Thermogram::clearDilution()
{
    if (m_dil_file->text().isEmpty()) {
        m_dilution->clear();
        m_dil_peaks.clear();
        /* Behaviour change, deliberate: clearing the dilution now really stops the subtraction. The
         * widget's clear() only drops its own copies, the handler keeps its peaks, and the renderer
         * re-read them from there - so the dilution kept being subtracted from a field the user had
         * emptied. Claude Generated */
        m_processor->setDilutionEnabled(false);
        UpdateData();
        m_dil_file->setStyleSheet("background-color: white");
        m_dilution->setDisabled(true);
    } else {
        setDilutionFile(m_dil_file->text());
    }
}

void Thermogram::ResolveInjectionVolumes()
{
    if (m_processor->injectionCount() == 0)
        return;

    bool ok = false;
    const qreal scalar = QString(m_injct->text()).replace(",", ".").toDouble(&ok);

    if (m_uniformInject->isChecked() && ok)
        m_processor->setUniformInjectionVolume(scalar);
    else
        // Keep the per-injection volumes; only fill rows the vector does not yet cover. This is the
        // renderer's old "value for rows past the vector" fallback, made a property of the vector.
        m_processor->padInjectionVolumes(ok ? scalar : 0.0);
}

void Thermogram::UpdateData()
{
    ResolveInjectionVolumes();
    UpdateTable();
}

void Thermogram::File2JsonBlock(const QString& filename, QJsonObject& block) const
{
    if (qApp->instance()->property("StoreFileName").toBool()) {
        QFileInfo info(filename);
        if (qApp->instance()->property("StoreAbsolutePath").toBool())
            block["file"] = m_exp_file->text();
        else
            block["file"] = info.fileName();
        if (qApp->instance()->property("StoreFileHash").toBool()) {
            QCryptographicHash fileHashed(QCryptographicHash::Md5);

            QFile file(filename);
            if (file.open(QIODevice::ReadOnly)) {
                fileHashed.addData(file.readAll());
                block["md5"] = QString(fileHashed.result());
            }
        }
    }
}

QJsonObject Thermogram::Raw() const
{
    QJsonObject raw, block;

    block["fit"] = m_experiment_thermogram->getThermogramParameter();
    File2JsonBlock(m_exp_file->text(), block);

    raw["experiment"] = block;

    if (!m_dil_file->text().isEmpty()) {
        block["fit"] = m_dilution_thermogram->getThermogramParameter();
        block["file"] = m_dil_file->text();
        File2JsonBlock(m_dil_file->text(), block);
        raw["dilution"] = block;
    }
    raw["injectvolume"] = m_injct->text();
    return raw;
}

void Thermogram::setRaw(const QJsonObject& object)
{
    m_raw_data = object;

    m_injct->setText(m_raw_data["injectvolume"].toString());

    if (m_raw_data.keys().contains("dilution")) {
        QJsonObject experiment = m_raw_data["dilution"].toObject();
        setDilutionFile(experiment["file"].toString());
        setDilutionFit(experiment["fit"].toObject());
    }

    QJsonObject experiment = m_raw_data["experiment"].toObject();
    setExperimentFit(experiment["fit"].toObject());
    setExperimentFile(experiment["file"].toString());

    if (m_raw_data.keys().contains("scaling"))
        setScaling(m_raw_data["scaling"].toString());
}

void Thermogram::setSystemParameter(const QJsonObject& object)
{
    m_systemparameter = object;

    m_CellVolume->setText(m_systemparameter[QString::number(AbstractItcModel::CellVolume)].toString());
    m_CellConcentration->setText(m_systemparameter[QString::number(AbstractItcModel::CellConcentration)].toString());
    m_SyringeConcentration->setText(m_systemparameter[QString::number(AbstractItcModel::SyringeConcentration)].toString());
    m_Temperature->setText(m_systemparameter[QString::number(AbstractItcModel::Temperature)].toString());
}

QString Thermogram::ProjectName() const
{
    QFileInfo file(m_exp_file->text());
    return file.baseName();
}

void Thermogram::setExperimentFit(const QJsonObject& json)
{
    m_experiment_thermogram->setThermogramParameter(json);
}

void Thermogram::setDilutionFit(const QJsonObject& json)
{
    m_dilution_thermogram->setThermogramParameter(json);
}

void Thermogram::ExportData()
{
    if (!ResultRows().isEmpty() && qFuzzyIsNull(ResultRows().first()[0])) {
        QMessageBox question(QMessageBox::Question, tr("Export Integration"), tr("First column is empty or zero. Do you still want to export the data?"), QMessageBox::Yes | QMessageBox::No, this);
        if (question.exec() == QMessageBox::No) {
            return;
        }
    }

    QString filename = QFileDialog::getSaveFileName(this, "Select file", getDir(), ("Origin Files(*.dh *.DH);;Table Files (*.dat *.txt)"));
    if (filename.isEmpty())
        return;

    setLastDir(filename);

    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
        return;

    QString output;
    QTextStream stream(&file);
    const QVector<QVector<qreal>> rows = ResultRows();

    if (filename.contains(".dh", Qt::CaseInsensitive)) {
        output = "10\n";
        output += "0," + QString::number(rows.size()) + ",0,0,0\n";
        output += QString::number(m_systemparameter[QString::number(AbstractItcModel::Temperature)].toString().toDouble() - 273) + "," + m_systemparameter[QString::number(AbstractItcModel::CellConcentration)].toString() + "," + m_systemparameter[QString::number(AbstractItcModel::SyringeConcentration)].toString() + "," + QString::number(m_systemparameter[QString::number(AbstractItcModel::CellVolume)].toString().toDouble() / 1000.0) + ",0\n";
        output += "0\n";
        output += "0\n";

        for (const QVector<qreal>& row : rows)
            output += QString::number(row[0]) + "," + QString::number(row[3]) + "\n";

        stream << output;
    } else {
        QChar mu = QChar(956);

        stream << QString("#Volume") + "\t" + " exp. heat " + "\t" + "dil. heat" + "\t" + "joined heat" + "\n";
        stream << QString("#[%1L]").arg(mu) + "\t" + "[raw]" + "\t" + "[raw]" + "\t" + "[J]" + "\n";

        for (const QVector<qreal>& row : rows) {
            stream << QString::number(row[0]) + "\t" + QString::number(row[1]) + "\t"
                    + QString::number(row[2]) + "\t" + QString::number(row[3]) + "\n";
        }
    }
}

void Thermogram::ImportRow()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir());
    if (filename.isEmpty())
        return;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QVector<qreal> inject;
    QStringList blob = QString(file.readAll()).split("\n");

    for (const QString& str : qAsConst(blob)) {
        if (str.isEmpty() || str.isNull())
            continue;

        QStringList line = str.simplified().split(" ");
        if (line.size() == 1 && !str.contains("#")) {
            inject << line[0].toDouble();
        }
    }

    // A comment-only or non-numeric file leaves nothing to import; last() would read off the end.
    if (inject.isEmpty()) {
        QMessageBox::warning(this, tr("Import Injection Volumes"),
            tr("No single-column numeric values found in %1.").arg(filename));
        return;
    }

    m_processor->setInjectionVolumes(inject);
    m_injct->setText(QString::number(inject.last()));
    UpdateTable();
}
