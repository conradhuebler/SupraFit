/*
 * NmrImport - dialog of the NMR titration import (spectra, signals, chemical shift table)
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

// Claude Generated: WP3 of roadmap/input_data_editing.md

#include <charts.h>

#include <cmath>

#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtGui/QIcon>
#include <QtGui/QPixmap>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

#include "src/core/models/datatable.h"
#include "src/ui/guitools/appearance.h"
#include "src/ui/widgets/DropTable.h"
#include "src/ui/widgets/traceeditorwidget.h"

#include "src/global.h"

#include "nmrimport.h"

namespace {
/// a number typed by the user: "7.25" or "7,25"; false for anything else
bool parseShift(const QString& text, double* value)
{
    QString t = text.trimmed();
    t.replace(',', '.');
    bool ok = false;
    const double v = QLocale::c().toDouble(t, &ok);
    if (ok && std::isfinite(v))
        *value = v;
    return ok && std::isfinite(v);
}
} // namespace

NmrImport::NmrImport(QWidget* parent)
    : QDialog(parent)
{
    setupUi();
    refresh();
}

QColor NmrImport::statusColor(NmrCellStatus status)
{
    switch (status) {
    case NmrCellStatus::Missing:
        return QColor(214, 72, 62);
    case NmrCellStatus::Conflict:
        return QColor(232, 160, 40);
    case NmrCellStatus::Manual:
        return QColor(86, 140, 222);
    case NmrCellStatus::FitFailed:
        return QColor(156, 92, 204);
    case NmrCellStatus::Ok:
        break;
    }
    return QColor();
}

void NmrImport::setupUi()
{
    QVBoxLayout* layout = new QVBoxLayout;

    // loading
    QHBoxLayout* load = new QHBoxLayout;
    m_directory = new QPushButton(tr("Open directory"));
    m_directory->setToolTip(tr("Reads every file of a directory with the chosen ending, one spectrum per titration step, in natural order (2 before 10)."));
    m_suffix = new QComboBox;
    m_suffix->setEditable(true);
    m_suffix->addItems({ ".txt", ".csv", ".dat", ".xy", ".asc" });
    m_suffix->setToolTip(tr("Ending of the spectrum files in the directory"));
    m_files = new QPushButton(tr("Add files"));
    m_files->setToolTip(tr("Two columns (ppm, intensity) = one spectrum; more columns = ppm and one spectrum per column; TopSpin totxt export."));
    m_path = new QLineEdit;
    m_path->setReadOnly(true);
    load->addWidget(m_directory);
    load->addWidget(m_suffix);
    load->addWidget(m_files);
    load->addWidget(m_path, 1);
    layout->addLayout(load);

    // page 1: spectra, chart, shifts, signals
    QWidget* left = new QWidget;
    QVBoxLayout* left_layout = new QVBoxLayout;
    left_layout->setContentsMargins(0, 0, 0, 0);
    left_layout->addWidget(new QLabel(tr("Spectra")));
    m_spectra = new QListWidget;
    m_spectra->setToolTip(tr("Titration order, top to bottom. Untick a spectrum to leave it out of the table."));
    left_layout->addWidget(m_spectra, 1);
    QHBoxLayout* order = new QHBoxLayout;
    m_up = new QPushButton(tr("Up"));
    m_down = new QPushButton(tr("Down"));
    order->addWidget(m_up);
    order->addWidget(m_down);
    left_layout->addLayout(order);
    QHBoxLayout* edit = new QHBoxLayout;
    m_remove = new QPushButton(tr("Remove"));
    m_reference = new QPushButton(tr("Reference"));
    m_reference->setToolTip(tr("The signals are defined in this spectrum and followed from it in both directions."));
    edit->addWidget(m_remove);
    edit->addWidget(m_reference);
    left_layout->addLayout(edit);
    left->setLayout(left_layout);
    left->setMinimumWidth(150);
    left->setMaximumWidth(220);

    m_editor = new TraceEditorWidget;
    m_results = new QTableWidget;
    m_results->setObjectName("nmrResults");
    m_results->setToolTip(tr("Chemical shifts of the signals per spectrum. Type a value to set a cell by hand, clear it to use the tracked value again."));
    m_results->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_results->verticalHeader()->setDefaultSectionSize(22);
    m_legend = new QLabel;
    m_legend->setWordWrap(true);
    m_details = new QCheckBox(tr("Fit details"));
    m_details->setObjectName("nmrFitDetails");
    m_details->setToolTip(tr("Adds the columns ±δ (standard error), FWHM, area and η of the fitted lines per signal."));
    QWidget* results = new QWidget;
    QVBoxLayout* results_layout = new QVBoxLayout;
    results_layout->setContentsMargins(0, 0, 0, 0);
    results_layout->addWidget(m_results, 1);
    QHBoxLayout* legend_row = new QHBoxLayout;
    legend_row->addWidget(m_legend, 1);
    legend_row->addWidget(m_details);
    results_layout->addLayout(legend_row);
    results->setLayout(results_layout);

    QSplitter* center = new QSplitter(Qt::Vertical);
    center->addWidget(m_editor);
    center->addWidget(results);
    center->setStretchFactor(0, 3);
    center->setStretchFactor(1, 2);
    center->setChildrenCollapsible(false);

    QSplitter* main = new QSplitter(Qt::Horizontal);
    main->addWidget(left);
    main->addWidget(center);
    main->addWidget(signalPanel());
    main->setStretchFactor(0, 0);
    main->setStretchFactor(1, 1);
    main->setStretchFactor(2, 0);
    main->setChildrenCollapsible(false);

    // page 2: concentrations and the table handed over
    QWidget* tables = new QWidget;
    QHBoxLayout* tables_layout = new QHBoxLayout;
    QVBoxLayout* conc_layout = new QVBoxLayout;
    QLabel* conc_label = new QLabel(tr("Concentrations (host, guest), one row per spectrum: paste with Ctrl+V"));
    conc_label->setWordWrap(true);
    conc_layout->addWidget(conc_label);
    m_concentrations = new DropTable;
    conc_layout->addWidget(m_concentrations, 1);
    QVBoxLayout* preview_layout = new QVBoxLayout;
    preview_layout->addWidget(new QLabel(tr("Table for the import")));
    m_preview = new DropTable;
    preview_layout->addWidget(m_preview, 1);
    m_table_status = new QLabel;
    m_table_status->setWordWrap(true);
    preview_layout->addWidget(m_table_status);
    tables_layout->addLayout(conc_layout, 1);
    tables_layout->addLayout(preview_layout, 2);
    tables->setLayout(tables_layout);

    m_tabs = new QTabWidget;
    m_tabs->addTab(main, tr("Spectra and signals"));
    m_tabs->addTab(tables, tr("Concentrations and table"));
    layout->addWidget(m_tabs, 1);

    QHBoxLayout* bottom = new QHBoxLayout;
    m_status = new QLabel;
    m_status->setWordWrap(true);
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bottom->addWidget(m_status, 1);
    bottom->addWidget(m_buttons);
    layout->addLayout(bottom);
    setLayout(layout);

    setWindowTitle(tr("NMR Titration Import"));
    // fits a 1366x768 screen; opens larger where there is room
    setMinimumSize(960, 600);
    resize(1280, 800);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &NmrImport::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_directory, &QPushButton::clicked, this, [this]() {
        const QString directory = QFileDialog::getExistingDirectory(this, tr("Open directory"), getDir());
        if (directory.isEmpty())
            return;
        setLastDir(directory);
        m_handler.clear();
        QString error;
        setDirectory(directory, m_suffix->currentText(), &error);
    });
    connect(m_files, &QPushButton::clicked, this, [this]() {
        const QStringList files = QFileDialog::getOpenFileNames(this, tr("Add files"), getDir());
        if (files.isEmpty())
            return;
        setLastDir(files.first());
        QString error;
        addFiles(files, &error);
    });
    connect(m_spectra, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        if (m_updating)
            return;
        m_handler.setSpectrumExcluded(m_spectra->row(item), item->checkState() != Qt::Checked);
        retrack();
    });
    auto move = [this](int delta) {
        const int row = m_spectra->currentRow();
        if (!m_handler.moveSpectrum(row, row + delta))
            return;
        refresh();
        m_spectra->setCurrentRow(row + delta);
    };
    connect(m_up, &QPushButton::clicked, this, [move]() { move(-1); });
    connect(m_down, &QPushButton::clicked, this, [move]() { move(+1); });
    connect(m_remove, &QPushButton::clicked, this, [this]() {
        if (m_handler.removeSpectrum(m_spectra->currentRow()))
            refresh();
    });
    connect(m_reference, &QPushButton::clicked, this, [this]() {
        if (m_spectra->currentRow() < 0)
            return;
        m_handler.setReferenceIndex(m_spectra->currentRow());
        retrack();
    });

    connect(m_editor, &TraceEditorWidget::operationsChanged, this, [this]() {
        m_handler.setOperations(m_editor->operations());
        retrack();
    });
    connect(m_editor, &TraceEditorWidget::chartUpdated, this, &NmrImport::updateMarkers);
    connect(m_editor->chart(), &ChartView::pointDoubleClicked, this, [this](const QPointF& point) { m_position->setValue(point.x()); });

    connect(m_results, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (m_updating)
            return;
        const int spectrum = item->row();
        const int per = m_details->isChecked() && m_handler.fitSettings().method == NmrShiftMethod::LineshapeFit ? 5 : 1;
        const QVector<int> columns = m_handler.enabledSignals();
        if (item->column() % per != 0)
            return;
        const int column = item->column() / per;
        if (column < 0 || column >= columns.size())
            return;
        const int signal = columns[column];
        double value = 0;
        if (item->text().trimmed().isEmpty())
            m_handler.clearManualShift(spectrum, signal);
        else if (parseShift(item->text(), &value))
            m_handler.setManualShift(spectrum, signal, value);
        else
            setStatus(tr("'%1' is not a chemical shift.").arg(item->text()), true);
        QMetaObject::invokeMethod(this, [this]() { updateResults(); updateMarkers(); updateTable(); }, Qt::QueuedConnection);
    });
    connect(m_concentrations, &DropTable::Edited, this, &NmrImport::updateTable);
    connect(m_details, &QCheckBox::toggled, this, [this]() { updateResults(); });
    connect(m_spectra, &QListWidget::currentRowChanged, this, [this]() {
        if (!m_updating)
            updateMarkers();
    });
}

QWidget* NmrImport::signalPanel()
{
    QGroupBox* box = new QGroupBox(tr("Signals"));
    QVBoxLayout* layout = new QVBoxLayout;
    QFormLayout* form = new QFormLayout;

    m_offset = new QDoubleSpinBox;
    m_offset->setDecimals(4);
    m_offset->setRange(-100, 100);
    m_offset->setSingleStep(0.001);
    m_offset->setSuffix(tr(" ppm"));
    m_offset->setKeyboardTracking(false);
    m_offset->setToolTip(tr("Added to the ppm axis of every spectrum (referencing); the signals move along."));
    form->addRow(tr("Reference offset"), m_offset);

    m_window = new QDoubleSpinBox;
    m_window->setDecimals(4);
    m_window->setRange(0.0005, 5);
    m_window->setSingleStep(0.005);
    m_window->setValue(m_handler.defaultWindow());
    m_window->setSuffix(tr(" ppm"));
    m_window->setKeyboardTracking(false);
    m_window->setToolTip(tr("Window of new signals: a signal is searched within this distance of its last position and of the position expected from its last two steps."));
    form->addRow(tr("Search window ±"), m_window);

    m_method = new QComboBox;
    m_method->setObjectName("nmrShiftMethod");
    m_method->addItems({ tr("Peak picking"), tr("Lineshape fit") });
    m_method->setToolTip(tr("Peak picking: δ is the position of the maximum. Lineshape fit: δ is the centre of a Gauss-Lorentz (pseudo-Voigt) line fitted to the whole signal, also where lines overlap."));
    form->addRow(tr("Shift from"), m_method);
    layout->addLayout(form);

    m_fit_options = new QWidget;
    QFormLayout* fit_form = new QFormLayout;
    fit_form->setContentsMargins(0, 0, 0, 0);
    m_width_mode = new QComboBox;
    m_width_mode->setObjectName("nmrWidthMode");
    m_width_mode->addItems({ tr("Free per spectrum"), tr("Shared over the series") });
    m_width_mode->setToolTip(tr("Free: width and Gauss-Lorentz mixing of each line are fitted in every spectrum. Shared: one width and mixing per signal for all spectra, fitted together."));
    fit_form->addRow(tr("Line width"), m_width_mode);
    m_baseline = new QComboBox;
    m_baseline->setObjectName("nmrBaseline");
    m_baseline->addItems({ tr("None"), tr("Constant"), tr("Linear") });
    m_baseline->setToolTip(tr("Baseline under the lines of a fit range."));
    fit_form->addRow(tr("Baseline"), m_baseline);
    m_fit_range = new QDoubleSpinBox;
    m_fit_range->setObjectName("nmrFitRange");
    m_fit_range->setDecimals(4);
    m_fit_range->setRange(0.001, 5);
    m_fit_range->setSingleStep(0.005);
    m_fit_range->setSuffix(tr(" ppm"));
    m_fit_range->setKeyboardTracking(false);
    m_fit_range->setToolTip(tr("Points within this distance of a signal are fitted; signals with overlapping ranges are fitted together, with further peaks in the range as neighbour lines."));
    fit_form->addRow(tr("Fit range ±"), m_fit_range);
    m_max_iterations = new QSpinBox;
    m_max_iterations->setObjectName("nmrMaxIterations");
    m_max_iterations->setRange(1, 10000);
    m_max_iterations->setKeyboardTracking(false);
    fit_form->addRow(tr("Max. iterations"), m_max_iterations);
    m_fallback = new QCheckBox(tr("Peak position where the fit fails"));
    m_fallback->setObjectName("nmrPeakFallback");
    m_fallback->setToolTip(tr("A cell whose fit fails takes the position of the picked peak instead of \"fit failed\"."));
    fit_form->addRow(m_fallback);
    m_fit_options->setLayout(fit_form);
    layout->addWidget(m_fit_options);

    m_pick = new QPushButton(tr("Pick from reference spectrum"));
    m_pick->setToolTip(tr("One signal per peak of the reference spectrum (peak picking of the processing steps, else peaks above 5 % of the intensity span); replaces the signals and the values set by hand."));
    layout->addWidget(m_pick);

    QHBoxLayout* add = new QHBoxLayout;
    m_position = new QDoubleSpinBox;
    m_position->setDecimals(4);
    m_position->setRange(-1000, 1000);
    m_position->setSuffix(tr(" ppm"));
    m_position->setToolTip(tr("Position of a new signal; a double-click in the chart takes the position."));
    m_add = new QPushButton(tr("Add"));
    m_add->setToolTip(tr("Adds a signal at this position with the search window."));
    add->addWidget(m_position, 1);
    add->addWidget(m_add);
    layout->addLayout(add);

    m_signal_table = new QTableWidget(0, 3);
    m_signal_table->setObjectName("nmrSignals");
    m_signal_table->setHorizontalHeaderLabels({ tr("Name"), tr("δ /ppm"), tr("± /ppm") });
    m_signal_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_signal_table->verticalHeader()->setDefaultSectionSize(22);
    m_signal_table->setToolTip(tr("Untick a signal to leave it out of the table; name, position and window can be edited."));
    layout->addWidget(m_signal_table, 1);
    m_remove_signal = new QPushButton(tr("Remove signal"));
    layout->addWidget(m_remove_signal);
    box->setLayout(layout);
    box->setMinimumWidth(230);
    box->setMaximumWidth(300);

    connect(m_offset, &QDoubleSpinBox::valueChanged, this, [this](double offset) {
        if (m_updating)
            return;
        m_handler.setReferenceOffset(offset);
        refresh();
    });
    connect(m_window, &QDoubleSpinBox::valueChanged, this, [this](double window) { m_handler.setDefaultWindow(window); });
    connect(m_method, &QComboBox::currentIndexChanged, this, &NmrImport::applyFitSettings);
    connect(m_width_mode, &QComboBox::currentIndexChanged, this, &NmrImport::applyFitSettings);
    connect(m_baseline, &QComboBox::currentIndexChanged, this, &NmrImport::applyFitSettings);
    connect(m_fit_range, &QDoubleSpinBox::valueChanged, this, &NmrImport::applyFitSettings);
    connect(m_max_iterations, &QSpinBox::valueChanged, this, &NmrImport::applyFitSettings);
    connect(m_fallback, &QCheckBox::toggled, this, &NmrImport::applyFitSettings);
    updateFitControls();
    connect(m_pick, &QPushButton::clicked, this, [this]() {
        QString error;
        const int n = m_handler.pickSignals(m_window->value(), &error);
        if (!error.isEmpty())
            setStatus(error, true);
        else
            setStatus(tr("%1 signal(s) picked in spectrum %2.").arg(n).arg(m_handler.referenceIndex() + 1), false);
        retrack();
    });
    connect(m_add, &QPushButton::clicked, this, [this]() {
        m_handler.addSignal(m_position->value(), m_window->value());
        retrack();
    });
    connect(m_remove_signal, &QPushButton::clicked, this, [this]() {
        if (m_handler.removeSignal(m_signal_table->currentRow()))
            retrack();
    });
    connect(m_signal_table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (m_updating)
            return;
        const int row = item->row();
        if (row < 0 || row >= m_handler.signalList().size())
            return;
        NmrSignal signal = m_handler.signalList()[row];
        double value = 0;
        if (item->column() == 0) {
            signal.name = item->text().trimmed().isEmpty() ? signal.name : item->text().trimmed();
            signal.enabled = item->checkState() == Qt::Checked;
        } else if (parseShift(item->text(), &value)) {
            if (item->column() == 1)
                signal.position = value;
            else if (value > 0)
                signal.window = value;
        }
        if (!m_handler.setSignal(row, signal))
            setStatus(tr("Signal %1 not changed: the window must be positive.").arg(row + 1), true);
        QMetaObject::invokeMethod(this, [this]() { retrack(); }, Qt::QueuedConnection);
    });
    return box;
}

/*! \brief Shows the fit settings of the handler in the controls (no tracking). Claude Generated. */
void NmrImport::updateFitControls()
{
    const bool updating = m_updating;
    m_updating = true;
    const NmrFitSettings& fit = m_handler.fitSettings();
    m_method->setCurrentIndex(fit.method == NmrShiftMethod::LineshapeFit ? 1 : 0);
    m_width_mode->setCurrentIndex(fit.widthMode == NmrWidthMode::Shared ? 1 : 0);
    m_baseline->setCurrentIndex(int(fit.baseline));
    m_fit_range->setValue(fit.range);
    m_max_iterations->setValue(fit.maxIterations);
    m_fallback->setChecked(fit.peakFallback);
    m_fit_options->setEnabled(fit.method == NmrShiftMethod::LineshapeFit);
    m_details->setEnabled(fit.method == NmrShiftMethod::LineshapeFit);
    m_updating = updating;
}

/*! \brief Passes the fit controls to the handler and tracks again. Claude Generated. */
void NmrImport::applyFitSettings()
{
    if (m_updating)
        return;
    NmrFitSettings fit;
    fit.method = m_method->currentIndex() == 1 ? NmrShiftMethod::LineshapeFit : NmrShiftMethod::PeakPicking;
    fit.widthMode = m_width_mode->currentIndex() == 1 ? NmrWidthMode::Shared : NmrWidthMode::Free;
    fit.baseline = LineshapeBaseline(std::clamp(m_baseline->currentIndex(), 0, 2));
    fit.range = m_fit_range->value();
    fit.maxIterations = m_max_iterations->value();
    fit.peakFallback = m_fallback->isChecked();
    m_handler.setFitSettings(fit);
    updateFitControls();
    retrack();
}

int NmrImport::fitSpectrum() const
{
    const int row = m_spectra->currentRow();
    return row >= 0 && row < m_handler.spectrumCount() ? row : m_handler.referenceIndex();
}

bool NmrImport::addFiles(const QStringList& files, QString* error)
{
    QStringList failed;
    for (const QString& file : files) {
        QString reason;
        if (!m_handler.addFile(file, &reason))
            failed << reason;
    }
    m_path->setText(files.size() == 1 ? files.first() : QFileInfo(files.value(0)).path());
    if (!failed.isEmpty()) {
        if (error)
            *error = failed.join("\n");
        setStatus(tr("Not read: %1").arg(failed.join("; ")), true);
    } else
        setStatus(tr("%1 spectra.").arg(m_handler.spectrumCount()), false);
    m_editor->setOperations(m_handler.operations());
    refresh();
    return failed.isEmpty();
}

bool NmrImport::setDirectory(const QString& directory, const QString& suffix, QString* error)
{
    QString reason;
    const bool ok = m_handler.addDirectory(directory, suffix, &reason);
    m_path->setText(directory);
    if (!reason.isEmpty())
        setStatus(tr("Not read: %1").arg(reason), true);
    else
        setStatus(tr("%1 spectra.").arg(m_handler.spectrumCount()), false);
    if (error)
        *error = reason;
    refresh();
    return ok;
}

void NmrImport::setData(const QJsonObject& project)
{
    QString error;
    m_handler.fromJson(project.value("raw").toObject(), &error);
    if (!error.isEmpty())
        setStatus(error, true);
    const QJsonObject independent = project.value("independent").toObject();
    if (!independent.isEmpty()) {
        DataTable table(independent);
        setConcentrations(&table);
    }
    m_updating = true;
    m_offset->setValue(m_handler.referenceOffset());
    m_window->setValue(m_handler.defaultWindow());
    m_updating = false;
    updateFitControls();
    m_editor->setOperations(m_handler.operations());
    m_editor->setShowProcessed(!m_handler.operations().isEmpty());
    refresh();
}

void NmrImport::setConcentrations(const DataTable* table)
{
    if (!table)
        return;
    DataTable* copy = new DataTable(*const_cast<DataTable*>(table));
    copy->setParent(m_concentrations);
    QAbstractItemModel* old = m_concentrations->model();
    m_concentrations->setModel(copy);
    if (old && old->parent() == m_concentrations)
        delete old;
    updateTable();
}

void NmrImport::retrack()
{
    m_handler.track();
    updateSpectra();
    updateSignals();
    updateResults();
    updateMarkers();
    updateTable();
}

void NmrImport::refresh()
{
    m_handler.track();
    m_editor->setTraces(m_handler.referencedSpectra()); // redraws, the markers follow (chartUpdated)
    updateSpectra();
    updateSignals();
    updateResults();
    updateTable();
}

void NmrImport::updateSpectra()
{
    m_updating = true;
    const int row = m_spectra->currentRow();
    m_spectra->clear();
    const QList<QColor>& colors = m_editor->traceColors();
    for (int i = 0; i < m_handler.spectrumCount(); ++i) {
        const XYTrace& trace = m_handler.spectra().at(i);
        const bool reference = i == m_handler.referenceIndex();
        QListWidgetItem* item = new QListWidgetItem(reference ? tr("%1 (reference)").arg(trace.name()) : trace.name());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_handler.isSpectrumExcluded(i) ? Qt::Unchecked : Qt::Checked);
        item->setToolTip(trace.source());
        if (reference) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
        if (i < colors.size()) {
            // a colour chip, not the background: the check box stays readable on every colour
            QPixmap chip(12, 12);
            chip.fill(colors[i]);
            item->setIcon(QIcon(chip));
        }
        m_spectra->addItem(item);
    }
    m_spectra->setCurrentRow(std::min(row, m_spectra->count() - 1));
    m_updating = false;
}

void NmrImport::updateSignals()
{
    m_updating = true;
    const QVector<NmrSignal>& list = m_handler.signalList();
    m_signal_table->setRowCount(list.size());
    for (int s = 0; s < list.size(); ++s) {
        QTableWidgetItem* name = new QTableWidgetItem(list[s].name);
        name->setFlags(name->flags() | Qt::ItemIsUserCheckable);
        name->setCheckState(list[s].enabled ? Qt::Checked : Qt::Unchecked);
        m_signal_table->setItem(s, 0, name);
        m_signal_table->setItem(s, 1, new QTableWidgetItem(QString::number(list[s].position, 'f', 4)));
        m_signal_table->setItem(s, 2, new QTableWidgetItem(QString::number(list[s].window, 'f', 4)));
    }
    m_updating = false;
}

void NmrImport::updateResults()
{
    m_updating = true;
    const QVector<int> columns = m_handler.enabledSignals();
    const bool fit = m_handler.fitSettings().method == NmrShiftMethod::LineshapeFit;
    const int per = fit && m_details->isChecked() ? 5 : 1; ///< columns per signal: δ (, ±δ, FWHM, area, η)
    const int rows = m_handler.spectrumCount();
    m_results->clear();
    m_results->setRowCount(rows);
    m_results->setColumnCount(columns.size() * per);
    QStringList header;
    for (int c : columns) {
        header << m_handler.signalList()[c].name;
        if (per > 1)
            header << tr("±δ") << tr("FWHM") << tr("Area") << tr("η");
    }
    m_results->setHorizontalHeaderLabels(header);
    m_results->horizontalHeader()->setSectionResizeMode(per > 1 ? QHeaderView::ResizeToContents : QHeaderView::Stretch);
    QStringList vertical;
    for (int k = 0; k < rows; ++k)
        vertical << QString::number(k + 1);
    m_results->setVerticalHeaderLabels(vertical);
    int missing = 0, conflict = 0, manual = 0, failed = 0;
    QTableWidgetItem* first = nullptr; ///< first cell without a unique shift, scrolled into view
    auto number = [](double v, char format, int precision) { return std::isfinite(v) ? QString::number(v, format, precision) : QString(); };
    for (int k = 0; k < rows; ++k) {
        const bool excluded = m_handler.isSpectrumExcluded(k);
        for (int c = 0; c < columns.size(); ++c) {
            const NmrCell cell = m_handler.cell(k, columns[c]);
            QTableWidgetItem* item = new QTableWidgetItem;
            const QString emptyText = cell.status == NmrCellStatus::FitFailed ? tr("fit failed") : tr("missing");
            item->setText(std::isfinite(cell.shift) ? QString::number(cell.shift, 'f', 4) : (excluded ? QString() : emptyText));
            QString tip = tr("%1, spectrum %2: %3").arg(m_handler.signalList()[columns[c]].name).arg(k + 1).arg(NmrHandler::statusName(cell.status));
            if (fit && std::isfinite(cell.peakShift))
                tip += tr("\npeak maximum %1 ppm").arg(cell.peakShift, 0, 'f', 4);
            if (fit && cell.fitState == NmrFitState::Failed)
                tip += tr("\nfit: %1").arg(cell.fitMessage);
            item->setToolTip(tip);
            if (excluded) {
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                item->setForeground(palette().color(QPalette::Disabled, QPalette::Text));
            } else {
                const QColor color = statusColor(cell.status);
                if (color.isValid()) {
                    item->setBackground(color);
                    item->setForeground(Appearance::ContrastText(color));
                }
                if (fit && cell.status == NmrCellStatus::Ok && cell.fitState == NmrFitState::Failed) {
                    QFont font = item->font(); // peak position taken because the fit failed
                    font.setItalic(true);
                    item->setFont(font);
                }
                if (!first && (cell.status == NmrCellStatus::Missing || cell.status == NmrCellStatus::Conflict || cell.status == NmrCellStatus::FitFailed))
                    first = item;
                missing += cell.status == NmrCellStatus::Missing;
                conflict += cell.status == NmrCellStatus::Conflict;
                manual += cell.status == NmrCellStatus::Manual;
                failed += cell.status == NmrCellStatus::FitFailed || (cell.status == NmrCellStatus::Ok && cell.fitState == NmrFitState::Failed);
            }
            m_results->setItem(k, c * per, item);
            if (per > 1) {
                const bool shown = !excluded && cell.fitState != NmrFitState::NotFitted;
                const QStringList values = { number(cell.shiftError, 'g', 2), number(cell.fwhm, 'f', 4), number(cell.area, 'g', 4), number(cell.eta, 'f', 2) };
                for (int d = 0; d < 4; ++d) {
                    QTableWidgetItem* detail = new QTableWidgetItem(shown ? values[d] : QString());
                    detail->setFlags(detail->flags() & ~Qt::ItemIsEditable);
                    if (cell.fitState == NmrFitState::Failed)
                        detail->setForeground(palette().color(QPalette::Disabled, QPalette::Text));
                    m_results->setItem(k, c * per + 1 + d, detail);
                }
            }
        }
    }
    if (first)
        m_results->scrollToItem(first, QAbstractItemView::PositionAtCenter);
    m_updating = false;
    auto chip = [](NmrCellStatus status, const QString& text) {
        const QColor color = statusColor(status);
        return QString("<span style='background-color:%1; color:%2'>&nbsp;%3&nbsp;</span>").arg(color.name(), Appearance::ContrastText(color).name(), text);
    };
    QString legend = QString("%1 %2 %3").arg(chip(NmrCellStatus::Missing, tr("missing: %1").arg(missing)), chip(NmrCellStatus::Conflict, tr("conflict: %1").arg(conflict)), chip(NmrCellStatus::Manual, tr("set by hand: %1").arg(manual)));
    if (fit)
        legend += QString(" %1 %2").arg(chip(NmrCellStatus::FitFailed, tr("fit failed: %1").arg(failed)), tr("(fit %1 ms)").arg(m_handler.fitMilliseconds(), 0, 'f', 0));
    m_legend->setText(legend);
}

/*! \brief Marks the tracked position of every signal on the drawn spectra (raw or processed, as the
 * chart shows them) and the reference positions as vertical lines. */
void NmrImport::updateMarkers()
{
    ChartView* chart = m_editor->chart();
    for (const QPointer<ScatterSeries>& series : m_markers) {
        if (series) {
            chart->removeSeries(series);
            delete series;
        }
    }
    m_markers.clear();
    chart->privateView()->removeAllVerticalLines();
    if (m_handler.spectrumCount() == 0)
        return;
    const TraceSet shown = m_editor->showProcessed() ? m_handler.processedSpectra() : m_handler.referencedSpectra();
    const QVector<int> columns = m_handler.enabledSignals();
    for (const QPointer<LineSeries>& series : m_fit_lines) {
        if (series) {
            chart->removeSeries(series);
            delete series;
        }
    }
    m_fit_lines.clear();
    for (int c : columns) {
        ScatterSeries* markers = new ScatterSeries;
        for (int k = 0; k < shown.size(); ++k) {
            const NmrCell cell = m_handler.cell(k, c);
            if (!std::isfinite(cell.shift) || m_handler.isSpectrumExcluded(k))
                continue;
            const double y = shown.at(k).interpolate(cell.shift);
            if (std::isfinite(y))
                markers->append(cell.shift, y);
        }
        markers->setMarkerSize(7);
        chart->addSeries(markers, false);
        m_markers << markers;
        markers->setColor(palette().color(QPalette::Text));
        chart->privateView()->addVerticalLine(m_handler.signalList()[c].position);
    }
    // the fitted model (dashed) and its residual (thin, 10 % of the spectrum height below zero)
    const int k = fitSpectrum();
    const QVector<NmrFitRegion> regions = m_handler.fitRegions(k);
    if (!regions.isEmpty() && k < shown.size() && !shown.at(k).isEmpty()) {
        const double offset = -0.1 * shown.at(k).y().cwiseAbs().maxCoeff();
        const QColor model = palette().color(QPalette::Text); // dashed: distinct from the coloured spectra
        QColor residual = palette().color(QPalette::Text);
        residual.setAlpha(140);
        for (const NmrFitRegion& region : regions) {
            if (region.block.x.size() == 0)
                continue;
            LineSeries* curve = new LineSeries;
            LineSeries* rest = new LineSeries;
            for (Eigen::Index i = 0; i < region.block.x.size(); ++i) {
                const double x = region.block.x(i);
                curve->append(x, region.block.evaluate(x));
                rest->append(x, offset + region.block.residuals(i));
            }
            chart->addSeries(curve, false);
            chart->addSeries(rest, false);
            curve->setColor(model);
            curve->setLineWidth(2);
            curve->setDashDotLine(true);
            rest->setColor(residual);
            rest->setLineWidth(1);
            m_fit_lines << curve << rest;
        }
    }
    m_editor->refreshMarkers();
}

void NmrImport::updateTable()
{
    QString error;
    const bool ok = validate(&error);
    m_table_status->setText(ok ? tr("%1 rows, %2 concentration and %3 shift column(s).").arg(m_preview->model() ? m_preview->model()->rowCount() : 0).arg(m_independent_count).arg(m_handler.enabledSignals().size()) : error);
    QPalette palette = m_table_status->palette();
    palette.setColor(QPalette::WindowText, ok ? this->palette().color(QPalette::WindowText) : (Appearance::IsDark() ? QColor(255, 120, 110) : QColor(180, 30, 20)));
    m_table_status->setPalette(palette);
}

bool NmrImport::validate(QString* error)
{
    m_input_table = QJsonObject();
    m_independent_count = 0;
    DataTable* concentrations = qobject_cast<DataTable*>(m_concentrations->model());
    std::unique_ptr<DataTable> table;
    QString reason;
    if (!concentrations || concentrations->columnCount() == 0 || concentrations->rowCount() == 0) {
        reason = tr("No concentrations: paste the host and guest concentrations of the spectra (one row per spectrum) on the page \"Concentrations and table\".");
        // the shifts alone, so the preview shows what there is
        table.reset(m_handler.shiftTable(&reason));
        if (table)
            reason = tr("No concentrations: paste the host and guest concentrations of the spectra (one row per spectrum) on the page \"Concentrations and table\".");
    } else
        table.reset(m_handler.importTable(concentrations, &reason));

    QAbstractItemModel* old = m_preview->model();
    if (table) {
        DataTable* shown = new DataTable(*table);
        shown->setParent(m_preview);
        m_preview->setModel(shown);
    } else
        m_preview->setModel(nullptr);
    if (old && old->parent() == m_preview)
        delete old;

    if (!concentrations || concentrations->columnCount() == 0 || concentrations->rowCount() == 0 || !table) {
        if (error)
            *error = reason;
        return false;
    }
    m_input_table = table->ExportTable(true);
    m_independent_count = concentrations->columnCount();
    return true;
}

void NmrImport::accept()
{
    QString error;
    if (!validate(&error)) {
        QMessageBox::warning(this, tr("Shift table not complete"), error);
        return;
    }
    QDialog::accept();
}

void NmrImport::setStatus(const QString& text, bool error)
{
    m_status->setText(text);
    QPalette palette = m_status->palette();
    palette.setColor(QPalette::WindowText, error ? (Appearance::IsDark() ? QColor(255, 120, 110) : QColor(180, 30, 20)) : this->palette().color(QPalette::WindowText));
    m_status->setPalette(palette);
}
