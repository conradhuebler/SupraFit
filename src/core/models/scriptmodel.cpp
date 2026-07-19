/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2020 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

//#define _CxxThreadPool_TimeOut 10
//#define _CxxThreadPool_Verbose true

#include "CxxThreadPool.h"

#include "src/global_config.h"

#include "src/core/models/AbstractModel.h"
#include "src/core/models/chaiinterpreter.h"
#include "src/core/models/dukmodelinterpreter.h"
#include "src/core/models/pymodelinterpreter.h"

#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include <QJSEngine>

#include <cmath>
#include <iostream>

#include <libpeakpick/mathhelper.h>
#include <libpeakpick/peakpick.h>

#include "scriptmodel.h"

CalculateThread::CalculateThread(int rows, int cols, DataTable* X, DataTable* Global, DataTable* Local, const QStringList& input_names, const QStringList& global_names, const QStringList& local_names, const QString& execute)
    : m_X(X)
    //, m_Global(Global)
    //, m_Local(Local)
    , m_input_names(input_names)
    , m_global_names(global_names)
    , m_local_names(local_names)
    , m_execute(execute)
    , m_rows(rows)
    , m_cols(cols)
{
    m_chai.setInput(m_X->Table());
    m_chai.setGlobal(Global->Table(), m_global_names);
    m_chai.setLocal(Local->Table());
    m_chai.setInputNames(m_input_names);
    // m_chai.setExecute(m_execute);
    // m_chai.InitialiseChai();
    m_result = new DataTable(rows, cols, NULL);
}

void CalculateThread::UpdateParameter(DataTable* Global, DataTable* Local)
{
    m_chai.setGlobal(Global->Table(), m_global_names);
    m_chai.setLocal(Local->Table());
}

CalculateThread::~CalculateThread()
{
    delete m_result;
}

int CalculateThread::execute()
{
    if (m_end > m_rows)
        m_end = m_rows;

    if (m_start > m_rows)
        m_start = m_rows;
    if (m_start == m_end) {
        m_valid = false;
        return 0;
    }
    m_chai.setInput(m_X->Table());
    // m_chai.setGlobal(m_Global->Table(), m_global_names);
    // m_chai.setLocal(m_Local->Table());
    m_chai.setInputNames(m_input_names);
    m_chai.UpdateChai();
    // QString execute = m_execute_chai.join("\n");
    for (int series = 0; series < m_cols; ++series) {
        for (int i = m_start; i < m_end; ++i) {
            QString cache = m_execute;
            for (int parameter = 0; parameter < m_X->columnCount(); ++parameter) {
                cache.replace(
                    m_input_names[parameter],
                    QString::number(m_X->data(i, parameter)));
            }
            int error = 0;

            double result = m_chai.Evaluate(cache.toUtf8(), error);
            if (error == 1) {
                cache.replace("var", "");
                result = m_chai.Evaluate(cache.toUtf8(), error);
            }
            m_result->data(i, series) = result;
            // m_result(i, series, result);
        }
    }
    return 0;
}

ScriptModel::ScriptModel(DataClass* data)
    : AbstractModel(data)
{
    m_complete = false;
    m_pre_input = { ModelName_Json, InputSize_Json, GlobalParameterSize_Json, GlobalParameterNames_Json, LocalParameterSize_Json, LocalParameterNames_Json, PrintX_Json, Engine_Json, ScriptReactions_Json, Equation_Json };
}

ScriptModel::ScriptModel(DataClass* data, const QJsonObject& model)
    : AbstractModel(data)
{
    m_pre_input = { ModelName_Json, InputSize_Json, GlobalParameterSize_Json, GlobalParameterNames_Json, LocalParameterSize_Json, LocalParameterNames_Json, PrintX_Json, Engine_Json, ScriptReactions_Json, Equation_Json };
    m_complete = AbstractModel::DefineModel(model);
}

ScriptModel::ScriptModel(AbstractModel* data)
    : AbstractModel(data)
{
    m_pre_input = { ModelName_Json, InputSize_Json, GlobalParameterSize_Json, GlobalParameterNames_Json, LocalParameterSize_Json, LocalParameterNames_Json, PrintX_Json, Engine_Json, ScriptReactions_Json, Equation_Json };
    // The base copy constructor brings m_defined_model along, so re-run DefineModel() to rebuild the
    // sizes, names, equation, engine and series-awareness from it. Without this every clone (Monte
    // Carlo, cross-validation, statistics, model duplication) silently had LocalParameterSize()==0,
    // SupportSeries()==false and an EMPTY equation — Clone() had its DefineModel call commented out.
    // Mirrors what fl_any_Model does in its copy constructor. Claude Generated.
    m_complete = DefineModel();
}

ScriptModel::~ScriptModel()
{
    if (m_threads.size())
        delete m_thread_pool;
}

void ScriptModel::InitialThreads()
{
    if (m_threads.size())
        return;

    setThreads(8);
    int use_threads = Threads();
    // while(use_threads > DataPoints())
    //     use_threads /= 2;
    m_thread_pool = new CxxThreadPool;
    m_thread_pool->setProgressBar(CxxThreadPool::ProgressBarType::None);
    for (int i = 0; i <= use_threads; ++i) {
        CalculateThread* thread = new CalculateThread(DataPoints(), SeriesCount(), IndependentModel(), GlobalParameter(), LocalParameter(), m_input_names, m_global_parameter_names, m_local_parameter_names, m_equation);
        thread->setRange(DataPoints() / use_threads * i, DataPoints() / use_threads * (i + 1));
        m_thread_pool->addThread(thread);
        m_threads << thread;
    }
    m_thread_pool->setActiveThreadCount(use_threads);
}

bool ScriptModel::DefineModel()
{
    /*
    QJsonObject parse = model;
    if (parse.contains("ModelDefinition"))
        parse = model["ModelDefinition"].toObject();
*/
    QJsonObject object;
    object = m_defined_model["GlobalParameterSize"];
    m_global_parameter_size = object["value"].toInt();

    object = m_defined_model["GlobalParameterNames"];
    if (!object.isEmpty())
        m_global_parameter_names = object["value"].toString().split("|");
    else {
        m_global_parameter_names.clear();
        for (int i = 0; i < m_global_parameter_size; ++i)
            m_global_parameter_names << QString("A%1").arg(i + 1);
    }
    while (m_global_parameter_names.size() > m_global_parameter_size)
        m_global_parameter_names.removeLast();

    object = m_defined_model["LocalParameterSize"];
    m_local_parameter_size = object["value"].toInt();

    // Local parameters are the per-series degrees of freedom, so a scripted model is series-aware
    // exactly when it declares locals. Without this the GUI treats every scripted model as single-
    // series and never enables per-series local editing/fitting (SupportSeries gates LocalEnabled,
    // the results/search widgets and the statistics loops). Claude Generated.
    m_support_series = m_local_parameter_size > 0;

    object = m_defined_model["LocalParameterNames"];
    if (!object.isEmpty())
        m_local_parameter_names = object["value"].toString().split("|");
    else {
        m_local_parameter_names.clear();
        for (int i = 0; i < m_local_parameter_size; ++i)
            m_local_parameter_names << QString("B%1").arg(i + 1);
    }
    while (m_local_parameter_names.size() > m_local_parameter_size)
        m_local_parameter_names.removeLast();

    object = m_defined_model["InputSize"];
    m_input_size = object["value"].toInt();

    // Equation text — read the new "Equation" key, falling back to the legacy "ChaiScript" key of
    // older projects. Accept both the descriptor form ({..., "value": {lines}}) and a raw {lines}
    // object so nothing regresses. Claude Generated.
    auto readEquation = [this](const QString& key) -> QString {
        const QJsonObject o = m_defined_model.value(key);
        if (o.isEmpty())
            return QString();
        const QJsonObject lines = o.contains("value") ? o["value"].toObject() : o;
        // The line keys are numbers stored as strings, and QJsonObject::keys() sorts them
        // LEXICOGRAPHICALLY ("0","1","10","11",…,"2"). Any equation with more than ten lines would be
        // reassembled in the wrong order — silently scrambling blocks and breaking brackets. Sort
        // numerically. Claude Generated.
        QStringList keys = lines.keys();
        std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) {
            bool okA = false, okB = false;
            const int ia = a.toInt(&okA);
            const int ib = b.toInt(&okB);
            return (okA && okB) ? ia < ib : a < b;
        });
        QStringList strings;
        for (const QString& k : keys)
            strings << lines[k].toString();
        return strings.join("\n");
    };
    m_equation = readEquation("Equation");
    if (m_equation.isEmpty())
        m_equation = readEquation("ChaiScript");

    // Scripting backend selection (default ExprTk); the actual fallback to ExprTk for backends that
    // were not compiled in happens in MakeScriptingEngine() when the engine is built.
    {
        const QJsonObject engineObj = m_defined_model.value("Engine");
        m_backend = engineObj.isEmpty() ? ScriptBackend::ExprTk
                                        : ScriptBackendFromString(engineObj["value"].toString());
    }
    // Optional reaction system: gives the equation a native equilibrium solver via spec_solve()/
    // spec_free()/spec_conc(). Empty -> plain equation mode, nothing changes. Claude Generated.
    {
        const QJsonObject reactions = m_defined_model.value("Reactions");
        const QString text = reactions.contains("value") ? reactions["value"].toString() : QString();
        m_has_speciation = !text.trimmed().isEmpty() && m_speciation.setReactions(text);
    }

    m_formula_prepared = false;
    m_engine.reset();

    object = m_defined_model["Name"];
    m_name = object["value"].toString();
    m_name_cached = m_name;
    setName(m_name);
    object = m_defined_model["InputNames"];
    if (!object.isEmpty()) {
        m_input_names = object["value"].toString().split("|");
    } else {
        m_input_names.clear();
        for (int i = 0; i < m_input_size; ++i)
            m_input_names << QString("X%1").arg(i + 1);
    }

    object = m_defined_model["DepModelNames"];
    if (!object.isEmpty()) {
        m_depmodel_names = object["value"].toString().split("|");
    } else {
        m_depmodel_names.clear();
        for (int i = 0; i < m_input_size; ++i)
            m_depmodel_names << QString("X%1").arg(i + 1);
    }

    object = m_defined_model["PrintX"];
    m_calculate_print = object["value"].toString();
    m_x_printout.clear();
    //   m_model_definition = GenerateModelDefinition();
    try {
        PrepareParameter(GlobalParameterSize(), LocalParameterSize());

    } catch (int error) {
        if (error == -2) {
            emit Info()->Warning(tr("Parameter missmatch. I will not allow it!"));
            emit Info()->Message(tr("You have %1 independet rows available, yet you choose %2 parameters. If you want fewer parameters than rows, just don't include them in the equations.").arg(IndependentModel()->columnCount()).arg(InputParameterSize()));
            return false;
        }
    }

    object = m_defined_model["GlobalParameterGuess"];
    QString values = object["value"].toString();

    QStringList limits = values.split("|");
    for (int i = 0; i < limits.size() && i < m_random_global.size(); ++i) {
        QStringList minmax = limits[i].split(";");
        if (minmax.size() != 2)
            continue;
        double min = minmax[0].remove("[").toDouble();
        double max = minmax[1].remove("]").toDouble();
        m_random_global[i] = QPair<double, double>(min, max);
    }

    object = m_defined_model["LocalParameterGuess"];
    values = object["value"].toString();

    limits = values.split("|");
    for (int i = 0; i < limits.size() && i < m_random_local.size(); ++i) {
        QStringList minmax = limits[i].split(";");
        if (minmax.size() != 2)
            continue;
        double min = minmax[0].remove("[").toDouble();
        double max = minmax[1].remove("]").toDouble();
#pragma message("check, implement and finalise once a day")
        for (int j = 0; j < SeriesCount(); ++j)
            m_random_local[j][i] = QPair<double, double>(min, max);
    }

    for (int i = 0; i < m_input_names.size(); ++i)
        IndependentModel()->setHeaderData(i, Qt::Horizontal, m_input_names[i], Qt::DisplayRole);

    for (int i = 0; i < m_depmodel_names.size(); ++i)
        DependentModel()->setHeaderData(i, Qt::Horizontal, m_depmodel_names[i], Qt::DisplayRole);
    // PrepareParameter() resets the enabled masks (m_enabled_global/m_enabled_local) to 0, and only
    // addGlobal-/addLocalParameter() set them again — i.e. only when the optimisation parameters are
    // collected. Without this call every parameter reports Global-/LocalEnabled()==false, so the GUI
    // paints them red and disables their "optimise" checkbox (and the live-update timer of the model
    // definition tab re-triggered DefineModel(), wiping the flags again on every edit). fl_any_Model
    // does the same at the end of its DefineModel(). Claude Generated.
    CollectOptimizationParameters();

    // The engine is (re)built and the equation compiled lazily in PrepareEngine() on the first
    // CalculateVariables(), once all parameter names/sizes are final. Claude Generated.
    UpdateModelDefinition();

#ifdef DEBUG_ON
    // Shows exactly what the model parsed from the definition dialog / project file. Claude Generated.
    qDebug().noquote() << QString("[ScriptModel::DefineModel] inputs=%1 globals=%2 (%3) locals=%4 (%5) series=%6 supportSeries=%7 engine=%8 equation=%9")
                                .arg(m_input_size)
                                .arg(m_global_parameter_size)
                                .arg(m_global_parameter_names.join(","))
                                .arg(m_local_parameter_size)
                                .arg(m_local_parameter_names.join(","))
                                .arg(SeriesCount())
                                .arg(m_support_series)
                                .arg(ScriptBackendName(m_backend))
                                .arg(m_equation.simplified());
#endif
    return true;
}

void ScriptModel::UpdateModelDefinition()
{
    QJsonObject object = m_defined_model["GlobalParameterNames"];
    object["value"] = m_global_parameter_names.join("|");
    m_defined_model["GlobalParameterNames"] = object;

    object = m_defined_model["LocalParameterNames"];
    object["value"] = m_local_parameter_names.join("|");
    m_defined_model["LocalParameterNames"] = object;

    QString guess;
    for (int i = 0; i < GlobalParameterSize(); ++i) {
        guess += "[" + QString::number(GlobalParameter(i)) + ";" + QString::number(GlobalParameter(i)) + "]|";
    }
    guess.removeLast();
    object["value"] = guess;
    m_defined_model["GlobalParameterGuess"] = object;
}

QVector<ScriptModelPreset> ScriptModel::Presets()
{
    // Fill a copy of a base descriptor with its value.
    auto withValue = [](QJsonObject descriptor, const QJsonValue& value) {
        descriptor["value"] = value;
        return descriptor;
    };
    // An equation (possibly multiple ";"-separated statements on separate lines) as the {"0":..,"1":..}
    // object the type-4 (multiline) editor expects; DefineModel joins the lines back with "\n".
    auto eq = [](const QString& text) {
        QJsonObject o;
        const QStringList lines = text.split('\n');
        // Zero-padded so QJsonObject's lexicographic key order matches the line order. CG.
        for (int i = 0; i < lines.size(); ++i)
            o[QStringLiteral("%1").arg(i, 3, 10, QLatin1Char('0'))] = lines[i];
        return o;
    };
    // Assemble one preset's definition block, mirroring the m_pre_input field order so the "New Model"
    // dialog opens with every widget pre-filled. Inputs default to X1..Xn (no InputNames field).
    auto build = [&](const QString& name, int inputSize, int globalSize, const QString& globalNames,
                     int localSize, const QString& localNames, const QString& equation,
                     const QString& printX = QStringLiteral("X1"),
                     const QString& globalGuess = QString()) {
        ScriptModelPreset p;
        p.name = name;
        p.inputSize = inputSize;
        p.block << withValue(ModelName_Json, name)
                << withValue(InputSize_Json, inputSize)
                << withValue(GlobalParameterSize_Json, globalSize)
                << withValue(GlobalParameterNames_Json, globalNames)
                << withValue(LocalParameterSize_Json, localSize)
                << withValue(LocalParameterNames_Json, localNames)
                << withValue(PrintX_Json, printX)
                << withValue(Engine_Json, QStringLiteral("ExprTk"))
                << withValue(Equation_Json, eq(equation));
        if (!globalGuess.isEmpty())
            p.block << withValue(GlobalParameterGuess_Json, globalGuess);
        return p;
    };

    QVector<ScriptModelPreset> presets;
    presets << build(QStringLiteral("Michaelis-Menten"), 1, 2, QStringLiteral("vmax|Km"), 0, QString(),
        QStringLiteral("(vmax*X1)/(Km+X1)"));
    presets << build(QStringLiteral("Linear"), 1, 2, QStringLiteral("m|b"), 0, QString(),
        QStringLiteral("m*X1 + b"));
    presets << build(QStringLiteral("Hill"), 1, 3, QStringLiteral("Vmax|K|n"), 0, QString(),
        QStringLiteral("(Vmax*pow(X1,n))/(pow(K,n) + pow(X1,n))"));
    presets << build(QStringLiteral("Exponential decay"), 1, 2, QStringLiteral("A|k"), 0, QString(),
        QStringLiteral("A*exp(-k*X1)"));
    presets << build(QStringLiteral("Quadratic"), 1, 3, QStringLiteral("a|b|c"), 0, QString(),
        QStringLiteral("a + b*X1 + c*X1*X1"));

    // Titration models: two independent columns (host total X1, guest total X2). The 1:1 complex has a
    // closed form AB = ½[(H0+G0+1/K) − √((H0+G0+1/K)² − 4·H0·G0)] (no equilibrium solver needed). The
    // observable is a SIGNAL per series built from the concentrations weighted by per-series LOCAL
    // coefficients — so each series (dataset/observable) fits its own coefficients (series-aware). The
    // first statement solves the complex, the last statement is the returned signal. Claude Generated.
    // The binding constant is fitted on a log10 scale (lgK), exactly like the built-in binding models:
    // K spans orders of magnitude, so optimising it linearly is badly conditioned and lands on wrong
    // optima. The x axis is the guest/host ratio (equivalents) — plotting against the constant host
    // total X1 would collapse the axis. Claude Generated.
    const QString solveAB = QStringLiteral("var K := pow(10, lgK);\n")
        + QStringLiteral("var AB := 0.5*((X1+X2+1/K) - sqrt((X1+X2+1/K)*(X1+X2+1/K) - 4*X1*X2));");
    const QString ratioX = QStringLiteral("X2/X1");
    const QString lgKGuess = QStringLiteral("[2;6]");
    // UV/Vis / fluorescence: extensive signal = free-host·eHost + complex·eAB (locals per series).
    presets << build(QStringLiteral("Binding 1:1 — signal (Σ c·ε)"), 2, 1, QStringLiteral("lgK"), 2,
        QStringLiteral("eHost|eAB"), solveAB + QStringLiteral("\n(X1 - AB)*eHost + AB*eAB"),
        ratioX, lgKGuess);
    // Host-observed NMR: intensive shift = mole-fraction-weighted host/complex shifts (locals per series).
    presets << build(QStringLiteral("Binding 1:1 — NMR shift"), 2, 1, QStringLiteral("lgK"), 2,
        QStringLiteral("dHost|dAB"), solveAB + QStringLiteral("\n((X1 - AB)*dHost + AB*dAB)/X1"),
        ratioX, lgKGuess);

    // 1:1 / 1:2 (H + G ⇌ HG, H + 2G ⇌ HG2). No closed form — the mass balance has to be solved
    // numerically, which is exactly what makes it the interesting performance case. Substituting
    // [H] = H0 / (1 + β11[G] + β12[G]²) collapses the 2x2 system to ONE equation in the free guest:
    //     f([G]) = [G] + H0·(β11[G] + 2β12[G]²)/(1 + β11[G] + β12[G]²) − G0
    // f is monotonically increasing with f(0) = −G0 < 0 and f(G0) > 0, so bisection on [0, G0] is
    // bracketed, so a SAFEGUARDED Newton is used: take the Newton step when it stays inside the
    // bracket, otherwise bisect. Quadratic convergence (~5 iterations) with the robustness of
    // bisection — a plain 60-step bisection was 2.7x slower. Observable = mole-fraction-weighted
    // host shifts (locals per series). Claude Generated.
    const QString solve112 = QStringLiteral(
        "var b11 := pow(10, lgK11);\n"
        "var b12 := pow(10, lgB12);\n"
        "var lo := 0;\n"
        "var hi := X2;\n"
        "var G := 0.5*X2;\n"
        "var d := 1; var f := 0; var df := 1;\n"
        "var num := 0; var dnum := 0; var dd := 0; var step := 0;\n"
        "var it := 0;\n"
        "while (it < 40)\n"
        "{\n"
        "  d := 1 + b11*G + b12*G*G;\n"
        "  num := b11*G + 2*b12*G*G;\n"
        "  f := G + X1*num/d - X2;\n"
        "  if (f > 0) { hi := G; } else { lo := G; };\n"
        "  if (abs(f) < 1e-16*(X2 + 1e-15)) { it := 1000; }\n"
        "  else\n"
        "  {\n"
        "    dnum := b11 + 4*b12*G;\n"
        "    dd := b11 + 2*b12*G;\n"
        "    df := 1 + X1*(dnum*d - num*dd)/(d*d);\n"
        "    step := G - f/df;\n"
        "    if ((step > lo) and (step < hi)) { G := step; } else { G := 0.5*(lo + hi); };\n"
        "    it += 1;\n"
        "  };\n"
        "};\n"
        "d := 1 + b11*G + b12*G*G;\n"
        "var H := X1/d;\n"
        "var HG := b11*H*G;\n"
        "var HG2 := b12*H*G*G;");
    presets << build(QStringLiteral("Binding 1:1/1:2 — NMR shift"), 2, 2,
        QStringLiteral("lgK11|lgB12"), 3, QStringLiteral("dH|dHG|dHG2"),
        solve112 + QStringLiteral("\n(H*dH + HG*dHG + HG2*dHG2)/X1"),
        ratioX, QStringLiteral("[2;6]|[3;9]"));

    // Same 1:1/1:2 system again, but the free guest comes from the primitive library instead of a
    // hand-written iteration: the mass balance reduces to a cubic in [G], and cubic_root() solves it
    // in closed form. No loop in the script at all. Claude Generated.
    presets << build(QStringLiteral("Binding 1:1/1:2 — cubic_root primitive"), 2, 2,
        QStringLiteral("lgK11|lgB12"), 3, QStringLiteral("dH|dHG|dHG2"),
        QStringLiteral(
            "var b11 := pow(10, lgK11);\n"
            "var b12 := pow(10, lgB12);\n"
            "// mass balance -> cubic in the free guest, cumulative betas: b12 G^3 + (b11 + b12(2H0-G0)) G^2\n"
            "// + (1 + b11(H0-G0)) G - G0 = 0   (equil.h uses STEPWISE K12, hence different coefficients)\n"
            "var G := cubic_root(b12, b11 + b12*(2*X1 - X2), 1 + b11*(X1 - X2), -X2);\n"
            "var d := 1 + b11*G + b12*G*G;\n"
            "var H := X1/d;\n"
            "var HG := b11*H*G;\n"
            "var HG2 := b12*H*G*G;\n"
            "(H*dH + HG*dHG + HG2*dHG2)/X1"),
        ratioX, QStringLiteral("[2;6]|[3;9]"));

    // Same 1:1/1:2 system, but the mass balance is solved by the NATIVE speciation engine called
    // from the script instead of a bisection loop written in ExprTk. Identical physics, so the two are
    // directly comparable — this is the pair the engine benchmark measures. Claude Generated.
    {
        ScriptModelPreset p = build(QStringLiteral("Binding 1:1/1:2 — native solver"), 2, 2,
            QStringLiteral("lgB11|lgB12"), 3, QStringLiteral("dH|dHG|dHG2"),
            QStringLiteral("spec_solve(X1, X2);\n"
                           "(spec_free(0)*dH + spec_conc(0)*dHG + spec_conc(1)*dHG2)/X1"),
            ratioX, QStringLiteral("[2;6]|[3;9]"));
        QJsonObject reactions = ScriptReactions_Json;
        reactions["value"] = QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2");
        p.block << reactions;
        presets << p;
    }
    return presets;
}

void ScriptModel::InitialGuess_Private()
{
    // Start from the middle of each declared guess range. InitialiseRandom() is a no-op unless the
    // application property "InitialiseRandom" is set, so without this every scripted parameter would
    // start at 0 — a hopeless start for e.g. a binding constant, and the reason a fitted constant came
    // out wrong. Writing the tables directly (not setGlobalParameter) bypasses the checked mask, which
    // is what an initial guess should do. Claude Generated.
    for (int i = 0; i < GlobalParameterSize() && i < m_random_global.size(); ++i)
        (*GlobalTable())[i] = 0.5 * (m_random_global[i].first + m_random_global[i].second);

    for (int j = 0; j < SeriesCount() && j < m_random_local.size(); ++j)
        for (int i = 0; i < LocalParameterSize() && i < m_random_local[j].size(); ++i)
            LocalTable()->data(j, i) = 0.5 * (m_random_local[j][i].first + m_random_local[j][i].second);

    InitialiseRandom();
}

void ScriptModel::PrepareEngine()
{
    bool fellBack = false;
    m_engine = MakeScriptingEngine(m_backend, &fellBack);
    if (fellBack)
        emit Info()->Message(tr("Scripting engine '%1' is not available in this build; using ExprTk instead.")
                                 .arg(ScriptBackendName(m_backend)));

    // Attach the native solver BEFORE compiling — spec_solve()/spec_free()/spec_conc() must exist
    // when the parser sees them. Claude Generated.
    m_engine->setSpeciation(m_has_speciation ? &m_speciation : nullptr);

    // All bindable identifiers, in a fixed order: inputs, then globals, then locals.
    QStringList variables;
    variables << m_input_names << m_global_parameter_names << m_local_parameter_names;

    m_formula_prepared = m_engine->prepare(m_equation, variables);
    // A failed compile would otherwise show up as a silently all-zero model. Claude Generated.
    if (!m_formula_prepared)
        emit Info()->Warning(tr("The model equation could not be compiled: %1").arg(m_engine->lastError()));

    // Resolve each parameter/input name to its stable engine slot exactly once — the hot loop then
    // only writes doubles, no per-point name lookups. Claude Generated.
    m_input_slots.resize(m_input_names.size());
    for (int k = 0; k < m_input_names.size(); ++k)
        m_input_slots[k] = m_engine->slotFor(m_input_names[k]);

    m_global_slots.resize(m_global_parameter_names.size());
    for (int g = 0; g < m_global_parameter_names.size(); ++g)
        m_global_slots[g] = m_engine->slotFor(m_global_parameter_names[g]);

    m_local_slots.resize(m_local_parameter_names.size());
    for (int l = 0; l < m_local_parameter_names.size(); ++l)
        m_local_slots[l] = m_engine->slotFor(m_local_parameter_names[l]);
}

void ScriptModel::CalculateVariables()
{
    if (!m_engine || !m_formula_prepared)
        PrepareEngine();

    const int inputs = m_input_slots.size();
    const int globals = m_global_slots.size();
    const int locals = m_local_slots.size();

    // Globals are series-independent — write them once per Calculate().
    for (int g = 0; g < globals; ++g)
        m_engine->set(m_global_slots[g], GlobalParameter(g));

    // Speciation mode: the first SpeciesCount() globals are lg beta and drive the native solver
    // (they are pushed here, not read by the script). Claude Generated.
    if (m_has_speciation) {
        const int species = m_speciation.SpeciesCount();
        std::vector<double> beta(species, 0.0);
        for (int k = 0; k < species && k < GlobalParameterSize(); ++k)
            beta[k] = std::pow(10.0, GlobalParameter(k));
        m_speciation.setStabilityConstants(beta);
    }

    for (int series = 0; series < SeriesCount(); ++series) {
        // Local parameters vary per series — bind this series' row before its points. This is the
        // wiring the old ExprTk path was missing (locals were silently ignored). Claude Generated.
        for (int l = 0; l < locals; ++l)
            m_engine->set(m_local_slots[l], LocalParameter(l, series));

        for (int i = 0; i < DataPoints(); ++i) {
            for (int k = 0; k < inputs; ++k)
                m_engine->set(m_input_slots[k], IndependentModel()->data(i, k));

            // Let spec_solve() warm-start from this point's previous solution. The totals depend only
            // on the point (not the series), so the cache is valid across series too. Claude Generated.
            if (m_has_speciation)
                m_engine->setSpeciationPoint(i);

            int error = 0;
            const double result = m_engine->evaluate(error);
            SetValue(i, series, result);
        }
    }
}

QSharedPointer<AbstractModel> ScriptModel::Clone(bool statistics)
{
    QSharedPointer<ScriptModel> model = QSharedPointer<ScriptModel>(new ScriptModel(this), &QObject::deleteLater);
    // model.data()->DefineModel(GenerateModelDefinition());
    finishClone(model, statistics);
    return model;
}

qreal ScriptModel::PrintOutIndependent(int i) const
{
    QJSEngine engine;

    if (m_calculate_print.isEmpty() || m_calculate_print.isNull())
        return IndependentModel()->data(i);
    else {
        if (i < m_x_printout.size())
            return m_x_printout[i];
        QString term = m_calculate_print;

        for (int cols = 0; cols < IndependentModel()->columnCount(); ++cols) {
            term.replace(QString("X%1").arg(cols + 1), QString::number(IndependentModel()->data(i, cols)));
            // engine.globalObject().setProperty(QString("X%1").arg(cols + 1), IndependentModel()->data(i, cols));
        }
        double result = engine.evaluate(term).toNumber();
        //        qDebug() << i << IndependentModel()->data(i, 0) << result;
        if (engine.hasError()) {
            return IndependentModel()->data(i);
        } else {
            m_x_printout << result;
            return result;
        }
    }
}

#include "scriptmodel.moc"
