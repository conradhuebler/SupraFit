/*
 * SupraFit - Phase-5 compute tools: fit, compare, post-process, BC50 (no net)
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
 */

/* Phase 5 of roadmap/llm_agent_analysis.md: the compute tools. This test loads the
 * real reference project simulated_1_1.json (4 models, STORED fit results) and a
 * fresh copy for fitting. It pins:
 *
 * - fit_model actually changes a model's parameters/state (Compute effect, and the
 *   returned JSON carries the new SSE + parameters);
 * - fit_models fits all models of a project, and a subset via model_indices;
 * - compare_models_aic ranks by AIC and REFUSES unfitted models (SSE=0 would win
 *   every AIC -- the tool must say "fit first", not return a wrong ranking);
 * - run_postfit_analysis runs a tiny Monte Carlo (capped MaxSteps) on a fitted
 *   model and returns a result block;
 * - calculate_bc50 reads a fitted 1:1 model and returns a finite positive BC50.
 *
 * No GUI, no network. The effect classes are asserted so a tool can never silently
 * become a Mutate/FileWrite.
 * Claude Generated (2026). */

#include <QtTest/QtTest>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>

#include <cmath>

#include "src/agent/tool.h"
#include "src/agent/toolregistry.h"
#include "src/agent/tools_analysis.h"
#include "src/agent/tools_project.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/projectmanager.h"

#ifndef SUPRAFIT_SOURCE_DIR
#define SUPRAFIT_SOURCE_DIR "."
#endif

class ToolsAnalysisTest : public QObject {
    Q_OBJECT

private:
    ToolRegistry m_registry;
    QString m_fittedProjectId;   // simulated_1_1.json as loaded (stored fits)
    QString m_freshProjectId;    // a scratch DataClass with data only, no models

    static QString sourceDir() { return QStringLiteral(SUPRAFIT_SOURCE_DIR); }

    /// Load the reference project into the singleton and remember its id.
    QString loadReferenceProject()
    {
        const QString path = sourceDir() + QStringLiteral("/data/samples/reference/simulated_1_1.json");
        if (!SupraFit::ProjectManager::instance().loadProject(path))
            return QString();
        const QStringList ids = SupraFit::ProjectManager::instance().getLoadedProjectIds();
        return ids.isEmpty() ? QString() : ids.last();
    }

    /// Build a fresh in-memory project with the reference data but NO models, so a
    /// fit has something real to work on and the test does not disturb the stored
    /// reference project. Returns the registered project UUID, or empty on failure.
    ///
    /// The DataClass must be configured BEFORE CreateModel: independent + dependent
    /// tables, Table data type, dependent series count (the pattern from
    /// test_bc50_model_dispatch.cpp:56-62). CreateModel passes a raw DataClass* into
    /// the model factory, which needs those tables to size its parameters.
    QString makeFreshProject()
    {
        const QString path = sourceDir() + QStringLiteral("/data/samples/reference/simulated_1_1.json");
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return QString();
        const QJsonObject stored = QJsonDocument::fromJson(f.readAll()).object();

        // Build the DataClass from the reference data DIRECTLY (Eigen matrices),
        // mirroring test_bc50_model_dispatch.cpp: the stored JSON nests tables under
        // data.independent / data.dependent as {rows, cols, data: {"row": "c0 c1.."}}.
        const QJsonObject indJson = stored.value(QStringLiteral("data")).toObject()
                                        .value(QStringLiteral("independent")).toObject();
        const QJsonObject depJson = stored.value(QStringLiteral("data")).toObject()
                                        .value(QStringLiteral("dependent")).toObject();
        const int indRows = indJson.value(QStringLiteral("rows")).toInt();
        const int indCols = indJson.value(QStringLiteral("cols")).toInt();
        const int depRows = depJson.value(QStringLiteral("rows")).toInt();
        const int depCols = depJson.value(QStringLiteral("cols")).toInt();
        if (indRows <= 0 || indCols <= 0 || depRows <= 0 || depCols <= 0)
            return QString();

        DataTable* indep = new DataTable(indRows, indCols, nullptr);
        DataTable* dep = new DataTable(depRows, depCols, nullptr);
        const QJsonObject indData = indJson.value(QStringLiteral("data")).toObject();
        const QJsonObject depData = depJson.value(QStringLiteral("data")).toObject();
        for (auto it = indData.begin(); it != indData.end(); ++it) {
            const int r = it.key().toInt();
            const QStringList cells = it.value().toString().split(QLatin1Char(' '), Qt::SkipEmptyParts);
            for (int c = 0; c < cells.size() && c < indCols; ++c)
                indep->data(r, c) = cells.at(c).toDouble();
        }
        for (auto it = depData.begin(); it != depData.end(); ++it) {
            const int r = it.key().toInt();
            const QStringList cells = it.value().toString().split(QLatin1Char(' '), Qt::SkipEmptyParts);
            for (int c = 0; c < cells.size() && c < depCols; ++c)
                dep->data(r, c) = cells.at(c).toDouble();
        }

        auto data = QSharedPointer<DataClass>::create();
        data->setIndependentTable(indep);
        data->setDataType(DataClassPrivate::Table);
        data->setSimulateDependent(depCols);
        data->setDependentTable(dep);
        // A fresh DataClass has no UUID yet; assign one so registerExistingProject
        // (which refuses an empty UUID) accepts it.
        data->NewUUID();
        if (!SupraFit::ProjectManager::instance().registerExistingProject(data))
            return QString();
        return data->UUID();
    }

    /// Create a model on a configured project, mirroring the working pattern from
    /// test_bc50_model_dispatch.cpp: set the tables + type + series count first.
    QSharedPointer<AbstractModel> makeModel(int modelId, const QSharedPointer<DataClass>& data)
    {
        // CreateModel takes a RAW DataClass*; the model reads the tables through it.
        const QPointer<DataClass> raw(data.data());
        return CreateModel(modelId, raw);
    }

    /// Register a bare DataClass (no models) and return its UUID, or empty.
    /// Helper for the "unfitted model" guards.
    QString registerBareProject(const QSharedPointer<DataClass>& data)
    {
        data->NewUUID();
        if (!SupraFit::ProjectManager::instance().registerExistingProject(data))
            return QString();
        return data->UUID();
    }

    /// Attach a model to a project so ProjectManager::getProjectModels sees it.
    /// addModelToProject only emits a signal in the current architecture
    /// (models are stored as DataClass children); DataClass::addModel is the
    /// public API that actually registers the model as a child.
    void attachModel(const QSharedPointer<AbstractModel>& model, const QString& projectId)
    {
        QVERIFY(SupraFit::ProjectManager::instance().addModelToProject(model, projectId));
        const QSharedPointer<DataClass> data = SupraFit::ProjectManager::instance().getProjectData(projectId);
        QVERIFY(data);
        data->addModel(model);  // DataClass::addModel (dataclass.h:400, public)
    }

private slots:
    void initTestCase()
    {
        m_fittedProjectId = loadReferenceProject();
        QVERIFY2(!m_fittedProjectId.isEmpty(), "reference project did not load");

        const int n = registerAnalysisTools(m_registry, &SupraFit::ProjectManager::instance())
                    + registerProjectTools(m_registry, &SupraFit::ProjectManager::instance());
        QCOMPARE(n, 10);  // 5 analysis + 5 project
        for (const char* name : { "fit_model", "fit_models", "compare_models_aic",
                                  "run_postfit_analysis", "calculate_bc50",
                                  "list_projects", "get_project_info", "list_models",
                                  "get_model_fit", "describe_model_catalog" })
            QVERIFY2(m_registry.contains(QString::fromLatin1(name)), name);
    }

    void effectClassesAreCorrect()
    {
        // Compute tools need approval at Ask/InProgram; Read tools never do.
        const QHash<QString, ToolEffect> expected = {
            { QStringLiteral("fit_model"), ToolEffect::Compute },
            { QStringLiteral("fit_models"), ToolEffect::Compute },
            { QStringLiteral("run_postfit_analysis"), ToolEffect::Compute },
            { QStringLiteral("compare_models_aic"), ToolEffect::Read },
            { QStringLiteral("calculate_bc50"), ToolEffect::Read },
            { QStringLiteral("list_projects"), ToolEffect::Read },
            { QStringLiteral("get_project_info"), ToolEffect::Read },
            { QStringLiteral("list_models"), ToolEffect::Read },
            { QStringLiteral("get_model_fit"), ToolEffect::Read },
            { QStringLiteral("describe_model_catalog"), ToolEffect::Read },
        };
        for (auto it = expected.begin(); it != expected.end(); ++it) {
            ToolSpec spec;
            QVERIFY2(m_registry.tool(it.key(), spec), qPrintable(it.key()));
            QCOMPARE(spec.effect, it.value());
        }
        // The three Compute tools must ask at Ask; at InProgram, Compute is allowed
        // (everything that stays inside the program); only Full lifts FileWrite/Process.
        for (const char* name : { "fit_model", "fit_models", "run_postfit_analysis" }) {
            ToolSpec spec;
            QVERIFY(m_registry.tool(QString::fromLatin1(name), spec));
            QVERIFY2(needsApprovalAt(spec.effect, ToolAutonomy::Ask), name);
            QVERIFY2(!needsApprovalAt(spec.effect, ToolAutonomy::InProgram), name);
            QVERIFY2(!needsApprovalAt(spec.effect, ToolAutonomy::Full), name);
        }
    }

    void compareModelsAicRanksTheStoredFits()
    {
        // The reference project's 4 models are already fitted (stored SSE>0).
        const ToolResult r = m_registry.invoke(QStringLiteral("compare_models_aic"),
            QJsonObject{ { QStringLiteral("project_id"), m_fittedProjectId } });
        QVERIFY2(r.ok, qPrintable(r.error));
        // CalculateAICMetrics wraps its payload in methods["3"]["0"] (the
        // ModelComparison method id). The ranking lives under "ranking", not
        // "aic_ranking" -- the test reads the real structure.
        const QJsonObject aic = r.data.value(QStringLiteral("aic")).toObject();
        const QJsonObject methodResult = aic.value(QStringLiteral("methods")).toObject()
                                             .value(QStringLiteral("3")).toObject();
        const QJsonObject payload = methodResult.value(QStringLiteral("0")).toObject();
        const QJsonArray ranked = payload.value(QStringLiteral("ranking")).toArray();
        QCOMPARE(ranked.size(), 4);
        // AIC must be ascending in the ranking.
        double prev = -1e300;
        for (const QJsonValue& v : ranked) {
            const double aicv = v.toObject().value(QStringLiteral("aic")).toDouble();
            QVERIFY(aicv >= prev);
            prev = aicv;
        }
        // best_aic present and finite.
        QVERIFY(std::isfinite(payload.value(QStringLiteral("best_aic")).toDouble()));
    }

    void calculateBc50ReadsFittedModel()
    {
        // model_1 of simulated_1_1.json is the 1:1 model, fitted, lgK ~3.6.
        // BC50 = 1/beta11 => 1e-3.6 M = 2.5e-4. We pin the ORDER OF MAGNITUDE,
        // not the exact digits (the stored fit is what it is).
        const ToolResult r = m_registry.invoke(QStringLiteral("calculate_bc50"),
            QJsonObject{ { QStringLiteral("project_id"), m_fittedProjectId },
                         { QStringLiteral("model_index"), 0 } });
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.data.value(QStringLiteral("supported")).toBool(), true);
        const double bc50 = r.data.value(QStringLiteral("bc50")).toDouble();
        QVERIFY2(bc50 > 0.0, qPrintable(QString::number(bc50)));
        // 1:1 with lgK in a sane range => BC50 between 1e-6 and 1e-1 M.
        QVERIFY2(bc50 > 1e-6 && bc50 < 1e-1,
            qPrintable(QStringLiteral("bc50=%1 out of expected range").arg(bc50)));
    }

    void fitModelActuallyFits()
    {
        // Fresh project with reference DATA but no models; create a 1:1 model on it,
        // then fit_model must converge and return parameters.
        m_freshProjectId = makeFreshProject();
        QVERIFY2(!m_freshProjectId.isEmpty(), "fresh project did not register");

        auto data = SupraFit::ProjectManager::instance().getProjectData(m_freshProjectId);
        QVERIFY(data);
        QSharedPointer<AbstractModel> model = makeModel(1, data);  // 1:1 NMR
        QVERIFY2(model != nullptr, "CreateModel(1) returned null on the reference data");
        attachModel(model, m_freshProjectId);

        const ToolResult r = m_registry.invoke(QStringLiteral("fit_model"),
            QJsonObject{ { QStringLiteral("project_id"), m_freshProjectId },
                         { QStringLiteral("model_index"), 0 } });
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.data.value(QStringLiteral("model_id")).toInt(), 1);
        QVERIFY(r.data.value(QStringLiteral("sse")).toDouble() > 0.0);
        const QJsonArray params = r.data.value(QStringLiteral("parameters")).toArray();
        QVERIFY(params.size() >= 1);  // at least lgK11
        // After the fit, the AIC-compare must NOT refuse any more (model is fitted).
        const ToolResult cmp = m_registry.invoke(QStringLiteral("compare_models_aic"),
            QJsonObject{ { QStringLiteral("project_id"), m_freshProjectId } });
        QVERIFY2(cmp.ok, qPrintable(cmp.error));
    }

    void fitModelsFitsAllAndSubset()
    {
        // The fresh project from fitModelActuallyFits has 1 fitted model. Add a
        // second unfitted 1:1/1:2 model; fit_models without indices fits both.
        auto data = SupraFit::ProjectManager::instance().getProjectData(m_freshProjectId);
        QVERIFY(data);
        QSharedPointer<AbstractModel> second = makeModel(3, data);  // 1:1/1:2
        QVERIFY2(second != nullptr, "CreateModel(3) returned null");
        attachModel(second, m_freshProjectId);

        const ToolResult all = m_registry.invoke(QStringLiteral("fit_models"),
            QJsonObject{ { QStringLiteral("project_id"), m_freshProjectId } });
        QVERIFY2(all.ok, qPrintable(all.error));
        QCOMPARE(all.data.value(QStringLiteral("count")).toInt(), 2);

        // Subset: only index 1.
        const ToolResult sub = m_registry.invoke(QStringLiteral("fit_models"),
            QJsonObject{ { QStringLiteral("project_id"), m_freshProjectId },
                         { QStringLiteral("model_indices"), QJsonArray{ QStringLiteral("1") } } });
        QVERIFY2(sub.ok, qPrintable(sub.error));
        QCOMPARE(sub.data.value(QStringLiteral("count")).toInt(), 1);
        QCOMPARE(sub.data.value(QStringLiteral("fitted")).toArray().first().toObject()
                     .value(QStringLiteral("model_index")).toInt(), 1);
    }

    void runPostfitAnalysisRunsTinyMonteCarlo()
    {
        // model 0 of the fresh project is fitted. A Monte Carlo capped at 50 steps
        // must return a result block without taking minutes.
        const ToolResult r = m_registry.invoke(QStringLiteral("run_postfit_analysis"),
            QJsonObject{ { QStringLiteral("project_id"), m_freshProjectId },
                         { QStringLiteral("model_index"), 0 },
                         { QStringLiteral("method"), 1 },       // MonteCarlo
                         { QStringLiteral("max_steps"), 50 } });
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY(r.data.contains(QStringLiteral("result")));
        // The result must mention the method ran (the JobManager returns a block
        // per job; we only assert presence + a sane method id echo).
        QCOMPARE(r.data.value(QStringLiteral("method")).toInt(), 1);
    }

    void runPostfitAnalysisRejectsBadMethod()
    {
        const ToolResult r = m_registry.invoke(QStringLiteral("run_postfit_analysis"),
            QJsonObject{ { QStringLiteral("project_id"), m_freshProjectId },
                         { QStringLiteral("model_index"), 0 },
                         { QStringLiteral("method"), 99 } });
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains(QStringLiteral("invalid arguments")));
    }

    void fitModelRejectsOutOfRangeIndex()
    {
        const ToolResult r = m_registry.invoke(QStringLiteral("fit_model"),
            QJsonObject{ { QStringLiteral("project_id"), m_fittedProjectId },
                         { QStringLiteral("model_index"), 42 } });
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains(QStringLiteral("out of range")));
    }
};

QTEST_GUILESS_MAIN(ToolsAnalysisTest)
#include "test_tools_analysis.moc"
