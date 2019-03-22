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
#include <QtCore/QJsonDocument>
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

#include <QtCharts/QChartView>

#include <stdio.h>

#include "suprafitgui.h"

void ProjectTree::UpdateStructure()
{

    // m_uuids.clear();
    // m_ptr_uuids.clear();

    for (int i = 0; i < m_data_list->size(); ++i) {
        QString uuid = (*m_data_list)[i].data()->UUID();

        if (!m_uuids.contains(uuid)) {
            m_uuids << uuid;
            m_ptr_uuids << &m_uuids.last();
        }

        for (int j = 0; j < (*m_data_list)[i].data()->ChildrenSize(); ++j) {
            QString sub_uuid = uuid + "|" + qobject_cast<AbstractModel*>((*m_data_list)[i].data()->Children(j))->ModelUUID();

            if (!m_uuids.contains(sub_uuid)) {
                m_uuids << sub_uuid;
                m_ptr_uuids << &m_uuids.last();
            }
        }
    }
    layoutChanged();
}

QString ProjectTree::UUID(const QModelIndex& index) const
{
    int i = m_ptr_uuids.indexOf(index.internalPointer());
    if (i == -1)
        return QString();

    return m_uuids[i];
}

int ProjectTree::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

int ProjectTree::rowCount(const QModelIndex& p) const
{
    int count = m_data_list->size();
    if (p.isValid()) {

        QString uuid = UUID(p);
        if (uuid.size() == 77) // Model Element
        {
            count = 0;

        } else if (uuid.size() == 38) // DataClass Element
        {
            count = (*m_data_list)[p.row()].data()->ChildrenSize();
        } else
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

        if (parent(index).isValid()) // Model Element
        {
            data = qobject_cast<AbstractModel*>((*m_data_list)[parent(index).row()].data()->Children(index.row()))->Name();

        } else // DataClass Element
        {
            data = (*m_data_list)[index.row()].data()->ProjectTitle();
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

        if (row == -1) {
            return index;
        }

        if (row < m_data_list->size()) {
            QString uuid = (*m_data_list)[row].data()->UUID();
            if (m_uuids.indexOf(uuid) == -1)
                return index;
            index = createIndex(row, column, m_ptr_uuids[m_uuids.indexOf(uuid)]);
        }
    } else {
        if (parent.row() < m_data_list->size()) {
            if (row < (*m_data_list)[parent.row()].data()->ChildrenSize()) {
                QString uuid = (*m_data_list)[parent.row()].data()->UUID();
                QString sub_uuid = uuid + "|" + qobject_cast<AbstractModel*>((*m_data_list)[parent.row()].data()->Children(row))->ModelUUID();
                index = createIndex(row, column, m_ptr_uuids[m_uuids.indexOf(sub_uuid)]);
            }
        }
    }

    return index;
}

QModelIndex ProjectTree::parent(const QModelIndex& child) const
{
    QModelIndex index;

    if (!child.isValid())
        return index;

    QString uuid = UUID(child);

    if (!(uuid.size() == 77 || uuid.size() == 38))
        return index;

    QStringList uuids = uuid.split("|");
    if (uuids.size() == 2) {
        for (int i = 0; i < m_data_list->size(); ++i) {
            if ((*m_data_list)[i].data()->UUID() == uuids[0]) {
                for (int j = 0; j < (*m_data_list)[i].data()->ChildrenSize(); ++j) {
                    if (qobject_cast<AbstractModel*>((*m_data_list)[i].data()->Children(j))->ModelUUID() == uuids[1]) {
                        index = createIndex(i, 0, m_ptr_uuids[m_uuids.indexOf(uuids[0])]);
                    }
                }
            }
        }
    } else
        return index;

    return index;
}

QMimeData* ProjectTree::mimeData(const QModelIndexList& indexes) const
{
    ModelMime* mimeData = new ModelMime();

    QString data;
    QJsonObject object;
    for (const QModelIndex& index : qAsConst(indexes)) {
        if (index.isValid()) {
            QJsonObject d, top;
            if (parent(index).isValid()) {
                data = "Model: " + QString::number(parent(index).row()) + "-" + QString::number(index.row());
                mimeData->setModel(true);
                QStringList uuids = UUID(index).split("|");
                if (uuids.size() == 2) {
                    mimeData->setDataUUID(uuids[0]);
                    mimeData->setModelUUID(uuids[1]);
                    for (int i = 0; i < m_data_list->size(); ++i) {
                        if (mimeData->DataUUID() == (*m_data_list)[i].data()->UUID()) {
                            for (int j = 0; j < (*m_data_list)[i].data()->ChildrenSize(); ++j) {
                                if (qobject_cast<AbstractModel*>((*m_data_list)[i].data()->Children(j))->ModelUUID() == mimeData->ModelUUID())
                                    top = qobject_cast<AbstractModel*>((*m_data_list)[i].data()->Children(j))->ExportModel(true, false);
                            }
                        }
                    }
                }
            } else {
                data = "Data: " + QString::number(index.row());
                mimeData->setDataUUID(UUID(index));
                for (int i = 0; i < m_data_list->size(); ++i) {
                    if (mimeData->DataUUID() == (*m_data_list)[i].data()->UUID()) {
                        d = (*m_data_list)[i].data()->ExportData();
                        top = (*m_data_list)[i].data()->ExportChildren(true, false);
                    }
                }
                top["data"] = d;
            }
            mimeData->setModelIndex(index);
            QJsonDocument document = QJsonDocument(top);
            mimeData->setData("application/x-suprafitmodel", document.toBinaryData());
        }
    }

    mimeData->setInstance(m_instance);
    mimeData->setText(data);
    return mimeData;
}

bool ProjectTree::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& index) const
{
    Q_UNUSED(action)

    QString string = data->text();
    QString sprmodel = data->data("application/x-suprafitmodel");

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
// TODO We want to synchronise these behaviour to get a instant feedback if the drag and drop action is supported
#warning before release resolve the this
bool ProjectTree::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& index)
{
    Q_UNUSED(action)

    QString string = data->text();

    if (string.contains("file:///")) {
        QStringList list = string.split("\n");
        for (QString str : list) {
            if (!str.isEmpty() && !str.isNull())
#ifdef _WIN32
                emit LoadFile(str.remove("file:///"));
#else
                emit LoadFile(str.remove("file://"));
#endif
        }
        return true;
    }

    if (!qobject_cast<const ModelMime*>(data)) {
        /* This could be from suprafit but a different main instance */

        QByteArray sprmodel = data->data("application/x-suprafitmodel");
        QJsonDocument doc = QJsonDocument::fromBinaryData(sprmodel);
        QJsonObject mod = doc.object();
        if (mod["model"].toInt() == 0) {
            if (!doc.isEmpty() && (!string.contains("Model") || !string.contains("Data"))) {
                emit LoadJsonObject(mod);
                return true;
            }
            return false;
        } else {
            qDebug() << row << index.row() << parent(index).row();
            if (index.isValid() && parent(index).isValid()) {
                emit CopyModel(mod, parent(index).row(), index.row());
                return true;
            }
        }
        return false;
    }

    const ModelMime* d = qobject_cast<const ModelMime*>(data);

    QByteArray sprmodel = data->data("application/x-suprafitmodel");
    QJsonDocument doc = QJsonDocument::fromBinaryData(sprmodel);

    if (d->Instance() != m_instance) {
        if (!doc.isEmpty() && string.contains("Data")) {
            emit LoadJsonObject(doc.object());
            return true;
        }
    }
    if (string.isEmpty() || string.isNull())
        return false;

    if (row == -1 && column == -1 && !index.isValid()) {
        const ModelMime* d = qobject_cast<const ModelMime*>(data);
        emit AddMetaModel(d->Index(), -1);
        return true;
    }
    if (index.isValid() && !parent(index).isValid()) {
        int r = index.row();
        const ModelMime* d = qobject_cast<const ModelMime*>(data);

        if (qobject_cast<MetaModel*>((*m_data_list)[r].data()) && index.isValid()) {
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
        QByteArray sprmodel = d->data("application/x-suprafitmodel");
        QJsonDocument doc = QJsonDocument::fromBinaryData(sprmodel);
        QJsonObject mod = doc.object();
        emit CopyModel(mod, parent(index).row(), index.row());
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

    m_project_tree = new ProjectTree(&m_data_list, this);
    connect(m_project_tree, &ProjectTree::AddMetaModel, this, &SupraFitGui::AddMetaModel);
    connect(m_project_tree, &ProjectTree::CopySystemParameter, this, &SupraFitGui::CopySystemParameter);
    connect(m_project_tree, &ProjectTree::CopyModel, this, &SupraFitGui::CopyModel);
    connect(m_project_tree, &ProjectTree::LoadFile, this, &SupraFitGui::LoadFile);
    connect(m_project_tree, &ProjectTree::LoadJsonObject, this, &SupraFitGui::LoadJson);

    m_project_view->setModel(m_project_tree);
    m_project_view->setDragEnabled(true);
    m_project_view->setAcceptDrops(true);
    m_project_view->setDropIndicatorShown(true);
    m_project_view->setDragDropMode(QAbstractItemView::DragDrop);

    m_blank_widget = new QWidget;

    m_recent_documents = new QListWidget;
    connect(m_recent_documents, &QListWidget::itemDoubleClicked, m_recent_documents, [this](const QListWidgetItem *item){
       LoadFile(item->text());
    });

    QLabel* logo = new QLabel;
    logo->setPixmap(QPixmap(":/misc/logo_small.png"));

    m_clear_recent = new QPushButton(tr("Clear list"));
    connect(m_clear_recent, &QPushButton::clicked, m_clear_recent, [this]()
    {
        qApp->instance()->setProperty("recent", QStringList());
        this->UpdateRecentList();
    });
    m_clear_recent->setFlat(true);
    m_clear_recent->setIcon(Icon("edit-clear-history"));

    QGridLayout *blank_layout = new QGridLayout;
    blank_layout->addWidget(logo, 0, 0, 2, 1);
    blank_layout->addWidget(new QLabel(tr("<h3>Recent documents</h3>")), 0, 1);
    blank_layout->addWidget(m_clear_recent, 0, 2);
    blank_layout->addWidget(m_recent_documents, 1, 1, 1, 3);
    m_blank_widget->setLayout(blank_layout);

    m_stack_widget = new QStackedWidget;
    m_stack_widget->addWidget(m_blank_widget);
    m_stack_widget->setObjectName("project_mainwindow");

    m_filename_line = new QLineEdit;
    m_filename_line->setClearButtonEnabled(true);
    connect(m_filename_line, &QLineEdit::textChanged, this, [this](const QString& str) { this->m_supr_file = str; setWindowTitle(); });
    QWidget* widget = new QWidget;
    widget->setObjectName("project_list");

    m_export_plain = new QPushButton(tr("Raw"));
    m_export_plain->setFlat(true);
    m_export_plain->setMaximumWidth(75);
    connect(m_export_plain, &QPushButton::clicked, this, &SupraFitGui::ExportAllPlain);

    m_export_suprafit = new QPushButton(tr("SupraFit"));
    m_export_suprafit->setFlat(true);
    m_export_suprafit->setMaximumWidth(75);
    connect(m_export_suprafit, &QPushButton::clicked, this, &SupraFitGui::ExportAllSupraFit);

    m_add_scatter = new DropButton(tr("Add Rand"));
    m_add_scatter->setFlat(true);
    m_add_scatter->setMaximumWidth(75);
    connect(m_add_scatter, &QPushButton::clicked, this, qOverload<>(&SupraFitGui::AddScatter));
    connect(m_add_scatter, &DropButton::DataDropped, this, [this](const QJsonObject& data) {
        QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("SupraFit Project File  (*.suprafit);;Json File (*.json)"));
        if (!str.isEmpty()) {
            JsonHandler::WriteJsonFile(data, str);
        }
        AddScatter(data);
    });

    m_close_all = new QPushButton(tr("Close"));
    m_close_all->setFlat(true);
    m_close_all->setIcon(Icon("document-close"));
    connect(m_close_all, &QPushButton::clicked, this, &SupraFitGui::CloseProjects);

    QHBoxLayout* tools = new QHBoxLayout;
    tools->addWidget(m_export_plain);
    tools->addWidget(m_export_suprafit);
    tools->addWidget(m_add_scatter);
    tools->addWidget(m_close_all);

    QGridLayout* layout = new QGridLayout;

    layout->addWidget(new QLabel(tr("Filename:")), 0, 0);
    layout->addWidget(m_filename_line, 0, 1);
    layout->addLayout(tools, 1, 0, 1, 2);
    layout->addWidget(m_project_view, 2, 0, 1, 2);
    widget->setLayout(layout);

    m_mainsplitter = new QSplitter(Qt::Horizontal);
    m_mainsplitter->setObjectName(tr("project_splitter"));
    m_mainsplitter->addWidget(widget);
    m_mainsplitter->addWidget(m_stack_widget);

    setCentralWidget(m_mainsplitter);

    ReadSettings();

    m_new_window = new QAction(Icon("window-new"), tr("New Window"), this);
    connect(m_new_window, SIGNAL(triggered(bool)), this, SLOT(NewWindow()));

    m_new_table = new QAction(Icon("document-new"), tr("New Table"), this);
    connect(m_new_table, SIGNAL(triggered(bool)), this, SLOT(NewTable()));
    m_new_table->setShortcut(QKeySequence::New);

    m_load = new QAction(Icon("document-open"), tr("Open Project"), this);
    connect(m_load, SIGNAL(triggered(bool)), this, SLOT(OpenFile()));
    m_load->setShortcut(QKeySequence::Open);

    m_save = new QAction(Icon("document-save"), tr("&Save Project"), this);
    m_save->setShortcuts(QKeySequence::Save);
    connect(m_save, SIGNAL(triggered(bool)), this, SLOT(SaveProjectAction()));

    m_save_as = new QAction(Icon("document-save-as"), tr("Save Project &As"), this);
    connect(m_save_as, SIGNAL(triggered(bool)), this, SLOT(SaveAsProjectAction()));
    m_save_as->setShortcut(QKeySequence::SaveAs);

    m_config = new QAction(Icon("configure"), tr("Settings"), this);
    connect(m_config, SIGNAL(triggered()), this, SLOT(SettingsDialog()));
    m_config->setShortcut(QKeySequence::Preferences);

    m_message = new QAction(Icon("help-hint"), tr("Messages"), this);
    connect(m_message, &QAction::triggered, this, &SupraFitGui::Messages);

    m_about = new QAction(QIcon(":/misc/suprafit.png"), tr("Info"), this);
    connect(m_about, SIGNAL(triggered()), this, SLOT(about()));

    m_license = new QAction(Icon("license"), tr("License Info"), this);
    connect(m_license, SIGNAL(triggered()), this, SLOT(LicenseInfo()));

    m_aboutqt = new QAction(Icon("help-about"), tr("About Qt"), this);
    connect(m_aboutqt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    m_close = new QAction(Icon("application-exit"), tr("Quit"), this);
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

    m_system_toolbar = new QToolBar;
    m_system_toolbar->setObjectName(tr("system_toolbar"));

    m_system_toolbar->addSeparator();
    m_system_toolbar->addAction(m_config);
    m_system_toolbar->addAction(m_message);
    m_system_toolbar->addAction(m_about);
    m_system_toolbar->addAction(m_license);
    m_system_toolbar->addAction(m_aboutqt);
    m_system_toolbar->addAction(m_close);
    m_system_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    addToolBar(m_system_toolbar);

    ReadGeometry();

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
    connect(m_project_view, &QTreeView::doubleClicked, this, &SupraFitGui::TreeDoubleClicked);
    connect(m_project_view, &QTreeView::clicked, this, &SupraFitGui::TreeClicked);

    //m_mainsplitter->hide();
    setWindowIcon(QIcon(":/misc/suprafit.png"));
    UpdateRecentList();
}

SupraFitGui::~SupraFitGui()
{
    if (m_instance)
        delete m_instance;
}

// Will someday be improved
/*
QVector<QJsonObject> SupraFitGui::ProjectFromFile(const QString& file)
{
    if (file.contains("json") || file.contains("suprafit")) {
        //QTimer::singleShot(0, m_splash, &QSplashScreen::show);
        //m_mainsplitter->setGraphicsEffect(new QGraphicsBlurEffect());
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

QVector<QJsonObject> SupraFitGui::ProjectFromFiles(const QStringList& files)
{
    QVector<QJsonObject> projects;
    for(const QString &str : files)
        projects << ProjectFromFile(str);

    return projects;
}
*/
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

void SupraFitGui::UpdateRecentList()
{
    const QStringList recent = qApp->instance()->property("recent").toStringList();
    m_recent_documents->clear();
    m_recent_documents->addItems(recent);
}

void SupraFitGui::LoadJson(const QJsonObject& str)
{
    bool valid = true;
    for (const QString& s : str.keys()) {
        if (str[s].toObject()["model"].toInt() == 200) {
            valid = false;
            /* int size = str[s].toObject()["size"].toInt();
            for(int i = 0; i < size; ++i)
            {
                valid = valid && m_hashed_data.contains(str[s].toObject()["uuids"].toObject()[QString::number(i)].toString());
            }*/
        }
    }
    if (!valid) {
        QMessageBox::warning(this, tr("Loading Meta Models."), tr("Adding Meta Models without the corresponding data is not supported!"), QMessageBox::Ok | QMessageBox::Default);

        return;
    }
    QTimer::singleShot(0, m_splash, &QSplashScreen::show);
    m_mainsplitter->setGraphicsEffect(new QGraphicsBlurEffect());
    SetData(str, "noname");
    m_mainsplitter->setGraphicsEffect(NULL);

    QTimer::singleShot(1, m_splash, &QSplashScreen::close);
}

bool SupraFitGui::SetData(const QJsonObject& object, const QString& file)
{
    if (object.isEmpty())
        return false;

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
    QPointer<MainWindow> window = new MainWindow();
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

    // Lets add this on first demand, should increase loading speed of big projects

    int index = m_stack_widget->addWidget(window);
    if (index == 1)
        m_stack_widget->setCurrentWidget(window);

    connect(window, &MainWindow::ModelsChanged, m_project_tree, [=]() {
        m_project_tree->UpdateStructure();
    });

    m_project_list.insert(m_project_list.size() - m_meta_models.size(), window);
    m_data_list.insert(m_data_list.size() - m_meta_models.size(), data);

    m_hashed_data[data.data()->UUID()] = data;
    m_project_tree->UpdateStructure();
    setActionEnabled(true);
    return true;
}

void SupraFitGui::AddMetaModel(const QModelIndex& index, int position)
{
    QStringList uuids = m_project_tree->UUID(index).split("|");

    if(uuids.size() != 2)
        return;

    QPointer<DataClass> data;
    for(int i = 0; i < m_hashed_data[uuids.first()].data()->ChildrenSize(); ++i)
    {
        if(qobject_cast<AbstractModel *>(m_hashed_data[uuids.first()].data()->Children(i))->ModelUUID() == uuids[1])
            data = m_hashed_data[uuids.first()].data()->Children(i);
    }

    if(!data)
        return;

    if (position == -1) {
        MainWindow* window = new MainWindow;

        QWeakPointer<MetaModel> model = qobject_cast<MetaModel*>(window->CreateMetaModel(m_hashed_wrapper[data->UUID()]));
        if (!model) {
            delete window;
            return;
        }

        m_stack_widget->addWidget(window);

        connect(window, &MainWindow::ModelsChanged, this, [=]() {
            m_project_tree->UpdateStructure();
        });
        m_project_list.append(window);

        m_data_list.append(model);
        m_hashed_data.insert(model.data()->UUID(), model);
        m_project_tree->UpdateStructure();
        setActionEnabled(true);
        model.data()->addModel(qobject_cast<AbstractModel*>(data));
        m_meta_models.append(model);
    } else if ((position - (m_data_list.size() - m_meta_models.size())) < m_meta_models.size()) {
        QWeakPointer<ChartWrapper> wrapper = m_project_list[position]->getChartWrapper();
        wrapper.data()->addWrapper(m_hashed_wrapper[uuids.first()]);

        m_meta_models[position - (m_data_list.size() - m_meta_models.size())].data()->addModel(qobject_cast<AbstractModel*>(data));
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

        connect(window, &MainWindow::ModelsChanged, m_project_tree, [=]() {
            m_project_tree->UpdateStructure();
        });

        m_project_list << window;

        m_data_list << model;
        m_project_tree->UpdateStructure();
        setActionEnabled(true);
        m_meta_models << model;

        int index = m_stack_widget->addWidget(window);
        if (index == 1)
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

    Waiter wait;
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
    ConfigDialog dialog(m_opt_config, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_opt_config = dialog.Config();
        //m_model_dataholder->setSettings(m_opt_config);
        WriteSettings();
    }
}

void SupraFitGui::setWindowTitle()
{
    QFileInfo info(m_filename_line->text());
    QMainWindow::setWindowTitle(QString("SupraFit *" + info.baseName() + "*"));
}

void SupraFitGui::ReadSettings()
{
    QSettings _settings;
    _settings.beginGroup("main");

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

    if (qApp->instance()->property("ColorFullSearch") == QVariant())
        qApp->instance()->setProperty("ColorFullSearch", true);

    if (qApp->instance()->property("auto_confidence") == QVariant())
        qApp->instance()->setProperty("auto_confidence", true);

    if (qApp->instance()->property("series_confidence") == QVariant())
        qApp->instance()->setProperty("series_confidence", false);

    if (qApp->instance()->property("charttheme") == QVariant())
        qApp->instance()->setProperty("charttheme", 0);

    if (qApp->instance()->property("dirlevel") == QVariant())
        qApp->instance()->setProperty("dirlevel", 1);

    if (qApp->instance()->property("p_value") == QVariant())
        qApp->instance()->setProperty("p_value", 0.95);

    if (qApp->instance()->property("markerSize") == QVariant())
        qApp->instance()->setProperty("markerSize", 6);

    if (qApp->instance()->property("lineWidth") == QVariant())
        qApp->instance()->setProperty("lineWidth", 20);

    if (qApp->instance()->property("chartScaling") == QVariant())
        qApp->instance()->setProperty("chartScaling", 4);

    if (qApp->instance()->property("transparentChart") == QVariant())
        qApp->instance()->setProperty("transparentChart", true);

    if (qApp->instance()->property("cropedChart") == QVariant())
        qApp->instance()->setProperty("cropedChart", true);

    if (qApp->instance()->property("noGrid") == QVariant())
        qApp->instance()->setProperty("noGrid", true);

    if (qApp->instance()->property("empAxis") == QVariant())
        qApp->instance()->setProperty("empAxis", true);

    if (qApp->instance()->property("xSize") == QVariant())
        qApp->instance()->setProperty("xSize", 600);

    if (qApp->instance()->property("ySize") == QVariant())
        qApp->instance()->setProperty("ySize", 400);

    if (qApp->instance()->property("FastConfidenceScaling") == QVariant())
        qApp->instance()->setProperty("FastConfidenceScaling", -4);

    if (qApp->instance()->property("FastConfidenceSteps") == QVariant())
        qApp->instance()->setProperty("FastConfidenceSteps", 1000);
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
    info += "<p>SupraFit User Interface is divided into three parts:<li>The <strong>project list </strong>on the left side,</li> <li> the <strong>Workspacein</strong> the middle and</li> <li> the <strong>Chart Widget</strong> the left hand side!</li></p>";
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

QPair<int, int> SupraFitGui::UUID2Widget(const QModelIndex& index)
{
    int widget = 0;
    int tab = -1;
    QStringList uuids = m_project_tree->UUID(index).split("|");
    if (uuids.size() == 1) {
        for (int i = 0; i < m_project_list.size(); ++i)
            if (m_project_list[i]->UUID() == uuids[0])
                widget = i;
    } else if (uuids.size() == 2) {
        for (int i = 0; i < m_project_list.size(); ++i)
            if (m_project_list[i]->UUID() == uuids[0]) {
                widget = i;
                for (int j = 0; j < m_project_list[i]->ModelCount(); ++j) {
                    if (m_project_list[i]->Model(j)->ModelUUID() == uuids[1])
                        tab = j;
                }
            }
    }
    return QPair<int, int>(widget, tab);
}

void SupraFitGui::TreeDoubleClicked(const QModelIndex& index)
{
    int widget = 0;
    int tab = -1;
    QPair<int, int> pair = UUID2Widget(index);
    widget = pair.first;
    tab = pair.second;

    if (m_stack_widget->indexOf(m_project_list[widget]) == -1)
        m_stack_widget->addWidget(m_project_list[widget]);
    m_stack_widget->setCurrentWidget(m_project_list[widget]);
    m_project_list[widget]->setCurrentTab(tab + 1);
}

void SupraFitGui::TreeClicked(const QModelIndex& index)
{
    int widget = 0;
    int tab = -1;
    QPair<int, int> pair = UUID2Widget(index);
    widget = pair.first;
    tab = pair.second;

    if (m_project_list[widget] == m_stack_widget->currentWidget())
        m_project_list[widget]->setCurrentTab(tab + 1);
}

void SupraFitGui::TreeRemoveRequest(const QModelIndex& index)
{

    int widget = 0;
    int tab = -1;
    QPair<int, int> pair = UUID2Widget(index);
    widget = pair.first;
    tab = pair.second;

    if (tab != -1) {
        m_project_list[widget]->RemoveTab(tab + 1);
    } else {
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
    m_project_tree->UpdateStructure();
}

void SupraFitGui::CloseProjects()
{
    for(int i = m_data_list.size() -1; i >= 0; --i)
    {
        MainWindow* mainwindow = m_project_list.takeAt(i);
        m_stack_widget->removeWidget(mainwindow);
        m_data_list.remove(i);
        m_hashed_data.remove(mainwindow->Data()->UUID());
        delete mainwindow;
        m_project_tree->UpdateStructure();
    }
    m_supr_file.clear();
    m_filename_line->clear();
}

void SupraFitGui::SaveData(const QModelIndex& index)
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("SupraFit Project File  (*.suprafit);;Json File (*.json)"));
    if (str.isEmpty() || str.isNull())
        return;

    QJsonObject object;
    int widget = 0;
    int tab = -1;
    QPair<int, int> pair = UUID2Widget(index);
    widget = pair.first;
    tab = pair.second;

    if (tab != -1)
        object = m_project_list[widget]->SaveProject();
    else
        object = m_project_list[widget]->SaveModel(tab + 1);

    JsonHandler::WriteJsonFile(object, str);
    setLastDir(str);
}

void SupraFitGui::CopySystemParameter(const QModelIndex& source, int position)
{
    if (position >= m_data_list.size())
        return;

    QStringList uuids = m_project_tree->UUID(source).split("|");
    if (uuids.size() != 1)
        return;

    QPointer<DataClass> data = m_hashed_data[uuids[0]].data();

    for (int i = 0; i < m_meta_models.size(); ++i) {
        if (m_meta_models[i].data()->UUID() == data->UUID())
            return;
    }
    m_data_list[position].data()->OverrideSystemParameter(data->SysPar());
}
/*
void SupraFitGui::CopyModel(const ModelMime* d, int data, int model)
{
    QByteArray sprmodel = d->data("application/x-suprafitmodel");
    QJsonDocument doc = QJsonDocument::fromBinaryData(sprmodel);
    QJsonObject object = doc.object();

    if (data < m_project_list.size()) {
        m_project_list[data]->Model(model)->ImportModel(object);
    } else
        qDebug() << "not found" << data << model;
}*/

void SupraFitGui::CopyModel(const QJsonObject& object, int data, int model)
{
    if (data < m_project_list.size()) {
        m_project_list[data]->Model(model)->ImportModel(object);
        m_project_list[data]->Model(model)->Calculate();
    } else
        qDebug() << "not found" << data << model;
}

void SupraFitGui::ExportAllPlain()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose directory"), getDir());
    if (dir.isEmpty() || dir.isNull())
        return;

    for (int i = 0; i < m_data_list.size(); ++i) {
        QString input, name = m_data_list[i].data()->ProjectTitle();
        QPointer<DataTable> indep_model = m_data_list[i].data()->IndependentModel();
        QPointer<DataTable> dep_model = m_data_list[i].data()->DependentModel();

        QStringList x = indep_model->ExportAsStringList();
        QStringList y = dep_model->ExportAsStringList();

        if (x.size() == y.size()) {
            for (int i = 0; i < x.size(); ++i)
                input += x[i].replace(",", ".") + "\t" + y[i].replace(",", ".") + "\n";
        }

        delete indep_model;
        delete dep_model;

        QString filename = dir + "/" + name;
        QFileInfo info(filename + ".dat");
        if (info.exists()) {
            int i = 1;
            QFileInfo info(filename + "_" + QString::number(i) + ".dat");
            while (info.exists()) {
                ++i;
                info = QFileInfo(filename + QString::number(i) + ".dat");
            }
            filename = filename + "_" + QString::number(i) + ".dat";
        } else
            filename = filename + ".dat";

        QFile file(filename);
        if (file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << input;
        }
    }
    setLastDir(dir);
}

void SupraFitGui::ExportAllSupraFit()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose directory"));
    if (dir.isEmpty() || dir.isNull())
        return;

    for (int i = 0; i < m_data_list.size(); ++i) {
        QString name = m_data_list[i].data()->ProjectTitle();
        QJsonObject data;
        data["data"] = m_data_list[i].data()->ExportData();

        QString filename = dir + "/" + name;
        QFileInfo info(filename + ".suprafit");
        if (info.exists()) {
            int i = 1;
            QFileInfo info(filename + "_" + QString::number(i) + ".suprafit");
            while (info.exists()) {
                ++i;
                info = QFileInfo(filename + QString::number(i) + ".suprafit");
            }
            filename = filename + "_" + QString::number(i) + ".suprafit";
        } else
            filename = filename + ".suprafit";

        JsonHandler::WriteJsonFile(data, filename);
    }
    setLastDir(dir);
}

void SupraFitGui::AddScatter()
{
    QString file = QFileDialog::getOpenFileName(this, "Select file", getDir(), tr("Supported files (*.suprafit *.json *.jdat *.txt *.dat *.itc *.ITC *.dh *.DH);;Json File (*.json);;SupraFit Project File  (*.suprafit);;Table Files (*.dat *.txt *.itc *.ITC);;Origin Files(*.dh *.DH);;All files (*.*)"));
    if (file.isEmpty())
        return;

    QJsonObject data;

    if (file.contains("json") || file.contains("jdat") || file.contains("suprafit")) {
        QJsonObject toplevel;

        if (JsonHandler::ReadJsonFile(toplevel, file)) {

            QStringList keys = toplevel.keys();
            if (keys.contains("data")) {
                data = toplevel;
            } else {
                QMessageBox::warning(this, tr("Loading Datas."), tr("Sorry, couldn't handle this data!"), QMessageBox::Ok | QMessageBox::Default);
                return;
            }
        }
    } else {
        ImportData dialog(file, this);

        if (dialog.exec() == QDialog::Accepted) {
            data = dialog.getProject();
        } else
            return;
    }
    AddScatter(data);
}

void SupraFitGui::AddScatter(const QJsonObject& object)
{
    DataTable* table = new DataTable(object["data"].toObject()["dependent"].toObject());
    for (int i = 0; i < m_data_list.size(); ++i) {
        m_data_list[i].data()->DependentModel()->Table() += table->Table();
        m_data_list[i].data()->Updated();
    }
    delete table;
}

void SupraFitGui::Messages()
{
    QPlainTextEdit* text = new QPlainTextEdit;
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(text);
    text->appendPlainText(qApp->instance()->property("messages").toString());
    QDialog dialog;
    dialog.setLayout(layout);
    dialog.resize(800, 600);
    dialog.exec();
}

void SupraFitGui::LicenseInfo()
{

    QPlainTextEdit* text = new QPlainTextEdit;
    text->setReadOnly(true);
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(text);

    QString content("License Information for SupraFit\n");
    QFile file(":/LICENSE.md");
    if (file.open(QIODevice::ReadOnly)) {
        content += QString(file.readAll());
    }

#ifdef noto_font
    content += "\n\nLicense Information for Google Noto Fonts\nhttps://github.com/googlei18n/noto-fonts\n\n";

    QFile file2(":/fonts/LICENSE");
    if (file2.open(QIODevice::ReadOnly)) {
        content += QString(file2.readAll());
    } else
        qDebug() << file2.errorString() << file2.fileName();
#endif
    text->setPlainText(content);

    QDialog dialog;
    dialog.setLayout(layout);
    dialog.resize(800, 600);
    dialog.exec();
}
