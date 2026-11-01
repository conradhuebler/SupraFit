/*
 * Project files of older versions and the transparency of a load - Claude Generated (2026)
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
 * Two things are pinned here:
 *  - a load never fails or loses a model without saying why (`ProjectManager::lastLoadReport`);
 *  - the project files of older versions still open (fixtures of the repository; and, when `SUPRAFIT_PROJECT_CORPUS` names a directory,
 *    every project file below it, with a table by file version).
 */

#include <QtTest/QtTest>

#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QTemporaryDir>

#include "src/core/jsonhandler.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/meta_model.h"
#include "src/core/models/models.h"
#include "src/core/projectmanager.h"

#ifndef SUPRAFIT_SOURCE_DIR
#define SUPRAFIT_SOURCE_DIR "."
#endif

using SupraFit::LoadIssue;
using SupraFit::LoadReport;
using SupraFit::ProjectManager;

class TestProjectCompat : public QObject {
    Q_OBJECT

private:
    static QString sample(const QString& relative) { return QStringLiteral(SUPRAFIT_SOURCE_DIR) + QStringLiteral("/data/samples/") + relative; }

    static LoadReport load(const QString& file)
    {
        ProjectManager::instance().clearAllProjects();
        ProjectManager::instance().loadProject(file);
        return ProjectManager::instance().lastLoadReport();
    }

    static QString write(const QTemporaryDir& dir, const QString& name, const QByteArray& content)
    {
        const QString path = dir.filePath(name);
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write(content);
        return path;
    }

    static bool hasError(const LoadReport& r)
    {
        return std::any_of(r.issues.begin(), r.issues.end(), [](const LoadIssue& i) { return i.severity == LoadIssue::Error; });
    }

private slots:
    // The system parameters of a thermogram carry the names of the ITC model, not "Imported parameter".
    void thermogramSystemParametersAreNamed()
    {
        QJsonObject json;
        QVERIFY(JsonHandler::ReadJsonFile(json, sample(QStringLiteral("reference/simulated_1_1.json"))));
        QJsonObject data = json["data"].toObject();
        data["DataType"] = 2; // thermogram
        QJsonObject system;
        system["1"] = "1.4";
        system["2"] = "0.1";
        system["3"] = "1.0";
        system["4"] = "298";
        system["1024"] = "Number";
        data["system"] = system;
        DataClass project;
        QVERIFY(project.ImportData(data));
        QCOMPARE(project.getSystemParameter(1).Name(), QStringLiteral("Cell Volume"));
        QCOMPARE(project.getSystemParameter(2).Name(), QStringLiteral("Cell concentration"));
        QCOMPARE(project.getSystemParameter(3).Name(), QStringLiteral("Syringe concentration"));
        QCOMPARE(project.getSystemParameter(4).Name(), QStringLiteral("Temperature"));
        QCOMPARE(project.getSystemParameter(1024).Name(), QStringLiteral("Plot Mode"));
        QCOMPARE(project.getSystemParameter(1024).getString(), QStringLiteral("Number"));
    }

    // Data that do not say their model family keep placeholders (1 is the cell volume of ITC, a volume of BET, the temperature of
    // TIAN); the model attached to them declares the names and the stored values stay. Claude Generated.
    void modelNamesPlaceholders()
    {
        QJsonObject json;
        QVERIFY(JsonHandler::ReadJsonFile(json, sample(QStringLiteral("reference/simulated_1_1.json"))));
        QJsonObject data = json["data"].toObject();
        data["DataType"] = 1; // table
        QJsonObject system;
        system["1"] = "1.4";
        system["5"] = "false";
        data["system"] = system;
        QSharedPointer<DataClass> project(new DataClass);
        QVERIFY(project->ImportData(data));
        QVERIFY(project->getSystemParameter(1).isPlaceholder());
        QSharedPointer<DataClass> table(new DataClass);
        QVERIFY(table->ImportData(data));
        // an ITC model needs one independent column: the table of an NMR titration has two, so take the first
        DataTable* independent = new DataTable(table->IndependentModel()->rowCount(), 1, nullptr); // owned by the data of the project
        for (int i = 0; i < independent->rowCount(); ++i)
            independent->data(i, 0) = table->IndependentModel()->data(i, 0);
        project->setIndependentTable(independent);
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::itc_ItoI, project.data());
        QVERIFY(model);
        QVERIFY(!project->getSystemParameter(1).isPlaceholder());
        QCOMPARE(project->getSystemParameter(1).Name(), QStringLiteral("Cell Volume"));
        QCOMPARE(project->getSystemParameter(1).Double(), 1.4);
        QVERIFY(project->getSystemParameter(5).isBool());
        QCOMPARE(project->getSystemParameter(5).Bool(), false);
    }

    // After a load no system parameter of a titration or ITC project is a nameless placeholder ("ImportedParamN"): the project files of
    // the repository and, when SUPRAFIT_PROJECT_CORPUS names a directory, every project file below it. Exempt is index 6, the heat unit
    // of the ITC models before 2018, which no model declares any more; the placeholders of other families (e.g. the initial
    // concentrations of the kinetics of 2018) are listed, not counted. Claude Generated.
    void systemParametersAreNamed()
    {
        QStringList roots{ sample(QString()) };
        if (!qEnvironmentVariable("SUPRAFIT_PROJECT_CORPUS").isEmpty())
            roots << qEnvironmentVariable("SUPRAFIT_PROJECT_CORPUS");
        auto titrationFamily = [](int model) { return (model >= 1 && model <= 4) || (model >= 10 && model <= 17) || (model >= 20 && model <= 36); };
        QStringList unnamed, other;
        int projects = 0;
        for (const QString& root : std::as_const(roots)) {
            QDirIterator it(root, QStringList{ "*.json", "*.suprafit", "*.jdat" }, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString file = it.next();
                if (!load(file).loaded)
                    continue; // not a project file (model definitions, CLI scripts)
                const auto hash = ProjectManager::instance().getProjectHash();
                for (auto p = hash.begin(); p != hash.end(); ++p) {
                    const QSharedPointer<DataClass> data = p.value().toStrongRef();
                    if (!data)
                        continue;
                    ++projects;
                    bool titration = data->DataType() == DataClassPrivate::Thermogram || data->getSystemObject().contains("1024") || data->getSystemObject().contains("1025");
                    for (int i = 0; i < data->ChildrenSize(); ++i)
                        if (data->Children(i))
                            titration = titration || titrationFamily(data->Children(i)->SFModel());
                    for (int index : data->getSystemParameterList()) {
                        if (!data->getSystemParameter(index).isPlaceholder())
                            continue;
                        const QString entry = QStringLiteral("%1 (data type %2): %3").arg(QFileInfo(file).fileName()).arg(int(data->DataType())).arg(index);
                        (titration && index != 6 ? unnamed : other) << entry;
                    }
                }
            }
        }
        unnamed.removeDuplicates();
        other.removeDuplicates();
        qInfo("%d projects", projects);
        for (const QString& entry : std::as_const(other))
            qInfo().noquote() << "declared by no model:" << entry;
        QVERIFY(projects > 0);
        QVERIFY2(unnamed.isEmpty(), qPrintable("nameless: " + unnamed.join(", ")));
    }

    // A file that cannot be read says why, in words, with the place where JSON broke.
    void failuresHaveReasons()
    {
        QTemporaryDir dir;
        const QByteArray good = qCompress(QJsonDocument(JsonHandler::LoadFile(sample("saved.json"))).toJson(QJsonDocument::Compact), 9);

        struct Case {
            QString name;
            QByteArray content;
            QString expected; ///< part of the reason
        };
        const QList<Case> cases = {
            { "empty.suprafit", QByteArray(), "empty" },
            { "garbage.suprafit", QByteArray("this is not a project file at all"), "neither JSON nor compressed" },
            { "cut.suprafit", good.left(good.size() / 2), "neither JSON nor compressed" },
            { "broken.json", QByteArray("{\n \"data\": {\n  \"a\": 1,\n  \"b\": \n }\n}"), "line" },
            { "array.json", QByteArray("[1, 2, 3]"), "array" },
            { "nodata.json", QByteArray("{\"something\": {\"else\": 1}}"), "no \"data\" entry" },
            { "emptydata.json", QByteArray("{\"data\": {}}"), "missing" },
        };
        for (const Case& c : cases) {
            const LoadReport r = load(write(dir, c.name, c.content));
            QVERIFY2(!r.loaded, qPrintable(c.name + ": loaded"));
            QVERIFY2(hasError(r), qPrintable(c.name + ": no reason"));
            QVERIFY2(r.Summary().contains(c.expected, Qt::CaseInsensitive), qPrintable(c.name + ": reason is \"" + r.Summary() + "\""));
            QVERIFY(r.HasProblems());
        }
        const LoadReport missing = load(dir.filePath("nothere.suprafit"));
        QVERIFY(!missing.loaded);
        QVERIFY2(missing.Summary().contains("does not exist"), qPrintable(missing.Summary()));
    }

    // The format comes from the content, not from the name of the file or of the directory it lies in.
    void formatComesFromTheContent()
    {
        QTemporaryDir dir;
        QDir().mkpath(dir.filePath("json_files"));
        const QJsonObject project = JsonHandler::LoadFile(sample("saved.json"));
        QVERIFY(!project.isEmpty());
        const QByteArray plain = QJsonDocument(project).toJson();
        const QByteArray packed = qCompress(QJsonDocument(project).toJson(QJsonDocument::Compact), 9);
        const QList<QPair<QString, QByteArray>> files = {
            { "json_files/packed.suprafit", packed }, // a directory with "json" in its name
            { "plain.suprafit", plain }, // JSON under the name of a project file
            { "packed.json", packed }, // a project file under the name of JSON
        };
        for (const auto& f : files) {
            const LoadReport r = load(write(dir, f.first, f.second));
            QVERIFY2(r.loaded, qPrintable(f.first + ": " + r.Summary()));
            QCOMPARE(r.modelsLoaded, r.modelsInFile);
            QVERIFY(r.modelsInFile >= 1);
        }
    }

    // A project with a model that cannot be read opens with the others, and the report names the one that is missing and why.
    void leftOutModelsAreNamed()
    {
        QTemporaryDir dir;
        QJsonObject project = JsonHandler::LoadFile(sample("reference/simulated_1_1.json"));
        QVERIFY(project.contains("model_1") && project.contains("model_2"));
        QJsonObject unknown = project["model_2"].toObject();
        unknown["model"] = 99999;
        project["model_2"] = unknown;
        QJsonObject noModelField = project["model_3"].toObject();
        noModelField.remove("model");
        project["model_3"] = noModelField;
        project["model_4"] = QJsonArray{ 1, 2 };

        const LoadReport r = load(write(dir, "partial.json", QJsonDocument(project).toJson()));
        QVERIFY2(r.loaded, qPrintable(r.Summary()));
        QCOMPARE(r.modelsInFile, 4);
        QCOMPARE(r.modelsLoaded, 1);
        QVERIFY(r.HasProblems());
        QVERIFY2(r.Summary().contains("1 of 4"), qPrintable(r.Summary()));
        const QString details = r.Details();
        qInfo().noquote() << details;
        QVERIFY(details.contains("model_2") && details.contains("99999"));
        QVERIFY(details.contains("model_3") && details.contains("\"model\" field"));
        QVERIFY(details.contains("model_4") && details.contains("array"));
        QVERIFY(!details.contains("[model_1]") || details.contains("Note")); // the model that was read is not reported as lost
    }

    // A file of a newer version loads what it can and says that it is newer.
    void newerFilesAreSaid()
    {
        QTemporaryDir dir;
        QJsonObject project = JsonHandler::LoadFile(sample("saved.json"));
        QJsonObject data = project["data"].toObject();
        data["SupraFit"] = 9999;
        project["data"] = data;
        const LoadReport r = load(write(dir, "future.json", QJsonDocument(project).toJson()));
        QVERIFY(r.loaded);
        QCOMPARE(r.fileVersion, 9999);
        QVERIFY(r.HasProblems());
        QVERIFY2(r.Summary().contains("newer"), qPrintable(r.Summary()));
    }

    // The files of older versions in the repository open without a problem report.
    void repositoryFixturesOpen_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<int>("version");
        QTest::newRow("2003 script model") << sample("saved.json") << 2003;
        QTest::newRow("2003 reference 1:1") << sample("reference/simulated_1_1.json") << 2003;
        QTest::newRow("2003 reference 2:1/1:2") << sample("reference/simulated_2_1_1_1_1_2.json") << 2003;
        QTest::newRow("compressed NMR titrations") << sample("NMR titrations/projects.suprafit") << 0;
    }
    void repositoryFixturesOpen()
    {
        QFETCH(QString, file);
        QFETCH(int, version);
        const LoadReport r = load(file);
        QVERIFY2(r.loaded, qPrintable(r.Summary()));
        QVERIFY2(!r.HasProblems(), qPrintable(r.Details()));
        QVERIFY(r.modelsInFile >= 1);
        if (version > 0)
            QCOMPARE(r.fileVersion, version);
    }

    static double firstModelSSE()
    {
        for (const QWeakPointer<DataClass>& weak : ProjectManager::instance().getAllProjects())
            if (const QSharedPointer<DataClass> project = weak.toStrongRef())
                for (int i = 0; i < project->ChildrenSize(); ++i)
                    if (AbstractModel* model = qobject_cast<AbstractModel*>(project->Children(i).data()))
                        return model->SSE();
        return -1;
    }

    // The files of SupraFit 1.x/2.x (data as "concentrations"/"signals", the model by its name, the old parameter keys), of a file with
    // several projects, and of a nmr_any model defined by the grid MaxA/MaxB: read, complete, and the numbers come out as stored.
    void legacyLayoutsAreRead()
    {
        const QString dir = QStringLiteral(SUPRAFIT_SOURCE_DIR) + QStringLiteral("/data/samples/legacy/");

        LoadReport r = load(dir + "legacy_1599_nmr_1_1.suprafit");
        QVERIFY2(r.loaded && !r.HasProblems(), qPrintable(r.Details()));
        QCOMPARE(r.modelsLoaded, 1);
        QVERIFY2(firstModelSSE() >= 0 && firstModelSSE() < 1e-12, qPrintable(QString("noise-free data, sum of squares %1").arg(firstModelSSE())));

        r = load(dir + "multi_project.suprafit");
        QVERIFY2(r.loaded && !r.HasProblems(), qPrintable(r.Details()));
        QCOMPARE(r.projectsInFile, 2);
        QCOMPARE(r.projectsLoaded, 2);
        QCOMPARE(r.modelsLoaded, 5);
        QCOMPARE(ProjectManager::instance().getAllProjects().size(), 2);

        r = load(dir + "nmr_any_grid_2003.json");
        QVERIFY2(r.loaded && r.modelsLoaded == 1, qPrintable(r.Details()));
        QVERIFY2(!r.HasProblems(), qPrintable(r.Details())); // the stored sum of squares (0.0119) is reproduced by the converted model
        QVERIFY2(r.Details().contains("A + B <=> AB; A + 2 B <=> AB2"), qPrintable(r.Details())); // and the conversion is said
        QVERIFY2(std::abs(firstModelSSE() - 0.01188813) < 1e-6, qPrintable(QString::number(firstModelSSE())));
    }

    // A script model of 2002 holds a ChaiScript/Python equation (`def Calculate(int i, int j) {...}`) that the ExprTk engine of today cannot
    // calculate: the project opens, the model is there, and the report says that its fit is not the file's.
    void chaiScriptEquationsAreSaid()
    {
        const LoadReport r = load(sample("ScriptModel_55f90e0.json"));
        QVERIFY(r.loaded);
        QCOMPARE(r.modelsLoaded, 1);
        QCOMPARE(r.fileVersion, 2002);
        QVERIFY2(r.HasProblems(), "a model that cannot be calculated was reported as fine");
        QVERIFY2(r.Details().contains("not reproduced") || r.Details().contains("could not be compiled"), qPrintable(r.Details()));
    }

    // Models that changed after files were written are read as the file had them: the bimolecular model before its scale factor (four
    // parameters), the flexible model of August 2022 (another formula), and an ITC fit that left out its first two points.
    void changedModelsReproduceTheFile()
    {
        const QString dir = QStringLiteral(SUPRAFIT_SOURCE_DIR) + QStringLiteral("/data/samples/legacy/");

        LoadReport r = load(dir + "kinetics_2003.json");
        QVERIFY2(r.loaded && !r.HasProblems(), qPrintable(r.Details()));
        QCOMPARE(r.projectsLoaded, 2);
        QCOMPARE(r.modelsLoaded, 2);

        r = load(dir + "itc_first_points.json");
        QVERIFY2(r.loaded && r.modelsLoaded == 1, qPrintable(r.Details()));
        QVERIFY2(!r.Details().contains("Warning"), qPrintable(r.Details())); // reproduced, no warning
        QVERIFY2(r.Details().contains("leave out the first 2 data points"), qPrintable(r.Details()));
        QVERIFY2(std::abs(firstModelSSE() - 0.0912966) < 1e-5, qPrintable(QString::number(firstModelSSE())));
    }

    // A meta model (it compares models of other projects) is stored as a project of its own in a multi-project file, and is rebuilt on loading from the
    // projects before it: same models, same fits, as a project of the manager again (so that it is saved the next time). Claude Generated.
    void metaModelsSurviveSavingAndLoading()
    {
        ProjectManager& pm = ProjectManager::instance();
        pm.clearAllProjects();
        QVERIFY(pm.loadProject(sample("reference/simulated_1_1.json")));
        QVERIFY(pm.loadProject(sample("reference/simulated_2_1_1_1.json")));
        QStringList ids = pm.getLoadedProjectIds();
        QCOMPARE(ids.size(), 2);

        // the meta model of one model of each project, made as the program makes it
        QSharedPointer<DataClass> base = QSharedPointer<DataClass>::create();
        base->setProjectTitle(QStringLiteral("Meta for test"));
        base->NewUUID();
        QSharedPointer<AbstractModel> created = CreateModel(SupraFit::MetaModel, base);
        ::MetaModel* meta = qobject_cast<::MetaModel*>(created.data());
        QVERIFY(meta);
        QList<double> sse;
        for (const QString& id : ids) {
            QSharedPointer<DataClass> project = pm.getProjectData(id);
            QVERIFY(project && project->ChildrenSize() >= 1);
            AbstractModel* model = qobject_cast<AbstractModel*>(project->Children(0).data());
            QVERIFY(model);
            sse << model->SSE();
            meta->addModel(model);
        }
        QCOMPARE(meta->ModelSize(), 2);
        QVERIFY(pm.registerExistingProject(created.staticCast<DataClass>()));
        QCOMPARE(pm.getLoadedProjectIds().size(), 3);

        // saved like the program saves several projects
        QJsonObject file;
        const QStringList all = pm.getLoadedProjectIds();
        for (int i = 0; i < all.size(); ++i)
            file[QStringLiteral("project_%1").arg(i)] = pm.getProjectFileJson(all[i]);
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("with_meta.suprafit"));
        QVERIFY(JsonHandler::WriteJsonFile(file, path));

        const LoadReport r = load(path); // clears the manager first
        QVERIFY2(r.loaded, qPrintable(r.Details()));
        QCOMPARE(r.projectsInFile, 3);
        QCOMPARE(r.projectsLoaded, 3);
        QVERIFY2(!r.Details().contains("Warning"), qPrintable(r.Details()));
        QVERIFY2(r.Details().contains("rebuilt"), qPrintable(r.Details()));
        const QVector<QWeakPointer<DataClass>> projects = pm.getAllProjects();
        QCOMPARE(projects.size(), 3);
        ::MetaModel* restored = nullptr;
        for (const QWeakPointer<DataClass>& weak : projects)
            if (QSharedPointer<DataClass> p = weak.toStrongRef())
                if (::MetaModel* m = qobject_cast<::MetaModel*>(p.data()))
                    restored = m;
        QVERIFY2(restored, "the meta model is not a project of the manager after loading");
        QCOMPARE(restored->ModelSize(), 2);
        QCOMPARE(restored->ProjectTitle(), QStringLiteral("Meta for test"));
        // the models the meta model was built from are not left behind as models of the projects they were made on (the meta model keeps copies)
        for (const QWeakPointer<DataClass>& weak : projects)
            if (QSharedPointer<DataClass> p = weak.toStrongRef())
                if (!qobject_cast<::MetaModel*>(p.data()))
                    QCOMPARE(p->ChildrenSize(), 4);
        QCoreApplication::processEvents(); // the temporary models are deleted later
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        for (const QWeakPointer<DataClass>& weak : projects)
            if (QSharedPointer<DataClass> p = weak.toStrongRef())
                if (!qobject_cast<::MetaModel*>(p.data()))
                    QCOMPARE(p->ChildrenSize(), 4);
        for (int i = 0; i < 2; ++i)
            QVERIFY2(std::abs(restored->Models()[i]->SSE() - sse[i]) < 1e-6 * std::max(1.0, sse[i]) + 1e-12, qPrintable(QStringLiteral("model %1: SSE %2, was %3").arg(i).arg(restored->Models()[i]->SSE()).arg(sse[i])));

        // and it is saved again (the manager writes it as a project of its own)
        const QStringList again = pm.getLoadedProjectIds();
        QCOMPARE(again.size(), 3);
        int metaBlocks = 0;
        for (const QString& id : again)
            if (pm.getProjectFileJson(id)["data"].toObject()["model"].toInt() == int(SupraFit::MetaModel))
                ++metaBlocks;
        QCOMPARE(metaBlocks, 1);

        // a meta model whose projects are not in the file is left out and the report says which
        QJsonObject onlyMeta;
        for (const QString& id : again)
            if (pm.getProjectFileJson(id)["data"].toObject()["model"].toInt() == int(SupraFit::MetaModel)) {
                onlyMeta[QStringLiteral("project_0")] = pm.getProjectFileJson(id);
                onlyMeta[QStringLiteral("project_1")] = pm.getProjectFileJson(again.first() == id ? again.last() : again.first());
            }
        const LoadReport lonely = load(write(dir, QStringLiteral("lonely.json"), QJsonDocument(onlyMeta).toJson()));
        QVERIFY(lonely.loaded);
        QVERIFY(lonely.HasProblems());
        QVERIFY2(lonely.Details().contains("is not in the file"), qPrintable(lonely.Details()));
    }

    // A model whose stored fit is not reproduced (a changed model, an equation that cannot be calculated) is read, and the report says so.
    void unreproducedFitIsSaid()
    {
        QTemporaryDir dir;
        QJsonObject project = JsonHandler::LoadFile(sample("reference/simulated_1_1.json"));
        QJsonObject model = project["model_1"].toObject();
        model["SSE"] = model["SSE"].toDouble() * 0.25; // the file claims a fit four times better than its parameters give
        project["model_1"] = model;
        LoadReport r = load(write(dir, "claim.json", QJsonDocument(project).toJson()));
        QVERIFY(r.loaded);
        QCOMPARE(r.modelsLoaded, 4);
        QVERIFY(r.HasProblems());
        QVERIFY2(r.Summary().contains("not reproduced"), qPrintable(r.Summary()));
        QVERIFY(r.Details().contains("[model_1]"));
        QVERIFY(!r.Details().contains("[model_2]")); // the others are fine

        // a better fit than the file says is not a problem (linear parameters are projected out): a note
        model["SSE"] = model["SSE"].toDouble() * 400;
        project["model_1"] = model;
        r = load(write(dir, "modest.json", QJsonDocument(project).toJson()));
        QVERIFY2(!r.HasProblems(), qPrintable(r.Details()));
        QVERIFY(r.Details().contains("Note [model_1]"));
    }

    // One file named by SUPRAFIT_PROJECT_FILE, one result line (for a driver that runs a corpus file by file, so that a crash costs one file).
    void oneFile()
    {
        const QString file = qEnvironmentVariable("SUPRAFIT_PROJECT_FILE");
        if (file.isEmpty())
            QSKIP("SUPRAFIT_PROJECT_FILE is not set");
        const LoadReport r = load(file);
        // the fit as stored against the model as read: the sum of squares is recomputed on loading, a model that was read wrongly shows it
        QString sse;
        QJsonObject json;
        if (r.loaded && JsonHandler::ReadFile(file, json)) {
            QList<double> stored;
            QList<QPair<int, int>> info; // model version and number of each
            QStringList topKeys = json.contains("data") ? QStringList{ QString() } : json.keys();
            std::sort(topKeys.begin(), topKeys.end(), [](const QString& a, const QString& b) { return a.mid(8).toInt() < b.mid(8).toInt(); }); // project_2 before project_10, as the loader reads them
            for (const QString& top : topKeys) {
                const QJsonObject block = top.isEmpty() ? json : json[top].toObject();
                if (!top.isEmpty() && !block.contains("data"))
                    continue;
                for (const QString& key : block.keys())
                    if (key.startsWith("model_")) {
                        const QJsonObject m = block[key].toObject();
                        const QJsonValue v = m.contains("SSE") ? m["SSE"] : m["sum_of_squares"];
                        stored << (v.isUndefined() ? -1.0 : v.toVariant().toDouble());
                        info << qMakePair(m["SupraFit"].toInt(), m["model"].isString() ? -1 : m["model"].toInt());
                    }
            }
            QList<int> unchecked; // rows excluded from the fit
            QList<double> loadedSSE, spread; // spread: the sum of squares of the data around their column means, the scale of an SSE
            for (const QWeakPointer<DataClass>& weak : ProjectManager::instance().getAllProjects())
                if (const QSharedPointer<DataClass> project = weak.toStrongRef())
                    for (int i = 0; i < project->ChildrenSize(); ++i)
                        if (AbstractModel* model = qobject_cast<AbstractModel*>(project->Children(i).data()))
                        {
                            loadedSSE << model->SSE();
                            if (qEnvironmentVariableIsSet("SUPRAFIT_PROJECT_DEBUG")) {
                                QStringList g, l;
                                for (int k = 0; k < model->GlobalParameterSize(); ++k)
                                    g << QString::number(model->GlobalParameter(k), 'g', 6);
                                for (int k = 0; k < model->LocalParameterSize(); ++k)
                                    l << QString::number(model->LocalParameter(k, 0), 'g', 6);
                                QStringList sys;
                                for (int index : model->getSystemParameterList())
                                    sys << QStringLiteral("%1=%2").arg(index).arg(model->getSystemParameter(index).value().toString());
                                fprintf(stderr, "DEBUG data %d..%d of %d rows, %d independent cols, system [%s] options %s\n", model->DataBegin(), model->DataEnd(), model->DataPoints(), model->IndependentModel()->columnCount(), qPrintable(sys.join(' ')), "");
                                fprintf(stderr, "DEBUG model %s id %d SSE %g global [%s] local0 [%s] converged %d\n", qPrintable(model->Name()), int(model->SFModel()), model->SSE(), qPrintable(g.join(' ')), qPrintable(l.join(' ')), int(model->isConverged()));
                            }
                            double total = 0;
                            const DataTable* dep = model->DependentModel();
                            for (int c = 0; c < dep->columnCount(); ++c) {
                                double mean = 0;
                                for (int row = 0; row < dep->rowCount(); ++row)
                                    mean += dep->data(row, c) / dep->rowCount();
                                for (int row = 0; row < dep->rowCount(); ++row)
                                    total += (dep->data(row, c) - mean) * (dep->data(row, c) - mean);
                            }
                            spread << total;
                            int off = 0;
                            for (int row = 0; model->DependentModel()->columnCount() && model->IndependentModel()->columnCount() && row < std::min(model->DependentModel()->rowCount(), model->IndependentModel()->rowCount()); ++row)
                                off += !model->DependentModel()->isChecked(row, 0) || !model->IndependentModel()->isChecked(row, 0);
                            unchecked << off;
                        }
            double worst = 0;
            int compared = 0;
            if (stored.size() == loadedSSE.size())
                for (int i = 0; i < stored.size(); ++i)
                    if (stored[i] > 0) {
                        ++compared;
                        worst = std::max(worst, std::abs(loadedSSE[i] - stored[i]) / std::max(stored[i], 1e-6 * spread[i])); // below a millionth of the spread an SSE is rounding
                    }
            if (qEnvironmentVariableIsSet("SUPRAFIT_PROJECT_ROWS") && stored.size() == loadedSSE.size())
                for (int i = 0; i < stored.size(); ++i)
                    fprintf(stderr, "MODELROW\t%s\t%d\t%d\t%g\t%g\t%g\t%d\n", qPrintable(QFileInfo(file).fileName()), info[i].first, info[i].second, stored[i], loadedSSE[i], spread[i], unchecked[i]);
            QStringList pairs;
            for (int i = 0; i < std::min<int>(stored.size(), 8) && stored.size() == loadedSSE.size(); ++i)
                pairs << QStringLiteral("%1/%2").arg(stored[i], 0, 'g', 4).arg(loadedSSE[i], 0, 'g', 4);
            sse = compared ? QStringLiteral("SSE %1 models, worst relative difference %2").arg(compared).arg(worst, 0, 'g', 3) + QStringLiteral(" stored/loaded: ") + pairs.join(' ') : QStringLiteral("SSE not compared");
        }
        fprintf(stderr, "RESULT\t%d\t%d\t%d\t%d/%d\t%s\t%s\n", int(r.loaded), r.projectsLoaded, r.projectsInFile, r.modelsLoaded, r.modelsInFile, qPrintable(r.Details().replace('\n', " | ").left(400)), qPrintable(sse));
    }

    // Every project file below SUPRAFIT_PROJECT_CORPUS (skipped if unset): a table by file version, and the rule that no load fails or
    // drops a model without a reason.
    void corpus()
    {
        const QString root = qEnvironmentVariable("SUPRAFIT_PROJECT_CORPUS");
        if (root.isEmpty())
            QSKIP("SUPRAFIT_PROJECT_CORPUS is not set");
        struct Tally {
            int files = 0, clean = 0, partial = 0, failed = 0, unexplained = 0;
        };
        QMap<QString, Tally> byVersion;
        QStringList failedFiles, unexplained;
        QDirIterator it(root, QStringList{ "*.suprafit", "*.jdat" }, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString file = it.next();
            if (qEnvironmentVariableIsSet("SUPRAFIT_PROJECT_CORPUS_VERBOSE"))
                fprintf(stderr, "loading %s\n", qPrintable(file)); // the last line before a crash names the file
            const LoadReport r = load(file);
            QString key = r.fileVersion > 0 ? QString::number(r.fileVersion) : QStringLiteral("unknown");
            if (!r.loaded && r.fileVersion == 0) { // the version of a file that did not open: from the JSON, if it can be read
                QJsonObject json;
                if (JsonHandler::ReadFile(file, json)) {
                    const int v = json["data"].toObject()["SupraFit"].toInt();
                    key = v > 0 ? QString::number(v) : (json.contains("project_0") ? QStringLiteral("multi-project") : QStringLiteral("unknown"));
                }
            }
            Tally& t = byVersion[key];
            ++t.files;
            if (!r.loaded) {
                ++t.failed;
                failedFiles << QStringLiteral("%1: %2").arg(QDir(root).relativeFilePath(file), r.Summary());
                if (r.Summary().isEmpty()) {
                    ++t.unexplained;
                    unexplained << file;
                }
            } else if (r.HasProblems()) {
                ++t.partial;
                if (r.Summary().isEmpty()) {
                    ++t.unexplained;
                    unexplained << file;
                }
            } else
                ++t.clean;
        }
        qInfo("%-16s %6s %6s %8s %7s", "file version", "files", "clean", "partial", "failed");
        for (auto v = byVersion.begin(); v != byVersion.end(); ++v)
            qInfo("%-16s %6d %6d %8d %7d", qPrintable(v.key()), v->files, v->clean, v->partial, v->failed);
        const int shown = std::min<int>(failedFiles.size(), 25);
        for (int i = 0; i < shown; ++i)
            qInfo().noquote() << "failed:" << failedFiles[i];
        QVERIFY2(unexplained.isEmpty(), qPrintable("without a reason: " + unexplained.join(", ")));
    }
};

QTEST_MAIN(TestProjectCompat)
#include "test_project_compat.moc"
