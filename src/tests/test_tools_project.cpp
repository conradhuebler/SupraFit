/*
 * SupraFit - Phase-4 project read tools against a real reference project (no net)
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

/* Phase 4 of roadmap/llm_agent_analysis.md: the first tools that expose REAL
 * SupraFit functionality (ProjectManager) to the agent layer. This test loads a
 * real reference project (data/samples/reference/simulated_1_1.json -- 4 models,
 * known SSE/converged values) and drives all five Phase-4 tools headlessly,
 * without a GUI and without a network.
 *
 * The exact expected numbers come from the stored project JSON, not from a fresh
 * fit -- so the test pins the TOOL layer (does it read what ProjectManager holds?),
 * not the fitter (pinned by test_bc50_* / test_reference_projects).
 * Claude Generated (2026). */

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QSet>
#include <QtTest/QtTest>

#include "src/agent/tool.h"
#include "src/agent/toolregistry.h"
#include "src/agent/tools_project.h"
#include "src/core/projectmanager.h"

#ifndef SUPRAFIT_SOURCE_DIR
#define SUPRAFIT_SOURCE_DIR "."
#endif

class ToolsProjectTest : public QObject {
    Q_OBJECT

private:
    ToolRegistry m_registry;
    QString m_projectId;

    static QString sourceDir() { return QStringLiteral(SUPRAFIT_SOURCE_DIR); }

private slots:
    void initTestCase()
    {
        // A real project with 4 fitted models: the PeerJ simulated 1:1 dataset.
        const QString path = sourceDir() + QStringLiteral("/data/samples/reference/simulated_1_1.json");
        QVERIFY2(QFile::exists(path), qPrintable(QStringLiteral("reference project not found: %1").arg(path)));
        QVERIFY(SupraFit::ProjectManager::instance().loadProject(path));
        const QStringList ids = SupraFit::ProjectManager::instance().getLoadedProjectIds();
        QVERIFY(!ids.isEmpty());
        m_projectId = ids.first();

        const int n = registerProjectTools(m_registry, &SupraFit::ProjectManager::instance());
        QCOMPARE(n, 5);
        for (const char* name : { "list_projects", "get_project_info", "list_models",
                                  "get_model_fit", "describe_model_catalog" })
            QVERIFY2(m_registry.contains(QString::fromLatin1(name)), name);
    }

    void schemasAreWellFormed()
    {
        // Registration already validated them; this re-checks the honoured subset
        // so a future schema edit that adds an unsupported key fails here, loudly.
        for (const QString& name : m_registry.names()) {
            ToolSpec spec;
            QVERIFY(m_registry.tool(name, spec));
            QString error;
            QVERIFY2(ToolRegistry::isValidSchema(spec.paramSchema, &error),
                qPrintable(QStringLiteral("%1: %2").arg(name, error)));
            QVERIFY(spec.handler != nullptr);
        }
    }

    void listProjectsFindsTheLoadedProject()
    {
        const ToolResult r = m_registry.invoke(QStringLiteral("list_projects"), {});
        QVERIFY2(r.ok, qPrintable(r.error));
        const QJsonArray projects = r.data.value(QStringLiteral("projects")).toArray();
        QVERIFY(projects.size() >= 1);

        bool found = false;
        for (const QJsonValue& v : projects) {
            const QJsonObject p = v.toObject();
            if (p.value(QStringLiteral("id")).toString() == m_projectId) {
                found = true;
                QCOMPARE(p.value(QStringLiteral("model_count")).toInt(), 4);
                QVERIFY(p.contains(QStringLiteral("title")));
            }
        }
        QVERIFY(found);
        QCOMPARE(r.data.value(QStringLiteral("count")).toInt(), projects.size());
    }

    void getProjectInfoReportsDataAndModels()
    {
        const ToolResult r = m_registry.invoke(QStringLiteral("get_project_info"),
            QJsonObject{ { QStringLiteral("project_id"), m_projectId } });
        QVERIFY2(r.ok, qPrintable(r.error));

        // simulated_1_1.json: 20 rows, independent 2 cols, dependent 7 cols, 4 models.
        QCOMPARE(r.data.value(QStringLiteral("independent_rows")).toInt(), 20);
        QCOMPARE(r.data.value(QStringLiteral("independent_cols")).toInt(), 2);
        QCOMPARE(r.data.value(QStringLiteral("dependent_rows")).toInt(), 20);
        QCOMPARE(r.data.value(QStringLiteral("dependent_cols")).toInt(), 7);
        QCOMPARE(r.data.value(QStringLiteral("model_count")).toInt(), 4);

        const QJsonArray models = r.data.value(QStringLiteral("models")).toArray();
        QCOMPARE(models.size(), 4);
        // First model is the 1:1 model (id 1), converged, with its stored SSE.
        const QJsonObject first = models.first().toObject();
        QCOMPARE(first.value(QStringLiteral("model_id")).toInt(), 1);
        QCOMPARE(first.value(QStringLiteral("converged")).toBool(), true);
        QVERIFY(first.value(QStringLiteral("sse")).toDouble() > 0.0);
    }

    void getProjectInfoRejectsUnknownId()
    {
        const ToolResult r = m_registry.invoke(QStringLiteral("get_project_info"),
            QJsonObject{ { QStringLiteral("project_id"), QStringLiteral("no-such-uuid") } });
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains(QStringLiteral("no-such-uuid")));
    }

    void getProjectInfoRejectsMissingArg()
    {
        const ToolResult r = m_registry.invoke(QStringLiteral("get_project_info"), {});
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains(QStringLiteral("invalid arguments")));
    }

    void listModelsMatchesProjectInfo()
    {
        const ToolResult r = m_registry.invoke(QStringLiteral("list_models"),
            QJsonObject{ { QStringLiteral("project_id"), m_projectId } });
        QVERIFY2(r.ok, qPrintable(r.error));
        const QJsonArray models = r.data.value(QStringLiteral("models")).toArray();
        QCOMPARE(models.size(), 4);
        // The four known model ids of simulated_1_1.json, in stored order.
        const QList<int> expectedIds = { 1, 3, 2, 4 };
        for (int i = 0; i < models.size(); ++i)
            QCOMPARE(models.at(i).toObject().value(QStringLiteral("model_id")).toInt(),
                     expectedIds.at(i));
    }

    void getModelFitReturnsStoredStatistics()
    {
        const ToolResult r = m_registry.invoke(QStringLiteral("get_model_fit"),
            QJsonObject{ { QStringLiteral("project_id"), m_projectId },
                         { QStringLiteral("model_index"), 0 } });
        QVERIFY2(r.ok, qPrintable(r.error));
        // Reference values for model_1 (1:1) of simulated_1_1.json, read from the
        // stored project -- the test pins the tool reading, not the fitter.
        QCOMPARE(r.data.value(QStringLiteral("model_id")).toInt(), 1);
        QVERIFY(r.data.value(QStringLiteral("sse")).toDouble() > 0.0);
        QVERIFY(r.data.contains(QStringLiteral("aic")));
        QVERIFY(r.data.contains(QStringLiteral("aicc")));
        QVERIFY(r.data.contains(QStringLiteral("r_squared")));
        QCOMPARE(r.data.value(QStringLiteral("converged")).toBool(), true);
        // R^2 must be a sane number for a converged fit.
        const double r2 = r.data.value(QStringLiteral("r_squared")).toDouble();
        QVERIFY2(r2 > 0.9 && r2 <= 1.0, qPrintable(QString::number(r2)));
    }

    void getModelFitRejectsOutOfRangeIndex()
    {
        const ToolResult r = m_registry.invoke(QStringLiteral("get_model_fit"),
            QJsonObject{ { QStringLiteral("project_id"), m_projectId },
                         { QStringLiteral("model_index"), 99 } });
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains(QStringLiteral("out of range")));
    }

    void describeModelCatalogListsKnownModels()
    {
        const ToolResult r = m_registry.invoke(QStringLiteral("describe_model_catalog"), {});
        QVERIFY2(r.ok, qPrintable(r.error));
        const QJsonArray models = r.data.value(QStringLiteral("models")).toArray();
        QVERIFY(models.size() >= 20);

        // Spot-check the ids the sample project actually uses.
        QSet<int> ids;
        for (const QJsonValue& v : models)
            ids.insert(v.toObject().value(QStringLiteral("model_id")).toInt());
        for (int id : { 1, 2, 3, 4, 10, 34, 35 })
            QVERIFY2(ids.contains(id), qPrintable(QString::number(id)));
        // Every entry carries name + technique.
        for (const QJsonValue& v : models) {
            QVERIFY(v.toObject().contains(QStringLiteral("name")));
            QVERIFY(v.toObject().contains(QStringLiteral("technique")));
        }
    }

    void toolsAreReadEffect()
    {
        // All Phase-4 tools are Read: they must run unattended at any autonomy.
        for (const QString& name : m_registry.names()) {
            ToolSpec spec;
            QVERIFY(m_registry.tool(name, spec));
            QVERIFY2(!needsApprovalAt(spec.effect, ToolAutonomy::Ask),
                qPrintable(QStringLiteral("%1 is not Read").arg(name)));
        }
    }
};

QTEST_GUILESS_MAIN(ToolsProjectTest)
#include "test_tools_project.moc"
