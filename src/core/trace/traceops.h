/*
 * TraceOps - GUI-free, JSON-serialisable processing steps for XYTrace
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

// Claude Generated: WP1 of roadmap/input_data_editing.md

#pragma once

#include <memory>
#include <vector>

#include <Eigen/Dense>

#include <QtCore/QJsonObject>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>

#include "xytrace.h"

/*! \brief One picked peak (local maximum) of a trace. */
struct TracePeak {
    Eigen::Index index = -1; ///< data point of the maximum
    double x = 0; ///< position, refined by the parabola through the maximum and its neighbours
    double height = 0; ///< y of that parabola's vertex (the data value if not refined)
    double integral = 0; ///< trapezoid area between left and right (only with Integrate, else NaN)
    Eigen::Index left = -1, right = -1; ///< integration limits: the minima next to the peak
};

/*! \brief The algorithms behind the operations, as plain functions (Claude Generated).
 *
 * All of them take a valid trace (XYTrace::isValid, monotone axis in either direction) and leave the
 * input unchanged. On an invalid input or parameter they return false / an empty result and set *error.
 */
namespace TraceOps {

/*! \brief Area under the trace by the trapezoidal rule between the points first..last (inclusive,
 * last = -1: the end). Uses |dx|, so the area of a positive signal is positive on a descending axis. */
double trapezoid(const XYTrace& trace, Eigen::Index first = 0, Eigen::Index last = -1);

/*! \brief Savitzky-Golay smoothing: every point is replaced by the value at its own x of the
 * least-squares polynomial of degree order over the window points around it.
 *
 * window: odd, >= 3, > order, <= number of points; order >= 0 (0 = moving average). The fit uses the
 * real x values, so a non-uniform axis is handled; on a uniform axis this is the classic convolution
 * (the weights are computed once). At the ends the window is shifted inside the data (no padding),
 * so polynomials up to degree order are reproduced exactly everywhere. The weights of a symmetric
 * window sum to 1 and conserve the area of the interior up to the edge effects.
 */
bool savitzkyGolay(const XYTrace& trace, int window, int order, XYTrace* result, QString* error = nullptr);

/*! \brief Polynomial baseline b(x) = sum_k c_k t^k with t = (x - center) / scale (center and half
 * width of the fitted x range; keeps the normal equations well conditioned). */
struct PolynomialBaseline {
    Eigen::VectorXd coefficients; ///< c_k in the scaled variable t
    double center = 0, scale = 1;
    /*! \brief b(x) */
    double evaluate(double x) const;
    Eigen::VectorXd evaluate(const Eigen::VectorXd& x) const;
    /*! \brief The same polynomial in powers of x: b(x) = sum_k a_k x^k. */
    Eigen::VectorXd powerCoefficients() const;
};

/*! \brief Least-squares fit of a polynomial of degree degree to the points outside the excluded
 * x ranges (each pair in any order, inclusive).
 *
 * Linear least squares solved by column-pivoting Householder QR of the Vandermonde matrix in the scaled
 * variable t (no normal equations). Needs at least degree + 1 points left; degree 0..10.
 */
bool fitPolynomialBaseline(const XYTrace& trace, int degree, const QVector<QPair<double, double>>& excluded,
    PolynomialBaseline* baseline, QString* error = nullptr);

/*! \brief How NormaliseOp scales y. */
enum class NormaliseMode {
    Max, ///< y / max|y|: the largest magnitude becomes 1 (sign kept)
    Area, ///< y / |trapezoid area|: the area becomes 1 (positive signal)
    MinMax ///< (y - min) / (max - min): y spans [0, 1]
};

/*! \brief Normalises y. The statistic (max|y|, area, min/max) is taken from the reference window
 * [refStart, refEnd] if useReference, otherwise from the whole trace, and the scaling is applied to
 * the whole trace. Fails if the statistic is zero or the window holds no point (two for Area). */
bool normalise(const XYTrace& trace, NormaliseMode mode, bool useReference, double refStart, double refEnd,
    XYTrace* result, QString* error = nullptr);

/*! \brief Local maxima above a threshold, at least minDistance apart.
 *
 * A peak is a point (or the middle of a plateau of equal points) higher than both neighbours; the first
 * and last point are never peaks. Threshold: absolute = y >= threshold; relative = y >= min + threshold
 * * (max - min), threshold in [0, 1]. Peaks closer than minDistance (x units, >= 0) are thinned by
 * keeping the higher one. Position and height are refined by the parabola through the maximum and its
 * neighbours (a plateau longer than two points: its middle x, its y). With integrate, the area is the trapezoid between the next minima on either side (walk
 * down until y rises again) of the trace as given, i.e. a baseline has to be subtracted before.
 * Result sorted by index.
 */
bool pickPeaks(const XYTrace& trace, double threshold, bool relative, double minDistance, bool integrate,
    QVector<TracePeak>* peaks, QString* error = nullptr);

/*! \brief Replaces the points inside the x ranges by straight lines between their neighbours (BlankOp,
 * Claude Generated); false with *error if the ranges cover every point or a bound is not finite. */
bool blankRanges(const XYTrace& trace, const QVector<QPair<double, double>>& ranges, XYTrace* result, QString* error = nullptr);

/*! \brief Robust standard deviation of the noise of a trace (Claude Generated).
 *
 * σ = 1.4826 · median over blocks of MAD_b, MAD_b = median |r_i - median_b(r)| within block b, r = y minus the
 * least-squares parabola of the block (slope and curvature of a wing are not noise). The blocks hold
 * max(64, n / 32) consecutive points (the rest joins the last block; fewer than 64 points: one block). For
 * Gaussian noise 1.4826 · MAD is σ. The median over the blocks ignores the blocks covered by lines or by the
 * steep wing of a strong line as long as they are fewer than half; a block wide compared with the
 * correlation length of smoothed or apodized noise keeps the estimate unbiased (first differences would
 * underestimate correlated noise). 0 for fewer than 3 points. */
double noiseLevel(const XYTrace& trace);

} // namespace TraceOps

/*! \brief One processing step with its parameters (Claude Generated).
 *
 * Parameter objects: public members, validate(), toJson()/fromJson() with "Type". apply() never
 * changes its input; on an error it returns an empty trace and sets *error.
 */
class TraceOp {
public:
    virtual ~TraceOp() = default;
    /*! \brief A switched-off step stays in the list (and in its JSON, "Enabled": false) but
     * TraceOpList::apply skips it. Claude Generated */
    bool enabled = true;
    /*! \brief JSON "Type" of the step. */
    virtual QString type() const = 0;
    virtual bool validate(QString* error = nullptr) const = 0;
    virtual XYTrace apply(const XYTrace& trace, QString* error = nullptr) const = 0;
    virtual QJsonObject toJson() const = 0;
    virtual std::unique_ptr<TraceOp> clone() const = 0;

    /*! \brief Step from its JSON; nullptr with *error for an unknown "Type" or invalid parameters.
     * "Enabled" (default true) is read for every type. */
    static std::unique_ptr<TraceOp> fromJson(const QJsonObject& json, QString* error = nullptr);
};

/*! \brief Keeps the points with x in [xStart, xEnd] (bounds in any order). */
class RangeOp : public TraceOp {
public:
    RangeOp(double start = 0, double end = 0)
        : xStart(start)
        , xEnd(end)
    {
    }
    double xStart, xEnd;
    QString type() const override { return "Range"; }
    bool validate(QString* error = nullptr) const override;
    XYTrace apply(const XYTrace& trace, QString* error = nullptr) const override;
    QJsonObject toJson() const override;
    std::unique_ptr<TraceOp> clone() const override { return std::make_unique<RangeOp>(*this); }
};

/*! \brief Savitzky-Golay smoothing (TraceOps::savitzkyGolay). */
class SmoothOp : public TraceOp {
public:
    SmoothOp(int w = 5, int o = 2)
        : window(w)
        , order(o)
    {
    }
    int window, order;
    QString type() const override { return "Smooth"; }
    bool validate(QString* error = nullptr) const override;
    XYTrace apply(const XYTrace& trace, QString* error = nullptr) const override;
    QJsonObject toJson() const override;
    std::unique_ptr<TraceOp> clone() const override { return std::make_unique<SmoothOp>(*this); }
};

/*! \brief Normalisation (TraceOps::normalise). */
class NormaliseOp : public TraceOp {
public:
    NormaliseOp(TraceOps::NormaliseMode m = TraceOps::NormaliseMode::Max)
        : mode(m)
    {
    }
    TraceOps::NormaliseMode mode;
    bool useReference = false;
    double refStart = 0, refEnd = 0;
    QString type() const override { return "Normalise"; }
    bool validate(QString* error = nullptr) const override;
    XYTrace apply(const XYTrace& trace, QString* error = nullptr) const override;
    QJsonObject toJson() const override;
    std::unique_ptr<TraceOp> clone() const override { return std::make_unique<NormaliseOp>(*this); }
};

/*! \brief Subtracts a polynomial baseline fitted outside the excluded ranges (TraceOps::fitPolynomialBaseline). */
class BaselineOp : public TraceOp {
public:
    BaselineOp(int d = 1)
        : degree(d)
    {
    }
    int degree;
    QVector<QPair<double, double>> excluded;
    QString type() const override { return "Baseline"; }
    bool validate(QString* error = nullptr) const override;
    XYTrace apply(const XYTrace& trace, QString* error = nullptr) const override;
    QJsonObject toJson() const override;
    std::unique_ptr<TraceOp> clone() const override { return std::make_unique<BaselineOp>(*this); }
};

/*! \brief Shift of the x axis, x' = x + offset (Claude Generated, WP3).
 *
 * Referencing of NMR spectra: the chemical shift scale is fixed by a reference signal (TMS, the
 * residual solvent signal) of known position; offset = known - observed position moves every point
 * of the spectrum by the same amount. The orientation of the axis and the y values are unchanged.
 * JSON: {"Type": "ShiftAxis", "Offset": offset}. */
class ShiftAxisOp : public TraceOp {
public:
    ShiftAxisOp(double o = 0)
        : offset(o)
    {
    }
    double offset;
    QString type() const override { return "ShiftAxis"; }
    bool validate(QString* error = nullptr) const override;
    XYTrace apply(const XYTrace& trace, QString* error = nullptr) const override;
    QJsonObject toJson() const override;
    std::unique_ptr<TraceOp> clone() const override { return std::make_unique<ShiftAxisOp>(*this); }
};

/*! \brief Blanks x ranges: the points inside are replaced by the straight line between the values next
 * to them (Claude Generated, Viewer 2).
 *
 * Use: an intense signal that is not part of the titration (TMS, CHCl3 or another solvent/standard line,
 * a solvent band) dominates the y scale, a relative peak-picking threshold and every statistic after
 * it. Blanking removes it from everything later in the pipeline. All points with x inside any of the
 * ranges (bounds in any order, inclusive; overlapping or touching ranges act as one) form runs of
 * consecutive points; each run is replaced by the line through the last point before and the first
 * point after it, so the trace stays continuous (no step) and the points outside are left bit for bit.
 * A run at an end of the trace takes the value of its one neighbour. A range without a point changes
 * nothing; ranges covering every point are an error (there is no value left to draw the line from).
 * Works on ascending and descending axes. A Normalise step that uses the blanked signal as reference
 * has to come before this step.
 * JSON: {"Type": "Blank", "Ranges": [[from, to], ...]}. */
class BlankOp : public TraceOp {
public:
    QVector<QPair<double, double>> ranges;
    QString type() const override { return "Blank"; }
    bool validate(QString* error = nullptr) const override;
    XYTrace apply(const XYTrace& trace, QString* error = nullptr) const override;
    QJsonObject toJson() const override;
    std::unique_ptr<TraceOp> clone() const override { return std::make_unique<BlankOp>(*this); }
};

/*! \brief Peak picking (TraceOps::pickPeaks); apply() passes the trace through, pick() gives the peaks. */
class PickPeaksOp : public TraceOp {
public:
    double threshold = 0.1;
    bool relative = true;
    double minDistance = 0;
    bool integrate = false;
    QString type() const override { return "PickPeaks"; }
    bool validate(QString* error = nullptr) const override;
    XYTrace apply(const XYTrace& trace, QString* error = nullptr) const override;
    QVector<TracePeak> pick(const XYTrace& trace, QString* error = nullptr) const;
    QJsonObject toJson() const override;
    std::unique_ptr<TraceOp> clone() const override { return std::make_unique<PickPeaksOp>(*this); }
};

/*! \brief Ordered pipeline of steps, applied non-destructively to a raw trace (Claude Generated).
 *
 * JSON: {"Version": 1, "Operations": [ {"Type": ..., parameters}, ... ]}; a switched-off step
 * carries "Enabled": false. Copies are deep.
 */
class TraceOpList {
public:
    static constexpr int Version = 1;

    TraceOpList() = default;
    TraceOpList(const TraceOpList& other);
    TraceOpList& operator=(const TraceOpList& other);
    TraceOpList(TraceOpList&&) = default;
    TraceOpList& operator=(TraceOpList&&) = default;

    inline int size() const { return int(m_ops.size()); }
    inline bool isEmpty() const { return m_ops.empty(); }
    void append(std::unique_ptr<TraceOp> op);
    void append(const TraceOp& op) { append(op.clone()); }
    bool remove(int i);
    bool move(int from, int to);
    void clear() { m_ops.clear(); }
    /*! \brief Step i (0 <= i < size(), asserted); may be edited. */
    TraceOp* at(int i);
    const TraceOp* at(int i) const;

    /*! \brief Runs all enabled steps on a copy of raw. On the first failing step: empty trace, *error names
     * the step. peaks (optional) receives the result of the last PickPeaks step (applied to the
     * trace as it is at that step). */
    XYTrace apply(const XYTrace& raw, QString* error = nullptr, QVector<TracePeak>* peaks = nullptr) const;

    QJsonObject toJson() const;
    /*! \brief Reads toJson(); false with *error (and *list unchanged) for a missing or newer version,
     * a missing "Operations" array, an unknown step type or invalid parameters. */
    static bool fromJson(const QJsonObject& json, TraceOpList* list, QString* error = nullptr);

private:
    std::vector<std::unique_ptr<TraceOp>> m_ops;
};
