/*
 * SupraFit - unit test for the general BFGS equilibrium speciation solver
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
 * Validates ConcentrationSolver (Musketeer method, DOI 10.1039/d4sc03354j) against
 * the analytic 1:1 root, a closed-form homo-dimerisation, and the grid EqnConc_2x solver.
 * Claude Generated.
 */

#include <cmath>
#include <vector>

#include <QtTest/QtTest>

#include <Eigen/Dense>

#include "EqnConc_2.h"

#include "src/core/concentrationsolver.h"
#include "src/core/equil.h"

class TestBFGSSolver : public QObject {
    Q_OBJECT

private:
    static double relError(double got, double expected)
    {
        return std::abs(got - expected) / std::max(std::abs(expected), 1e-30);
    }

private slots:
    /** 1:1 host-guest (A + B <=> AB) against the analytic quadratic root ItoI::HostConcentration. */
    void test_1_1_vs_analytic()
    {
        const double A0 = 1e-3, B0 = 2e-3, logK = 4.0;

        Eigen::MatrixXi M(2, 1); // components A,B  x  complex AB
        M << 1,
            1;

        ConcentrationSolver solver;
        solver.setStoichiometry(M);
        solver.setStabilityConstants({ std::pow(10.0, logK) });
        solver.setTotalConcentrations({ A0, B0 });
        solver.setConvergeThreshold(1e-12);
        solver.Guess();
        std::vector<double> s = solver.solve();

        const double freeA_analytic = ItoI::HostConcentration(A0, B0, logK);
        // free B analytic: B0 - complex, complex = A0 - freeA
        const double freeB_analytic = B0 - (A0 - freeA_analytic);

        QVERIFY(solver.Converged());
        QVERIFY2(relError(s[0], freeA_analytic) < 1e-6,
            qPrintable(QString("free A %1 vs %2").arg(s[0]).arg(freeA_analytic)));
        QVERIFY2(relError(s[1], freeB_analytic) < 1e-6,
            qPrintable(QString("free B %1 vs %2").arg(s[1]).arg(freeB_analytic)));
    }

    /** Pure self-aggregation 2A <=> A2 against the closed-form quadratic mass balance. */
    void test_dimerization_vs_closed_form()
    {
        const double A0 = 1e-3, Kdim = 1e5;

        Eigen::MatrixXi M(1, 1); // component A  x  complex A2
        M << 2;

        ConcentrationSolver solver;
        solver.setStoichiometry(M);
        solver.setStabilityConstants({ Kdim });
        solver.setTotalConcentrations({ A0 });
        solver.setConvergeThreshold(1e-12);
        solver.Guess();
        std::vector<double> s = solver.solve();

        // A0 = a + 2*Kdim*a^2  ->  2*Kdim*a^2 + a - A0 = 0
        const double a_closed = (-1.0 + std::sqrt(1.0 + 8.0 * Kdim * A0)) / (4.0 * Kdim);

        QVERIFY(solver.Converged());
        QVERIFY2(relError(s[0], a_closed) < 1e-6,
            qPrintable(QString("free A %1 vs closed-form %2").arg(s[0]).arg(a_closed)));

        // mass balance closure
        std::vector<double> all = solver.AllConcentrations(); // {A, A2}
        QVERIFY(relError(all[0] + 2.0 * all[1], A0) < 1e-8);
    }

    /** Mixed A_aB_b grid (maxA=maxB=2) cross-checked against the EqnConc_2x polynomial solver. */
    void test_mixed_vs_eqnconc()
    {
        const double A0 = 1e-3, B0 = 1.5e-3;
        const int maxA = 2, maxB = 2;

        // constants flat by EqnConc Index(a,b) = (a-1)*maxB + (b-1); order (1,1),(1,2),(2,1),(2,2)
        std::vector<double> beta(maxA * maxB);
        beta[0] = std::pow(10.0, 4.0); // A1B1
        beta[1] = std::pow(10.0, 6.5); // A1B2
        beta[2] = std::pow(10.0, 6.0); // A2B1
        beta[3] = std::pow(10.0, 9.0); // A2B2

        EqnConc_2x ref;
        ref.setStoichiometry({ maxA, maxB });
        ref.setStabilityConstants(beta);
        ref.setInitialConcentrations({ A0, B0 });
        ref.Guess();
        ref.setMaxIter(10000);
        ref.setConvergeThreshold(1e-16);
        std::vector<double> ref_free = ref.solver();

        // same system as an explicit stoichiometry matrix (columns in the same order)
        Eigen::MatrixXi M(2, maxA * maxB);
        int col = 0;
        for (int a = 1; a <= maxA; ++a)
            for (int b = 1; b <= maxB; ++b) {
                M(0, col) = a;
                M(1, col) = b;
                ++col;
            }

        ConcentrationSolver solver;
        solver.setStoichiometry(M);
        solver.setStabilityConstants(beta);
        solver.setTotalConcentrations({ A0, B0 });
        solver.setConvergeThreshold(1e-12);
        solver.Guess();
        std::vector<double> s = solver.solve();

        QVERIFY(solver.Converged());
        QVERIFY2(relError(s[0], ref_free[0]) < 1e-4,
            qPrintable(QString("free A %1 vs EqnConc %2").arg(s[0]).arg(ref_free[0])));
        QVERIFY2(relError(s[1], ref_free[1]) < 1e-4,
            qPrintable(QString("free B %1 vs EqnConc %2").arg(s[1]).arg(ref_free[1])));

        // mass balance closure for A: A0 = a + sum_j M(0,j) c_j
        std::vector<double> all = solver.AllConcentrations();
        double totalA = all[0];
        for (int j = 0; j < M.cols(); ++j)
            totalA += M(0, j) * all[2 + j];
        QVERIFY2(relError(totalA, A0) < 1e-8,
            qPrintable(QString("mass balance A %1 vs %2").arg(totalA).arg(A0)));
    }

    /** Analytic parameter sensitivities S = dx/dln(beta) (implicit-function theorem, reusing the
     *  solution Hessian) must match central finite differences of the speciation solve. This is the
     *  core of the analytic outer-fit Jacobian (Approach B). Claude Generated. */
    void test_sensitivity_vs_finite_difference()
    {
        const double A0 = 1e-3, B0 = 2e-3;
        Eigen::MatrixXi M(2, 2); // components A,B  x  species AB, A2B
        M << 1, 2,
            1, 1;
        const std::vector<double> beta = { std::pow(10.0, 4.2), std::pow(10.0, 6.6) };

        auto freeConc = [&](const std::vector<double>& b) {
            ConcentrationSolver s;
            s.setMethod(ConcentrationSolver::Method::LevenbergMarquardt);
            s.setStoichiometry(M);
            s.setStabilityConstants(b);
            s.setTotalConcentrations({ A0, B0 });
            s.setConvergeThreshold(1e-13);
            s.Guess();
            return s.solve();
        };

        ConcentrationSolver solver;
        solver.setMethod(ConcentrationSolver::Method::LevenbergMarquardt);
        solver.setStoichiometry(M);
        solver.setStabilityConstants(beta);
        solver.setTotalConcentrations({ A0, B0 });
        solver.setConvergeThreshold(1e-13);
        solver.Guess();
        const std::vector<double> s0 = solver.solve();
        QVERIFY(solver.Converged());
        const Eigen::MatrixXd S = solver.sensitivityMatrix(); // (2 components x 2 species)
        QCOMPARE(int(S.rows()), 2);
        QCOMPARE(int(S.cols()), 2);
        QVERIFY2(S.cwiseAbs().sum() > 1e-6, "sensitivity matrix is all-zero"); // must be non-trivial

        const double h = 1e-4; // perturbation in ln(beta_j)
        for (int j = 0; j < 2; ++j) {
            std::vector<double> bp = beta, bm = beta;
            bp[j] = beta[j] * std::exp(h);
            bm[j] = beta[j] * std::exp(-h);
            const std::vector<double> sp = freeConc(bp);
            const std::vector<double> sm = freeConc(bm);
            for (int c = 0; c < 2; ++c) {
                const double fd = (std::log(sp[c]) - std::log(sm[c])) / (2.0 * h);
                const double analytic = S(c, j);
                QVERIFY2(std::abs(fd - analytic) < 1e-5 * (std::abs(analytic) + 1.0),
                    qPrintable(QString("dx[%1]/dlnbeta[%2]: analytic %3 vs FD %4").arg(c).arg(j).arg(analytic, 0, 'g', 8).arg(fd, 0, 'g', 8)));
            }
        }
    }
};

QTEST_MAIN(TestBFGSSolver)
#include "test_concentrationsolver.moc"
