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
#include <QtCore/QMimeData>
#include <QtCore/QSettings>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>
#include <QtCore/QWeakPointer>

#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGraphicsBlurEffect>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QSplashScreen>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedWidget>
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
            data = qobject_cast<AbstractModel*>(dataclass)->Name();
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

QMimeData* ProjectTree::mimeData(const QModelIndexList& indexes) const
{
    ModelMime* mimeData = new ModelMime();

    QString data;

    for (const QModelIndex& index : qAsConst(indexes)) {
        if (index.isValid()) {
            if (parent(index).isValid()) {
                data = "Model: " + QString::number(parent(index).row()) + "-" + QString::number(index.row());
                mimeData->setModel(true);
            } else {
                data = "Data: " + QString::number(index.row());
            }
            mimeData->setData(static_cast<DataClass*>(index.internalPointer()));
            mimeData->setModelIndex(index);
        }
    }
    mimeData->setText(data);
    return mimeData;
}

bool ProjectTree::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& index) const
{
    Q_UNUSED(action)

    QString string = data->text();
    // const ModelMime* d = qobject_cast<const ModelMime*>(data);

    if (row == -1 && column == -1)
        return true; //FIXME stranger things are going on here

    if (string.contains("Data") && !parent(index).isValid())
        return true;
    else if ((string.contains("Data") && parent(index).isValid()))
        return false;
    else
        return true;
}

bool ProjectTree::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& index)
{
    Q_UNUSED(action)

    QString string = data->text();
    qDebug() << string;
    if (string.contains("file:///")) {
        QStringList list = string.split("\n");
        for (QString str : list) {
            if (!str.isEmpty() && !str.isNull())
                emit LoadFile(str.remove("file://"));
        }
        return true;
    }
    if (row == -1 && column == -1 && !index.isValid()) {
        const ModelMime* d = qobject_cast<const ModelMime*>(data);
        emit AddMetaModel(d->Index(), -1);
        return true;
    }
    if (index.isValid() && !parent(index).isValid()) {
        int r = index.row();
        const ModelMime* d = qobject_cast<const ModelMime*>(data);

        if ((*m_project_list)[r]->isMetaModel() && index.isValid()) {
            if (d->Index().parent().isValid())
                emit AddMetaModel(d->Index(), r);
            else
                emit UiMessage(tr("It doesn't make sense to add whole project to a meta model.\nTry one of the models within this project."));
        } else if (index.isValid()) {
            if (!d->Index().parent().isValid())
                emit CopySystemParameter(d->Index(), r);
            else
                emit UiMessage(tr("Nothing to tell"));
        }
        return true;
    } else if (index.isValid() && parent(index).isValid()) {
        const ModelMime* d = qobject_cast<const ModelMime*>(data);
        emit CopyModel(d->Index(), parent(index).row(), index.row());
    } else
        return true;
    return true;
}

SupraFitGui::SupraFitGui()
{
    m_instance = new Instance;
    Instance::setInstance(m_instance);

    m_splash = new QSplashScreen(this, QPixmap(":/misc/logo_small.png"));

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

    action = new QAction("Save", m_project_view);
    m_project_view->addAction(action);
    connect(action, &QAction::triggered, action, [this]() {
        SaveData(m_project_view->currentIndex());
    });

    action = new QAction("Delete", m_project_view);
    connect(action, &QAction::triggered, action, [this]() {
        TreeRemoveRequest(m_project_view->currentIndex());
    });

    m_project_view->addAction(action);

    m_project_tree = new ProjectTree(&m_project_list);
    connect(m_project_tree, &ProjectTree::AddMetaModel, this, &SupraFitGui::AddMetaModel);
    connect(m_project_tree, &ProjectTree::CopySystemParameter, this, &SupraFitGui::CopySystemParameter);
    connect(m_project_tree, &ProjectTree::CopyModel, this, &SupraFitGui::CopyModel);
    connect(m_project_tree, &ProjectTree::LoadFile, this, &SupraFitGui::LoadFile);

    m_project_view->setModel(m_project_tree);
    m_project_view->setDragEnabled(true);
    m_project_view->setAcceptDrops(true);
    m_project_view->setDropIndicatorShown(true);
    m_project_view->setDragDropMode(QAbstractItemView::DragDrop);

    QLabel* logo = new QLabel;
    logo->setPixmap(QPixmap(":/misc/logo_small.png"));
    m_stack_widget = new QStackedWidget;
    m_stack_widget->addWidget(logo);

    m_filename_line = new QLineEdit;
    m_filename_line->setClearButtonEnabled(true);
    connect(m_filename_line, &QLineEdit::textChanged, this, [this](const QString& str) { this->m_supr_file = str; });
    QWidget* widget = new QWidget;
    QGridLayout* layout = new QGridLayout;

    layout->addWidget(new QLabel(tr("Filename:")), 0, 0);
    layout->addWidget(m_filename_line, 0, 1);
    layout->addWidget(m_project_view, 1, 0, 1, 2);
    widget->setLayout(layout);

    m_mainsplitter = new QSplitter(Qt::Horizontal);
    m_mainsplitter->addWidget(widget);
    m_mainsplitter->addWidget(m_stack_widget);

    setCentralWidget(m_mainsplitter);

    ReadSettings();

    m_new_window = new QAction(Icon("window-new"), tr("New Window"));
    connect(m_new_window, SIGNAL(triggered(bool)), this, SLOT(NewWindow()));

    m_new_table = new QAction(Icon("document-new"), tr("New Table"));
    connect(m_new_table, SIGNAL(triggered(bool)), this, SLOT(NewTable()));
    m_new_table->setShortcut(QKeySequence::New);

    m_load = new QAction(Icon("document-open"), tr("Open Project"));
    connect(m_load, SIGNAL(triggered(bool)), this, SLOT(OpenFile()));
    m_load->setShortcut(QKeySequence::Open);

    m_save = new QAction(Icon("document-save"), tr("&Save Project"));
    m_save->setShortcuts(QKeySequence::Save);
    connect(m_save, SIGNAL(triggered(bool)), this, SLOT(SaveProjectAction()));

    m_save_as = new QAction(Icon("document-save-as"), tr("Save Project &As"));
    connect(m_save_as, SIGNAL(triggered(bool)), this, SLOT(SaveAsProjectAction()));
    m_save_as->setShortcut(QKeySequence::SaveAs);

    m_config = new QAction(Icon("configure"), tr("Settings"));
    connect(m_config, SIGNAL(triggered()), this, SLOT(SettingsDialog()));
    m_config->setShortcut(QKeySequence::Preferences);

    m_about = new QAction(QIcon(":/misc/suprafit.png"), tr("Info"));
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
    m_main_toolbar->addAction(m_save_as);
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

    //m_mainsplitter->hide();
    setWindowIcon(QIcon(":/misc/suprafit.png"));
}

SupraFitGui::~SupraFitGui()
{
    if (m_instance)
        delete m_instance;
}

void SupraFitGui::LoadFile(const QString& file)
{
    if (file.contains("json") || file.contains("suprafit")) {
        QTimer::singleShot(0, m_splash, &QSplashScreen::show);
        m_mainsplitter->setGraphicsEffect(new QGraphicsBlurEffect());
    }
    bool invalid_json = false;
    if (file.contains("json") || file.contains("jdat") || file.contains("suprafit")) {
        invalid_json = !LoadProject(file);
        if (!invalid_json) {
            m_mainsplitter->setGraphicsEffect(NULL);
            QTimer::singleShot(1, m_splash, &QSplashScreen::close);
            return;
        }
    } else {
        ImportTable(file);
    }
    m_mainsplitter->setGraphicsEffect(NULL);

    QTimer::singleShot(1, m_splash, &QSplashScreen::close);

    if (invalid_json)
        QMessageBox::warning(this, tr("Loading Datas."), tr("Sorry, but this doesn't contain any titration tables!"), QMessageBox::Ok | QMessageBox::Default);
}

void SupraFitGui::setActionEnabled(bool enabled)
{
    m_save->setEnabled(enabled);
}

bool SupraFitGui::SetData(const QJsonObject& object, const QString& file)
{
    QString uuid = object["data"].toObject()["uuid"].toString();
    if (m_hashed_data.keys().contains(uuid)) {
        QString name = m_hashed_data[uuid].data()->ProjectTitle();
        QMessageBox question(QMessageBox::Question, tr("Data already open"), tr("The current data has already been opened. At least the UUID\n%1\nwith the project name\n%2\nexist. Continue?").arg(uuid).arg(name), QMessageBox::Yes | QMessageBox::No, this);
        if (question.exec() == QMessageBox::No) {
            m_mainsplitter->show();
            return true;
        }
    }

    /* there is some bad code in here
     * let the stack widget take ownership of the widget before anything happens
     * in it, prevents suprafit to crash while loading to many (2!) models in one mainwindow
     * lets fix it sometimes
     */
    QPointer<MainWindow> window = new MainWindow(m_stack_widget);
    if (object["data"].toObject().contains("model")) {
        m_cached_meta << object;
        return true;
    }

    QWeakPointer<DataClass> data = window->SetData(object);
    if (!data)
        return false;
    m_hashed_wrapper.insert(data.data()->UUID(), window->getChartWrapper());

    QString name = data.data()->ProjectTitle();
    if (name.isEmpty() || name.isNull()) {
        name = file;
        data.data()->setProjectTitle(name);
    }

    m_stack_widget->addWidget(window);
    m_stack_widget->setCurrentWidget(window);

    connect(window, &MainWindow::ModelsChanged, this, [=]() {
        m_project_tree->layoutChanged();
    });

    m_project_list.insert(m_project_list.size() - m_meta_models.size(), window);
    m_data_list.insert(m_data_list.size() - m_meta_models.size(), data);

    m_hashed_data[data.data()->UUID()] = data;
    m_project_tree->layoutChanged();
    setActionEnabled(true);
    return true;
}

void SupraFitGui::AddMetaModel(const QModelIndex& index, int position)
{
    QPointer<DataClass> data = static_cast<DataClass*>(index.internalPointer());
    if (position == -1) {
        MainWindow* window = new MainWindow;

        QWeakPointer<MetaModel> model = qobject_cast<MetaModel*>(window->CreateMetaModel(m_hashed_wrapper[data->UUID()]));
        if (!model) {
            delete window;
            return;
        }

        bool is_model = qobject_cast<AbstractModel*>(data);
        if (!is_model) {
            delete window;
            return;
        }

        m_stack_widget->addWidget(window);

        connect(window, &MainWindow::ModelsChanged, this, [=]() {
            m_project_tree->layoutChanged();
        });
        m_project_list.append(window);

        m_data_list.append(model);
        m_project_tree->layoutChanged();
        setActionEnabled(true);
        model.data()->addModel(qobject_cast<AbstractModel*>(data));
        m_meta_models.append(model);
    } else if ((position - (m_data_list.size() - m_meta_models.size())) < m_meta_models.size()) {
        m_meta_models[position - (m_data_list.size() - m_meta_models.size())].data()->addModel(qobject_cast<AbstractModel*>(data));

        QWeakPointer<ChartWrapper> wrapper = m_project_list[position]->getChartWrapper();
        wrapper.data()->addWrapper(m_hashed_wrapper[data->UUID()]);
    }
}

void SupraFitGui::LoadMetaModels()
{
    for (const QJsonObject& object : m_cached_meta) {
        QJsonObject uuids = object["data"].toObject()["uuids"].toObject();
        int size = object["data"].toObject()["size"].toInt();

        QPointer<MainWindow> window = new MainWindow;

        QWeakPointer<MetaModel> model = qobject_cast<MetaModel*>(window->CreateMetaModel());
        if (!model)
            continue;
        QWeakPointer<ChartWrapper> wrapper = window->getChartWrapper();

        for (int i = 0; i < size; ++i) {

            QWeakPointer<DataClass> data = m_hashed_data[uuids[QString::number(i)].toString()];

            if (!data) {
                qDebug() << "found no data set for meta model, skipping";
                continue;
            }
            QJsonObject rawmodel = object["data"].toObject()["raw"].toObject()[QString::number(i)].toObject();

            QSharedPointer<AbstractModel> t = CreateModel(SupraFit::Model(rawmodel["model"].toInt()), data);
            wrapper.data()->addWrapper(m_hashed_wrapper[uuids[QString::number(i)].toString()]);

            t->ImportModel(rawmodel);
            model.data()->addModel(t.data());
        }
        model.data()->ImportModel(object["data"].toObject());

        m_stack_widget->addWidget(window);

        connect(window, &MainWindow::ModelsChanged, this, [=]() {
            m_project_tree->layoutChanged();
        });

        m_project_list << window;

        m_data_list << model;
        m_project_tree->layoutChanged();
        setActionEnabled(true);
        m_meta_models << model;

        m_stack_widget->addWidget(window);
        m_stack_widget->setCurrentWidget(window);
    }

    m_cached_meta.clear();
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
        m_mainsplitter->show();
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
        m_mainsplitter->show();
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
        if (m_supr_file.isEmpty() || m_supr_file.isNull()) {
            m_supr_file = filename;
            m_filename_line->setText(m_supr_file);
        }
        QStringList keys = toplevel.keys();
        if (keys.contains("data")) {
            return SetData(toplevel, info.baseName());
        } else {
            bool exit = true;
            int index = 1;
            for (const QString& str : qAsConst(keys)) {
                QApplication::processEvents();
                QJsonObject object = toplevel[str].toObject();
                exit = exit && SetData(object, info.baseName() + "-" + QString::number(index));
                index++;
            }
            LoadMetaModels();
            return exit;
        }
    }
    return false;
}

void SupraFitGui::SaveProjectAction()
{

    if (m_supr_file.isEmpty() || m_supr_file.isNull()) {
        QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("SupraFit Project File  (*.suprafit);;Json File (*.json)"));
        if (str.isEmpty())
            return;
        m_supr_file = str;
        m_filename_line->setText(m_supr_file);
    }

    QVector<QJsonObject> projects;
    for (int i = 0; i < m_project_list.size(); i++) {
        QPointer<MainWindow> project_widget = m_project_list[i];
        projects << project_widget->SaveProject();
    }
    if (projects.isEmpty())
        return;
    else if (projects.size() == 1) {
        JsonHandler::WriteJsonFile(projects.first(), m_supr_file);
    } else {
        QJsonObject json;
        for (int i = 0; i < projects.size(); ++i)
            json["project_" + QString::number(i)] = projects[i];
        JsonHandler::WriteJsonFile(json, m_supr_file);
    }
    setLastDir(m_supr_file);
}

void SupraFitGui::SaveAsProjectAction()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("SupraFit Project File  (*.suprafit);;Json File (*.json)"));
    if (!str.isEmpty()) {
        m_supr_file = str;
        m_filename_line->setText(m_supr_file);

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

    if (qApp->instance()->property("threads") == QVariant())
        qApp->instance()->setProperty("threads", QThread::idealThreadCount());

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
    int widget = 0;
    int tab = -1;
    if (m_project_tree->parent(index).isValid()) {
        widget = m_project_tree->parent(index).row();
        tab = index.row();
    } else {
        if (index.internalPointer() != NULL)
            widget = index.row();
        else
            return;
    }
    m_stack_widget->setCurrentWidget(m_project_list[widget]);
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
        m_stack_widget->removeWidget(mainwindow);
        m_data_list.remove(widget);
        m_hashed_data.remove(mainwindow->Data()->UUID());
        delete mainwindow;
    }
    for (int i = m_meta_models.size() - 1; i >= 0; --i)
        if (!m_meta_models[i])
            m_meta_models.remove(i);
    if (m_data_list.size() == 0) {
        m_supr_file.clear();
        m_filename_line->clear();
    }
    UpdateTreeView(true);
}

void SupraFitGui::UpdateTreeView(bool regenerate)
{
    if (!regenerate) {
        m_project_tree->layoutChanged();
        return;
    }

    m_project_tree->disconnect();

    ProjectTree* project_tree = new ProjectTree(&m_project_list);
    m_project_view->setModel(project_tree);

    m_project_tree = project_tree;
    connect(m_project_tree, &ProjectTree::AddMetaModel, this, &SupraFitGui::AddMetaModel);
    connect(m_project_tree, &ProjectTree::CopySystemParameter, this, &SupraFitGui::CopySystemParameter);
    connect(m_project_tree, &ProjectTree::CopyModel, this, &SupraFitGui::CopyModel);
    connect(m_project_tree, &ProjectTree::LoadFile, this, &SupraFitGui::LoadFile);

    if (m_project_list.size())
        m_project_list.first()->show();
}

void SupraFitGui::SaveData(const QModelIndex& index)
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("SupraFit Project File  (*.suprafit);;Json File (*.json)"));
    if (str.isEmpty() || str.isNull())
        return;

    QJsonObject object;
    int widget = 0;
    int tab = -1;
    if (m_project_tree->parent(index).isValid()) {
        widget = m_project_tree->parent(index).row();
        tab = index.row();
        object = m_project_list[widget]->SaveModel(tab + 1);
    } else {
        widget = index.row();
        object = m_project_list[widget]->SaveProject();
    }
    JsonHandler::WriteJsonFile(object, str);
    setLastDir(str);
}

void SupraFitGui::CopySystemParameter(const QModelIndex& source, int position)
{
    if (position >= m_data_list.size())
        return;

    QPointer<DataClass> data = static_cast<DataClass*>(source.internalPointer());
    for (int i = 0; i < m_meta_models.size(); ++i) {
        if (m_meta_models[i].data()->UUID() == data.data()->UUID())
            return;
    }
    m_data_list[position].data()->OverrideSystemParameter(data->SysPar());
}

void SupraFitGui::CopyModel(const QModelIndex& source, int data, int model)
{
    QPointer<AbstractModel> m = static_cast<AbstractModel*>(source.internalPointer());
    if (data < m_project_list.size()) {
        m_project_list[data]->Model(model)->ImportModel(m->ExportModel(true, true));
    } else
        qDebug() << "not found" << data << model;
}
