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
            const QStringList topKeys = json.contains("data") ? QStringList{ QString() } : json.keys();
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
                    fprintf(stderr, "MODELROW\t%s\t%d\t%d\t%g\t%g\t%g\n", qPrintable(QFileInfo(file).fileName()), info[i].first, info[i].second, stored[i], loadedSSE[i], spread[i]);
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
