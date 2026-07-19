/*
 * SupraFit - microbenchmark: classic Levenberg-Marquardt vs the opt-in VarPro solver
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
 * Manual perf tool (not a ctest). Fits nmr_any / uvvis_any on synthetic multi-series data with the
 * classic full-vector Levenberg-Marquardt (FitSolver=LevMar) and the variable-projection solver
 * (FitSolver=VarPro), and prints wall-time and the reached SSE. VarPro removes the linear locals from
 * the numerically-differentiated Jacobian, so its advantage grows with the number of series (each
 * series adds locals the classic solver perturbs one-by-one, re-running the speciation solve each
 * time). Build target: benchmark_varpro. Claude Generated.
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonObject>
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

// Host-constant / guest-titrated 2-component data with a synthesised noise-free multi-series signal.
static DataClass* makeData(int N, int series, int modelId, const QString& reactions,
    const QVector<double>& betas, double localScale)
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

    QSharedPointer<AbstractModel> truth = CreateModel(static_cast<SupraFit::Model>(modelId), data);
    QJsonObject def;
    def["Reactions"] = strOpt(reactions);
    truth->DefineModel(def);
    truth->InitialGuess();
    for (int k = 0; k < betas.size(); ++k)
        truth->setGlobalParameter(betas[k], k);
    for (int s = 0; s < series; ++s)
        for (int p = 0; p < truth->LocalParameterSize(); ++p)
            truth->setLocalParameter(localScale * (1.0 - 0.1 * p - 0.03 * s), p, s);
    truth->Calculate();
    data->setDependentTable(new DataTable(truth->ModelTable()->Table()));
    return data;
}

// Median wall-time (ms) of @p reps fresh fits with the given solver; also returns the reached SSE.
static double timeFit(int modelId, const QString& solver, const QString& reactions, QPointer<DataClass> data, int reps, double& sse)
{
    double best = 1e300;
    sse = 0;
    for (int r = 0; r < reps; ++r) {
        QSharedPointer<AbstractModel> model = CreateModel(static_cast<SupraFit::Model>(modelId), data);
        QJsonObject def;
        def["Reactions"] = strOpt(reactions);
        model->DefineModel(def);
        QJsonObject cfg = model->getOptimizerConfig();
        cfg["FitSolver"] = solver;
        model->setOptimizerConfig(cfg);
        model->InitialGuess();
        QElapsedTimer timer;
        timer.start();
        Minimizer m(false);
        m.setModel(model);
        m.Minimize();
        const double ms = timer.nsecsElapsed() / 1e6;
        best = std::min(best, ms); // best-of to suppress scheduling noise
        sse = model->SSE();
    }
    return best;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    qApp->setProperty("threads", 1); // single-thread for clean, comparable timing

    struct Case {
        const char* name;
        int model;
        QString reactions;
        QVector<double> betas;
        double localScale;
    };
    const QVector<Case> cases = {
        { "nmr 1:1/1:2", static_cast<int>(SupraFit::nmr_any), QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2"), { 3.8, 5.9 }, 9.0 },
        { "uvvis 1:1/1:2", static_cast<int>(SupraFit::uvvis_any), QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2"), { 3.8, 5.9 }, 4000.0 },
        { "nmr 2:1/1:1", static_cast<int>(SupraFit::nmr_any), QStringLiteral("A + B <=> AB\n2 A + B <=> A2B"), { 4.2, 6.6 }, 9.0 },
    };
    const QVector<int> seriesCounts = { 3, 10, 30, 60 };
    const int N = 24, reps = 5;

    std::printf("\nVarPro vs classic LevMar - best of %d fits, %d points, single-threaded\n", reps, N);
    std::printf("%-16s %7s %11s %11s %9s   %-10s\n", "case", "series", "LevMar/ms", "VarPro/ms", "speedup", "SSE(Lev|Var)");
    std::printf("%s\n", QString(78, '-').toLocal8Bit().constData());
    for (const Case& c : cases) {
        for (int series : seriesCounts) {
            QPointer<DataClass> data = makeData(N, series, c.model, c.reactions, c.betas, c.localScale);
            double sseLev = 0, sseVar = 0;
            const double lev = timeFit(c.model, QStringLiteral("LevMar"), c.reactions, data, reps, sseLev);
            const double var = timeFit(c.model, QStringLiteral("VarPro"), c.reactions, data, reps, sseVar);
            std::printf("%-16s %7d %11.2f %11.2f %8.2fx   %.1e | %.1e\n",
                c.name, series, lev, var, lev / var, sseLev, sseVar);
            delete data;
        }
        std::printf("\n");
    }
    return 0;
}
