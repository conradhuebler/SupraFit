/*
 * SupraFit - regression tests for the numerical quadrature used by the BC50 post-processing
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

/* Pins SimpsonIntegrate against analytically known integrals. Written after a defect that had gone
 * unnoticed because nothing tested the routine directly: the loop advanced x by 1/increments while
 * using `delta` as the panel width, so the panels overlapped by ~delta^2 each. Over ~1/delta panels
 * that is an O(delta) relative error - first order - which completely masked Simpson's O(h^4), and
 * it also left the interval uncovered near `upper`. Both are caught by the tests below.
 * Claude Generated (2026). */

#include <QtCore/QVector>
#include <QtTest/QtTest>

#include <cmath>
#include <functional>

#include "src/core/libmath.h"

class QuadratureTest : public QObject {
    Q_OBJECT

private:
    static double integrate(std::function<qreal(qreal, const QVector<qreal>)> f, double lower, double upper, double delta)
    {
        return SimpsonIntegrate(lower, upper, f, QVector<qreal>(), delta);
    }

private slots:

    /* Simpson's rule integrates cubics exactly, so any tiling defect shows up immediately:
     * the overlapping panels of the old implementation produced a ~1e-4 relative error here. */
    void exactForPolynomials()
    {
        // int_0^1 x^3 dx = 1/4
        const double cubic = integrate([](qreal x, const QVector<qreal>&) { return x * x * x; }, 0, 1, 1e-3);
        QVERIFY2(std::abs(cubic - 0.25) < 1e-12,
            qPrintable(QString("int x^3 over [0,1] = %1, expected 0.25").arg(cubic, 0, 'g', 17)));

        // int_2_5 (3x^2 - 2x + 1) dx = [x^3 - x^2 + x] = (125-25+5) - (8-4+2) = 105 - 6 = 99
        const double poly = integrate([](qreal x, const QVector<qreal>&) { return 3 * x * x - 2 * x + 1; }, 2, 5, 1e-3);
        QVERIFY2(std::abs(poly - 99.0) < 1e-9,
            qPrintable(QString("int (3x^2-2x+1) over [2,5] = %1, expected 99").arg(poly, 0, 'g', 17)));
    }

    /* An interval other than [0,1]: the old code computed x = lower + i/increments, which spans a
     * unit length regardless of (upper - lower), so it silently integrated the wrong range. */
    void respectsTheIntegrationRange()
    {
        // int_0^10 x dx = 50
        const double linear = integrate([](qreal x, const QVector<qreal>&) { return x; }, 0, 10, 1e-2);
        QVERIFY2(std::abs(linear - 50.0) < 1e-9,
            qPrintable(QString("int x over [0,10] = %1, expected 50").arg(linear, 0, 'g', 17)));
    }

    /* Smooth non-polynomial integrand: refining the step must actually buy accuracy. With the
     * overlap defect the error was stuck at O(delta) and this ratio was ~10 instead of ~1e4. */
    void convergesWithStepSize()
    {
        auto f = [](qreal x, const QVector<qreal>&) { return std::sin(M_PI * x); };
        const double exact = 2.0 / M_PI;

        const double coarse = std::abs(integrate(f, 0, 1, 1e-2) - exact);
        const double fine = std::abs(integrate(f, 0, 1, 1e-3) - exact);

        QVERIFY2(fine < coarse / 1e3,
            qPrintable(QString("10x smaller step improved the error only from %1 to %2")
                    .arg(coarse, 0, 'g', 6)
                    .arg(fine, 0, 'g', 6)));
        QVERIFY(fine < 1e-12);
    }

    /* The BC50 integrands are defined on [0,1) - at complete saturation alpha = x/(1-x) diverges.
     * Some evaluate to inf/inf = NaN there although the limit is finite. A closed rule would
     * propagate that NaN through the whole sum; the endpoint fallback must keep it finite. */
    void handlesRemovableEndpointSingularity()
    {
        // f(x) = sqrt(1 + 4*alpha)/(1 + alpha), alpha = x/(1-x). At x = 1 this is inf/inf = NaN,
        // the true limit is 0. Mirrors BC50::ItoII::BC50_Y.
        auto f = [](qreal x, const QVector<qreal>&) {
            const double alpha = x / (1 - x);
            return std::sqrt(1 + 4 * alpha) / (1 + alpha);
        };
        QVERIFY2(!std::isfinite(f(1.0, QVector<qreal>())), "test integrand should be NaN at the endpoint");

        const double value = integrate(f, 0, 1, 1e-4);
        QVERIFY2(std::isfinite(value), "a removable endpoint singularity must not poison the integral");
        QVERIFY(value > 0);
    }

    /* An integrable endpoint singularity: int_0^1 (1-x)^(-1/2) dx = 2. The integrand is +inf at
     * x = 1, but the integral converges. The result must stay finite and in the right ballpark. */
    void handlesIntegrableEndpointSingularity()
    {
        auto f = [](qreal x, const QVector<qreal>&) { return 1.0 / std::sqrt(1.0 - x); };

        const double value = integrate(f, 0, 1, 1e-5);
        QVERIFY2(std::isfinite(value), "an integrable singularity must not produce inf/NaN");
        QVERIFY2(std::abs(value - 2.0) < 0.05,
            qPrintable(QString("int (1-x)^-0.5 over [0,1] = %1, expected 2").arg(value, 0, 'g', 8)));
    }

    /* Degenerate arguments must not loop forever or divide by zero. */
    void rejectsDegenerateRanges()
    {
        auto f = [](qreal x, const QVector<qreal>&) { return x; };
        QCOMPARE(integrate(f, 1, 1, 1e-3), 0.0);
        QCOMPARE(integrate(f, 5, 2, 1e-3), 0.0);
        QCOMPARE(integrate(f, 0, 1, 0.0), 0.0);
    }

    /* A step larger than the interval must still integrate the interval, not skip it. */
    void handlesStepLargerThanInterval()
    {
        auto f = [](qreal x, const QVector<qreal>&) { return x; };
        // int_0^1 x dx = 0.5, Simpson is exact for it even on a single panel
        QVERIFY(std::abs(integrate(f, 0, 1, 10.0) - 0.5) < 1e-12);
    }
};

QTEST_MAIN(QuadratureTest)

#include "test_quadrature.moc"
