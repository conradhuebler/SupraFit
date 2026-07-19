/*
 * SupraFit - closed-form vs. Newton cubic root solver
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
 * The analytic (Cardano / trigonometric) cubic solver must return an actual root — verified by
 * substituting it back — and, on the cubics the 1:1/1:2 titration model generates, it must agree with
 * the historical Newton search. The Newton path is deliberately NOT asserted to be a root everywhere:
 * it has known defects (its third "root" iterates on the derivative). Claude Generated.
 */

#include <algorithm>
#include <cmath>

#include <QtTest/QtTest>

#include "src/core/libmath.h"

class TestCubicSolver : public QObject {
    Q_OBJECT

private:
    static double f(double a, double b, double c, double d, double x)
    {
        return ((a * x + b) * x + c) * x + d;
    }

    /*! \brief Coefficients of the guest cubic of the 1:1/1:2 model (see equil.h, ItoI_ItoII). */
    static void guestCubic(double H0, double G0, double K11, double K12,
        double& a, double& b, double& c, double& d)
    {
        a = K11 * K12;
        b = K11 * (2.0 * K12 * H0 - K12 * G0 + 1.0);
        c = K11 * (H0 - G0) + 1.0;
        d = -G0;
    }

private slots:
    void cleanup() { CubicSolver::setMethod(CubicSolver::Method::Newton); }

    // The returned value must actually satisfy the cubic, across both discriminant branches.
    void testAnalyticIsARoot()
    {
        struct Case {
            double a, b, c, d;
        };
        const Case cases[] = {
            { 1.0, -6.0, 11.0, -6.0 }, // roots 1, 2, 3 (three real)
            { 1.0, 0.0, 0.0, -8.0 }, // root 2 (one real)
            { 2.0, -4.0, -22.0, 24.0 }, // roots 1, -3, 4
            { 1.0, 0.0, -15.0, -4.0 }, // roots 4, -2±sqrt(3)
            { 1.0, 3.0, 3.0, 1.0 }, // triple root -1
        };
        for (const Case& k : cases) {
            const double x = AnalyticCubicRoot(k.a, k.b, k.c, k.d);
            const double residual = f(k.a, k.b, k.c, k.d, x);
            QVERIFY2(std::abs(residual) < 1e-8,
                qPrintable(QString("cubic (%1,%2,%3,%4): x=%5 residual=%6")
                               .arg(k.a).arg(k.b).arg(k.c).arg(k.d).arg(x).arg(residual)));
        }
    }

    // On the cubics a real 1:1/1:2 titration produces, analytic and Newton must agree, and the root
    // must be a physically valid free guest concentration (0 <= [G] <= G0).
    void testAgreesOnTitrationCubics()
    {
        const double H0 = 1e-3;
        for (double lgK11 : { 2.0, 4.0, 6.0 }) {
            for (double lgK12 : { 1.0, 3.0, 5.0, 7.0, 9.0 }) {
                const double K11 = std::pow(10.0, lgK11);
                const double K12 = std::pow(10.0, lgK12);
                for (int i = 0; i <= 20; ++i) {
                    const double G0 = 3e-3 * i / 20.0;
                    double a, b, c, d;
                    guestCubic(H0, G0, K11, K12, a, b, c, d);

                    CubicSolver::setMethod(CubicSolver::Method::Analytic);
                    const double analytic = MinCubicRoot(a, b, c, d);
                    CubicSolver::setMethod(CubicSolver::Method::Newton);
                    const double newton = MinCubicRoot(a, b, c, d);

                    // At G0 = 0 the root is exactly 0 and d = 0, so a RELATIVE residual criterion is
                    // meaningless there (any numerical epsilon is itself the dominant term). That point
                    // is covered by the bracket and Newton-agreement checks below.
                    if (G0 > 0) {
                        const double terms = std::max({ std::abs(a * analytic * analytic * analytic),
                            std::abs(b * analytic * analytic), std::abs(c * analytic), std::abs(d) });
                        QVERIFY2(std::abs(f(a, b, c, d, analytic)) < 1e-9 * terms,
                            qPrintable(QString("analytic not a root at lgK11=%1 lgK12=%2 G0=%3 (x=%4)")
                                           .arg(lgK11).arg(lgK12).arg(G0).arg(analytic)));
                    }
                    QVERIFY2(analytic >= -1e-15 && analytic <= G0 + 1e-12,
                        qPrintable(QString("free guest %1 outside [0, %2]").arg(analytic).arg(G0)));
                    // The Newton path stops at |f(x)| < 1e-8 ABSOLUTE, so at small concentrations its
                    // root is only good to ~1e-8 — e.g. at G0 = 0 it returns ~3e-9 where the exact
                    // answer (and the analytic solver) is 0. Compare within that limit, not tighter.
                    QVERIFY2(std::abs(analytic - newton) < 1e-8 + 1e-6 * G0,
                        qPrintable(QString("analytic %1 != newton %2 (lgK11=%3 lgK12=%4 G0=%5)")
                                       .arg(analytic).arg(newton).arg(lgK11).arg(lgK12).arg(G0)));
                }
            }
        }
    }

    // The selector must default to the historical path.
    void testDefaultsToNewton()
    {
        QVERIFY(CubicSolver::method() == CubicSolver::Method::Newton);
    }
};

QTEST_MAIN(TestCubicSolver)
#include "test_cubicsolver.moc"
