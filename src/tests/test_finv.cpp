/*
 * SupraFit - accuracy of the F-distribution quantile (ToolSet::finv / FisherQuantile)
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

/* Pins ToolSet::FisherQuantile (inverse regularised incomplete beta, Halley) against an independent reference: the power series
 * I_x(a, b) = x^a (1-x)^b / (a B(a, b)) * 2F1(a + b, 1; a + 1; x) in quadruple precision (libquadmath, long double without it),
 * inverted by bisection - a different expansion and
 * a different root finder than the implementation (continued fraction, Halley). Closed forms for m = 2 and n = 2 check the
 * reference itself. The grid of the ctest is p in {0.5, 0.68, 0.9, 0.95, 0.99, 0.999}, m 1..40 and 30 values of n in 1..2000;
 * SUPRAFIT_FINV_FULL=1 runs every n in 1..2000, SUPRAFIT_FINV_OLD=1 also reports the deviation of the former Fisher_Dist::finv.
 * Claude Generated (2026). */

#include <QtCore/QElapsedTimer>
#include <QtTest/QtTest>

#include <atomic>
#include <cmath>
#include <mutex>
#include <thread>
#include <vector>

#include <fisher/fisher_dist.h>

#if defined(__SIZEOF_FLOAT128__) && defined(SUPRAFIT_FINV_QUAD)
#include <quadmath.h>
#endif

#include "src/core/toolset.h"

namespace {

#if defined(__SIZEOF_FLOAT128__) && defined(SUPRAFIT_FINV_QUAD)
/* quadruple precision (libquadmath): rounding of the long sums near x -> 1 stays far below double */
typedef __float128 Real;
inline Real LogGamma(Real x) { return lgammaq(x); }
inline Real Log(Real x) { return logq(x); }
inline Real Log1p(Real x) { return log1pq(x); }
inline Real Exp(Real x) { return expq(x); }
inline Real Sqrt(Real x) { return sqrtq(x); }
const Real kSeriesTolerance = Real(1e-36L);
const Real kBisectionTolerance = Real(1e-32L);
#else
typedef long double Real;
inline Real LogGamma(Real x) { return lgammal(x); }
inline Real Log(Real x) { return logl(x); }
inline Real Log1p(Real x) { return log1pl(x); }
inline Real Exp(Real x) { return expl(x); }
inline Real Sqrt(Real x) { return sqrtl(x); }
const Real kSeriesTolerance = 1e-22L;
const Real kBisectionTolerance = 1e-19L;
#endif

/*! \brief I_x(a, b) by the hypergeometric series (all terms positive, no cancellation; ratio -> x, so slow only for x near 1). */
Real SeriesLowerTail(Real x, Real a, Real b)
{
    const Real log_prefactor = LogGamma(a + b) - LogGamma(a + 1) - LogGamma(b) + a * Log(x) + b * Log1p(-x);
    Real term = 1, sum = 1;
    for (long k = 0; k < 100000000; ++k) {
        const Real ratio = (a + b + k) / (a + 1 + k) * x;
        term *= ratio;
        sum += term;
        if (ratio < 1 && term < kSeriesTolerance * sum)
            break;
    }
    return Exp(log_prefactor) * sum;
}

/*! \brief Lower tail I_y(a, b) summed directly (the bisection targets a tail <= 0.5, a complement would cancel); 1 - I_{1-y}(b, a) only for y > 0.999. */
Real ReferenceLowerTail(Real y, Real a, Real b)
{
    if (y <= Real(0.999))
        return SeriesLowerTail(y, a, b);
    return 1 - SeriesLowerTail(1 - y, b, a);
}

/*! \brief Solves I_t(a, b) = target (increasing in t) by bisection, geometric while the bracket spans decades. */
Real ReferenceInverse(Real target, Real a, Real b)
{
    Real lo = Real(1e-300), hi = 1;
    for (int i = 0; i < 600; ++i) {
        const Real mid = (hi / lo > 2) ? Sqrt(lo * hi) : Real(0.5) * (lo + hi);
        if (ReferenceLowerTail(mid, a, b) < target)
            lo = mid;
        else
            hi = mid;
        if (hi - lo <= kBisectionTolerance * hi)
            break;
    }
    return Real(0.5) * (lo + hi);
}

/*! \brief Reference F quantile; the small tail is solved for, as in the implementation (1 - p is exact for p >= 0.5). */
long double ReferenceQuantile(double p, int m, int n)
{
    const Real a = Real(0.5) * m, b = Real(0.5) * n;
    if (p <= 0.5) {
        const Real y = ReferenceInverse(p, a, b);
        return (long double)(n * y / (m * (1 - y)));
    }
    const Real z = ReferenceInverse(Real(1) - Real(p), b, a);
    return (long double)(n * (1 - z) / (m * z));
}

const std::vector<double> kProbabilities = { 0.5, 0.68, 0.9, 0.95, 0.99, 0.999 };

struct Worst {
    double error = 0;
    double p = 0;
    int m = 0, n = 0;
    double value = 0, reference = 0;
};

/*! \brief Worst relative deviation of \p f from the reference over the grid, in parallel. */
template <typename F>
Worst WorstOverGrid(const std::vector<int>& ns, F f, int max_m = 40, const std::vector<double>& ps = kProbabilities)
{
    struct Point {
        double p;
        int m, n;
    };
    std::vector<Point> points;
    for (double p : ps)
        for (int m = 1; m <= max_m; ++m)
            for (int n : ns)
                points.push_back({ p, m, n });
    std::atomic<size_t> next(0);
    std::mutex mutex;
    Worst worst;
    auto work = [&]() {
        Worst local;
        for (size_t i = next++; i < points.size(); i = next++) {
            const Point& pt = points[i];
            const long double reference = ReferenceQuantile(pt.p, pt.m, pt.n);
            const double value = f(pt.p, pt.m, pt.n);
            const double error = double(fabsl((value - reference) / reference));
            if (!(error <= local.error)) // NaN counts as worst
                local = { std::isnan(error) ? INFINITY : error, pt.p, pt.m, pt.n, value, double(reference) };
        }
        std::lock_guard<std::mutex> lock(mutex);
        if (local.error > worst.error)
            worst = local;
    };
    const unsigned threads = qMax(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < threads; ++t)
        pool.emplace_back(work);
    for (auto& t : pool)
        t.join();
    return worst;
}

std::vector<int> GridN()
{
    if (qEnvironmentVariableIntValue("SUPRAFIT_FINV_FULL") == 1) {
        std::vector<int> all;
        for (int n = 1; n <= 2000; ++n)
            all.push_back(n);
        return all;
    }
    return { 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 15, 20, 25, 30, 40, 50, 75, 100, 150, 200, 249, 250, 251, 300, 500, 750, 1000, 1250, 1500, 2000 };
}

void PrintWorst(const char* what, const Worst& w)
{
    qInfo("%s: worst relative error %.3g at p = %g, m = %d, n = %d (%.17g vs %.17g)", what, w.error, w.p, w.m, w.n, w.value, w.reference);
}

} // namespace

class FinvTest : public QObject {
    Q_OBJECT

private slots:
    /*! The reference against closed forms: m = 2 gives x = n/2 ((1-p)^(-2/n) - 1), n = 2 gives y = p^(2/m), x = 2y / (m (1-y)). */
    void referenceAgainstClosedForms()
    {
        double worst = 0;
        for (double p : kProbabilities) {
            for (int k = 1; k <= 2000; k = (k < 40 ? k + 1 : k < 500 ? k * 2 : k + 500)) {
                // expm1/log1p keep the small differences (1-p)^(-2/n) - 1 and 1 - y exact to long double
                const long double exact_m2 = 0.5L * k * expm1l(-2.0L / k * logl(1.0L - p));
                worst = qMax(worst, double(fabsl(ReferenceQuantile(p, 2, k) / exact_m2 - 1)));
                if (k <= 40) {
                    const long double log_y = 2.0L / k * logl((long double)p);
                    const long double exact_n2 = 2 * expl(log_y) / (k * -expm1l(log_y));
                    worst = qMax(worst, double(fabsl(ReferenceQuantile(p, k, 2) / exact_n2 - 1)));
                }
            }
        }
        qInfo("reference vs closed forms (m = 2, n = 2): worst relative error %.3g", worst);
        QVERIFY(worst < 1e-17);
    }

    /*! FisherQuantile against the reference over the grid: worst relative error 1.3e-15 here, 5.7e-15 over the full grid
     * (480000 points, at p = 0.9, m = 1, n = 1715: the log-gamma prefactor for large n); the former version was off by up to 4 %. */
    void quantileAgainstReference()
    {
        const Worst worst = WorstOverGrid(GridN(), [](double p, int m, int n) { return ToolSet::FisherQuantile(p, m, n); });
        PrintWorst("FisherQuantile vs reference", worst);
        QVERIFY(worst.error < 1e-13);
    }

    /*! The memoised finv returns the same values, the fallback for an invalid domain stays 1, the ends 0 and infinity. */
    void memoAndDomain()
    {
        for (double p : kProbabilities)
            for (int m : { 1, 3, 7 })
                for (int n : { 1, 10, 600 }) {
                    QCOMPARE(ToolSet::finv(p, m, n), ToolSet::FisherQuantile(p, m, n));
                    QCOMPARE(ToolSet::finv(p, m, n), ToolSet::FisherQuantile(p, m, n));
                }
        QCOMPARE(ToolSet::finv(95, 3, 10), 1.0);
        QCOMPARE(ToolSet::finv(0.95, 3, 0), 1.0);
        QCOMPARE(ToolSet::finv(0.95, 0, 3), 1.0);
        QCOMPARE(ToolSet::FisherQuantile(0, 3, 10), 0.0);
        QVERIFY(std::isinf(ToolSet::FisherQuantile(1, 3, 10)));
        // Textbook values (F tables): F(0.95; 1, 1) = 161.4476, F(0.95; 3, 10) = 3.708265, F(0.99; 5, 20) = 4.102685
        QVERIFY(std::abs(ToolSet::FisherQuantile(0.95, 1, 1) / 161.4476 - 1) < 1e-6);
        QVERIFY(std::abs(ToolSet::FisherQuantile(0.95, 3, 10) / 3.708265 - 1) < 1e-6);
        QVERIFY(std::abs(ToolSet::FisherQuantile(0.99, 5, 20) / 4.102685 - 1) < 1e-6);
    }

    /*! Time per call of the new and (on request) the former implementation. */
    void timing()
    {
        QElapsedTimer timer;
        timer.start();
        int calls = 0;
        double sink = 0;
        for (double p : kProbabilities)
            for (int m = 1; m <= 40; m += 3)
                for (int n : { 1, 5, 20, 100, 1000 }) {
                    sink += ToolSet::FisherQuantile(p, m, n);
                    ++calls;
                }
        qInfo("FisherQuantile: %.2f us per call (%d calls, %g)", timer.nsecsElapsed() / 1e3 / calls, calls, sink);
        if (qEnvironmentVariableIntValue("SUPRAFIT_FINV_OLD") != 1)
            return;
        timer.restart();
        calls = 0;
        for (double p : kProbabilities)
            for (int m = 1; m <= 40; m += 3)
                for (int n : { 1, 5, 20, 100, 1000 }) {
                    sink += Fisher_Dist::finv(p, m, n);
                    ++calls;
                }
        qInfo("Fisher_Dist::finv: %.2f ms per call (%d calls, %g)", timer.nsecsElapsed() / 1e6 / calls, calls, sink);
    }

    /*! Deviation of the former Simpson stepping (Fisher_Dist::finv) from the reference: reported only, it is what changes. */
    void formerImplementation()
    {
        if (qEnvironmentVariableIntValue("SUPRAFIT_FINV_OLD") != 1)
            QSKIP("SUPRAFIT_FINV_OLD=1 compares the former Fisher_Dist::finv (slow)");
        const auto former = [](double p, int m, int n) { return Fisher_Dist::finv(p, m, n); };
        std::vector<int> capped; // the former version replaced m, n > 250 by 250
        for (int n : GridN())
            if (n <= 250)
                capped.push_back(n);
        for (double p : kProbabilities) {
            const QByteArray label = QByteArray("Fisher_Dist::finv vs reference, p = ") + QByteArray::number(p);
            PrintWorst((label + ", n <= 250").constData(), WorstOverGrid(capped, former, 40, { p }));
            PrintWorst((label + ", n <= 2000").constData(), WorstOverGrid(GridN(), former, 40, { p }));
        }
        std::vector<int> typical; // a fit: up to 10 parameters, 5 to 200 degrees of freedom
        for (int n = 5; n <= 200; ++n)
            typical.push_back(n);
        PrintWorst("Fisher_Dist::finv vs reference, p = 0.95, m <= 10, 5 <= n <= 200", WorstOverGrid(typical, former, 10, { 0.95 }));
    }
};

QTEST_GUILESS_MAIN(FinvTest)
#include "test_finv.moc"
