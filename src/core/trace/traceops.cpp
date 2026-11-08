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

// Claude Generated: WP1 of roadmap/input_data_editing.md. Own implementations (Eigen) instead of
// libpeakpick: its spectrum/baseline code has known out-of-range reads (see the roadmap).

#include <algorithm>
#include <cmath>
#include <limits>

#include <QtCore/QDebug>
#include <QtCore/QJsonArray>

#include "traceops.h"

namespace {
const double kNaN = std::numeric_limits<double>::quiet_NaN();

void setError(QString* error, const QString& message)
{
    if (error)
        *error = message;
}

/// Common precondition of the algorithms: equal lengths, finite, strictly monotone (or < 2 points)
bool checkTrace(const XYTrace& trace, QString* error)
{
    QString reason;
    if (!trace.isValid(&reason)) {
        setError(error, QString("invalid trace: %1").arg(reason));
        return false;
    }
    return true;
}

/*! \brief Savitzky-Golay weights: w with p(x0) = w . y for the least-squares polynomial of degree order
 * through the points xs. Scaled variable t = (x - x0) / max|x - x0| keeps the Vandermonde matrix well
 * conditioned; the value at x0 is the constant coefficient = row 0 of the pseudo inverse. */
Eigen::VectorXd sgWeights(const Eigen::VectorXd& xs, double x0, int order)
{
    const Eigen::Index w = xs.size();
    double scale = (xs.array() - x0).abs().maxCoeff();
    if (!(scale > 0))
        scale = 1;
    Eigen::MatrixXd A(w, order + 1);
    for (Eigen::Index j = 0; j < w; ++j) {
        const double t = (xs(j) - x0) / scale;
        double p = 1;
        for (int k = 0; k <= order; ++k) {
            A(j, k) = p;
            p *= t;
        }
    }
    const Eigen::CompleteOrthogonalDecomposition<Eigen::MatrixXd> cod(A);
    const Eigen::MatrixXd pinv = cod.pseudoInverse(); // (order + 1) x w
    return pinv.row(0).transpose();
}

QString modeName(TraceOps::NormaliseMode mode)
{
    switch (mode) {
    case TraceOps::NormaliseMode::Max:
        return "Max";
    case TraceOps::NormaliseMode::Area:
        return "Area";
    case TraceOps::NormaliseMode::MinMax:
        return "MinMax";
    }
    return "Max";
}

bool readNumber(const QJsonObject& json, const QString& key, double* value, QString* error, bool required)
{
    const QJsonValue v = json.value(key);
    if (v.isUndefined()) {
        if (required)
            setError(error, QString("parameter %1 is missing").arg(key));
        return !required;
    }
    if (!v.isDouble()) {
        setError(error, QString("parameter %1 is not a number").arg(key));
        return false;
    }
    *value = v.toDouble();
    return true;
}

bool readInt(const QJsonObject& json, const QString& key, int* value, QString* error)
{
    double d = *value;
    if (!readNumber(json, key, &d, error, false))
        return false;
    if (d != std::floor(d) || std::abs(d) > 1e6) {
        setError(error, QString("parameter %1 is not an integer").arg(key));
        return false;
    }
    *value = int(d);
    return true;
}
} // namespace

namespace TraceOps {

double trapezoid(const XYTrace& trace, Eigen::Index first, Eigen::Index last)
{
    const Eigen::Index n = std::min(trace.x().size(), trace.y().size());
    if (last < 0 || last >= n)
        last = n - 1;
    first = std::max<Eigen::Index>(first, 0);
    double area = 0;
    for (Eigen::Index i = first; i < last; ++i)
        area += 0.5 * (trace.y()(i) + trace.y()(i + 1)) * std::abs(trace.x()(i + 1) - trace.x()(i));
    return area;
}

bool savitzkyGolay(const XYTrace& trace, int window, int order, XYTrace* result, QString* error)
{
    if (!checkTrace(trace, error))
        return false;
    const Eigen::Index n = trace.size();
    if (window < 3 || window % 2 == 0) {
        setError(error, QString("Savitzky-Golay window %1 must be odd and at least 3").arg(window));
        return false;
    }
    if (order < 0 || order >= window) {
        setError(error, QString("Savitzky-Golay order %1 must be between 0 and window - 1 (%2)").arg(order).arg(window - 1));
        return false;
    }
    if (n < window) {
        setError(error, QString("trace has %1 points, fewer than the window %2").arg(n).arg(window));
        return false;
    }
    const Eigen::VectorXd& x = trace.x();
    const Eigen::VectorXd& y = trace.y();
    const int half = window / 2;

    // uniform axis: the interior weights are the same for every point (classic SG convolution)
    const double step = (x(n - 1) - x(0)) / double(n - 1);
    bool uniform = true;
    for (Eigen::Index i = 1; i < n && uniform; ++i)
        uniform = std::abs((x(i) - x(i - 1)) - step) <= 1e-9 * std::abs(step);
    Eigen::VectorXd interior;
    if (uniform)
        interior = sgWeights(x.segment(0, window), x(half), order);

    Eigen::VectorXd smoothed(n);
    for (Eigen::Index i = 0; i < n; ++i) {
        const Eigen::Index start = std::clamp<Eigen::Index>(i - half, 0, n - window);
        const bool centred = (start == i - half);
        if (uniform && centred)
            smoothed(i) = interior.dot(y.segment(start, window));
        else
            smoothed(i) = sgWeights(x.segment(start, window), x(i), order).dot(y.segment(start, window));
    }
    XYTrace out = trace;
    out.setData(x, smoothed);
    if (result)
        *result = out;
    return true;
}

double PolynomialBaseline::evaluate(double x) const
{
    const double t = (x - center) / scale;
    double value = 0;
    for (Eigen::Index k = coefficients.size() - 1; k >= 0; --k) // Horner
        value = value * t + coefficients(k);
    return value;
}

Eigen::VectorXd PolynomialBaseline::evaluate(const Eigen::VectorXd& x) const
{
    Eigen::VectorXd result(x.size());
    for (Eigen::Index i = 0; i < x.size(); ++i)
        result(i) = evaluate(x(i));
    return result;
}

Eigen::VectorXd PolynomialBaseline::powerCoefficients() const
{
    // sum_k c_k ((x - c)/s)^k = sum_k c_k s^-k sum_j binom(k, j) x^j (-c)^(k-j)
    const Eigen::Index m = coefficients.size();
    Eigen::VectorXd a = Eigen::VectorXd::Zero(m);
    for (Eigen::Index k = 0; k < m; ++k) {
        const double ck = coefficients(k) / std::pow(scale, double(k));
        double binom = 1;
        for (Eigen::Index j = 0; j <= k; ++j) {
            a(j) += ck * binom * std::pow(-center, double(k - j));
            binom = binom * double(k - j) / double(j + 1);
        }
    }
    return a;
}

bool fitPolynomialBaseline(const XYTrace& trace, int degree, const QVector<QPair<double, double>>& excluded,
    PolynomialBaseline* baseline, QString* error)
{
    if (!checkTrace(trace, error))
        return false;
    if (degree < 0 || degree > 10) {
        setError(error, QString("baseline degree %1 must be between 0 and 10").arg(degree));
        return false;
    }
    const Eigen::VectorXd& x = trace.x();
    std::vector<Eigen::Index> used;
    for (Eigen::Index i = 0; i < x.size(); ++i) {
        bool out = false;
        for (const auto& r : excluded)
            out = out || (x(i) >= std::min(r.first, r.second) && x(i) <= std::max(r.first, r.second));
        if (!out)
            used.push_back(i);
    }
    if (Eigen::Index(used.size()) < degree + 1) {
        setError(error, QString("%1 points outside the excluded ranges, a degree %2 baseline needs %3")
                            .arg(used.size())
                            .arg(degree)
                            .arg(degree + 1));
        return false;
    }
    double lo = x(used.front()), hi = x(used.front());
    for (Eigen::Index i : used) {
        lo = std::min(lo, x(i));
        hi = std::max(hi, x(i));
    }
    PolynomialBaseline b;
    b.center = 0.5 * (lo + hi);
    b.scale = 0.5 * (hi - lo);
    if (!(b.scale > 0))
        b.scale = 1;

    Eigen::MatrixXd A(Eigen::Index(used.size()), degree + 1);
    Eigen::VectorXd rhs(Eigen::Index(used.size()));
    for (std::size_t r = 0; r < used.size(); ++r) {
        const double t = (x(used[r]) - b.center) / b.scale;
        double p = 1;
        for (int k = 0; k <= degree; ++k) {
            A(Eigen::Index(r), k) = p;
            p *= t;
        }
        rhs(Eigen::Index(r)) = trace.y()(used[r]);
    }
    Eigen::ColPivHouseholderQR<Eigen::MatrixXd> qr(A);
    if (qr.rank() < degree + 1) {
        setError(error, QString("baseline fit is rank deficient (rank %1 of %2): too few distinct x").arg(qr.rank()).arg(degree + 1));
        return false;
    }
    b.coefficients = qr.solve(rhs);
    if (baseline)
        *baseline = b;
    return true;
}

bool normalise(const XYTrace& trace, NormaliseMode mode, bool useReference, double refStart, double refEnd,
    XYTrace* result, QString* error)
{
    if (!checkTrace(trace, error))
        return false;
    const XYTrace reference = useReference ? trace.range(refStart, refEnd) : trace;
    const Eigen::Index minimum = (mode == NormaliseMode::Area) ? 2 : 1;
    if (reference.size() < minimum) {
        setError(error, QString("the reference window [%1, %2] holds %3 points, %4 needed").arg(refStart).arg(refEnd).arg(reference.size()).arg(minimum));
        return false;
    }
    const Eigen::VectorXd& ry = reference.y();
    Eigen::VectorXd y = trace.y();
    double divisor = 0, offset = 0;
    switch (mode) {
    case NormaliseMode::Max:
        divisor = ry.cwiseAbs().maxCoeff();
        break;
    case NormaliseMode::Area:
        divisor = std::abs(trapezoid(reference));
        break;
    case NormaliseMode::MinMax:
        offset = ry.minCoeff();
        divisor = ry.maxCoeff() - offset;
        break;
    }
    if (!(divisor > 0) || !std::isfinite(divisor)) {
        setError(error, QString("normalisation %1: the reference value is zero").arg(modeName(mode)));
        return false;
    }
    y = (y.array() - offset) / divisor;
    XYTrace out = trace;
    out.setData(trace.x(), y);
    if (result)
        *result = out;
    return true;
}

bool pickPeaks(const XYTrace& trace, double threshold, bool relative, double minDistance, bool integrate,
    QVector<TracePeak>* peaks, QString* error)
{
    if (!checkTrace(trace, error))
        return false;
    if (!std::isfinite(threshold) || (relative && (threshold < 0 || threshold > 1))) {
        setError(error, QString("peak threshold %1 must be finite (relative: in [0, 1])").arg(threshold));
        return false;
    }
    if (!(minDistance >= 0) || !std::isfinite(minDistance)) {
        setError(error, QString("minimal peak distance %1 must be >= 0").arg(minDistance));
        return false;
    }
    QVector<TracePeak> found;
    const Eigen::VectorXd& x = trace.x();
    const Eigen::VectorXd& y = trace.y();
    const Eigen::Index n = y.size();
    if (n < 3) {
        if (peaks)
            peaks->clear();
        return true;
    }
    const double limit = relative ? y.minCoeff() + threshold * (y.maxCoeff() - y.minCoeff()) : threshold;

    for (Eigen::Index i = 1; i < n - 1; ++i) {
        if (!(y(i) > y(i - 1)))
            continue;
        Eigen::Index j = i; // end of a plateau of equal values
        while (j + 1 < n && y(j + 1) == y(i))
            ++j;
        if (j + 1 >= n || !(y(j + 1) < y(i))) {
            i = j;
            continue;
        }
        if (y(i) >= limit) {
            TracePeak peak;
            peak.index = (i + j) / 2;
            peak.x = x(peak.index);
            peak.height = y(peak.index);
            peak.integral = kNaN;
            if (j - i > 1) {
                // longer plateau (saturated detector): its middle
                peak.x = 0.5 * (x(i) + x(j));
            } else {
                // vertex of the parabola y = a u^2 + b u + y_i with u = x - x_i through the neighbours;
                // for two equal points (band centred between them) the vertex lies between them
                const double u0 = x(i - 1) - x(i), u2 = x(i + 1) - x(i);
                const double d0 = y(i - 1) - y(i), d2 = y(i + 1) - y(i);
                const double det = u0 * u0 * u2 - u2 * u2 * u0;
                if (det != 0) {
                    const double a = (d0 * u2 - d2 * u0) / det;
                    const double b = (u0 * u0 * d2 - u2 * u2 * d0) / det;
                    if (a < 0) {
                        const double u = std::clamp(-b / (2 * a), std::min(u0, u2), std::max(u0, u2));
                        peak.x = x(i) + u;
                        peak.height = y(i) + a * u * u + b * u;
                    }
                }
            }
            found << peak;
        }
        i = j;
    }

    // thinning: keep the higher of two peaks closer than minDistance
    if (minDistance > 0 && found.size() > 1) {
        QVector<TracePeak> byHeight = found;
        std::stable_sort(byHeight.begin(), byHeight.end(), [](const TracePeak& a, const TracePeak& b) { return a.height > b.height; });
        QVector<TracePeak> kept;
        for (const TracePeak& p : byHeight) {
            bool far = true;
            for (const TracePeak& k : kept)
                far = far && std::abs(p.x - k.x) >= minDistance;
            if (far)
                kept << p;
        }
        std::sort(kept.begin(), kept.end(), [](const TracePeak& a, const TracePeak& b) { return a.index < b.index; });
        found = kept;
    }

    for (TracePeak& p : found) {
        Eigen::Index left = p.index, right = p.index;
        while (left > 0 && y(left - 1) <= y(left))
            --left;
        while (right < n - 1 && y(right + 1) <= y(right))
            ++right;
        p.left = left;
        p.right = right;
        if (integrate)
            p.integral = trapezoid(trace, left, right);
    }
    if (peaks)
        *peaks = found;
    return true;
}

} // namespace TraceOps

// ---------------------------------------------------------------------------------------------
// Operations

std::unique_ptr<TraceOp> TraceOp::fromJson(const QJsonObject& json, QString* error)
{
    const QString type = json.value("Type").toString();
    std::unique_ptr<TraceOp> op;
    if (type == "Range") {
        auto r = std::make_unique<RangeOp>();
        if (!readNumber(json, "XStart", &r->xStart, error, true) || !readNumber(json, "XEnd", &r->xEnd, error, true))
            return nullptr;
        op = std::move(r);
    } else if (type == "Smooth") {
        auto s = std::make_unique<SmoothOp>();
        const QString method = json.value("Method").toString("SavitzkyGolay");
        if (method != "SavitzkyGolay") {
            setError(error, QString("unknown smoothing method '%1'").arg(method));
            return nullptr;
        }
        if (!readInt(json, "Window", &s->window, error) || !readInt(json, "Order", &s->order, error))
            return nullptr;
        op = std::move(s);
    } else if (type == "Normalise") {
        auto nrm = std::make_unique<NormaliseOp>();
        const QString mode = json.value("Mode").toString("Max");
        if (mode == "Max")
            nrm->mode = TraceOps::NormaliseMode::Max;
        else if (mode == "Area")
            nrm->mode = TraceOps::NormaliseMode::Area;
        else if (mode == "MinMax")
            nrm->mode = TraceOps::NormaliseMode::MinMax;
        else {
            setError(error, QString("unknown normalisation mode '%1'").arg(mode));
            return nullptr;
        }
        nrm->useReference = json.contains("RefStart") || json.contains("RefEnd");
        if (nrm->useReference && (!readNumber(json, "RefStart", &nrm->refStart, error, true) || !readNumber(json, "RefEnd", &nrm->refEnd, error, true)))
            return nullptr;
        op = std::move(nrm);
    } else if (type == "Baseline") {
        auto b = std::make_unique<BaselineOp>();
        if (!readInt(json, "Degree", &b->degree, error))
            return nullptr;
        const QJsonValue excluded = json.value("Excluded");
        if (!excluded.isUndefined() && !excluded.isArray()) {
            setError(error, "parameter Excluded is not an array");
            return nullptr;
        }
        for (const QJsonValue& v : excluded.toArray()) {
            const QJsonArray pair = v.toArray();
            if (pair.size() != 2 || !pair[0].isDouble() || !pair[1].isDouble()) {
                setError(error, "an excluded range is not a pair of numbers");
                return nullptr;
            }
            b->excluded << qMakePair(pair[0].toDouble(), pair[1].toDouble());
        }
        op = std::move(b);
    } else if (type == "PickPeaks") {
        auto p = std::make_unique<PickPeaksOp>();
        if (!readNumber(json, "Threshold", &p->threshold, error, false) || !readNumber(json, "MinDistance", &p->minDistance, error, false))
            return nullptr;
        const QString mode = json.value("ThresholdMode").toString("Relative");
        if (mode != "Relative" && mode != "Absolute") {
            setError(error, QString("unknown threshold mode '%1'").arg(mode));
            return nullptr;
        }
        p->relative = (mode == "Relative");
        p->integrate = json.value("Integrate").toBool(false);
        op = std::move(p);
    } else {
        setError(error, type.isEmpty() ? QString("operation without Type") : QString("unknown operation type '%1'").arg(type));
        return nullptr;
    }
    QString reason;
    if (!op->validate(&reason)) {
        setError(error, QString("%1: %2").arg(type, reason));
        return nullptr;
    }
    op->enabled = json.value("Enabled").toBool(true);
    return op;
}

bool RangeOp::validate(QString* error) const
{
    if (!std::isfinite(xStart) || !std::isfinite(xEnd)) {
        setError(error, "range bounds must be finite");
        return false;
    }
    return true;
}

XYTrace RangeOp::apply(const XYTrace& trace, QString* error) const
{
    if (!validate(error))
        return XYTrace();
    return trace.range(xStart, xEnd);
}

QJsonObject RangeOp::toJson() const
{
    return QJsonObject{ { "Type", type() }, { "XStart", xStart }, { "XEnd", xEnd } };
}

bool SmoothOp::validate(QString* error) const
{
    if (window < 3 || window % 2 == 0) {
        setError(error, QString("window %1 must be odd and at least 3").arg(window));
        return false;
    }
    if (order < 0 || order >= window) {
        setError(error, QString("order %1 must be between 0 and window - 1").arg(order));
        return false;
    }
    return true;
}

XYTrace SmoothOp::apply(const XYTrace& trace, QString* error) const
{
    XYTrace result;
    if (!TraceOps::savitzkyGolay(trace, window, order, &result, error))
        return XYTrace();
    return result;
}

QJsonObject SmoothOp::toJson() const
{
    return QJsonObject{ { "Type", type() }, { "Method", "SavitzkyGolay" }, { "Window", window }, { "Order", order } };
}

bool NormaliseOp::validate(QString* error) const
{
    if (useReference && (!std::isfinite(refStart) || !std::isfinite(refEnd))) {
        setError(error, "reference window bounds must be finite");
        return false;
    }
    return true;
}

XYTrace NormaliseOp::apply(const XYTrace& trace, QString* error) const
{
    XYTrace result;
    if (!validate(error) || !TraceOps::normalise(trace, mode, useReference, refStart, refEnd, &result, error))
        return XYTrace();
    return result;
}

QJsonObject NormaliseOp::toJson() const
{
    QJsonObject json{ { "Type", type() }, { "Mode", modeName(mode) } };
    if (useReference) {
        json["RefStart"] = refStart;
        json["RefEnd"] = refEnd;
    }
    return json;
}

bool BaselineOp::validate(QString* error) const
{
    if (degree < 0 || degree > 10) {
        setError(error, QString("degree %1 must be between 0 and 10").arg(degree));
        return false;
    }
    for (const auto& r : excluded)
        if (!std::isfinite(r.first) || !std::isfinite(r.second)) {
            setError(error, "excluded ranges must be finite");
            return false;
        }
    return true;
}

XYTrace BaselineOp::apply(const XYTrace& trace, QString* error) const
{
    TraceOps::PolynomialBaseline baseline;
    if (!validate(error) || !TraceOps::fitPolynomialBaseline(trace, degree, excluded, &baseline, error))
        return XYTrace();
    XYTrace result = trace;
    result.setData(trace.x(), trace.y() - baseline.evaluate(trace.x()));
    return result;
}

QJsonObject BaselineOp::toJson() const
{
    QJsonArray ranges;
    for (const auto& r : excluded)
        ranges.append(QJsonArray{ r.first, r.second });
    return QJsonObject{ { "Type", type() }, { "Degree", degree }, { "Excluded", ranges } };
}

bool PickPeaksOp::validate(QString* error) const
{
    if (!std::isfinite(threshold) || (relative && (threshold < 0 || threshold > 1))) {
        setError(error, QString("threshold %1 must be finite (relative: in [0, 1])").arg(threshold));
        return false;
    }
    if (!(minDistance >= 0) || !std::isfinite(minDistance)) {
        setError(error, QString("minimal distance %1 must be >= 0").arg(minDistance));
        return false;
    }
    return true;
}

XYTrace PickPeaksOp::apply(const XYTrace& trace, QString* error) const
{
    if (!validate(error))
        return XYTrace();
    return trace;
}

QVector<TracePeak> PickPeaksOp::pick(const XYTrace& trace, QString* error) const
{
    QVector<TracePeak> peaks;
    TraceOps::pickPeaks(trace, threshold, relative, minDistance, integrate, &peaks, error);
    return peaks;
}

QJsonObject PickPeaksOp::toJson() const
{
    return QJsonObject{ { "Type", type() }, { "Threshold", threshold }, { "ThresholdMode", relative ? "Relative" : "Absolute" },
        { "MinDistance", minDistance }, { "Integrate", integrate } };
}

// ---------------------------------------------------------------------------------------------
// Pipeline

TraceOpList::TraceOpList(const TraceOpList& other)
{
    *this = other;
}

TraceOpList& TraceOpList::operator=(const TraceOpList& other)
{
    if (this == &other)
        return *this;
    std::vector<std::unique_ptr<TraceOp>> ops;
    ops.reserve(other.m_ops.size());
    for (const auto& op : other.m_ops)
        ops.push_back(op->clone());
    m_ops = std::move(ops);
    return *this;
}

void TraceOpList::append(std::unique_ptr<TraceOp> op)
{
    if (op)
        m_ops.push_back(std::move(op));
}

bool TraceOpList::remove(int i)
{
    if (i < 0 || i >= size())
        return false;
    m_ops.erase(m_ops.begin() + i);
    return true;
}

bool TraceOpList::move(int from, int to)
{
    if (from < 0 || to < 0 || from >= size() || to >= size())
        return false;
    auto op = std::move(m_ops[from]);
    m_ops.erase(m_ops.begin() + from);
    m_ops.insert(m_ops.begin() + to, std::move(op));
    return true;
}

TraceOp* TraceOpList::at(int i)
{
    Q_ASSERT(i >= 0 && i < size());
    return m_ops[i].get();
}

const TraceOp* TraceOpList::at(int i) const
{
    Q_ASSERT(i >= 0 && i < size());
    return m_ops[i].get();
}

XYTrace TraceOpList::apply(const XYTrace& raw, QString* error, QVector<TracePeak>* peaks) const
{
    XYTrace current = raw;
    for (int i = 0; i < size(); ++i) {
        const TraceOp* op = m_ops[i].get();
        if (!op->enabled)
            continue;
        QString reason;
        if (auto pick = dynamic_cast<const PickPeaksOp*>(op)) {
            QVector<TracePeak> found;
            if (!TraceOps::pickPeaks(current, pick->threshold, pick->relative, pick->minDistance, pick->integrate, &found, &reason)) {
                setError(error, QString("step %1 (%2): %3").arg(i + 1).arg(op->type(), reason));
                return XYTrace();
            }
            if (peaks)
                *peaks = found;
            continue;
        }
        XYTrace next = op->apply(current, &reason);
        if (!reason.isEmpty()) {
            setError(error, QString("step %1 (%2): %3").arg(i + 1).arg(op->type(), reason));
            return XYTrace();
        }
        current = next;
    }
    return current;
}

QJsonObject TraceOpList::toJson() const
{
    QJsonArray ops;
    for (const auto& op : m_ops) {
        QJsonObject step = op->toJson();
        if (!op->enabled) // only written when off: lists without switched-off steps keep their JSON
            step["Enabled"] = false;
        ops.append(step);
    }
    return QJsonObject{ { "Version", Version }, { "Operations", ops } };
}

bool TraceOpList::fromJson(const QJsonObject& json, TraceOpList* list, QString* error)
{
    const QJsonValue version = json.value("Version");
    if (!version.isDouble()) {
        setError(error, "operation list without Version");
        return false;
    }
    if (version.toInt() > Version || version.toInt() < 1) {
        setError(error, QString("operation list version %1 is not supported (this SupraFit reads up to %2)").arg(version.toInt()).arg(Version));
        return false;
    }
    if (!json.value("Operations").isArray()) {
        setError(error, "operation list without Operations array");
        return false;
    }
    TraceOpList result;
    const QJsonArray ops = json.value("Operations").toArray();
    for (int i = 0; i < ops.size(); ++i) {
        if (!ops[i].isObject()) {
            setError(error, QString("operation %1 is not an object").arg(i + 1));
            return false;
        }
        QString reason;
        auto op = TraceOp::fromJson(ops[i].toObject(), &reason);
        if (!op) {
            setError(error, QString("operation %1: %2").arg(i + 1).arg(reason));
            return false;
        }
        result.append(std::move(op));
    }
    if (list)
        *list = std::move(result);
    return true;
}
