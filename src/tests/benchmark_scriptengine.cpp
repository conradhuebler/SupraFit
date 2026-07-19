/*
 * SupraFit - scripted-model engine benchmark
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
 * Manual perf tool (NOT a ctest). Answers one question: what does SCRIPTING cost compared to the
 * hardcoded C++ model of the SAME system? Everything below is the 1:1/1:2 host-guest equilibrium:
 *
 *   1. nmr_ItoI_ItoII  — hardcoded C++, the baseline
 *   2. nmr_any         — reaction-driven, native speciation engine
 *   3. scripted        — mass balance solved in ExprTk itself (safeguarded Newton: Newton step,
 *                        bisection fallback when it leaves the bracket)
 *   4. scripted        — same system, but the native speciation engine called from the script
 *                        via spec_solve()/spec_free()/spec_conc()
 *
 * 3 and 4 are physically identical (verified in test_scriptmodel), so the pair isolates exactly what
 * moving the iterative solve out of the interpreter is worth.
 *
 * Two workloads: a raw Calculate() loop (one LM residual evaluation) and a Monte Carlo run, which is
 * the realistic heavy load — MaxSteps resampled data sets, each fitted from scratch, and it goes
 * through Clone(). Build in RELEASE; the debug build distorts the ratios badly. Claude Generated.
 */

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

#include <Eigen/Dense>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonObject>

#include "src/capabilities/montecarlostatistics.h"
#include "src/core/libmath.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/models/scriptmodel.h"
#include "src/global.h"

namespace {

/*! \brief Titration layout: constant host total, guest titrated in, `series` observables. */
DataClass* makeData(int points, int series)
{
    Eigen::MatrixXd indep(points, 2);
    Eigen::MatrixXd dep(points, series);
    for (int i = 0; i < points; ++i) {
        indep(i, 0) = 1e-3;
        indep(i, 1) = 3e-3 * i / double(points - 1);
        for (int j = 0; j < series; ++j)
            dep(i, j) = 0.0;
    }
    DataClass* data = new DataClass();
    data->setIndependentTable(new DataTable(indep));
    data->setDataType(DataClassPrivate::Table);
    data->setSimulateDependent(series);
    data->setDependentTable(new DataTable(dep));
    data->setDataBegin(0);
    data->setDataEnd(points);
    return data;
}

QHash<QString, QJsonObject> presetToElements(const ScriptModelPreset& preset)
{
    QHash<QString, QJsonObject> elements;
    for (const QJsonObject& d : preset.block)
        elements[d["name"].toString()] = d;
    return elements;
}

/*! \brief Build a scripted model from the preset whose name contains @p needle, seed its parameters. */
QSharedPointer<AbstractModel> makeModel(const QString& needle, DataClass* data)
{
    const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
    int idx = -1;
    for (int i = 0; i < presets.size(); ++i)
        if (presets[i].name.contains(needle)) {
            idx = i;
            break;
        }
    if (idx < 0) {
        printf("  !! preset containing '%s' not found\n", qPrintable(needle));
        return QSharedPointer<AbstractModel>();
    }

    QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
    model->setModelDefinition(presetToElements(presets[idx]));
    if (!model->DefineModel(QJsonObject())) {
        printf("  !! DefineModel failed for '%s'\n", qPrintable(presets[idx].name));
        return QSharedPointer<AbstractModel>();
    }
    // plausible constants; locals get distinct values per series
    for (int g = 0; g < model->GlobalParameterSize(); ++g)
        model->setGlobalParameter(g == 0 ? 4.0 : 7.0, g);
    for (int s = 0; s < model->SeriesCount(); ++s)
        for (int p = 0; p < model->LocalParameterSize(); ++p)
            model->setLocalParameter(8.0 + 0.3 * p - 0.1 * s, p, s);
    return model;
}

/*! \brief The hardcoded C++ model of the SAME system — the baseline scripting is measured against. */
QSharedPointer<AbstractModel> makeBuiltin(SupraFit::Model id, DataClass* data, const QString& reactions)
{
    QSharedPointer<AbstractModel> model = CreateModel(id, data);
    if (!model)
        return model;
    if (!reactions.isEmpty()) { // reaction-driven models stay undefined without this
        QJsonObject value;
        value["value"] = reactions;
        QJsonObject def;
        def["Reactions"] = value;
        model->DefineModel(def);
    }
    if (!model->Complete()) {
        printf("  !! model %d is not complete — skipped\n", static_cast<int>(id));
        return QSharedPointer<AbstractModel>();
    }
    model->InitialGuess();
    for (int g = 0; g < model->GlobalParameterSize(); ++g)
        model->setGlobalParameter(g == 0 ? 4.0 : 7.0, g);
    for (int s = 0; s < model->SeriesCount(); ++s)
        for (int p = 0; p < model->LocalParameterSize(); ++p)
            model->setLocalParameter(8.0 + 0.3 * p - 0.1 * s, p, s);
    return model;
}

/*! \brief Time `reps` full Calculate() calls; returns nanoseconds per data-point evaluation. */
double bench(const QSharedPointer<AbstractModel>& model, int reps, int points, int series)
{
    if (!model)
        return -1.0;
    for (int i = 0; i < 5; ++i) // warm up (first call compiles the formula)
        model->Calculate();

    // The constants MUST change between repetitions. Repeating an identical Calculate() lets the
    // speciation warm-start cache resume exactly at the previous solution, so an iterative solver
    // converges in ~0 iterations — an artefact that made the numerical models look faster than the
    // analytic ones and contradicted the Monte Carlo numbers. In a real fit every residual evaluation
    // sees new parameters, which is what this reproduces. Claude Generated.
    const double base = model->GlobalParameter(0);
    const auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r) {
        model->setGlobalParameter(base + 0.01 * ((r % 21) - 10), 0);
        model->Calculate();
    }
    const auto t1 = std::chrono::steady_clock::now();

    const double ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    return ns / double(reps) / double(points) / double(series);
}

/*! \brief Wall time of a Monte Carlo run — the realistic heavy load: MaxSteps resampled data sets,
 * each fitted from scratch. Goes through Clone(), so it also exercises the model-copy path. */
double benchMonteCarlo(const QSharedPointer<AbstractModel>& model, int steps, const QString& fitSolver)
{
    if (!model)
        return -1.0;
    if (!fitSolver.isEmpty()) {
        QJsonObject cfg = model->getOptimizerConfig();
        cfg["FitSolver"] = fitSolver;
        model->setOptimizerConfig(cfg); // inherited by the MC clones
    }
    model->Calculate(); // MC derives its noise level from the current fit residuals

    QJsonObject controller;
    controller["MaxSteps"] = steps;
    controller["VarianceSource"] = 2; // SEy
    controller["Variance"] = 1e-3;
    controller["confidence"] = 95.0;
    controller["PlotBins"] = 30;
    controller["LightWeight"] = true;
    controller["Method"] = 1;

    MonteCarloStatistics mc;
    mc.setModel(model);
    mc.setController(controller);

    const auto t0 = std::chrono::steady_clock::now();
    mc.Run();
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

/*! \brief A scripted model from an explicit equation/engine — for comparing backends on identical
 * input. The equation must be valid in every language under test (no ExprTk-only ":=" statements). */
QSharedPointer<AbstractModel> makeScripted(const QString& equation, const QString& engine,
    int globals, const QString& globalNames, int locals, const QString& localNames, DataClass* data)
{
    auto descr = [](const QJsonValue& v) { QJsonObject o; o["value"] = v; return o; };
    QJsonObject lines;
    lines["000"] = equation;

    QJsonObject def;
    def["Name"] = descr(QStringLiteral("engine benchmark"));
    def["InputSize"] = descr(2);
    def["GlobalParameterSize"] = descr(globals);
    def["GlobalParameterNames"] = descr(globalNames);
    def["LocalParameterSize"] = descr(locals);
    def["LocalParameterNames"] = descr(localNames);
    def["Engine"] = descr(engine);
    def["Equation"] = descr(lines);

    QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
    if (!model->DefineModel(def)) {
        printf("  !! DefineModel failed for engine %s\n", qPrintable(engine));
        return QSharedPointer<AbstractModel>();
    }
    for (int g = 0; g < model->GlobalParameterSize(); ++g)
        model->setGlobalParameter(4.0, g);
    for (int s = 0; s < model->SeriesCount(); ++s)
        for (int p = 0; p < model->LocalParameterSize(); ++p)
            model->setLocalParameter(8.0 + 0.3 * p - 0.1 * s, p, s);
    return model;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    setvbuf(stdout, nullptr, _IONBF, 0);

    qApp->instance()->setProperty("threads", 4);
    // "analytic" selects the closed-form cubic root solver for the fixed titration models.
    const bool analyticCubic = (argc > 1 && QString(argv[1]) == QLatin1String("analytic"));
    if (analyticCubic)
        CubicSolver::setMethod(CubicSolver::Method::Analytic);
    printf("\n  cubic root solver: %s\n", analyticCubic ? "Analytic (closed form)" : "Newton (default)");
    const int points = 40;
    const int series = 6;
    const int reps = 200;

    printf("\nScripted-model engine benchmark\n");
    printf("  %d points x %d series, %d Calculate() reps each\n", points, series, reps);
    printf("  (one Calculate() == one residual evaluation of the LM functor)\n\n");
    printf("  all rows are the SAME 1:1/1:2 system — baseline is the hardcoded C++ model\n\n");
    printf("  %-46s %14s %10s\n", "model", "ns/point-eval", "relative");

    struct Row {
        const char* label;
        SupraFit::Model builtin; ///< Unknown -> scripted model identified by `needle`
        QString needle;
        QString reactions; ///< required by the reaction-driven built-in models
    };
    const Row rows[] = {
        { "nmr_ItoI_ItoII (hardcoded C++)", SupraFit::nmr_ItoI_ItoII, QString(), QString() },
        { "nmr_any (native speciation engine)", SupraFit::nmr_any, QString(), QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2") },
        { "scripted: safeguarded Newton in ExprTk", SupraFit::Unknown, QStringLiteral("Binding 1:1/1:2 — NMR shift"), QString() },
        { "scripted: cubic_root() primitive", SupraFit::Unknown, QStringLiteral("cubic_root"), QString() },
        { "scripted: native solver via spec_solve()", SupraFit::Unknown, QStringLiteral("native solver"), QString() },
    };

    double reference = -1.0;
    for (const Row& row : rows) {
        DataClass* data = makeData(points, series);
        QSharedPointer<AbstractModel> model = (row.builtin == SupraFit::Unknown)
            ? makeModel(row.needle, data)
            : makeBuiltin(row.builtin, data, row.reactions);
        const double ns = bench(model, reps, points, series);
        if (ns < 0) {
            printf("  %-46s %14s %10s\n", row.label, "n/a", "-");
            delete data;
            continue;
        }
        if (reference < 0)
            reference = ns;
        printf("  %-46s %14.1f %9.2fx\n", row.label, ns, ns / reference);
        model.clear();
        delete data;
    }

    // Monte Carlo: the realistic heavy load (MaxSteps resampled data sets, each fitted). Also goes
    // through Clone(), so it exercises the model-copy path. Claude Generated.
    const int mcSteps = 100;
    printf("\n  Monte Carlo (%d steps, full fits, via Clone())\n", mcSteps);
    printf("  VarPro projects the linear local parameters out of the fit — only the hardcoded models\n");
    printf("  implement it; a scripted model has no way to declare that it is linear in its locals.\n\n");
    printf("  %-46s %-8s %10s %10s\n", "model", "solver", "ms total", "relative");
    double mcReference = -1.0;
    for (const Row& row : rows) {
        // LevMar for every model, plus VarPro where the model supports it.
        const QStringList solvers = { QStringLiteral("LevMar"), QStringLiteral("VarPro") };
        for (const QString& solver : solvers) {
            DataClass* data = makeData(points, series);
            QSharedPointer<AbstractModel> model = (row.builtin == SupraFit::Unknown)
                ? makeModel(row.needle, data)
                : makeBuiltin(row.builtin, data, row.reactions);
            if (!model) {
                delete data;
                break;
            }
            if (solver == QLatin1String("VarPro") && !model->SupportsVarPro()) {
                model.clear();
                delete data;
                continue; // not supported -> no row
            }
            const double ms = benchMonteCarlo(model, mcSteps, solver);
            if (ms >= 0) {
                if (mcReference < 0)
                    mcReference = ms;
                printf("  %-46s %-8s %10.0f %9.2fx\n", row.label, qPrintable(solver), ms,
                    mcReference > 0 ? ms / mcReference : 1.0);
            }
            model.clear();
            delete data;
        }
    }

    // Backend comparison: identical equation, identical data, different scripting engine. Small and
    // large expression, so the per-evaluation dispatch overhead can be told apart from the arithmetic.
    printf("\n  Scripting backends (same equation, same data)\n\n");
    printf("  %-24s %-12s %14s %10s\n", "expression", "engine", "ns/point-eval", "relative");
    const QString smallEq = QStringLiteral("(A*X1)/(B+X1)");
    const QString ab11 = QStringLiteral("(0.5*((X1+X2+1/A) - sqrt((X1+X2+1/A)*(X1+X2+1/A) - 4*X1*X2)))");
    const QString largeEq = QString("((X1 - %1)*dH + %1*dAB)/X1").arg(ab11);
    struct Expr {
        const char* label;
        QString eq;
        int globals;
        QString gnames;
        int locals;
        QString lnames;
    };
    const Expr exprs[] = {
        { "small (Michaelis-Menten)", smallEq, 2, QStringLiteral("A|B"), 0, QString() },
        { "large (1:1 closed form)", largeEq, 1, QStringLiteral("A"), 2, QStringLiteral("dH|dAB") },
    };
    for (const Expr& e : exprs) {
        double ref = -1.0;
        for (const QString& engine : AvailableScriptBackends()) {
            DataClass* data = makeData(points, series);
            QSharedPointer<AbstractModel> model
                = makeScripted(e.eq, engine, e.globals, e.gnames, e.locals, e.lnames, data);
            const double ns = bench(model, reps, points, series);
            if (ns > 0) {
                if (ref < 0)
                    ref = ns;
                printf("  %-24s %-12s %14.1f %9.2fx\n", e.label, qPrintable(engine), ns, ns / ref);
            }
            model.clear();
            delete data;
        }
    }
    printf("\n");
    return 0;
}
