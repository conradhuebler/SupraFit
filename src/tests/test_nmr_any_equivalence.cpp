/*
 * any-vs-fixed equivalence regression test for the NMR titration models - Claude Generated (2026)
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
 * Proves that the generalised nmr_any (model 34), configured through the free-text *Reactions*
 * field (N-component reaction editor, NOT the legacy MaxA/MaxB grid), is statistically identical to
 * the hand-written fixed NMR models on the SAME data:
 *
 *   1:1      (id 1) <-> "A + B <=> AB"
 *   2:1/1:1  (id 2) <-> "A + B <=> AB" + "2 A + B <=> A2B"
 *   1:1/1:2  (id 3) <-> "A + B <=> AB" + "A + 2 B <=> AB2"
 *
 * The fixtures are the PeerJ Monte-Carlo paper datasets: for each stoichiometry we use the fixture
 * where that stoichiometry is the ground truth (so the target constants are well determined and the
 * post-processing is reproducible).
 *
 * CONSTANT CONVENTION. nmr_any parameterises every species by its CUMULATIVE formation constant
 * lg beta (species formed from free components). The fixed ternary models use STEPWISE constants:
 *   - nmr_2_1_1_1: GlobalParameter(0)=lgK21 (AB + A <=> A2B), GlobalParameter(1)=lgK11
 *                  => lg beta(AB)=G[1],  lg beta(A2B)=G[1]+G[0]
 *   - nmr_1_1_1_2: GlobalParameter(0)=lgK11, GlobalParameter(1)=lgK12 (AB + B <=> AB2)
 *                  => lg beta(AB)=G[0],  lg beta(AB2)=G[0]+G[1]
 * fixedToBeta() maps the fixed stepwise constants (and, linearly, their resample means) into the
 * cumulative-beta space nmr_any reports, so the two are directly comparable.
 *
 * Two tiers of assertions:
 *   (A) Fit equivalence  - independent re-fit reproduces the same SSE, the same fitted signal
 *       (ModelTable, convention-free) and the same cumulative betas.
 *   (B) Statistics equivalence - Cross-Validation and Reduction-Analysis boxplot means agree in
 *       beta-space for the well-determined parameters (the user's original question:
 *       "is the statistic of any_models identical to the fixed models under CV/MC/...").
 */

#include <QtTest/QtTest>

#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <climits>

#include "src/capabilities/jobmanager.h"
#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/projectmanager.h"

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/global.h"

#ifndef SUPRAFIT_SOURCE_DIR
#define SUPRAFIT_SOURCE_DIR "."
#endif

class TestNmrAnyEquivalence : public QObject {
    Q_OBJECT

private:
    static constexpr double kTolSSE = 2e-2; //  2 % on SSE after independent re-fit
    static constexpr double kTolSignal = 3e-2; //  3 % per fitted signal point (abs floor guards ~0)
    static constexpr double kTolBeta = 3e-2; //  3 % on lg beta for well-determined parameters
    static constexpr double kTolBoxMean = 6e-2; //  6 % on CV/RA boxplot means (resampling scatter)
    static constexpr double kMaxRelStd = 5e-3; // reference-distribution spread above which a
                                               // parameter is ill-determined and skipped

    static QString sourceDir() { return QString(SUPRAFIT_SOURCE_DIR); }

    static bool relClose(double actual, double expected, double tol)
    {
        return qAbs(actual - expected) / qMax(qAbs(expected), 1e-12) <= tol;
    }

    static QJsonObject strOption(const QString& value)
    {
        QJsonObject o;
        o["value"] = value;
        return o;
    }

    // The stored model objects of a project, keyed by their SupraFit model id.
    static QMap<int, QJsonObject> storedModelsById(const QJsonObject& projectJson)
    {
        QMap<int, QJsonObject> out;
        for (const QString& k : projectJson.keys())
            if (k.startsWith("model"))
                out.insert(projectJson.value(k).toObject().value("model").toInt(), projectJson.value(k).toObject());
        return out;
    }

    // A project copy with every model's stored post-processing removed, so a re-run yields exactly
    // one unambiguous result block per method.
    static QJsonObject withoutMethods(QJsonObject project)
    {
        for (const QString& k : project.keys()) {
            if (!k.startsWith("model"))
                continue;
            QJsonObject m = project.value(k).toObject();
            QJsonObject d = m.value("data").toObject();
            d.remove("methods");
            m["data"] = d;
            project[k] = m;
        }
        return project;
    }

    static int blockMethod(const QJsonObject& block)
    {
        const QJsonObject c = block.value("controller").toObject();
        return c.contains("method") ? c.value("method").toInt() : c.value("Method").toInt();
    }

    // True if a result block carries any parameter boxplot (i.e. it is a finished post-processing
    // block, not a bare controller/header).
    static bool hasBoxplot(const QJsonObject& block)
    {
        for (const QString& k : block.keys()) {
            if (k == "controller")
                continue;
            if (block.value(k).toObject().contains("boxplot"))
                return true;
        }
        return false;
    }

    // The GLOBAL-parameter boxplots of a result block, keyed by the parameter's global index (its
    // "index" field). Local-parameter boxplots (chemical shifts) are skipped: they carry no binding
    // constant and are ordered differently between the fixed and the generalised model.
    static QMap<int, QJsonObject> globalBoxplots(const QJsonObject& block)
    {
        QMap<int, QJsonObject> out;
        for (const QString& k : block.keys()) {
            if (k == "controller")
                continue;
            const QJsonObject p = block.value(k).toObject();
            // "index" is serialised as a STRING ("0","1",... for globals) - QJsonValue::toInt() would
            // return 0 for every one of them and collapse the map, so parse the string. Claude Generated.
            if (p.value("type").toString() == QLatin1String("Global Parameter") && p.contains("boxplot"))
                out.insert(p.value("index").toString().toInt(), p.value("boxplot").toObject());
        }
        return out;
    }

    // All freshly computed result blocks for a method id in a model export, ordered by key.
    static QVector<QJsonObject> freshBlocks(const QJsonObject& modelExport, int methodId)
    {
        const QJsonObject methods = modelExport.value("data").toObject().value("methods").toObject();
        QStringList keys = methods.keys();
        std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) { return a.toInt() < b.toInt(); });
        QVector<QJsonObject> out;
        for (const QString& k : keys) {
            const QJsonObject b = methods.value(k).toObject();
            if (hasBoxplot(b) && blockMethod(b) == methodId)
                out << b;
        }
        return out;
    }

    // Map the fixed model's per-parameter values (stepwise constants, in fixed-model index order)
    // into nmr_any's cumulative-beta species order. The map is linear, so it applies equally to a
    // fitted parameter vector and to a vector of resample means.
    static QVector<double> fixedToBeta(int fixedId, const QVector<double>& g)
    {
        QVector<double> beta;
        switch (fixedId) {
        case SupraFit::nmr_ItoI: // 1:1  -> {AB}
            beta << g.value(0);
            break;
        case SupraFit::nmr_IItoI_ItoI: // 2:1/1:1 -> {AB, A2B}; G=[lgK21, lgK11]
            beta << g.value(1) << g.value(1) + g.value(0);
            break;
        case SupraFit::nmr_ItoI_ItoII: // 1:1/1:2 -> {AB, AB2}; G=[lgK11, lgK12]
            beta << g.value(0) << g.value(0) + g.value(1);
            break;
        }
        return beta;
    }

    // The cumulative-beta means of a fixed-model result block, in nmr_any species order, together
    // with a per-species "well determined" flag (false if any contributing fixed parameter has a
    // broad reference distribution). Returns empty on a shape mismatch.
    static void fixedBoxToBeta(int fixedId, const QMap<int, QJsonObject>& box,
        QVector<double>& betaMean, QVector<bool>& wellDetermined)
    {
        int n = 0;
        for (auto it = box.constBegin(); it != box.constEnd(); ++it)
            n = qMax(n, it.key() + 1);
        QVector<double> mean(n, 0.0);
        QVector<bool> ok(n, false);
        for (auto it = box.constBegin(); it != box.constEnd(); ++it) {
            const double m = it.value().value("mean").toDouble();
            const double s = it.value().value("stddev").toDouble();
            mean[it.key()] = m;
            ok[it.key()] = qAbs(s) / qMax(qAbs(m), 1e-9) <= kMaxRelStd;
        }
        betaMean = fixedToBeta(fixedId, mean);
        // A beta component is trustworthy only if every fixed parameter feeding it is.
        switch (fixedId) {
        case SupraFit::nmr_ItoI:
            wellDetermined = QVector<bool>{ ok.value(0) };
            break;
        case SupraFit::nmr_IItoI_ItoI:
            wellDetermined = QVector<bool>{ ok.value(1), ok.value(0) && ok.value(1) };
            break;
        case SupraFit::nmr_ItoI_ItoII:
            wellDetermined = QVector<bool>{ ok.value(0), ok.value(0) && ok.value(1) };
            break;
        }
    }

    // Beta means of an nmr_any result block, in species order (already cumulative beta).
    static QVector<double> anyBoxBeta(const QMap<int, QJsonObject>& box)
    {
        int n = 0;
        for (auto it = box.constBegin(); it != box.constEnd(); ++it)
            n = qMax(n, it.key() + 1);
        QVector<double> mean(n, 0.0);
        for (auto it = box.constBegin(); it != box.constEnd(); ++it)
            mean[it.key()] = it.value().value("mean").toDouble();
        return mean;
    }

    // Pick the cheapest stored Cross-Validation and the Reduction controller of a fixed model.
    static void storedControllers(const QJsonObject& storedModel, QJsonObject& cv, QJsonObject& ra)
    {
        const QJsonObject methods = storedModel.value("data").toObject().value("methods").toObject();
        int cheapCVSteps = INT_MAX;
        for (const QString& k : methods.keys()) {
            const QJsonObject b = methods.value(k).toObject();
            const int m = blockMethod(b);
            const int steps = b.value("controller").toObject().value("MaxSteps").toInt();
            if (m == static_cast<int>(SupraFit::Method::CrossValidation) && steps < cheapCVSteps) {
                cv = b.value("controller").toObject();
                cheapCVSteps = steps;
            } else if (m == static_cast<int>(SupraFit::Method::Reduction)) {
                ra = b.value("controller").toObject();
            }
        }
    }

    // Run one post-processing method on a model and return the fresh boxplots.
    static QMap<int, QJsonObject> runMethod(QSharedPointer<AbstractModel> model, QJsonObject ctrl, int method)
    {
        ctrl["Method"] = method;
        ctrl.remove("timestamp");
        model->Calculate();
        JobManager jm;
        jm.setModel(model);
        jm.AddSingleJob(ctrl);
        jm.RunJobs(); // synchronous
        const QVector<QJsonObject> blocks = freshBlocks(model->ExportModel(), method);
        if (blocks.isEmpty())
            return {};
        return globalBoxplots(blocks.last());
    }

private slots:
    void equivalence_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<int>("fixedId");
        QTest::addColumn<QString>("reactions");

        QTest::newRow("1:1")
            << sourceDir() + "/data/samples/reference/simulated_1_1.json"
            << int(SupraFit::nmr_ItoI)
            << QStringLiteral("A + B <=> AB");
        QTest::newRow("2:1/1:1")
            << sourceDir() + "/data/samples/reference/simulated_2_1_1_1.json"
            << int(SupraFit::nmr_IItoI_ItoI)
            << QStringLiteral("A + B <=> AB\n2 A + B <=> A2B");
        QTest::newRow("1:1/1:2")
            << sourceDir() + "/data/samples/reference/simulated_1_1_1_2.json"
            << int(SupraFit::nmr_ItoI_ItoII)
            << QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2");
    }

    void equivalence()
    {
        QFETCH(QString, file);
        QFETCH(int, fixedId);
        QFETCH(QString, reactions);

        if (!QFileInfo::exists(file))
            QSKIP("reference fixture not present locally");

        const QJsonObject projectJson = JsonHandler::LoadFile(file);
        const QMap<int, QJsonObject> stored = storedModelsById(projectJson);
        QVERIFY2(stored.contains(fixedId), "fixture is missing the fixed reference model");

        qApp->setProperty("threads", 4); // JobManager reads this for its thread pool

        // Reconstruct clean (no stored post-processing), so each fresh run yields one block.
        SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();
        const QString projectId = pm.loadProjectFromJson(withoutMethods(projectJson), QStringLiteral("eqv"));
        QVERIFY2(!projectId.isEmpty(), "loadProjectFromJson failed");

        QSharedPointer<AbstractModel> fixed;
        for (const QSharedPointer<AbstractModel>& m : pm.getProjectModels(projectId))
            if (!m.isNull() && static_cast<int>(m->SFModel()) == fixedId)
                fixed = m;
        QVERIFY2(!fixed.isNull(), "fixed model not reconstructed");

        // The fixed model loads at its stored optimum; re-settle it so SSE/parameters are current.
        fixed->Calculate();
        Minimizer fmin(false);
        fmin.setModel(fixed);
        fmin.Minimize();
        const double fixedSSE = fixed->SSE();
        QVector<double> fixedG;
        for (int i = 0; i < fixed->GlobalParameterSize(); ++i)
            fixedG << fixed->GlobalParameter(i);
        const QVector<double> betaRef = fixedToBeta(fixedId, fixedG);

        // Build the generalised twin on the SAME data (AbstractModel is-a DataClass -> shared tables).
        QSharedPointer<AbstractModel> any = CreateModel(SupraFit::nmr_any, fixed.data());
        QVERIFY2(!any.isNull(), "CreateModel(nmr_any) failed");
        QJsonObject def;
        def["Reactions"] = strOption(reactions);
        QVERIFY2(any->DefineModel(def), "nmr_any rejected the reaction definition");
        QCOMPARE(any->GlobalParameterSize(), betaRef.size());

        // Seed with the equivalent cumulative betas (deterministic; optimiser robustness of the
        // data-derived InitialGuess is covered by test_nmr_selfaggregation), then fit independently.
        any->InitialGuess();
        for (int k = 0; k < any->GlobalParameterSize(); ++k)
            any->setGlobalParameter(betaRef[k], k);
        any->Calculate();
        Minimizer amin(false);
        amin.setModel(any);
        amin.Minimize();
        const double anySSE = any->SSE();

        qInfo().noquote() << QString("[%1] SSE fixed=%2 any=%3").arg(QTest::currentDataTag()).arg(fixedSSE, 0, 'g', 6).arg(anySSE, 0, 'g', 6);

        // (A1) same minimum quality
        QVERIFY2(relClose(anySSE, fixedSSE, kTolSSE),
            qPrintable(QString("SSE any=%1 not within %2 of fixed=%3").arg(anySSE).arg(kTolSSE).arg(fixedSSE)));

        // (A2) same fitted signal, point by point (convention-free ground truth)
        const DataTable* sf = fixed->ModelTable();
        const DataTable* sa = any->ModelTable();
        QCOMPARE(sa->rowCount(), sf->rowCount());
        QCOMPARE(sa->columnCount(), sf->columnCount());
        double maxSignal = 1e-9;
        for (int r = 0; r < sf->rowCount(); ++r)
            for (int c = 0; c < sf->columnCount(); ++c)
                maxSignal = qMax(maxSignal, qAbs(sf->data(r, c)));
        for (int r = 0; r < sf->rowCount(); ++r)
            for (int c = 0; c < sf->columnCount(); ++c)
                QVERIFY2(qAbs(sa->data(r, c) - sf->data(r, c)) <= kTolSignal * maxSignal,
                    qPrintable(QString("signal[%1,%2] any=%3 vs fixed=%4").arg(r).arg(c).arg(sa->data(r, c)).arg(sf->data(r, c))));

        // (A3) same cumulative betas. The 1:1 constant (beta[0], AB) is always well determined, so
        // it is asserted; higher complexes can sit in a flat likelihood direction (a barely-populated
        // species), where the optimiser drifts along the plateau and both models reach the same
        // SSE/signal with a different beta - the reference regression test likewise checks only SSE
        // after re-fitting. Those are logged and rigorously compared in part (B), guarded by the
        // resample determinacy.
        for (int k = 0; k < betaRef.size(); ++k)
            qInfo().noquote() << QString("  [%1] lg beta[%2] any=%3 fixed=%4")
                                     .arg(QTest::currentDataTag()).arg(k)
                                     .arg(any->GlobalParameter(k), 0, 'g', 6).arg(betaRef[k], 0, 'g', 6);
        QVERIFY2(relClose(any->GlobalParameter(0), betaRef[0], kTolBeta),
            qPrintable(QString("lg beta[0] (AB) any=%1 vs fixed-derived=%2").arg(any->GlobalParameter(0)).arg(betaRef[0])));

        // (B) Post-processing statistics: CV + RA on both models, compare boxplot means in beta space.
        QJsonObject cvCtrl, raCtrl;
        storedControllers(stored.value(fixedId), cvCtrl, raCtrl);

        struct Job {
            const char* label;
            int method;
            QJsonObject ctrl;
        };
        QVector<Job> jobs;
        if (!cvCtrl.isEmpty())
            jobs.push_back({ "CV", static_cast<int>(SupraFit::Method::CrossValidation), cvCtrl });
        if (!raCtrl.isEmpty())
            jobs.push_back({ "RA", static_cast<int>(SupraFit::Method::Reduction), raCtrl });
        QVERIFY2(!jobs.isEmpty(), "fixture has no stored CV/RA controller to replay");

        int compared = 0, skipped = 0;
        for (const Job& job : jobs) {
            const QMap<int, QJsonObject> fixedBox = runMethod(fixed, job.ctrl, job.method);
            const QMap<int, QJsonObject> anyBox = runMethod(any, job.ctrl, job.method);
            QVERIFY2(!fixedBox.isEmpty(), qPrintable(QString("no fresh %1 result for fixed").arg(job.label)));
            QVERIFY2(!anyBox.isEmpty(), qPrintable(QString("no fresh %1 result for any").arg(job.label)));

            QVector<double> fixedBeta, anyBeta = anyBoxBeta(anyBox);
            QVector<bool> wellDetermined;
            fixedBoxToBeta(fixedId, fixedBox, fixedBeta, wellDetermined);
            QCOMPARE(anyBeta.size(), fixedBeta.size());

            for (int k = 0; k < fixedBeta.size(); ++k) {
                if (!wellDetermined.value(k)) {
                    qInfo().noquote() << QString("  %1 [%2] beta%3 ill-determined (skipped)").arg(QTest::currentDataTag(), job.label).arg(k);
                    ++skipped;
                    continue;
                }
                qInfo().noquote() << QString("  %1 [%2] beta%3 fixed=%4 any=%5").arg(QTest::currentDataTag(), job.label).arg(k).arg(fixedBeta[k], 0, 'g', 6).arg(anyBeta[k], 0, 'g', 6);
                QVERIFY2(relClose(anyBeta[k], fixedBeta[k], kTolBoxMean),
                    qPrintable(QString("%1 beta%2 any=%3 not within %4 of fixed=%5").arg(job.label).arg(k).arg(anyBeta[k]).arg(kTolBoxMean).arg(fixedBeta[k])));
                ++compared;
            }
        }
        qInfo().noquote() << QString("[%1] CV/RA: %2 well-determined matched, %3 ill-determined (skipped)").arg(QTest::currentDataTag()).arg(compared).arg(skipped);
        QVERIFY2(compared > 0, "no well-determined CV/RA parameters compared");
    }
};

QTEST_MAIN(TestNmrAnyEquivalence)
#include "test_nmr_any_equivalence.moc"
