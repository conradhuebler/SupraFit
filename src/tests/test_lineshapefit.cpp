/*
 * Headless tests of the pseudo-Voigt lineshape fit (LineshapeFit)
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

// Claude Generated: WP4 of roadmap/input_data_editing.md.
//
// The line shape (analytic Jacobian vs central differences, exact area, Gauss/Lorentz limits), the
// recovery of the parameters over a spread (width, point density 2-20 points per FWHM, noise 0-5 % of
// the height, η 0-1, separation of two lines 0.5-4 FWHM), the comparison with the peak-picking
// position in the same scenarios, the calibration of the standard errors, failure cases and the
// shared width of a series. Worst cases are printed (qInfo) and pinned; LSF_DEBUG=1 prints every case.

#include <cmath>
#include <random>

#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

#include "src/core/trace/lineshapefit.h"
#include "src/core/trace/traceops.h"

namespace {
const double kNaN = std::numeric_limits<double>::quiet_NaN();

/// trace on a descending axis (like NMR) around centre, dx per point, ±halfRange
XYTrace makeTrace(const QVector<PseudoVoigtLine>& lines, double centre, double halfRange, double dx, double noise, double baseline, std::mt19937& rng)
{
    const int n = int(std::round(2 * halfRange / dx)) + 1;
    Eigen::VectorXd x(n), y(n);
    std::normal_distribution<double> gauss(0, 1);
    for (int i = 0; i < n; ++i) {
        x(i) = centre + halfRange - i * dx;
        double v = baseline;
        for (const PseudoVoigtLine& l : lines)
            v += LineshapeFit::evaluate(l, x(i));
        y(i) = v + noise * gauss(rng);
    }
    return XYTrace(x, y);
}

/// start of a line: position and width off the truth, η 0.5, area from the given height
LineshapeLineStart start(double position, double fwhm, double heightAboveBaseline, double window)
{
    LineshapeLineStart s;
    s.position.value = position;
    s.position.lower = position - window;
    s.position.upper = position + window;
    s.fwhm.value = fwhm;
    s.eta.value = 0.5;
    s.area.value = LineshapeFit::areaFromHeight(heightAboveBaseline, fwhm, 0.5);
    s.area.lower = 0;
    return s;
}

/// peak-picking positions of the trace (parabola refinement, as TraceOps does for the NMR tracking)
QVector<double> pickedPositions(const XYTrace& t, double threshold)
{
    QVector<TracePeak> peaks;
    TraceOps::pickPeaks(t, threshold, true, 0, false, &peaks);
    QVector<double> x;
    for (const TracePeak& p : peaks)
        x << p.x;
    return x;
}

double nearest(const QVector<double>& list, double x)
{
    double best = kNaN;
    for (double v : list)
        if (!std::isfinite(best) || std::abs(v - x) < std::abs(best - x))
            best = v;
    return best;
}
} // namespace

class TestLineshapeFit : public QObject {
    Q_OBJECT

private slots:
    /// analytic gradient against central differences, incl. the Gauss and Lorentz limits
    void jacobianMatchesNumeric()
    {
        double worst = 0;
        for (double eta : { 0.0, 0.3, 0.5, 1.0 })
            for (double w : { 0.002, 0.02, 1.5 })
                for (double u : { -4.0, -1.0, -0.37, 0.0, 0.2, 0.5, 2.5 }) {
                    const PseudoVoigtLine line{ 7.2, w, 0.8, eta };
                    const double x = line.position + u * w;
                    Eigen::Vector4d g;
                    LineshapeFit::evaluate(line, x, &g);
                    const double h[4] = { 1e-6 * w, 1e-6 * w, 1e-6, 1e-6 };
                    for (int p = 0; p < 4; ++p) {
                        PseudoVoigtLine plus = line, minus = line;
                        PseudoVoigtLine* ls[2] = { &plus, &minus };
                        for (int k = 0; k < 2; ++k) {
                            double* field = p == 0 ? &ls[k]->position : p == 1 ? &ls[k]->fwhm : p == 2 ? &ls[k]->area : &ls[k]->eta;
                            *field += (k == 0 ? h[p] : -h[p]);
                        }
                        const double numeric = (LineshapeFit::evaluate(plus, x) - LineshapeFit::evaluate(minus, x)) / (2 * h[p]);
                        // relative to the scale of this derivative over the line (value at u = 0.5)
                        const double scale = std::max({ std::abs(g(p)), std::abs(numeric), LineshapeFit::height(line) / (p < 2 ? w : 1.0) * 1e-3 });
                        worst = std::max(worst, std::abs(numeric - g(p)) / scale);
                    }
                }
        qInfo().noquote() << QString("Jacobian: worst relative difference to central differences %1").arg(worst, 0, 'g', 3);
        QVERIFY2(worst < 1e-6, qPrintable(QString::number(worst)));
    }

    /// the area parameter is the analytic area; finite-range integral matches erf/atan; limits
    void areaAndLimits()
    {
        double worstFinite = 0;
        for (double eta : { 0.0, 0.25, 0.5, 1.0 })
            for (double w : { 0.01, 1.0 }) {
                const PseudoVoigtLine line{ 3.0, w, 2.5, eta };
                const double R = 20 * w;
                const int n = 200000; // Simpson, even panels
                const double h = 2 * R / n;
                double s = 0;
                for (int i = 0; i <= n; ++i) {
                    const double f = LineshapeFit::evaluate(line, line.position - R + i * h);
                    s += f * ((i == 0 || i == n) ? 1 : (i % 2 ? 4 : 2));
                }
                s *= h / 3;
                const double expected = line.area * ((1 - eta) * std::erf(2 * std::sqrt(std::log(2.0)) * R / w) + eta * 2 / M_PI * std::atan(2 * R / w));
                worstFinite = std::max(worstFinite, std::abs(s - expected) / line.area);
            }
        qInfo().noquote() << QString("Area: worst |numeric - analytic finite-range area| / A = %1").arg(worstFinite, 0, 'g', 3);
        QVERIFY(worstFinite < 1e-10);

        // η = 0: Gauss with σ = w / (2 sqrt(2 ln2)); η = 1: Lorentz with γ = w / 2; height and half height
        const double w = 0.4, A = 1.7, d = 1.0;
        const double sigma = w / (2 * std::sqrt(2 * std::log(2.0))), gamma = w / 2;
        for (double x : { 0.3, 0.9, 1.0, 1.25, 2.0 }) {
            const double gauss = A / (sigma * std::sqrt(2 * M_PI)) * std::exp(-(x - d) * (x - d) / (2 * sigma * sigma));
            const double lorentz = A / M_PI * gamma / ((x - d) * (x - d) + gamma * gamma);
            QVERIFY(std::abs(LineshapeFit::evaluate({ d, w, A, 0.0 }, x) - gauss) < 1e-12 * A / w);
            QVERIFY(std::abs(LineshapeFit::evaluate({ d, w, A, 1.0 }, x) - lorentz) < 1e-12 * A / w);
        }
        for (double eta : { 0.0, 0.5, 1.0 }) {
            const PseudoVoigtLine line{ d, w, A, eta };
            QVERIFY(std::abs(LineshapeFit::evaluate(line, d) - LineshapeFit::height(line)) < 1e-12);
            QVERIFY(std::abs(LineshapeFit::evaluate(line, d + w / 2) - 0.5 * LineshapeFit::height(line)) < 1e-12);
            QVERIFY(std::abs(LineshapeFit::areaFromHeight(LineshapeFit::height(line), w, eta) - A) < 1e-12);
        }
    }

    /// half-height width of a picked line, also with a neighbour on one side
    void fwhmEstimate()
    {
        std::mt19937 rng(1);
        const XYTrace single = makeTrace({ { 7.0, 0.01, 1, 0.5 } }, 7.0, 0.1, 0.0005, 0, 0, rng);
        const Eigen::Index peak = single.nearestIndex(7.0);
        QVERIFY(std::abs(LineshapeFit::estimateFwhm(single.x(), single.y(), peak) - 0.01) < 0.0005);
        const XYTrace pair = makeTrace({ { 7.0, 0.01, 1, 0.5 }, { 7.012, 0.01, 1, 0.5 } }, 7.0, 0.1, 0.0005, 0, 0, rng);
        const double w = LineshapeFit::estimateFwhm(pair.x(), pair.y(), pair.nearestIndex(7.0));
        QVERIFY2(std::abs(w - 0.01) < 0.002, qPrintable(QString::number(w)));
        QVERIFY(std::isnan(LineshapeFit::estimateFwhm(pair.x(), pair.y(), -1)));
    }

    /// single line: recovery over widths, point density, η, noise; peak picking for comparison
    void singleLineSpread()
    {
        double worstPos = 0, worstArea = 0, worstWidth = 0, worstPick = 0, worstPosNoiseFree = 0;
        QString labelPos;
        int cases = 0;
        unsigned seed = 100;
        for (double w : { 0.002, 0.02 })
            for (double density : { 2.0, 3.0, 5.0, 10.0, 20.0 })
                for (double eta : { 0.0, 0.5, 1.0 })
                    for (double noise : { 0.0, 0.01, 0.05 })
                        for (int rep = 0; rep < 3; ++rep) {
                            std::mt19937 rng(seed++);
                            const double dx = w / density;
                            const double truePos = 7.0 + 0.37 * dx; // between two points
                            const PseudoVoigtLine truth{ truePos, w, LineshapeFit::areaFromHeight(1.0, w, eta), eta };
                            const XYTrace t = makeTrace({ truth }, 7.0, 5 * w, dx, noise, 0.02, rng);
                            const double pick = nearest(pickedPositions(t, 0.3), truePos);
                            QVector<LineshapeLineStart> starts{ start(truePos + 0.2 * w, 1.3 * w, 1.0, w) };
                            const LineshapeResult r = LineshapeFit::fit(t, t.xMin(), t.xMax(), starts);
                            const QString label = QString("FWHM %1, %2 points/FWHM, eta %3, noise %4").arg(w).arg(density).arg(eta).arg(noise);
                            QVERIFY2(r.ok(), qPrintable(label + ": " + r.message + " " + LineshapeFit::statusName(r.status)));
                            const PseudoVoigtLine& f = r.blocks[0].lines[0];
                            const double ePos = std::abs(f.position - truePos) / w;
                            if (ePos > worstPos) {
                                worstPos = ePos;
                                labelPos = label;
                            }
                            if (noise == 0)
                                worstPosNoiseFree = std::max(worstPosNoiseFree, ePos);
                            worstArea = std::max(worstArea, std::abs(f.area - truth.area) / truth.area);
                            worstWidth = std::max(worstWidth, std::abs(f.fwhm - w) / w);
                            if (std::isfinite(pick))
                                worstPick = std::max(worstPick, std::abs(pick - truePos) / w);
                            else
                                worstPick = std::max(worstPick, 1e9);
                            if (qEnvironmentVariableIsSet("LSF_DEBUG"))
                                qInfo().noquote() << QString("%1: fit %2 pick %3 FWHM, %4 it, %5 ms").arg(label).arg(ePos, 0, 'g', 3).arg(std::abs(pick - truePos) / w, 0, 'g', 3).arg(r.iterations).arg(r.milliseconds, 0, 'f', 3);
                            ++cases;
                        }
        qInfo().noquote() << QString("Single line over %1 cases (noise 0-5 %, 2-20 points/FWHM, eta 0-1):").arg(cases);
        qInfo().noquote() << QString("  fit: worst position error %1 FWHM (%2), noise-free %3 FWHM").arg(worstPos, 0, 'g', 3).arg(labelPos).arg(worstPosNoiseFree, 0, 'g', 3);
        qInfo().noquote() << QString("  fit: worst relative error of area %1, of width %2").arg(worstArea, 0, 'g', 3).arg(worstWidth, 0, 'g', 3);
        qInfo().noquote() << QString("  peak picking: worst position error %1 FWHM").arg(worstPick, 0, 'g', 3);
        QVERIFY(worstPosNoiseFree < 1e-6);
        QVERIFY2(worstPos < kPinSinglePosition, qPrintable(QString::number(worstPos)));
        QVERIFY2(worstArea < kPinSingleArea, qPrintable(QString::number(worstArea)));
        QVERIFY2(worstWidth < kPinSingleWidth, qPrintable(QString::number(worstWidth)));
        QVERIFY(worstPos < worstPick);
    }

    /// two overlapping lines: fit vs peak picking per separation
    void overlappingLinesSpread()
    {
        unsigned seed = 1000;
        double worstFitAll = 0, worstAreaAll = 0, worstWidthAll = 0;
        QString worstLabel;
        int failed = 0, cases = 0;
        for (double separation : { 4.0, 2.0, 1.5, 1.0, 0.75, 0.5 }) {
            double worstFit = 0, worstPick = 0, worstFitResolved = 0;
            int unresolved = 0, n = 0;
            for (double density : { 3.0, 5.0, 10.0, 20.0 })
                for (double eta : { 0.0, 0.5, 1.0 })
                    for (double noise : { 0.0, 0.01, 0.05 })
                        for (int rep = 0; rep < 2; ++rep) {
                            std::mt19937 rng(seed++);
                            const double w = 0.01, dx = w / density;
                            const double p1 = 7.0 + 0.31 * dx, p2 = p1 + separation * w;
                            const PseudoVoigtLine l1{ p1, w, LineshapeFit::areaFromHeight(1.0, w, eta), eta };
                            const PseudoVoigtLine l2{ p2, w, LineshapeFit::areaFromHeight(0.6, w, eta), eta };
                            const XYTrace t = makeTrace({ l1, l2 }, 0.5 * (p1 + p2), 0.5 * separation * w + 5 * w, dx, noise, 0, rng);
                            // peak picking: both lines resolved?
                            const QVector<double> picked = pickedPositions(t, 0.2);
                            double pickError = kNaN;
                            const double n1 = nearest(picked, p1), n2 = nearest(picked, p2);
                            if (picked.size() >= 2 && n1 != n2)
                                pickError = std::max(std::abs(n1 - p1), std::abs(n2 - p2)) / w;
                            // starts as the tracking gives them: the picked positions where resolved, else both
                            // ±0.25 FWHM around the single maximum; width 1.2 w
                            double s1, s2;
                            if (std::isfinite(pickError)) {
                                s1 = n1;
                                s2 = n2;
                            } else {
                                const double m = picked.isEmpty() ? 0.5 * (p1 + p2) : nearest(picked, 0.5 * (p1 + p2));
                                s1 = m - 0.25 * w;
                                s2 = m + 0.25 * w;
                                ++unresolved;
                            }
                            const QVector<LineshapeLineStart> starts{ start(s1, 1.2 * w, 1.0, 2 * w), start(s2, 1.2 * w, 0.6, 2 * w) };
                            const LineshapeResult r = LineshapeFit::fit(t, t.xMin(), t.xMax(), starts);
                            const QString label = QString("separation %1 FWHM, %2 points/FWHM, eta %3, noise %4").arg(separation).arg(density).arg(eta).arg(noise);
                            ++cases;
                            ++n;
                            if (!r.ok()) {
                                ++failed;
                                if (qEnvironmentVariableIsSet("LSF_DEBUG"))
                                    qInfo().noquote() << label << LineshapeFit::statusName(r.status) << r.message << (r.blocks.isEmpty() ? QString() : QString("positions %1 %2, true %3 %4").arg(r.blocks[0].lines[0].position, 0, 'f', 5).arg(r.blocks[0].lines[1].position, 0, 'f', 5).arg(p1, 0, 'f', 5).arg(p2, 0, 'f', 5));
                                continue;
                            }
                            QVector<PseudoVoigtLine> f = r.blocks[0].lines;
                            if (f[0].position > f[1].position)
                                std::swap(f[0], f[1]);
                            const double e = std::max(std::abs(f[0].position - p1), std::abs(f[1].position - p2)) / w;
                            worstFit = std::max(worstFit, e);
                            if (e > worstFitAll) {
                                worstFitAll = e;
                                worstLabel = label;
                            }
                            worstAreaAll = std::max({ worstAreaAll, std::abs(f[0].area - l1.area) / l1.area, std::abs(f[1].area - l2.area) / l2.area });
                            worstWidthAll = std::max({ worstWidthAll, std::abs(f[0].fwhm - w) / w, std::abs(f[1].fwhm - w) / w });
                            if (std::isfinite(pickError)) {
                                worstPick = std::max(worstPick, pickError);
                                worstFitResolved = std::max(worstFitResolved, e);
                            }
                            if (qEnvironmentVariableIsSet("LSF_DEBUG"))
                                qInfo().noquote() << QString("%1: fit %2 pick %3 FWHM").arg(label).arg(e, 0, 'g', 3).arg(pickError, 0, 'g', 3);
                        }
            qInfo().noquote() << QString("Two lines (height 1 : 0.6), separation %1 FWHM over %2 cases: fit worst %3 FWHM (%6 FWHM where peak picking resolves both); peak picking worst %4 FWHM, unresolved in %5 cases")
                                     .arg(separation)
                                     .arg(n)
                                     .arg(worstFit, 0, 'g', 3)
                                     .arg(worstPick, 0, 'g', 3)
                                     .arg(unresolved)
                                     .arg(worstFitResolved, 0, 'g', 3);
            // where the lines overlap the fit is better than the picked maxima on the same (resolved) cases
            if (separation <= 1.5 && worstPick > 0)
                QVERIFY2(worstFitResolved < worstPick, qPrintable(QString("separation %1: fit %2, pick %3").arg(separation).arg(worstFit).arg(worstPick)));
        }
        qInfo().noquote() << QString("Two lines over %1 cases: worst position error %2 FWHM (%3), area %4, width %5 (relative); %6 fits not converged")
                                 .arg(cases)
                                 .arg(worstFitAll, 0, 'g', 3)
                                 .arg(worstLabel)
                                 .arg(worstAreaAll, 0, 'g', 3)
                                 .arg(worstWidthAll, 0, 'g', 3)
                                 .arg(failed);
        QVERIFY2(failed <= kPinOverlapFailed, qPrintable(QString::number(failed)));
        QVERIFY2(worstFitAll < kPinOverlapPosition, qPrintable(QString::number(worstFitAll)));
    }

    /// standard errors: fraction of |δ - truth| within 1σ and 2σ over many noise realisations
    void standardErrorsCalibrated()
    {
        for (double separation : { 0.0, 1.0 }) {
            const int N = 400;
            int in1 = 0, in2 = 0, inArea1 = 0, total = 0;
            for (int k = 0; k < N; ++k) {
                std::mt19937 rng(5000 + k);
                const double w = 0.01, dx = w / 10;
                QVector<PseudoVoigtLine> truth{ { 7.0, w, LineshapeFit::areaFromHeight(1.0, w, 0.5), 0.5 } };
                QVector<LineshapeLineStart> starts{ start(7.0 + 0.1 * w, 1.2 * w, 1.0, 2 * w) };
                if (separation > 0) {
                    truth << PseudoVoigtLine{ 7.0 + separation * w, w, LineshapeFit::areaFromHeight(0.6, w, 0.5), 0.5 };
                    starts << start(7.0 + separation * w - 0.1 * w, 1.2 * w, 0.6, 2 * w);
                }
                const XYTrace t = makeTrace(truth, 7.0, 6 * w, dx, 0.02, 0, rng);
                const LineshapeResult r = LineshapeFit::fit(t, t.xMin(), t.xMax(), starts);
                QVERIFY2(r.ok(), qPrintable(r.message));
                for (int l = 0; l < truth.size(); ++l) {
                    const double e = std::abs(r.blocks[0].lines[l].position - truth[l].position);
                    const double s = r.blocks[0].errors[l].position;
                    QVERIFY(std::isfinite(s) && s > 0);
                    in1 += e <= s;
                    in2 += e <= 2 * s;
                    inArea1 += std::abs(r.blocks[0].lines[l].area - truth[l].area) <= r.blocks[0].errors[l].area;
                    ++total;
                }
            }
            const double f1 = double(in1) / total, f2 = double(in2) / total, fa = double(inArea1) / total;
            qInfo().noquote() << QString("Standard errors (%1 line(s), separation %2 FWHM, noise 2 %, %3 fits): |δ - true| <= 1σ in %4, <= 2σ in %5; area <= 1σ in %6 (normal: 0.683, 0.954)")
                                     .arg(separation > 0 ? 2 : 1)
                                     .arg(separation)
                                     .arg(N)
                                     .arg(f1, 0, 'f', 3)
                                     .arg(f2, 0, 'f', 3)
                                     .arg(fa, 0, 'f', 3);
            // binomial spread of a fraction of 0.683 over n >= 400 values: sd <= 0.023; ±3.5 sd
            QVERIFY2(f1 > 0.60 && f1 < 0.76, qPrintable(QString::number(f1)));
            QVERIFY2(f2 > 0.91 && f2 < 0.99, qPrintable(QString::number(f2)));
        }
    }

    /// failures give a status and a message, never NaN parameters or a crash
    void failureCases()
    {
        std::mt19937 rng(7);
        const double w = 0.01;
        const XYTrace t = makeTrace({ { 7.0, w, 0.02, 0.5 }, { 7.015, w, 0.012, 0.5 } }, 7.0, 0.06, 0.001, 0.01, 0, rng);

        // two identical starts on a pair of lines: rounding breaks the symmetry; whatever the outcome, finite values
        LineshapeResult r = LineshapeFit::fit(t, t.xMin(), t.xMax(), { start(7.007, w, 0.5, 0.05), start(7.007, w, 0.5, 0.05) });
        QVERIFY(r.status != LineshapeStatus::Invalid);
        for (const PseudoVoigtLine& l : r.blocks[0].lines)
            QVERIFY(std::isfinite(l.position) && std::isfinite(l.fwhm) && std::isfinite(l.area) && std::isfinite(l.eta));
        qInfo().noquote() << QString("Identical starts on a pair 1.5 FWHM apart: %1, positions %2 / %3").arg(LineshapeFit::statusName(r.status)).arg(r.blocks[0].lines[0].position, 0, 'f', 5).arg(r.blocks[0].lines[1].position, 0, 'f', 5);

        // rank deficient by construction: two lines at the same fixed position, width and η, only the areas
        // free -> identical columns of J: Degenerate, no standard errors, finite values
        LineshapeLineStart same = start(7.007, w, 0.5, 0.05);
        same.position.fixed = same.fwhm.fixed = same.eta.fixed = true;
        r = LineshapeFit::fit(t, t.xMin(), t.xMax(), { same, same });
        QCOMPARE(r.status, LineshapeStatus::Degenerate);
        QVERIFY(!r.message.isEmpty());
        for (const PseudoVoigtLine& l : r.blocks[0].lines)
            QVERIFY(std::isfinite(l.position) && std::isfinite(l.fwhm) && std::isfinite(l.area) && std::isfinite(l.eta));
        QVERIFY(std::isnan(r.blocks[0].errors[0].area));

        // a line outside the fit range
        r = LineshapeFit::fit(t, 6.98, 7.02, { start(7.5, w, 1, 0.05) });
        QCOMPARE(r.status, LineshapeStatus::Invalid);
        QVERIFY2(r.message.contains("outside"), qPrintable(r.message));

        // empty and too short data
        r = LineshapeFit::fit(t, 8.0, 8.1, { start(8.05, w, 1, 0.05) });
        QCOMPARE(r.status, LineshapeStatus::Invalid);
        LineshapeBlock tiny;
        tiny.x = Eigen::Vector3d(7.0, 7.001, 7.002);
        tiny.y = Eigen::Vector3d(1, 2, 1);
        tiny.lines = { start(7.001, w, 1, 0.05) };
        r = LineshapeFit::fitSeries({ tiny });
        QCOMPARE(r.status, LineshapeStatus::Invalid);
        QVERIFY2(r.message.contains("too few"), qPrintable(r.message));
        QCOMPARE(LineshapeFit::fitSeries({}).status, LineshapeStatus::Invalid);

        // NaN in the data
        LineshapeBlock nan;
        nan.x = t.x();
        nan.y = t.y();
        nan.y(10) = kNaN;
        nan.lines = { start(7.0, w, 1, 0.05) };
        r = LineshapeFit::fitSeries({ nan });
        QCOMPARE(r.status, LineshapeStatus::Invalid);
        QVERIFY(r.message.contains("NaN"));

        // start outside its own bounds, non-positive width
        LineshapeLineStart bad = start(7.0, w, 1, 0.05);
        bad.position.lower = 7.01;
        QCOMPARE(LineshapeFit::fit(t, t.xMin(), t.xMax(), { bad }).status, LineshapeStatus::Invalid);
        bad = start(7.0, -w, 1, 0.05);
        QCOMPARE(LineshapeFit::fit(t, t.xMin(), t.xMax(), { bad }).status, LineshapeStatus::Invalid);

        // iteration limit
        r = LineshapeFit::fit(t, t.xMin(), t.xMax(), { start(7.004, 2 * w, 0.5, 0.05), start(7.012, 2 * w, 0.5, 0.05) }, LineshapeOptions{ LineshapeBaseline::Constant, 1, 1e-10 });
        QCOMPARE(r.status, LineshapeStatus::MaxIterations);

        // pure noise: the line goes to a bound or zero area, values stay finite
        const XYTrace noise = makeTrace({}, 7.0, 0.05, 0.001, 0.01, 0, rng);
        r = LineshapeFit::fit(noise, noise.xMin(), noise.xMax(), { start(7.0, w, 0.03, 0.01) });
        QVERIFY(r.status != LineshapeStatus::Invalid);
        const PseudoVoigtLine& l = r.blocks[0].lines[0];
        QVERIFY(std::isfinite(l.position) && l.fwhm > 0 && l.area >= 0 && l.eta >= 0 && l.eta <= 1);
        QVERIFY(std::abs(l.position - 7.0) <= 0.01 + 1e-15);

        // fixed parameters stay
        LineshapeLineStart fixedEta = start(7.002, w, 0.02 / w, 0.05);
        fixedEta.eta.value = 0.8;
        fixedEta.eta.fixed = true;
        r = LineshapeFit::fit(t, t.xMin(), t.xMax(), { fixedEta, start(7.015, w, 0.012 / w, 0.05) });
        QVERIFY(r.status == LineshapeStatus::Converged);
        QCOMPARE(r.blocks[0].lines[0].eta, 0.8);
        QVERIFY(std::isnan(r.blocks[0].errors[0].eta));
    }

    /// a series with one true width: the shared width gives a smaller scatter of δ than free widths
    void sharedWidthOfASeries()
    {
        const int spectra = 10, realisations = 30;
        const double w = 0.01, dx = w / 5, eta = 0.5;
        double sumFree = 0, sumShared = 0, maxShared = 0;
        int count = 0;
        double ms = 0;
        for (int rep = 0; rep < realisations; ++rep) {
            std::mt19937 rng(9000 + rep);
            QVector<LineshapeBlock> blocks;
            QVector<QVector<double>> truth;
            for (int k = 0; k < spectra; ++k) {
                const double p1 = 7.0 + 0.004 * k, p2 = p1 + 1.0 * w; // overlapping pair moving together
                const XYTrace t = makeTrace({ { p1, w, LineshapeFit::areaFromHeight(1, w, eta), eta }, { p2, w, LineshapeFit::areaFromHeight(0.6, w, eta), eta } }, p1 + 0.5 * w, 5 * w, dx, 0.03, 0, rng);
                LineshapeBlock block;
                block.x = t.x();
                block.y = t.y();
                LineshapeLineStart a = start(p1 + 0.1 * w, 1.2 * w, 1, 2 * w), b = start(p2 - 0.1 * w, 1.2 * w, 0.6, 2 * w);
                a.shared = 0;
                b.shared = 1;
                block.lines = { a, b };
                blocks << block;
                truth << QVector<double>{ p1, p2 };
            }
            const LineshapeResult shared = LineshapeFit::fitSeries(blocks);
            QVERIFY2(shared.ok(), qPrintable(shared.message));
            ms = std::max(ms, shared.milliseconds);
            // shared width identical in every block
            QCOMPARE(shared.blocks[3].lines[0].fwhm, shared.blocks[7].lines[0].fwhm);
            for (int k = 0; k < spectra; ++k) {
                LineshapeBlock own = blocks[k];
                own.lines[0].shared = own.lines[1].shared = -1;
                const LineshapeResult free = LineshapeFit::fitSeries({ own });
                QVERIFY2(free.ok(), qPrintable(free.message));
                for (int l = 0; l < 2; ++l) {
                    const double ef = (free.blocks[0].lines[l].position - truth[k][l]) / w;
                    const double es = (shared.blocks[k].lines[l].position - truth[k][l]) / w;
                    sumFree += ef * ef;
                    sumShared += es * es;
                    maxShared = std::max(maxShared, std::abs(es));
                    ++count;
                }
            }
        }
        const double rmsFree = std::sqrt(sumFree / count), rmsShared = std::sqrt(sumShared / count);
        qInfo().noquote() << QString("Series of %1 spectra, pair 1 FWHM apart, noise 3 %, %2 realisations: rms δ error FREE %3 FWHM, SHARED %4 FWHM (worst %5); shared fit of %6 parameters at most %7 ms")
                                 .arg(spectra)
                                 .arg(realisations)
                                 .arg(rmsFree, 0, 'g', 3)
                                 .arg(rmsShared, 0, 'g', 3)
                                 .arg(maxShared, 0, 'g', 3)
                                 .arg(spectra * 5 + 4)
                                 .arg(ms, 0, 'f', 2);
        QVERIFY2(rmsShared < rmsFree, qPrintable(QString("%1 vs %2").arg(rmsShared).arg(rmsFree)));
    }

    /// linear baseline: a sloped background is taken up, not the line
    void linearBaseline()
    {
        std::mt19937 rng(3);
        const double w = 0.01;
        XYTrace t = makeTrace({ { 7.0, w, LineshapeFit::areaFromHeight(1, w, 0.3), 0.3 } }, 7.0, 0.05, 0.001, 0.0, 0, rng);
        Eigen::VectorXd y = t.y();
        for (Eigen::Index i = 0; i < y.size(); ++i)
            y(i) += 0.1 + 2.0 * (t.x()(i) - 7.0);
        t.setData(t.x(), y);
        const LineshapeResult r = LineshapeFit::fit(t, t.xMin(), t.xMax(), { start(7.002, 1.2 * w, 1, 0.02) }, LineshapeOptions{ LineshapeBaseline::Linear, 200, 1e-10 });
        QVERIFY2(r.ok(), qPrintable(r.message));
        QVERIFY(std::abs(r.blocks[0].lines[0].position - 7.0) < 1e-8);
        QVERIFY(std::abs(r.blocks[0].baseline(0) - 0.1) < 1e-8);
        QVERIFY(std::abs(r.blocks[0].evaluate(7.03) - (LineshapeFit::evaluate(r.blocks[0].lines[0], 7.03) + 0.1 + 2.0 * 0.03)) < 1e-8);
        QVERIFY(r.blocks[0].residuals.cwiseAbs().maxCoeff() < 1e-8);
        QCOMPARE(LineshapeFit::baselineFromName(LineshapeFit::baselineName(LineshapeBaseline::Linear)), LineshapeBaseline::Linear);
    }

private:
    // pinned worst cases (measured values are printed by the tests); measured 2026-11-08, see the qInfo lines
    static constexpr double kPinSinglePosition = 0.08;
    static constexpr double kPinSingleArea = 0.35;
    static constexpr double kPinSingleWidth = 0.3;
    static constexpr double kPinOverlapPosition = 0.5;
    static constexpr int kPinOverlapFailed = 2; // both Degenerate at 3 points/FWHM, noise 5 %, separation <= 0.75 FWHM (a line runs off)
};

QTEST_GUILESS_MAIN(TestLineshapeFit)
#include "test_lineshapefit.moc"
