/*
 * FidProcessing - from a free induction decay (FID) to a phased 1D NMR spectrum
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

// Claude Generated: JEOL import of roadmap/input_data_editing.md (findings: docs/jeol_import_2026-11-20.md)

#pragma once

#include <limits>

#include <Eigen/Dense>

#include <QtCore/QJsonObject>
#include <QtCore/QString>

#include "xytrace.h"

namespace JeolReader {
struct Data;
}

/*! \brief A complex free induction decay with what is needed to turn it into a ppm spectrum
 * (Claude Generated).
 *
 * Convention (independent of the instrument): a line at chemical shift δ contributes
 * exp(+i 2π ν t) with ν = (δ - offset) · frequency [Hz, frequency in MHz]; point j is sampled at
 * t_j = (j - groupDelay) / sweep. A reader converts its format into this convention (JEOL Delta
 * stores the complex conjugate, see FidProcessing::fromJeol).
 */
struct Fid {
    Eigen::VectorXcd points; ///< complex time-domain points
    double sweep = 0; ///< spectral width = sampling rate [Hz]
    double frequency = 0; ///< spectrometer frequency of the nucleus [MHz]
    double offset = 0; ///< chemical shift of the carrier (centre of the spectrum) [ppm]
    double groupDelay = 0; ///< points recorded before t = 0 (digital filter delay), may be fractional; 0 = none
    QString name; ///< display name (file name)
    QString source; ///< file path
    QString nucleus; ///< e.g. "Proton"
    QString description; ///< one line about the acquisition (instrument, solvent, temperature) for display

    /*! \brief At least 2 finite points, positive sweep and frequency, finite offset, 0 <= groupDelay < size. */
    bool isValid(QString* reason = nullptr) const;
    /*! \brief Lossless JSON ("Re", "Im" as TraceJson strings, "Sweep", "Frequency", "Offset", "GroupDelay", "Name", "Nucleus"). */
    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject& json, Fid* fid, QString* error = nullptr);
};

/*! \brief Parameters of the FID processing; JSON with "Version" like TraceOpList (Claude Generated). */
struct FidSettings {
    static constexpr int Version = 1;
    bool groupDelayCorrection = true; ///< shift the time origin by Fid::groupDelay (linear phase ph1 = 360° · groupDelay)
    double lineBroadening = 0.3; ///< exponential apodization exp(-π LB t): Lorentzian broadening LB [Hz] (0 = off)
    double gaussBroadening = 0; ///< Gaussian apodization exp(-(π GB t)² / (4 ln 2)): Gaussian broadening GB [Hz] FWHM (0 = off)
    int zeroFill = 2; ///< spectrum size = smallest power of two >= points · zeroFill (1..16)
    double ph0 = 0; ///< zero-order phase [degree]
    double ph1 = 0; ///< first-order phase [degree] across the full spectral width, see FidProcessing::phase
    double pivot = std::numeric_limits<double>::quiet_NaN(); ///< ppm where ph1 has no effect; NaN = the carrier
    /// every spectrum is phased by FidProcessing::autoPhase instead of ph0/ph1; the default, because the receiver
    /// phase differs from experiment to experiment (real JEOL series: ph0 spread over 200°, docs/jeol_import_2026-11-20.md)
    bool autoPhase = true;
    bool autoFirstOrder = true; ///< autoPhase also fits ph1 (else ph1 is kept)

    QJsonObject toJson() const;
    /*! \brief Reads toJson(); missing keys keep their defaults; false with *error for a newer version
     * or values out of range. */
    static bool fromJson(const QJsonObject& json, FidSettings* settings, QString* error = nullptr);
    /*! \brief Values within their ranges (LB, GB >= 0 and finite, zeroFill 1..16, phases finite). */
    bool isValid(QString* reason = nullptr) const;
    bool operator==(const FidSettings& other) const;
    inline bool operator!=(const FidSettings& other) const { return !(*this == other); }
};

/*! \brief Phases applied to one spectrum. */
struct FidPhase {
    double ph0 = 0, ph1 = 0; ///< [degree]
    double pivot = 0; ///< [ppm]
    bool automatic = false; ///< found by autoPhase
    int evaluations = 0; ///< cost evaluations of autoPhase
};

/*! \brief The processing chain FID -> spectrum, model-free and GUI-free (Claude Generated).
 *
 * 1. Apodization: point j is multiplied by w(t_j), t_j = (j - groupDelay)/sweep (w = 1 for t < 0, the
 *    points of the filter before t = 0): exponential exp(-π LB t) broadens every Lorentzian line by LB Hz
 *    (FWHM), Gaussian exp(-(π GB t)²/(4 ln 2)) convolves with a Gaussian of GB Hz FWHM. Without a
 *    digital filter (groupDelay 0) the first point is halved (the discrete transform counts t = 0 once
 *    instead of half, which would lift the baseline).
 * 2. Zero filling to M = the smallest power of two >= N · zeroFill. Only powers of two are transformed,
 *    so a radix-2 FFT is sufficient (no Bluestein/mixed radix needed): the padding to M is part of the
 *    processing anyway, also for N that is not a power of two.
 * 3. FFT S_k = Σ_j f_j exp(-i 2π j k / M), reordered so that k runs from -M/2 to M/2-1 (frequency
 *    ν_k = k · sweep / M), i.e. ascending δ_k = offset + ν_k / frequency.
 * 4. Group delay: S_k · exp(+i 2π ν_k · groupDelay / sweep) moves the time origin to the true t = 0
 *    (exact for a fractional delay; the filter's lead-in points wrap to negative times).
 * 5. Phase: R_k = Re(S_k · exp(i θ_k)), θ_k = ph0 + ph1 · (ν_k - ν_pivot)/sweep (degrees); a first-order phase
 *    of 360° is a time shift of one point. autoPhase finds ph0/ph1 by ACME (see there).
 * 6. Output: R on a DESCENDING ppm axis (NMR convention), one point per k.
 */
namespace FidProcessing {

/*! \brief Largest spectrum size (points) after zero filling. */
constexpr Eigen::Index MaxSize = Eigen::Index(1) << 24;

bool isPowerOfTwo(Eigen::Index n);
/*! \brief Smallest power of two >= n · factor (factor clamped to 1..16); 0 if larger than MaxSize. */
Eigen::Index paddedSize(Eigen::Index n, int factor);

/*! \brief In-place radix-2 FFT (decimation in time). Forward: S_k = Σ_j f_j exp(-2πi jk/M); inverse
 * with exp(+2πi jk/M) and 1/M. false (data unchanged) if the size is not a power of two. Claude Generated */
bool fft(Eigen::VectorXcd& data, bool inverse = false);

/*! \brief Steps 1-4: complex spectrum with ascending ppm axis (both of size paddedSize). Claude Generated */
bool complexSpectrum(const Fid& fid, const FidSettings& settings, Eigen::VectorXcd* spectrum, Eigen::VectorXd* ppm, QString* error = nullptr);

/*! \brief Step 5: real part after the phase θ_k = ph0 + ph1 · (δ_k - pivot) · frequency / sweep. Claude Generated */
Eigen::VectorXd phase(const Eigen::VectorXcd& spectrum, const Eigen::VectorXd& ppm, double frequency, double sweep, double ph0, double ph1, double pivot);

/*! \brief ACME cost of the phased real part R (Chen et al., J. Magn. Reson. 158 (2002) 164):
 * entropy -Σ h_k ln h_k of the normalised first derivative h_k = |R'_k| / Σ|R'| plus a penalty
 * γ Σ_{R_k<0} (R_k / max|S|)² for negative intensity. An absorption spectrum has narrow derivative
 * peaks (low entropy) and no negative lobes. Claude Generated */
double acmeCost(const Eigen::VectorXd& real, double scale);

/*! \brief Automatic phase correction by minimising acmeCost (Claude Generated).
 *
 * Coarse: grid of ph0 (10° steps) and, with firstOrder, ph1 in [-ph1Range, ph1Range] (30° steps) on a
 * spectrum reduced to <= 8192 points by summing blocks of neighbouring points (the sum of a line's
 * complex points keeps its phase). Fine: from each of the 4 lowest local minima of the grid (a spectrum of
 * few lines has several basins of almost equal cost) Nelder-Mead (restarted once) on a spectrum reduced to
 * <= 65536 points, stopped at a simplex smaller than 0.01° (without firstOrder: golden section on ph0), ph1
 * held inside ± ph1Range by a quadratic wall; the lowest cost wins and is polished on the full spectrum.
 * Returns ph0 in (-180, 180]. Without firstOrder ph1 stays at the given value.
 */
FidPhase autoPhase(const Eigen::VectorXcd& spectrum, const Eigen::VectorXd& ppm, double frequency, double sweep, double pivot,
    bool firstOrder = true, double ph1Start = 0, double ph1Range = 180);

/*! \brief Steps 1-6 as a trace (name/source of the FID, x axis "δ"/"ppm"); empty trace and false with *error
 * for an invalid FID or settings. *used receives the phases applied. Claude Generated */
bool process(const Fid& fid, const FidSettings& settings, XYTrace* spectrum, FidPhase* used = nullptr, QString* error = nullptr);

/*! \brief JEOL Delta time-domain data as a Fid: the complex conjugate of the stored points (JEOL stores
 * ν with the opposite sign: without it CHCl3 appears at 2.75 instead of 7.25 ppm), sweep X_SWEEP, frequency
 * X_FREQ, offset X_OFFSET, groupDelay from the digital filter description (0 without filter). false
 * with *error for frequency-domain data or missing acquisition parameters. Claude Generated */
bool fromJeol(const JeolReader::Data& data, Fid* fid, QString* error = nullptr);

} // namespace FidProcessing
