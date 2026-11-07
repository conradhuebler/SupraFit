/*
 * SupraFit - restoring meta models from a project file: identity with the model-by-model path and timing
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
 * ProjectManager::restoreMetaModel rebuilds a meta model of a file. The reference below is the way it
 * was rebuilt before (one interactive MetaModel::addModel per model, each preparing the whole meta
 * model again); the manager's result must be identical to it: ExportModel() JSON, SSE before and
 * after Calculate(), combined parameters, connect type, title, names of the models.
 *
 * Files: a file generated here (meta models with connect type None, All and Custom), the file named
 * by SUPRAFIT_META_TEST_FILE and every file below SUPRAFIT_META_CORPUS that holds a meta model.
 * SUPRAFIT_META_TIMING=<n> loads each file n times and prints the time spent in restoreMetaModel
 * (min/median/max). Claude Generated.
 */

#include <QtTest/QtTest>

#include <QtCore/QDirIterator>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>

#include <algorithm>

#include "src/core/jsonhandler.h"
#include "src/core/loadtiming.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/meta_model.h"
#include "src/core/models/models.h"
#include "src/core/projectmanager.h"

#include <fmt/format.h>

using SupraFit::ProjectManager;

class TestMetaRestore : public QObject {
    Q_OBJECT

    static QString sample(const QString& relative) { return QStringLiteral(SUPRAFIT_SOURCE_DIR) + QStringLiteral("/data/samples/") + relative; }

    /*! \brief The meta model blocks of a multi-project file, in file order. */
    static QList<QJsonObject> metaBlocks(const QString& path)
    {
        QJsonObject json;
        QList<QJsonObject> blocks;
        if (!JsonHandler::ReadFile(path, json))
            return blocks;
        QStringList keys;
        for (const QString& key : json.keys())
            if (key.startsWith(QStringLiteral("project_")))
                keys << key;
        std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) { return a.mid(8).toInt() < b.mid(8).toInt(); });
        for (const QString& key : keys) {
            const QJsonObject block = json[key].toObject()["data"].toObject();
            if (block.contains("model") && block["model"].toInt() == int(SupraFit::MetaModel))
                blocks << block;
        }
        return blocks;
    }

    /*! \brief Reference: the meta model of a block rebuilt as before, one interactive addModel per model (the projects of its models
     *  must be loaded in the manager). Returns the meta model, its base data in \p base. */
    static QSharedPointer<AbstractModel> referenceRestore(const QJsonObject& block, QSharedPointer<DataClass>& base)
    {
        ProjectManager& pm = ProjectManager::instance();
        const QString name = block["name"].toString();
        const QString label = name.isEmpty() ? QStringLiteral("MetaModel") : name;
        const int size = block["size"].toInt();
        const QJsonObject uuids = block["uuids"].toObject(), raw = block["raw"].toObject();
        base = QSharedPointer<DataClass>::create();
        base->setProjectTitle(label);
        if (base->UUID().isEmpty())
            base->NewUUID();
        QSharedPointer<AbstractModel> created = CreateModel(SupraFit::MetaModel, base);
        ::MetaModel* meta = qobject_cast<::MetaModel*>(created.data());
        if (!meta)
            return {};
        for (int i = 0; i < size; ++i) {
            const QSharedPointer<DataClass> source = pm.getProjectData(uuids[QString::number(i)].toString());
            const QJsonObject modelBlock = raw[QString::number(i)].toObject();
            if (!source)
                continue;
            const int id = modelBlock["model"].isString() ? int(Name2Model(modelBlock["model"].toString())) : modelBlock["model"].toInt();
            QSharedPointer<AbstractModel> model = CreateModel(id, source.data());
            if (!model)
                continue;
            const bool imported = modelBlock["SupraFit"].toInt() < 2002 ? model->LegacyImportModel(modelBlock) : model->ImportModel(modelBlock);
            if (!imported) {
                source->RemoveChild(model.data());
                continue;
            }
            meta->addModel(model.data());
            source->RemoveChild(model.data());
        }
        if (meta->ModelSize() == 0)
            return {};
        meta->ImportModel(block);
        meta->setProjectTitle(label);
        return created;
    }

    /*! \brief The meta models the manager holds, in the order it restored them. */
    static QList<::MetaModel*> restoredMetaModels()
    {
        QList<::MetaModel*> metas;
        for (const QWeakPointer<DataClass>& weak : ProjectManager::instance().getAllProjects())
            if (QSharedPointer<DataClass> p = weak.toStrongRef())
                if (::MetaModel* m = qobject_cast<::MetaModel*>(p.data()))
                    metas << m;
        return metas;
    }

    static QByteArray compact(const QJsonObject& json) { return QJsonDocument(json).toJson(QJsonDocument::Compact); }

    /*! \brief Restores every meta model of \p path through the manager and the reference way and compares them. */
    static void compareFile(const QString& path)
    {
        ProjectManager& pm = ProjectManager::instance();
        pm.clearAllProjects();
        QVERIFY2(pm.loadProject(path), qPrintable(path));
        const QList<QJsonObject> blocks = metaBlocks(path);
        const QList<::MetaModel*> restored = restoredMetaModels();
        QVERIFY2(!blocks.isEmpty(), qPrintable(path));
        QCOMPARE(restored.size(), blocks.size());
        for (int i = 0; i < blocks.size(); ++i) {
            ::MetaModel* fast = restored[i];
            QSharedPointer<DataClass> base;
            QSharedPointer<AbstractModel> referenceModel = referenceRestore(blocks[i], base);
            ::MetaModel* reference = qobject_cast<::MetaModel*>(referenceModel.data());
            QVERIFY(reference);
            const QString what = QStringLiteral("%1, meta model %2 \"%3\"").arg(QFileInfo(path).fileName()).arg(i).arg(fast->ProjectTitle());

            // the model table as restoring left it, before any call rebuilds it
            const DataTable* fastTable = static_cast<const AbstractModel*>(fast)->ModelTable();
            const DataTable* referenceTable = static_cast<const AbstractModel*>(reference)->ModelTable();
            QVERIFY2(fastTable && referenceTable, qPrintable(what));
            QVERIFY2(const_cast<DataTable*>(fastTable)->ExportTable(true) == const_cast<DataTable*>(referenceTable)->ExportTable(true), qPrintable(what));
            QCOMPARE(fast->ProjectTitle(), reference->ProjectTitle());
            QCOMPARE(int(fast->ConnectionType()), int(reference->ConnectionType()));
            QCOMPARE(fast->ModelSize(), reference->ModelSize());
            for (int m = 0; m < fast->ModelSize(); ++m) {
                QCOMPARE(fast->Models()[m]->Name(), reference->Models()[m]->Name());
                QCOMPARE(fast->Models()[m]->UUID(), reference->Models()[m]->UUID());
            }
            QVERIFY2(fast->CombinedParameter() == reference->CombinedParameter(), qPrintable(what));
            for (int p = 0; p < fast->GlobalParameterSize(); ++p) {
                QCOMPARE(fast->GlobalParameterName(p), reference->GlobalParameterName(p));
                QCOMPARE(fast->GlobalParameter(p), reference->GlobalParameter(p));
            }
            QVERIFY2(fast->GlobalTable()->ExportTable(true) == reference->GlobalTable()->ExportTable(true), qPrintable(what));
            QVERIFY2(fast->LocalTable()->ExportTable(true) == reference->LocalTable()->ExportTable(true), qPrintable(what));
            QVERIFY2(fast->CollectOptimizationParameters() == reference->CollectOptimizationParameters(), qPrintable(what));
            QCOMPARE(fast->GlobalParameterSize(), reference->GlobalParameterSize());
            QCOMPARE(fast->LocalParameterSize(), reference->LocalParameterSize());
            QCOMPARE(fast->SeriesCount(), reference->SeriesCount());
            QCOMPARE(fast->DataPoints(), reference->DataPoints());
            QCOMPARE(fast->Size(), reference->Size());
            QCOMPARE(fast->IndependentVariableSize(), reference->IndependentVariableSize());
            QCOMPARE(fast->InputParameterSize(), reference->InputParameterSize());
            QVERIFY2(fast->IndependentModel()->ExportTable(true) == reference->IndependentModel()->ExportTable(true), qPrintable(what));
            QVERIFY2(fast->DependentModel()->ExportTable(true) == reference->DependentModel()->ExportTable(true), qPrintable(what));
            QVERIFY2(fast->SSE() == reference->SSE(), qPrintable(QStringLiteral("%1: SSE %2 vs %3").arg(what).arg(fast->SSE(), 0, 'g', 17).arg(reference->SSE(), 0, 'g', 17)));

            const QByteArray a = compact(fast->ExportModel()), b = compact(reference->ExportModel());
            if (a != b) {
                QFile fa(QDir::temp().filePath("meta_fast.json")), fb(QDir::temp().filePath("meta_reference.json"));
                fa.open(QIODevice::WriteOnly), fa.write(QJsonDocument(fast->ExportModel()).toJson());
                fb.open(QIODevice::WriteOnly), fb.write(QJsonDocument(reference->ExportModel()).toJson());
            }
            QVERIFY2(a == b, qPrintable(QStringLiteral("%1: ExportModel differs").arg(what)));

            fast->Calculate();
            reference->Calculate();
            QVERIFY2(fast->SSE() == reference->SSE(), qPrintable(QStringLiteral("%1: SSE after Calculate %2 vs %3").arg(what).arg(fast->SSE(), 0, 'g', 17).arg(reference->SSE(), 0, 'g', 17)));
            QVERIFY2(compact(fast->ExportModel()) == compact(reference->ExportModel()), qPrintable(QStringLiteral("%1: ExportModel after Calculate differs").arg(what)));
            QVERIFY2(fast->CombinedParameter() == reference->CombinedParameter(), qPrintable(what));
            fmt::print("  identical: {} ({} models, connect type {}, SSE {:.10g})\n", what.toStdString(), fast->ModelSize(), int(fast->ConnectionType()), fast->SSE());
        }
        pm.clearAllProjects();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    /*! \brief The files of SUPRAFIT_META_TEST_FILE and below SUPRAFIT_META_CORPUS that hold a meta model. */
    static QStringList externalFiles()
    {
        QStringList files;
        const QString single = qEnvironmentVariable("SUPRAFIT_META_TEST_FILE");
        if (!single.isEmpty())
            files << single;
        const QString corpus = qEnvironmentVariable("SUPRAFIT_META_CORPUS");
        if (!corpus.isEmpty()) {
            QDirIterator it(corpus, { QStringLiteral("*.suprafit"), QStringLiteral("*.json") }, QDir::Files, QDirIterator::Subdirectories);
            QStringList found;
            while (it.hasNext())
                found << it.next();
            std::sort(found.begin(), found.end());
            for (const QString& f : found)
                if (!metaBlocks(f).isEmpty())
                    files << f;
        }
        return files;
    }

    QTemporaryDir m_dir;
    QString m_generated;

private slots:
    void initTestCase()
    {
        qputenv("SUPRAFIT_LOAD_TIMING", "1");
        QVERIFY(m_dir.isValid());
    }

    // A file with meta models of every connect type, saved as the program saves several projects. Claude Generated.
    void generatedFileWithMetaModels()
    {
        ProjectManager& pm = ProjectManager::instance();
        pm.clearAllProjects();
        const QStringList references{ "simulated_1_1.json", "simulated_1_1_1_2.json", "simulated_2_1_1_1.json", "simulated_2_1_1_1_1_2.json" };
        for (const QString& r : references)
            QVERIFY(pm.loadProject(sample("reference/" + r)));
        const QStringList ids = pm.getLoadedProjectIds();
        QCOMPARE(ids.size(), references.size());

        auto makeMeta = [&](const QString& title) {
            QSharedPointer<DataClass> base = QSharedPointer<DataClass>::create();
            base->setProjectTitle(title);
            base->NewUUID();
            return CreateModel(SupraFit::MetaModel, base);
        };
        auto child = [&](int project, int index) {
            return qobject_cast<AbstractModel*>(pm.getProjectData(ids[project])->Children(index).data());
        };
        QList<QSharedPointer<AbstractModel>> metas;
        // None: models of different kinds
        metas << makeMeta("Meta none");
        for (int p = 0; p < ids.size(); ++p)
            qobject_cast<::MetaModel*>(metas.last().data())->addModel(child(p, p));
        // All: the 1:1 model of every project
        metas << makeMeta("Meta all");
        {
            ::MetaModel* meta = qobject_cast<::MetaModel*>(metas.last().data());
            for (int p = 0; p < ids.size(); ++p)
                meta->addModel(child(p, 0));
            meta->setConnectType(::MetaModel::All);
            meta->Calculate();
        }
        // Custom: parameters moved by hand
        metas << makeMeta("Meta custom");
        {
            ::MetaModel* meta = qobject_cast<::MetaModel*>(metas.last().data());
            for (int p = 0; p < ids.size(); ++p)
                meta->addModel(child(p, 1));
            meta->MoveParameterList(1, 0);
            meta->MoveSingleParameter(2);
            meta->Calculate();
        }
        for (const QSharedPointer<AbstractModel>& m : metas)
            QVERIFY(pm.registerExistingProject(m.staticCast<DataClass>()));

        QJsonObject file;
        const QStringList all = pm.getLoadedProjectIds();
        for (int i = 0; i < all.size(); ++i)
            file[QStringLiteral("project_%1").arg(i)] = pm.getProjectFileJson(all[i]);
        m_generated = m_dir.filePath(QStringLiteral("generated_meta.suprafit"));
        QVERIFY(JsonHandler::WriteJsonFile(file, m_generated));
        metas.clear();
        pm.clearAllProjects();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QMap<QString, int> connect;
        for (const QJsonObject& block : metaBlocks(m_generated))
            connect[block["name"].toString()] = block["connecttype"].toInt();
        QCOMPARE(connect.size(), 3);
        QCOMPARE(connect.value("Meta none", -1), int(::MetaModel::None));
        QCOMPARE(connect.value("Meta all", -1), int(::MetaModel::All));
        QCOMPARE(connect.value("Meta custom", -1), int(::MetaModel::Custom));
        compareFile(m_generated);
    }

    // addModels emits ModelAdded once per model with its copy, as addModel does, and only after the meta model is prepared. Claude Generated.
    void addModelsSignals()
    {
        ProjectManager& pm = ProjectManager::instance();
        pm.clearAllProjects();
        QVERIFY(pm.loadProject(sample("reference/simulated_1_1.json")));
        QVERIFY(pm.loadProject(sample("reference/simulated_2_1_1_1.json")));
        QVector<QPointer<AbstractModel>> models;
        for (const QString& id : pm.getLoadedProjectIds())
            models << qobject_cast<AbstractModel*>(pm.getProjectData(id)->Children(0).data());
        QSharedPointer<DataClass> base = QSharedPointer<DataClass>::create();
        base->NewUUID();
        QSharedPointer<AbstractModel> created = CreateModel(SupraFit::MetaModel, base);
        ::MetaModel* meta = qobject_cast<::MetaModel*>(created.data());
        QVERIFY(meta);
        QList<int> sizesAtSignal;
        QList<AbstractModel*> emitted;
        connect(meta, &::MetaModel::ModelAdded, this, [&](QSharedPointer<AbstractModel> model) {
            emitted << model.data();
            sizesAtSignal << meta->ModelSize();
        });
        meta->addModels(models);
        QCOMPARE(emitted.size(), models.size());
        for (int i = 0; i < emitted.size(); ++i) {
            QCOMPARE(emitted[i], meta->Models()[i].data());
            QCOMPARE(sizesAtSignal[i], models.size());
        }
        meta->addModels({});
        QCOMPARE(emitted.size(), models.size());
        created.clear();
        pm.clearAllProjects();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // Every meta model of the external files is restored identically. Claude Generated.
    void externalFilesAreRestoredIdentically()
    {
        const QStringList files = externalFiles();
        if (files.isEmpty())
            QSKIP("SUPRAFIT_META_TEST_FILE / SUPRAFIT_META_CORPUS not set");
        for (const QString& f : files)
            compareFile(f);
    }

    // The external files saved again by this version: their model blocks are current ones, which restoring reads only once
    // (older blocks are read a second time, as before). Covers e.g. ITC models whose files are older. Claude Generated.
    void externalFilesSavedAgainAreRestoredIdentically()
    {
        const QStringList files = externalFiles();
        if (files.isEmpty())
            QSKIP("SUPRAFIT_META_TEST_FILE / SUPRAFIT_META_CORPUS not set");
        ProjectManager& pm = ProjectManager::instance();
        int n = 0;
        for (const QString& f : files) {
            pm.clearAllProjects();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QVERIFY2(pm.loadProject(f), qPrintable(f));
            QJsonObject file;
            const QStringList all = pm.getLoadedProjectIds();
            for (int i = 0; i < all.size(); ++i)
                file[QStringLiteral("project_%1").arg(i)] = pm.getProjectFileJson(all[i]);
            const QString saved = m_dir.filePath(QStringLiteral("saved_again_%1_%2").arg(n++).arg(QFileInfo(f).fileName()));
            QVERIFY(JsonHandler::WriteJsonFile(file, saved));
            pm.clearAllProjects();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            if (metaBlocks(saved).isEmpty())
                continue;
            compareFile(saved);
        }
    }

    // SUPRAFIT_META_TIMING=<n>: the time of restoreMetaModel per file, n loads each. Claude Generated.
    void timing()
    {
        const int repeats = qEnvironmentVariableIntValue("SUPRAFIT_META_TIMING");
        if (repeats <= 0)
            QSKIP("SUPRAFIT_META_TIMING not set");
        QStringList files = externalFiles();
        if (!m_generated.isEmpty())
            files.prepend(m_generated);
        ProjectManager& pm = ProjectManager::instance();
        fmt::print("\n{:<48} {:>5} {:>7} {:>10} {:>10} {:>10} {:>10}\n", "file", "metas", "models", "min ms", "median ms", "max ms", "load ms");
        for (const QString& f : files) {
            QVector<double> restore, total;
            int metas = 0, models = 0;
            for (int r = 0; r < repeats; ++r) {
                pm.clearAllProjects();
                QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                LoadTiming::Reset();
                QElapsedTimer timer;
                timer.start();
                QVERIFY(pm.loadProject(f));
                total << timer.nsecsElapsed() / 1e6;
                restore << LoadTiming::Sections().value(QStringLiteral("pm.restoreMetaModel")).first;
                metas = 0, models = 0;
                for (::MetaModel* m : restoredMetaModels())
                    ++metas, models += m->ModelSize();
            }
            if (qEnvironmentVariableIsSet("SUPRAFIT_META_SECTIONS"))
                for (auto it = LoadTiming::Sections().cbegin(); it != LoadTiming::Sections().cend(); ++it)
                    if (it.key().startsWith("meta") || it.key().startsWith("pm.restore"))
                        fmt::print("    {:<36} {:>9.2f} ms {:>5}x\n", it.key().toStdString(), it.value().first, it.value().second);
            std::sort(restore.begin(), restore.end());
            std::sort(total.begin(), total.end());
            fmt::print("{:<48} {:>5} {:>7} {:>10.2f} {:>10.2f} {:>10.2f} {:>10.2f}\n", QFileInfo(f).fileName().toStdString(), metas, models,
                restore.first(), restore[restore.size() / 2], restore.last(), total[total.size() / 2]);
        }
        pm.clearAllProjects();
    }
};

QTEST_MAIN(TestMetaRestore)
#include "test_meta_restore.moc"
