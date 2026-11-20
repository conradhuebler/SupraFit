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

#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

#include <QtCore/QFileInfo>
#include <QtCore/QPair>
#include <QtCore/QVector>
#include <QtCore/QJsonValue>
#include <QtCore/QStringList>

#include "jeolreader.h"
#include "tracejson.h"

#include "fidprocessing.h"

namespace {
const double kPi = 3.14159265358979323846;
const double kDeg = kPi / 180.0;
/// weight of the negative-intensity penalty of the ACME cost
const double kAcmePenalty = 100.0;
/// size of the reduced spectrum of the coarse phase search
const Eigen::Index kCoarseSize = 8192;
/// size of the reduced spectrum of the searches from the grid minima (the winner is polished on the full one)
const Eigen::Index kMediumSize = 65536;

void setError(QString* error, const QString& message)
{
    if (error)
        *error = message;
}

/// angle in (-180, 180]
double wrapDegrees(double angle)
{
    double a = std::fmod(angle, 360.0);
    if (a <= -180.0)
        a += 360.0;
    else if (a > 180.0)
        a -= 360.0;
    return a;
}

/// ACME cost of the spectrum phased by (ph0, ph1)
/// ACME cost of the spectrum phased by θ_k = ph0 + ph1 · relative_k (degrees). relative is uniform (an
/// FFT axis or its block means), so the rotation is a recurrence exp(iθ_{k+1}) = exp(iθ_k) exp(iΔ), set
/// exactly every 256 points; same result as acmeCost(phase(...)) to rounding, without a cos/sin per point
double phaseCost(const Eigen::VectorXcd& spectrum, const Eigen::VectorXd& relative, double scale, double ph0, double ph1)
{
    const Eigen::Index m = spectrum.size();
    if (m < 3)
        return FidProcessing::acmeCost(FidProcessing::phase(spectrum, relative, 1.0, 1.0, ph0, ph1, 0.0), scale);
    thread_local Eigen::VectorXd real;
    real.resize(m);
    const double step = (relative(m - 1) - relative(0)) / double(m - 1);
    const std::complex<double> w = std::polar(1.0, ph1 * kDeg * step);
    const std::complex<double>* s = spectrum.data();
    double* r = real.data();
    std::complex<double> z;
    for (Eigen::Index k = 0; k < m; ++k) {
        if ((k & 255) == 0)
            z = std::polar(1.0, (ph0 + ph1 * relative(k)) * kDeg);
        r[k] = s[k].real() * z.real() - s[k].imag() * z.imag();
        z *= w;
    }
    return FidProcessing::acmeCost(real, scale);
}

/// minimises f over 2 parameters (Nelder-Mead with standard coefficients); returns the best point
template <typename F>
Eigen::Vector2d nelderMead(F f, const Eigen::Vector2d& start, double step, double tolerance, int maxEvaluations, int* evaluations)
{
    Eigen::Vector2d p[3] = { start, start + Eigen::Vector2d(step, 0), start + Eigen::Vector2d(0, step) };
    double v[3];
    for (int i = 0; i < 3; ++i)
        v[i] = f(p[i]);
    int n = 3;
    while (n < maxEvaluations) {
        // order: best p[0], worst p[2]
        for (int i = 0; i < 3; ++i)
            for (int j = i + 1; j < 3; ++j)
                if (v[j] < v[i]) {
                    std::swap(v[i], v[j]);
                    std::swap(p[i], p[j]);
                }
        const double size = std::max((p[1] - p[0]).cwiseAbs().maxCoeff(), (p[2] - p[0]).cwiseAbs().maxCoeff());
        if (size < tolerance)
            break;
        const Eigen::Vector2d c = 0.5 * (p[0] + p[1]);
        auto along = [&](double t) { return Eigen::Vector2d(c + t * (p[2] - c)); };
        const Eigen::Vector2d r = along(-1.0);
        const double vr = f(r);
        ++n;
        if (vr < v[0]) {
            const Eigen::Vector2d e = along(-2.0);
            const double ve = f(e);
            ++n;
            if (ve < vr) {
                p[2] = e;
                v[2] = ve;
            } else {
                p[2] = r;
                v[2] = vr;
            }
        } else if (vr < v[1]) {
            p[2] = r;
            v[2] = vr;
        } else {
            const Eigen::Vector2d k = vr < v[2] ? along(-0.5) : along(0.5);
            const double vk = f(k);
            ++n;
            if (vk < std::min(vr, v[2])) {
                p[2] = k;
                v[2] = vk;
            } else {
                // shrink towards the best point
                for (int i = 1; i < 3; ++i) {
                    p[i] = p[0] + 0.5 * (p[i] - p[0]);
                    v[i] = f(p[i]);
                    ++n;
                }
            }
        }
    }
    int best = 0;
    for (int i = 1; i < 3; ++i)
        if (v[i] < v[best])
            best = i;
    if (evaluations)
        *evaluations += n;
    return p[best];
}
} // namespace

// ---------------------------------------------------------------------------------------------
// Fid

bool Fid::isValid(QString* reason) const
{
    auto fail = [reason](const QString& text) {
        setError(reason, text);
        return false;
    };
    if (points.size() < 2)
        return fail(QStringLiteral("fewer than two points"));
    if (!points.allFinite())
        return fail(QStringLiteral("points that are not finite numbers"));
    if (!(sweep > 0) || !std::isfinite(sweep))
        return fail(QString("spectral width %1 Hz").arg(sweep));
    if (!(frequency > 0) || !std::isfinite(frequency))
        return fail(QString("spectrometer frequency %1 MHz").arg(frequency));
    if (!std::isfinite(offset))
        return fail(QStringLiteral("no carrier position"));
    if (!(groupDelay >= 0) || groupDelay >= double(points.size()))
        return fail(QString("group delay %1 points").arg(groupDelay));
    return true;
}

QJsonObject Fid::toJson() const
{
    QJsonObject json;
    json["Re"] = TraceJson::vectorToString(points.real());
    json["Im"] = TraceJson::vectorToString(points.imag());
    json["Sweep"] = sweep;
    json["Frequency"] = frequency;
    json["Offset"] = offset;
    json["GroupDelay"] = groupDelay;
    json["Name"] = name;
    if (!nucleus.isEmpty())
        json["Nucleus"] = nucleus;
    if (!description.isEmpty())
        json["Description"] = description;
    return json;
}

bool Fid::fromJson(const QJsonObject& json, Fid* fid, QString* error)
{
    Eigen::VectorXd re, im;
    QString reason;
    if (!TraceJson::vectorFromJson(json.value("Re"), &re, &reason) || !TraceJson::vectorFromJson(json.value("Im"), &im, &reason)) {
        setError(error, QString("stored FID: %1").arg(reason));
        return false;
    }
    if (re.size() != im.size()) {
        setError(error, QStringLiteral("stored FID: real and imaginary parts of different length"));
        return false;
    }
    Fid result;
    result.points.resize(re.size());
    result.points.real() = re;
    result.points.imag() = im;
    result.sweep = json.value("Sweep").toDouble();
    result.frequency = json.value("Frequency").toDouble();
    result.offset = json.value("Offset").toDouble(std::numeric_limits<double>::quiet_NaN());
    result.groupDelay = json.value("GroupDelay").toDouble(0);
    result.name = json.value("Name").toString();
    result.nucleus = json.value("Nucleus").toString();
    result.description = json.value("Description").toString();
    if (!result.isValid(&reason)) {
        setError(error, QString("stored FID: %1").arg(reason));
        return false;
    }
    if (fid)
        *fid = result;
    return true;
}

// ---------------------------------------------------------------------------------------------
// FidSettings

QJsonObject FidSettings::toJson() const
{
    QJsonObject json;
    json["Version"] = Version;
    json["GroupDelayCorrection"] = groupDelayCorrection;
    json["LineBroadening"] = lineBroadening;
    json["GaussBroadening"] = gaussBroadening;
    json["ZeroFill"] = zeroFill;
    json["Ph0"] = ph0;
    json["Ph1"] = ph1;
    if (std::isfinite(pivot))
        json["Pivot"] = pivot;
    json["AutoPhase"] = autoPhase;
    json["AutoFirstOrder"] = autoFirstOrder;
    return json;
}

bool FidSettings::fromJson(const QJsonObject& json, FidSettings* settings, QString* error)
{
    const int version = json.value("Version").toInt(Version);
    if (version > Version) {
        setError(error, QString("FID processing of version %1, this SupraFit reads up to %2").arg(version).arg(Version));
        return false;
    }
    FidSettings s;
    s.groupDelayCorrection = json.value("GroupDelayCorrection").toBool(s.groupDelayCorrection);
    s.lineBroadening = json.value("LineBroadening").toDouble(s.lineBroadening);
    s.gaussBroadening = json.value("GaussBroadening").toDouble(s.gaussBroadening);
    s.zeroFill = json.value("ZeroFill").toInt(s.zeroFill);
    s.ph0 = json.value("Ph0").toDouble(s.ph0);
    s.ph1 = json.value("Ph1").toDouble(s.ph1);
    s.pivot = json.contains("Pivot") ? json.value("Pivot").toDouble(s.pivot) : s.pivot;
    s.autoPhase = json.value("AutoPhase").toBool(s.autoPhase);
    s.autoFirstOrder = json.value("AutoFirstOrder").toBool(s.autoFirstOrder);
    QString reason;
    if (!s.isValid(&reason)) {
        setError(error, QString("FID processing: %1").arg(reason));
        return false;
    }
    if (settings)
        *settings = s;
    return true;
}

bool FidSettings::isValid(QString* reason) const
{
    if (!(lineBroadening >= 0) || !std::isfinite(lineBroadening) || !(gaussBroadening >= 0) || !std::isfinite(gaussBroadening)) {
        setError(reason, QStringLiteral("line broadening must be zero or positive"));
        return false;
    }
    if (zeroFill < 1 || zeroFill > 16) {
        setError(reason, QString("zero filling factor %1 (1 to 16)").arg(zeroFill));
        return false;
    }
    if (!std::isfinite(ph0) || !std::isfinite(ph1)) {
        setError(reason, QStringLiteral("phases must be numbers"));
        return false;
    }
    return true;
}

bool FidSettings::operator==(const FidSettings& other) const
{
    const bool samePivot = (std::isnan(pivot) && std::isnan(other.pivot)) || pivot == other.pivot;
    return groupDelayCorrection == other.groupDelayCorrection && lineBroadening == other.lineBroadening && gaussBroadening == other.gaussBroadening
        && zeroFill == other.zeroFill && ph0 == other.ph0 && ph1 == other.ph1 && samePivot && autoPhase == other.autoPhase && autoFirstOrder == other.autoFirstOrder;
}

// ---------------------------------------------------------------------------------------------
// processing

namespace FidProcessing {

bool isPowerOfTwo(Eigen::Index n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

Eigen::Index paddedSize(Eigen::Index n, int factor)
{
    factor = std::clamp(factor, 1, 16);
    if (n < 1 || n > MaxSize)
        return 0;
    const Eigen::Index target = n * factor;
    Eigen::Index m = 1;
    while (m < target)
        m <<= 1;
    return m > MaxSize ? 0 : m;
}

bool fft(Eigen::VectorXcd& data, bool inverse)
{
    const Eigen::Index n = data.size();
    if (!isPowerOfTwo(n))
        return false;
    // bit reversal
    for (Eigen::Index i = 1, j = 0; i < n; ++i) {
        Eigen::Index bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j |= bit;
        if (i < j)
            std::swap(data(i), data(j));
    }
    // twiddles exp(∓2πi k/n), each computed directly (no accumulated rounding of a recursion)
    const double sign = inverse ? 1.0 : -1.0;
    std::vector<std::complex<double>> twiddle(std::max<Eigen::Index>(1, n / 2));
    for (Eigen::Index k = 0; k < n / 2; ++k)
        twiddle[k] = std::polar(1.0, sign * 2.0 * kPi * double(k) / double(n));
    std::complex<double>* a = data.data();
    for (Eigen::Index length = 2; length <= n; length <<= 1) {
        const Eigen::Index half = length / 2, stride = n / length;
        for (Eigen::Index start = 0; start < n; start += length) {
            for (Eigen::Index k = 0; k < half; ++k) {
                const std::complex<double> t = twiddle[k * stride] * a[start + k + half];
                a[start + k + half] = a[start + k] - t;
                a[start + k] += t;
            }
        }
    }
    if (inverse)
        data /= double(n);
    return true;
}

bool complexSpectrum(const Fid& fid, const FidSettings& settings, Eigen::VectorXcd* spectrum, Eigen::VectorXd* ppm, QString* error)
{
    QString reason;
    if (!fid.isValid(&reason) || !settings.isValid(&reason)) {
        setError(error, QString("%1: %2").arg(fid.name.isEmpty() ? QStringLiteral("FID") : fid.name, reason));
        return false;
    }
    const Eigen::Index n = fid.points.size();
    const Eigen::Index m = paddedSize(n, settings.zeroFill);
    if (m == 0) {
        setError(error, QString("%1: %2 points x %3 exceed the largest spectrum of %4 points").arg(fid.name).arg(n).arg(settings.zeroFill).arg(MaxSize));
        return false;
    }
    const double delay = settings.groupDelayCorrection ? fid.groupDelay : 0.0;
    const double dwell = 1.0 / fid.sweep;
    Eigen::VectorXcd data = Eigen::VectorXcd::Zero(m);
    const double gauss = settings.gaussBroadening > 0 ? (kPi * settings.gaussBroadening) * (kPi * settings.gaussBroadening) / (4.0 * std::log(2.0)) : 0.0;
    for (Eigen::Index j = 0; j < n; ++j) {
        const double t = std::max(0.0, (double(j) - fid.groupDelay) * dwell);
        double w = 1.0;
        if (settings.lineBroadening > 0)
            w *= std::exp(-kPi * settings.lineBroadening * t);
        if (gauss > 0)
            w *= std::exp(-gauss * t * t);
        data(j) = fid.points(j) * w;
    }
    if (fid.groupDelay == 0)
        data(0) *= 0.5;
    fft(data);
    // reorder: index k of the output = frequency (k - m/2) · sweep / m, ascending
    Eigen::VectorXcd shifted(m);
    shifted.head(m / 2) = data.tail(m / 2);
    shifted.tail(m / 2) = data.head(m / 2);
    Eigen::VectorXd axis(m);
    for (Eigen::Index k = 0; k < m; ++k) {
        const double nu = double(k - m / 2) * fid.sweep / double(m);
        axis(k) = fid.offset + nu / fid.frequency;
        if (delay != 0)
            shifted(k) *= std::polar(1.0, 2.0 * kPi * nu * delay * dwell);
    }
    if (spectrum)
        *spectrum = std::move(shifted);
    if (ppm)
        *ppm = std::move(axis);
    return true;
}

Eigen::VectorXd phase(const Eigen::VectorXcd& spectrum, const Eigen::VectorXd& ppm, double frequency, double sweep, double ph0, double ph1, double pivot)
{
    const Eigen::Index m = spectrum.size();
    Eigen::VectorXd real(m);
    const double slope = ph1 * kDeg * frequency / sweep; // radian per ppm
    for (Eigen::Index k = 0; k < m; ++k) {
        const double theta = ph0 * kDeg + slope * (ppm(k) - pivot);
        real(k) = spectrum(k).real() * std::cos(theta) - spectrum(k).imag() * std::sin(theta);
    }
    return real;
}

double acmeCost(const Eigen::VectorXd& real, double scale)
{
    const Eigen::Index m = real.size();
    if (m < 3 || !(scale > 0))
        return 0;
    double sum = 0;
    for (Eigen::Index k = 1; k + 1 < m; ++k)
        sum += std::abs(real(k + 1) - real(k - 1));
    double entropy = 0;
    if (sum > 0) {
        for (Eigen::Index k = 1; k + 1 < m; ++k) {
            const double h = std::abs(real(k + 1) - real(k - 1)) / sum;
            if (h > 0)
                entropy -= h * std::log(h);
        }
    }
    double penalty = 0;
    for (Eigen::Index k = 0; k < m; ++k) {
        if (real(k) < 0) {
            const double r = real(k) / scale;
            penalty += r * r;
        }
    }
    return entropy + kAcmePenalty * penalty;
}

FidPhase autoPhase(const Eigen::VectorXcd& spectrum, const Eigen::VectorXd& ppm, double frequency, double sweep, double pivot,
    bool firstOrder, double ph1Start, double ph1Range)
{
    FidPhase result;
    result.pivot = pivot;
    result.automatic = true;
    result.ph1 = ph1Start;
    const Eigen::Index m = spectrum.size();
    if (m < 4 || ppm.size() != m || !(sweep > 0) || !(frequency > 0))
        return result;
    // relative position (ν - ν_pivot)/sweep: phase() with frequency = sweep = 1 and pivot 0 takes it as "ppm"
    Eigen::VectorXd relative = (ppm.array() - pivot) * frequency / sweep;

    // reduced spectra: sums of blocks of neighbouring points (the sum of a line's complex points keeps its phase)
    struct Reduced {
        Eigen::VectorXcd spectrum;
        Eigen::VectorXd relative;
        double scale = 0;
    };
    auto reduce = [&](Eigen::Index size) {
        Reduced r;
        const Eigen::Index block = std::max<Eigen::Index>(1, m / size);
        if (block == 1) {
            r.spectrum = spectrum;
            r.relative = relative;
        } else {
            const Eigen::Index n = m / block;
            r.spectrum.resize(n);
            r.relative.resize(n);
            for (Eigen::Index i = 0; i < n; ++i) {
                r.spectrum(i) = spectrum.segment(i * block, block).sum();
                r.relative(i) = relative.segment(i * block, block).mean();
            }
        }
        r.scale = r.spectrum.cwiseAbs().maxCoeff();
        return r;
    };
    const Reduced coarse = reduce(kCoarseSize);
    const Reduced medium = reduce(kMediumSize);
    const double scale = spectrum.cwiseAbs().maxCoeff();
    if (!(scale > 0))
        return result;
    const double range = std::max(0.0, ph1Range);
    const int steps1 = firstOrder ? int(std::ceil(range / 30.0)) : 0;
    const int rows = 2 * steps1 + 1, cols = 36;
    Eigen::MatrixXd grid(rows, cols);
    for (int b = 0; b < rows; ++b) {
        const double p1 = firstOrder ? (b - steps1) * 30.0 : ph1Start;
        for (int a = 0; a < cols; ++a) {
            grid(b, a) = phaseCost(coarse.spectrum, coarse.relative, coarse.scale, a * 10.0 - 180.0, p1);
            ++result.evaluations;
        }
    }
    // starts: the lowest local minima of the grid (ph0 wraps around); a few lines leave several basins
    // of almost equal cost, so the fine search runs from each of the best kStarts
    const int kStarts = 4;
    QVector<QPair<double, QPair<int, int>>> minima;
    for (int b = 0; b < rows; ++b)
        for (int a = 0; a < cols; ++a) {
            bool minimum = true;
            for (int db = -1; db <= 1 && minimum; ++db)
                for (int da = -1; da <= 1 && minimum; ++da) {
                    const int bb = b + db, aa = (a + da + cols) % cols;
                    if ((db != 0 || da != 0) && bb >= 0 && bb < rows && grid(bb, aa) < grid(b, a))
                        minimum = false;
                }
            if (minimum)
                minima.append({ grid(b, a), { b, a } });
        }
    std::sort(minima.begin(), minima.end(), [](const auto& x, const auto& y) { return x.first < y.first; });
    if (minima.size() > kStarts)
        minima.resize(kStarts);

    // ph1 is kept inside ± range (a quadratic wall outside): lines in a narrow region leave ph0 and ph1
    // almost exchangeable, and the simplex can otherwise walk to several hundred degrees
    auto cost2 = [&](const Reduced& r, const Eigen::Vector2d& p) {
        const double outside = std::max(0.0, std::abs(p(1)) - range);
        return phaseCost(r.spectrum, r.relative, r.scale, p(0), p(1)) + outside * outside;
    };
    // golden section of the zero-order phase in [lo, hi]
    auto golden = [&](const Reduced& r, double lo, double hi) {
        const double g = 0.5 * (std::sqrt(5.0) - 1.0);
        double x1 = hi - g * (hi - lo), x2 = lo + g * (hi - lo);
        double f1 = phaseCost(r.spectrum, r.relative, r.scale, x1, ph1Start), f2 = phaseCost(r.spectrum, r.relative, r.scale, x2, ph1Start);
        result.evaluations += 2;
        while (hi - lo > 0.01) {
            if (f1 < f2) {
                hi = x2;
                x2 = x1;
                f2 = f1;
                x1 = hi - g * (hi - lo);
                f1 = phaseCost(r.spectrum, r.relative, r.scale, x1, ph1Start);
            } else {
                lo = x1;
                x1 = x2;
                f1 = f2;
                x2 = lo + g * (hi - lo);
                f2 = phaseCost(r.spectrum, r.relative, r.scale, x2, ph1Start);
            }
            ++result.evaluations;
        }
        return 0.5 * (lo + hi);
    };

    // fine: from every start on the medium spectrum (<= kMediumSize points)
    double best = std::numeric_limits<double>::infinity();
    double best0 = 0, best1 = ph1Start;
    for (const auto& start : minima) {
        const double s0 = start.second.second * 10.0 - 180.0;
        const double s1 = firstOrder ? (start.second.first - steps1) * 30.0 : ph1Start;
        double p0 = s0, p1 = s1, cost = 0;
        if (firstOrder) {
            auto f = [&](const Eigen::Vector2d& p) { return cost2(medium, p); };
            // restarted from its own result: the valley along ph0 + ph1·x of the lines is narrow and a
            // simplex can shrink in it before it reaches the bottom
            Eigen::Vector2d p = nelderMead(f, Eigen::Vector2d(s0, s1), 10.0, 0.01, 400, &result.evaluations);
            p = nelderMead(f, p, 2.0, 0.01, 400, &result.evaluations);
            p0 = p(0);
            p1 = p(1);
            cost = f(p);
        } else {
            p0 = golden(medium, s0 - 10.0, s0 + 10.0);
            cost = phaseCost(medium.spectrum, medium.relative, medium.scale, p0, ph1Start);
        }
        ++result.evaluations;
        if (cost < best) {
            best = cost;
            best0 = p0;
            best1 = p1;
        }
    }
    // last: the winner polished on the full spectrum, if the medium one was reduced
    if (medium.spectrum.size() < m) {
        const Reduced full{ spectrum, relative, scale };
        if (firstOrder) {
            auto f = [&](const Eigen::Vector2d& p) { return cost2(full, p); };
            const Eigen::Vector2d p = nelderMead(f, Eigen::Vector2d(best0, best1), 1.0, 0.01, 200, &result.evaluations);
            best0 = p(0);
            best1 = p(1);
        } else
            best0 = golden(full, best0 - 1.0, best0 + 1.0);
    }
    result.ph0 = wrapDegrees(best0);
    result.ph1 = best1;
    return result;
}

bool process(const Fid& fid, const FidSettings& settings, XYTrace* spectrum, FidPhase* used, QString* error)
{
    Eigen::VectorXcd complex;
    Eigen::VectorXd ppm;
    if (!complexSpectrum(fid, settings, &complex, &ppm, error)) {
        if (spectrum)
            *spectrum = XYTrace();
        return false;
    }
    const double pivot = std::isfinite(settings.pivot) ? settings.pivot : fid.offset;
    FidPhase applied;
    if (settings.autoPhase)
        applied = autoPhase(complex, ppm, fid.frequency, fid.sweep, pivot, settings.autoFirstOrder, settings.ph1);
    else {
        applied.ph0 = settings.ph0;
        applied.ph1 = settings.ph1;
        applied.pivot = pivot;
    }
    const Eigen::VectorXd real = phase(complex, ppm, fid.frequency, fid.sweep, applied.ph0, applied.ph1, pivot);
    if (spectrum) {
        // descending ppm (NMR convention)
        XYTrace trace(ppm.reverse(), real.reverse(), fid.name);
        trace.setSource(fid.source);
        trace.setXAxis(QStringLiteral("δ"), QStringLiteral("ppm"));
        *spectrum = trace;
    }
    if (used)
        *used = applied;
    return true;
}

bool fromJeol(const JeolReader::Data& data, Fid* fid, QString* error)
{
    const QString name = QFileInfo(data.source).fileName();
    auto fail = [&](const QString& reason) {
        setError(error, QString("%1: %2").arg(name.isEmpty() ? QStringLiteral("JEOL file") : name, reason));
        return false;
    };
    if (!data.isTimeDomain())
        return fail(QStringLiteral("the data are not a FID (time domain); processed JEOL spectra are not supported"));
    if (data.axisType != 3)
        return fail(QStringLiteral("a real-only FID cannot be transformed (complex data expected)"));
    Fid result;
    result.points = data.values.conjugate();
    result.sweep = data.sweep;
    result.frequency = data.frequency;
    result.offset = data.offset;
    // the delay counts from the first stored point; values holds the valid range only
    result.groupDelay = data.digitalFilter && std::isfinite(data.groupDelay) ? std::max(0.0, data.groupDelay - double(data.firstPoint)) : 0.0;
    result.name = name;
    result.source = data.source;
    result.nucleus = data.nucleus;
    QStringList parts;
    if (!data.instrument.isEmpty())
        parts << data.instrument;
    if (!data.nucleus.isEmpty())
        parts << QString("%1 %2 MHz").arg(data.nucleus).arg(data.frequency, 0, 'f', 2);
    if (!data.solvent.isEmpty())
        parts << data.solvent;
    if (std::isfinite(data.temperature))
        parts << QString("%1 °C").arg(data.temperature, 0, 'f', 1);
    parts << QString("%1 points, %2 Hz").arg(data.values.size()).arg(data.sweep, 0, 'f', 1);
    if (data.scans > 0)
        parts << QString("%1 scans").arg(data.scans);
    result.description = parts.join(", ");
    QString reason;
    if (!result.isValid(&reason))
        return fail(reason);
    if (fid)
        *fid = result;
    return true;
}

} // namespace FidProcessing
