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

#include "src/core/trace/fidprocessing.h"
#include "src/core/trace/lineshapefit.h"
#include "src/core/trace/traceops.h"
#include "src/core/trace/traceset.h"

class DataTable;

/*! \brief What a signal follows (Claude Generated, multiplet signals). */
enum class NmrSignalType {
    Peak, ///< one line: δ = position of that line (the behaviour of every project without the key)
    Multiplet ///< a group of lines within ± halfSpan of their centre: δ = the centre (class description, step 3b)
};

/*! \brief Estimator of the centre of a multiplet from its lines (Claude Generated, class description step 3b). */
enum class NmrMultipletCentre {
    Centroid, ///< (a) intensity-weighted centre of the baseline-corrected area over the span of the group
    OuterMidpoint, ///< (b) midpoint of the outermost lines
    LineMean ///< (c) height-weighted mean of the picked line positions
};

/*! \brief A signal followed through the titration: its position in the reference spectrum and the
 * half width of the search window, both in ppm of the referenced scale. Claude Generated.
 * A multiplet (type Multiplet) is a group of lines whose centre is followed; position is then the centre in the
 * reference spectrum and window the allowed motion of the centre. */
struct NmrSignal {
    QString name;
    double position = 0; ///< δ in the reference spectrum [ppm] (multiplet: its centre)
    double window = 0.05; ///< search window ± window around the last known position [ppm]
    bool enabled = true; ///< false: the signal is not a column of the shift table
    NmrSignalType type = NmrSignalType::Peak; ///< Claude Generated: one line or a multiplet
    double halfSpan = 0; ///< multiplet: half width of the group around its centre [ppm] (the lines lie within ± halfSpan)
    QString pattern = QString(); ///< multiplet: expected pattern (s, d, t, q, dd, dt, ..., m; empty = none), only a hint (line count warning)
    inline bool isMultiplet() const { return type == NmrSignalType::Multiplet; }
};

/*! \brief State of one cell (spectrum, signal) of the shift table. Claude Generated. */
enum class NmrCellStatus {
    Ok, ///< exactly this signal claims the peak found in its window
    Missing, ///< no peak in the window; the shift is NaN (never interpolated)
    Conflict, ///< another signal claims the same peak; the shift is that peak's position
    Manual, ///< set by the user (overrides the tracking and feeds it)
    FitFailed ///< lineshape fit: the fit of this cell failed; the shift is NaN like a missing one (WP4)
};

/*! \brief Where the shift of the table comes from (Claude Generated, WP4). */
enum class NmrShiftMethod {
    PeakPicking, ///< δ = position of the picked maximum (parabola refinement)
    LineshapeFit ///< δ = centre of a pseudo-Voigt fitted to the whole signal (LineshapeFit)
};

/*! \brief What the detection threshold of the default peak picking of the tracking refers to (Claude Generated).
 * Only used when the processing has no enabled PickPeaks step (that step's own threshold applies then). */
enum class NmrThresholdScope {
    Spectrum, ///< a maximum counts if y >= min + 0.05 (max - min) of the whole processed spectrum (projects without the option)
    Window ///< per signal and spectrum: height above the baseline of the search window >= max(0.05 x the highest peak there, 6 σ of the noise), see step 3a
};

/*! \brief Width and η of the fitted lines over the series (Claude Generated, WP4). */
enum class NmrWidthMode {
    Free, ///< every spectrum is fitted on its own: width and η per spectrum
    Shared ///< one width and one η per signal for all spectra: one joint fit of the series per group
};

/*! \brief Settings of the lineshape fit, stored as "LineshapeFit" (Claude Generated, WP4). */
struct NmrFitSettings {
    NmrShiftMethod method = NmrShiftMethod::PeakPicking;
    NmrWidthMode widthMode = NmrWidthMode::Free;
    LineshapeBaseline baseline = LineshapeBaseline::Constant;
    double range = 0.03; ///< fit range ± range around each signal [ppm]; overlapping ranges are fitted together
    int maxIterations = 200;
    bool peakFallback = false; ///< a cell whose fit fails takes the peak-picking δ instead of "fit failed"
    QJsonObject toJson() const;
    static NmrFitSettings fromJson(const QJsonObject& json);
    bool operator==(const NmrFitSettings& other) const
    {
        return method == other.method && widthMode == other.widthMode && baseline == other.baseline && range == other.range
            && maxIterations == other.maxIterations && peakFallback == other.peakFallback;
    }
};

/*! \brief Fit state of a cell (Claude Generated, WP4). */
enum class NmrFitState {
    NotFitted, ///< peak picking, or no start (missing/conflict/excluded)
    Fitted, ///< converged, the line inside its bounds
    Failed ///< see NmrCell::fitMessage
};

/*! \brief One cell of the shift table. Claude Generated. */
struct NmrCell {
    double shift = std::numeric_limits<double>::quiet_NaN(); ///< δ [ppm] of the table (peak-picking or fit δ, see NmrFitSettings::method)
    double height = std::numeric_limits<double>::quiet_NaN(); ///< peak height (processed spectrum)
    double integral = std::numeric_limits<double>::quiet_NaN(); ///< peak area (only with "Integrate" of the PickPeaks step)
    NmrCellStatus status = NmrCellStatus::Missing;
    // ---- WP4: lineshape fit (NaN without a fit)
    double peakShift = std::numeric_limits<double>::quiet_NaN(); ///< δ of the picked peak (the tracking result)
    double fitShift = std::numeric_limits<double>::quiet_NaN(); ///< centre of the fitted line [ppm]
    double shiftError = std::numeric_limits<double>::quiet_NaN(); ///< standard error of fitShift [ppm]
    double fwhm = std::numeric_limits<double>::quiet_NaN(); ///< fitted full width at half maximum [ppm]
    double area = std::numeric_limits<double>::quiet_NaN(); ///< fitted area [ppm · intensity]
    double eta = std::numeric_limits<double>::quiet_NaN(); ///< fitted Lorentz fraction η of the pseudo-Voigt
    NmrFitState fitState = NmrFitState::NotFitted;
    QString fitMessage; ///< why the fit failed
    // ---- multiplet signals (Claude Generated; empty for a single peak)
    QVector<double> lines; ///< positions of the lines of the group [ppm], ascending
    QVector<double> lineHeights; ///< their heights above the baseline of the search region
    double groupShare = std::numeric_limits<double>::quiet_NaN(); ///< sum of the line heights / that of the highest group of the region
};

/*! \brief One fitted range of one spectrum: the lines (signals and unassigned neighbour lines) with
 * baseline and residuals, for drawing and inspection (Claude Generated, WP4). */
struct NmrFitRegion {
    double from = 0, to = 0; ///< ppm range
    QVector<int> lineSignal; ///< per fitted line: index of the signal, -1 = neighbour line of no signal
    LineshapeBlockResult block;
    LineshapeStatus status = LineshapeStatus::Invalid;
    QString message;
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
 *    TraceOps::pickPeaks), or of a default picking (no minimal distance) if the list has none. The threshold
 *    of the default picking (thresholdScope()) is relative to the whole spectrum (Spectrum: y >= min + 0.05
 *    (max - min); a line much stronger than a signal, a standard or the solvent, can hide it) or to the
 *    search window of each signal (Window, step 3a).
 * 3. Starting at the reference spectrum the series is walked forward and backward. For each signal
 *    the candidates are the peaks within ± window of its last known position (the window moves with
 *    the signal); a peak within ± window of the prediction (linear extrapolation of the last two known
 *    positions, per spectrum index; the last position if only one is known) is a candidate, too, so
 *    a signal that moves faster than the window per step is followed while its motion is steady.
 * 3a. Window threshold (Claude Generated): the region is [min(last, prediction) - window, max(last, prediction)
 *    + window]; σ is the noise of the spectrum (TraceOps::noiseLevel, robust against lines). A local maximum
 *    of the region is a candidate if its height above the chord (straight line through the medians of the
 *    first and the last tenth of the region widened by the window on both sides) is >= max(0.05 H, 6 σ), or
 *    its prominence (height above the higher of the lowest points on either side, walked until a higher point)
 *    is >= max(0.05 H, 10 σ); H = the largest of both measures in the region. The chord keeps a line that
 *    overlaps a neighbour, the prominence a line on the curved wing of a strong line. 6 σ: the highest of N
 *    noise maxima exceeds 6 σ with probability about N · 1e-9; 10 σ for the prominence because it adds the
 *    depth of the lowest point (a pure-noise window: about 5-7 σ). Measured false-peak rate in test_nmrhandler;
 *    0.05 is the relative threshold of the spectrum rule, applied locally.
 *    A candidate lower than minHeight() times the height of the signal's last peak is not taken
 *    (noise maxima, weak neighbour lines; the intensity of a signal changes little from step to step).
 *    Of several candidates the one of least cost |δ - prediction| / window + |ln(height / last height)|
 *    wins: nearest to the prediction, unless its height jumps (a noise maximum or a neighbour line close
 *    to an overshooting prediction). For the first peak of a signal (no height known yet) the peak
 *    nearest to the position among those at least half as high as the highest candidate.
 * 3b. Multiplets (Claude Generated, NmrSignal::type Multiplet): a multiplet (d, t, dd, ...) is one nucleus whose
 *    line is split by couplings J into lines of fixed spacing around its chemical shift; the fast-exchange shift is
 *    the CENTRE of the group, and the lines move together. Following single lines fails when the centre moves more
 *    than a line spacing per step (the lines are swapped), so the group is followed as one: the region is
 *    [min(last, prediction) - (h + w), max(last, prediction) + (h + w)] (h = halfSpan, w = window); its candidate
 *    lines are the peaks there (with the window threshold: those passing step 3a in the region). A line's height is
 *    taken above the chord of the region (as in step 3a). Every run of neighbouring candidates within 2h is a
 *    possible group; in it, maxima lower than multipletRipple() x the highest line of the run (ripple beside the
 *    lines) or with a prominence below 6 σ (noise on a line; prominence as in step 3a) do not count, and the run must
 *    begin and end on lines; barely resolved lines keep their height but have a shallow dip, hence height for the
 *    ripple and prominence for the noise. In the first spectrum (no group known) the group is the lines within ± h
 *    of the position (the most lines, then the highest). Later a group whose centre lies within w of the last
 *    position or of the prediction is a candidate; the one of least cost |c - prediction| / w + |ln(S / S_last)|
 *    + |n - n_last| + |spread - spread_last| / h wins (S = sum of its line heights / that of the highest group of
 *    the region: the intensity of whole experiments jumps, the share does not; n lines; J does not change, so a group
 *    with a foreign line or without one of its own costs more); S < minHeight() x S_last is not taken.
 *    The centre c (multipletCentre()):
 *    (a) Centroid: Σ w x / Σ w, w = max(y - b, 0) over [outer lines ∓ e], b the chord of the region (the ends of the
 *        interval itself still lie on the tails of the lines), e = 1.5 x the width at half height of the outer lines,
 *        at most half the gap to the next line outside the group (one e for both sides: a one-sided cut would move
 *        the centroid);
 *    (b) OuterMidpoint: (x_first + x_last) / 2;
 *    (c) LineMean: Σ h_i x_i / Σ h_i of the picked lines (heights above the chord).
 *    Assumptions and sources of error: first-order multiplets (Δν >> J) are symmetric, all three give the shift.
 *    Strong coupling (Δν/J of a few) bends the intensities towards the partner (roof effect) and moves the lines:
 *    for an AB pair the midpoint (b) lies outside the true shift by C - Δν/2 (2C = sqrt(Δν² + J²)), the
 *    intensity-weighted (a)/(c) inside by Δν/2 - Δν²/(4C); a constant bias cancels in the shift change, a Δν that
 *    changes along the titration does not. A neighbour multiplet closer than about one line width adds its tail to
 *    (a) (linear part removed by b) and can merge with the outer line ((b), (c)); groups that share a line or whose
 *    spans interleave or that hold a line within ± h of the other's centre are a Conflict. Noise moves the picked
 *    positions ((b), (c)) more than the area (a); a lost outer line moves (b) by half a spacing, (c) and (a) less; a
 *    partly resolved outer pair moves (b) by up to half its spacing. Default (b): the smallest errors measured in
 *    test_nmrmultiplet and test_jeol (docs/nmr_multiplets_2026-11-21.md).
 *    The number of lines found is compared with the pattern (expectedLineCount) only as a warning (lineCountWarning).
 *    Lineshape fit: every line of the group is a line of the fit (starts from the picked lines, shared width with
 *    "Shared"); the centre is the area-weighted mean of the fitted positions.
 * 4. No candidate: Missing (NaN, the last known position stays). Two signals claim the same peak:
 *    both Conflict (shift = the peak, their last known positions are not moved by it). A manual
 *    value (setManualShift) replaces the cell (Manual) and is a known position for the tracking.
 *
 * The shift table is released (shiftTable(), importTable()) only if every used cell is Ok or Manual;
 * a spectrum can be excluded (setSpectrumExcluded) and a signal switched off (NmrSignal::enabled).
 *
 * Lineshape fit (WP4, fitSettings().method == LineshapeFit): after the tracking the picked peaks are the
 * starts of a pseudo-Voigt fit of the whole signal (LineshapeFit), which gives δ also where overlapping
 * lines pull the maxima together:
 * 5. Groups: signals whose fit ranges (± range around the tracked δ) overlap in any spectrum are fitted
 *    together. Per spectrum and group the range is [lowest δ - range, highest δ + range]; it holds one line per
 *    signal with a start (Ok: the picked δ, position bounded by ± its search window; Manual: the position
 *    fixed at the manual δ) and one line per further picked peak in the range at least minHeight() times as
 *    high as the lowest signal of the group (a neighbour line, not reported). Signals in Conflict (one maximum
 *    for two signals) get one line each and the range covers their search windows: an unclaimed peak within the window of such a signal (around its
 *    last fitted position, else its reference position) is its start, else it starts at its last fitted
 *    position or spread around the shared maximum; if the fit separates them (≥ 0.25 FWHM) the cells become
 *    Ok. Missing cells get no line.
 * 6. Free widths: the spectra are fitted in the order of the tracking (from the reference outwards);
 *    width and η start from the last successful fit of the signal, area from the picked height; a failed
 *    fit is repeated with the starts from the peak alone (width at half height, η = 0.5).
 *    Shared widths: one joint fit per group over all used spectra (positions, areas, baselines per spectrum,
 *    width and η per signal).
 * 7. A cell's fit counts if the fit converged, its position, width and area are not at a bound and its area
 *    is at least 1 % of the largest line of the range;
 *    otherwise FitFailed (shift NaN), or with peakFallback the picked δ (status Ok, fitState Failed).
 *    The table then holds the fitted δ; shiftErrorTable() their standard errors.
 * Fit settings are stored ("LineshapeFit"); the results are not, they are recomputed on loading (the fit is
 * deterministic: same spectra, steps and signals give the same numbers).
 *
 * Persistence (toJson/fromJson): the raw block of a DataClass of type Spectrum with "Profile": "nmr";
 * the spectra as in SpectraHandler ("Data" only with the application property StoreRawData, else
 * read again from "Sources"), "TraceOps", "ReferenceOffset", "ReferenceSpectrum", "Signals", "Manual",
 * "Excluded", "Window", "MinHeight", "LineshapeFit", "ThresholdScope" ("Window"; written only then, missing = Spectrum).
 * Multiplets (Claude Generated): a signal carries "Type": "Multiplet", "HalfSpan" and "Pattern" (if set) only when it
 * is one; "MultipletCentre" and "MultipletRipple" are written only when a signal is a multiplet.
 *
 * FIDs (JEOL .jdf, Claude Generated): a time-domain file is transformed by FidProcessing with fidSettings()
 * (one set for the whole series: apodization, zero filling, phase or automatic phase per spectrum); the
 * spectrum list holds the transformed spectra, the FIDs are kept to transform them again when the settings
 * change. Stored: "FidProcessing" (only when there is a FID); with StoreRawData additionally "Fid" (the
 * complex points per spectrum position "1".."n", lossless), else the files are read again from "Sources".
 * "Data" still holds the transformed spectra, so a reader without FID support sees them.
 *
 * Speed (Claude Generated, viewer fixes): the referencing, the steps, the peaks and the noise of every spectrum are
 * computed once and kept per spectrum id until the steps or the offset change or the spectra are replaced
 * (processedEntry); a retrack after a change of signals, window, scope, order or exclusions only follows the signals.
 * The cache is mutable state: a handler is used from one thread at a time. FIDs of addFiles() and setFidSettings()
 * are transformed in parallel (OpenMP), each exactly as alone.
 */
class NmrHandler {
public:
    NmrHandler() = default;
    ~NmrHandler() = default;

    // ---- spectra
    /*! \brief Reads one file (two columns = one spectrum, more columns = x + one spectrum per column,
     * TopSpin totxt; a JEOL Delta file is recognised by its content and its FID transformed with
     * fidSettings()) and appends its spectra; false with *error, nothing added then. */
    bool addFile(const QString& filename, QString* error = nullptr);
    /*! \brief addFile() for every file, read and transformed in parallel (OpenMP), appended in the order of files;
     * returns per file the reason it was not read (empty: read). The spectra equal those of addFile(). Claude Generated */
    QVector<QString> addFiles(const QStringList& files);
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
    /*! \brief Peaks of every spectrum (class description, step 2). With the Window threshold and no PickPeaks
     * step: the local maxima that pass the window rule (step 3a) in a window ± defaultWindow() centred on
     * themselves (used for pickSignals() and the neighbour lines of the lineshape fit). */
    QVector<QVector<TracePeak>> peaks(QString* error = nullptr) const;

    // ---- FID processing (Claude Generated, JEOL import)
    /*! \brief True if at least one spectrum was transformed from a FID. */
    inline bool hasFids() const { return !m_fids.isEmpty(); }
    /*! \brief True if the spectrum at index was transformed from a FID. */
    bool isFid(int index) const;
    inline const FidSettings& fidSettings() const { return m_fid_settings; }
    /*! \brief Replaces the settings and transforms every FID again (ids, signals and manual shifts stay);
     * false with *error for invalid settings or a failed transform (the old settings stay then). Call
     * track() afterwards. */
    bool setFidSettings(const FidSettings& settings, QString* error = nullptr);
    /*! \brief Phase applied to the spectrum at index (zero phases for a spectrum that is no FID). */
    FidPhase fidPhase(int index) const;
    /*! \brief Acquisition summary of the FID at index (instrument, nucleus, solvent, ...); empty for other spectra. */
    QString fidDescription(int index) const;
    /*! \brief Finds ph0/ph1 by FidProcessing::autoPhase on the reference spectrum (the first FID if the
     * reference is none) and sets them as the common phase of the series (autoPhase off); false with *error
     * without FIDs. */
    bool autoPhaseFromReference(QString* error = nullptr);

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
     * shifts), each with this window; only peaks with from <= δ <= to (default: the whole spectrum) are taken,
     * the detection itself is unchanged (relative to the whole spectrum or to the window of each maximum).
     * Returns their number. */
    int pickSignals(double window, QString* error = nullptr, double from = -std::numeric_limits<double>::infinity(),
        double to = std::numeric_limits<double>::infinity());
    /*! \brief Adds a signal at position and moves it onto the peak the tracking takes for it in the reference
     * spectrum (step 3: the nearest of the peaks within the window that are at least half as high as the highest
     * there, with the detection threshold of the tracking); without such a peak, or when another signal claims it
     * (conflict), the signal stays at position. *snapped tells which. Tracks; returns the index. Claude Generated */
    int addSignalAtPeak(double position, double window, bool* snapped = nullptr);

    // ---- multiplets (Claude Generated, class description step 3b)
    /*! \brief Adds a multiplet for the range [from, to] of the reference spectrum: half span = half the range, centre =
     * the centre of the lines found there by the tracking (the middle of the range without lines; *found tells which).
     * Tracks; returns the index (-1 for an empty range). */
    int addMultiplet(double from, double to, double window, bool* found = nullptr);
    /*! \brief Multiplets from the peaks of the reference spectrum between from and to (replacing the signals and the
     * manual shifts): neighbouring lines closer than maxCoupling [ppm] form one group (single linkage); lines whose
     * height is below ripple x the highest line of their group or below 6 σ of the noise are left out (and the groups
     * split again where they bridged a gap); a maximum with a prominence below 6 σ is no line (as in the tracking);
     * half span = half the spread of the lines + 0.25 x the spread, within 0.1-0.5 x maxCoupling (at least 3 points);
     * centre by multipletCentre(); a group of one line is a singlet (pattern "s"). The detection is that of
     * pickSignals(). Returns the number of multiplets. */
    int pickMultiplets(double window, double maxCoupling, double ripple, QString* error = nullptr,
        double from = -std::numeric_limits<double>::infinity(), double to = std::numeric_limits<double>::infinity());
    /*! \brief Estimator of the multiplet centre (stored as "MultipletCentre"; default OuterMidpoint: the smallest errors in
     * test_nmrmultiplet and against the operator's MestReNova centres, docs/nmr_multiplets_2026-11-21.md). */
    inline NmrMultipletCentre multipletCentre() const { return m_multiplet_centre; }
    inline void setMultipletCentre(NmrMultipletCentre centre) { m_multiplet_centre = centre; }
    /*! \brief Smallest height of a line of a multiplet relative to its highest line (default 0.05, "MultipletRipple"). */
    inline double multipletRipple() const { return m_multiplet_ripple; }
    inline void setMultipletRipple(double fraction)
    {
        if (fraction >= 0 && fraction < 1)
            m_multiplet_ripple = fraction;
    }
    /*! \brief True if a signal is a multiplet. */
    bool hasMultiplets() const;
    /*! \brief Spectrometer frequency of the nucleus [MHz] from the first FID (NaN without FIDs), for J in Hz -> ppm. */
    double spectrometerFrequency() const;
    /*! \brief Number of lines of a first-order pattern: s 1, d 2, t 3, q 4, p/quint 5, sext 6, sept 7 and products of
     * the letters d/t/q/p (dd 4, dt 6, ddd 8); -1 for m, an empty or an unknown pattern (no check). */
    static int expectedLineCount(const QString& pattern);
    /*! \brief Warning for a multiplet cell whose number of lines differs from its pattern ("3 lines, dd has 4"); empty
     * otherwise. Equal couplings or unresolved lines give fewer lines, so it is never an error. */
    QString lineCountWarning(int spectrum, int signal) const;
    static QString signalTypeName(NmrSignalType type);
    static QString multipletCentreName(NmrMultipletCentre centre);
    static NmrMultipletCentre multipletCentreFromName(const QString& name, NmrMultipletCentre fallback = NmrMultipletCentre::OuterMidpoint);
    /// noise part of the prominence of a multiplet line in units of σ of the noise (the candidates passed the detection
    /// already; 6 σ as the chord rule of step 3a: 10 σ dropped real outer lines at 5 % noise in test_nmrmultiplet)
    static constexpr double MultipletProminenceSigmas = 6.0;
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

    /*! \brief Reference of the detection threshold of the default peak picking (stored as "ThresholdScope";
     * default Spectrum, the behaviour of projects without the key). Claude Generated. */
    inline NmrThresholdScope thresholdScope() const { return m_threshold_scope; }
    inline void setThresholdScope(NmrThresholdScope scope) { m_threshold_scope = scope; }
    /*! \brief True if the tracking uses the window threshold: scope Window and no enabled PickPeaks step. */
    bool usesWindowThreshold() const;
    /*! \brief True if Operations() hold an enabled PickPeaks step (its threshold is used, not thresholdScope()). */
    bool hasPickStep() const;
    /// relative part of the window threshold (fraction of the highest peak above the window baseline)
    static constexpr double WindowRelativeThreshold = 0.05;
    /// noise part of the window threshold in units of σ of the noise (TraceOps::noiseLevel)
    static constexpr double WindowNoiseSigmas = 6.0;
    /// noise part of the prominence criterion of the window threshold in units of σ
    static constexpr double WindowProminenceSigmas = 10.0;

    // ---- lineshape fit (WP4)
    inline const NmrFitSettings& fitSettings() const { return m_fit; }
    /*! \brief Replaces the settings (invalid range/iterations are clamped); call track() afterwards. */
    void setFitSettings(const NmrFitSettings& settings);
    /*! \brief Fitted ranges of a spectrum of the last track() (empty for peak picking). */
    QVector<NmrFitRegion> fitRegions(int spectrum) const;
    /*! \brief Wall time of the fits of the last track() [ms]. */
    inline double fitMilliseconds() const { return m_fit_ms; }

    // ---- tracking and table
    /*! \brief Follows every signal through the series (class description, steps 3 and 4), then fits the
     * lines if fitSettings().method is LineshapeFit (steps 5-7); fills cells(). */
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
    /*! \brief Standard errors of the fitted δ in the layout of shiftTable() (header "σ(δ) <name> /ppm"; NaN where
     * there is no fit, e.g. manual cells); nullptr with *error without a lineshape fit or without rows/columns. */
    DataTable* shiftErrorTable(QString* error = nullptr) const;

    /*! \brief Number of spectrum processings (referencing, steps, peak picking) since construction: a cached
     * result is reused while spectra, steps and reference offset are unchanged (for tests). Claude Generated */
    inline int processingRuns() const { return m_processing_runs; }

    // ---- persistence
    QJsonObject toJson() const;
    /*! \brief Reads toJson(), then track(). Spectra that cannot be read (file missing without raw data)
     * are named in *error; false if no spectrum is read although some were stored. */
    bool fromJson(const QJsonObject& json, QString* error = nullptr);
    /*! \brief True for a raw block written by toJson() ("Profile": "nmr"). */
    static bool isNmrBlock(const QJsonObject& raw);

    static QString statusName(NmrCellStatus status);

private:
    void trackDirection(int start, int step, const QVector<QVector<TracePeak>>& peaks, const QVector<XYTrace>* processed = nullptr,
        const QVector<double>* noise = nullptr, bool windowThreshold = false);
    /// Claude Generated: one candidate group of a multiplet (step 3b)
    struct MultipletGroup {
        QVector<int> members; ///< indices into the peaks of the spectrum, ascending x
        QVector<double> lines, heights; ///< positions and heights above the chord of the region
        double centre = std::numeric_limits<double>::quiet_NaN();
        double height = 0; ///< sum of the line heights
        double area = std::numeric_limits<double>::quiet_NaN(); ///< baseline-corrected area over the span (centroid)
        QVector<double> outside; ///< lines of something else nearby (limit the centroid integration)
        double chord[4] = { 0, 0, 0, 0 }; ///< baseline of the region (x0, y0, x1, y1)
    };
    /// candidate groups of a multiplet of half span h in [lo, hi] of trace (step 3b); passing: indices into found that
    /// pass the detection (nullptr: all of found in the region)
    QVector<MultipletGroup> multipletGroups(const XYTrace& trace, const QVector<TracePeak>& found, const QVector<int>* passing, double lo, double hi,
        double halfSpan, double noise) const;
    /// centre of the lines of a group by the estimator; chord = baseline (x0, y0, x1, y1); area of the centroid in *area
    double multipletCentreOf(const XYTrace& trace, const QVector<TracePeak>& found, const QVector<int>& members, const QVector<double>& heights,
        const QVector<double>& outside, NmrMultipletCentre estimator, const double chord[4], double* area = nullptr) const;
    QVector<QVector<TracePeak>> peaks(QString* error, QVector<XYTrace>* processed, bool allMaxima = false) const;
    /// maxima that pass the window rule in a window ± defaultWindow() centred on themselves; with near only those
    /// inside the given x ranges of their spectrum
    QVector<QVector<TracePeak>> windowDetectedPeaks(const QVector<XYTrace>& processed, const QVector<QVector<TracePeak>>& maxima, const QVector<double>& noise,
        const QVector<QVector<QPair<double, double>>>* near = nullptr) const;
    void fitLineshapes(const QVector<XYTrace>& processed, const QVector<QVector<TracePeak>>& peaks);
    QString spectrumLabel(int index) const;
    /// Claude Generated: what the processing of one spectrum gave (cached, see processedEntry)
    struct ProcessedEntry {
        XYTrace referenced; ///< raw spectrum + referenceOffset()
        XYTrace processed; ///< after Operations(); empty if reason is set
        QString reason; ///< error of Operations()
        QVector<TracePeak> stepPeaks; ///< peaks of the last enabled PickPeaks step
        QVector<TracePeak> fallback[2]; ///< default picking: [0] threshold 0.05, [1] every maximum
        QString fallbackReason[2];
        bool hasFallback[2] = { false, false };
        double noise = std::numeric_limits<double>::quiet_NaN(); ///< TraceOps::noiseLevel of the processed spectrum (NaN: not yet)
    };
    /// processing of spectrum index for the current spectra, steps and offset; computed once and kept until one of
    /// them changes (the same functions on the same input: the numbers are those of a new computation)
    const ProcessedEntry& processedEntry(int index) const;
    /// default picking of an entry (0.05 of the span, or every maximum)
    const QVector<TracePeak>& fallbackPeaks(const ProcessedEntry& entry, bool allMaxima) const;
    /// σ of the noise of the processed spectrum index (cached)
    double noiseOf(int index) const;
    /// drops every cached processing (spectra replaced)
    inline void invalidateProcessing() { m_processed.clear(); }
    /// reads a file: spectra of a text file, or the FID of a JEOL file (*fid set, *isFid true)
    bool readSource(const QString& filename, QVector<XYTrace>* traces, Fid* fid, bool* isFid, QString* error) const;

    TraceSet m_spectra; ///< raw spectra in titration order; source = file, name = file name [column]
    QHash<QString, int> m_columns; ///< spectrum id -> data column in its file (1 = first after x), for reading again
    TraceOpList m_operations;
    double m_reference_offset = 0;
    double m_window = 0.05;
    double m_min_height = 0.3;
    NmrThresholdScope m_threshold_scope = NmrThresholdScope::Spectrum;
    NmrMultipletCentre m_multiplet_centre = NmrMultipletCentre::OuterMidpoint; ///< Claude Generated
    double m_multiplet_ripple = 0.05; ///< Claude Generated
    int m_reference = 0;
    QVector<NmrSignal> m_signals;
    QMap<QString, QMap<int, double>> m_manual; ///< spectrum id -> signal -> δ
    QSet<QString> m_excluded; ///< spectrum ids
    QVector<QVector<NmrCell>> m_cells;
    NmrFitSettings m_fit;
    QVector<QVector<NmrFitRegion>> m_fit_regions; ///< [spectrum]
    double m_fit_ms = 0;
    QHash<QString, Fid> m_fids; ///< spectrum id -> FID it was transformed from
    QHash<QString, FidPhase> m_fid_phases; ///< spectrum id -> phase applied
    FidSettings m_fid_settings;
    mutable QHash<QString, ProcessedEntry> m_processed; ///< spectrum id -> processing (Claude Generated)
    mutable QString m_processed_key; ///< steps and offset of m_processed
    mutable int m_processing_runs = 0;
};
