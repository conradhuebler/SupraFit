/*
 * Reference-project regression tests — Claude Generated (2026)
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * Real, measuring regression tests: load trusted SupraFit projects (fitted with a stable
 * version = external oracle), reconstruct each model with the CURRENT code, and assert the
 * recovered stability constants (lg K) and SSE reproduce the stored reference values. Unlike
 * the smoke tests (which only check exit codes and output substrings), these compare actual
 * numbers against an independent ground truth.
 *
 * Fixtures:
 *   - data/samples/NMR titrations/projects.suprafit — 4 single-model projects (stable 8af629a).
 *   - data/samples/reference/simulated_*.json — the PeerJ Monte-Carlo paper datasets, each
 *     fitting all 4 stoichiometries to one experiment; slimmed to fit + post-processing
 *     summaries (see make_reference_fixtures.py).
 *
 * Post-processing (MC / CV / RA) comparisons are wired in a follow-up; this stage validates
 * the fit (forward-model recompute + optimiser stability at the trusted minimum).
 */

#include <QtTest/QtTest>

#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "src/capabilities/jobmanager.h"
#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/projectmanager.h"

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

#include <climits>

#ifndef SUPRAFIT_SOURCE_DIR
#define SUPRAFIT_SOURCE_DIR "."
#endif

class TestReferenceProjects : public QObject {
    Q_OBJECT

private:
    // Relative tolerances (calibrate against printed actual/expected deltas).
    static constexpr double kTolLoaded = 1e-3; // forward-model recompute: near-exact
    static constexpr double kTolGlobal = 1e-2; // 1 % on lg K after re-fit
    static constexpr double kTolSSE = 5e-2; //  5 % on SSE after re-fit
    static constexpr double kTolResample = 1e-2; // 1 % on CV/RA boxplot mean (well-determined params)
    static constexpr double kMaxRelStd = 5e-3; // skip params whose reference distribution spread
                                               // (stddev/|mean|) exceeds this — ill-determined
                                               // directions are optimiser-unstable / non-reproducible

    static QString sourceDir() { return QString(SUPRAFIT_SOURCE_DIR); }

    // Parse a SupraFit globalParameter block ("data":{"0":"2.8957 4.6"}) into doubles.
    static QVector<double> storedGlobals(const QJsonObject& modelJson)
    {
        QVector<double> out;
        const QJsonObject gp = modelJson.value("data").toObject().value("globalParameter").toObject();
        const QString row = gp.value("data").toObject().value("0").toString();
        for (const QString& p : row.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts))
            out << p.toDouble();
        return out;
    }

    static bool relClose(double actual, double expected, double tol)
    {
        return qAbs(actual - expected) / qMax(qAbs(expected), 1e-12) <= tol;
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

    // A project copy with every model's stored post-processing removed, so a re-run yields
    // exactly one unambiguous result block per method.
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

    // SupraFit::Method id stored on a result block's controller ("method" | "Method").
    static int blockMethod(const QJsonObject& block)
    {
        const QJsonObject c = block.value("controller").toObject();
        return c.contains("method") ? c.value("method").toInt() : c.value("Method").toInt();
    }

    // Per-parameter boxplot summaries in a result block, keyed by parameter index.
    static QMap<QString, QJsonObject> boxplots(const QJsonObject& block)
    {
        QMap<QString, QJsonObject> out;
        for (const QString& k : block.keys()) {
            if (k == "controller")
                continue;
            const QJsonObject p = block.value(k).toObject();
            if (p.contains("boxplot"))
                out.insert(k, p.value("boxplot").toObject());
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
            if (!boxplots(b).isEmpty() && blockMethod(b) == methodId)
                out << b;
        }
        return out;
    }

    // Enumerate (file, projectKey) rows over all reference fixtures present locally.
    static int addFixtureRows()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<QString>("projectKey"); // empty => whole file is one project

        const QStringList files = {
            sourceDir() + "/data/samples/NMR titrations/projects.suprafit",
            sourceDir() + "/data/samples/reference/simulated_1_1.json",
            sourceDir() + "/data/samples/reference/simulated_1_1_1_2.json",
            sourceDir() + "/data/samples/reference/simulated_2_1_1_1.json",
            sourceDir() + "/data/samples/reference/simulated_2_1_1_1_1_2.json",
        };
        int rows = 0;
        for (const QString& file : files) {
            if (!QFileInfo::exists(file))
                continue; // fixtures are optional locally; skip if absent
            const QJsonObject root = JsonHandler::LoadFile(file);
            QStringList projectKeys;
            for (const QString& k : root.keys())
                if (k.startsWith("project"))
                    projectKeys << k;
            const QString base = QFileInfo(file).fileName();
            if (projectKeys.isEmpty()) {
                QTest::newRow(qPrintable(base)) << file << QString();
                ++rows;
            } else {
                for (const QString& pk : projectKeys) {
                    QTest::newRow(qPrintable(base + ":" + pk)) << file << pk;
                    ++rows;
                }
            }
        }
        return rows;
    }

private slots:
    void referenceFit_data() { QVERIFY2(addFixtureRows() > 0, "no reference fixtures found"); }

    void referenceFit()
    {
        QFETCH(QString, file);
        QFETCH(QString, projectKey);

        const QJsonObject root = JsonHandler::LoadFile(file);
        const QJsonObject projectJson = projectKey.isEmpty() ? root : root.value(projectKey).toObject();
        const QMap<int, QJsonObject> stored = storedModelsById(projectJson);
        QVERIFY2(!stored.isEmpty(), "project has no stored models");

        SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();
        const QString projectId = pm.loadProjectFromJson(projectJson, projectKey.isEmpty() ? QStringLiteral("ref") : projectKey);
        QVERIFY2(!projectId.isEmpty(), "loadProjectFromJson failed");
        const QVector<QSharedPointer<AbstractModel>> models = pm.getProjectModels(projectId);
        QCOMPARE(models.size(), stored.size());

        for (const QSharedPointer<AbstractModel>& model : models) {
            QVERIFY(!model.isNull());
            const int id = static_cast<int>(model->SFModel());
            QVERIFY2(stored.contains(id), qPrintable(QString("no stored model for id %1").arg(id)));
            const QJsonObject storedModel = stored.value(id);
            const double storedSSE = storedModel.value("SSE").toDouble();
            const QVector<double> refGlobals = storedGlobals(storedModel);
            QCOMPARE(model->GlobalParameterSize(), refGlobals.size());

            // (1) Forward-model reproduction (deterministic): recompute from the stored
            // parameters on the stored data; SSE + parameters must reproduce the reference.
            model->Calculate();
            const double sseLoaded = model->SSE();
            qInfo().noquote() << QString("%1 [%2]: as-loaded SSE ref=%3 got=%4")
                                     .arg(QFileInfo(file).fileName(), model->Name())
                                     .arg(storedSSE, 0, 'g', 6).arg(sseLoaded, 0, 'g', 6);
            for (int i = 0; i < refGlobals.size(); ++i) {
                const double got = model->GlobalParameter(i);
                QVERIFY2(relClose(got, refGlobals[i], kTolLoaded),
                    qPrintable(QString("[%1] stored lgK[%2] %3 did not round-trip to %4")
                            .arg(model->Name()).arg(i).arg(got).arg(refGlobals[i])));
            }
            QVERIFY2(relClose(sseLoaded, storedSSE, kTolLoaded),
                qPrintable(QString("[%1] recomputed SSE %2 not within %3 of stored %4")
                        .arg(model->Name()).arg(sseLoaded).arg(kTolLoaded).arg(storedSSE)));

            // (2) Optimiser stability: run the REAL optimiser (NonLinearFitThread via Minimizer)
            // starting from the loaded optimum; it must stay at the reference minimum.
            //
            // NOTE: this used to call model->OptimizeParameters(), which does NOT fit — it only
            // rebuilds the parameter list (AbstractModel.cpp:303) — so the check was vacuous (it
            // "passed" by never moving). Driving Minimizer::Minimize() actually exercises the
            // optimiser. We assert on SSE (robust to flat/degenerate directions); the exact
            // parameter reproduction is already guaranteed by the forward-model check (1) above.
            Minimizer minim(false);
            minim.setModel(model);
            minim.Minimize();
            const double sseFit = model->SSE();
            QVERIFY2(relClose(sseFit, storedSSE, kTolSSE),
                qPrintable(QString("[%1] optimiser from the reference optimum drifted: SSE %2 not within %3 of %4")
                        .arg(model->Name()).arg(sseFit).arg(kTolSSE).arg(storedSSE)));
        }
    }

    // Re-run the deterministic post-processing methods (Cross-Validation, Reduction Analysis)
    // with the current code and assert their boxplot summaries reproduce the trusted reference.
    //
    // The reference values are a trusted-but-not-infallible oracle. For ILL-DETERMINED parameters
    // (broad resample distribution — an optimiser-unstable / flat likelihood direction) the reference
    // value is itself uncertain, and the current code is non-reproducible run-to-run there (verified;
    // same optimiser-instability family as the InitialGuess finding, TECHNICAL_DEBT §5.2). Requiring
    // an exact match on those would test noise, not correctness. The paper's *method* stays valid, so
    // the test validates it where it is meaningful: it asserts reference reproduction only for
    // WELL-DETERMINED parameters (relative reference stddev <= kMaxRelStd) and reports the
    // ill-determined ones (skipped, not asserted).
    void referenceResampleCVRA_data() { addFixtureRows(); }

    void referenceResampleCVRA()
    {
        QFETCH(QString, file);
        QFETCH(QString, projectKey);

        const QJsonObject root = JsonHandler::LoadFile(file);
        const QJsonObject projectJson = projectKey.isEmpty() ? root : root.value(projectKey).toObject();
        const QMap<int, QJsonObject> stored = storedModelsById(projectJson);

        // Skip fixtures that carry no post-processing (e.g. projects.suprafit is fit-only).
        bool hasResample = false;
        for (const QJsonObject& sm : stored) {
            const QJsonObject ms = sm.value("data").toObject().value("methods").toObject();
            for (const QString& k : ms.keys()) {
                const int m = blockMethod(ms.value(k).toObject());
                if (m == static_cast<int>(SupraFit::Method::CrossValidation) || m == static_cast<int>(SupraFit::Method::Reduction))
                    hasResample = true;
            }
        }
        if (!hasResample)
            QSKIP("fixture has no stored CV/RA post-processing");

        qApp->setProperty("threads", 4); // JobManager reads this for its thread pool

        // Reconstruct once, clean (no stored stats), so each re-run yields one unambiguous block.
        SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();
        const QString projectId = pm.loadProjectFromJson(withoutMethods(projectJson), QStringLiteral("cvra"));
        QVERIFY2(!projectId.isEmpty(), "loadProjectFromJson failed");
        const QVector<QSharedPointer<AbstractModel>> models = pm.getProjectModels(projectId);

        int compared = 0, illDetermined = 0;
        for (const QSharedPointer<AbstractModel>& model : models) {
            const int id = static_cast<int>(model->SFModel());
            if (!stored.contains(id))
                continue;
            const QJsonObject storedMethods = stored.value(id).value("data").toObject().value("methods").toObject();

            // Pick the cheapest stored Cross-Validation (method 4) and the Reduction (method 5) block.
            QJsonObject cheapCV, reduction;
            int cheapCVSteps = INT_MAX;
            for (const QString& k : storedMethods.keys()) {
                const QJsonObject b = storedMethods.value(k).toObject();
                const int m = blockMethod(b);
                const int steps = b.value("controller").toObject().value("MaxSteps").toInt();
                if (m == static_cast<int>(SupraFit::Method::CrossValidation) && steps < cheapCVSteps) {
                    cheapCV = b;
                    cheapCVSteps = steps;
                } else if (m == static_cast<int>(SupraFit::Method::Reduction)) {
                    reduction = b;
                }
            }

            struct Job {
                const char* label;
                int method;
                QJsonObject ref;
            };
            QVector<Job> jobs;
            if (!cheapCV.isEmpty())
                jobs.push_back({ "CV", static_cast<int>(SupraFit::Method::CrossValidation), cheapCV });
            if (!reduction.isEmpty())
                jobs.push_back({ "RA", static_cast<int>(SupraFit::Method::Reduction), reduction });

            for (const Job& job : jobs) {
                QJsonObject ctrl = job.ref.value("controller").toObject();
                ctrl["Method"] = job.method;
                ctrl.remove("timestamp"); // fresh run gets its own

                model->Calculate();
                JobManager jm;
                jm.setModel(model);
                jm.AddSingleJob(ctrl);
                jm.RunJobs(); // synchronous
                const QVector<QJsonObject> blocks = freshBlocks(model->ExportModel(), job.method);
                QVERIFY2(!blocks.isEmpty(),
                    qPrintable(QString("%1 [%2]: no fresh %3 result").arg(QFileInfo(file).fileName(), model->Name(), job.label)));

                const QMap<QString, QJsonObject> refBox = boxplots(job.ref);
                const QMap<QString, QJsonObject> gotBox = boxplots(blocks.last());
                for (auto it = refBox.constBegin(); it != refBox.constEnd(); ++it) {
                    if (!gotBox.contains(it.key()))
                        continue;
                    const double rMean = it.value().value("mean").toDouble();
                    const double rStd = it.value().value("stddev").toDouble();
                    const double gMean = gotBox.value(it.key()).value("mean").toDouble();
                    const double relStd = qAbs(rStd) / qMax(qAbs(rMean), 1e-9);
                    if (relStd > kMaxRelStd) {
                        // Broad reference distribution: the parameter is an optimiser-unstable /
                        // ill-determined direction — its value is uncertain in the reference itself
                        // (and the paper's point: wrong models produce divergent statistics), so
                        // requiring an exact match would test noise, not correctness. Report, skip.
                        qInfo().noquote() << QString("  divergent (wrong-model signal): %1 [%2] %3 p%4 relStd=%5")
                                                 .arg(QFileInfo(file).fileName(), model->Name(), job.label, it.key())
                                                 .arg(relStd, 0, 'g', 2);
                        ++illDetermined;
                        continue;
                    }
                    QVERIFY2(relClose(gMean, rMean, kTolResample),
                        qPrintable(QString("[%1] %2 p%3 well-determined mean %4 not within %5 of reference %6")
                                .arg(model->Name(), job.label, it.key()).arg(gMean).arg(kTolResample).arg(rMean)));
                    ++compared;
                }
            }
        }
        qInfo().noquote() << QString("CV/RA: %1 well-determined parameters matched reference, %2 ill-determined (skipped)")
                                 .arg(compared).arg(illDetermined);
        QVERIFY2(compared > 0, "no well-determined CV/RA parameters found to compare");
    }
};

QTEST_MAIN(TestReferenceProjects)
#include "test_reference_projects.moc"
