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

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtGui/QColor>
#include <QtWidgets/QDialog>

#include "src/core/nmrhandler.h"

class DataTable;
class DropTable;
class ImportKeyGuard;
class StatusLine;
class LineSeries;
class ScatterSeries;
class TraceEditorWidget;

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QTableWidget;

/*! \brief Import of an NMR titration (Claude Generated).
 *
 * Renders an NmrHandler (the science and the persistence live there): the spectra (order, used or
 * left out, reference spectrum), the trace editor with the processing steps, the signals (position,
 * window, name; picked from the reference spectrum or added at a position, a double-click in the chart
 * takes the position), the tracked shifts with a colour per status and manual values, and the drop
 * table of the concentrations. Every change tracks again. accept() refuses a table with missing or
 * conflicting cells or without concentrations (message box with the cells).
 *
 * WP4: "Shift from" switches between peak picking and the lineshape fit of NmrHandler (options: width free
 * or shared, baseline, fit range, iterations, peak position where a fit fails); "Fit details" adds the
 * columns ±δ, FWHM, area and η per signal to the result table; the chart shows the fitted model and its
 * residual for the selected spectrum.
 *
 * FIDs (JEOL .jdf): the panel "Processing (FID)" under the spectra list (shown only when the series holds a
 * FID) sets the common FID processing of NmrHandler: line broadening, zero filling, phase 0/1 for all
 * spectra, "Auto phase" (from the reference spectrum, written into the phase fields) or automatic phase per
 * spectrum.
 *
 * UX package 1: keyboard rule and discard question of ImportKeyGuard; a status line at the bottom says
 * on both pages what OK hands over or why it cannot (validate()); "Double-click adds the signal"
 * (QSettings "nmrimport/doubleClickAdds", default on) adds a signal at a double-click, else the
 * double-click only sets the position; picking signals, removing a spectrum or a signal asks first when
 * values set by hand (or more than one signal) would be lost; a wait cursor around work that took more
 * than 0.3 s the last time (AdaptiveWaiter).
 */
class NmrImport : public QDialog {
    Q_OBJECT
public:
    explicit NmrImport(QWidget* parent = nullptr);

    /*! \brief Reads files (one spectrum, a matrix of spectra or TopSpin totxt each); false with *error. */
    bool addFiles(const QStringList& files, QString* error = nullptr);
    /*! \brief Reads the files of a directory ending with suffix in natural order. */
    bool setDirectory(const QString& directory, const QString& suffix, QString* error = nullptr);
    /*! \brief Restores a project ({"raw": NMR block, "independent": concentrations}). */
    void setData(const QJsonObject& project);
    /*! \brief Sets the concentration table (rows = spectra); the dialog takes a copy. */
    void setConcentrations(const DataTable* table);

    inline NmrHandler* handler() { return &m_handler; }
    /*! \brief The raw block of the project (NmrHandler::toJson). */
    inline QJsonObject ProjectData() const { return m_handler.toJson(); }
    /*! \brief Concentrations + shifts as a table JSON (DataTable::ExportTable); empty if not valid. */
    inline QJsonObject InputTable() const { return m_input_table; }
    /*! \brief Number of concentration columns of InputTable(). */
    inline int IndependentCount() const { return m_independent_count; }

    /*! \brief Builds the import table; false with *error (cells without a unique shift, concentrations). */
    bool validate(QString* error = nullptr);

    /*! \brief Status colour of a cell of the shift table (invalid for Ok). */
    static QColor statusColor(NmrCellStatus status);

    /*! \brief Tracks again and redraws everything (after changes made through handler()). */
    void refresh();

    inline ImportKeyGuard* keyGuard() const { return m_guard; }
    /*! \brief Number of cells set by hand, of one spectrum and/or one signal (-1: all). Claude Generated */
    int manualCount(int spectrum = -1, int signal = -1) const;
    /*! \brief Adds a signal at position (name and window as "Add") and tracks again. Claude Generated */
    void addSignalAt(double position);

public slots:
    void accept() override;
    void reject() override;

private:
    void setupUi();
    QWidget* signalPanel();
    QWidget* fidPanel();
    /*! \brief Shows the FID settings of the handler; the panel is visible only with FIDs. Claude Generated */
    void updateFidControls();
    /*! \brief Passes the FID controls to the handler, transforms again and redraws. Claude Generated */
    void applyFidSettings();
    void updateSpectra();
    void updateSignals();
    void updateResults();
    void updateMarkers();
    void updateTable();
    void retrack();
    void setStatus(const QString& text, bool error);
    /*! \brief Hint of the double-click in the trace editor, after the option. Claude Generated */
    void updateDoubleClickHint();
    void applyFitSettings();
    void updateFitControls();
    /*! \brief Spectrum whose fit is drawn: the selected one, else the reference. */
    int fitSpectrum() const;

    NmrHandler m_handler;
    QJsonObject m_input_table;
    int m_independent_count = 0;
    bool m_updating = false;

    QLineEdit* m_path;
    QComboBox* m_suffix;
    QPushButton *m_directory, *m_files;
    QTabWidget* m_tabs;
    QListWidget* m_spectra;
    QPushButton *m_up, *m_down, *m_remove, *m_reference;
    TraceEditorWidget* m_editor;
    QTableWidget* m_results;
    QDoubleSpinBox *m_offset, *m_window, *m_position;
    QPushButton *m_pick, *m_add, *m_remove_signal;
    QTableWidget* m_signal_table;
    QComboBox *m_method, *m_width_mode, *m_baseline;
    QDoubleSpinBox* m_fit_range;
    QSpinBox* m_max_iterations;
    QCheckBox *m_fallback, *m_details;
    QWidget* m_fit_options;
    StatusLine* m_status; ///< result of the last action (read, picked, refused input)
    StatusLine* m_ready; ///< what OK hands over or why not, visible on both pages
    QLabel* m_legend;
    QCheckBox* m_double_adds;
    QCheckBox* m_reverse_axis; ///< ppm axis from high (left) to low (right); QSettings "nmrimport/reverseAxis"
    ImportKeyGuard* m_guard;
    qint64 m_track_ms = 0, m_fid_ms = 0; ///< duration of the last tracking / FID transform (wait cursor)
    DropTable *m_concentrations, *m_preview;
    QLabel* m_table_status;
    QDialogButtonBox* m_buttons;
    QGroupBox* m_fid_box;
    QDoubleSpinBox *m_line_broadening, *m_ph0, *m_ph1;
    QComboBox* m_zero_fill;
    QPushButton* m_auto_phase;
    QCheckBox* m_auto_each;
    QLabel* m_fid_info;
    QList<QPointer<ScatterSeries>> m_markers; ///< tracked positions in the chart (deleted with clearChart)
    QList<QPointer<LineSeries>> m_fit_lines; ///< fitted model and residual of the selected spectrum
};
