/*
 * NmrHandler - series of 1D NMR spectra of a titration and the chemical shifts of tracked signals
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

#include <cmath>
#include <limits>

#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include "src/core/trace/traceops.h"
#include "src/core/trace/traceset.h"

class DataTable;

/*! \brief A signal followed through the titration: its position in the reference spectrum and the
 * half width of the search window, both in ppm of the referenced scale. Claude Generated. */
struct NmrSignal {
    QString name;
    double position = 0; ///< δ in the reference spectrum [ppm]
    double window = 0.05; ///< search window ± window around the last known position [ppm]
    bool enabled = true; ///< false: the signal is not a column of the shift table
};

/*! \brief State of one cell (spectrum, signal) of the shift table. Claude Generated. */
enum class NmrCellStatus {
    Ok, ///< exactly this signal claims the peak found in its window
    Missing, ///< no peak in the window; the shift is NaN (never interpolated)
    Conflict, ///< another signal claims the same peak; the shift is that peak's position
    Manual ///< set by the user (overrides the tracking and feeds it)
};

/*! \brief One cell of the shift table. Claude Generated. */
struct NmrCell {
    double shift = std::numeric_limits<double>::quiet_NaN(); ///< δ [ppm]
    double height = std::numeric_limits<double>::quiet_NaN(); ///< peak height (processed spectrum)
    double integral = std::numeric_limits<double>::quiet_NaN(); ///< peak area (only with "Integrate" of the PickPeaks step)
    NmrCellStatus status = NmrCellStatus::Missing;
};

/*! \brief Import of an NMR titration: a series of 1D spectra -> table of chemical shifts δ [ppm]
 * (rows = titration steps, columns = tracked signals) for the NMR models (Claude Generated).
 *
 * Science (fast exchange): if the exchange between free and bound host is fast on the NMR time
 * scale, every nucleus shows ONE averaged signal at δ_obs = Σ_i x_i δ_i (x_i the mole fractions of
 * the species carrying the nucleus). Along the titration this signal moves continuously from δ_free
 * towards δ_complex; the NMR models (AbstractNMRModel) fit exactly these δ_obs values. The handler
 * therefore follows each signal from spectrum to spectrum (track()):
 *
 * 1. The signals are defined in the reference spectrum (referenceIndex(); automatically from its
 *    peaks, pickSignals(), or by a list of positions with a search window ± w, setSignals()).
 * 2. Every spectrum is processed (ShiftAxis by referenceOffset(), then Operations()); its peaks are
 *    those of the last enabled PickPeaks step of Operations() (position refined by the parabola of
 *    TraceOps::pickPeaks), or of a default picking (relative threshold 0.05, no minimal distance)
 *    if the list has none.
 * 3. Starting at the reference spectrum the series is walked forward and backward. For each signal
 *    the candidates are the peaks within ± window of its last known position (the window moves with
 *    the signal); a peak within ± window of the prediction (linear extrapolation of the last two known
 *    positions, per spectrum index; the last position if only one is known) is a candidate, too, so
 *    a signal that moves faster than the window per step is followed while its motion is steady.
 *    A candidate lower than minHeight() times the height of the signal's last peak is not taken
 *    (noise maxima, weak neighbour lines; the intensity of a signal changes little from step to step).
 *    Of several candidates the one of least cost |δ - prediction| / window + |ln(height / last height)|
 *    wins: nearest to the prediction, unless its height jumps (a noise maximum or a neighbour line close
 *    to an overshooting prediction). For the first peak of a signal (no height known yet) the peak
 *    nearest to the position among those at least half as high as the highest candidate.
 * 4. No candidate: Missing (NaN, the last known position stays). Two signals claim the same peak:
 *    both Conflict (shift = the peak, their last known positions are not moved by it). A manual
 *    value (setManualShift) replaces the cell (Manual) and is a known position for the tracking.
 *
 * The shift table is released (shiftTable(), importTable()) only if every used cell is Ok or Manual;
 * a spectrum can be excluded (setSpectrumExcluded) and a signal switched off (NmrSignal::enabled).
 *
 * Persistence (toJson/fromJson): the raw block of a DataClass of type Spectrum with "Profile": "nmr";
 * the spectra as in SpectraHandler ("Data" only with the application property StoreRawData, else
 * read again from "Sources"), "TraceOps", "ReferenceOffset", "ReferenceSpectrum", "Signals", "Manual",
 * "Excluded", "Window", "MinHeight".
 */
class NmrHandler {
public:
    NmrHandler() = default;
    ~NmrHandler() = default;

    // ---- spectra
    /*! \brief Reads one file (two columns = one spectrum, more columns = x + one spectrum per column,
     * TopSpin totxt) and appends its spectra; false with *error, nothing added then. */
    bool addFile(const QString& filename, QString* error = nullptr);
    /*! \brief Appends the files of a directory whose name ends with suffix (empty = all files) in natural
     * order ("2" before "10", QCollator numeric mode); false with *error if none could be read
     * (files that cannot be read are listed in *error, the others are added). */
    bool addDirectory(const QString& directory, const QString& suffix, QString* error = nullptr);
    /*! \brief Appends a spectrum (its x axis may be descending). Returns its id. */
    QString addSpectrum(const XYTrace& spectrum);
    bool removeSpectrum(int index);
    bool moveSpectrum(int from, int to);
    void clear();

    inline const TraceSet& spectra() const { return m_spectra; }
    inline int spectrumCount() const { return m_spectra.size(); }

    // ---- processing
    inline const TraceOpList& operations() const { return m_operations; }
    inline void setOperations(const TraceOpList& operations) { m_operations = operations; }

    /*! \brief Offset added to the ppm axis of every spectrum before Operations() (referencing). */
    inline double referenceOffset() const { return m_reference_offset; }
    /*! \brief Sets the offset; with moveSignals the signal positions and manual shifts move along, so
     * they stay on the same peaks. */
    void setReferenceOffset(double offset, bool moveSignals = true);
    /*! \brief Changes the offset so that a peak observed at observed (current scale) lies at target. */
    inline void referenceTo(double observed, double target) { setReferenceOffset(m_reference_offset + target - observed); }

    /*! \brief Spectra shifted by referenceOffset() (the input of Operations()). */
    TraceSet referencedSpectra() const;
    /*! \brief Spectra after the referencing and Operations(); a spectrum whose processing fails is empty,
     * *error names the first failure. */
    TraceSet processedSpectra(QString* error = nullptr) const;
    /*! \brief Peaks used by the tracking, per spectrum (see the class description, step 2). */
    QVector<QVector<TracePeak>> peaks(QString* error = nullptr) const;

    // ---- signals
    inline int referenceIndex() const { return m_reference; }
    /*! \brief Spectrum in which the signals are defined; clamped to the spectra. */
    void setReferenceIndex(int index);
    inline const QVector<NmrSignal>& signalList() const { return m_signals; }
    /*! \brief Replaces the signals; manual shifts are dropped where the column no longer exists. */
    void setSignals(const QVector<NmrSignal>& list);
    /*! \brief Appends a signal (name = its position with 3 decimals if empty); returns its index. */
    int addSignal(double position, double window, const QString& name = QString());
    bool removeSignal(int index);
    bool setSignal(int index, const NmrSignal& signal);
    /*! \brief Signals from the peaks of the reference spectrum (replacing the present ones and the manual
     * shifts), each with this window; returns their number. */
    int pickSignals(double window, QString* error = nullptr);
    /*! \brief Default window of new signals (stored as "Window"). */
    inline double defaultWindow() const { return m_window; }
    inline void setDefaultWindow(double window)
    {
        if (window > 0 && std::isfinite(window))
            m_window = window;
    }

    /*! \brief Smallest height of a candidate relative to the last peak of the signal (default 0.3, "MinHeight"). */
    inline double minHeight() const { return m_min_height; }
    inline void setMinHeight(double fraction)
    {
        if (fraction >= 0 && fraction <= 1)
            m_min_height = fraction;
    }

    // ---- tracking and table
    /*! \brief Follows every signal through the series (class description, steps 3 and 4); fills cells(). */
    void track();
    /*! \brief Cells of the last track(): [spectrum][signal]. */
    inline const QVector<QVector<NmrCell>>& cells() const { return m_cells; }
    /*! \brief Cell (spectrum, signal) of the last track(); a missing cell for indices out of range. */
    NmrCell cell(int spectrum, int signal) const;

    /*! \brief Sets the shift of a cell by hand (status Manual) and tracks again. */
    bool setManualShift(int spectrum, int signal, double shift);
    /*! \brief Removes a manual shift and tracks again. */
    bool clearManualShift(int spectrum, int signal);
    bool hasManualShift(int spectrum, int signal) const;

    /*! \brief Excluded spectra are not rows of the table and are skipped by the tracking. */
    void setSpectrumExcluded(int index, bool excluded);
    bool isSpectrumExcluded(int index) const;
    /*! \brief Positions of the spectra that are rows of the table. */
    QVector<int> includedSpectra() const;
    /*! \brief Indices of the enabled signals (columns of the table). */
    QVector<int> enabledSignals() const;

    /*! \brief Cells that keep the table from being used (Missing/Conflict in an included row and an
     * enabled column), one line each ("spectrum 3 (name), signal H1: missing"). */
    QStringList problems() const;
    /*! \brief Shift table (included spectra x enabled signals, header "δ <name> /ppm") of the last
     * track(); nullptr with *error (the list of problems()) if a cell is Missing or Conflict, or
     * there is no spectrum or signal. The caller owns the table. */
    DataTable* shiftTable(QString* error = nullptr) const;
    /*! \brief Concentrations joined with shiftTable(): the rows of independent are those of all spectra
     * (the excluded ones are left out) or of the included spectra only; nullptr with *error otherwise. */
    DataTable* importTable(const DataTable* independent, QString* error = nullptr) const;

    // ---- persistence
    QJsonObject toJson() const;
    /*! \brief Reads toJson(), then track(). Spectra that cannot be read (file missing without raw data)
     * are named in *error; false if no spectrum is read although some were stored. */
    bool fromJson(const QJsonObject& json, QString* error = nullptr);
    /*! \brief True for a raw block written by toJson() ("Profile": "nmr"). */
    static bool isNmrBlock(const QJsonObject& raw);

    static QString statusName(NmrCellStatus status);

private:
    void trackDirection(int start, int step, const QVector<QVector<TracePeak>>& peaks);
    QString spectrumLabel(int index) const;

    TraceSet m_spectra; ///< raw spectra in titration order; source = file, name = file name [column]
    QHash<QString, int> m_columns; ///< spectrum id -> data column in its file (1 = first after x), for reading again
    TraceOpList m_operations;
    double m_reference_offset = 0;
    double m_window = 0.05;
    double m_min_height = 0.3;
    int m_reference = 0;
    QVector<NmrSignal> m_signals;
    QMap<QString, QMap<int, double>> m_manual; ///< spectrum id -> signal -> δ
    QSet<QString> m_excluded; ///< spectrum ids
    QVector<QVector<NmrCell>> m_cells;
};
