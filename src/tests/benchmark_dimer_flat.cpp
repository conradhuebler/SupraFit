/*
 * SupraFit - microbenchmark: LevMar vs VarPro on a FLAT (unidentifiable) dimerisation direction
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
 */

/*
 * Manual evaluation tool (not a ctest). Quantifies the solver-dependent behaviour the VarPro header
 * documents: when the FIT model carries a direction the DATA do not constrain, that direction is flat.
 * Here the data are synthesised from a plain 1:1/1:2 NMR model (NO dimerisation, truth lg β11/β12),
 * but we fit nmr_any WITH a preceding A-dimerisation (2 A ⇌ A2, 1:1, 1:2) — so lg β(A2) has no signal
 * to bind it and is unidentifiable. We compare classic full-vector Levenberg-Marquardt (FitSolver=LevMar)
 * against the variable-projection solver (FitSolver=VarPro) across several start values of lg β(A2):
 *
 *  - reached SSE (clean, noise-free data → a correct fit drives SSE ~0; locals compensate the spurious
 *    dimer either way, so SSE alone is not the tell);
 *  - recovered lg β(A2), lg β11, lg β12 (β11/β12 must recover the truth; β(A2) is flat → ideally driven
 *    to "off" / very negative, or left at the start if the solver sits on the flat direction);
 *  - wall time and convergence flag.
 *
 * Expected signature: VarPro profiles the linear locals out and optimises only the globals on the
 * concentrated SSE → stays on the valley bottom (β11/β12 recover truth). LevMar optimises the full
 * (globals + series×locals) vector jointly → on the flat/curved (β(A2), δ_A) valley it can drift or
 * stall, landing at init-dependent (β, δ) points with β11/β12 off. Build target: benchmark_dimer_flat.
 * Claude Generated.
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include <cstdio>

#include <Eigen/Dense>

#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/global.h"

static QJsonObject strOpt(const QString& v)
{
    QJsonObject o;
    o["value"] = v;
    return o;
}

// Noise-free multi-series 1:1/1:2 NMR signal synthesised at the truth constants + linear locals.
// Truth reactions carry NO dimerisation — the dimer is only added in the fit model.
static DataClass* makeClean112Data(int N, int series, const QString& truthReactions,
    const QVector<double>& truthBetas, double shiftScale)
{
    Eigen::MatrixXd indep(N, 2);
    Eigen::MatrixXd dep(N, series);
    for (int i = 0; i < N; ++i) {
        indep(i, 0) = 1e-3;
        indep(i, 1) = 3e-3 * i / (N - 1);
        for (int j = 0; j < series; ++j)
            dep(i, j) = 0.0;
    }
    DataClass* data = new DataClass();
    data->setIndependentTable(new DataTable(indep));
    data->setDataType(DataClassPrivate::Table);
    data->setSimulateDependent(series);
    data->setDependentTable(new DataTable(dep));
    data->setDataBegin(0);
    data->setDataEnd(N);

    QSharedPointer<AbstractModel> truth = CreateModel(SupraFit::nmr_any, data);
    QJsonObject def;
    def["Reactions"] = strOpt(truthReactions);
    truth->DefineModel(def);
    truth->InitialGuess();
    for (int k = 0; k < truthBetas.size(); ++k)
        truth->setGlobalParameter(truthBetas[k], k);
    for (int s = 0; s < series; ++s)
        for (int p = 0; p < truth->LocalParameterSize(); ++p)
            truth->setLocalParameter(shiftScale * (1.0 - 0.1 * p - 0.03 * s), p, s);
    truth->Calculate();
    data->setDependentTable(new DataTable(truth->ModelTable()->Table()));
    return data;
}

// Fit a fresh nmr_any dimer model with the given solver and a controlled start (initA2, initB11, initB12).
// Returns SSE, recovered globals, convergence flag and the wall time (best of @p reps to damp noise).
struct FitResult {
    double sse = 0;
    double betaA2 = 0, beta11 = 0, beta12 = 0;
    bool converged = false;
    double ms = 0;
};

static FitResult fitDimer(const QString& solver, const QString& fitReactions, QPointer<DataClass> data,
    double initA2, double initB11, double initB12, int reps)
{
    FitResult best;
    best.sse = 1e300;
    best.ms = 1e300;
    for (int r = 0; r < reps; ++r) {
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
        QJsonObject def;
        def["Reactions"] = strOpt(fitReactions);
        model->DefineModel(def);
        QJsonObject cfg = model->getOptimizerConfig();
        cfg["FitSolver"] = solver;
        model->setOptimizerConfig(cfg);
        model->InitialGuess();
        // GP order follows the reaction list: [β(A2), β11, β12]. Identical, controlled start for both solvers.
        model->setGlobalParameter(initA2, 0);
        model->setGlobalParameter(initB11, 1);
        model->setGlobalParameter(initB12, 2);

        QElapsedTimer timer;
        timer.start();
        Minimizer m(false);
        m.setModel(model);
        m.Minimize();
        const double ms = timer.nsecsElapsed() / 1e6;

        if (ms < best.ms)
            best.ms = ms;
        const double sse = model->SSE();
        if (sse < best.sse) {
            best.sse = sse;
            best.betaA2 = model->GlobalParameter(0);
            best.beta11 = model->GlobalParameter(1);
            best.beta12 = model->GlobalParameter(2);
            best.converged = model->isConverged();
        }
    }
    return best;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    qApp->setProperty("threads", 1); // single-thread for clean, comparable timing

    const QString truthReactions = QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2");
    const QString fitReactions = QStringLiteral("2 A <=> A2\nA + B <=> AB\nA + 2 B <=> AB2");
    const QVector<double> truthBetas = { 3.8, 5.9 }; // lg β11, lg β12
    const double shiftScale = 9.0;

    // Controlled, near-truth start for β11/β12 (same for both solvers); only the spurious β(A2) start varies.
    const double initB11 = 3.5, initB12 = 5.5;
    const QVector<double> initA2values = { 1.0, 3.0, 5.0 };
    const QVector<int> seriesCounts = { 3, 10 };
    const int N = 24, reps = 3;

    std::printf("\nDimer-model fit on CLEAN 1:1/1:2 data (no dimer in truth) — flat lg beta(A2) direction\n");
    std::printf("truth: lg beta11=%.2f  lg beta12=%.2f  | fit reactions: 2A<=>A2 + 1:1 + 1:2  | best of %d, %d pts, 1 thread\n",
        truthBetas[0], truthBetas[1], reps, N);
    std::printf("%-6s %-7s %-8s %12s %10s %9s %9s %9s %6s\n",
        "series", "initA2", "solver", "SSE", "lgA2", "lgB11", "lgB12", "time/ms", "conv");
    std::printf("%s\n", QString(86, '-').toLocal8Bit().constData());

    for (int series : seriesCounts) {
        QPointer<DataClass> data = makeClean112Data(N, series, truthReactions, truthBetas, shiftScale);
        for (double initA2 : initA2values) {
            const FitResult lev = fitDimer(QStringLiteral("LevMar"), fitReactions, data, initA2, initB11, initB12, reps);
            const FitResult var = fitDimer(QStringLiteral("VarPro"), fitReactions, data, initA2, initB11, initB12, reps);
            std::printf("%-6d %-7.1f %-8s %12.3e %10.4f %9.4f %9.4f %9.1f %6s\n",
                series, initA2, "LevMar", lev.sse, lev.betaA2, lev.beta11, lev.beta12, lev.ms, lev.converged ? "yes" : "no");
            std::printf("%-6s %-7s %-8s %12.3e %10.4f %9.4f %9.4f %9.1f %6s\n",
                "", "", "VarPro", var.sse, var.betaA2, var.beta11, var.beta12, var.ms, var.converged ? "yes" : "no");
        }
        std::printf("\n");
        delete data;
    }
    std::printf("Read: β11/β12 should recover truth (%.2f/%.2f). lgA2 is flat — a correct fit drives it\n"
                "off (large negative) or leaves it; the *spread* across initA2 vs truth-recovery of β11/β12\n"
                "is the solver-evaluation signal (VarPro profiles locals → valley bottom; LevMar joint drift).\n",
        truthBetas[0], truthBetas[1]);
    return 0;
}