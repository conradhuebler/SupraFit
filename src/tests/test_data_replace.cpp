/*
 * SupraFit - Edit Data replaces the data of a project: compatibility check, outdated models, undo
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

/*
 * DataClass::checkReplacement and ProjectManager::replaceProjectData / undoLastDataReplacement, GUI-free (Claude Generated).
 * Fixtures: the NMR reference project simulated_1_1.json (7 series, 20 points, 4 fitted models) and an ITC project made from
 * data/samples/legacy/itc_first_points.json (stored as a thermogram project with title, root directory and content).
 */

#include <QtTest/QtTest>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>

#include "src/capabilities/jobmanager.h"
#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/meta_model.h"
#include "src/core/models/models.h"
#include "src/core/projectmanager.h"

#include <fmt/format.h>

#ifndef SUPRAFIT_SOURCE_DIR
#define SUPRAFIT_SOURCE_DIR "."
#endif

using SupraFit::DataReplacePolicy;
using SupraFit::DataReplaceReport;
using SupraFit::LoadIssue;
using SupraFit::ProjectManager;

class TestDataReplace : public QObject {
    Q_OBJECT

    static QString sample(const QString& relative) { return QStringLiteral(SUPRAFIT_SOURCE_DIR) + QStringLiteral("/data/samples/") + relative; }

    /*! \brief A table block as DataTable::ExportTable writes it, from values (all cells checked). */
    static QJsonObject tableJson(const Eigen::MatrixXd& values, const QStringList& header, const Eigen::MatrixXd& checked = Eigen::MatrixXd())
    {
        DataTable table(int(values.rows()), int(values.cols()), nullptr);
        table.Table() = values;
        if (checked.rows() == values.rows() && checked.cols() == values.cols())
            table.setCheckedTable(checked);
        table.setHeader(header);
        return table.ExportTable(true);
    }

    /*! \brief The data block \p data with its last row removed from all four tables. */
    static QJsonObject dropLastRow(QJsonObject data)
    {
        for (const char* key : { "independent", "dependent", "independent_raw", "dependent_raw" }) {
            DataTable table;
            table.ImportTable(data[QLatin1String(key)].toObject());
            if (table.rowCount() < 2)
                continue; // a raw table may be empty
            data[QLatin1String(key)] = tableJson(table.Table().topRows(table.rowCount() - 1), table.header(), table.CheckedTable().topRows(table.rowCount() - 1));
        }
        data["begin_data"] = 0;
        data["end_data"] = 0; // the whole range of the new data
        return data;
    }

    /*! \brief The data block \p data with one more dependent series (a copy of the last one, shifted). */
    static QJsonObject addSeries(QJsonObject data)
    {
        for (const char* key : { "dependent", "dependent_raw" }) {
            DataTable table;
            table.ImportTable(data[QLatin1String(key)].toObject());
            if (table.columnCount() == 0)
                continue;
            Eigen::MatrixXd values(table.rowCount(), table.columnCount() + 1);
            values.leftCols(table.columnCount()) = table.Table();
            values.col(table.columnCount()) = table.Table().col(table.columnCount() - 1).array() + 0.1;
            data[QLatin1String(key)] = tableJson(values, table.header() << QStringLiteral("extra"));
        }
        return data;
    }

    /*! \brief ExportData() without the time stamp (every export has a new one). */
    static QByteArray dataBytes(DataClass* project)
    {
        QJsonObject data = project->ExportData();
        data.remove("timestamp");
        return QJsonDocument(data).toJson(QJsonDocument::Compact);
    }
    static QByteArray modelBytes(DataClass* project) { return QJsonDocument(project->ExportChildren()).toJson(QJsonDocument::Compact); }

    static QList<AbstractModel*> models(DataClass* project)
    {
        QList<AbstractModel*> list;
        for (int i = 0; i < project->ChildrenSize(); ++i)
            if (AbstractModel* m = qobject_cast<AbstractModel*>(project->Children(i).data()))
                list << m;
        return list;
    }

    static QSharedPointer<DataClass> loadNmr()
    {
        ProjectManager& pm = ProjectManager::instance();
        pm.clearAllProjects();
        if (!pm.loadProject(sample("reference/simulated_1_1.json")))
            return {};
        return pm.getProjectData(pm.lastLoadReport().projectId);
    }

    /*! \brief The ITC fixture: thermogram project with title, root directory, content and its 1:1 model fitted. */
    static QSharedPointer<DataClass> loadItc()
    {
        ProjectManager& pm = ProjectManager::instance();
        pm.clearAllProjects();
        QJsonObject file;
        if (!JsonHandler::ReadJsonFile(file, sample("legacy/itc_first_points.json")))
            return {};
        QJsonObject data = file["data"].toObject();
        data["DataType"] = 2;
        data["title"] = "ITC replacement";
        data["root_dir"] = "/measurements/itc";
        data["content"] = "operator notes";
        file["data"] = data;
        const QString id = pm.loadProjectFromJson(file, "itc_fixture");
        QSharedPointer<DataClass> project = pm.getProjectData(id);
        if (!project || models(project.data()).isEmpty())
            return {};
        QSharedPointer<AbstractModel> model = project->SharedModel(0);
        Minimizer minimizer(false);
        minimizer.setModel(model);
        minimizer.Minimize();
        return project;
    }

    static bool hasIssue(const SupraFit::LoadReport& r, LoadIssue::Severity severity, const QString& scope, const QString& part)
    {
        return std::any_of(r.issues.begin(), r.issues.end(), [&](const LoadIssue& i) { return i.severity == severity && i.scope == scope && i.message.contains(part); });
    }

private slots:
    void initTestCase()
    {
        const int t = QThread::idealThreadCount();
        qApp->setProperty("threads", t > 1 ? t : 2);
    }

    // (a) one point less: compatible, parameters kept bit for bit, SSE recalculated, outdated mark survives save and load
    void compatibleOnePointLess()
    {
        ProjectManager& pm = ProjectManager::instance();
        QSharedPointer<DataClass> project = loadNmr();
        QVERIFY(project);
        const QString id = project->UUID();
        const QList<AbstractModel*> list = models(project.data());
        QCOMPARE(list.size(), 4);
        QList<QJsonObject> globals, locals;
        for (AbstractModel* m : list) {
            QVERIFY(!m->isDataOutdated()); // (e) an old file without the key loads as current
            globals << m->GlobalTable()->ExportTable(true);
            locals << m->LocalTable()->ExportTable(true);
        }
        const QJsonObject newData = dropLastRow(project->ExportData());
        QCOMPARE(project->checkReplacement(newData).result, DataReplaceCheck::Compatible);

        const DataReplaceReport report = pm.replaceProjectData(id, newData);
        QVERIFY2(report.replaced, qPrintable(report.Details()));
        QCOMPARE(report.check.oldRows, 20);
        QCOMPARE(report.check.newRows, 19);
        QCOMPARE(project->DataPoints(), 19);
        QCOMPARE(project->UUID(), id);
        QCOMPARE(report.models.size(), 4);
        QVERIFY(pm.hasUnsavedData(id));
        QVERIFY(pm.canUndoDataReplacement(id));
        for (int i = 0; i < list.size(); ++i) {
            AbstractModel* m = list[i];
            QVERIFY(m->isDataOutdated());
            QVERIFY(m->isStatisticOutdated());
            QVERIFY(m->GlobalTable()->ExportTable(true) == globals[i]);
            QVERIFY(m->LocalTable()->ExportTable(true) == locals[i]);
            QCOMPARE(m->ModelTable()->rowCount(), 19);
            // the same parameters calculated directly on the new data
            QSharedPointer<DataClass> fresh(new DataClass(project->ExportData()));
            QSharedPointer<AbstractModel> reference = CreateModel(m->SFModel(), fresh.data());
            QJsonObject block = m->ExportModel();
            block.remove("SSE");
            QVERIFY(reference->ImportModel(block));
            reference->Calculate();
            QCOMPARE(m->SSE(), reference->SSE());
            QCOMPARE(report.models[i].sseAfter, m->SSE());
            fmt::print("  {}: SSE {:.10g} -> {:.10g}\n", report.models[i].key.toStdString(), report.models[i].sseBefore, report.models[i].sseAfter);
        }

        QTemporaryDir dir;
        const QString path = dir.filePath("replaced.json");
        QVERIFY(pm.saveProject(path, id));
        QVERIFY(!pm.hasUnsavedData(id));
        QJsonObject file;
        QVERIFY(JsonHandler::ReadJsonFile(file, path));
        QCOMPARE(file["data"].toObject()["SupraFit"].toInt(), qint_version); // (e) the file version is not raised
        QVERIFY(file["model_0"].toObject()["outdated"].toBool());

        pm.clearAllProjects();
        QVERIFY(pm.loadProject(path));
        const SupraFit::LoadReport& load = pm.lastLoadReport();
        QCOMPARE(load.modelsLoaded, 4);
        for (const LoadIssue& issue : load.issues)
            QVERIFY2(issue.severity != LoadIssue::Warning, qPrintable(issue.scope + ": " + issue.message));
        QVERIFY2(hasIssue(load, LoadIssue::Note, "model_0", "marked outdated"), qPrintable(load.Details()));
        QSharedPointer<DataClass> loaded = pm.getProjectData(load.projectId);
        QVERIFY(loaded);
        QCOMPARE(loaded->DataPoints(), 19);
        for (AbstractModel* m : models(loaded.data()))
            QVERIFY(m->isDataOutdated());
    }

    // (b) a fit after the replacement clears the mark, markCurrent() too
    void fitAndMarkCurrentClear()
    {
        ProjectManager& pm = ProjectManager::instance();
        QSharedPointer<DataClass> project = loadNmr();
        QVERIFY(project);
        QVERIFY(pm.replaceProjectData(project->UUID(), dropLastRow(project->ExportData())).replaced);
        const QList<AbstractModel*> list = models(project.data());
        Minimizer minimizer(false);
        minimizer.setModel(project->SharedModel(0));
        minimizer.Minimize();
        QVERIFY(!list[0]->isDataOutdated());
        QVERIFY(!list[0]->ExportModel().contains("outdated"));
        QVERIFY(list[1]->isDataOutdated());
        list[1]->markCurrent();
        QVERIFY(!list[1]->isDataOutdated());
        QVERIFY(list[2]->isDataOutdated());
    }

    // (c) one series more: incompatible, nothing changed; DropModels removes the models and keeps the identity of the project
    void incompatibleSeries()
    {
        ProjectManager& pm = ProjectManager::instance();
        QSharedPointer<DataClass> project = loadNmr();
        QVERIFY(project);
        const QString id = project->UUID();
        const QByteArray dataBefore = dataBytes(project.data()), modelsBefore = modelBytes(project.data());
        const QJsonObject newData = addSeries(project->ExportData());
        const DataReplaceCheck check = project->checkReplacement(newData);
        QCOMPARE(check.result, DataReplaceCheck::Incompatible);
        QCOMPARE(check.oldSeries, 7);
        QCOMPARE(check.newSeries, 8);
        QVERIFY(!check.reasons.isEmpty());

        const DataReplaceReport refused = pm.replaceProjectData(id, newData, DataReplacePolicy::KeepModels);
        QVERIFY(!refused.replaced);
        QVERIFY(refused.HasErrors());
        QVERIFY(dataBytes(project.data()) == dataBefore);
        QVERIFY(modelBytes(project.data()) == modelsBefore);
        QVERIFY(!pm.hasUnsavedData(id));
        QVERIFY(!pm.canUndoDataReplacement(id));

        const QString title = project->ProjectTitle(), root = project->RootDir();
        const QJsonObject system = project->getSystemObject();
        const DataReplaceReport dropped = pm.replaceProjectData(id, newData, DataReplacePolicy::DropModels);
        QVERIFY2(dropped.replaced, qPrintable(dropped.Details()));
        QCOMPARE(dropped.modelsRemoved, 4);
        QCOMPARE(project->ChildrenSize(), 0);
        QCOMPARE(project->SeriesCount(), 8);
        QCOMPARE(project->UUID(), id);
        QCOMPARE(project->ProjectTitle(), title);
        QCOMPARE(project->RootDir(), root);
        QVERIFY(project->getSystemObject() == system);

        // (g) undo makes the dropped models again, bit for bit
        QVERIFY(pm.undoLastDataReplacement(id));
        QVERIFY(dataBytes(project.data()) == dataBefore);
        QVERIFY(modelBytes(project.data()) == modelsBefore);
    }

    // (d) identical data change nothing
    void identicalData()
    {
        ProjectManager& pm = ProjectManager::instance();
        QSharedPointer<DataClass> project = loadNmr();
        QVERIFY(project);
        const QByteArray modelsBefore = modelBytes(project.data());
        const DataReplaceReport report = pm.replaceProjectData(project->UUID(), project->ExportData());
        QCOMPARE(report.check.result, DataReplaceCheck::Identical);
        QVERIFY(!report.replaced);
        QVERIFY(!report.HasErrors());
        QVERIFY(modelBytes(project.data()) == modelsBefore);
        QVERIFY(!pm.hasUnsavedData(project->UUID()));
        for (AbstractModel* m : models(project.data()))
            QVERIFY(!m->isDataOutdated());
    }

    // (f) a meta model with a model of the replaced project is marked outdated, its model of the other project is not
    void metaModelOutdated()
    {
        ProjectManager& pm = ProjectManager::instance();
        pm.clearAllProjects();
        QVERIFY(pm.loadProject(sample("reference/simulated_1_1.json")));
        const QString first = pm.lastLoadReport().projectId;
        QVERIFY(pm.loadProject(sample("reference/simulated_1_1_1_2.json")));
        const QString second = pm.lastLoadReport().projectId;
        QSharedPointer<DataClass> base = QSharedPointer<DataClass>::create();
        base->setProjectTitle("Meta");
        base->NewUUID();
        QSharedPointer<AbstractModel> created = CreateModel(SupraFit::MetaModel, base);
        ::MetaModel* meta = qobject_cast<::MetaModel*>(created.data());
        QVERIFY(meta);
        meta->addModel(qobject_cast<AbstractModel*>(pm.getProjectData(first)->Children(0).data()));
        meta->addModel(qobject_cast<AbstractModel*>(pm.getProjectData(second)->Children(0).data()));
        QVERIFY(pm.registerExistingProject(created.staticCast<DataClass>()));
        QVERIFY(!meta->isDataOutdated());

        QSharedPointer<DataClass> project = pm.getProjectData(first);
        QJsonObject data = project->ExportData();
        DataTable dependent;
        dependent.ImportTable(data["dependent"].toObject());
        Eigen::MatrixXd values = dependent.Table();
        values(3, 2) += 0.01; // one shift changed: same shape, other values
        data["dependent"] = tableJson(values, dependent.header());
        data["dependent_raw"] = data["dependent"];
        const DataReplaceReport report = pm.replaceProjectData(first, data);
        QVERIFY2(report.replaced, qPrintable(report.Details()));
        QCOMPARE(report.check.result, DataReplaceCheck::Compatible);
        QVERIFY(report.outdatedMetaModels.contains(meta->UUID()));
        QVERIFY(meta->isDataOutdated());
        QVERIFY(meta->Models()[0]->isDataOutdated());
        QVERIFY(!meta->Models()[1]->isDataOutdated());
        QVERIFY(pm.replaceProjectData(meta->UUID(), data).HasErrors()); // a meta model has no data of its own

        QVERIFY(pm.undoLastDataReplacement(first));
        QVERIFY(!meta->isDataOutdated());
        QVERIFY(!meta->Models()[0]->isDataOutdated());
        created.clear();
        pm.clearAllProjects();
    }

    // (g) undo restores data, models and marks bit for bit; a second undo does nothing
    void undoOnce()
    {
        ProjectManager& pm = ProjectManager::instance();
        QSharedPointer<DataClass> project = loadNmr();
        QVERIFY(project);
        const QString id = project->UUID();
        const QByteArray dataOriginal = dataBytes(project.data()), modelsOriginal = modelBytes(project.data());
        const QJsonObject once = dropLastRow(project->ExportData());
        QVERIFY(pm.replaceProjectData(id, once).replaced);
        const QByteArray dataOnce = dataBytes(project.data()), modelsOnce = modelBytes(project.data());
        QVERIFY(pm.replaceProjectData(id, dropLastRow(project->ExportData())).replaced);
        QCOMPARE(project->DataPoints(), 18);

        QVERIFY(pm.undoLastDataReplacement(id)); // back to the first replacement: outdated marks included
        QCOMPARE(project->DataPoints(), 19);
        QVERIFY(dataBytes(project.data()) == dataOnce);
        QVERIFY(modelBytes(project.data()) == modelsOnce);
        for (AbstractModel* m : models(project.data()))
            QVERIFY(m->isDataOutdated());
        QString reason;
        QVERIFY(!pm.undoLastDataReplacement(id, &reason));
        QVERIFY(!reason.isEmpty());
        QVERIFY(dataBytes(project.data()) == dataOnce);

        // and from the first replacement back to the file
        QVERIFY(pm.replaceProjectData(id, project->ExportData()).check.result == DataReplaceCheck::Identical);
        project = loadNmr();
        const QString id2 = project->UUID();
        QVERIFY(pm.replaceProjectData(id2, once).replaced);
        QVERIFY(pm.undoLastDataReplacement(id2));
        QVERIFY(dataBytes(project.data()) == dataOriginal);
        QVERIFY(modelBytes(project.data()) == modelsOriginal);
        QVERIFY(!pm.hasUnsavedData(id2));
    }

    // (h) a running job refuses the replacement
    void runningJobRefuses()
    {
        ProjectManager& pm = ProjectManager::instance();
        QSharedPointer<DataClass> project = loadNmr();
        QVERIFY(project);
        const QString id = project->UUID();
        const QByteArray dataBefore = dataBytes(project.data());
        const QJsonObject newData = dropLastRow(project->ExportData());
        QJsonObject job = MonteCarloConfigBlock;
        job["MaxSteps"] = 10;
        job["Method"] = static_cast<int>(SupraFit::Method::MonteCarlo);
        JobManager manager;
        manager.setModel(project->SharedModel(0));
        manager.AddSingleJob(job);
        DataReplaceReport during;
        bool running = false;
        connect(&manager, &JobManager::started, this, [&]() {
            running = pm.hasRunningJobs(id);
            during = pm.replaceProjectData(id, newData);
        });
        manager.RunJobs();
        QVERIFY(running);
        QVERIFY(!during.replaced);
        QVERIFY(during.HasErrors());
        QVERIFY(dataBytes(project.data()) == dataBefore);
        QVERIFY(!pm.hasRunningJobs(id));
        // the clones of the job are deleted later (deleteLater deleter) and share the data until then; the GUI returns to its event
        // loop first, the test has none (see TODO.md: clones of a finished job still follow the data)
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(pm.replaceProjectData(id, newData).replaced);
    }

    // (i) ITC: title, system parameters, root directory, content and UUID of the project stay; the data of the dialog do not
    // carry them
    void itcKeepsProjectIdentity()
    {
        ProjectManager& pm = ProjectManager::instance();
        QSharedPointer<DataClass> project = loadItc();
        QVERIFY(project);
        const QString id = project->UUID();
        AbstractModel* model = models(project.data()).first();
        QVERIFY(!model->isDataOutdated());
        const QJsonObject system = project->ExportData()["system"].toObject();
        QVERIFY(!system.isEmpty());
        const int rows = project->DataPoints();

        QJsonObject fromDialog = dropLastRow(project->ExportData());
        fromDialog["title"] = "Thermogram"; // what a new data set of the import dialog says
        fromDialog["root_dir"] = "";
        fromDialog["content"] = "";
        fromDialog["system"] = QJsonObject();
        fromDialog["uuid"] = "{00000000-0000-0000-0000-000000000000}";
        const QJsonObject global = model->GlobalTable()->ExportTable(true), local = model->LocalTable()->ExportTable(true);
        const DataReplaceReport report = pm.replaceProjectData(id, fromDialog);
        QVERIFY2(report.replaced, qPrintable(report.Details()));
        QCOMPARE(project->DataPoints(), rows - 1);
        QCOMPARE(project->UUID(), id);
        QCOMPARE(project->ProjectTitle(), QStringLiteral("ITC replacement"));
        QCOMPARE(project->RootDir(), QStringLiteral("/measurements/itc"));
        QCOMPARE(project->getContent(), QStringLiteral("operator notes"));
        if (project->ExportData()["system"].toObject() != system)
            fmt::print("  system before {}\n  system after  {}\n", QJsonDocument(system).toJson(QJsonDocument::Compact).toStdString(), QJsonDocument(project->ExportData()["system"].toObject()).toJson(QJsonDocument::Compact).toStdString());
        QVERIFY(project->ExportData()["system"].toObject() == system);
        QVERIFY(model->isDataOutdated());
        QVERIFY(model->GlobalTable()->ExportTable(true) == global);
        QVERIFY(model->LocalTable()->ExportTable(true) == local);
        QCOMPARE(model->ModelTable()->rowCount(), rows - 1);
        {
            QSharedPointer<DataClass> fresh(new DataClass(project->ExportData()));
            QSharedPointer<AbstractModel> reference = CreateModel(model->SFModel(), fresh.data());
            QJsonObject block = model->ExportModel();
            block.remove("SSE");
            QVERIFY(reference->ImportModel(block));
            reference->Calculate();
            QCOMPARE(model->SSE(), reference->SSE());
        }
        fmt::print("  ITC: SSE {:.10g} -> {:.10g}\n", report.models[0].sseBefore, report.models[0].sseAfter);

        // one injection more than before: the tables of the model grow with the data
        QJsonObject more = project->ExportData();
        DataTable independent, dependent;
        independent.ImportTable(QJsonObject(fromDialog)["independent"].toObject());
        QJsonObject original;
        QVERIFY(JsonHandler::ReadJsonFile(original, sample("legacy/itc_first_points.json")));
        more["independent"] = original["data"].toObject()["independent"];
        more["dependent"] = original["data"].toObject()["dependent"];
        more["independent_raw"] = more["independent"];
        more["dependent_raw"] = more["dependent"];
        more["end_data"] = 0;
        const DataReplaceReport grown = pm.replaceProjectData(id, more);
        QVERIFY2(grown.replaced, qPrintable(grown.Details()));
        QCOMPARE(project->DataPoints(), rows);
        QCOMPARE(model->ModelTable()->rowCount(), rows);
        QVERIFY(std::isfinite(model->SSE()));
    }

    // (j) the points left out of the fit stay left out: the dialog marks every injection, the fixture leaves injections out of
    // the fit; the undo gives the values back with all their digits (Claude Generated)
    void itcKeepsLeftOutPoints()
    {
        ProjectManager& pm = ProjectManager::instance();
        QSharedPointer<DataClass> project = loadItc();
        QVERIFY(project);
        const QString id = project->UUID();
        AbstractModel* model = models(project.data()).first();
        QVERIFY(!project->DependentModel()->isChecked(0, 0)); // the fixture leaves the first injection out
        const double fitted = model->SSE();
        int leftOut = 0; // cells left out in the rows both data sets have
        const Eigen::MatrixXd marksBefore = project->DependentModel()->CheckedTable();
        for (int i = 0; i + 1 < project->DataPoints(); ++i) {
            leftOut += project->IndependentModel()->isChecked(i, 0) ? 0 : 1;
            leftOut += project->DependentModel()->isChecked(i, 0) ? 0 : 1;
        }
        fmt::print("  ITC fixture: {} cells left out\n", leftOut);
        const QByteArray dataBefore = dataBytes(project.data()), modelsBefore = modelBytes(project.data());

        // as the import dialog hands the data over: every cell marked, one injection less
        QJsonObject fromDialog = dropLastRow(project->ExportData());
        for (const char* key : { "independent", "dependent" }) {
            DataTable table;
            table.ImportTable(fromDialog[QLatin1String(key)].toObject());
            table.setCheckedAll(true);
            fromDialog[QLatin1String(key)] = table.ExportTable(true);
        }
        const DataReplaceReport report = pm.replaceProjectData(id, fromDialog);
        QVERIFY2(report.replaced, qPrintable(report.Details()));
        QCOMPARE(report.uncheckedKept, leftOut);
        QVERIFY(!project->DependentModel()->isChecked(0, 0));
        QVERIFY(project->DependentModel()->CheckedTable() == marksBefore.topRows(marksBefore.rows() - 1));
        fmt::print("  ITC left-out point kept: SSE fitted {:.6g}, after the replacement {:.6g}\n", fitted, model->SSE());
        // the last injection is gone, the first stays out: the parameters still describe the data (with the first injection
        // used again, the SSE rose from 0.075 to 356)
        QVERIFY2(model->SSE() < 10 * fitted + 1e-3, qPrintable(QString::number(model->SSE())));

        QVERIFY(pm.undoLastDataReplacement(id));
        QVERIFY(dataBytes(project.data()) == dataBefore);
        QVERIFY(modelBytes(project.data()) == modelsBefore);
        QCOMPARE(model->SSE(), fitted);

        // DropModels takes the marks of the new data (no model depends on the old ones)
        const DataReplaceReport dropped = pm.replaceProjectData(id, fromDialog, DataReplacePolicy::DropModels);
        QVERIFY(dropped.replaced);
        QCOMPARE(dropped.uncheckedKept, 0);
        QVERIFY(project->DependentModel()->isChecked(0, 0));
    }
};

QTEST_MAIN(TestDataReplace)
#include "test_data_replace.moc"
