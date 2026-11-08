/*
 * LineshapeFit - model-free least-squares fit of pseudo-Voigt lines to a trace
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

// Claude Generated: WP4 of roadmap/input_data_editing.md
//
// Placed in core/trace: it is a model-free building block on XYTrace data (like TraceOps), knows
// nothing of NMR or of the binding models; NmrHandler uses it, other profiles may.

#pragma once

#include <cmath>
#include <limits>

#include <Eigen/Dense>

#include <QtCore/QString>
#include <QtCore/QVector>

#include "xytrace.h"

/*! \brief Pseudo-Voigt line with a common full width at half maximum for the Gauss and the Lorentz part.
 *
 * f(x) = A [ (1 - η) G(x; δ, w) + η L(x; δ, w) ]
 *   G(x) = sqrt(4 ln2 / π) / w · exp(-4 ln2 (x - δ)² / w²)      (Gauss, σ = w / (2 sqrt(2 ln2)))
 *   L(x) = 2 / (π w) · 1 / (1 + 4 (x - δ)² / w²)               (Lorentz, half width γ = w / 2)
 * Both parts are normalised to unit area over the whole real axis, so the area of the line is A
 * EXACTLY (analytically); over a finite range [δ - R, δ + R] it is A [(1 - η) erf(2 sqrt(ln2) R / w)
 * + η (2/π) atan(2 R / w)] — the Lorentz tails make the difference noticeable (R = 5 w: 3.7 % of
 * the Lorentz area lies outside). Height at δ: A [(1 - η) sqrt(4 ln2 / π) + η 2 / π] / w.
 * The linear Gauss-Lorentz mixture approximates the true Voigt profile (convolution) to about 1 % of the
 * height; η is a shape parameter, not the Lorentz fraction of a convolution. Claude Generated.
 */
struct PseudoVoigtLine {
    double position = 0; ///< δ, centre of the line [x units]
    double fwhm = 1; ///< w > 0, full width at half maximum [x units]
    double area = 1; ///< A, area over the whole axis [x units · y units]
    double eta = 0.5; ///< η ∈ [0, 1]: 0 = Gauss, 1 = Lorentz
};

/*! \brief Baseline under the lines of a fit range. */
enum class LineshapeBaseline {
    None, ///< b(x) = 0
    Constant, ///< b(x) = b0
    Linear ///< b(x) = b0 + b1 t, t = (x - centre) / halfwidth of the fit range
};

/*! \brief Outcome of a fit. */
enum class LineshapeStatus {
    Converged, ///< the sum of squares no longer decreases (relative change or step below the tolerance)
    MaxIterations, ///< stopped at the iteration limit
    Degenerate, ///< converged, but J^T J of the free parameters is singular: no standard errors (e.g. two identical lines)
    Invalid ///< not started: empty/short data, NaN, a start outside the range or its bounds
};

/*! \brief One fit parameter: start value and how it may move (the "lock" of a parameter).
 *
 * fixed: the value stays; otherwise it moves within [lower, upper] (infinite = unbounded). The bounds
 * are kept by projection (see LineshapeFit), so the value is never NaN or outside them. */
struct LineshapeParameter {
    double value = 0;
    bool fixed = false;
    double lower = -std::numeric_limits<double>::infinity();
    double upper = std::numeric_limits<double>::infinity();
};

/*! \brief Start of one line. shared >= 0: fwhm and eta belong to the shared set of that index (fitSeries,
 * all lines with the same index have ONE width and ONE η; control and start from the first of them,
 * the start value is their mean); -1: the line has its own width and η. */
struct LineshapeLineStart {
    LineshapeParameter position, fwhm, area, eta;
    int shared = -1;
};

/*! \brief Data of one block of a (series) fit: the points of the fit range and the lines in it. */
struct LineshapeBlock {
    Eigen::VectorXd x, y;
    QVector<LineshapeLineStart> lines;
};

/*! \brief Options of a fit. */
struct LineshapeOptions {
    LineshapeBaseline baseline = LineshapeBaseline::Constant;
    int maxIterations = 200;
    double tolerance = 1e-10; ///< relative change of the sum of squares / of the scaled step that ends the fit
};

/*! \brief Result for one block. Errors are standard errors from the covariance (J^T J)^-1 s²,
 * s² = SSE / (m - p) of the WHOLE fit (m points, p free parameters not at a bound); NaN for a fixed
 * parameter, a parameter at its bound and in a degenerate fit. */
struct LineshapeBlockResult {
    QVector<PseudoVoigtLine> lines, errors;
    QVector<bool> positionAtBound, fwhmAtBound, areaAtBound; ///< parameter ended at (and pushes against) its bound
    Eigen::VectorXd baseline; ///< b0 (, b1)
    double centre = 0, halfwidth = 1; ///< of the linear baseline variable t
    Eigen::VectorXd x, residuals; ///< residual = data - model at x
    double sse = 0;
    /*! \brief Model at x (lines + baseline). */
    double evaluate(double x) const;
};

/*! \brief Result of a fit (one or several blocks). */
struct LineshapeResult {
    LineshapeStatus status = LineshapeStatus::Invalid;
    QString message; ///< why a fit is Invalid/Degenerate/MaxIterations
    QVector<LineshapeBlockResult> blocks;
    double sse = 0, rms = 0; ///< over all points; rms = sqrt(SSE / m)
    double s2 = 0; ///< SSE / (m - p)
    int points = 0, freeParameters = 0, iterations = 0;
    double milliseconds = 0; ///< wall time of the fit (timing analysis)
    inline bool ok() const { return status == LineshapeStatus::Converged; }
};

/*! \brief Least-squares fit of a sum of pseudo-Voigt lines and a baseline (Claude Generated).
 *
 * Minimises SSE = Σ_i (y_i - Σ_lines f(x_i) - b(x_i))² with a damped Gauss-Newton (Levenberg-Marquardt)
 * iteration written for this problem (not the Eigen/AbstractModel optimiser: we need bounds, fixed
 * parameters, parameters shared between blocks and the covariance):
 *  - analytic Jacobian of every parameter (formulas in lineshapefit.cpp, checked against central
 *    differences in test_lineshapefit);
 *  - step (J^T J + λ diag(J^T J)) Δ = -J^T r (Marquardt scaling, LDLT); a step is taken if the SSE
 *    decreases (λ / 3), else λ · 4;
 *  - bounds by projection with an active set: a free parameter on its bound whose gradient points out
 *    is held for the step; the new point is clamped into the box, so values are never outside or NaN;
 *  - converged when the relative SSE change or the scaled step falls below options.tolerance, or no
 *    decrease is possible any more (λ overflow at a minimum within machine precision);
 *  - standard errors from (J^T J)^-1 s² of the free parameters not at a bound; the rank is checked on
 *    the column-normalised J^T J (smallest / largest eigenvalue < 1e-12: Degenerate). The parameters of
 *    a line whose area went to 0 are left out of the covariance (they are undetermined).
 *
 * Series (fitSeries): the blocks (e.g. one spectrum each) are fitted together; their residual vectors are
 * stacked, positions/areas/baselines belong to their block, width and η of lines with the same shared
 * index are one parameter for all blocks. The sparsity is used where it costs: J^T J and J^T r are
 * accumulated block by block from the small dense Jacobian of each block (its baseline, its lines, the
 * shared widths), never from the stacked J. The damped system itself is solved densely (p x p, p ≤ a few
 * hundred for a titration); a Schur complement over the block-arrow structure would only pay off for
 * much larger series.
 */
namespace LineshapeFit {

/*! \brief Value of the line at x. */
double evaluate(const PseudoVoigtLine& line, double x);
/*! \brief Value and partial derivatives at x: grad = (∂f/∂δ, ∂f/∂w, ∂f/∂A, ∂f/∂η). */
double evaluate(const PseudoVoigtLine& line, double x, Eigen::Vector4d* grad);
/*! \brief Height of the line at its centre. */
double height(const PseudoVoigtLine& line);
/*! \brief Area of a line of this height, width and η (inverse of height()). */
double areaFromHeight(double height, double fwhm, double eta);

/*! \brief Full width at half height of the peak at index peak of y(x) above a baseline: walk down on
 * both sides until y - baseline < (y(peak) - baseline) / 2, interpolate the crossing linearly. Of the two
 * sides the narrower one counts (a neighbour line raises the other); NaN if no side crosses. */
double estimateFwhm(const Eigen::VectorXd& x, const Eigen::VectorXd& y, Eigen::Index peak, double baseline = 0);

/*! \brief Fit of the lines to the points of trace with x in [from, to] (any order). */
LineshapeResult fit(const XYTrace& trace, double from, double to, const QVector<LineshapeLineStart>& lines,
    const LineshapeOptions& options = LineshapeOptions());

/*! \brief Joint fit of several blocks (stacked residuals, shared width/η by LineshapeLineStart::shared). */
LineshapeResult fitSeries(const QVector<LineshapeBlock>& blocks, const LineshapeOptions& options = LineshapeOptions());

QString statusName(LineshapeStatus status);
QString baselineName(LineshapeBaseline baseline);
LineshapeBaseline baselineFromName(const QString& name, LineshapeBaseline fallback = LineshapeBaseline::Constant);

} // namespace LineshapeFit
