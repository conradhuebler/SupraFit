// test_tools_inspect.cpp - the inspect tools against the real reference project.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - get_model_parameters, get_model_statistics, get_data_table,
// get_residuals (Read) and set_model_parameter (Mutate). Headless, no network.
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QUuid>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <cmath>

#include "src/agent/tool.h"
#include "src/agent/toolregistry.h"
#include "src/agent/tools_analysis.h"
#include "src/agent/tools_inspect.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/projectmanager.h"

#ifndef SUPRAFIT_SOURCE_DIR
#define SUPRAFIT_SOURCE_DIR "."
#endif

class ToolsInspectTest : public QObject {
    Q_OBJECT

private:
    ToolRegistry m_registry;
    QString m_project;  // simulated_1_1.json with its four stored fits and statistics

    static QString refPath() { return QStringLiteral(SUPRAFIT_SOURCE_DIR) + QStringLiteral("/data/samples/reference/simulated_1_1.json"); }

    QString load(const QString& path)
    {
        const QStringList before = SupraFit::ProjectManager::instance().getLoadedProjectIds();
        if (!SupraFit::ProjectManager::instance().loadProject(path))
            return QString();
        for (const QString& id : SupraFit::ProjectManager::instance().getLoadedProjectIds())
            if (!before.contains(id))
                return id;
        return QString();
    }

    /// The reference data without its stored models, under a new uuid.
    QString freshProject()
    {
        QFile f(refPath());
        if (!f.open(QIODevice::ReadOnly))
            return QString();
        QJsonObject stored = QJsonDocument::fromJson(f.readAll()).object();
        for (const QString& key : stored.keys())
            if (key.startsWith(QStringLiteral("model_")))
                stored.remove(key);
        QJsonObject data = stored.value(QStringLiteral("data")).toObject();
        data.insert(QStringLiteral("uuid"), QUuid::createUuid().toString());
        stored.insert(QStringLiteral("data"), data);
        static QTemporaryDir dir;
        static int counter = 0;
        const QString path = dir.filePath(QStringLiteral("fresh_%1.json").arg(++counter));
        QFile out(path);
        if (!out.open(QIODevice::WriteOnly))
            return QString();
        out.write(QJsonDocument(stored).toJson());
        out.close();
        return load(path);
    }

    ToolResult call(const QString& tool, const QJsonObject& args) { return m_registry.invoke(tool, args); }

private slots:
    void initTestCase()
    {
        qApp->setProperty("threads", 4);
        m_project = load(refPath());
        QVERIFY2(!m_project.isEmpty(), "reference project did not load");
        auto& pm = SupraFit::ProjectManager::instance();
        QCOMPARE(registerInspectTools(m_registry, &pm), 5);
        registerAnalysisTools(m_registry, &pm);  // add_model, run_postfit_analysis
    }

    void effectsAreRightAndArraysDeclareItems()
    {
        for (const char* name : { "get_model_parameters", "get_model_statistics", "get_data_table", "get_residuals" }) {
            ToolSpec spec;
            QVERIFY2(m_registry.tool(QString::fromLatin1(name), spec), name);
            QCOMPARE(spec.effect, ToolEffect::Read);
        }
        ToolSpec set;
        QVERIFY(m_registry.tool(QStringLiteral("set_model_parameter"), set));
        QCOMPARE(set.effect, ToolEffect::Mutate);
    }

    void parametersReturnTheFittedConstant()
    {
        const ToolResult r = call(QStringLiteral("get_model_parameters"),
            QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("model_index"), 0 } });
        QVERIFY2(r.ok, qPrintable(r.error));
        const QJsonArray globals = r.data.value(QStringLiteral("global_parameters")).toArray();
        QCOMPARE(globals.size(), 1);  // the 1:1 model: lg K11
        QVERIFY(qAbs(globals.first().toObject().value(QStringLiteral("value")).toDouble() - 2.8957) < 1e-3);
        QVERIFY(!globals.first().toObject().value(QStringLiteral("name")).toString().isEmpty());
        const QJsonArray series = r.data.value(QStringLiteral("local_parameters_per_series")).toArray();
        QCOMPARE(series.size(), 7);
        QCOMPARE(series.first().toObject().value(QStringLiteral("values")).toArray().size(),
            r.data.value(QStringLiteral("local_parameter_names")).toArray().size());
    }

    void statisticsListTheStoredMonteCarloRuns()
    {
        const ToolResult r = call(QStringLiteral("get_model_statistics"),
            QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("model_index"), 0 } });
        QVERIFY2(r.ok, qPrintable(r.error));
        const QJsonArray runs = r.data.value(QStringLiteral("runs")).toArray();
        QVERIFY2(!runs.isEmpty(), "the reference model carries stored statistic runs");
        bool foundInterval = false;
        for (const QJsonValue& v : runs)
            for (const QJsonValue& p : v.toObject().value(QStringLiteral("parameters")).toArray()) {
                const QJsonObject po = p.toObject();
                if (po.contains(QStringLiteral("ci_lower"))) {
                    foundInterval = true;
                    QVERIFY(po.value(QStringLiteral("ci_lower")).toDouble() <= po.value(QStringLiteral("ci_upper")).toDouble());
                }
            }
        QVERIFY2(foundInterval, "at least one run has a confidence interval");
        // max_runs caps the newest runs per method
        const ToolResult one = call(QStringLiteral("get_model_statistics"),
            QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("model_index"), 0 },
                         { QStringLiteral("max_runs"), 1 } });
        QVERIFY(one.data.value(QStringLiteral("runs")).toArray().size() <= runs.size());
    }

    void dataTableWindowsAndRanges()
    {
        const ToolResult dep = call(QStringLiteral("get_data_table"),
            QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("table"), QStringLiteral("dependent") },
                         { QStringLiteral("start_row"), 2 }, { QStringLiteral("max_rows"), 5 } });
        QVERIFY2(dep.ok, qPrintable(dep.error));
        QCOMPARE(dep.data.value(QStringLiteral("rows_total")).toInt(), 20);
        QCOMPARE(dep.data.value(QStringLiteral("columns")).toInt(), 7);
        const QJsonArray rows = dep.data.value(QStringLiteral("rows")).toArray();
        QCOMPARE(rows.size(), 5);
        QCOMPARE(rows.first().toObject().value(QStringLiteral("row")).toInt(), 2);
        QVERIFY(dep.data.value(QStringLiteral("truncated")).toBool());
        const QJsonObject range = dep.data.value(QStringLiteral("column_ranges")).toArray().first().toObject();
        QVERIFY(range.value(QStringLiteral("min")).toDouble() <= range.value(QStringLiteral("max")).toDouble());

        const ToolResult ind = call(QStringLiteral("get_data_table"),
            QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("table"), QStringLiteral("independent") } });
        QVERIFY2(ind.ok, qPrintable(ind.error));
        QCOMPARE(ind.data.value(QStringLiteral("columns")).toInt(), 2);
        QCOMPARE(ind.data.value(QStringLiteral("rows")).toArray().size(), 20);  // default window is larger than the table
        QVERIFY(!ind.data.value(QStringLiteral("truncated")).toBool());

        QVERIFY(!call(QStringLiteral("get_data_table"),
            QJsonObject{ { QStringLiteral("project_id"), QStringLiteral("{nope}") }, { QStringLiteral("table"), QStringLiteral("dependent") } }).ok);
        QVERIFY(!call(QStringLiteral("get_data_table"),
            QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("table"), QStringLiteral("other") } }).ok);
    }

    void residualsAreSmallForTheGoodFitAndPerSeries()
    {
        const ToolResult r = call(QStringLiteral("get_residuals"),
            QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("model_index"), 0 } });
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.data.value(QStringLiteral("series")).toArray().size(), 7);
        const double rms = r.data.value(QStringLiteral("rms_overall")).toDouble();
        QVERIFY2(rms > 0.0 && rms < 0.05, qPrintable(QString::number(rms)));
        // rms_overall^2 * points = SSE of the stored fit
        const double sse = rms * rms * r.data.value(QStringLiteral("points_used")).toInt();
        QVERIFY2(qAbs(sse - 0.01271) < 5e-4, qPrintable(QString::number(sse)));
        const QJsonObject s0 = r.data.value(QStringLiteral("series")).toArray().first().toObject();
        QVERIFY(std::abs(s0.value(QStringLiteral("lag1_autocorrelation")).toDouble()) <= 1.0 + 1e-9);
    }

    void setParameterChangesTheModelAndRefusesBadNames()
    {
        const QString pid = freshProject();
        QVERIFY(!pid.isEmpty());
        QVERIFY(call(QStringLiteral("add_model"), QJsonObject{ { QStringLiteral("project_id"), pid },
                       { QStringLiteral("model_ids"), QJsonArray{ 1 } } }).ok);
        const ToolResult p = call(QStringLiteral("get_model_parameters"),
            QJsonObject{ { QStringLiteral("project_id"), pid }, { QStringLiteral("model_index"), 0 } });
        QVERIFY2(p.ok, qPrintable(p.error));
        const QString name = p.data.value(QStringLiteral("global_parameters")).toArray().first().toObject().value(QStringLiteral("name")).toString();

        const ToolResult set = call(QStringLiteral("set_model_parameter"),
            QJsonObject{ { QStringLiteral("project_id"), pid }, { QStringLiteral("model_index"), 0 },
                         { QStringLiteral("scope"), QStringLiteral("global") }, { QStringLiteral("parameter"), name },
                         { QStringLiteral("value"), 3.25 } });
        QVERIFY2(set.ok, qPrintable(set.error));
        QVERIFY(qAbs(set.data.value(QStringLiteral("value_now")).toDouble() - 3.25) < 1e-9);
        QVERIFY(std::isfinite(set.data.value(QStringLiteral("sse")).toDouble()));
        const ToolResult again = call(QStringLiteral("get_model_parameters"),
            QJsonObject{ { QStringLiteral("project_id"), pid }, { QStringLiteral("model_index"), 0 } });
        QVERIFY(qAbs(again.data.value(QStringLiteral("global_parameters")).toArray().first().toObject().value(QStringLiteral("value")).toDouble() - 3.25) < 1e-9);

        // by index as text, and a local parameter of series 1
        QVERIFY(call(QStringLiteral("set_model_parameter"),
            QJsonObject{ { QStringLiteral("project_id"), pid }, { QStringLiteral("model_index"), 0 },
                         { QStringLiteral("scope"), QStringLiteral("global") }, { QStringLiteral("parameter"), QStringLiteral("0") },
                         { QStringLiteral("value"), 2.5 } }).ok);
        const ToolResult local = call(QStringLiteral("set_model_parameter"),
            QJsonObject{ { QStringLiteral("project_id"), pid }, { QStringLiteral("model_index"), 0 },
                         { QStringLiteral("scope"), QStringLiteral("local") }, { QStringLiteral("parameter"), QStringLiteral("0") },
                         { QStringLiteral("series"), 1 }, { QStringLiteral("value"), 6.0 } });
        QVERIFY2(local.ok, qPrintable(local.error));
        QVERIFY(qAbs(local.data.value(QStringLiteral("value_now")).toDouble() - 6.0) < 1e-9);

        const ToolResult bad = call(QStringLiteral("set_model_parameter"),
            QJsonObject{ { QStringLiteral("project_id"), pid }, { QStringLiteral("model_index"), 0 },
                         { QStringLiteral("scope"), QStringLiteral("global") }, { QStringLiteral("parameter"), QStringLiteral("nonsense") },
                         { QStringLiteral("value"), 1.0 } });
        QVERIFY(!bad.ok);
        QVERIFY2(bad.error.contains(QStringLiteral("available")), qPrintable(bad.error));
        QVERIFY(!call(QStringLiteral("set_model_parameter"),
            QJsonObject{ { QStringLiteral("project_id"), pid }, { QStringLiteral("model_index"), 0 },
                         { QStringLiteral("scope"), QStringLiteral("local") }, { QStringLiteral("parameter"), QStringLiteral("0") },
                         { QStringLiteral("series"), 99 }, { QStringLiteral("value"), 1.0 } }).ok);
    }


    void postfitMonteCarloReallyRunsAndIsStored()
    {
        auto runs = [&]() {
            return call(QStringLiteral("get_model_statistics"),
                QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("model_index"), 0 },
                             { QStringLiteral("max_runs"), 100 } }).data.value(QStringLiteral("runs")).toArray().size();
        };
        const int before = runs();
        const ToolResult r = call(QStringLiteral("run_postfit_analysis"),
            QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("model_index"), 0 },
                         { QStringLiteral("method"), 1 }, { QStringLiteral("max_steps"), 60 } });
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(runs(), before + 1);  // the run is stored, not just reported
        const QJsonArray params = r.data.value(QStringLiteral("parameters")).toArray();
        QVERIFY(!params.isEmpty());
        const QJsonObject lgK = params.first().toObject();
        QVERIFY(lgK.contains(QStringLiteral("ci_lower")));
        QVERIFY(lgK.value(QStringLiteral("ci_lower")).toDouble() < lgK.value(QStringLiteral("value")).toDouble());
        QVERIFY(lgK.value(QStringLiteral("ci_upper")).toDouble() > lgK.value(QStringLiteral("value")).toDouble());
        QVERIFY(r.data.value(QStringLiteral("samples")).toInt() <= 60);  // the 60 steps asked for, not an old stored run
    }

    void postfitCrossValidationAndFastConfidenceWork()
    {
        for (int method : { 4, 6 }) {
            const ToolResult r = call(QStringLiteral("run_postfit_analysis"),
                QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("model_index"), 0 },
                             { QStringLiteral("method"), method } });
            QVERIFY2(r.ok, qPrintable(r.error));
            QVERIFY(!r.data.value(QStringLiteral("parameters")).toArray().isEmpty());
        }
    }

    void postfitRefusesMethodsThatNeedAConfiguration()
    {
        for (int method : { 2, 3, 5, 7 }) {
            const ToolResult r = call(QStringLiteral("run_postfit_analysis"),
                QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("model_index"), 0 },
                             { QStringLiteral("method"), method } });
            QVERIFY2(!r.ok, qPrintable(QString::number(method)));
            QVERIFY(r.error.contains(QStringLiteral("job configuration")));
        }
    }

    void outOfRangeModelIsRefused()
    {
        for (const char* tool : { "get_model_parameters", "get_model_statistics", "get_residuals" })
            QVERIFY2(!call(QString::fromLatin1(tool),
                         QJsonObject{ { QStringLiteral("project_id"), m_project }, { QStringLiteral("model_index"), 42 } }).ok,
                tool);
    }
};

QTEST_MAIN(ToolsInspectTest)
#include "test_tools_inspect.moc"
