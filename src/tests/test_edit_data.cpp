/*
 * SupraFit - Edit Data in the main window: controller, outdated display, undo, unsaved data
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
 *
 */

/*
 * Offscreen test of Edit Data in a SupraFitGui (Claude Generated, E1). The import dialogs are replaced by a dialog handler that
 * hands over prepared data blocks, the questions by handlers; everything else is the real window: the controller of the
 * project, the model panes (status chip, banner, Fit, Restore), the project list, the close question, save and load.
 * Not covered: the real import dialogs with a mouse, the modal question boxes clicked by hand, fits running in a thread while
 * the user clicks. SUPRAFIT_EDITDATA_GRAB=<dir> saves PNGs of the pane (light, dark), the question and the project list.
 */

#include <QtTest/QtTest>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTreeView>

#include "src/capabilities/jobmanager.h"
#include "src/core/jsonhandler.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/models.h"
#include "src/core/projectmanager.h"
#include "src/global.h"
#include "src/ui/guitools/appearance.h"
#include "src/ui/guitools/outdatedbanner.h"
#include "src/ui/guitools/statusline.h"
#include "src/ui/instance.h"
#include "src/ui/mainwindow/editdatacontroller.h"
#include "src/ui/mainwindow/mainwindow.h"
#include "src/ui/mainwindow/modeldataholder.h"
#include "src/ui/mainwindow/modelwidget.h"
#include "src/ui/mainwindow/projecttree.h"
#include "src/ui/mainwindow/suprafitgui.h"
#include "src/ui/widgets/modelactions.h"

#include <fmt/format.h>

#ifndef SUPRAFIT_SOURCE_DIR
#define SUPRAFIT_SOURCE_DIR "."
#endif

using SupraFit::ProjectManager;
using Outcome = EditDataController::Outcome;
using Choice = EditDataController::Choice;

class TestEditData : public QObject {
    Q_OBJECT

    static QString sample(const QString& relative) { return QStringLiteral(SUPRAFIT_SOURCE_DIR) + QStringLiteral("/data/samples/") + relative; }

    static QJsonObject tableJson(const Eigen::MatrixXd& values, const QStringList& header, const Eigen::MatrixXd& checked = Eigen::MatrixXd())
    {
        DataTable table(int(values.rows()), int(values.cols()), nullptr);
        table.Table() = values;
        if (checked.rows() == values.rows() && checked.cols() == values.cols())
            table.setCheckedTable(checked);
        table.setHeader(header);
        return table.ExportTable(true);
    }
    /*! \brief The data block with its last row removed from all four tables (as an import dialog with one point less). */
    static QJsonObject dropLastRow(QJsonObject data)
    {
        for (const char* key : { "independent", "dependent", "independent_raw", "dependent_raw" }) {
            DataTable table;
            table.ImportTable(data[QLatin1String(key)].toObject());
            if (table.rowCount() < 2)
                continue;
            data[QLatin1String(key)] = tableJson(table.Table().topRows(table.rowCount() - 1), table.header(), table.CheckedTable().topRows(table.rowCount() - 1));
        }
        data["begin_data"] = 0;
        data["end_data"] = 0;
        return data;
    }
    /*! \brief The data block with one more series: incompatible with the models. */
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
        data["uuid"] = QUuid::createUuid().toString(); // a dialog makes a new data set
        return data;
    }
    static QByteArray dataBytes(DataClass* project)
    {
        QJsonObject data = project->ExportData();
        data.remove("timestamp");
        return QJsonDocument(data).toJson(QJsonDocument::Compact);
    }

    MainWindow* windowOf(const QString& id) const
    {
        for (MainWindow* window : m_gui->ProjectWindows())
            if (window->UUID() == id)
                return window;
        return nullptr;
    }
    ModelDataHolder* holderOf(const QString& id) const
    {
        MainWindow* window = windowOf(id);
        return window ? window->getModelDataHolder() : nullptr;
    }
    QList<ModelWidget*> panesOf(const QString& id) const
    {
        QList<ModelWidget*> panes;
        if (ModelDataHolder* holder = holderOf(id))
            for (const QPointer<ModelWidget>& w : holder->ModelWidgets())
                if (w)
                    panes << w.data();
        return panes;
    }
    /*! \brief Opens a copy of a project file (with a new UUID: every test has its own project) and waits for its panes. */
    QString open(const QString& source, int models)
    {
        QJsonObject content;
        if (!JsonHandler::ReadJsonFile(content, source))
            return QString();
        QJsonObject data = content["data"].toObject();
        data["uuid"] = QUuid::createUuid().toString();
        content["data"] = data;
        const QString file = m_dir.filePath(QStringLiteral("project_%1.json").arg(++m_opened));
        if (!JsonHandler::WriteJsonFile(content, file))
            return QString();
        m_gui->LoadFile(file);
        const QString id = ProjectManager::instance().lastLoadReport().projectId;
        for (int i = 0; i < 200 && panesOf(id).size() < models; ++i)
            QTest::qWait(20);
        return id;
    }
    QStringList treeTexts() const
    {
        QStringList texts;
        ProjectTree* tree = m_gui->findChild<ProjectTree*>();
        if (!tree)
            return texts;
        for (int i = 0; i < tree->rowCount(); ++i) {
            const QModelIndex top = tree->index(i, 0);
            texts << tree->data(top, Qt::DisplayRole).toString();
            for (int j = 0; j < tree->rowCount(top); ++j)
                texts << tree->data(tree->index(j, 0, top), Qt::DisplayRole).toString();
        }
        return texts;
    }
    void grab(QWidget* widget, const QString& name) const
    {
        if (m_grab.isEmpty() || !widget)
            return;
        QApplication::processEvents();
        widget->grab().save(m_grab + "/" + name + ".png");
    }
    /*! \brief A controller whose dialog hands over \p data. */
    static void handOver(EditDataController* controller, const QJsonObject& data)
    {
        controller->setDialogHandler([data](QWidget*, const QSharedPointer<DataClass>&, EditDataController::Input& input) {
            input.data = data;
            return true;
        });
    }

    SupraFitGui* m_gui = nullptr;
    QTemporaryDir m_dir;
    QString m_grab;
    int m_opened = 0;

private slots:
    void initTestCase()
    {
        const int t = QThread::idealThreadCount();
        qApp->setProperty("threads", t > 1 ? t : 2);
        qApp->setProperty("ask_on_exit", false);
        qApp->setProperty("auto_confidence", false);
        m_grab = qEnvironmentVariable("SUPRAFIT_EDITDATA_GRAB");
        if (!m_grab.isEmpty())
            QDir().mkpath(m_grab);
        // a modal window (first-start message, a question nobody answers here) is closed
        QTimer* closer = new QTimer(this);
        closer->setObjectName("closer");
        connect(closer, &QTimer::timeout, this, []() {
            QWidget* w = QApplication::activeModalWidget();
            if (w && w->property("keep").isNull()) {
                qInfo("closed a modal window: %s", qPrintable(w->windowTitle()));
                w->close();
            }
        });
        closer->start(100);
        m_gui = new SupraFitGui;
        m_gui->resize(1366, 768);
        m_gui->show();
        QTest::qWait(50);
    }

    void cleanupTestCase()
    {
        ProjectManager::instance().clearAllProjects();
        delete m_gui;
    }

    // identical data: nothing happens, no outdated mark
    void identicalNoChange()
    {
        const QString id = open(sample("reference/simulated_1_1.json"), 4);
        QCOMPARE(panesOf(id).size(), 4);
        QSharedPointer<DataClass> project = ProjectManager::instance().getProjectData(id);
        EditDataController* controller = holderOf(id)->EditController();
        handOver(controller, project->ExportData());
        QCOMPARE(controller->edit(id), Outcome::Unchanged);
        QVERIFY(!ProjectManager::instance().hasUnsavedData(id));
        for (ModelWidget* pane : panesOf(id)) {
            QVERIFY(!pane->Model()->isDataOutdated());
            QVERIFY(pane->Banner()->isHidden());
        }
    }

    // compatible data: the models stay and are marked; chip, banner, hints, project list; Mark as current, Fit, Restore,
    // Load, undo
    void compatibleKeepsModels()
    {
        ProjectManager& pm = ProjectManager::instance();
        const QString id = open(sample("reference/simulated_1_1.json"), 4);
        QSharedPointer<DataClass> project = pm.getProjectData(id);
        ModelDataHolder* holder = holderOf(id);
        EditDataController* controller = holder->EditController();
        const QByteArray before = dataBytes(project.data());
        handOver(controller, dropLastRow(project->ExportData()));
        QSignalSpy replaced(windowOf(id), &MainWindow::DataReplaced);
        QCOMPARE(controller->edit(id), Outcome::Replaced);
        QVERIFY(!replaced.isEmpty());
        QCOMPARE(project->DataPoints(), 19);
        QVERIFY(pm.hasUnsavedData(id));
        QVERIFY(holder->TitleBarWidget()->UndoEditAction()->isEnabled());
        const QList<ModelWidget*> panes = panesOf(id);
        QCOMPARE(panes.size(), 4);
        for (ModelWidget* pane : panes) {
            QVERIFY(pane->Model()->isDataOutdated());
            QCOMPARE(pane->StatusChip()->text(), QObject::tr("Data changed – refit"));
            QVERIFY(!pane->Banner()->isHidden());
            QCOMPARE(pane->Model()->ModelTable()->rowCount(), 19);
            StatusLine* hint = pane->findChild<StatusLine*>("resultsOutdatedHint");
            QVERIFY(hint && !hint->isHidden());
        }
        QTRY_VERIFY(treeTexts().filter(QObject::tr("(outdated)")).size() == 4);

        // pictures: the pane with chip and banner in both schemes, the project list
        if (!m_grab.isEmpty()) {
            holder->setCurrentTab(1);
            Appearance::Apply(Appearance::Light);
            grab(panes[0], "modelwidget_outdated_light");
            grab(m_gui, "mainwindow_outdated_light");
            Appearance::Apply(Appearance::Dark);
            grab(panes[0], "modelwidget_outdated_dark");
            grab(m_gui, "mainwindow_outdated_dark");
            Appearance::Apply(Appearance::Light);
            if (QTreeView* view = m_gui->findChild<QTreeView*>()) {
                view->expandAll();
                view->resizeColumnToContents(0);
                grab(view, "projecttree_outdated");
            }
        }

        // Mark as current
        panes[1]->Banner()->currentButton()->click();
        QVERIFY(!panes[1]->Model()->isDataOutdated());
        QVERIFY(panes[1]->Banner()->isHidden());
        QVERIFY(panes[1]->StatusChip()->text() != QObject::tr("Data changed – refit"));
        // Fit again (the Fit of the pane)
        panes[0]->Banner()->fitButton()->click();
        // the clone of the fit is deleted when the window returns to its event loop (deleteLater), as after a click
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!panes[0]->Model()->isDataOutdated());
        QVERIFY(panes[0]->Banner()->isHidden());
        QTRY_VERIFY(treeTexts().filter(QObject::tr("(outdated)")).size() == 2);
        // Restore goes back to the parameters before: still outdated
        ModelActions* actions = panes[2]->findChild<ModelActions*>();
        QVERIFY(actions);
        emit actions->Restore();
        QVERIFY(panes[2]->Model()->isDataOutdated());
        QVERIFY(std::isfinite(panes[2]->Model()->SSE()));
        // parameters loaded from a file are no fit of the current data either
        QJsonObject parameters = panes[3]->Model()->ExportModel();
        parameters.remove("outdated");
        panes[3]->LoadJson(parameters);
        QVERIFY(panes[3]->Model()->isDataOutdated());
        // Restore after a fit: the fitted state is current
        emit panes[0]->findChild<ModelActions*>()->Restore();
        QVERIFY(!panes[0]->Model()->isDataOutdated());

        // Undo Edit Data: the data and models of before
        holder->TitleBarWidget()->UndoEditAction()->trigger();
        QCOMPARE(project->DataPoints(), 20);
        QVERIFY(dataBytes(project.data()) == before);
        QVERIFY(!pm.hasUnsavedData(id));
        for (ModelWidget* pane : panesOf(id)) {
            QVERIFY(!pane->Model()->isDataOutdated());
            QVERIFY(pane->Banner()->isHidden());
            QCOMPARE(pane->Model()->ModelTable()->rowCount(), 20);
        }
        QVERIFY(!EditDataController::canUndo(id));
    }

    // incompatible data: the question; "Create a new project" leaves the project as it is, "Remove the models" closes the panes,
    // undo brings them back
    void incompatibleAsks()
    {
        ProjectManager& pm = ProjectManager::instance();
        const QString id = open(sample("reference/simulated_1_1.json"), 4);
        QSharedPointer<DataClass> project = pm.getProjectData(id);
        EditDataController* controller = holderOf(id)->EditController();
        const QJsonObject newData = addSeries(project->ExportData());
        const QByteArray before = dataBytes(project.data());

        // the question as it is shown, with the reasons and the default button
        const DataReplaceCheck check = project->checkReplacement(newData);
        QCOMPARE(check.result, DataReplaceCheck::Incompatible);
        QMessageBox* box = EditDataController::IncompatibleQuestion(m_gui, project->ProjectTitle(), check);
        box->setProperty("keep", true);
        QCOMPARE(box->defaultButton()->objectName(), QString("editNewProject"));
        QVERIFY(box->informativeText().contains(check.reasons.first()));
        box->show();
        grab(box, "incompatible_question");
        delete box;

        int asked = 0;
        Choice answer = Choice::NewProject;
        controller->setChoiceHandler([&](QWidget*, const QString&, const DataReplaceCheck& c) {
            ++asked;
            return c.result == DataReplaceCheck::Incompatible ? answer : Choice::Cancel;
        });
        handOver(controller, newData);
        const int projects = pm.getLoadedProjectIds().size();
        QCOMPARE(controller->edit(id), Outcome::NewProject);
        QCOMPARE(asked, 1);
        QTRY_COMPARE(pm.getLoadedProjectIds().size(), projects + 1);
        QVERIFY(dataBytes(project.data()) == before);
        QCOMPARE(panesOf(id).size(), 4);
        for (ModelWidget* pane : panesOf(id))
            QVERIFY(!pane->Model()->isDataOutdated());

        answer = Choice::Cancel;
        QCOMPARE(controller->edit(id), Outcome::Cancelled);
        QVERIFY(dataBytes(project.data()) == before);

        answer = Choice::DropModels;
        QCOMPARE(controller->edit(id), Outcome::Replaced);
        QCOMPARE(project->SeriesCount(), 8);
        QCOMPARE(project->ChildrenSize(), 0);
        QCOMPARE(panesOf(id).size(), 0);
        QTabWidget* tabs = windowOf(id)->findChild<QTabWidget*>();
        QVERIFY(tabs);
        QCOMPARE(tabs->count(), 1); // the data overview only

        QVERIFY(controller->undo(id));
        QVERIFY(dataBytes(project.data()) == before);
        QTRY_COMPARE(panesOf(id).size(), 4); // the models come back through modelAdded
    }

    // a running analysis: the question; no cancels, yes stops it and replaces the data afterwards
    void runningJobAsks()
    {
        ProjectManager& pm = ProjectManager::instance();
        const QString id = open(sample("reference/simulated_1_1.json"), 4);
        QSharedPointer<DataClass> project = pm.getProjectData(id);
        EditDataController* controller = holderOf(id)->EditController();
        handOver(controller, dropLastRow(project->ExportData()));
        bool confirm = false;
        int asked = 0;
        QString question;
        controller->setConfirmHandler([&](QWidget*, const QString&, const QString& text) {
            ++asked;
            question = text;
            return confirm;
        });
        QJsonObject job = MonteCarloConfigBlock;
        job["MaxSteps"] = 2000;
        job["Method"] = static_cast<int>(SupraFit::Method::MonteCarlo);

        auto run = [&](Outcome& during) {
            JobManager manager;
            manager.setModel(project->SharedModel(0));
            manager.AddSingleJob(job);
            bool done = false;
            connect(&manager, &JobManager::started, this, [&]() {
                if (!done) {
                    done = true;
                    during = controller->edit(id);
                }
            });
            manager.RunJobs();
        };
        Outcome during = Outcome::Replaced;
        run(during);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(asked, 1);
        QCOMPARE(question, QObject::tr("Stop the running analyses and replace the data?"));
        QCOMPARE(during, Outcome::Cancelled);
        QCOMPARE(project->DataPoints(), 20);

        confirm = true;
        QSignalSpy finished(controller, &EditDataController::finished);
        QElapsedTimer clock;
        clock.start();
        run(during);
        fmt::print("  interrupted Monte Carlo returned after {} ms\n", clock.elapsed());
        // the window returns to its event loop after the job: the clones of the job are deleted there (deleteLater); the test
        // function runs outside any loop, a later qWait would not delete them
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(asked, 2);
        QCOMPARE(during, Outcome::Deferred);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QCOMPARE(finished.first().at(1).value<Outcome>(), Outcome::Replaced);
        QCOMPARE(project->DataPoints(), 19);
        QVERIFY(controller->undo(id));
    }

    // unsaved data: the close question (Cancel keeps the window); saving clears the state; loading the saved file shows the
    // banner again
    void unsavedAndSaved()
    {
        ProjectManager& pm = ProjectManager::instance();
        const QString id = open(sample("reference/simulated_1_1.json"), 4);
        QSharedPointer<DataClass> project = pm.getProjectData(id);
        EditDataController* controller = holderOf(id)->EditController();
        handOver(controller, dropLastRow(project->ExportData()));
        QCOMPARE(controller->edit(id), Outcome::Replaced);
        QVERIFY(pm.projectsWithUnsavedData().contains(id));

        QString question;
        QTimer answer;
        answer.setInterval(20);
        connect(&answer, &QTimer::timeout, this, [&]() {
            QMessageBox* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (!box)
                return;
            box->setProperty("keep", true);
            if (box->text().contains(QObject::tr("Save the changed data?")))
                question = box->text();
            grab(box, "close_question");
            box->button(QMessageBox::Cancel) ? box->button(QMessageBox::Cancel)->click() : box->reject();
        });
        QTimer* closer = findChild<QTimer*>("closer");
        closer->stop(); // this question is answered here
        answer.start();
        // the chat question comes first when chats changed; the test project has none
        QVERIFY(pm.projectsWithUnsavedAttachments().isEmpty());
        m_gui->close();
        answer.stop();
        closer->start();
        QVERIFY2(!question.isEmpty(), "no question for the changed data");
        QVERIFY(m_gui->isVisible());

        const QString path = m_dir.filePath("edited.suprafit");
        QVERIFY(pm.saveProject(path, id));
        QVERIFY(!pm.hasUnsavedData(id));

        // the saved file as another project (open() gives it a new UUID: the project is still open)
        const QString loaded = open(path, 4);
        QVERIFY(loaded != id);
        QCOMPARE(panesOf(loaded).size(), 4);
        for (ModelWidget* pane : panesOf(loaded)) {
            QVERIFY(pane->Model()->isDataOutdated());
            QVERIFY(!pane->Banner()->isHidden());
            QCOMPARE(pane->StatusChip()->text(), QObject::tr("Data changed – refit"));
        }
    }

    // ITC through the Edit Data action of the project: the first injection left out stays out
    void itcThroughEditDataAction()
    {
        ProjectManager& pm = ProjectManager::instance();
        QJsonObject file;
        QVERIFY(JsonHandler::ReadJsonFile(file, sample("legacy/itc_first_points.json")));
        QJsonObject data = file["data"].toObject();
        data["DataType"] = 2;
        data["uuid"] = QUuid::createUuid().toString();
        file["data"] = data;
        const QString id = pm.loadProjectFromJson(file, "itc_fixture");
        for (int i = 0; i < 200 && panesOf(id).isEmpty(); ++i)
            QTest::qWait(20);
        QCOMPARE(panesOf(id).size(), 1);
        QSharedPointer<DataClass> project = pm.getProjectData(id);
        ModelDataHolder* holder = holderOf(id);
        QJsonObject fromDialog = dropLastRow(project->ExportData());
        DataTable dependent;
        dependent.ImportTable(fromDialog["dependent"].toObject());
        dependent.setCheckedAll(true); // the dialog marks every injection
        fromDialog["dependent"] = dependent.ExportTable(true);
        handOver(holder->EditController(), fromDialog);
        const int rows = project->DataPoints();
        holder->TitleBarWidget()->EditDataAction()->trigger();
        QCOMPARE(project->DataPoints(), rows - 1);
        QVERIFY(!project->DependentModel()->isChecked(0, 0));
        ModelWidget* pane = panesOf(id).first();
        QVERIFY(pane->Model()->isDataOutdated());
        QVERIFY(!pane->Banner()->isHidden());
        QVERIFY(holder->EditController()->lastReport().uncheckedKept > 0);
    }

    // the table of an import dialog becomes the data block without the Import Table dialog (DataClass::ImportTableData)
    void importTableData()
    {
        QJsonObject file;
        QVERIFY(JsonHandler::ReadJsonFile(file, sample("reference/simulated_1_1.json")));
        DataClass project(file["data"].toObject());
        DataTable* table = project.IndependentModel()->BlockColumns(0, project.IndependentModel()->columnCount());
        table->appendColumns(project.DependentModel());
        const QJsonObject block = DataClass::ImportTableData(table, 2, DataClassPrivate::DataType(project.DataType()), project.RawData(), "title");
        delete table;
        const DataReplaceCheck check = project.checkReplacement(block);
        QVERIFY2(check.result != DataReplaceCheck::Incompatible, qPrintable(check.Text()));
        QCOMPARE(check.newRows, project.DataPoints());
        QCOMPARE(check.newSeries, project.SeriesCount());
        QCOMPARE(block["active_series"].toString().split(' ').size(), project.SeriesCount());
    }
};

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QTemporaryDir settingsDir;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsDir.path());
    QCoreApplication::setOrganizationName(QStringLiteral("SupraFitEditDataTest"));
    QCoreApplication::setApplicationName(QStringLiteral("test_edit_data"));
    Instance::setInstance(new Instance);
    Appearance::Install();
    TestEditData test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_edit_data.moc"
