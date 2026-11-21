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

#include <algorithm>
#include <cmath>

#include <QtCore/QCollator>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
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
#include "src/ui/guitools/dialogmemory.h"
#include "src/ui/guitools/importkeyguard.h"
#include "src/ui/guitools/morelesstoggle.h"
#include "src/ui/guitools/seriessourcebar.h"
#include "src/ui/guitools/statusline.h"
#include "src/ui/guitools/waiter.h"
#include "src/ui/widgets/DropTable.h"
#include "src/ui/widgets/concentrationeditor.h"
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
    m_guard = new ImportKeyGuard(this);
    QVBoxLayout* layout = new QVBoxLayout;

    m_memory = new DialogMemory(this, "nmrimport");

    // loading: directory, files, drag and drop (Claude Generated, UX package 2a)
    m_source = new SeriesSourceBar;
    m_source->setObjectName("nmrSource");
    m_source->setEndings({ ".txt", ".csv", ".dat", ".xy", ".asc", ".jdf" }, true);
    m_source->setMatch(SeriesSourceBar::Match::EndsWith);
    m_source->setFileFilter(tr("Spectra (*.txt *.csv *.dat *.xy *.asc *.jdf);;JEOL Delta (*.jdf);;All files (*)"));
    m_source->setDirectoryToolTip(tr("Replaces the series by every file of a directory with the chosen ending, one spectrum per titration step, in natural order (2 before 10)."));
    m_source->setAddFilesToolTip(tr("Appends files: two columns (ppm, intensity) = one spectrum; more columns = ppm and one spectrum per column; TopSpin totxt export; JEOL Delta FID (.jdf). Files or a directory can also be dropped on the dialog."));
    m_source->setEnding(m_memory->value("fileEnding").toString());
    m_source->installDropTarget(this);
    layout->addWidget(m_source);

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
    left_layout->addWidget(fidPanel());
    left->setLayout(left_layout);
    left->setMinimumWidth(150);
    left->setMaximumWidth(240);

    m_editor = new TraceEditorWidget;
    m_editor->setSettingsKey("nmrimport"); // stack offsets of this dialog (QSettings traceeditor/nmrimport/...)
    // chemical shift high to low from left to right, whatever the order of the file (Claude Generated)
    m_reverse_axis = new QCheckBox(tr("Chemical shift axis: high to low (left to right)"));
    m_reverse_axis->setObjectName("nmrReverseAxis");
    m_reverse_axis->setToolTip(tr("Draws the ppm axis with high values on the left, as usual for 1H spectra. Off: low values on the left."));
    m_reverse_axis->setChecked(QSettings().value("nmrimport/reverseAxis", true).toBool());
    m_editor->addToolWidget(m_reverse_axis);
    m_editor->setAxisDirection(m_reverse_axis->isChecked() ? TraceEditorWidget::AxisDirection::Reversed : TraceEditorWidget::AxisDirection::Normal);
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
    center->setObjectName("nmrCenterSplitter");
    center->addWidget(m_editor);
    center->addWidget(results);
    center->setStretchFactor(0, 3);
    center->setStretchFactor(1, 2);
    center->setChildrenCollapsible(false);

    QSplitter* main = new QSplitter(Qt::Horizontal);
    main->setObjectName("nmrMainSplitter");
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
    m_concentrations = new ConcentrationEditor(tr("Concentrations (host, guest), one row per spectrum"));
    m_concentrations->setObjectName("nmrConcentrations");
    QVBoxLayout* preview_layout = new QVBoxLayout;
    preview_layout->addWidget(new QLabel(tr("Table for the import")));
    m_preview = new DropTable;
    preview_layout->addWidget(m_preview, 1);
    m_table_status = new QLabel;
    m_table_status->setWordWrap(true);
    preview_layout->addWidget(m_table_status);
    tables_layout->addWidget(m_concentrations, 1);
    tables_layout->addLayout(preview_layout, 2);
    tables->setLayout(tables_layout);

    m_tabs = new QTabWidget;
    m_tabs->addTab(main, tr("Spectra and signals"));
    m_tabs->addTab(tables, tr("Concentrations and table"));
    layout->addWidget(m_tabs, 1);

    // bottom: the last action and what OK hands over, on both pages (Claude Generated, UX package 1)
    QHBoxLayout* bottom = new QHBoxLayout;
    QVBoxLayout* lines = new QVBoxLayout;
    lines->setSpacing(2);
    m_status = new StatusLine;
    m_status->setObjectName("nmrStatus");
    m_ready = new StatusLine;
    m_ready->setObjectName("nmrReady");
    // hint for signals missing in every spectrum: own line, shown only while the table has such a signal
    m_hint = new StatusLine;
    m_hint->setObjectName("nmrMissingHint");
    m_hint->setVisible(false);
    lines->addWidget(m_hint);
    lines->addWidget(m_status);
    lines->addWidget(m_ready);
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bottom->addLayout(lines, 1);
    bottom->addWidget(m_buttons);
    layout->addLayout(bottom);
    setLayout(layout);

    setWindowTitle(tr("NMR Titration Import"));
    // fits a 1366x768 screen; opens larger where there is room
    setMinimumSize(960, 600);
    resize(1280, 800);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &NmrImport::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &NmrImport::reject);
    connect(m_source, &SeriesSourceBar::directoryChosen, this, [this](const QString& directory) {
        QString error;
        // a directory without such a file only says so; the series stays
        if (!SeriesSourceBar::filesInDirectory(directory, m_source->ending(), SeriesSourceBar::Match::EndsWith).isEmpty())
            m_handler.clear();
        setDirectory(directory, m_source->ending(), &error);
    });
    connect(m_source, &SeriesSourceBar::filesChosen, this, [this](const QStringList& files) { addFiles(files); });
    connect(m_source, &SeriesSourceBar::filesDropped, this, [this](const QStringList& files, const QList<SeriesSourceBar::Failure>& failed) { addFiles(files, nullptr, failed); });
    connect(m_source, &SeriesSourceBar::endingChanged, this, [this](const QString& ending) { m_memory->setValue("fileEnding", ending.trimmed()); });
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
        const int row = m_spectra->currentRow();
        if (row < 0 || row >= m_handler.spectrumCount())
            return;
        const int manual = manualCount(row, -1);
        if (manual > 0 && !m_guard->confirm(tr("Remove spectrum %1 and discard %2 hand-set value(s)?").arg(row + 1).arg(manual), tr("Remove")))
            return;
        if (m_handler.removeSpectrum(row))
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
    connect(m_reverse_axis, &QCheckBox::toggled, this, [this](bool on) {
        QSettings().setValue("nmrimport/reverseAxis", on);
        m_editor->setAxisDirection(on ? TraceEditorWidget::AxisDirection::Reversed : TraceEditorWidget::AxisDirection::Normal);
    });
    connect(m_editor->chart(), &ChartView::pointDoubleClicked, this, [this](const QPointF& point) {
        // the chart gives display x: the reference spectrum is drawn shifted by its x offset (Claude Generated)
        const double x = point.x() - m_editor->stackOffset(m_handler.referenceIndex()).x();
        m_position->setValue(x);
        if (m_double_adds->isChecked())
            addSignalAtPeak(x);
    });
    // Claude Generated: labels of the signals in the chart and rows of the signal table select each other
    connect(m_editor, &TraceEditorWidget::labelClicked, this, [this](int index) {
        if (index >= 0 && index < m_signal_table->rowCount())
            m_signal_table->selectRow(index);
    });

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
    connect(m_concentrations, &ConcentrationEditor::Edited, this, [this]() {
        m_guard->markDirty();
        updateTable();
    });
    connect(m_details, &QCheckBox::toggled, this, [this]() { updateResults(); });
    connect(m_spectra, &QListWidget::currentRowChanged, this, [this]() {
        if (!m_updating) {
            updateMarkers();
            updateFidControls();
        }
    });
    updateDoubleClickHint();
    m_guard->removeDefaultButtons();

    // remembered place, splitters and the options of a new series (Claude Generated, UX package 2a)
    m_memory->restoreGeometry();
    m_memory->addSplitter(main, "main");
    m_memory->addSplitter(center, "center");
    restoreOptions();
}

/*! \brief Panel of the FID processing (Claude Generated, JEOL import). */
QWidget* NmrImport::fidPanel()
{
    m_fid_box = new QGroupBox(tr("Processing (FID)"));
    m_fid_box->setObjectName("nmrFidPanel");
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(4, 4, 4, 4);
    QFormLayout* form = new QFormLayout;
    form->setContentsMargins(0, 0, 0, 0);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);

    m_line_broadening = new QDoubleSpinBox;
    m_line_broadening->setObjectName("nmrLineBroadening");
    m_line_broadening->setDecimals(2);
    m_line_broadening->setRange(0, 50);
    m_line_broadening->setSingleStep(0.1);
    m_line_broadening->setSuffix(tr(" Hz"));
    m_line_broadening->setKeyboardTracking(false);
    m_line_broadening->setToolTip(tr("Exponential apodization: every line is broadened by this width (lower noise, wider lines)."));
    form->addRow(tr("LB"), m_line_broadening);

    m_zero_fill = new QComboBox;
    m_zero_fill->setObjectName("nmrZeroFill");
    for (int factor : { 1, 2, 4, 8 })
        m_zero_fill->addItem(tr("× %1").arg(factor), factor);
    m_zero_fill->setToolTip(tr("Zero filling: the spectrum has the next power of two of this many times the points of the FID."));
    form->addRow(tr("Zero fill"), m_zero_fill);

    m_ph0 = new QDoubleSpinBox;
    m_ph0->setObjectName("nmrPh0");
    m_ph0->setDecimals(1);
    m_ph0->setRange(-360, 360);
    m_ph0->setSingleStep(1);
    m_ph0->setSuffix(tr(" °"));
    m_ph0->setKeyboardTracking(false);
    m_ph0->setToolTip(tr("Zero-order phase of all spectra."));
    form->addRow(tr("ph0"), m_ph0);

    m_ph1 = new QDoubleSpinBox;
    m_ph1->setObjectName("nmrPh1");
    m_ph1->setDecimals(1);
    m_ph1->setRange(-3600, 3600);
    m_ph1->setSingleStep(1);
    m_ph1->setSuffix(tr(" °"));
    m_ph1->setKeyboardTracking(false);
    m_ph1->setToolTip(tr("First-order phase across the spectral width, zero at the centre of the spectrum (after the correction of the digital filter delay)."));
    form->addRow(tr("ph1"), m_ph1);
    layout->addLayout(form);

    m_auto_phase = new QPushButton(tr("Auto phase"));
    m_auto_phase->setObjectName("nmrAutoPhase");
    m_auto_phase->setToolTip(tr("Finds ph0 and ph1 of the reference spectrum (entropy minimisation) and uses them for all spectra."));
    layout->addWidget(m_auto_phase);
    m_auto_each = new QCheckBox(tr("Auto phase each spectrum"));
    m_auto_each->setObjectName("nmrAutoPhaseEach");
    m_auto_each->setToolTip(tr("Every spectrum gets its own phase by entropy minimisation instead of the common ph0 and ph1."));
    layout->addWidget(m_auto_each);
    m_fid_info = new QLabel;
    m_fid_info->setObjectName("nmrFidInfo");
    m_fid_info->setWordWrap(true);
    layout->addWidget(m_fid_info);
    m_fid_box->setLayout(layout);
    m_fid_box->setVisible(false);

    // a held arrow key changes LB/ph0/ph1 many times: transform once after the last change if transforms are slow
    m_fid_timer = new QTimer(this);
    m_fid_timer->setSingleShot(true);
    m_fid_timer->setInterval(300);
    connect(m_fid_timer, &QTimer::timeout, this, &NmrImport::applyFidSettings);
    connect(m_line_broadening, &QDoubleSpinBox::valueChanged, this, &NmrImport::requestFidSettings);
    connect(m_zero_fill, &QComboBox::currentIndexChanged, this, &NmrImport::applyFidSettings);
    connect(m_ph0, &QDoubleSpinBox::valueChanged, this, &NmrImport::requestFidSettings);
    connect(m_ph1, &QDoubleSpinBox::valueChanged, this, &NmrImport::requestFidSettings);
    connect(m_auto_each, &QCheckBox::toggled, this, &NmrImport::applyFidSettings);
    connect(m_auto_phase, &QPushButton::clicked, this, [this]() {
        AdaptiveWaiter wait(&m_fid_ms, true); // entropy minimisation of the reference spectrum: always slow enough
        QString error;
        if (!m_handler.autoPhaseFromReference(&error))
            setStatus(error, true);
        else
            setStatus(tr("Phase of spectrum %1: ph0 %2°, ph1 %3°.").arg(m_handler.referenceIndex() + 1).arg(m_handler.fidSettings().ph0, 0, 'f', 1).arg(m_handler.fidSettings().ph1, 0, 'f', 1), false);
        refresh();
    });
    return m_fid_box;
}

void NmrImport::updateFidControls()
{
    const bool updating = m_updating;
    m_updating = true;
    const FidSettings& fid = m_handler.fidSettings();
    m_fid_box->setVisible(m_handler.hasFids());
    m_line_broadening->setValue(fid.lineBroadening);
    m_zero_fill->setCurrentIndex(std::max(0, m_zero_fill->findData(fid.zeroFill)));
    m_ph0->setValue(fid.ph0);
    m_ph1->setValue(fid.ph1);
    m_auto_each->setChecked(fid.autoPhase);
    m_ph0->setEnabled(!fid.autoPhase);
    m_ph1->setEnabled(!fid.autoPhase);
    m_auto_phase->setEnabled(!fid.autoPhase);
    const int row = m_spectra->currentRow();
    const int k = m_handler.isFid(row) ? row : m_handler.referenceIndex();
    if (m_handler.isFid(k)) {
        const FidPhase phase = m_handler.fidPhase(k);
        m_fid_info->setText(tr("%1\nSpectrum %2: ph0 %3°, ph1 %4°").arg(m_handler.fidDescription(k)).arg(k + 1).arg(phase.ph0, 0, 'f', 1).arg(phase.ph1, 0, 'f', 1));
    } else
        m_fid_info->clear();
    m_updating = updating;
}

void NmrImport::requestFidSettings()
{
    if (m_updating)
        return;
    if (m_fid_ms <= 300)
        applyFidSettings();
    else
        m_fid_timer->start(); // restarted by every further change
}

NmrHandler* NmrImport::handler()
{
    flushFidSettings();
    return &m_handler;
}

void NmrImport::flushFidSettings()
{
    if (m_fid_timer->isActive()) {
        m_fid_timer->stop();
        applyFidSettings();
    }
}

void NmrImport::applyFidSettings()
{
    m_fid_timer->stop();
    if (m_updating)
        return;
    FidSettings fid = m_handler.fidSettings();
    fid.lineBroadening = m_line_broadening->value();
    fid.zeroFill = m_zero_fill->currentData().toInt();
    fid.ph0 = m_ph0->value();
    fid.ph1 = m_ph1->value();
    fid.autoPhase = m_auto_each->isChecked();
    m_memory->setValue("lineBroadening", fid.lineBroadening); // ph0/ph1 belong to the series: not kept
    m_memory->setValue("zeroFill", m_zero_fill->currentIndex());
    AdaptiveWaiter wait(&m_fid_ms); // every FID is transformed again
    QString error;
    if (!m_handler.setFidSettings(fid, &error))
        setStatus(error, true);
    refresh();
}

QWidget* NmrImport::signalPanel()
{
    QGroupBox* box = new QGroupBox(tr("Signals"));
    QVBoxLayout* layout = new QVBoxLayout;
    QFormLayout* form = new QFormLayout;

    m_offset = new QDoubleSpinBox;
    m_offset->setObjectName("nmrReferenceOffset");
    m_offset->setDecimals(4);
    m_offset->setRange(-100, 100);
    m_offset->setSingleStep(0.001);
    m_offset->setSuffix(tr(" ppm"));
    m_offset->setKeyboardTracking(false);
    m_offset->setToolTip(tr("Added to the ppm axis of every spectrum (referencing); the signals move along."));
    form->addRow(tr("Reference offset"), m_offset);

    m_window = new QDoubleSpinBox;
    m_window->setObjectName("nmrSearchWindow");
    m_window->setDecimals(4);
    m_window->setRange(0.0005, 5);
    m_window->setSingleStep(0.005);
    m_window->setValue(m_handler.defaultWindow());
    m_window->setSuffix(tr(" ppm"));
    m_window->setKeyboardTracking(false);
    m_window->setToolTip(tr("Window of new signals: a signal is searched within this distance of its last position and of the position expected from its last two steps."));
    form->addRow(tr("Search window ±"), m_window);

    // Claude Generated: what the detection threshold of the default peak picking refers to
    m_threshold = new QComboBox;
    m_threshold->setObjectName("nmrThresholdScope");
    m_threshold->addItems({ tr("whole spectrum"), tr("search window") });
    m_threshold->setToolTip(tr("Relative to the whole spectrum, a peak counts above 5 % of its intensity range, so a much stronger line can hide small signals; relative to the search window, a peak counts if it stands out of the noise and of the other peaks in the window of its signal (without a peak picking step)."));
    form->addRow(tr("Detection threshold"), m_threshold);

    m_method = new QComboBox;
    m_method->setObjectName("nmrShiftMethod");
    m_method->addItems({ tr("Peak picking"), tr("Lineshape fit") });
    m_method->setToolTip(tr("Peak picking: δ is the position of the maximum. Lineshape fit: δ is the centre of a Gauss-Lorentz (pseudo-Voigt) line fitted to the whole signal, also where lines overlap."));
    form->addRow(tr("Shift from"), m_method);
    layout->addLayout(form);

    // the options of the lineshape fit behind "..more.."; they open by themselves for a lineshape fit
    // (Claude Generated, UX package 2a)
    m_fit_options = new QWidget;
    m_fit_options->setObjectName("nmrFitOptions");
    m_fit_toggle = new MoreLessToggle(m_fit_options);
    m_fit_toggle->setObjectName("nmrFitOptionsToggle");
    m_fit_toggle->setToolTip(tr("Show or hide the options of the lineshape fit"));
    QHBoxLayout* toggle_row = new QHBoxLayout;
    toggle_row->setContentsMargins(0, 0, 0, 0);
    QLabel* toggle_label = new QLabel(tr("Lineshape fit options"));
    toggle_label->setToolTip(m_fit_toggle->toolTip());
    toggle_row->addWidget(toggle_label);
    toggle_row->addStretch(1);
    toggle_row->addWidget(m_fit_toggle);
    layout->addLayout(toggle_row);
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
    m_pick->setObjectName("nmrPick");
    m_pick->setToolTip(tr("One signal per peak of the reference spectrum between Start and End in the visible part of the chart (peaks of a peak picking step, else by the detection threshold), or one multiplet per group of lines with \"Group lines into multiplets\"; replaces the signals and the values set by hand. The signals are numbered in the chart as in the list."));
    layout->addWidget(m_pick);
    layout->addWidget(multipletPanel());

    QHBoxLayout* add = new QHBoxLayout;
    m_position = new QDoubleSpinBox;
    m_position->setObjectName("nmrPosition");
    m_position->setDecimals(4);
    m_position->setRange(-1000, 1000);
    m_position->setSuffix(tr(" ppm"));
    m_position->setToolTip(tr("Position of a new signal (taken as typed); a double-click in the chart sets it."));
    m_add = new QPushButton(tr("Add"));
    m_add->setToolTip(tr("Adds a signal at this position with the search window."));
    add->addWidget(m_position, 1);
    add->addWidget(m_add);
    layout->addLayout(add);
    m_add_multiplet = new QPushButton(tr("Multiplet from range"));
    m_add_multiplet->setObjectName("nmrAddMultiplet");
    m_add_multiplet->setToolTip(tr("Adds a multiplet for the range Start..End (drag with the right mouse button over its lines): its centre is followed instead of a single line; half span = half the range."));
    layout->addWidget(m_add_multiplet);

    // Claude Generated: type, centre, window, half span and pattern of every signal
    m_signal_table = new QTableWidget(0, 6);
    m_signal_table->setObjectName("nmrSignals");
    m_signal_table->setHorizontalHeaderLabels({ tr("Name"), tr("Type"), tr("δ /ppm"), tr("±"), tr("½ span"), tr("Pattern") });
    m_signal_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_signal_table->horizontalHeader()->setStretchLastSection(true); // the pattern takes what is left
    m_signal_table->horizontalHeader()->setMinimumSectionSize(24);
    m_signal_table->verticalHeader()->setDefaultSectionSize(22);
    m_signal_table->setToolTip(tr("Untick a signal to leave it out of the table; name, type, position (the centre of a multiplet), window, half span and pattern can be edited. The row number is the label of the signal in the chart; selecting a row highlights it."));
    m_signal_table->horizontalHeaderItem(1)->setToolTip(tr("Peak: one line is followed. Multiplet: the group of lines within ± half span of its centre is followed, δ is the centre."));
    m_signal_table->horizontalHeaderItem(4)->setToolTip(tr("Multiplet: the lines lie within ± this distance of the centre [ppm]."));
    m_signal_table->horizontalHeaderItem(5)->setToolTip(tr("Multiplet: expected pattern (s, d, t, q, dd, dt, ..., m); a different number of lines found is shown as a hint."));
    layout->addWidget(m_signal_table, 1);
    m_remove_signal = new QPushButton(tr("Remove signal"));
    m_remove_signal->setObjectName("nmrRemoveSignal");
    layout->addWidget(m_remove_signal);
    m_double_adds = new QCheckBox(tr("Double-click adds the signal"));
    m_double_adds->setObjectName("nmrDoubleClickAdds");
    m_double_adds->setToolTip(tr("A double-click in the chart adds a signal at the peak the tracking finds near the click in the reference spectrum (at the click itself if there is none). Off: the double-click only sets the position, \"Add\" adds the signal there."));
    m_double_adds->setChecked(QSettings().value("nmrimport/doubleClickAdds", true).toBool());
    layout->addWidget(m_double_adds);
    box->setLayout(layout);
    box->setMinimumWidth(400); // the six columns of the signal table without a scroll bar
    box->setMaximumWidth(460);

    connect(m_offset, &QDoubleSpinBox::valueChanged, this, [this](double offset) {
        if (m_updating)
            return;
        m_handler.setReferenceOffset(offset);
        refresh();
    });
    connect(m_window, &QDoubleSpinBox::valueChanged, this, [this](double window) {
        m_handler.setDefaultWindow(window);
        if (!m_updating) {
            m_guard->markDirty();
            m_memory->setValue("searchWindow", window);
        }
    });
    connect(m_double_adds, &QCheckBox::toggled, this, [this](bool on) {
        QSettings().setValue("nmrimport/doubleClickAdds", on);
        updateDoubleClickHint();
    });
    connect(m_threshold, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_updating)
            return;
        m_handler.setThresholdScope(index == 1 ? NmrThresholdScope::Window : NmrThresholdScope::Spectrum);
        m_memory->setValue("thresholdScope", index); // default of the next new series
        retrack();
    });
    connect(m_method, &QComboBox::currentIndexChanged, this, &NmrImport::applyFitSettings);
    connect(m_fit_toggle, &QToolButton::clicked, this, [this]() { m_memory->setValue("fitOptionsExpanded", m_fit_toggle->isExpanded()); });
    connect(m_width_mode, &QComboBox::currentIndexChanged, this, &NmrImport::applyFitSettings);
    connect(m_baseline, &QComboBox::currentIndexChanged, this, &NmrImport::applyFitSettings);
    connect(m_fit_range, &QDoubleSpinBox::valueChanged, this, &NmrImport::applyFitSettings);
    connect(m_max_iterations, &QSpinBox::valueChanged, this, &NmrImport::applyFitSettings);
    connect(m_fallback, &QCheckBox::toggled, this, &NmrImport::applyFitSettings);
    updateFitControls();
    connect(m_pick, &QPushButton::clicked, this, [this]() {
        const int signals_now = m_handler.signalList().size();
        const int manual = manualCount();
        if (signals_now > 1 || manual > 0) {
            const QString question = manual > 0 ? tr("Replace %1 signal(s) and discard %2 hand-set value(s)?").arg(signals_now).arg(manual)
                                                : tr("Replace %1 signals?").arg(signals_now);
            if (!m_guard->confirm(question, tr("Replace")))
                return;
        }
        QString error;
        double from = 0, to = 0;
        pickRange(&from, &to);
        const bool group = m_group->isChecked();
        const int n = group ? m_handler.pickMultiplets(m_window->value(), maxCouplingPpm(), m_ripple->value() / 100.0, &error, from, to)
                            : m_handler.pickSignals(m_window->value(), &error, from, to);
        if (!error.isEmpty())
            setStatus(error, true);
        else if (group)
            setStatus(tr("%1 multiplet(s) picked in spectrum %2 between %3 and %4 ppm (J ≤ %5 Hz).").arg(n).arg(m_handler.referenceIndex() + 1).arg(from, 0, 'f', 3).arg(to, 0, 'f', 3).arg(m_max_coupling->value(), 0, 'f', 1), false);
        else
            setStatus(tr("%1 signal(s) picked in spectrum %2 between %3 and %4 ppm.").arg(n).arg(m_handler.referenceIndex() + 1).arg(from, 0, 'f', 3).arg(to, 0, 'f', 3), false);
        retrack();
    });
    connect(m_signal_table, &QTableWidget::currentCellChanged, this, [this]() {
        if (!m_updating)
            showSelectedSignal();
    });
    connect(m_add, &QPushButton::clicked, this, [this]() { addSignalAt(m_position->value()); });
    connect(m_add_multiplet, &QPushButton::clicked, this, [this]() { addMultipletFromRange(); });
    connect(m_remove_signal, &QPushButton::clicked, this, [this]() {
        const int row = m_signal_table->currentRow();
        if (row < 0 || row >= m_handler.signalList().size())
            return;
        const int manual = manualCount(-1, row);
        if (manual > 0 && !m_guard->confirm(tr("Remove signal %1 and discard %2 hand-set value(s)?").arg(m_handler.signalList()[row].name).arg(manual), tr("Remove")))
            return;
        if (m_handler.removeSignal(row))
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
        } else if (item->column() == 5) {
            signal.pattern = item->text().trimmed();
        } else if (parseShift(item->text(), &value)) {
            if (item->column() == 2)
                signal.position = value;
            else if (item->column() == 3 && value > 0)
                signal.window = value;
            else if (item->column() == 4 && value >= 0)
                signal.halfSpan = value;
        }
        if (!m_handler.setSignal(row, signal))
            setStatus(tr("Signal %1 not changed: the window must be positive.").arg(row + 1), true);
        QMetaObject::invokeMethod(this, [this]() { retrack(); }, Qt::QueuedConnection);
    });
    return box;
}

/*! \brief Options of the multiplets behind "..more.." (Claude Generated): grouping of the pick by the largest coupling J
 * in Hz (converted with the spectrometer frequency, from the FIDs or typed), ripple threshold, centre estimator. */
QWidget* NmrImport::multipletPanel()
{
    QWidget* panel = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    m_multiplet_options = new QWidget;
    m_multiplet_options->setObjectName("nmrMultipletOptions");
    m_multiplet_toggle = new MoreLessToggle(m_multiplet_options);
    m_multiplet_toggle->setObjectName("nmrMultipletOptionsToggle");
    m_multiplet_toggle->setToolTip(tr("Show or hide the options of the multiplets"));
    m_group = new QCheckBox(tr("Group lines into multiplets"));
    m_group->setObjectName("nmrGroupMultiplets");
    m_group->setToolTip(tr("\"Pick from reference spectrum\" makes one multiplet of the lines that lie closer together than the largest coupling J; weak ripple beside the lines and maxima below the noise threshold are left out."));
    QHBoxLayout* toggle_row = new QHBoxLayout;
    toggle_row->setContentsMargins(0, 0, 0, 0);
    toggle_row->addWidget(m_group);
    toggle_row->addStretch(1);
    toggle_row->addWidget(m_multiplet_toggle);
    layout->addLayout(toggle_row);

    QFormLayout* form = new QFormLayout;
    form->setContentsMargins(0, 0, 0, 0);
    m_max_coupling = new QDoubleSpinBox;
    m_max_coupling->setObjectName("nmrMaxCoupling");
    m_max_coupling->setDecimals(1);
    m_max_coupling->setRange(0.5, 100);
    m_max_coupling->setValue(20);
    m_max_coupling->setSuffix(tr(" Hz"));
    m_max_coupling->setKeyboardTracking(false);
    m_max_coupling->setToolTip(tr("Lines closer than this coupling form one multiplet."));
    form->addRow(tr("J ≤"), m_max_coupling);
    m_frequency = new QDoubleSpinBox;
    m_frequency->setObjectName("nmrSpectrometerFrequency");
    m_frequency->setDecimals(2);
    m_frequency->setRange(10, 2000);
    m_frequency->setValue(400);
    m_frequency->setSuffix(tr(" MHz"));
    m_frequency->setKeyboardTracking(false);
    m_frequency->setToolTip(tr("Frequency of the observed nucleus, converts J from Hz to ppm; taken from the FIDs when the series has them."));
    form->addRow(tr("Spectrometer"), m_frequency);
    m_ripple = new QDoubleSpinBox;
    m_ripple->setObjectName("nmrMultipletRipple");
    m_ripple->setDecimals(1);
    m_ripple->setRange(0, 50);
    m_ripple->setValue(100 * m_handler.multipletRipple());
    m_ripple->setSuffix(tr(" %"));
    m_ripple->setKeyboardTracking(false);
    m_ripple->setToolTip(tr("A maximum lower than this share of the highest line of its multiplet (as height above its surroundings) is ripple, not a line."));
    form->addRow(tr("Ripple below"), m_ripple);
    m_centre = new QComboBox;
    m_centre->setObjectName("nmrMultipletCentre");
    m_centre->addItems({ tr("Centroid of the area"), tr("Midpoint of the outer lines"), tr("Height-weighted mean of the lines") });
    m_centre->setCurrentIndex(int(m_handler.multipletCentre())); // the default of the core
    m_centre->setToolTip(tr("Centre of a multiplet: centroid of its area above the baseline, midpoint of its outermost lines, or mean of its line positions weighted by their heights. The lineshape fit uses the area-weighted mean of the fitted lines."));
    form->addRow(tr("Centre"), m_centre);
    m_multiplet_options->setLayout(form);
    layout->addWidget(m_multiplet_options);
    panel->setLayout(layout);

    connect(m_multiplet_toggle, &QToolButton::clicked, this, [this]() { m_memory->setValue("multipletOptionsExpanded", m_multiplet_toggle->isExpanded()); });
    connect(m_group, &QCheckBox::toggled, this, [this](bool on) {
        if (!m_updating)
            m_memory->setValue("groupMultiplets", on);
    });
    connect(m_max_coupling, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        if (!m_updating)
            m_memory->setValue("maxCoupling", value);
    });
    connect(m_frequency, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        if (!m_updating && !m_handler.hasFids())
            m_memory->setValue("spectrometerFrequency", value);
    });
    connect(m_ripple, &QDoubleSpinBox::valueChanged, this, &NmrImport::applyMultipletSettings);
    connect(m_centre, &QComboBox::currentIndexChanged, this, &NmrImport::applyMultipletSettings);
    return panel;
}

void NmrImport::applyMultipletSettings()
{
    if (m_updating)
        return;
    m_handler.setMultipletRipple(m_ripple->value() / 100.0);
    m_handler.setMultipletCentre(NmrMultipletCentre(std::clamp(m_centre->currentIndex(), 0, 2)));
    m_memory->setValue("multipletRipple", m_ripple->value());
    m_memory->setValue("multipletCentre", m_centre->currentIndex());
    if (m_handler.hasMultiplets())
        retrack();
}

double NmrImport::maxCouplingPpm() const
{
    return m_max_coupling->value() / m_frequency->value();
}

bool NmrImport::addMultipletFromRange()
{
    double from = 0, to = 0;
    pickRange(&from, &to);
    if (!(to - from <= kMaxMultipletRange)) {
        setStatus(tr("The range %1..%2 ppm is wider than %3 ppm: drag with the right mouse button over the lines of one multiplet.").arg(from, 0, 'f', 4).arg(to, 0, 'f', 4).arg(kMaxMultipletRange), true);
        return false;
    }
    bool found = false;
    const int index = m_handler.addMultiplet(from, to, m_window->value(), &found);
    retrack();
    if (index < 0 || index >= m_handler.signalList().size())
        return false;
    const NmrSignal& signal = m_handler.signalList()[index];
    const NmrCell cell = m_handler.cell(m_handler.referenceIndex(), index);
    if (found)
        setStatus(tr("Multiplet %1 added: centre %2 ppm, %3 line(s), half span %4 ppm.").arg(signal.name).arg(signal.position, 0, 'f', 4).arg(cell.lines.size()).arg(signal.halfSpan, 0, 'f', 4), false);
    else
        setStatus(tr("Multiplet %1 added at the middle of %2..%3 ppm: no line found there.").arg(signal.name).arg(from, 0, 'f', 4).arg(to, 0, 'f', 4), false);
    m_signal_table->selectRow(index);
    return true;
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
    if (fit.method == NmrShiftMethod::LineshapeFit)
        m_fit_toggle->setExpanded(true); // the options of the chosen method in sight
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
    // defaults of the next new series (a project brings its own)
    m_memory->setValue("shiftFrom", m_method->currentIndex());
    m_memory->setValue("lineWidth", m_width_mode->currentIndex());
    m_memory->setValue("baseline", m_baseline->currentIndex());
    m_memory->setValue("fitRange", m_fit_range->value());
    m_memory->setValue("maxIterations", m_max_iterations->value());
    m_memory->setValue("peakFallback", m_fallback->isChecked());
    updateFitControls();
    retrack();
}

int NmrImport::fitSpectrum() const
{
    const int row = m_spectra->currentRow();
    return row >= 0 && row < m_handler.spectrumCount() ? row : m_handler.referenceIndex();
}

bool NmrImport::addFiles(const QStringList& files, QString* error, const QList<SeriesSourceBar::Failure>& before)
{
    QList<SeriesSourceBar::Failure> failed = before;
    QStringList reasons;
    for (const SeriesSourceBar::Failure& failure : before)
        reasons << failure.reason;
    int read = 0;
    QElapsedTimer timer; // a FID series: reading is about the time of a transform of the series (debounce of LB/ph0/ph1)
    timer.start();
    const QVector<QString> problems = m_handler.addFiles(files); // in parallel, appended in this order
    for (int i = 0; i < files.size(); ++i) {
        if (problems.value(i).isEmpty())
            ++read;
        else {
            failed.append({ files[i], problems[i] });
            reasons << problems[i];
        }
    }
    if (m_handler.hasFids() && read > 0)
        m_fid_ms = std::max(m_fid_ms, timer.elapsed());
    if (!files.isEmpty())
        m_source->setPath(files.size() == 1 ? files.first() : QFileInfo(files.value(0)).path());
    m_source->report(read, failed);
    if (error && !reasons.isEmpty())
        *error = reasons.join("\n");
    m_editor->setOperations(m_handler.operations());
    refresh();
    return failed.isEmpty();
}

bool NmrImport::setDirectory(const QString& directory, const QString& suffix, QString* error)
{
    m_source->setPath(directory);
    QString reason;
    if (!QFileInfo(directory).isDir())
        reason = tr("Directory %1 does not exist.").arg(directory);
    const QStringList files = reason.isEmpty() ? SeriesSourceBar::filesInDirectory(directory, suffix, SeriesSourceBar::Match::EndsWith) : QStringList();
    if (reason.isEmpty() && files.isEmpty())
        reason = suffix.isEmpty() ? tr("No file in %1.").arg(directory) : tr("No file ending with %1 in %2.").arg(suffix, directory);
    if (!reason.isEmpty()) {
        m_source->reportMessage(reason, StatusLine::Error);
        if (error)
            *error = reason;
        refresh();
        return false;
    }
    addFiles(files, error);
    return m_handler.spectrumCount() > 0;
}

/*! \brief Defaults of a new series from the memory: search window, "Shift from" and the lineshape fit
 * options, LB and zero fill of the FIDs; a stored value outside the limits of its control is ignored.
 * Runs before a project is read (setData), whose values then win. Claude Generated (UX package 2a) */
void NmrImport::restoreOptions()
{
    m_updating = true;
    if (m_memory->restore(m_window, "searchWindow"))
        m_handler.setDefaultWindow(m_window->value());
    // a new series looks for peaks relative to the search window unless another choice is remembered
    int scope = 1;
    m_memory->readInt("thresholdScope", 0, m_threshold->count() - 1, &scope);
    m_threshold->setCurrentIndex(scope);
    m_handler.setThresholdScope(scope == 1 ? NmrThresholdScope::Window : NmrThresholdScope::Spectrum);
    m_memory->restore(m_method, "shiftFrom");
    m_memory->restore(m_width_mode, "lineWidth");
    m_memory->restore(m_baseline, "baseline");
    m_memory->restore(m_fit_range, "fitRange");
    m_memory->restore(m_max_iterations, "maxIterations");
    m_memory->restore(m_fallback, "peakFallback");
    NmrFitSettings fit;
    fit.method = m_method->currentIndex() == 1 ? NmrShiftMethod::LineshapeFit : NmrShiftMethod::PeakPicking;
    fit.widthMode = m_width_mode->currentIndex() == 1 ? NmrWidthMode::Shared : NmrWidthMode::Free;
    fit.baseline = LineshapeBaseline(std::clamp(m_baseline->currentIndex(), 0, 2));
    fit.range = m_fit_range->value();
    fit.maxIterations = m_max_iterations->value();
    fit.peakFallback = m_fallback->isChecked();
    m_handler.setFitSettings(fit);

    FidSettings fid = m_handler.fidSettings();
    double lb = 0;
    if (m_memory->readDouble("lineBroadening", m_line_broadening->minimum(), m_line_broadening->maximum(), &lb))
        fid.lineBroadening = lb;
    int zero = 0;
    if (m_memory->readInt("zeroFill", 0, m_zero_fill->count() - 1, &zero))
        fid.zeroFill = m_zero_fill->itemData(zero).toInt();
    m_handler.setFidSettings(fid); // no FIDs yet: nothing is transformed
    bool expanded = false;
    if (m_memory->readBool("fitOptionsExpanded", &expanded))
        m_fit_toggle->setExpanded(expanded, false);
    // Claude Generated: multiplet options
    bool group = false;
    if (m_memory->readBool("groupMultiplets", &group))
        m_group->setChecked(group);
    m_memory->restore(m_max_coupling, "maxCoupling");
    m_memory->restore(m_frequency, "spectrometerFrequency");
    m_memory->restore(m_ripple, "multipletRipple");
    m_memory->restore(m_centre, "multipletCentre");
    m_handler.setMultipletRipple(m_ripple->value() / 100.0);
    m_handler.setMultipletCentre(NmrMultipletCentre(std::clamp(m_centre->currentIndex(), 0, 2)));
    bool multipletExpanded = false;
    if (m_memory->readBool("multipletOptionsExpanded", &multipletExpanded))
        m_multiplet_toggle->setExpanded(multipletExpanded, false);
    m_updating = false;
    updateFitControls();
    updateFidControls();
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
    m_threshold->setCurrentIndex(m_handler.thresholdScope() == NmrThresholdScope::Window ? 1 : 0); // the project's choice
    if (m_handler.hasMultiplets()) { // Claude Generated: a project with multiplets brings their options
        m_ripple->setValue(100 * m_handler.multipletRipple());
        m_centre->setCurrentIndex(int(m_handler.multipletCentre()));
    } else {
        m_handler.setMultipletRipple(m_ripple->value() / 100.0);
        m_handler.setMultipletCentre(NmrMultipletCentre(std::clamp(m_centre->currentIndex(), 0, 2)));
    }
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
    m_concentrations->setTable(table); // a copy; Edited() builds the table again
    updateTable();
}

void NmrImport::retrack()
{
    AdaptiveWaiter wait(&m_track_ms);
    m_guard->markDirty();
    m_handler.track();
    m_editor->setExcludedTraces(excludedSpectra());
    updateSpectra();
    updateSignals();
    updateResults();
    updateMarkers();
    updateTable();
    m_concentrations->setSpectraCount(m_handler.spectrumCount(), usedSpectra());
}

void NmrImport::refresh()
{
    AdaptiveWaiter wait(&m_track_ms);
    m_guard->markDirty();
    m_handler.track();
    m_editor->setExcludedTraces(excludedSpectra());
    m_editor->setTraces(m_handler.referencedSpectra()); // redraws, the markers follow (chartUpdated); same spectra keep the zoom
    m_updating = true;
    m_offset->setValue(m_handler.referenceOffset()); // also when the handler was changed directly
    m_updating = false;
    updateFidControls();
    m_threshold->setEnabled(!m_handler.hasPickStep()); // a peak picking step brings its own threshold
    // Claude Generated: the frequency of the FIDs converts J (typed only without FIDs)
    const double frequency = m_handler.spectrometerFrequency();
    if (std::isfinite(frequency) && frequency >= m_frequency->minimum() && frequency <= m_frequency->maximum()) {
        m_updating = true;
        m_frequency->setValue(frequency);
        m_updating = false;
    }
    m_frequency->setEnabled(!m_handler.hasFids());
    updateSpectra();
    updateSignals();
    updateResults();
    updateTable();
    m_source->setSpectraCount(m_handler.spectrumCount());
    m_concentrations->setSpectraCount(m_handler.spectrumCount(), usedSpectra());
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
        const NmrSignal& signal = list[s];
        QTableWidgetItem* name = new QTableWidgetItem(signal.name);
        name->setFlags(name->flags() | Qt::ItemIsUserCheckable);
        name->setCheckState(signal.enabled ? Qt::Checked : Qt::Unchecked);
        m_signal_table->setItem(s, 0, name);
        // Claude Generated: the type as a choice; peak <-> multiplet keeps the position
        QTableWidgetItem* type = new QTableWidgetItem;
        type->setFlags(type->flags() & ~Qt::ItemIsEditable);
        m_signal_table->setItem(s, 1, type);
        QComboBox* choice = qobject_cast<QComboBox*>(m_signal_table->cellWidget(s, 1));
        if (!choice) {
            choice = new QComboBox;
            choice->setObjectName("nmrSignalType");
            choice->addItems({ tr("peak"), tr("multiplet") });
            choice->setFrame(false);
            m_signal_table->setCellWidget(s, 1, choice);
            connect(choice, &QComboBox::currentIndexChanged, this, [this, choice](int index) {
                if (m_updating)
                    return;
                int row = -1;
                for (int r = 0; r < m_signal_table->rowCount() && row < 0; ++r)
                    if (m_signal_table->cellWidget(r, 1) == choice)
                        row = r;
                if (row < 0 || row >= m_handler.signalList().size())
                    return;
                NmrSignal changed = m_handler.signalList()[row];
                changed.type = index == 1 ? NmrSignalType::Multiplet : NmrSignalType::Peak;
                if (changed.isMultiplet() && !(changed.halfSpan > 0))
                    changed.halfSpan = 0.5 * maxCouplingPpm(); // a doublet of the largest coupling; edit "½ span" for others
                m_handler.setSignal(row, changed);
                QMetaObject::invokeMethod(this, [this]() { retrack(); }, Qt::QueuedConnection);
            });
        }
        choice->setCurrentIndex(signal.isMultiplet() ? 1 : 0);
        m_signal_table->setItem(s, 2, new QTableWidgetItem(QString::number(signal.position, 'f', 4)));
        m_signal_table->setItem(s, 3, new QTableWidgetItem(QString::number(signal.window, 'f', 4)));
        QTableWidgetItem* span = new QTableWidgetItem(signal.isMultiplet() ? QString::number(signal.halfSpan, 'f', 4) : QString());
        QTableWidgetItem* pattern = new QTableWidgetItem(signal.isMultiplet() ? signal.pattern : QString());
        if (!signal.isMultiplet()) {
            span->setFlags(span->flags() & ~Qt::ItemIsEditable);
            pattern->setFlags(pattern->flags() & ~Qt::ItemIsEditable);
        }
        m_signal_table->setItem(s, 4, span);
        m_signal_table->setItem(s, 5, pattern);
        const QString tip = signalToolTip(s);
        for (int column = 0; column < 6; ++column)
            m_signal_table->item(s, column)->setToolTip(tip);
        choice->setToolTip(tip);
        // a line count that does not fit the pattern: the pattern cell is marked
        const QString warning = m_handler.lineCountWarning(m_handler.referenceIndex(), s);
        if (!warning.isEmpty()) {
            const QColor color = statusColor(NmrCellStatus::Conflict);
            pattern->setBackground(color);
            pattern->setForeground(Appearance::ContrastText(color));
        }
    }
    m_updating = false;
}

/*! \brief Tooltip of a signal row (Claude Generated): its peak, or the centre, lines and hint of a multiplet, in the
 * reference spectrum. */
QString NmrImport::signalToolTip(int s) const
{
    const NmrCell cell = m_handler.cell(m_handler.referenceIndex(), s);
    const bool multiplet = s >= 0 && s < m_handler.signalList().size() && m_handler.signalList()[s].isMultiplet();
    if (!multiplet)
        return std::isfinite(cell.height) ? tr("Signal %1: peak at %2 ppm, height %3 in the reference spectrum").arg(s + 1).arg(cell.shift, 0, 'f', 4).arg(cell.height, 0, 'g', 4)
                                          : tr("Signal %1: no peak in the reference spectrum").arg(s + 1);
    if (cell.status != NmrCellStatus::Ok || cell.lines.isEmpty())
        return tr("Signal %1: multiplet, no lines found in the reference spectrum").arg(s + 1);
    QStringList lines;
    for (double x : cell.lines)
        lines << QString::number(x, 'f', 4);
    QString tip = tr("Signal %1: multiplet, centre %2 ppm, %3 line(s) at %4 ppm in the reference spectrum").arg(s + 1).arg(cell.shift, 0, 'f', 4).arg(cell.lines.size()).arg(lines.join(", "));
    const QString warning = m_handler.lineCountWarning(m_handler.referenceIndex(), s);
    if (!warning.isEmpty())
        tip += "\n" + tr("Hint: %1 (equal couplings or unresolved lines give fewer lines; check the half span).").arg(warning);
    return tip;
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
    QVector<int> missingPerColumn(columns.size(), 0); ///< the cells the legend counts as missing, per signal
    int used = 0; ///< spectra not left out
    QTableWidgetItem* first = nullptr; ///< first cell without a unique shift, scrolled into view
    auto number = [](double v, char format, int precision) { return std::isfinite(v) ? QString::number(v, format, precision) : QString(); };
    for (int k = 0; k < rows; ++k) {
        const bool excluded = m_handler.isSpectrumExcluded(k);
        used += !excluded;
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
            if (!cell.lines.isEmpty())
                tip += tr("\nmultiplet centre of %1 line(s)").arg(cell.lines.size());
            const QString warning = m_handler.lineCountWarning(k, columns[c]);
            if (!warning.isEmpty())
                tip += tr("\nhint: %1").arg(warning);
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
                missingPerColumn[c] += cell.status == NmrCellStatus::Missing;
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

    // a signal missing in every spectrum used: most often a much stronger line sets the threshold of the
    // peak picking (relative to the spectrum); say what helps (Claude Generated, Viewer 2)
    QStringList lost;
    QList<int> lost_columns;
    for (int c = 0; c < columns.size(); ++c) {
        if (used > 0 && missingPerColumn[c] == used) {
            lost << m_handler.signalList()[columns[c]].name;
            lost_columns << c;
        }
    }
    // Claude Generated: the remedy depends on the threshold; relative to the search window a stronger line
    // elsewhere no longer hides the signal, so only the window or the range is left
    QString hint;
    if (!lost.isEmpty()) {
        if (m_handler.usesWindowThreshold())
            hint = tr("%1: no peak in the window in any spectrum: check the position, widen the search window or change the range.").arg(lost.join(", "));
        else if (m_handler.hasPickStep())
            hint = tr("%1: no peak in the window in any spectrum; a much stronger signal (e.g. a standard) raises the detection threshold: add a Range or Blank region step.").arg(lost.join(", "));
        else
            hint = tr("%1: no peak in the window in any spectrum; a much stronger line (e.g. a standard) hides it: set \"Detection threshold\" to \"search window\" or add a Range or Blank region step.").arg(lost.join(", "));
    }
    for (int c : lost_columns)
        for (int k = 0; k < rows; ++k)
            if (QTableWidgetItem* item = m_results->item(k, c * per))
                item->setToolTip(item->toolTip() + "\n" + hint);
    m_missing_hint = hint;
    if (hint.isEmpty())
        m_hint->clearMessage();
    else
        m_hint->setMessage(hint, StatusLine::Warning);
    m_hint->setVisible(!hint.isEmpty());
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
    // Claude Generated: the series of the last call take the new points when their number fits (a new series costs
    // the chart about 8 ms); else they are made again
    auto drop = [chart](auto& list) {
        for (const auto& series : list) {
            if (series) {
                chart->removeSeries(series);
                delete series;
            }
        }
        list.clear();
    };
    chart->privateView()->removeAllVerticalLines();
    if (m_handler.spectrumCount() == 0) {
        drop(m_markers);
        drop(m_fit_lines);
        m_editor->setLabels(-1, {});
        return;
    }
    const TraceSet shown = m_editor->showProcessed() ? m_handler.processedSpectra() : m_handler.referencedSpectra();
    const QVector<int> columns = m_handler.enabledSignals();
    bool keep = m_markers.size() == columns.size();
    for (const QPointer<ScatterSeries>& series : m_markers)
        keep = keep && series;
    if (!keep)
        drop(m_markers);
    for (int index = 0; index < columns.size(); ++index) {
        const int c = columns[index];
        QList<QPointF> points;
        for (int k = 0; k < shown.size(); ++k) {
            const NmrCell cell = m_handler.cell(k, c);
            if (!std::isfinite(cell.shift) || m_handler.isSpectrumExcluded(k))
                continue;
            const double y = shown.at(k).interpolate(cell.shift);
            if (std::isfinite(y)) // drawn where the editor draws spectrum k (stack offset, intensity factor)
                points.append(m_editor->displayPoint(k, cell.shift, y));
        }
        ScatterSeries* markers = keep ? m_markers[index].data() : new ScatterSeries;
        if (keep)
            markers->replace(points);
        else {
            markers->append(points);
            markers->setMarkerSize(7);
            chart->addSeries(markers, false);
            m_markers << markers;
        }
        markers->setColor(palette().color(QPalette::Text));
        // where the reference spectrum is drawn (its x offset)
        chart->privateView()->addVerticalLine(m_handler.signalList()[c].position + m_editor->stackOffset(m_handler.referenceIndex()).x());
    }
    // numbered labels at the peaks of the signals in the reference spectrum, numbers = rows of the signal table
    // (Claude Generated)
    QVector<TraceEditorWidget::Label> labels;
    const int ref = m_handler.referenceIndex();
    for (int s = 0; s < m_handler.signalList().size(); ++s) {
        const NmrSignal& signal = m_handler.signalList()[s];
        const NmrCell cell = m_handler.cell(ref, s);
        TraceEditorWidget::Label label;
        label.x = std::isfinite(cell.shift) ? cell.shift : signal.position;
        label.y = ref < shown.size() ? shown.at(ref).interpolate(label.x) : 0;
        for (double line : cell.lines) // Claude Generated: a multiplet's label above its highest line
            if (ref < shown.size() && std::isfinite(shown.at(ref).interpolate(line)))
                label.y = std::max(label.y, shown.at(ref).interpolate(line));
        if (!std::isfinite(label.y))
            label.y = 0;
        label.text = QString::number(s + 1);
        label.dimmed = !signal.enabled;
        label.ticks = cell.lines; // Claude Generated: the lines of a multiplet under its label
        label.toolTip = signal.isMultiplet() ? signalToolTip(s) : std::isfinite(cell.height)
            ? tr("%1 (signal %2): %3 ppm, height %4 in spectrum %5").arg(signal.name).arg(s + 1).arg(label.x, 0, 'f', 4).arg(cell.height, 0, 'g', 4).arg(ref + 1)
            : tr("%1 (signal %2): %3 ppm, no peak in spectrum %4").arg(signal.name).arg(s + 1).arg(label.x, 0, 'f', 4).arg(ref + 1);
        labels << label;
    }
    m_editor->setLabels(ref, labels);
    m_editor->setHighlightedLabel(m_signal_table->currentRow());
    // the fitted model (dashed) and its residual (thin, 10 % of the spectrum height below zero)
    const int k = fitSpectrum();
    m_editor->setHighlightedTrace(m_spectra->currentRow() >= 0 ? k : -1); // the spectrum whose fit is drawn
    const QVector<NmrFitRegion> regions = m_handler.fitRegions(k);
    int curves = 0; ///< fit curve + residual per drawn region
    if (k < shown.size() && !shown.at(k).isEmpty())
        for (const NmrFitRegion& region : regions)
            curves += region.block.x.size() > 0 ? 2 : 0;
    bool keepLines = m_fit_lines.size() == curves;
    for (const QPointer<LineSeries>& series : m_fit_lines)
        keepLines = keepLines && series;
    if (!keepLines)
        drop(m_fit_lines);
    int line = 0;
    if (curves > 0) {
        // with the stack offset and intensity factor of spectrum k; the residual line keeps its place 10 % of the
        // measured height below the baseline (in the frame of the y axis), only its amplitude is scaled
        const double offset = -0.1 * shown.at(k).y().cwiseAbs().maxCoeff();
        const QColor model = palette().color(QPalette::Text); // dashed: distinct from the coloured spectra
        QColor residual = palette().color(QPalette::Text);
        residual.setAlpha(140);
        for (const NmrFitRegion& region : regions) {
            if (region.block.x.size() == 0)
                continue;
            QList<QPointF> fitted, residuals;
            for (Eigen::Index i = 0; i < region.block.x.size(); ++i) {
                const double x = region.block.x(i);
                fitted.append(m_editor->displayPoint(k, x, region.block.evaluate(x)));
                residuals.append(m_editor->displayPoint(k, x, region.block.residuals(i)) + QPointF(0, offset));
            }
            if (keepLines) {
                m_fit_lines[line++]->replace(fitted);
                m_fit_lines[line++]->replace(residuals);
                continue;
            }
            LineSeries* curve = new LineSeries;
            LineSeries* rest = new LineSeries;
            curve->append(fitted);
            rest->append(residuals);
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

int NmrImport::usedSpectra() const
{
    int used = 0;
    for (int k = 0; k < m_handler.spectrumCount(); ++k)
        used += !m_handler.isSpectrumExcluded(k);
    return used;
}

QVector<bool> NmrImport::excludedSpectra() const
{
    QVector<bool> excluded(m_handler.spectrumCount());
    for (int k = 0; k < excluded.size(); ++k)
        excluded[k] = m_handler.isSpectrumExcluded(k);
    return excluded;
}

void NmrImport::updateTable()
{
    QString error;
    const bool ok = validate(&error);
    const QString summary = tr("%1 rows, %2 concentration and %3 shift column(s).").arg(m_preview->model() ? m_preview->model()->rowCount() : 0).arg(m_independent_count).arg(m_handler.enabledSignals().size());
    m_table_status->setText(ok ? summary : error);
    QPalette palette = m_table_status->palette();
    palette.setColor(QPalette::WindowText, ok ? this->palette().color(QPalette::WindowText) : StatusLine::color(StatusLine::Error));
    m_table_status->setPalette(palette);
    // the same on both pages, in one line (the full text is the tooltip)
    if (ok)
        m_ready->setMessage(tr("OK hands over %1").arg(summary), StatusLine::Info);
    else
        m_ready->setMessage(tr("OK not possible yet: %1").arg(error), m_handler.spectrumCount() == 0 ? StatusLine::Info : StatusLine::Warning);
}

int NmrImport::manualCount(int spectrum, int signal) const
{
    int count = 0;
    for (int k = 0; k < m_handler.spectrumCount(); ++k) {
        if (spectrum >= 0 && k != spectrum)
            continue;
        for (int s = 0; s < m_handler.signalList().size(); ++s)
            if ((signal < 0 || s == signal) && m_handler.hasManualShift(k, s))
                ++count;
    }
    return count;
}

void NmrImport::addSignalAt(double position)
{
    const int index = m_handler.addSignal(position, m_window->value());
    retrack();
    if (index >= 0 && index < m_handler.signalList().size())
        setStatus(tr("Signal %1 added at %2 ppm.").arg(m_handler.signalList()[index].name).arg(position, 0, 'f', 4), false);
}

void NmrImport::addSignalAtPeak(double position)
{
    bool snapped = false;
    const int index = m_handler.addSignalAtPeak(position, m_window->value(), &snapped);
    retrack();
    if (index < 0 || index >= m_handler.signalList().size())
        return;
    const NmrSignal& signal = m_handler.signalList()[index];
    if (snapped)
        setStatus(tr("Signal %1 added at the peak at %2 ppm (double-click at %3 ppm).").arg(signal.name).arg(signal.position, 0, 'f', 4).arg(position, 0, 'f', 4), false);
    else
        setStatus(tr("Signal %1 added at %2 ppm: no free peak within the search window.").arg(signal.name).arg(position, 0, 'f', 4), false);
    m_signal_table->selectRow(index);
}

void NmrImport::pickRange(double* from, double* to) const
{
    double lo = std::min(m_editor->rangeStart(), m_editor->rangeEnd()), hi = std::max(m_editor->rangeStart(), m_editor->rangeEnd());
    if (QValueAxis* x = m_editor->chart()->axisX()) {
        // the visible part of the axis in ppm of the reference spectrum (drawn with its x offset)
        const double dx = m_editor->stackOffset(m_handler.referenceIndex()).x();
        const double a = std::min(x->min(), x->max()) - dx, b = std::max(x->min(), x->max()) - dx;
        if (b > a && std::max(lo, a) < std::min(hi, b)) {
            lo = std::max(lo, a);
            hi = std::min(hi, b);
        }
    }
    *from = lo;
    *to = hi;
}

void NmrImport::showSelectedSignal()
{
    const int row = m_signal_table->currentRow();
    m_editor->setHighlightedLabel(row);
    if (row < 0 || row >= m_handler.signalList().size() || m_handler.spectrumCount() == 0)
        return;
    const int ref = m_handler.referenceIndex();
    const NmrCell cell = m_handler.cell(ref, row);
    const double x = std::isfinite(cell.shift) ? cell.shift : m_handler.signalList()[row].position;
    m_editor->showX(x + m_editor->stackOffset(ref).x());
}

void NmrImport::updateDoubleClickHint()
{
    m_editor->setDoubleClickHint(m_double_adds->isChecked() ? tr("Double-click: add a signal at the peak there") : tr("Double-click: set the position of a new signal"));
}

bool NmrImport::validate(QString* error)
{
    m_input_table = QJsonObject();
    m_independent_count = 0;
    DataTable* concentrations = m_concentrations->dataTable();
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
    flushFidSettings();
    QString error;
    if (!validate(&error)) {
        QMessageBox::warning(this, tr("Shift table not complete"), error);
        return;
    }
    QDialog::accept();
}

void NmrImport::setStatus(const QString& text, bool error)
{
    m_status->setMessage(text, error ? StatusLine::Error : StatusLine::Info);
}

void NmrImport::reject()
{
    if (m_guard->confirmDiscard())
        QDialog::reject();
}
