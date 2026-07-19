/*
 * SupraFit - model-level test for the scripted-model engine (ScriptModel, ExprTk backend)
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
 * Verifies the rebuilt scripted-model engine:
 *  (1) plain equation matches the closed form (Michaelis-Menten);
 *  (2) LOCAL parameters actually participate and vary per series — the bug the old ExprTk path had
 *      (setLocal was a no-op) would make both series identical, which this test rejects;
 *  (3) the legacy "ChaiScript" JSON key still loads (backward compatibility for old projects).
 * Claude Generated.
 */

#include <cmath>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtTest/QtTest>

#include <Eigen/Dense>

#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/models/exprtkinterpreter.h"
#include "src/core/models/scriptmodel.h"
#include "src/global.h"

class TestScriptModel : public QObject {
    Q_OBJECT

private:
    static constexpr int N = 15;

    static QJsonObject descr(const QJsonValue& value)
    {
        QJsonObject o;
        o["value"] = value;
        return o;
    }

    // Build a ScriptModel definition block from its parts. equationKey lets us exercise both the new
    // "Equation" key and the legacy "ChaiScript" key. Claude Generated.
    static QJsonObject makeDefinition(const QString& equation, int globalSize, const QString& globalNames,
        int localSize, const QString& localNames, const QString& equationKey = QStringLiteral("Equation"))
    {
        QJsonObject lines;
        lines["0"] = equation;

        QJsonObject def;
        def["Name"] = descr(QStringLiteral("UnitTest ScriptModel"));
        def["InputSize"] = descr(1);
        def["InputNames"] = descr(QStringLiteral("X1"));
        def["DepModelNames"] = descr(QStringLiteral("Y"));
        def["GlobalParameterSize"] = descr(globalSize);
        def["GlobalParameterNames"] = descr(globalNames);
        def["LocalParameterSize"] = descr(localSize);
        def["LocalParameterNames"] = descr(localNames);
        def[equationKey] = descr(lines);
        return def;
    }

    // A DataClass with `inputs` independent columns and `series` (zeroed) dependent columns of N points.
    static DataClass* makeData(int series, int inputs = 1)
    {
        Eigen::MatrixXd indep(N, inputs);
        Eigen::MatrixXd dep(N, series);
        for (int i = 0; i < N; ++i) {
            indep(i, 0) = 0.1 + 0.5 * i; // X1 = 0.1 .. 7.1 (host total for titration presets)
            if (inputs > 1)
                indep(i, 1) = 2e-3 * i / (N - 1); // X2 = guest total 0 .. 2e-3
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
        return data;
    }

    // Proper titration layout: constant host total, guest titrated in. Claude Generated.
    static DataClass* makeTitrationData(int series)
    {
        Eigen::MatrixXd indep(N, 2);
        Eigen::MatrixXd dep(N, series);
        for (int i = 0; i < N; ++i) {
            indep(i, 0) = 1e-3; // host total
            indep(i, 1) = 3e-3 * i / (N - 1); // guest total 0 .. 3e-3
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
        return data;
    }

    /*! \brief Independent C++ reference for the 1:1/1:2 speciation (bisection on the free guest). */
    static void reference112(double H0, double G0, double b11, double b12,
        double& H, double& G, double& HG, double& HG2)
    {
        double lo = 0.0, hi = G0;
        for (int it = 0; it < 200; ++it) {
            const double g = 0.5 * (lo + hi);
            const double d = 1.0 + b11 * g + b12 * g * g;
            const double f = g + H0 * (b11 * g + 2.0 * b12 * g * g) / d - G0;
            if (f > 0)
                hi = g;
            else
                lo = g;
        }
        G = 0.5 * (lo + hi);
        const double d = 1.0 + b11 * G + b12 * G * G;
        H = H0 / d;
        HG = b11 * H * G;
        HG2 = b12 * H * G * G;
    }

    // Turn a preset's descriptor block into the QHash the model consumes, keyed by each descriptor's
    // "name" field (the same shape the GUI dialog produces). Claude Generated.
    static QHash<QString, QJsonObject> presetToElements(const ScriptModelPreset& preset)
    {
        QHash<QString, QJsonObject> elements;
        for (const QJsonObject& d : preset.block)
            elements[d["name"].toString()] = d;
        return elements;
    }

private slots:
    // (1) closed-form correctness on a single series with only global parameters.
    void testMichaelisMenten()
    {
        DataClass* data = makeData(1);
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
        QVERIFY(!model.isNull());
        QVERIFY(model->DefineModel(makeDefinition("(vmax*X1)/(Km+X1)", 2, "vmax|Km", 0, "")));

        QCOMPARE(model->GlobalParameterSize(), 2);
        QCOMPARE(model->InputParameterSize(), 1);

        const double vmax = 3.5, Km = 0.8;
        model->setGlobalParameter(vmax, 0);
        model->setGlobalParameter(Km, 1);
        model->Calculate();

        for (int i = 0; i < N; ++i) {
            const double x = data->IndependentModel()->data(i, 0);
            const double expected = (vmax * x) / (Km + x);
            const double got = model->ModelTable()->data(i, 0);
            QVERIFY2(std::abs(got - expected) < 1e-9,
                qPrintable(QString("MM point %1: got %2 expected %3").arg(i).arg(got).arg(expected)));
        }
        delete data;
    }

    // (2) local parameters participate AND differ per series. Equation: A1*X1 + B1 (global slope,
    // per-series local intercept). The old path ignored B1 -> both series would be identical.
    void testLocalParametersPerSeries()
    {
        DataClass* data = makeData(2);
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
        QVERIFY(!model.isNull());
        QVERIFY(model->DefineModel(makeDefinition("A1*X1 + B1", 1, "A1", 1, "B1")));

        QCOMPARE(model->GlobalParameterSize(), 1);
        QCOMPARE(model->LocalParameterSize(), 1);

        const double A1 = 2.0;
        const double B0 = 10.0, B1 = -5.0; // per-series intercepts
        model->setGlobalParameter(A1, 0);
        model->setLocalParameter(B0, 0, 0);
        model->setLocalParameter(B1, 0, 1);
        model->Calculate();

        bool seriesDiffer = false;
        for (int i = 0; i < N; ++i) {
            const double x = data->IndependentModel()->data(i, 0);
            const double got0 = model->ModelTable()->data(i, 0);
            const double got1 = model->ModelTable()->data(i, 1);
            QVERIFY2(std::abs(got0 - (A1 * x + B0)) < 1e-9,
                qPrintable(QString("series0 point %1: got %2 expected %3").arg(i).arg(got0).arg(A1 * x + B0)));
            QVERIFY2(std::abs(got1 - (A1 * x + B1)) < 1e-9,
                qPrintable(QString("series1 point %1: got %2 expected %3").arg(i).arg(got1).arg(A1 * x + B1)));
            if (std::abs(got0 - got1) > 1e-9)
                seriesDiffer = true;
        }
        QVERIFY2(seriesDiffer, "series are identical -> local parameters were ignored (regression)");
        delete data;
    }

    // (4) end-to-end demonstration: a scripted Michaelis-Menten model is fitted by the production
    // optimizer (Minimizer/Levenberg-Marquardt) and recovers the parameters from noise-free data.
    void testFitRecoversParameters()
    {
        DataClass* data = makeData(1);
        const double vmaxTruth = 3.5, KmTruth = 0.8;

        // synthesize the ground-truth MM curve with a ScriptModel, use it as the dependent data
        {
            QSharedPointer<AbstractModel> truth = CreateModel(SupraFit::ScriptModel, data);
            QVERIFY(truth->DefineModel(makeDefinition("(vmax*X1)/(Km+X1)", 2, "vmax|Km", 0, "")));
            truth->setGlobalParameter(vmaxTruth, 0);
            truth->setGlobalParameter(KmTruth, 1);
            truth->Calculate();
            data->setDependentTable(new DataTable(truth->ModelTable()->Table()));
        }

        // fit a fresh scripted model from a deliberately off start
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
        QVERIFY(model->DefineModel(makeDefinition("(vmax*X1)/(Km+X1)", 2, "vmax|Km", 0, "")));
        model->InitialGuess();
        model->setGlobalParameter(2.0, 0);
        model->setGlobalParameter(2.0, 1);

        Minimizer minimizer(false);
        minimizer.setModel(model);
        minimizer.Minimize();

        const double vmax = model->GlobalParameter(0);
        const double Km = model->GlobalParameter(1);
        qInfo().noquote() << QString("  scripted MM fit -> vmax=%1 (truth %2)  Km=%3 (truth %4)  SSE=%5")
                                 .arg(vmax, 0, 'g', 6).arg(vmaxTruth)
                                 .arg(Km, 0, 'g', 6).arg(KmTruth)
                                 .arg(model->SSE(), 0, 'g', 3);

        QVERIFY2(model->SSE() < 1e-10, qPrintable(QString("SSE %1 not ~0").arg(model->SSE())));
        QVERIFY2(std::abs(vmax - vmaxTruth) < 1e-3, qPrintable(QString("vmax %1 != %2").arg(vmax).arg(vmaxTruth)));
        QVERIFY2(std::abs(Km - KmTruth) < 1e-3, qPrintable(QString("Km %1 != %2").arg(Km).arg(KmTruth)));
        delete data;
    }

    // (5) the 1:1 titration preset (2 inputs, multi-statement equation with per-series locals) produces a
    // series-aware SIGNAL: each series maps the closed-form concentrations through its own local
    // coefficients, so the series differ. Also exercises ExprTk's multi-statement "var AB := …;" form.
    void testTitrationSignalPreset()
    {
        const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
        int idx = -1;
        for (int i = 0; i < presets.size(); ++i)
            if (presets[i].name.startsWith(QStringLiteral("Binding 1:1 — signal"))) {
                idx = i;
                break;
            }
        QVERIFY2(idx >= 0, "1:1 signal preset not found");
        const ScriptModelPreset& preset = presets[idx];
        QCOMPARE(preset.inputSize, 2);

        DataClass* data = makeData(2, 2); // 2 series, host + guest columns
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
        model->setModelDefinition(presetToElements(preset));
        QVERIFY(model->DefineModel(QJsonObject()));
        QCOMPARE(model->InputParameterSize(), 2);
        QCOMPARE(model->GlobalParameterSize(), 1); // K
        QCOMPARE(model->LocalParameterSize(), 2); // eHost, eAB

        const double lgK = 4.0;
        const double K = std::pow(10.0, lgK);
        const double eHost[2] = { 100.0, 120.0 };
        const double eAB[2] = { 5000.0, 4500.0 };
        model->setGlobalParameter(lgK, 0); // the preset fits lg K, not K
        for (int s = 0; s < 2; ++s) {
            model->setLocalParameter(eHost[s], 0, s);
            model->setLocalParameter(eAB[s], 1, s);
        }
        model->Calculate();

        bool seriesDiffer = false;
        for (int i = 0; i < N; ++i) {
            const double H0 = data->IndependentModel()->data(i, 0);
            const double G0 = data->IndependentModel()->data(i, 1);
            const double sum = H0 + G0 + 1.0 / K;
            const double AB = 0.5 * (sum - std::sqrt(sum * sum - 4.0 * H0 * G0)); // analytic 1:1 complex
            for (int s = 0; s < 2; ++s) {
                const double expected = (H0 - AB) * eHost[s] + AB * eAB[s];
                const double got = model->ModelTable()->data(i, s);
                QVERIFY2(std::abs(got - expected) < 1e-9,
                    qPrintable(QString("1:1 signal point %1 series %2: got %3 expected %4")
                                   .arg(i).arg(s).arg(got).arg(expected)));
            }
            if (std::abs(model->ModelTable()->data(i, 0) - model->ModelTable()->data(i, 1)) > 1e-9)
                seriesDiffer = true;
        }
        QVERIFY2(seriesDiffer, "titration series identical -> not series-aware");
        delete data;
    }

    // (6) the decisive one: fitting a titration signal model must optimise the GLOBAL binding constant
    // AND the per-series LOCAL coefficients together, recovering all of them. If the locals were not
    // collected as optimisation parameters the fit is degenerate and lg K comes out wrong.
    void testTitrationFitRecoversGlobalAndLocals()
    {
        const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
        int idx = -1;
        for (int i = 0; i < presets.size(); ++i)
            if (presets[i].name.startsWith(QStringLiteral("Binding 1:1 — signal"))) {
                idx = i;
                break;
            }
        QVERIFY(idx >= 0);
        const ScriptModelPreset& preset = presets[idx];

        const double lgKTruth = 4.2;
        const double eHostTruth[2] = { 100.0, 120.0 };
        const double eABTruth[2] = { 5000.0, 4500.0 };

        DataClass* data = makeData(2, 2);
        { // synthesize the noise-free truth signal
            QSharedPointer<AbstractModel> truth = CreateModel(SupraFit::ScriptModel, data);
            truth->setModelDefinition(presetToElements(preset));
            QVERIFY(truth->DefineModel(QJsonObject()));
            truth->setGlobalParameter(lgKTruth, 0);
            for (int s = 0; s < 2; ++s) {
                truth->setLocalParameter(eHostTruth[s], 0, s);
                truth->setLocalParameter(eABTruth[s], 1, s);
            }
            truth->Calculate();
            data->setDependentTable(new DataTable(truth->ModelTable()->Table()));
        }

        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
        model->setModelDefinition(presetToElements(preset));
        QVERIFY(model->DefineModel(QJsonObject()));
        model->InitialGuess(); // lg K from the preset guess range, locals from theirs

        // The GUI paints a parameter red and disables its "optimise" checkbox when Global-/LocalEnabled()
        // is false. PrepareParameter() zeroes those masks, so DefineModel() must re-collect the
        // optimisation parameters — otherwise every scripted parameter shows up as excluded.
        for (int g = 0; g < model->GlobalParameterSize(); ++g)
            QVERIFY2(model->GlobalEnabled(g), qPrintable(QString("global %1 reported as excluded").arg(g)));
        for (int l = 0; l < model->LocalParameterSize(); ++l)
            QVERIFY2(model->LocalEnabled(l), qPrintable(QString("local %1 reported as excluded").arg(l)));

        // Re-running DefineModel (the definition tab does this on every edit) must not wipe the flags.
        QVERIFY(model->DefineModel(QJsonObject()));
        for (int l = 0; l < model->LocalParameterSize(); ++l)
            QVERIFY2(model->LocalEnabled(l),
                qPrintable(QString("local %1 excluded after a definition update").arg(l)));

        // Everything the GUI needs to build the per-series parameter widgets:
        // SupportSeries() gates ModelElement creation (modelwidget.cpp), and every series must be active.
        QVERIFY2(model->SupportSeries(), "SupportSeries() false -> GUI builds no per-series widgets");
        QCOMPARE(model->SeriesCount(), 2);
        for (int s = 0; s < 2; ++s)
            QVERIFY2(model->ActiveSignals(s), qPrintable(QString("series %1 inactive -> locals skipped").arg(s)));

        // 1 global (lg K) + 2 locals x 2 series must all be optimisable
        const QVector<qreal> params = model->CollectOptimizationParameters();
        QCOMPARE(params.size(), 5);

        Minimizer minimizer(false);
        minimizer.setModel(model);
        minimizer.Minimize();

        qInfo().noquote() << QString("  1:1 fit -> lgK=%1 (truth %2)  eHost=[%3, %4]  eAB=[%5, %6]  SSE=%7")
                                 .arg(model->GlobalParameter(0), 0, 'g', 6).arg(lgKTruth)
                                 .arg(model->LocalParameter(0, 0), 0, 'g', 6)
                                 .arg(model->LocalParameter(0, 1), 0, 'g', 6)
                                 .arg(model->LocalParameter(1, 0), 0, 'g', 6)
                                 .arg(model->LocalParameter(1, 1), 0, 'g', 6)
                                 .arg(model->SSE(), 0, 'g', 3);

        QVERIFY2(std::abs(model->GlobalParameter(0) - lgKTruth) < 1e-3,
            qPrintable(QString("lgK %1 != truth %2").arg(model->GlobalParameter(0)).arg(lgKTruth)));
        for (int s = 0; s < 2; ++s) {
            QVERIFY2(std::abs(model->LocalParameter(0, s) - eHostTruth[s]) < 1e-2,
                qPrintable(QString("eHost[%1] %2 != %3").arg(s).arg(model->LocalParameter(0, s)).arg(eHostTruth[s])));
            QVERIFY2(std::abs(model->LocalParameter(1, s) - eABTruth[s]) < 1e-2,
                qPrintable(QString("eAB[%1] %2 != %3").arg(s).arg(model->LocalParameter(1, s)).arg(eABTruth[s])));
        }
        delete data;
    }

    // (7) a clone must stay a fully defined model. Clones drive Monte Carlo, cross-validation and
    // statistics; ScriptModel's copy constructor never re-ran DefineModel(), so every clone silently
    // had 0 local parameters, SupportSeries()==false and an empty equation.
    void testCloneKeepsDefinition()
    {
        const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
        int idx = -1;
        for (int i = 0; i < presets.size(); ++i)
            if (presets[i].name.startsWith(QStringLiteral("Binding 1:1 — signal"))) {
                idx = i;
                break;
            }
        QVERIFY(idx >= 0);

        DataClass* data = makeData(2, 2);
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
        model->setModelDefinition(presetToElements(presets[idx]));
        QVERIFY(model->DefineModel(QJsonObject()));
        model->setGlobalParameter(4.2, 0);
        for (int s = 0; s < 2; ++s) {
            model->setLocalParameter(100.0 + 20.0 * s, 0, s);
            model->setLocalParameter(5000.0 - 500.0 * s, 1, s);
        }
        model->Calculate();

        QSharedPointer<AbstractModel> clone = model->Clone(false);
        QVERIFY(!clone.isNull());
        QCOMPARE(clone->GlobalParameterSize(), model->GlobalParameterSize());
        QCOMPARE(clone->LocalParameterSize(), model->LocalParameterSize());
        QCOMPARE(clone->InputParameterSize(), model->InputParameterSize());
        QVERIFY2(clone->SupportSeries(), "clone lost series-awareness");

        clone->Calculate();
        for (int i = 0; i < N; ++i)
            for (int s = 0; s < 2; ++s)
                QVERIFY2(std::abs(clone->ModelTable()->data(i, s) - model->ModelTable()->data(i, s)) < 1e-9,
                    qPrintable(QString("clone differs at point %1 series %2: %3 vs %4")
                                   .arg(i).arg(s).arg(clone->ModelTable()->data(i, s)).arg(model->ModelTable()->data(i, s))));
        delete data;
    }

    // (8) 1:1/1:2 — no closed form, the script solves the mass balance itself (bisection loop). This
    // exercises ExprTk control flow (for/if/local vars) and is the heavy case for performance work.
    void testBinding112Preset()
    {
        const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
        int idx = -1;
        for (int i = 0; i < presets.size(); ++i)
            if (presets[i].name.startsWith(QStringLiteral("Binding 1:1/1:2 — NMR"))) {
                idx = i;
                break;
            }
        QVERIFY2(idx >= 0, "1:1/1:2 preset not found");
        const ScriptModelPreset& preset = presets[idx];
        QCOMPARE(preset.inputSize, 2);

        { // surface the ExprTk parser diagnostic instead of a silently zero model
            QString equation;
            for (const QJsonObject& d : preset.block)
                if (d["name"].toString() == QLatin1String("Equation")) {
                    const QJsonObject lines = d["value"].toObject();
                    QStringList l;
                    for (const QString& k : lines.keys())
                        l << lines[k].toString();
                    equation = l.join("\n");
                }
            ExprTkEngine probe;
            const QStringList vars = { "X1", "X2", "lgK11", "lgB12", "dH", "dHG", "dHG2" };
            const bool compiled = probe.prepare(equation, vars);
            if (!compiled)
                qWarning().noquote() << "ExprTk compile error:" << probe.lastError()
                                     << "\n--- equation ---\n" << equation;
            QVERIFY2(compiled, "1:1/1:2 equation does not compile");
        }

        DataClass* data = makeTitrationData(2);
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
        model->setModelDefinition(presetToElements(preset));
        QVERIFY(model->DefineModel(QJsonObject()));
        QCOMPARE(model->GlobalParameterSize(), 2); // lgK11, lgB12
        QCOMPARE(model->LocalParameterSize(), 3); // dH, dHG, dHG2

        const double lgK11 = 4.0, lgB12 = 7.0;
        const double b11 = std::pow(10.0, lgK11), b12 = std::pow(10.0, lgB12);
        const double dH[2] = { 8.0, 7.5 };
        const double dHG[2] = { 8.6, 8.1 };
        const double dHG2[2] = { 9.1, 8.4 };
        model->setGlobalParameter(lgK11, 0);
        model->setGlobalParameter(lgB12, 1);
        for (int s = 0; s < 2; ++s) {
            model->setLocalParameter(dH[s], 0, s);
            model->setLocalParameter(dHG[s], 1, s);
            model->setLocalParameter(dHG2[s], 2, s);
        }
        model->Calculate();

        bool seriesDiffer = false;
        for (int i = 0; i < N; ++i) {
            const double H0 = data->IndependentModel()->data(i, 0);
            const double G0 = data->IndependentModel()->data(i, 1);
            double H, G, HG, HG2;
            reference112(H0, G0, b11, b12, H, G, HG, HG2);

            // the reference itself must satisfy both mass balances
            QVERIFY2(std::abs(H + HG + HG2 - H0) < 1e-12 * std::max(1.0, H0), "host mass balance violated");
            QVERIFY2(std::abs(G + HG + 2.0 * HG2 - G0) < 1e-9 * std::max(1e-6, G0), "guest mass balance violated");

            for (int s = 0; s < 2; ++s) {
                const double expected = (H * dH[s] + HG * dHG[s] + HG2 * dHG2[s]) / H0;
                const double got = model->ModelTable()->data(i, s);
                QVERIFY2(std::abs(got - expected) < 1e-9,
                    qPrintable(QString("1:1/1:2 point %1 series %2: got %3 expected %4")
                                   .arg(i).arg(s).arg(got).arg(expected)));
            }
            if (std::abs(model->ModelTable()->data(i, 0) - model->ModelTable()->data(i, 1)) > 1e-9)
                seriesDiffer = true;
        }
        QVERIFY2(seriesDiffer, "1:1/1:2 series identical -> not series-aware");
        delete data;
    }

    // (9) the native equilibrium solver called FROM the script (spec_solve/spec_free/spec_conc) must
    // reproduce the same physics as the hand-written bisection — that equivalence is what makes the
    // two presets a fair performance comparison.
    void testNativeSolverPreset()
    {
        const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
        int idx = -1;
        for (int i = 0; i < presets.size(); ++i)
            if (presets[i].name.contains(QStringLiteral("native solver"))) {
                idx = i;
                break;
            }
        QVERIFY2(idx >= 0, "native-solver preset not found");

        DataClass* data = makeTitrationData(2);
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
        model->setModelDefinition(presetToElements(presets[idx]));
        QVERIFY(model->DefineModel(QJsonObject()));
        QCOMPARE(model->GlobalParameterSize(), 2);
        QCOMPARE(model->LocalParameterSize(), 3);
        QCOMPARE(model->InputParameterSize(), 2);

        const double lgB11 = 4.0, lgB12 = 7.0;
        const double b11 = std::pow(10.0, lgB11), b12 = std::pow(10.0, lgB12);
        const double dH[2] = { 8.0, 7.5 };
        const double dHG[2] = { 8.6, 8.1 };
        const double dHG2[2] = { 9.1, 8.4 };
        model->setGlobalParameter(lgB11, 0);
        model->setGlobalParameter(lgB12, 1);
        for (int s = 0; s < 2; ++s) {
            model->setLocalParameter(dH[s], 0, s);
            model->setLocalParameter(dHG[s], 1, s);
            model->setLocalParameter(dHG2[s], 2, s);
        }
        model->Calculate();

        for (int i = 0; i < N; ++i) {
            const double H0 = data->IndependentModel()->data(i, 0);
            const double G0 = data->IndependentModel()->data(i, 1);
            double H, G, HG, HG2;
            reference112(H0, G0, b11, b12, H, G, HG, HG2);
            for (int s = 0; s < 2; ++s) {
                const double expected = (H * dH[s] + HG * dHG[s] + HG2 * dHG2[s]) / H0;
                const double got = model->ModelTable()->data(i, s);
                // solver tolerance, not machine precision: the speciation solver converges to ~1e-12
                QVERIFY2(std::abs(got - expected) < 1e-6 * std::max(1.0, std::abs(expected)),
                    qPrintable(QString("native solver point %1 series %2: got %3 expected %4")
                                   .arg(i).arg(s).arg(got).arg(expected)));
            }
        }
        delete data;
    }

    // (10) backend equivalence: the same equation must give the same numbers on every engine that is
    // compiled in. Uses a plain arithmetic expression valid in both languages (ExprTk's ":=" variable
    // syntax is not ChaiScript's, so the multi-statement presets are ExprTk-only).
    void testChaiScriptBackendAgrees()
    {
#ifndef _Models
        QSKIP("built without _Models -> no ChaiScript backend");
#else
        const QString equation = QStringLiteral("(vmax*X1)/(Km+X1)");
        const double vmax = 3.5, Km = 0.8;

        QVector<double> exprtk, chai;
        for (const QString& engine : { QStringLiteral("ExprTk"), QStringLiteral("ChaiScript") }) {
            DataClass* data = makeData(1);
            QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
            QJsonObject def = makeDefinition(equation, 2, "vmax|Km", 0, "");
            def["Engine"] = descr(engine);
            QVERIFY(model->DefineModel(def));
            model->setGlobalParameter(vmax, 0);
            model->setGlobalParameter(Km, 1);
            model->Calculate();
            QVector<double>& sink = (engine == QLatin1String("ExprTk")) ? exprtk : chai;
            for (int i = 0; i < N; ++i)
                sink << model->ModelTable()->data(i, 0);
            delete data;
        }

        QCOMPARE(exprtk.size(), N);
        QCOMPARE(chai.size(), N);
        for (int i = 0; i < N; ++i) {
            const double x = 0.1 + 0.5 * i;
            const double expected = (vmax * x) / (Km + x);
            QVERIFY2(std::abs(chai[i] - expected) < 1e-9,
                qPrintable(QString("ChaiScript point %1: got %2 expected %3").arg(i).arg(chai[i]).arg(expected)));
            QVERIFY2(std::abs(chai[i] - exprtk[i]) < 1e-12,
                qPrintable(QString("engines disagree at %1: chai %2 vs exprtk %3").arg(i).arg(chai[i]).arg(exprtk[i])));
        }
#endif
    }

    // (11) the primitive library: cubic_root() replaces the hand-written iteration entirely and must
    // give the same concentrations. This is the payoff of exposing efficient native methods.
    void testCubicRootPrimitivePreset()
    {
        const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
        int idx = -1;
        for (int i = 0; i < presets.size(); ++i)
            if (presets[i].name.contains(QStringLiteral("cubic_root"))) {
                idx = i;
                break;
            }
        QVERIFY2(idx >= 0, "cubic_root preset not found");

        DataClass* data = makeTitrationData(2);
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
        model->setModelDefinition(presetToElements(presets[idx]));
        QVERIFY(model->DefineModel(QJsonObject()));

        const double lgK11 = 4.0, lgB12 = 7.0;
        const double b11 = std::pow(10.0, lgK11), b12 = std::pow(10.0, lgB12);
        const double dH[2] = { 8.0, 7.5 };
        const double dHG[2] = { 8.6, 8.1 };
        const double dHG2[2] = { 9.1, 8.4 };
        model->setGlobalParameter(lgK11, 0);
        model->setGlobalParameter(lgB12, 1);
        for (int s = 0; s < 2; ++s) {
            model->setLocalParameter(dH[s], 0, s);
            model->setLocalParameter(dHG[s], 1, s);
            model->setLocalParameter(dHG2[s], 2, s);
        }
        model->Calculate();

        for (int i = 0; i < N; ++i) {
            const double H0 = data->IndependentModel()->data(i, 0);
            const double G0 = data->IndependentModel()->data(i, 1);
            double H, G, HG, HG2;
            reference112(H0, G0, b11, b12, H, G, HG, HG2);
            for (int s = 0; s < 2; ++s) {
                const double expected = (H * dH[s] + HG * dHG[s] + HG2 * dHG2[s]) / H0;
                const double got = model->ModelTable()->data(i, s);
                QVERIFY2(std::abs(got - expected) < 1e-9,
                    qPrintable(QString("cubic_root point %1 series %2: got %3 expected %4")
                                   .arg(i).arg(s).arg(got).arg(expected)));
            }
        }
        delete data;
    }

    // (3) legacy projects store the equation under the old "ChaiScript" key -> must still load.
    void testLegacyChaiScriptKey()
    {
        DataClass* data = makeData(1);
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
        QVERIFY(!model.isNull());
        QVERIFY(model->DefineModel(
            makeDefinition("A1*X1 + A2", 2, "A1|A2", 0, "", QStringLiteral("ChaiScript"))));

        const double A1 = 1.5, A2 = 0.25;
        model->setGlobalParameter(A1, 0);
        model->setGlobalParameter(A2, 1);
        model->Calculate();

        for (int i = 0; i < N; ++i) {
            const double x = data->IndependentModel()->data(i, 0);
            const double expected = A1 * x + A2;
            const double got = model->ModelTable()->data(i, 0);
            QVERIFY2(std::abs(got - expected) < 1e-9,
                qPrintable(QString("legacy point %1: got %2 expected %3").arg(i).arg(got).arg(expected)));
        }
        delete data;
    }
};

QTEST_MAIN(TestScriptModel)
#include "test_scriptmodel.moc"
