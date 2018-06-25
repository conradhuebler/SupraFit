/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global.h"
#include "src/version.h"

#include "src/core/AbstractModel.h"
#include "src/core/dataclass.h"
#include "src/core/jsonhandler.h"

#include "src/ui/dialogs/configdialog.h"
#include "src/ui/dialogs/importdata.h"

#include "src/ui/guitools/instance.h"

#include "src/ui/mainwindow/chartwidget.h"
#include "src/ui/mainwindow/datawidget.h"
#include "src/ui/mainwindow/mainwindow.h"
#include "src/ui/mainwindow/modeldataholder.h"
#include "src/ui/mainwindow/modelwidget.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtCore/QSettings>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>
#include <QtCore/QWeakPointer>

#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeView>

#include <stdio.h>

#include "suprafitgui.h"

int ProjectTree::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

int ProjectTree::rowCount(const QModelIndex& parent) const
{
    int count = m_project_list->size();

    QPointer<DataClass> data = static_cast<DataClass*>(parent.internalPointer());

    bool model = qobject_cast<AbstractModel*>(data);

    if (parent.isValid()) {
        if (!model)
            count = (*m_project_list)[parent.row()]->ModelCount();
        else
            count = 0;
    }
    return count;
}

QVariant ProjectTree::data(const QModelIndex& index, int role) const
{
    QVariant data;
    if (!index.isValid())
        return data;

    if (role == Qt::DisplayRole) {
        QPointer<DataClass> dataclass = static_cast<DataClass*>(index.internalPointer());
        bool model = qobject_cast<AbstractModel*>(dataclass);

        if (!model)
            data = dataclass->ProjectTitle();
        else {
            data = Model2Name(qobject_cast<AbstractModel*>(dataclass)->SFModel());
        }
    }
    return data;
}

QModelIndex ProjectTree::index(int row, int column, const QModelIndex& parent) const
{

    QModelIndex index;
    if (!hasIndex(row, column, parent))
        index = QModelIndex();

    if (!parent.isValid()) {
        index = createIndex(row, column, (*m_project_list)[row]->Data());
    } else {
        index = createIndex(row, column, (*m_project_list)[parent.row()]->Model(row));
    }

    return index;
}

QModelIndex ProjectTree::parent(const QModelIndex& child) const
{
    QModelIndex index;

    if (!child.isValid())
        return index;

    QPointer<DataClass> data = static_cast<DataClass*>(child.internalPointer());

    if (!data)
        return index;

    bool model = qobject_cast<AbstractModel*>(data);

    QPointer<DataClass> p;
    int dataclass = -1, modelclass = -1;
    int count1 = 0, count2 = 0;
    for (int i = 0; i < m_project_list->size(); ++i) {
        for (int j = 0; j < (*m_project_list)[i]->ModelCount(); ++j) {

            if ((*m_project_list)[i]->Data() == data) {
                dataclass = i;
                count1++;
            }
            if (!model)
                continue;

            if ((*m_project_list)[i]->Model(j) == qobject_cast<AbstractModel*>(data)) {
                modelclass = j;
                dataclass = i;
                p = (*m_project_list)[i]->Data();
                count2++;
                break;
            }
        }
    }
    if (modelclass != -1)
        index = createIndex(dataclass, 0, p);

    return index;
}

SupraFitGui::SupraFitGui()
{
    m_instance = new Instance;
    Instance::setInstance(m_instance);

    m_layout = new QGridLayout;

    QWidget* widget = new QWidget;
    widget->setLayout(m_layout);

    m_project_view = new QTreeView;
    m_project_view->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_project_view->setDragEnabled(true);
    m_project_view->setDragDropMode(QAbstractItemView::DragOnly);

    QAction* action;
    action = new QAction("Load", m_project_view);
    m_project_view->addAction(action);
    connect(action, &QAction::triggered, action, [this]() {
        TreeClicked(m_project_view->currentIndex());

    });

    action = new QAction("Add MetaModel", m_project_view);
    m_project_view->addAction(action);
    connect(action, &QAction::triggered, action, [this]() {
        AddMetaModel(m_project_view->currentIndex());
    });

    action = new QAction("Delete", m_project_view);
    connect(action, &QAction::triggered, action, [this]() {
        TreeRemoveRequest(m_project_view->currentIndex());
    });

    m_project_view->addAction(action);

    m_project_tree = new ProjectTree(&m_project_list);
    m_project_view->setModel(m_project_tree);

    m_layout->addWidget(m_project_view, 0, 0);

    m_project_view->setMaximumWidth(200);

    setCentralWidget(widget);

    ReadSettings();

    m_new_window = new QAction(Icon("window-new"), tr("New Window"));
    connect(m_new_window, SIGNAL(triggered(bool)), this, SLOT(NewWindow()));

    m_new_table = new QAction(Icon("document-new"), tr("New Table"));
    connect(m_new_table, SIGNAL(triggered(bool)), this, SLOT(NewTable()));

    m_load = new QAction(Icon("document-open"), tr("Open Project"));
    connect(m_load, SIGNAL(triggered(bool)), this, SLOT(OpenFile()));

    m_save = new QAction(Icon("document-save-all"), tr("Save Project"));
    connect(m_save, SIGNAL(triggered(bool)), this, SLOT(SaveProjectAction()));

    m_config = new QAction(Icon("configure"), tr("Settings"));
    connect(m_config, SIGNAL(triggered()), this, SLOT(SettingsDialog()));

    m_about = new QAction(Icon("help-about"), tr("Info"));
    connect(m_about, SIGNAL(triggered()), this, SLOT(about()));

    m_aboutqt = new QAction(Icon("help-about"), tr("About Qt"));
    connect(m_aboutqt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    m_close = new QAction(Icon("application-exit"), tr("Quit"));
    connect(m_close, SIGNAL(triggered()), SLOT(close()));

    m_main_toolbar = new QToolBar;
    m_main_toolbar->setObjectName(tr("main_toolbar"));
    m_main_toolbar->addAction(m_new_window);
    m_main_toolbar->addSeparator();
    m_main_toolbar->addAction(m_new_table);
    m_main_toolbar->addAction(m_load);
    m_main_toolbar->addAction(m_save);
    m_main_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    addToolBar(m_main_toolbar);

    /*m_model_toolbar = new QToolBar;
    m_model_toolbar->setObjectName(tr("model_toolbar"));
    m_model_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_model_toolbar->addAction(m_importmodel);
    m_model_toolbar->addAction(m_export);
    addToolBar(m_model_toolbar);*/

    m_system_toolbar = new QToolBar;
    m_system_toolbar->setObjectName(tr("system_toolbar"));

    m_system_toolbar->addSeparator();
    m_system_toolbar->addAction(m_config);
    m_system_toolbar->addAction(m_about);
    m_system_toolbar->addAction(m_aboutqt);
    m_system_toolbar->addAction(m_close);
    m_system_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    addToolBar(m_system_toolbar);

    ReadGeometry();
    LogFile();
    setActionEnabled(false);
    setStyleSheet("QSplitter::handle:vertical {"
                  "background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
                  "stop:0 #eee, stop:1 #ccc);"
                  "border: 1px solid #777;"
                  "height: 1px;"
                  "margin-top: 2px;"
                  "margin-bottom: 2px;"
                  "border-radius: 4px;"
                  "}");
    qApp->installEventFilter(this);
    connect(m_project_view, &QTreeView::doubleClicked, this, &SupraFitGui::TreeClicked);
    m_project_view->hide();
    //connect(m_project_view, &QTreeView::customContextMenuRequested, this, &SupraFitGui::ContextMenu);
    //connect(m_project_view, &QTreeView::doubleClicked, this, &SupraFitGui::TreeRemoveRequest);
}

SupraFitGui::~SupraFitGui()
{
    if (m_instance)
        delete m_instance;
}

void SupraFitGui::LoadFile(const QString& file)
{
    bool invalid_json = false;
    if (file.contains("json") || file.contains("jdat") || file.contains("suprafit")) {
        invalid_json = !LoadProject(file);
        if (!invalid_json)
            return;
    } else {
        ImportTable(file);
    }

    if (invalid_json)
        QMessageBox::warning(this, tr("Loading Datas."), tr("Sorry, but this doesn't contain any titration tables!"), QMessageBox::Ok | QMessageBox::Default);
}

void SupraFitGui::setActionEnabled(bool enabled)
{
    m_save->setEnabled(enabled);
}

bool SupraFitGui::SetData(const QJsonObject& object, const QString& file)
{
    for (int i = 0; i < m_project_list.size(); ++i)
        m_project_list[i]->hide();

    QPointer<MainWindow> window = new MainWindow;
    QWeakPointer<DataClass> data = window->SetData(object);
    if (!data)
        return false;

    m_project_view->show();

    QString name = data.data()->ProjectTitle();
    if (name.isEmpty() || name.isNull()) {
        name = file;
        data.data()->setProjectTitle(name);
    }
    window->setWindowFlags(Qt::Widget);

    m_layout->addWidget(window, 0, 1);
    connect(window, &MainWindow::ModelsChanged, this, [=]() {
        m_project_tree->layoutChanged();
    });

    m_project_list << window;

    m_data_list << data;
    m_project_tree->layoutChanged();
    setActionEnabled(true);
    return true;
}

void SupraFitGui::NewWindow()
{
    if (!m_data_list.size()) {
        QMessageBox question(QMessageBox::Question, tr("New Window"), tr("Do yout really want to open a new window?"), QMessageBox::Yes | QMessageBox::No, this);
        if (question.exec() == QMessageBox::No) {
            return;
        }
    }
    SupraFitGui* mainwindow = new SupraFitGui;
    mainwindow->show();
}

void SupraFitGui::NewTable()
{
    ImportData dialog;
    if (dialog.exec() == QDialog::Accepted) {
        SetData(dialog.getProject(), dialog.ProjectFile());
        }
}

void SupraFitGui::OpenFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir(), tr("Supported files (*.suprafit *.json *.jdat *.txt *.dat *.itc *.ITC *.dh *.DH);;Json File (*.json);;SupraFit Project File  (*.suprafit);;Table Files (*.dat *.txt *.itc *.ITC);;Origin Files(*.dh *.DH);;All files (*.*)"));
    if (filename.isEmpty())
        return;
    setLastDir(filename);
    LoadFile(filename);
    setActionEnabled(true);
}

void SupraFitGui::ImportTable(const QString& file)
{
    ImportData dialog(file, this);

    if (dialog.exec() == QDialog::Accepted) {
        SetData(dialog.getProject(), dialog.ProjectFile());
    }
}

bool SupraFitGui::LoadProject(const QString& filename)
{
    Waiter wait;
    QFileInfo info(filename);
    qApp->instance()->setProperty("projectpath", info.absoluteFilePath());
    qApp->instance()->setProperty("projectname", info.baseName());

    QJsonObject toplevel;

    if (JsonHandler::ReadJsonFile(toplevel, filename)) {

        QStringList keys = toplevel.keys();
        if (keys.contains("data")) {
            return SetData(toplevel, info.baseName());
        } else {
            bool exit = true;
            int index = 1;
            for (const QString& str : qAsConst(keys)) {
                QJsonObject object = toplevel[str].toObject();
                exit = exit && SetData(object, info.baseName() + "-" + QString::number(index));
                index++;
            }
            return exit;
        }
    }
    return false;
}

void SupraFitGui::SaveProjectAction()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("SupraFit Project File  (*.suprafit);;Json File (*.json);;All files (*.*)"));
    if (!str.isEmpty()) {

        QVector<QJsonObject> projects;
        for (int i = 0; i < m_project_list.size(); i++) {
            QPointer<MainWindow> project_widget = m_project_list[i];
            projects << project_widget->SaveProject();
        }
        if (projects.isEmpty())
            return;
        else if (projects.size() == 1) {
            JsonHandler::WriteJsonFile(projects.first(), str);
        } else {
            QJsonObject json;
            for (int i = 0; i < projects.size(); ++i)
                json["project_" + QString::number(i)] = projects[i];
            JsonHandler::WriteJsonFile(json, str);
        }
        setLastDir(str);
    }
}

void SupraFitGui::SettingsDialog()
{
    ConfigDialog dialog(m_opt_config, m_printlevel, m_logfile, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_opt_config = dialog.Config();
        //m_model_dataholder->setSettings(m_opt_config);
        m_printlevel = dialog.PrintLevel();
        m_logfile = dialog.LogFile();
        WriteSettings();
    }
}

void SupraFitGui::LogFile()
{
    if (m_logfile.isEmpty() && m_file.isOpen())
        m_file.close();
    else if (m_file.fileName() != m_logfile && m_file.isOpen()) {
        m_file.close();
        m_file.setFileName(m_logfile);
        if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;
    } else if (!m_file.isOpen()) {
        m_file.setFileName(m_logfile);
        if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;
    }
}

void SupraFitGui::WriteMessages(const QString& message, int priority)
{
    //     QTextStream stdout_stream(&m_stdout);
    //      stdout_stream << message << "\n";
    emit AppendPlainText(message);
    return;
    if (priority <= m_printlevel) {
        QTextStream fileout_stream(&m_file);
        fileout_stream << message << "\n";

        // QTimer::singleShot(0, m_logWidget, SLOT(appendPlainText(message)));
    }
}

void SupraFitGui::setWindowTitle()
{
    QMainWindow::setWindowTitle(QString("*" + qApp->instance()->property("projectname").toString() + "*"));
}

void SupraFitGui::MessageBox(const QString& str, int priority)
{
    if (priority == 0) {
        QMessageBox::critical(this, tr("Optimizer Error."), str, QMessageBox::Ok | QMessageBox::Default);
    } else if (priority == 1) {
        QMessageBox::warning(this, tr("Optimizer warning."), str, QMessageBox::Ok | QMessageBox::Default);
    }
}

void SupraFitGui::ReadSettings()
{
    QSettings _settings;
    _settings.beginGroup("main");
    m_logfile = _settings.value("logfile").toString();
    m_printlevel = _settings.value("printlevel", 3).toInt();
    for (const QString& str : qAsConst(m_properties))
        qApp->instance()->setProperty(qPrintable(str), _settings.value(str));
    _settings.endGroup();

    /* We start with the default values, which should replace all invalid QVariants eg. no yet set properties */
    if (qApp->instance()->property("ask_on_exit") == QVariant())
        qApp->instance()->setProperty("ask_on_exit", true);

    if (qApp->instance()->property("save_on_exit") == QVariant())
        qApp->instance()->setProperty("save_on_exit", true);

    if (qApp->instance()->property("tooltips") == QVariant()) {
        qApp->instance()->setProperty("tooltips", true);
        QTimer::singleShot(10, this, SLOT(FirstStart()));
    }

    if (qApp->instance()->property("chartanimation") == QVariant())
        qApp->instance()->setProperty("chartanimation", true);

    if (qApp->instance()->property("auto_confidence") == QVariant())
        qApp->instance()->setProperty("auto_confidence", true);

    if (qApp->instance()->property("series_confidence") == QVariant())
        qApp->instance()->setProperty("series_confidence", false);

    if (qApp->instance()->property("charttheme") == QVariant())
        qApp->instance()->setProperty("charttheme", 1);

    if (qApp->instance()->property("dirlevel") == QVariant())
        qApp->instance()->setProperty("dirlevel", 1);

    if (qApp->instance()->property("p_value") == QVariant())
        qApp->instance()->setProperty("p_value", 0.95);
}

void SupraFitGui::ReadGeometry()
{
    QSettings _settings;
    _settings.beginGroup("window");
    restoreGeometry(_settings.value("geometry").toByteArray());
    restoreState(_settings.value("state").toByteArray());
    _settings.endGroup();
}

void SupraFitGui::WriteSettings(bool ignore_window_state)
{
    QSettings _settings;

    _settings.beginGroup("main");
    _settings.setValue("logfile", m_logfile);
    _settings.setValue("printlevel", m_printlevel);

    QStringList properties;
    for (const QString& str : qAsConst(m_properties))
        _settings.setValue(str, qApp->instance()->property(qPrintable(str)));
    _settings.endGroup();

    if (!ignore_window_state) {
        _settings.beginGroup("window");
        _settings.setValue("geometry", saveGeometry());
        _settings.setValue("state", saveState());
        _settings.endGroup();
    }
}

void SupraFitGui::about()
{
    QMessageBox::about(this, tr("About this application"), SupraFit::aboutHtml());
}

void SupraFitGui::FirstStart()
{
    QString info;
    info += "<p>Welcome to SupraFit, a non-linear fitting tool for supramoleculare NMR titration experiments.< /p>";
    info += "<p>The main window of SupraFit is arranged via dock widgets:";
    info += "<ul><li>Workspace Widget</li><li>Chart Dock</li><li>Models Stack</li><li>Log Dock</li></ul></p>";
    info += "<p>Short information about them can be found as tooltips by hovering over the widget.</p>";
    info += "<p>All dock widgets can be dragged around using the mouse, hidden via toolbar or by clicking the <em>close button</em> on the dock widget.</p>";
    info += "<p>The log widget is hidden on first startup.</p>";
    info += "<p><strong>All</strong> tooltips can globally disabled in the config dialog.</p>";
    QMessageBox::about(this, tr("First Start Information"), info);
}

void SupraFitGui::closeEvent(QCloseEvent* event)
{
    if (qApp->instance()->property("ask_on_exit").toBool()) {
        QCheckBox* checkbox = new QCheckBox;
        checkbox->setText(tr("Don't ask this again!"));
        QMessageBox question(QMessageBox::Question, tr("About to close"), tr("Do yout really want to close this window?"), QMessageBox::Yes | QMessageBox::No, this);
        question.setCheckBox(checkbox);
        if (question.exec() == QMessageBox::No) {
            qApp->instance()->setProperty("ask_on_exit", !question.checkBox()->isChecked());
            event->ignore();
            return;
        }
        qApp->instance()->setProperty("ask_on_exit", !question.checkBox()->isChecked());
    }

    if (qApp->instance()->property("save_on_exit").toBool() && !qApp->instance()->property("projectpath").toString().isEmpty()) {
        QString filename = qApp->instance()->property("projectpath").toString();
        QFileInfo info(filename + ".autosave.suprafit");
        if (info.exists()) {
            int i = 1;
            QFileInfo info(filename + ".autosave_" + QString::number(i) + ".suprafit");
            while (info.exists()) {
                ++i;
                info = QFileInfo(filename + ".autosave" + QString::number(i) + ".suprafit");
            }
            filename = filename + ".autosave_" + QString::number(i) + ".suprafit";
        } else
            filename = filename + ".autosave.suprafit";
        // m_model_dataholder->SaveWorkspace(filename);
    }

    m_stdout.close();

    WriteSettings(false);

    QMainWindow::closeEvent(event);
}

bool SupraFitGui::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::ToolTip) {
        return !qApp->instance()->property("tooltips").toBool();
    } else
        return QMainWindow::eventFilter(obj, event);
}

void SupraFitGui::TreeClicked(const QModelIndex& index)
{
    for (int i = 0; i < m_project_list.size(); ++i)
        m_project_list[i]->hide();

    int widget = 0;
    int tab = -1;
    if (m_project_tree->parent(index).isValid()) {
        widget = m_project_tree->parent(index).row();
        tab = index.row();
    } else {
        widget = index.row();
    }
    m_project_list[widget]->show();
    m_project_list[widget]->setCurrentTab(tab + 1);
}

void SupraFitGui::TreeRemoveRequest(const QModelIndex& index)
{

    int widget = 0;
    int tab = -1;
    if (m_project_tree->parent(index).isValid()) {
        widget = m_project_tree->parent(index).row();
        tab = index.row();
        m_project_list[widget]->RemoveTab(tab + 1);
    } else {
        widget = index.row();
        MainWindow* mainwindow = m_project_list.takeAt(widget);
        delete mainwindow;
    }

    UpdateTreeView(true);
}

void SupraFitGui::UpdateTreeView(bool regenerate)
{
    if (!regenerate) {
        m_project_tree->layoutChanged();
        return;
    }

    delete m_project_tree;
    ProjectTree* project_tree = new ProjectTree(&m_project_list);
    m_project_view->setModel(project_tree);
    if (m_project_tree)
        delete m_project_tree;
    m_project_tree = project_tree;

    if (m_project_list.size())
        m_project_list.first()->show();
}

void SupraFitGui::AddMetaModel(const QModelIndex& index)
{
    QPointer<DataClass> data = static_cast<DataClass*>(index.internalPointer());

    if (!m_meta_model) {
        MainWindow* window = new MainWindow;

        QWeakPointer<MetaModel> model = qobject_cast<MetaModel*>(window->CreateMetaModel());
        if (!model)
            return;

        bool is_model = qobject_cast<AbstractModel*>(data);
        if (!is_model)
            return;

        m_layout->addWidget(window, 0, 1);

        connect(window, &MainWindow::ModelsChanged, this, [=]() {
            m_project_tree->layoutChanged();
        });

        m_project_list << window;

        m_data_list << model;
        m_project_tree->layoutChanged();
        setActionEnabled(true);
        m_meta_model = model;
    }
    m_meta_model.data()->addModel(qobject_cast<AbstractModel*>(data));
}
