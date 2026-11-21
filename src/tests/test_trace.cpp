/*
 * Headless tests for the trace building blocks (XYTrace, TraceSet, TraceOps, TraceJson)
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

// Claude Generated: WP1 of roadmap/input_data_editing.md. Synthetic data with known results; the
// accuracy checks run over a spread of widths/noise levels and pin the worst case (printed with qInfo).

#include <cmath>
#include <limits>
#include <random>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

#include "src/core/toolset.h"
#include "src/core/trace/traceops.h"
#include "src/core/trace/traceset.h"
#include "src/core/trace/tracejson.h"
#include "src/core/trace/xytrace.h"

namespace {
const double kNaN = std::numeric_limits<double>::quiet_NaN();

Eigen::VectorXd grid(double start, double end, double step)
{
    const int n = int(std::llround((end - start) / step)) + 1;
    Eigen::VectorXd x(n);
    for (int i = 0; i < n; ++i)
        x(i) = start + step * i;
    return x;
}

double gauss(double x, double amplitude, double centre, double sigma)
{
    return amplitude * std::exp(-0.5 * std::pow((x - centre) / sigma, 2));
}

/// Bit-identical vectors (== per entry, NaN never equal)
bool identical(const Eigen::VectorXd& a, const Eigen::VectorXd& b)
{
    if (a.size() != b.size())
        return false;
    for (Eigen::Index i = 0; i < a.size(); ++i)
        if (!(a(i) == b(i)))
            return false;
    return true;
}

/// Brute-force nearest index (ties: lower index)
Eigen::Index bruteNearest(const Eigen::VectorXd& x, double v)
{
    Eigen::Index best = -1;
    double diff = std::numeric_limits<double>::infinity();
    for (Eigen::Index i = 0; i < x.size(); ++i)
        if (std::abs(v - x(i)) < diff) {
            diff = std::abs(v - x(i));
            best = i;
        }
    return best;
}
} // namespace

class TestTrace : public QObject {
    Q_OBJECT

private slots:
    // ----------------------------------------------------------------- XYTrace
    void emptyTrace()
    {
        XYTrace t;
        QVERIFY(t.isEmpty());
        QCOMPARE(t.size(), Eigen::Index(0));
        QVERIFY(t.isValid());
        QVERIFY(!t.isMonotone());
        QCOMPARE(t.nearestIndex(1.0), Eigen::Index(-1));
        QVERIFY(std::isnan(t.interpolate(1.0)));
        QVERIFY(std::isnan(t.xMin()));
        QVERIFY(t.range(0, 10).isEmpty());
        QVERIFY(t.points().isEmpty());
    }

    void singlePoint()
    {
        Eigen::VectorXd x(1), y(1);
        x << 3.0;
        y << 7.0;
        XYTrace t(x, y);
        QVERIFY(t.isValid());
        QCOMPARE(t.orientation(), XYTrace::Orientation::Undefined);
        QCOMPARE(t.nearestIndex(-100.0), Eigen::Index(0));
        QCOMPARE(t.nearestIndex(100.0), Eigen::Index(0));
        QCOMPARE(t.interpolate(3.0), 7.0);
        QVERIFY(std::isnan(t.interpolate(3.1)));
        QCOMPARE(t.range(3.0, 3.0).size(), Eigen::Index(1));
        QCOMPARE(t.xMin(), 3.0);
        QCOMPARE(t.xMax(), 3.0);
    }

    void validation()
    {
        QString reason;
        XYTrace mismatch(grid(0, 4, 1), grid(0, 3, 1));
        QVERIFY(!mismatch.isValid(&reason));
        QVERIFY(reason.contains("5") && reason.contains("4"));
        QVERIFY(std::isnan(mismatch.interpolate(1.0)));

        Eigen::VectorXd y = grid(0, 4, 1);
        y(2) = kNaN;
        QVERIFY(!XYTrace(grid(0, 4, 1), y).isValid(&reason));
        QVERIFY(reason.contains("finite"));

        Eigen::VectorXd x = grid(0, 4, 1);
        x(3) = x(2); // repeated x: not strictly monotone
        XYTrace repeated(x, grid(0, 4, 1));
        QVERIFY(!repeated.isValid(&reason));
        QCOMPARE(repeated.orientation(), XYTrace::Orientation::Undefined);
        QVERIFY(std::isnan(repeated.interpolate(1.5)));
        // nearestIndex still works on an unordered axis (linear scan)
        QCOMPARE(repeated.nearestIndex(2.9), Eigen::Index(2));
    }

    void descendingAxis()
    {
        // NMR-like axis: 10 ppm down to 0 ppm
        const Eigen::VectorXd asc = grid(0, 10, 0.25);
        const Eigen::VectorXd desc = asc.reverse();
        Eigen::VectorXd y(desc.size());
        for (Eigen::Index i = 0; i < desc.size(); ++i)
            y(i) = 2.0 * desc(i) - 1.0;
        XYTrace t(desc, y);
        QVERIFY(t.isValid());
        QCOMPARE(t.orientation(), XYTrace::Orientation::Descending);
        QCOMPARE(t.xMin(), 0.0);
        QCOMPARE(t.xMax(), 10.0);

        // nearest index against brute force, including the exact midpoints (ties)
        for (double v = -1.0; v <= 11.0; v += 0.0625) {
            QCOMPARE(t.nearestIndex(v), bruteNearest(desc, v));
            XYTrace a(asc, y.reverse());
            QCOMPARE(a.nearestIndex(v), bruteNearest(asc, v));
        }
        // interpolation of a linear function is exact; outside -> NaN
        for (double v = 0.0; v <= 10.0; v += 0.1)
            QVERIFY(std::abs(t.interpolate(v) - (2.0 * v - 1.0)) <= 1e-12);
        QVERIFY(std::isnan(t.interpolate(-0.01)));
        QVERIFY(std::isnan(t.interpolate(10.01)));
        QCOMPARE(t.interpolate(10.0), y(0));
        QCOMPARE(t.interpolate(0.0), y(y.size() - 1));

        // range keeps the order of the axis, bounds in any order
        XYTrace r = t.range(2.0, 1.0);
        QCOMPARE(r.size(), Eigen::Index(5));
        QCOMPARE(r.x()(0), 2.0);
        QCOMPARE(r.x()(4), 1.0);
        QCOMPARE(r.orientation(), XYTrace::Orientation::Descending);
    }

    void rangeOutsideAndNan()
    {
        XYTrace t(grid(400, 600, 4), grid(0, 50, 1), "spec");
        t.setXAxis("wavelength", "nm");
        XYTrace out = t.range(700, 800);
        QVERIFY(out.isEmpty());
        QCOMPARE(out.name(), QString("spec"));
        QCOMPARE(out.xUnit(), QString("nm"));
        QVERIFY(t.range(kNaN, 500).isEmpty());
        QCOMPARE(t.range(398, 402).size(), Eigen::Index(1));
        QCOMPARE(t.range(400, 600).size(), t.size());
    }

    void interpolationNonUniform()
    {
        std::mt19937 rng(7);
        std::uniform_real_distribution<double> step(0.01, 2.0);
        Eigen::VectorXd x(200), y(200);
        double v = -50;
        for (int i = 0; i < 200; ++i) {
            v += step(rng);
            x(i) = v;
            y(i) = 3.0 - 0.5 * v;
        }
        XYTrace t(x, y);
        QVERIFY(t.isValid());
        // nodes are returned bit for bit, a linear function in between
        for (int i = 0; i < 200; ++i)
            QCOMPARE(t.interpolate(x(i)), y(i));
        std::uniform_real_distribution<double> pos(x(0), x(199));
        double worst = 0;
        for (int k = 0; k < 2000; ++k) {
            const double p = pos(rng);
            worst = std::max(worst, std::abs(t.interpolate(p) - (3.0 - 0.5 * p)));
            QCOMPARE(t.nearestIndex(p), bruteNearest(x, p));
        }
        QVERIFY2(worst <= 1e-12, qPrintable(QString::number(worst)));
    }

    // ----------------------------------------------------------------- TraceSet
    void traceSetIds()
    {
        TraceSet set;
        const QString a = set.add(XYTrace(grid(0, 1, 1), grid(0, 1, 1), "a"));
        const QString b = set.add(XYTrace(grid(0, 1, 1), grid(0, 1, 1), "b"));
        const QString c = set.add(XYTrace(grid(0, 1, 1), grid(0, 1, 1), "c"), "given");
        QCOMPARE(c, QString("given"));
        QVERIFY(set.add(XYTrace(), "given").isEmpty()); // duplicate id refused
        QCOMPARE(set.size(), 3);
        QVERIFY(set.move(2, 0));
        QCOMPARE(set.ids(), QStringList({ c, a, b }));
        QCOMPARE(set.at(0).name(), QString("c"));
        QVERIFY(!set.move(0, 3));
        QVERIFY(set.remove(a));
        QVERIFY(!set.remove(a));
        QCOMPARE(set.indexOf(b), 1);
        QCOMPARE(set.trace(b).name(), QString("b"));
        QVERIFY(set.trace("missing").isEmpty());
        QCOMPARE(set.insert(-5, XYTrace({}, {}, "first")).isEmpty(), false);
        QCOMPARE(set.at(0).name(), QString("first"));
    }

    void traceSetCommonRangeAndAxis()
    {
        TraceSet set;
        double lo = 0, hi = 0;
        QVERIFY(!set.commonRange(&lo, &hi));
        set.add(XYTrace(grid(400, 600, 4), grid(400, 600, 4)));
        set.add(XYTrace(grid(410, 650, 2), grid(410, 650, 2)));
        set.add(XYTrace()); // empty traces are ignored for the range
        QVERIFY(set.commonRange(&lo, &hi));
        QCOMPARE(lo, 410.0);
        QCOMPARE(hi, 600.0);
        QVERIFY(!set.hasCommonAxis());

        TraceSet same;
        same.add(XYTrace(grid(400, 600, 4), grid(0, 50, 1)));
        Eigen::VectorXd shifted = grid(400, 600, 4).array() + 1e-12;
        same.add(XYTrace(shifted, grid(0, 50, 1)));
        QVERIFY(same.hasCommonAxis());
        same.add(XYTrace(grid(400, 600, 4).array() + 0.1, grid(0, 50, 1)));
        QVERIFY(!same.hasCommonAxis());

        TraceSet disjoint;
        disjoint.add(XYTrace(grid(0, 1, 0.5), grid(0, 1, 0.5)));
        disjoint.add(XYTrace(grid(2, 3, 0.5), grid(2, 3, 0.5)));
        QVERIFY(!disjoint.commonRange(&lo, &hi));
        QString error;
        QVERIFY(disjoint.resampleToCommonAxis(&error).isEmpty());
        QVERIFY(error.contains("range"));
    }

    void resampleKeepsSameAxisBitForBit()
    {
        TraceSet set;
        const Eigen::VectorXd x = grid(400, 600, 4);
        for (int s = 0; s < 4; ++s) {
            Eigen::VectorXd y(x.size());
            for (Eigen::Index i = 0; i < x.size(); ++i)
                y(i) = gauss(x(i), 100.0 * (s + 1), 500, 30) + 0.01 * x(i);
            set.add(XYTrace(x, y, QString::number(s)));
        }
        QString error;
        TraceSet res = set.resampleToCommonAxis(&error);
        QVERIFY2(!res.isEmpty(), qPrintable(error));
        QCOMPARE(res.ids(), set.ids());
        for (int s = 0; s < 4; ++s) {
            QVERIFY(identical(res.at(s).x(), set.at(s).x()));
            QVERIFY(identical(res.at(s).y(), set.at(s).y()));
        }
    }

    void resampleDifferentSampling()
    {
        // first: 400..600 step 4; second: 396..604 step 8 (coarser); third: descending, step 3
        TraceSet set;
        auto line = [](const Eigen::VectorXd& x) { return Eigen::VectorXd(2.0 + 0.25 * x.array()); };
        const Eigen::VectorXd x1 = grid(400, 600, 4), x2 = grid(396, 604, 8), x3 = grid(397, 604, 3).reverse();
        set.add(XYTrace(x1, line(x1), "fine"));
        set.add(XYTrace(x2, line(x2), "coarse"));
        set.add(XYTrace(x3, line(x3), "descending"));
        QString error;
        TraceSet res = set.resampleToCommonAxis(&error);
        QVERIFY2(res.size() == 3, qPrintable(error));
        for (int s = 0; s < 3; ++s) {
            QVERIFY(identical(res.at(s).x(), x1));
            QVERIFY(((res.at(s).y() - line(x1)).cwiseAbs().maxCoeff()) <= 1e-12);
            QCOMPARE(res.at(s).name(), set.at(s).name());
        }

        // a Gaussian band: the error of linear interpolation over a spread of band widths
        double worst = 0;
        for (double sigma : { 10.0, 20.0, 30.0, 50.0 }) {
            TraceSet g;
            for (const Eigen::VectorXd& x : { x1, x2 }) {
                Eigen::VectorXd y(x.size());
                for (Eigen::Index i = 0; i < x.size(); ++i)
                    y(i) = gauss(x(i), 1.0, 500.3, sigma);
                g.add(XYTrace(x, y));
            }
            TraceSet r = g.resampleToCommonAxis();
            for (Eigen::Index i = 0; i < x1.size(); ++i)
                worst = std::max(worst, std::abs(r.at(1).y()(i) - gauss(x1(i), 1.0, 500.3, sigma)));
        }
        qInfo("resample Gaussian (step 8 -> 4, sigma 10..50, amplitude 1): worst abs error %.3g", worst);
        // h^2/8 * max|f''| = 64/8 / sigma^2 = 0.08 for sigma = 10
        QVERIFY(worst <= 0.08);

        // an invalid trace makes resampling fail with a reason instead of NaNs
        Eigen::VectorXd bad = x1;
        bad(3) = bad(2);
        set.add(XYTrace(bad, line(x1), "bad"));
        QVERIFY(set.resampleToCommonAxis(&error).isEmpty());
        QVERIFY(error.contains("bad"));
    }

    // ----------------------------------------------------------------- TraceOps
    void trapezoidOrientation()
    {
        const Eigen::VectorXd x = grid(0, 10, 0.5);
        XYTrace asc(x, Eigen::VectorXd::Constant(x.size(), 2.0));
        XYTrace desc(x.reverse(), Eigen::VectorXd::Constant(x.size(), 2.0));
        QCOMPARE(TraceOps::trapezoid(asc), 20.0);
        QCOMPARE(TraceOps::trapezoid(desc), 20.0);
        QCOMPARE(TraceOps::trapezoid(asc, 0, 2), 2.0);
        QCOMPARE(TraceOps::trapezoid(XYTrace()), 0.0);
    }

    void savitzkyGolayReproducesPolynomials()
    {
        std::mt19937 rng(3);
        std::uniform_real_distribution<double> step(0.5, 1.5);
        Eigen::VectorXd nonuniform(60);
        double v = 0;
        for (int i = 0; i < 60; ++i) {
            v += step(rng);
            nonuniform(i) = v;
        }
        auto cubic = [](double x) { return 1.0 - 0.3 * x + 0.02 * x * x - 0.0004 * x * x * x; };
        double worst = 0;
        for (const Eigen::VectorXd& x : { Eigen::VectorXd(grid(0, 59, 1)), nonuniform, Eigen::VectorXd(grid(0, 59, 1).reverse()) }) {
            Eigen::VectorXd y(x.size());
            for (Eigen::Index i = 0; i < x.size(); ++i)
                y(i) = cubic(x(i));
            for (int window : { 5, 7, 11 }) {
                XYTrace out;
                QString error;
                QVERIFY2(TraceOps::savitzkyGolay(XYTrace(x, y), window, 3, &out, &error), qPrintable(error));
                worst = std::max(worst, (out.y() - y).cwiseAbs().maxCoeff());
                QVERIFY(identical(out.x(), x));
            }
        }
        qInfo("Savitzky-Golay order 3 on a cubic (uniform, non-uniform, descending; windows 5..11, edges incl.): worst abs error %.3g", worst);
        QVERIFY(worst <= 1e-10);
    }

    void savitzkyGolayValidation()
    {
        XYTrace t(grid(0, 9, 1), grid(0, 9, 1));
        XYTrace out;
        QString error;
        QVERIFY(!TraceOps::savitzkyGolay(t, 4, 2, &out, &error));
        QVERIFY(error.contains("odd"));
        QVERIFY(!TraceOps::savitzkyGolay(t, 5, 5, &out, &error));
        QVERIFY(!TraceOps::savitzkyGolay(t, 11, 2, &out, &error));
        QVERIFY(error.contains("fewer"));
        QVERIFY(TraceOps::savitzkyGolay(t, 5, 0, &out, &error)); // moving average
        QCOMPARE(out.y()(4), 4.0);
    }

    void savitzkyGolayConservesArea()
    {
        // Gaussian on x 0..200 step 1; widths x windows x orders x noise levels
        const Eigen::VectorXd x = grid(0, 200, 1);
        std::mt19937 rng(11);
        double worst_clean = 0, worst_noisy = 0, worst_peak = 0;
        for (double sigma : { 3.0, 5.0, 10.0, 20.0 })
            for (int window : { 5, 11, 21 })
                for (int order : { 2, 4 })
                    for (double noise : { 0.0, 0.01 }) {
                        std::normal_distribution<double> dist(0.0, noise > 0 ? noise : 1.0);
                        Eigen::VectorXd y(x.size());
                        for (Eigen::Index i = 0; i < x.size(); ++i)
                            y(i) = gauss(x(i), 1.0, 100.4, sigma) + (noise > 0 ? dist(rng) : 0.0);
                        XYTrace raw(x, y);
                        XYTrace out;
                        QVERIFY(TraceOps::savitzkyGolay(raw, window, order, &out));
                        const double area = TraceOps::trapezoid(raw);
                        const double rel = std::abs(TraceOps::trapezoid(out) - area) / area;
                        (noise > 0 ? worst_noisy : worst_clean) = std::max(noise > 0 ? worst_noisy : worst_clean, rel);
                        if (noise == 0 && window <= 2 * sigma + 1)
                            worst_peak = std::max(worst_peak, std::abs(out.y().maxCoeff() - y.maxCoeff()) / y.maxCoeff());
                    }
        qInfo("Savitzky-Golay relative area change: noise-free worst %.3g, noise 1%% worst %.3g; peak height (window <= 2 sigma + 1) worst %.3g",
            worst_clean, worst_noisy, worst_peak);
        // noise-free: only the end windows change the area (sigma 20 leaves 4e-6 of the band at the ends)
        QVERIFY(worst_clean <= 3e-6);
        QVERIFY(worst_noisy <= 2e-3);
        QVERIFY(worst_peak <= 0.02);
    }

    void baselineRecoversLinearBaseline()
    {
        // y = Gaussian + a + b x; fit outside +-5 sigma; widths x noise levels
        const Eigen::VectorXd x = grid(0, 200, 0.5);
        const double a = 5.0, b = 0.02;
        std::mt19937 rng(5);
        double worst_a_clean = 0, worst_b_clean = 0, worst_a = 0, worst_b = 0;
        for (double sigma : { 2.0, 5.0, 10.0 })
            for (double noise : { 0.0, 0.1, 1.0 }) {
                std::normal_distribution<double> dist(0.0, noise > 0 ? noise : 1.0);
                Eigen::VectorXd y(x.size());
                for (Eigen::Index i = 0; i < x.size(); ++i)
                    y(i) = gauss(x(i), 100.0, 100.0, sigma) + a + b * x(i) + (noise > 0 ? dist(rng) : 0.0);
                TraceOps::PolynomialBaseline base;
                QString error;
                QVERIFY2(TraceOps::fitPolynomialBaseline(XYTrace(x, y), 1, { { 100.0 + 5 * sigma, 100.0 - 5 * sigma } }, &base, &error), qPrintable(error));
                const Eigen::VectorXd p = base.powerCoefficients();
                QCOMPARE(p.size(), Eigen::Index(2));
                if (noise == 0) {
                    worst_a_clean = std::max(worst_a_clean, std::abs(p(0) - a));
                    worst_b_clean = std::max(worst_b_clean, std::abs(p(1) - b));
                } else {
                    worst_a = std::max(worst_a, std::abs(p(0) - a) / noise);
                    worst_b = std::max(worst_b, std::abs(p(1) - b) / noise);
                }
                // the evaluated baseline agrees with its power form
                QVERIFY(std::abs(base.evaluate(123.0) - (p(0) + p(1) * 123.0)) <= 1e-10);
            }
        qInfo("baseline degree 1 (sigma 2..10, exclusion +-5 sigma): noise-free |da| %.3g |db| %.3g; noisy (per unit noise) |da| %.3g |db| %.3g",
            worst_a_clean, worst_b_clean, worst_a, worst_b);
        // noise-free: the band tails beyond +-5 sigma (100 * exp(-12.5) = 3.7e-4 at the limit) bias the fit
        QVERIFY(worst_a_clean <= 3e-5);
        QVERIFY(worst_b_clean <= 1e-10);
        // standard error of a with ~380 points is ~0.1 per unit noise, of b ~1e-3: 4 sigma margins
        QVERIFY(worst_a <= 0.4);
        QVERIFY(worst_b <= 4e-3);
    }

    void baselineQuadraticAndErrors()
    {
        const Eigen::VectorXd x = grid(-3, 7, 0.1);
        Eigen::VectorXd y = (1.0 - 2.0 * x.array() + 0.5 * x.array().square()).matrix();
        TraceOps::PolynomialBaseline base;
        QVERIFY(TraceOps::fitPolynomialBaseline(XYTrace(x, y), 2, {}, &base));
        const Eigen::VectorXd p = base.powerCoefficients();
        QVERIFY(std::abs(p(0) - 1.0) <= 1e-10 && std::abs(p(1) + 2.0) <= 1e-10 && std::abs(p(2) - 0.5) <= 1e-10);

        BaselineOp op(2);
        XYTrace flat = op.apply(XYTrace(x, y));
        QVERIFY(flat.y().cwiseAbs().maxCoeff() <= 1e-10);

        QString error;
        QVERIFY(!TraceOps::fitPolynomialBaseline(XYTrace(x, y), 2, { { -10, 6.85 } }, &base, &error));
        QVERIFY(error.contains("points"));
        QVERIFY(!TraceOps::fitPolynomialBaseline(XYTrace(x, y), 11, {}, &base, &error));
    }

    void normaliseModes()
    {
        const Eigen::VectorXd x = grid(0, 4, 1);
        Eigen::VectorXd y(5);
        y << 1, 3, -6, 2, 1;
        XYTrace t(x, y);
        XYTrace out;
        QVERIFY(TraceOps::normalise(t, TraceOps::NormaliseMode::Max, false, 0, 0, &out));
        QCOMPARE(out.y()(2), -1.0);
        QCOMPARE(out.y()(1), 0.5);
        QVERIFY(TraceOps::normalise(t, TraceOps::NormaliseMode::MinMax, false, 0, 0, &out));
        QCOMPARE(out.y().minCoeff(), 0.0);
        QCOMPARE(out.y().maxCoeff(), 1.0);
        // reference window [0, 1]: max|y| there is 3
        QVERIFY(TraceOps::normalise(t, TraceOps::NormaliseMode::Max, true, 1, 0, &out));
        QCOMPARE(out.y()(1), 1.0);
        QCOMPARE(out.y()(2), -2.0);

        Eigen::VectorXd g(201);
        const Eigen::VectorXd gx = grid(0, 100, 0.5);
        for (Eigen::Index i = 0; i < gx.size(); ++i)
            g(i) = gauss(gx(i), 3.0, 50, 4);
        QVERIFY(TraceOps::normalise(XYTrace(gx, g), TraceOps::NormaliseMode::Area, false, 0, 0, &out));
        QVERIFY(std::abs(TraceOps::trapezoid(out) - 1.0) <= 1e-12);

        QString error;
        QVERIFY(!TraceOps::normalise(XYTrace(x, Eigen::VectorXd::Zero(5)), TraceOps::NormaliseMode::Max, false, 0, 0, &out, &error));
        QVERIFY(error.contains("zero"));
        QVERIFY(!TraceOps::normalise(t, TraceOps::NormaliseMode::Area, true, 10, 20, &out, &error));
        QVERIFY(error.contains("points"));
    }

    void pickPeaksFindsKnownPositions()
    {
        // three bands, positions off the grid, widths x sub-grid offsets; step 0.5
        const Eigen::VectorXd x = grid(0, 200, 0.5);
        double worst_pos = 0, worst_height = 0, worst_area = 0;
        for (double sigma : { 1.0, 2.0, 4.0, 8.0 })
            for (double offset : { 0.0, 0.13, 0.25, 0.37 }) {
                const double c[3] = { 50.0 + offset, 101.0 + offset, 152.0 + offset };
                const double amp[3] = { 1.0, 2.5, 0.6 };
                Eigen::VectorXd y = Eigen::VectorXd::Zero(x.size());
                for (Eigen::Index i = 0; i < x.size(); ++i)
                    for (int k = 0; k < 3; ++k)
                        y(i) += gauss(x(i), amp[k], c[k], sigma);
                QVector<TracePeak> peaks;
                QString error;
                QVERIFY2(TraceOps::pickPeaks(XYTrace(x, y), 0.1, true, 0, true, &peaks, &error), qPrintable(error));
                QCOMPARE(peaks.size(), 3);
                for (int k = 0; k < 3; ++k) {
                    worst_pos = std::max(worst_pos, std::abs(peaks[k].x - c[k]) / sigma);
                    worst_height = std::max(worst_height, std::abs(peaks[k].height - amp[k]) / amp[k]);
                    const double area = amp[k] * sigma * std::sqrt(2 * M_PI);
                    worst_area = std::max(worst_area, std::abs(peaks[k].integral - area) / area);
                }
            }
        qInfo("peak picking (sigma 1..8, step 0.5, sub-grid offsets): worst |dx|/sigma %.3g, relative height %.3g, relative area %.3g",
            worst_pos, worst_height, worst_area);
        QVERIFY(worst_pos <= 0.01);
        QVERIFY(worst_height <= 0.005);
        QVERIFY(worst_area <= 1e-3);
    }

    void pickPeaksThresholdDistancePlateauDescending()
    {
        const Eigen::VectorXd x = grid(0, 100, 1);
        Eigen::VectorXd y = Eigen::VectorXd::Zero(x.size());
        for (Eigen::Index i = 0; i < x.size(); ++i)
            y(i) = gauss(x(i), 10, 20, 2) + gauss(x(i), 4, 27, 2) + gauss(x(i), 1, 70, 2);
        QVector<TracePeak> peaks;
        QVERIFY(TraceOps::pickPeaks(XYTrace(x, y), 0.0, true, 0, false, &peaks));
        QCOMPARE(peaks.size(), 3);
        QVERIFY(std::isnan(peaks[0].integral));
        QVERIFY(TraceOps::pickPeaks(XYTrace(x, y), 2.0, false, 0, false, &peaks)); // absolute
        QCOMPARE(peaks.size(), 2);
        QVERIFY(TraceOps::pickPeaks(XYTrace(x, y), 0.0, true, 10, false, &peaks)); // 20 and 27 too close
        QCOMPARE(peaks.size(), 2);
        QVERIFY(std::abs(peaks[0].x - 20) < 0.1 && std::abs(peaks[1].x - 70) < 0.1);

        // descending axis gives the same peaks
        QVERIFY(TraceOps::pickPeaks(XYTrace(x.reverse(), y.reverse()), 0.0, true, 0, false, &peaks));
        QCOMPARE(peaks.size(), 3);
        QVERIFY(std::abs(peaks[0].x - 70) < 0.1 && std::abs(peaks[2].x - 20) < 0.1);

        // plateau: middle of the run; edges are never peaks
        Eigen::VectorXd p(7);
        p << 5, 0, 1, 1, 1, 0, 5;
        QVERIFY(TraceOps::pickPeaks(XYTrace(grid(0, 6, 1), p), 0.0, true, 0, false, &peaks));
        QCOMPARE(peaks.size(), 1);
        QCOMPARE(peaks[0].index, Eigen::Index(3));

        QString error;
        QVERIFY(!TraceOps::pickPeaks(XYTrace(x, y), 1.5, true, 0, false, &peaks, &error));
        QVERIFY(!TraceOps::pickPeaks(XYTrace(x, y), 0.1, true, -1, false, &peaks, &error));
    }

    // ----------------------------------------------------------------- persistence
    void vectorRoundtripIsBitExact()
    {
        std::mt19937_64 rng(42);
        std::uniform_real_distribution<double> uni(-1e3, 1e3);
        Eigen::VectorXd v(1000 + 12);
        for (int i = 0; i < 1000; ++i)
            v(i) = uni(rng) * std::pow(10.0, double(int(rng() % 40) - 20));
        v.tail(12) << 0.1, 1.0 / 3.0, M_PI, -0.0, 0.0, 1e-300, 4.9e-324, 1.7976931348623157e308, -2.2250738585072014e-308, 400, 123456789012345678.0, 0.30000000000000004;

        const QString text = TraceJson::vectorToString(v);
        // through the serialised JSON document, as in a project file
        QJsonObject obj{ { "x", text } };
        const QJsonObject back = QJsonDocument::fromJson(QJsonDocument(obj).toJson()).object();
        Eigen::VectorXd read;
        QString error;
        QVERIFY2(TraceJson::vectorFromJson(back["x"], &read, &error), qPrintable(error));
        QCOMPARE(read.size(), v.size());
        for (Eigen::Index i = 0; i < v.size(); ++i)
            QVERIFY2(read(i) == v(i), qPrintable(QString::number(i)));
        QVERIFY(std::signbit(read(v.size() - 9))); // -0.0 keeps its sign

        Eigen::VectorXd special(3);
        special << kNaN, std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity();
        QVERIFY(TraceJson::vectorFromJson(TraceJson::vectorToString(special), &read));
        QVERIFY(std::isnan(read(0)) && read(1) == special(1) && read(2) == special(2));

        // measured values stay short: "400" not "400.00000000000000"
        Eigen::VectorXd measured(3);
        measured << 400, 0.1234, 1e-5;
        QCOMPARE(TraceJson::vectorToString(measured), QString("400 0.1234 1e-05"));
        qInfo("vector text: %.2f characters per random full-precision value", double(text.size()) / v.size());
    }

    void vectorOldFormatArrayAndErrors()
    {
        Eigen::VectorXd v(4);
        v << 400.123456789, 0.000123456789, -5, 1e10 / 3;
        // old format: ToolSet::DoubleList2String, 6 significant digits
        const QString old = ToolSet::DoubleList2String(v);
        Eigen::VectorXd read;
        QVERIFY(TraceJson::vectorFromJson(old, &read));
        QVERIFY(identical(read, ToolSet::String2DoubleEigVec(old)));

        QVERIFY(TraceJson::vectorFromJson(QJsonArray{ 1.5, -2, 3e-7 }, &read));
        QCOMPARE(read.size(), Eigen::Index(3));
        QCOMPARE(read(2), 3e-7);
        QVERIFY(TraceJson::vectorFromJson(QString("  1  2\t3 "), &read));
        QCOMPARE(read.size(), Eigen::Index(3));
        QVERIFY(TraceJson::vectorFromJson(QString(), &read));
        QCOMPARE(read.size(), Eigen::Index(0));
        QVERIFY(TraceJson::vectorFromJson(QJsonValue(), &read)); // missing key -> empty

        QString error;
        read = v;
        QVERIFY(!TraceJson::vectorFromJson(QString("1 2 abc 4"), &read, &error));
        QVERIFY(error.contains("abc"));
        QVERIFY(identical(read, v)); // untouched on error
        QVERIFY(!TraceJson::vectorFromJson(QJsonArray{ 1, "x" }, &read, &error));
        QVERIFY(!TraceJson::vectorFromJson(QJsonValue(true), &read, &error));
    }

    void traceJsonRoundtrip()
    {
        std::mt19937_64 rng(1);
        std::normal_distribution<double> n(0, 1);
        Eigen::VectorXd x = grid(10, 0, -0.001); // NMR-like, descending, 10001 points
        Eigen::VectorXd y(x.size());
        for (Eigen::Index i = 0; i < y.size(); ++i)
            y(i) = n(rng);
        XYTrace t(x, y, "step 3");
        t.setSource("/data/nmr/3.csv");
        t.setXAxis("chemical shift", "ppm");
        t.setYAxis("intensity", "a.u.");
        const QJsonObject json = QJsonDocument::fromJson(QJsonDocument(t.toJson()).toJson()).object();
        XYTrace back;
        QString error;
        QVERIFY2(XYTrace::fromJson(json, &back, &error), qPrintable(error));
        QVERIFY(identical(back.x(), t.x()));
        QVERIFY(identical(back.y(), t.y()));
        QCOMPARE(back.name(), t.name());
        QCOMPARE(back.source(), t.source());
        QCOMPARE(back.xUnit(), QString("ppm"));
        QCOMPARE(back.yLabel(), QString("intensity"));
        QCOMPARE(back.orientation(), XYTrace::Orientation::Descending);

        QJsonObject broken = json;
        broken["y"] = "1 2";
        QVERIFY(!XYTrace::fromJson(broken, &back, &error));
        QVERIFY(error.contains("10001"));
    }

    // ----------------------------------------------------------------- pipeline
    void pipelineRoundtripAndNonDestructive()
    {
        TraceOpList list;
        list.append(RangeOp(20, 180));
        list.append(SmoothOp(7, 3));
        BaselineOp base(1);
        base.excluded << qMakePair(80.0, 120.0);
        list.append(base);
        NormaliseOp norm(TraceOps::NormaliseMode::Area);
        norm.useReference = true;
        norm.refStart = 60.5;
        norm.refEnd = 140.25;
        list.append(norm);
        PickPeaksOp pick;
        pick.threshold = 0.3;
        pick.minDistance = 2.5;
        pick.integrate = true;
        list.append(pick);

        const Eigen::VectorXd x = grid(0, 200, 0.5);
        Eigen::VectorXd y(x.size());
        for (Eigen::Index i = 0; i < x.size(); ++i)
            y(i) = gauss(x(i), 50, 100.2, 4) + 3 + 0.01 * x(i);
        const XYTrace raw(x, y, "raw");
        const XYTrace rawCopy = raw;

        QString error;
        QVector<TracePeak> peaks;
        XYTrace processed = list.apply(raw, &error, &peaks);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QVERIFY(identical(raw.x(), rawCopy.x()) && identical(raw.y(), rawCopy.y()));
        QCOMPARE(processed.xMin(), 20.0);
        QCOMPARE(processed.xMax(), 180.0);
        QCOMPARE(processed.name(), QString("raw"));
        QCOMPARE(peaks.size(), 1);
        QVERIFY(std::abs(peaks[0].x - 100.2) < 0.05);

        const QJsonObject json = list.toJson();
        QCOMPARE(json["Version"].toInt(), 1);
        const QJsonObject parsed = QJsonDocument::fromJson(QJsonDocument(json).toJson()).object();
        TraceOpList back;
        QVERIFY2(TraceOpList::fromJson(parsed, &back, &error), qPrintable(error));
        QCOMPARE(back.size(), 5);
        QCOMPARE(back.toJson(), json);
        QVector<TracePeak> peaks2;
        XYTrace again = back.apply(raw, &error, &peaks2);
        QVERIFY(identical(again.y(), processed.y()));
        QCOMPARE(peaks2.size(), 1);
        QCOMPARE(peaks2[0].x, peaks[0].x);

        // deep copy: editing the copy leaves the original alone
        TraceOpList copy = list;
        static_cast<RangeOp*>(copy.at(0))->xStart = 50;
        QCOMPARE(static_cast<const RangeOp*>(list.at(0))->xStart, 20.0);
        QVERIFY(copy.move(4, 0));
        QCOMPARE(copy.at(0)->type(), QString("PickPeaks"));
        QVERIFY(copy.remove(0));
        QCOMPARE(copy.size(), 4);
    }

    void pipelineErrors()
    {
        TraceOpList list;
        QString error;
        auto parse = [&](const QByteArray& text) {
            error.clear();
            return TraceOpList::fromJson(QJsonDocument::fromJson(text).object(), &list, &error);
        };
        QVERIFY(!parse(R"({"Operations": []})"));
        QVERIFY(error.contains("Version"));
        QVERIFY(!parse(R"({"Version": 2, "Operations": []})"));
        QVERIFY(error.contains("2"));
        QVERIFY(!parse(R"({"Version": 1})"));
        QVERIFY(!parse(R"({"Version": 1, "Operations": [{"Type": "Fourier"}]})"));
        QVERIFY(error.contains("Fourier"));
        QVERIFY(!parse(R"({"Version": 1, "Operations": [{"XStart": 1}]})"));
        QVERIFY(error.contains("Type"));
        QVERIFY(!parse(R"({"Version": 1, "Operations": [{"Type": "Smooth", "Window": 4, "Order": 2}]})"));
        QVERIFY(error.contains("odd"));
        QVERIFY(!parse(R"({"Version": 1, "Operations": [{"Type": "Range", "XStart": "a", "XEnd": 2}]})"));
        QVERIFY(!parse(R"({"Version": 1, "Operations": [{"Type": "Range", "XStart": 1}]})"));
        QVERIFY(error.contains("XEnd"));
        QVERIFY(!parse(R"({"Version": 1, "Operations": [{"Type": "Normalise", "Mode": "Median"}]})"));
        QVERIFY(!parse(R"({"Version": 1, "Operations": [{"Type": "Baseline", "Degree": 1, "Excluded": [[1]]}]})"));
        QVERIFY(!parse(R"({"Version": 1, "Operations": [{"Type": "Baseline", "Degree": 1.5}]})"));
        QVERIFY(!parse(R"({"Version": 1, "Operations": [{"Type": "PickPeaks", "Threshold": 3}]})"));
        QVERIFY(!parse(R"({"Version": 1, "Operations": [3]})"));
        QCOMPARE(list.size(), 0); // untouched by every failed parse
        QVERIFY(parse(R"({"Version": 1, "Operations": []})"));

        // a failing step names itself; the result is empty
        TraceOpList smooth;
        smooth.append(SmoothOp(21, 2));
        XYTrace out = smooth.apply(XYTrace(grid(0, 9, 1), grid(0, 9, 1)), &error);
        QVERIFY(out.isEmpty());
        QVERIFY(error.contains("step 1") && error.contains("Smooth"));
    }

    /// Claude Generated: a switched-off step is kept and stored, but not applied
    void pipelineDisabledStep()
    {
        const Eigen::VectorXd x = grid(0, 20, 1);
        Eigen::VectorXd y(x.size());
        for (Eigen::Index i = 0; i < x.size(); ++i)
            y(i) = std::sin(x(i)) + 2;
        const XYTrace raw(x, y);

        TraceOpList list;
        list.append(SmoothOp(5, 2));
        QCOMPARE(list.toJson()["Operations"].toArray()[0].toObject().contains("Enabled"), false);
        list.at(0)->enabled = false;
        QString error;
        XYTrace out = list.apply(raw, &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QVERIFY(identical(out.y(), raw.y()));

        const QJsonObject json = list.toJson();
        QCOMPARE(json["Operations"].toArray()[0].toObject()["Enabled"].toBool(true), false);
        TraceOpList back;
        QVERIFY2(TraceOpList::fromJson(json, &back, &error), qPrintable(error));
        QCOMPARE(back.at(0)->enabled, false);
        QCOMPARE(back.toJson(), json);
        TraceOpList copy = back;
        QCOMPARE(copy.at(0)->enabled, false);
        copy.at(0)->enabled = true;
        QVERIFY(!identical(copy.apply(raw, &error).y(), raw.y()));
    }

    /// Claude Generated (WP3): ShiftAxis moves x by the offset, keeps y and the orientation, round-trips in JSON
    void shiftAxisReferencesTheScale()
    {
        Eigen::VectorXd x(5), y(5);
        x << 8, 7.5, 7, 6.5, 6; // descending ppm
        y << 0, 1, 3, 1, 0;
        const XYTrace raw(x, y, "nmr");
        ShiftAxisOp shift(-0.25);
        QString error;
        const XYTrace out = shift.apply(raw, &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(out.orientation(), XYTrace::Orientation::Descending);
        QCOMPARE(out.name(), QString("nmr"));
        for (Eigen::Index i = 0; i < x.size(); ++i) {
            QCOMPARE(out.x()(i), x(i) - 0.25);
            QCOMPARE(out.y()(i), y(i));
        }
        QVERIFY(identical(raw.x(), x)); // the input is unchanged

        TraceOpList list;
        list.append(shift);
        list.append(PickPeaksOp());
        QVector<TracePeak> peaks;
        list.apply(raw, &error, &peaks);
        QCOMPARE(peaks.size(), 1);
        QCOMPARE(peaks[0].x, 6.75);

        const QJsonObject json = list.toJson();
        QCOMPARE(json["Operations"].toArray()[0].toObject()["Type"].toString(), QString("ShiftAxis"));
        TraceOpList back;
        QVERIFY2(TraceOpList::fromJson(json, &back, &error), qPrintable(error));
        QCOMPARE(back.toJson(), json);

        QVERIFY(!TraceOp::fromJson(QJsonObject{ { "Type", "ShiftAxis" } }, &error));
        QVERIFY(error.contains("Offset"));
        QVERIFY(!ShiftAxisOp(std::numeric_limits<double>::infinity()).validate(&error));
    }

    // ----------------------------------------------------------------- Blank (Claude Generated, Viewer 2)
    /*! \brief A 1H-like spectrum with a dominating standard (height 1) at the end of the axis (TMS, 0 ppm)
     * or in the middle (CHCl3, 7.26 ppm) and three small signals (2 %) is blanked around the standard.
     * Spread: line width sigma, half width of the range (in sigma), points of the spectrum, axis direction.
     * Pinned: points outside the ranges are bit-identical, the blanked run lies on the chord between its
     * neighbours (every discrete slope from the left to the right neighbour equals the chord slope: no
     * step, no kink inside), after the blank Normalise and PickPeaks see only the small signals. */
    void blankRemovesTheStandard()
    {
        double worst_outside = 0, worst_slope = 0, worst_edge = 0;
        int cases = 0;
        for (bool descending : { false, true })
            for (int n : { 2001, 8193, 32769 })
                for (double sigma : { 0.002, 0.005, 0.01, 0.02 })
                    for (double width : { 4.0, 6.0, 10.0 })
                        for (double standard : { 0.0, 7.26 }) {
                            Eigen::VectorXd x(n), y(n);
                            for (int i = 0; i < n; ++i) {
                                const double t = descending ? 10.0 - 10.0 * i / double(n - 1) : 10.0 * i / double(n - 1);
                                x(i) = t;
                                y(i) = gauss(t, 1.0, standard, sigma) + 0.001 * t; // sloping baseline: the chord is not flat
                                for (double c : { 1.5, 3.7, 5.2 })
                                    y(i) += gauss(t, 0.02, c, 0.01);
                            }
                            const XYTrace raw(x, y, "spectrum");
                            BlankOp blank;
                            blank.ranges << qMakePair(standard + width * sigma, standard - width * sigma); // reversed on purpose
                            QString error;
                            const XYTrace out = blank.apply(raw, &error);
                            QVERIFY2(error.isEmpty(), qPrintable(error));
                            QCOMPARE(out.size(), raw.size());
                            const double lo = standard - width * sigma, hi = standard + width * sigma;
                            Eigen::Index first = -1, last = -1;
                            for (Eigen::Index i = 0; i < n; ++i) {
                                QVERIFY(std::isfinite(out.y()(i)));
                                if (x(i) >= lo && x(i) <= hi) {
                                    if (first < 0)
                                        first = i;
                                    last = i;
                                } else
                                    worst_outside = std::max(worst_outside, std::abs(out.y()(i) - y(i)));
                            }
                            QVERIFY(first >= 0);
                            const Eigen::Index left = first - 1, right = last + 1;
                            if (left < 0 || right >= n) {
                                // the standard at the end of the axis: the run takes the value of its one neighbour
                                const double edge = left < 0 ? y(right) : y(left);
                                for (Eigen::Index i = first; i <= last; ++i)
                                    worst_edge = std::max(worst_edge, std::abs(out.y()(i) - edge));
                            } else {
                                const double chord = (y(right) - y(left)) / (x(right) - x(left));
                                const double scale = std::abs(y(left)) + std::abs(y(right));
                                for (Eigen::Index i = left; i < right; ++i) {
                                    const double slope = (out.y()(i + 1) - out.y()(i)) / (x(i + 1) - x(i));
                                    worst_slope = std::max(worst_slope, std::abs(slope - chord) * std::abs(x(right) - x(left)) / scale);
                                }
                            }
                            // after the blank: the largest magnitude is a small signal, picking at 5 % finds the three
                            // (flat baseline here: on a sloping one the tail of the standard leaves a real local maximum)
                            Eigen::VectorXd flat = y;
                            for (int i = 0; i < n; ++i)
                                flat(i) -= 0.001 * x(i);
                            TraceOpList list;
                            list.append(blank);
                            list.append(NormaliseOp());
                            list.append(PickPeaksOp());
                            static_cast<PickPeaksOp*>(list.at(2))->threshold = 0.05;
                            QVector<TracePeak> peaks;
                            const XYTrace processed = list.apply(XYTrace(x, flat), &error, &peaks);
                            QVERIFY2(error.isEmpty(), qPrintable(error));
                            QVERIFY2(peaks.size() == 3, qPrintable(QString("%1 peaks, n %2 sigma %3 width %4 at %5").arg(peaks.size()).arg(n).arg(sigma).arg(width).arg(standard)));
                            for (const TracePeak& p : peaks)
                                QVERIFY(std::abs(p.x - standard) > width * sigma);
                            QVERIFY(processed.y().maxCoeff() <= 1.0);
                            ++cases;
                        }
        qInfo("blank (%d cases: sigma 0.002..0.02 ppm, range +-4..10 sigma, 2001..32769 points, both axis directions, standard at the end and inside): "
              "worst |change| outside %.3g, worst relative slope deviation from the chord %.3g, worst |y - neighbour| of a run at the end %.3g",
            cases, worst_outside, worst_slope, worst_edge);
        QCOMPARE(worst_outside, 0.0);
        QCOMPARE(worst_edge, 0.0);
        QVERIFY(worst_slope <= 1e-9);
    }

    void blankEdgeCases()
    {
        Eigen::VectorXd x(8), y(8);
        x << 0, 1, 2, 3, 4, 5, 6, 7;
        y << 1, 2, 9, 9, 3, 4, 10, 5;
        const XYTrace raw(x, y, "t");
        QString error;
        auto run = [&](const QVector<QPair<double, double>>& ranges) {
            BlankOp op;
            op.ranges = ranges;
            error.clear();
            return op.apply(raw, &error);
        };
        // outside the data, between two points, no ranges: unchanged bit for bit
        QVERIFY(identical(run({ { 10, 20 } }).y(), y));
        QVERIFY(identical(run({ { 2.2, 2.8 } }).y(), y));
        QVERIFY(identical(run({}).y(), y));
        QVERIFY(error.isEmpty());
        // one run: straight line from x 1 (2) to x 4 (3)
        XYTrace out = run({ { 1.5, 3.5 } });
        QCOMPARE(out.y()(2), 2.0 + 1.0 / 3.0);
        QCOMPARE(out.y()(3), 2.0 + 2.0 / 3.0);
        QVERIFY(identical(raw.y(), y)); // the input is unchanged
        // reversed and overlapping ranges act as their union; touching ranges as one run
        QVERIFY(identical(run({ { 3.5, 1.5 } }).y(), out.y()));
        QVERIFY(identical(run({ { 1.5, 2.6 }, { 2.4, 3.5 } }).y(), out.y()));
        QVERIFY(identical(run({ { 1.5, 2.2 }, { 2.8, 3.5 } }).y(), out.y())); // no point between them
        // at the ends: the value of the neighbour
        out = run({ { -1, 1.2 } });
        QCOMPARE(out.y()(0), 9.0);
        QCOMPARE(out.y()(1), 9.0);
        out = run({ { 5.5, 7 } });
        QCOMPARE(out.y()(6), 4.0);
        QCOMPARE(out.y()(7), 4.0);
        // two runs
        out = run({ { 2, 3 }, { 6, 6 } });
        QCOMPARE(out.y()(6), 4.5);
        QCOMPARE(out.y()(2), 2.0 + 1.0 / 3.0);
        // descending axis: the same line
        Eigen::VectorXd xd = x.reverse(), yd = y.reverse();
        BlankOp op;
        op.ranges = { { 1.5, 3.5 } };
        const XYTrace down = op.apply(XYTrace(xd, yd), &error);
        QCOMPARE(down.y()(5), 2.0 + 1.0 / 3.0);
        QCOMPARE(down.y()(4), 2.0 + 2.0 / 3.0);
        // every point covered: an error, no empty trace in silence
        QVERIFY(run({ { -1, 8 } }).isEmpty());
        QVERIFY(error.contains("every point"));
        QVERIFY(run({ { -1, 3.5 }, { 3.5, 9 } }).isEmpty());
        // not finite
        op.ranges = { { 0, std::numeric_limits<double>::infinity() } };
        QVERIFY(!op.validate(&error));
        QVERIFY(op.apply(raw, &error).isEmpty());
    }

    void blankJsonAndPipeline()
    {
        QString error;
        BlankOp blank;
        blank.ranges = { { 7.3, 7.2 }, { -0.1, 0.1 } };
        TraceOpList list;
        list.append(NormaliseOp());
        list.append(blank);
        list.at(1)->enabled = false;
        const QJsonObject json = list.toJson();
        const QJsonObject expected = QJsonDocument::fromJson(R"({"Version": 1, "Operations": [
            {"Type": "Normalise", "Mode": "Max"},
            {"Type": "Blank", "Ranges": [[7.3, 7.2], [-0.1, 0.1]], "Enabled": false}]})")
                                         .object();
        QCOMPARE(json, expected);
        TraceOpList back;
        QVERIFY2(TraceOpList::fromJson(json, &back, &error), qPrintable(error));
        QCOMPARE(back.toJson(), json);
        QVERIFY(!back.at(1)->enabled);
        // a newer version is refused
        QJsonObject newer = json;
        newer["Version"] = TraceOpList::Version + 1;
        QVERIFY(!TraceOpList::fromJson(newer, &back, &error));
        // empty ranges: allowed (no change)
        auto op = TraceOp::fromJson(QJsonObject{ { "Type", "Blank" } }, &error);
        QVERIFY(op);
        QCOMPARE(op->toJson(), (QJsonObject{ { "Type", "Blank" }, { "Ranges", QJsonArray() } }));
        // malformed: message, no crash
        QVERIFY(!TraceOp::fromJson(QJsonObject{ { "Type", "Blank" }, { "Ranges", "7.2 7.3" } }, &error));
        QVERIFY(error.contains("Ranges"));
        QVERIFY(!TraceOp::fromJson(QJsonObject{ { "Type", "Blank" }, { "Ranges", QJsonArray{ QJsonArray{ 1, 2, 3 } } } }, &error));
        QVERIFY(error.contains("pair"));
        QVERIFY(!TraceOp::fromJson(QJsonObject{ { "Type", "Blank" }, { "Ranges", QJsonArray{ QJsonArray{ 1, "a" } } } }, &error));

        // order matters: Normalise on the standard first keeps the scale of the standard
        Eigen::VectorXd x = grid(0, 10, 0.01), y(x.size());
        for (Eigen::Index i = 0; i < x.size(); ++i)
            y(i) = gauss(x(i), 10.0, 5.0, 0.02) + gauss(x(i), 0.5, 2.0, 0.05);
        TraceOpList before, after;
        BlankOp standard;
        standard.ranges = { { 4.8, 5.2 } };
        before.append(NormaliseOp());
        before.append(standard);
        after.append(standard);
        after.append(NormaliseOp());
        const double small_before = before.apply(XYTrace(x, y), &error).y().maxCoeff();
        const double small_after = after.apply(XYTrace(x, y), &error).y().maxCoeff();
        QVERIFY(std::abs(small_before - 0.05) < 1e-6);
        QVERIFY(std::abs(small_after - 1.0) < 1e-12);
    }
};

QTEST_GUILESS_MAIN(TestTrace)
#include "test_trace.moc"
