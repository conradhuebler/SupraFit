/*
 * SupraFit - microbenchmark for the BFGS equilibrium speciation solver
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
 * Times the solver on representative equilibrium systems over a titration sweep (mirroring how the
 * models call it during a fit: one solver instance, per-point setTotalConcentrations + solve). Prints
 * ns/solve and iterations/solve so optimisations can be compared apples-to-apples. Claude Generated.
 */

#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include <Eigen/Dense>

#include "src/core/bfgsconcentrationsolver.h"
#include "src/core/equil.h"

namespace {

struct Scenario {
    std::string name;
    Eigen::MatrixXi stoich; // components x species
    std::vector<double> beta; // linear
    int components;
};

// A titration sweep: component 0 held constant, the others ramped. Returns P total-vectors.
std::vector<std::vector<double>> sweep(int components, int points)
{
    std::vector<std::vector<double>> totals;
    for (int i = 0; i < points; ++i) {
        std::vector<double> t(components);
        t[0] = 1e-3;
        for (int c = 1; c < components; ++c)
            t[c] = 2e-3 * (i + 1) / points / c;
        totals.push_back(t);
    }
    return totals;
}

double g_threshold = 1e-12;

// Return {ns per solve, avg iterations per solve, max mass-balance residual}.
void benchScenario(const Scenario& s, int points, int reps, BFGSConcentrationSolver::Method method)
{
    BFGSConcentrationSolver solver;
    solver.setMethod(method);
    solver.setStoichiometry(s.stoich);
    solver.setStabilityConstants(s.beta);
    solver.setMaxIter(1000);
    solver.setConvergeThreshold(g_threshold);

    const auto totals = sweep(s.components, points);

    // warmup
    for (int i = 0; i < points; ++i) {
        solver.setTotalConcentrations(totals[i]);
        solver.solve();
    }

    long long totalIter = 0;
    double worstResidual = 0.0;
    const auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r) {
        for (int i = 0; i < points; ++i) {
            solver.setTotalConcentrations(totals[i]);
            const std::vector<double> free = solver.solve();
            totalIter += solver.LastIterations();
            if (r == reps - 1) {
                // check mass balance on the last rep
                const std::vector<double> all = solver.AllConcentrations();
                for (int c = 0; c < s.components; ++c) {
                    double bal = free[c];
                    for (int j = 0; j < (int)s.beta.size(); ++j)
                        bal += s.stoich(c, j) * all[s.components + j];
                    worstResidual = std::max(worstResidual, std::abs(bal - totals[i][c]) / totals[i][c]);
                }
            }
        }
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    const long long solves = (long long)reps * points;
    std::printf("  %-22s  %8.0f ns/solve  %6.2f iter/solve  %6.1f M solves/s  resid=%.1e\n",
        s.name.c_str(), ns / solves, double(totalIter) / solves, solves / (ns / 1e9) / 1e6, worstResidual);
}

Eigen::MatrixXi M(std::initializer_list<std::initializer_list<int>> rows)
{
    std::vector<std::vector<int>> r(rows.begin(), rows.end());
    Eigen::MatrixXi m(r.size(), r[0].size());
    for (int i = 0; i < (int)r.size(); ++i)
        for (int j = 0; j < (int)r[i].size(); ++j)
            m(i, j) = r[i][j];
    return m;
}

} // namespace

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0); // unbuffered so progress shows when redirected
    const int points = 32;
    const int reps = (argc > 1) ? std::atoi(argv[1]) : 1000;
    if (argc > 2)
        g_threshold = std::atof(argv[2]);
    std::printf("convergence threshold = %.0e\n", g_threshold);
    std::printf("BFGS speciation benchmark (%d-point sweep x %d reps = %d solves/scenario)\n",
        points, reps, points * reps);

    std::vector<Scenario> scenarios;
    // 1:1  A + B <=> AB
    scenarios.push_back({ "1:1 (AB)", M({ { 1 }, { 1 } }), { 1e4 }, 2 });
    // 2:1/1:1 grid  AB, A2B
    scenarios.push_back({ "2:1/1:1 (AB,A2B)", M({ { 1, 2 }, { 1, 1 } }), { 1e4, 1e7 }, 2 });
    // 1:1/1:2 grid  AB, AB2
    scenarios.push_back({ "1:1/1:2 (AB,AB2)", M({ { 1, 1 }, { 1, 2 } }), { 1e4, 1e7 }, 2 });
    // self-aggregation  AB, A2
    scenarios.push_back({ "self-agg (AB,A2)", M({ { 1, 2 }, { 1, 0 } }), { 1e4, 1e5 }, 2 });
    // 3-component competitive  AB, AC
    scenarios.push_back({ "3-comp (AB,AC)", M({ { 1, 1 }, { 1, 0 }, { 0, 1 } }), { 1e4, 3e3 }, 3 });

    // Compare both minimisation methods head to head (argv[3] = "bfgs" or "levmar" limits to one).
    std::vector<BFGSConcentrationSolver::Method> methods = {
        BFGSConcentrationSolver::Method::LevenbergMarquardt, BFGSConcentrationSolver::Method::BFGS
    };
    if (argc > 3)
        methods = { BFGSConcentrationSolver::MethodFromString(QString::fromLatin1(argv[3])) };

    for (BFGSConcentrationSolver::Method method : methods) {
        std::printf("--- method: %s ---\n", BFGSConcentrationSolver::MethodToString(method).toLatin1().constData());
        for (const Scenario& s : scenarios) {
            std::printf("  running %-20s ...\r", s.name.c_str());
            benchScenario(s, points, reps, method);
        }
    }

    // Reference: closed-form analytic 1:1 root (the hard-coded model path).
    {
        const auto totals = sweep(2, points);
        volatile double sink = 0;
        const auto t0 = std::chrono::steady_clock::now();
        for (int r = 0; r < reps; ++r)
            for (int i = 0; i < points; ++i)
                sink += ItoI::HostConcentration(totals[i][0], totals[i][1], 4.0);
        const auto t1 = std::chrono::steady_clock::now();
        const double ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        std::printf("  %-22s  %8.0f ns/solve  (analytic closed form, hard-coded 1:1)\n",
            "ItoI::HostConc", ns / ((long long)reps * points));
        (void)sink;
    }
    return 0;
}
