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
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "src/core/jsonhandler.h"
#include "src/core/projectmanager.h"

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

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

private slots:
    void referenceFit_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<QString>("projectKey"); // empty => whole file is one project

        struct Fx {
            const char* path;
        };
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
        QVERIFY2(rows > 0, "no reference fixtures found");
    }

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

            // (2) Optimiser stability: re-fit from the loaded optimum; must stay there.
            model->OptimizeParameters();
            const double sseFit = model->SSE();
            for (int i = 0; i < refGlobals.size(); ++i) {
                const double got = model->GlobalParameter(i);
                QVERIFY2(relClose(got, refGlobals[i], kTolGlobal),
                    qPrintable(QString("[%1] re-fit lgK[%2] %3 not within %4 of %5")
                            .arg(model->Name()).arg(i).arg(got).arg(kTolGlobal).arg(refGlobals[i])));
            }
            QVERIFY2(relClose(sseFit, storedSSE, kTolSSE),
                qPrintable(QString("[%1] re-fit SSE %2 not within %3 of %4")
                        .arg(model->Name()).arg(sseFit).arg(kTolSSE).arg(storedSSE)));
        }
    }
};

QTEST_MAIN(TestReferenceProjects)
#include "test_reference_projects.moc"
