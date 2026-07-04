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
 * Real, measuring regression tests: load a trusted `.suprafit` project (fitted with
 * a stable SupraFit version = external oracle), reconstruct each model with the CURRENT
 * code, re-fit it, and assert the recovered stability constants (lg K) and SSE reproduce
 * the stored reference values. Unlike the smoke tests (which only check exit codes and
 * output substrings), these compare actual numbers against an independent ground truth.
 *
 * Fixture: data/samples/NMR titrations/projects.suprafit (4 projects, fit-only, from
 * stable commit 8af629a). Post-processing (MC/CV/RA) references are added once fixtures
 * carrying those results are available.
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

    // Parse a SupraFit globalParameter block ("data":{"0":"2.8957 4.6"}) into doubles.
    static QVector<double> storedGlobals(const QJsonObject& modelJson)
    {
        QVector<double> out;
        const QJsonObject gp = modelJson.value("data").toObject().value("globalParameter").toObject();
        const QString row = gp.value("data").toObject().value("0").toString();
        const QStringList parts = row.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        for (const QString& p : parts)
            out << p.toDouble();
        return out;
    }

    static bool relClose(double actual, double expected, double tol)
    {
        const double denom = qMax(qAbs(expected), 1e-12);
        return qAbs(actual - expected) / denom <= tol;
    }

    QString fixturePath() const
    {
        return QString(SUPRAFIT_SOURCE_DIR) + "/data/samples/NMR titrations/projects.suprafit";
    }

private slots:
    void referenceFitReproduction_data()
    {
        QTest::addColumn<QString>("projectKey");

        const QString path = fixturePath();
        QVERIFY2(QFileInfo::exists(path), qPrintable("fixture missing: " + path));
        const QJsonObject root = JsonHandler::LoadFile(path);
        QVERIFY2(!root.isEmpty(), "fixture did not decode to a JSON project");

        int projects = 0;
        for (const QString& key : root.keys()) {
            if (key.startsWith("project")) {
                QTest::newRow(qPrintable(key)) << key;
                ++projects;
            }
        }
        QVERIFY2(projects > 0, "no project_* entries in fixture");
    }

    void referenceFitReproduction()
    {
        QFETCH(QString, projectKey);

        const QJsonObject root = JsonHandler::LoadFile(fixturePath());
        const QJsonObject projectJson = root.value(projectKey).toObject();

        // Locate the (single) stored model in this project for the reference values.
        QString modelKey;
        for (const QString& k : projectJson.keys())
            if (k.startsWith("model"))
                modelKey = k;
        QVERIFY2(!modelKey.isEmpty(), "project has no stored model");
        const QJsonObject storedModel = projectJson.value(modelKey).toObject();
        const double storedSSE = storedModel.value("SSE").toDouble();
        const QVector<double> refGlobals = storedGlobals(storedModel);
        QVERIFY2(!refGlobals.isEmpty(), "no stored global parameters");

        // Reconstruct with the current code via the production ProjectManager path.
        SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();
        const QString projectId = pm.loadProjectFromJson(projectJson, projectKey);
        QVERIFY2(!projectId.isEmpty(), "loadProjectFromJson failed");
        const QVector<QSharedPointer<AbstractModel>> models = pm.getProjectModels(projectId);
        QCOMPARE(models.size(), 1);
        QSharedPointer<AbstractModel> model = models.first();
        QVERIFY(!model.isNull());
        QCOMPARE(model->GlobalParameterSize(), refGlobals.size());

        // (1) Forward-model reproduction (deterministic): recompute from the stored
        // parameters on the stored data — the SSE must reproduce what the stable version
        // recorded, and the stored parameters must round-trip through the accessors.
        model->Calculate();
        const double sseLoaded = model->SSE();
        qInfo().noquote() << QString("%1 [%2]: as-loaded SSE ref=%3 got=%4")
                                 .arg(projectKey, model->Name())
                                 .arg(storedSSE, 0, 'g', 6)
                                 .arg(sseLoaded, 0, 'g', 6);
        for (int i = 0; i < refGlobals.size(); ++i) {
            const double got = model->GlobalParameter(i);
            qInfo().noquote() << QString("    lgK[%1] ref=%2 loaded=%3")
                                     .arg(i).arg(refGlobals[i], 0, 'g', 6).arg(got, 0, 'g', 6);
            QVERIFY2(relClose(got, refGlobals[i], kTolLoaded),
                qPrintable(QString("stored lgK[%1] %2 did not round-trip to %3")
                        .arg(i).arg(got).arg(refGlobals[i])));
        }
        QVERIFY2(relClose(sseLoaded, storedSSE, kTolLoaded),
            qPrintable(QString("recomputed SSE %1 not within %2 of stored %3")
                    .arg(sseLoaded).arg(kTolLoaded).arg(storedSSE)));

        // (2) Optimizer stability: re-fit starting from the loaded optimum; the current
        // optimizer must stay at the trusted minimum (parameters + SSE reproduce).
        model->OptimizeParameters();
        const double sseFit = model->SSE();
        qInfo().noquote() << QString("    re-fit SSE=%1").arg(sseFit, 0, 'g', 6);
        for (int i = 0; i < refGlobals.size(); ++i) {
            const double got = model->GlobalParameter(i);
            qInfo().noquote() << QString("    re-fit lgK[%1] ref=%2 got=%3")
                                     .arg(i).arg(refGlobals[i], 0, 'g', 6).arg(got, 0, 'g', 6);
            QVERIFY2(relClose(got, refGlobals[i], kTolGlobal),
                qPrintable(QString("re-fit lgK[%1] %2 not within %3 of %4")
                        .arg(i).arg(got).arg(kTolGlobal).arg(refGlobals[i])));
        }
        QVERIFY2(relClose(sseFit, storedSSE, kTolSSE),
            qPrintable(QString("re-fit SSE %1 not within %2 of %3").arg(sseFit).arg(kTolSSE).arg(storedSSE)));
    }
};

QTEST_MAIN(TestReferenceProjects)
#include "test_reference_projects.moc"
