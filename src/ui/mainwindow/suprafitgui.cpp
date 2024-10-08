/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2024 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/global_infos.h"
#include "src/version.h"

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

#include "src/core/filehandler.h"
#include "src/core/jsonhandler.h"

#include "src/ui/instance.h"

#include "src/ui/dialogs/configdialog.h"
#include "src/ui/dialogs/importdata.h"
#include "src/ui/dialogs/spectraimport.h"

#include "src/ui/guitools/guitools.h"

#include "src/ui/mainwindow/chartwidget.h"
#include "src/ui/mainwindow/datawidget.h"
#include "src/ui/mainwindow/mainwindow.h"
#include "src/ui/mainwindow/modeldataholder.h"
#include "src/ui/mainwindow/modelwidget.h"
#include "src/ui/mainwindow/projecttree.h"

#include "src/ui/widgets/messagedock.h"

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMimeData>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QSettings>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>
#include <QtCore/QWeakPointer>

#include <QtGui/QAction>

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
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeView>

#include <QtCharts/QChartView>

#include <stdio.h>

#include "suprafitgui.h"

SupraFitGui::SupraFitGui()
{
    m_instance = new Instance;
    Instance::setInstance(m_instance);
    ReadSettings();

    m_message_dock = new QDockWidget;
    m_messages_widget = new MessageDock;
    m_message_dock->setWidget(m_messages_widget);
    m_message_dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    m_message_dock->setTitleBarWidget(m_messages_widget->TitleBarWidget());
    m_message_dock->setObjectName(tr("message_dock"));
    addDockWidget(Qt::BottomDockWidgetArea, m_message_dock);

    m_splash = new QSplashScreen(QPixmap(":/misc/logo_small.png"));

    m_project_view = new QTreeView;
    m_project_view->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_project_view->setDragEnabled(true);
    m_project_view->setDragDropMode(QAbstractItemView::DragOnly);
    m_project_view->setItemDelegate(new ProjectTreeEntry());
    m_project_view->setExpandsOnDoubleClick(false);
    //m_project_view->setSelectionMode(QAbstractItemView::NoSelection);

    QAction* action;
    /*
    action = new QAction("Load", m_project_view);
    m_project_view->addAction(action);
    connect(action, &QAction::triggered, action, [this]() {
        TreeClicked(m_project_view->currentIndex());

    });
    */

    action = new QAction("Duplicate", m_project_view);
    m_project_view->addAction(action);
    action->setIcon(Icon("window-duplicate"));

    connect(action, &QAction::triggered, action, [this]() {
        Duplicate(m_project_view->currentIndex());
    });

    action = new QAction("Add Up", m_project_view);
    m_project_view->addAction(action);
    action->setIcon(Icon("list-add-blue"));

    connect(action, &QAction::triggered, action, [this]() {
        AddUpData(m_project_view->currentIndex(), true);
    });

    action = new QAction("Substract", m_project_view);
    m_project_view->addAction(action);
    action->setIcon(Icon("list-remove-blue"));

    connect(action, &QAction::triggered, action, [this]() {
        AddUpData(m_project_view->currentIndex(), false);
    });

    action = new QAction;
    action->setSeparator(true);
    m_project_view->addAction(action);

    action = new QAction("Export", m_project_view);
    m_project_view->addAction(action);
    action->setIcon(Icon("document-save"));

    connect(action, &QAction::triggered, action, [this]() {
        SaveData(m_project_view->currentIndex());
    });

    action = new QAction("Delete", m_project_view);
    action->setIcon(Icon("trash-empty"));
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

    m_blank_widget = new QSplitter(Qt::Horizontal);
    m_blank_widget->setObjectName("blank_widget");
    QWidget* logoHolder = new QWidget;

    QVBoxLayout* logolayout = new QVBoxLayout;

    m_recentWidget = new QWidget;
    m_recentWidget->setObjectName("recent_list");
    m_recentWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    m_recent_documents = new QListWidget;
    connect(m_recent_documents, &QListWidget::itemDoubleClicked, m_recent_documents, [this](const QListWidgetItem* item) {
        const QString str = item->data(Qt::UserRole).toString();
        UpdateRecentListProperty(str);
        //QStringList recent = qApp->instance()->property("recent").toStringList();
        //recent.removeOne(str);
        //recent.prepend(str);
        //qApp->instance()->setProperty("recent", recent);
        LoadFile(str);
        UpdateRecentList();
    });
    //m_recent_documents->setMaximumWidth(200);

    m_logolabel = new LogoLabel(this);
    m_logolabel->setPixmap(":/misc/logo_small.png");
    m_logolabel->setObjectName("logowidget");
    // m_logolabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    logolayout->addWidget(m_logolabel);
    logoHolder->setLayout(logolayout);

    m_clear_recent = new QPushButton(tr("Clear list"));
    connect(m_clear_recent, &QPushButton::clicked, m_clear_recent, [this]()
    {
        qApp->instance()->setProperty("recent", QStringList());
        this->UpdateRecentList();
    });
    m_clear_recent->setFlat(true);
    m_clear_recent->setIcon(Icon("edit-clear-history"));

    QGridLayout *blank_layout = new QGridLayout;
    blank_layout->addWidget(new QLabel(tr("<h3>Recent documents</h3>")), 0, 0);
    blank_layout->addWidget(m_clear_recent, 0, 1);
    blank_layout->addWidget(m_recent_documents, 1, 0, 1, 2);
    m_recentWidget->setLayout(blank_layout);
    m_blank_widget->addWidget(logoHolder);
    m_blank_widget->addWidget(m_recentWidget);
    m_blank_widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_stack_widget = new QStackedWidget;
    m_stack_widget->addWidget(m_blank_widget);
    m_stack_widget->setObjectName("project_mainwindow");

    m_filename_line = new QLineEdit;
    m_filename_line->setClearButtonEnabled(true);
    connect(m_filename_line, &QLineEdit::textChanged, this, [this](const QString& str) { this->m_supr_file = str; setWindowTitle(); });
    m_project_holder = new QWidget;
    m_project_holder->setObjectName("project_list");
    m_project_holder->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_project_holder->setMaximumWidth(400);

    m_export_plain = new QToolButton;
    m_export_plain->setAutoRaise(true);
    m_export_plain->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QAction* export_action = new QAction(tr("Export Data"));
    export_action->setToolTip(tr("Export the input data of every single project and save it as individual plain table."));
    export_action->setIcon(Icon("kspread"));
    m_export_plain->setDefaultAction(export_action);
    connect(export_action, &QAction::triggered, this, &SupraFitGui::ExportAllPlain);

    m_export_suprafit = new QToolButton;
    m_export_suprafit->setAutoRaise(true);
    m_export_suprafit->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QAction* export_suprafit = new QAction(tr("Export Projects"));
    export_suprafit->setToolTip(tr("Export every single project and save it as individual SupraFit project."));
    export_suprafit->setIcon(QIcon(":/misc/SupraFit.png"));
    m_export_suprafit->setDefaultAction(export_suprafit);
    connect(export_suprafit, &QAction::triggered, this, &SupraFitGui::ExportAllSupraFit);

    m_add_scatter = new DropButton;
    m_add_scatter->setAutoRaise(true);
    m_add_scatter->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QAction* add_scatter = new QAction(tr("Drop Random"));
    add_scatter->setToolTip(tr("Drop an table with random numbers (or click and select one) to override the dependent tables."));
    add_scatter->setIcon(Icon("clock"));
    m_add_scatter->setDefaultAction(add_scatter);
    connect(add_scatter, &QAction::triggered, this, qOverload<>(&SupraFitGui::AddScatter));
    connect(m_add_scatter, &DropButton::DataDropped, this, [this](const QJsonObject& data) {
        QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("SupraFit Project File  (*.suprafit);;Json File (*.json)"));
        if (!str.isEmpty()) {
            JsonHandler::WriteJsonFile(data, str);
        }
        AddScatter(data);
    });

    m_close_all = new QToolButton;
    m_close_all->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_close_all->setAutoRaise(true);

    QAction* close_action = new QAction(tr("Close All"));
    close_action->setIcon(Icon("document-close"));
    close_action->setToolTip(tr("Close all open prjects."));

    m_close_all->setDefaultAction(close_action);
    connect(close_action, &QAction::triggered, this, &SupraFitGui::CloseProjects);

    QHBoxLayout* tools = new QHBoxLayout;
    tools->addWidget(m_export_plain);
    tools->addWidget(m_export_suprafit);
    if (qApp->instance()->property("advanced_ui").toBool()) {
        tools->addWidget(m_add_scatter);
    }
    tools->addWidget(m_close_all);

    QGridLayout* layout = new QGridLayout;

    layout->addWidget(new QLabel(tr("Filename:")), 0, 0);
    layout->addWidget(m_filename_line, 0, 1);
    layout->addLayout(tools, 1, 0, 1, 2);
    layout->addWidget(m_project_view, 2, 0, 1, 2);
    m_project_holder->setLayout(layout);

    m_mainsplitter = new QSplitter(Qt::Horizontal);
    m_mainsplitter->setObjectName(tr("project_splitter"));
    m_mainsplitter->addWidget(m_project_holder);
    m_mainsplitter->addWidget(m_stack_widget);

    setCentralWidget(m_mainsplitter);

    m_new_window = new QAction(Icon("window-new"), tr("New Window"), this);
    connect(m_new_window, SIGNAL(triggered(bool)), this, SLOT(NewWindow()));

    m_new_table = new QAction(Icon("document-new"), tr("New Table"), this);
    connect(m_new_table, SIGNAL(triggered(bool)), this, SLOT(NewTable()));
    m_new_table->setShortcut(QKeySequence::New);

    m_load = new QAction(Icon("document-open"), tr("Open Project"), this);
    connect(m_load, SIGNAL(triggered(bool)), this, SLOT(OpenFile()));
    m_load->setShortcut(QKeySequence::Open);

    m_thermogram = new QAction(Icon("thermogram"), tr("Open Thermogram"), this);
    connect(m_thermogram, SIGNAL(triggered(bool)), this, SLOT(OpenThermogram()));

    m_spectra = new QAction(Icon("spectra_ico"), tr("Open Spectra"), this);
    connect(m_spectra, SIGNAL(triggered(bool)), this, SLOT(OpenSpectraDir()));

    m_save = new QAction(Icon("document-save"), tr("&Save Project"), this);
    m_save->setShortcuts(QKeySequence::Save);
    connect(m_save, SIGNAL(triggered(bool)), this, SLOT(SaveProjectAction()));

    m_save_as = new QAction(Icon("document-save-as"), tr("Save Project &As"), this);
    connect(m_save_as, SIGNAL(triggered(bool)), this, SLOT(SaveAsProjectAction()));
    m_save_as->setShortcut(QKeySequence::SaveAs);

    m_message_dock_action = m_message_dock->toggleViewAction();
    m_message_dock_action->setIcon(QIcon(":/icons/help-hint.png"));
    m_message_dock_action->setToolTip(tr("No unread messages"));
    m_message_dock_action->setText(tr("Toggle Message"));
    m_message_dock_action->setShortcut(QKeySequence(tr("F4")));

    connect(m_messages_widget, &MessageDock::Presence, this, [this]() {
        if (m_message_dock->isHidden() && !m_alert) {
            m_message_dock_action->setIcon(QIcon(":/icons/help-hint-green.png"));
            m_message_dock_action->setToolTip(tr("There are unread messages. But that is ok."));
        }
    });

    connect(m_messages_widget, &MessageDock::UiInfo, this, [this]() {
        if (m_message_dock->isHidden() && !m_alert) {
            m_message_dock_action->setIcon(QIcon(":/icons/help-hint-blue.png"));
            m_message_dock_action->setToolTip(tr("There are unread user interface messagess. But that is ok."));
        }
    });

    connect(m_messages_widget, &MessageDock::Attention, this, [this]() {
        if (m_message_dock->isHidden()) {
            m_message_dock_action->setIcon(QIcon(":/icons/help-hint-red.png"));
            m_message_dock_action->setToolTip(tr("There are unread ERRORs. Please don't not ignore them."));
            m_alert = true;
        }
    });

    connect(m_message_dock_action, &QAction::toggled, this, [this]() {
        m_message_dock_action->setIcon(QIcon(":/icons/help-hint.png"));
        m_message_dock_action->setToolTip(tr("No unread messages"));
        m_alert = false;
    });

    m_project_action = new QAction(tr("Toggle Projects"));
    /*
    m_project_action->setIcon(Icon("view-list-text"));
    m_project_action->setCheckable(true);
    m_project_action->setChecked(true);
    m_project_action->setShortcut(QKeySequence(tr("F3")));

    m_show_tree = new QPropertyAnimation(m_project_holder, "size");
    m_show_tree->setDuration(100);

    connect(m_project_action, &QAction::toggled, this, [this](){
        QSize size;
        if(m_project_holder->width())
        {
            m_project_tree_size = m_project_holder->width();

            m_show_tree->setStartValue(QSize(m_project_tree_size, m_project_holder->height()));
            size = QSize(0, m_project_holder->height());
        }else
        {
            m_show_tree->setStartValue(QSize(0, m_project_holder->height()));
            size = QSize(m_project_tree_size, m_project_holder->height());
        }
        m_show_tree->setEndValue(size);
        m_show_tree->start();
        QTimer::singleShot(100, m_project_holder, [this, size](){m_project_holder->resize(size);});
    });
    */
    m_config = new QAction(Icon("configure"), tr("Settings"), this);
    connect(m_config, SIGNAL(triggered()), this, SLOT(SettingsDialog()));
    m_config->setShortcut(QKeySequence::Preferences);

    m_about = new QAction(QIcon(":/misc/SupraFit.png"), tr("Info"), this);
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
    m_main_toolbar->addAction(m_thermogram);
    m_main_toolbar->addAction(m_spectra);
    m_main_toolbar->addAction(m_save);
    m_main_toolbar->addAction(m_save_as);
    m_main_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    addToolBar(m_main_toolbar);

    m_system_toolbar = new QToolBar;
    m_system_toolbar->setObjectName(tr("system_toolbar"));
    //m_system_toolbar->addAction(m_project_action);
    m_system_toolbar->addAction(m_message_dock_action);
    m_system_toolbar->addSeparator();
    m_system_toolbar->addAction(m_config);
    m_system_toolbar->addAction(m_about);
    m_system_toolbar->addAction(m_license);
    m_system_toolbar->addAction(m_aboutqt);
    m_system_toolbar->addAction(m_close);
    m_system_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    addToolBar(m_system_toolbar);

    m_project_tree_size = m_project_holder->width();


    setActionEnabled(false);
    setStyleSheet("QSplitter::handle:vertical {"
                  "background: qlineargradient(x1:0, y1:0, x2:1, y2:1;"
                  "stop:0 #eee, stop:1 #ccc);"
                  "border: 1px solid #777;"
                  "height: 1px;"
                  "margin-top: 2px;"
                  "margin-bottom: 2px;"
                  "border-radius: 4px;"
                  "}"
                  "QDockWidget {"
                  "border: 1px solid #777;"
                  "background: qlineargradient(x1:0, y1:0, x2:1, y2:1;}");

    QStatusBar* statusbar = new QStatusBar;
    QLabel* version = new QLabel(tr("SupraFit %1").arg(git_tag));
    statusbar->addPermanentWidget(version);
    setStatusBar(statusbar);

    qApp->installEventFilter(this);
    connect(m_project_view, &QTreeView::doubleClicked, this, &SupraFitGui::TreeDoubleClicked);
    connect(m_project_view, &QTreeView::clicked, this, &SupraFitGui::TreeClicked);
    m_project_view->setColumnWidth(0, 250);

    setWindowIcon(QIcon(":/misc/SupraFit.png"));
    UpdateRecentList();
    m_logolabel->resize(873 * 0.5, 419 * 0.5);
    m_logolabel->setMaximumSize(873, 419);
    ReadGeometry();
    m_messages_widget->Message("SupraFit is up and running");
}

SupraFitGui::~SupraFitGui()
{
    if (m_instance)
        delete m_instance;
}

void SupraFitGui::Message(const QString& str)
{
    m_messages_widget->Message(str);
}

void SupraFitGui::Info(const QString& str)
{
    m_messages_widget->Info(str);
}

void SupraFitGui::Warning(const QString& str)
{
    m_messages_widget->Warning(str);
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
void SupraFitGui::LoadFile(const QString& file, int overwrite_type)
{
    QFileInfo info(file);

    if (overwrite_type == 2) // This is reservered for thermograms, that can not automatically be deduced by file name (like *.itc)
    {
        ImportData dialog(this);
        dialog.ImportThermogram(file);
        if (dialog.exec() == QDialog::Accepted) {
            SetData(dialog.getProject(), dialog.ProjectFile(), getDir());
            m_mainsplitter->show();
        }
        return;
    }

    if (file.contains("|||")) {
        SpectraImport* spectra = new SpectraImport(file);

        if (spectra->exec()) {
            ImportData dialog(this);
            DataTable* tmp = new DataTable;
            tmp->ImportTable(spectra->InputTable());
            dialog.LoadTable(tmp, 2);
            dialog.setSpectraData(spectra->ProjectData());
            if (dialog.exec() == QDialog::Accepted) {
                SetData(dialog.getProject(), dialog.ProjectFile(), getDir());
                m_mainsplitter->show();
            }
        }
        return;
    }
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
            UpdateRecentListProperty(file);
            UpdateRecentList();
            return;
        }
    } else {
        ImportTable(file);
    }
    m_mainsplitter->setGraphicsEffect(NULL);

    QTimer::singleShot(1, m_splash, &QSplashScreen::close);

    if (invalid_json)
        QMessageBox::warning(this, tr("Loading Datas."), tr("Sorry, but this doesn't contain any titration tables!"), QMessageBox::Ok | QMessageBox::Default);
    else
        UpdateRecentList();
}

void SupraFitGui::SpectraEdited(const QJsonObject& table, const QJsonObject& data)
{
    ImportData dialog(this);
    DataTable* tmp = new DataTable;
    tmp->ImportTable(table);
    dialog.LoadTable(tmp, 2);
    dialog.setSpectraData(data);
    if (dialog.exec() == QDialog::Accepted) {
        SetData(dialog.getProject(), dialog.ProjectFile(), getDir());
        m_mainsplitter->show();
    }
}

void SupraFitGui::OpenSpectraDir()
{
    SpectraImport* spectra = new SpectraImport;

    if (spectra->exec()) {
        ImportData dialog(this);
        DataTable* tmp = new DataTable;
        dialog.LoadTable(tmp, 2);
        dialog.setSpectraData(spectra->ProjectData());
        if (dialog.exec() == QDialog::Accepted) {
            SetData(dialog.getProject(), dialog.ProjectFile(), getDir());
            m_mainsplitter->show();
        }
    }
    UpdateRecentList();
}

void SupraFitGui::setActionEnabled(bool enabled)
{
    m_save->setEnabled(enabled);
    m_save_as->setEnabled(enabled);
}

void SupraFitGui::UpdateRecentList()
{
    const QStringList recent = qApp->instance()->property("recent").toStringList();
    m_recent_documents->clear();
    for (const QString& str : recent) {
        QFileInfo info(str);
        if (info.exists() || str.contains("|||")) {
            QListWidgetItem* item = new QListWidgetItem(info.baseName() + "." + info.suffix());
            item->setData(Qt::UserRole, str);
            item->setData(Qt::ToolTipRole, str);
            if (info.suffix() == "suprafit" || info.suffix() == "json")
                item->setData(Qt::DecorationRole, QPixmap(":/misc/SupraFit.png").scaled(32, 32));
            else if (info.suffix() == "txt" || info.suffix() == "dat")
                item->setData(Qt::DecorationRole, QPixmap(":/icons/kspread.png").scaled(32, 32));
            else if (info.suffix() == "itc")
                item->setData(Qt::DecorationRole, QPixmap(":/icons/thermogram.png").scaled(32, 32));
            else if (info.isDir() || str.contains("|||"))
                item->setData(Qt::DecorationRole, QPixmap(":/icons/spectra_ico.png").scaled(32, 32));
            else
                item->setData(Qt::DecorationRole, QPixmap(":/icons/document-edit.png").scaled(32, 32));

            m_recent_documents->addItem(item);
        }
    }
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
    SetData(str, "noname", getDir());
    m_mainsplitter->setGraphicsEffect(NULL);
    UpdateRecentList();
    QTimer::singleShot(1, m_splash, &QSplashScreen::close);
}

bool SupraFitGui::SetData(const QJsonObject& object, const QString& file, const QString& path)
{
    if (object.isEmpty())
        return false;

    QString uuid = object["data"].toObject()["uuid"].toString();
    if (m_hashed_data.keys().contains(uuid)) {
        QString name = m_hashed_data[uuid].toStrongRef().data()->ProjectTitle();
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
    connect(window, &MainWindow::Message, m_messages_widget, &MessageDock::Message);
    connect(window, &MainWindow::Warning, m_messages_widget, &MessageDock::Warning);
    connect(window, &MainWindow::SpectraEdited, this, &SupraFitGui::SpectraEdited);
    connect(window, &MainWindow::AddProject, this, &SupraFitGui::LoadJson);

    QWeakPointer<DataClass> data = window->SetData(object);
    if (!data) {
        disconnect(window);
        delete window;
        return false;
    }
    if (data.toStrongRef().data()->isSimulation()) {
#pragma message("implement more simulation stuff")
    } else if (!data.toStrongRef().data()->Size()) {
        disconnect(window);
        delete window;
        return false;
    }
    m_hashed_wrapper.insert(data.toStrongRef().data()->UUID(), window->getChartWrapper());

    QString name = data.toStrongRef().data()->ProjectTitle();
    data.toStrongRef().data()->setRootDir(path);
    if (name.isEmpty() || name.isNull()) {
        name = file;
        data.toStrongRef().data()->setProjectTitle(name);
    }

    // Lets add this on first demand, should increase loading speed of big projects

    m_last_index = m_stack_widget->addWidget(window);
    //if (index == 1)
    m_stack_widget->setCurrentWidget(window);

    connect(window, &MainWindow::ModelsChanged, m_project_tree, [=]() {
        m_project_tree->UpdateStructure();
    });

    m_project_list.insert(m_project_list.size() - m_meta_models.size(), window);
    m_data_list.insert(m_data_list.size() - m_meta_models.size(), data);

    m_hashed_data[data.toStrongRef().data()->UUID()] = data;
    m_project_tree->UpdateStructure();
    setActionEnabled(true);

    return true;
}

void SupraFitGui::AddMetaModel(const QModelIndex& index, int position)
{
    Waiter wait;
    QStringList uuids = m_project_tree->UUID(index).split("|");

    if(uuids.size() != 2)
        return;

    QPointer<DataClass> data;
    for (int i = 0; i < m_hashed_data[uuids.first()].toStrongRef().data()->ChildrenSize(); ++i) {
        if (qobject_cast<AbstractModel*>(m_hashed_data[uuids.first()].toStrongRef().data()->Children(i))->ModelUUID() == uuids[1])
            data = m_hashed_data[uuids.first()].toStrongRef().data()->Children(i);
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
        connect(model.toStrongRef().data(), &MetaModel::Message, m_messages_widget, &MessageDock::Message);
        connect(model.toStrongRef().data(), &MetaModel::Warning, m_messages_widget, &MessageDock::Warning);

        m_stack_widget->addWidget(window);

        connect(window, &MainWindow::ModelsChanged, m_project_tree, [=]() {
            m_project_tree->UpdateStructure();
        });
        m_project_list.append(window);

        m_data_list.append(model);
        m_hashed_data.insert(model.toStrongRef().data()->UUID(), model);
        m_project_tree->UpdateStructure();
        setActionEnabled(true);
        model.toStrongRef().data()->addModel(qobject_cast<AbstractModel*>(data));
        m_meta_models.append(model);
    } else if ((position - (m_data_list.size() - m_meta_models.size())) < m_meta_models.size()) {
        QWeakPointer<ChartWrapper> wrapper = m_project_list[position]->getChartWrapper();
        wrapper.toStrongRef().data()->addWrapper(m_hashed_wrapper[uuids.first()]);

        m_meta_models[position - (m_data_list.size() - m_meta_models.size())].toStrongRef().data()->addModel(qobject_cast<AbstractModel*>(data));
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
            wrapper.toStrongRef().data()->addWrapper(m_hashed_wrapper[uuids[QString::number(i)].toString()]);

            t->ImportModel(rawmodel);
            model.toStrongRef().data()->addModel(t.data());

            connect(model.toStrongRef().data(), &DataClass::Message, m_messages_widget, &MessageDock::Message, Qt::UniqueConnection);
            connect(model.toStrongRef().data(), &DataClass::Warning, m_messages_widget, &MessageDock::Warning, Qt::UniqueConnection);
        }
        model.toStrongRef().data()->ImportModel(object["data"].toObject());

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
        SetData(dialog.getProject(), dialog.ProjectFile(), getDir());
        m_mainsplitter->show();
    }
}

void SupraFitGui::OpenFile()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this, "Select file", getDir(), m_supported_files);
    if (filenames.isEmpty())
        return;
    for (const QString& filename : qAsConst(filenames)) {
        setLastDir(filename);
        LoadFile(filename);
    }
    setActionEnabled(true);
}

void SupraFitGui::OpenThermogram()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this, "Select file", getDir(), m_supported_files);
    if (filenames.isEmpty())
        return;
    for (const QString& filename : qAsConst(filenames)) {
        setLastDir(filename);
        LoadFile(filename, 2);
    }
    setActionEnabled(true);
}

void SupraFitGui::ImportTable(const QString& file)
{
    ImportData dialog(file, this);

    if (dialog.exec() == QDialog::Accepted) {
        SetData(dialog.getProject(), dialog.ProjectFile(), getDir());
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

        if (keys.contains("data", Qt::CaseInsensitive)) {
            return SetData(toplevel, info.baseName(), info.absolutePath());
        } else if ((keys.contains("datapoints", Qt::CaseInsensitive) && keys.contains("equations", Qt::CaseInsensitive)) || keys.contains("Main", Qt::CaseInsensitive)) {
#pragma message("move the simulation import to the filehandler soon!")
            ImportData dialog(filename, this);
            if (dialog.exec() == QDialog::Accepted) {
                SetData(dialog.getProject(), dialog.ProjectFile(), getDir());
                m_mainsplitter->show();
                return true;
            }
        } else {
            bool exit = true;
            int index = 1;
            for (int i = 0; i < keys.size(); ++i) {
                QApplication::processEvents();
                QString str = QString("project_%1").arg(i);
                if (!keys.contains(str))
                    continue;
                QJsonObject object = toplevel[str].toObject();
                exit = exit && SetData(object, info.baseName() + "-" + QString::number(index), info.absolutePath());
                index++;
            }

            m_project_tree->setActiveIndex(m_last_index - 1);
            //   m_stack_widget->setCurrentWidget(0);

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

    Waiter wait;
    QMultiMap<QString, QJsonObject> projects;
    for (int i = 0; i < m_project_list.size(); i++) {
        QPointer<MainWindow> project_widget = m_project_list[i];
        projects.insert(project_widget->Name(), project_widget->SaveProject());
    }
    if (projects.isEmpty())
        return;
    else if (projects.size() == 1) {
        JsonHandler::WriteJsonFile(projects.first(), m_supr_file);
    } else {
        QJsonObject json;

        int i = 0;
        for (const auto& model : projects) {
            json["project_" + QString::number(i)] = model;
            i++;
        }
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

        Waiter wait;
        QMultiMap<QString, QJsonObject> projects;
        for (int i = 0; i < m_project_list.size(); i++) {
            QPointer<MainWindow> project_widget = m_project_list[i];
            projects.insert(project_widget->Name(), project_widget->SaveProject());
        }
        if (projects.isEmpty())
            return;
        else if (projects.size() == 1) {
            JsonHandler::WriteJsonFile(projects.first(), m_supr_file);
        } else {
            QJsonObject json;

            int i = 0;
            for (const auto& model : projects) {
                json["project_" + QString::number(i)] = model;
                i++;
            }
            JsonHandler::WriteJsonFile(json, m_supr_file);
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
        emit Instance::GlobalInstance()->ConfigurationChanged();
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

    if (qApp->instance()->property("tooltips") == QVariant())
        qApp->instance()->setProperty("tooltips", true);

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

    if (qApp->instance()->property("thermogram_guidelines") == QVariant())
        qApp->instance()->setProperty("thermogram_guidelines", true);

    if (qApp->instance()->property("p_value") == QVariant())
        qApp->instance()->setProperty("p_value", 0.95);

    if (qApp->instance()->property("calibration_start") == QVariant())
        qApp->instance()->setProperty("calibration_start", 0);

    if (qApp->instance()->property("calibration_heat") == QVariant())
        qApp->instance()->setProperty("calibration_heat", 0);

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

    if (qApp->instance()->property("MetaSeries") == QVariant())
        qApp->instance()->setProperty("MetaSeries", false);

    if (qApp->instance()->property("noGrid") == QVariant())
        qApp->instance()->setProperty("noGrid", true);

    if (qApp->instance()->property("UnsafeCopy") == QVariant())
        qApp->instance()->setProperty("UnsafeCopy", false);

    if (qApp->instance()->property("empAxis") == QVariant())
        qApp->instance()->setProperty("empAxis", true);

    if (qApp->instance()->property("auto_thermo_dialog") == QVariant())
        qApp->instance()->setProperty("auto_thermo_dialog", false);

    if (qApp->instance()->property("OverwriteBins") == QVariant())
        qApp->instance()->setProperty("OverwriteBins", false);

    if (qApp->instance()->property("advanced_ui") == QVariant()) {
        qApp->instance()->setProperty("advanced_ui", false);
        QTimer::singleShot(10, this, SLOT(FirstStart()));
    }

    if (qApp->instance()->property("ModelParameterColums") == QVariant())
        qApp->instance()->setProperty("ModelParameterColums", 2);

    if (qApp->instance()->property("xSize") == QVariant())
        qApp->instance()->setProperty("xSize", 600);

    if (qApp->instance()->property("ySize") == QVariant())
        qApp->instance()->setProperty("ySize", 400);

    if (qApp->instance()->property("EntropyBins") == QVariant())
        qApp->instance()->setProperty("EntropyBins", 30);

    if (qApp->instance()->property("FastConfidenceScaling") == QVariant())
        qApp->instance()->setProperty("FastConfidenceScaling", -4);

    if (qApp->instance()->property("FastConfidenceSteps") == QVariant())
        qApp->instance()->setProperty("FastConfidenceSteps", 100);

    if (qApp->instance()->property("InitialiseRandom") == QVariant())
        qApp->instance()->setProperty("InitialiseRandom", true);

    if (qApp->instance()->property("StoreRawData") == QVariant())
        qApp->instance()->setProperty("StoreRawData", true);

    if (qApp->instance()->property("StoreRawData") == QVariant())
        qApp->instance()->setProperty("StoreRawData", true);

    if (qApp->instance()->property("FullShannon") == QVariant())
        qApp->instance()->setProperty("FullShannon", false);

    if (qApp->instance()->property("StoreFileName") == QVariant())
        qApp->instance()->setProperty("StoreFileName", true);

    if (qApp->instance()->property("StoreAbsolutePath") == QVariant())
        qApp->instance()->setProperty("StoreAbsolutePath", false);

    if (qApp->instance()->property("StoreFileHash") == QVariant())
        qApp->instance()->setProperty("StoreFileHash", false);

    if (qApp->instance()->property("FindFileRecursive") == QVariant())
        qApp->instance()->setProperty("FindFileRecursive", false);

    if (qApp->instance()->property("LastSpectraType") == QVariant())
        qApp->instance()->setProperty("LastSpectraType", "csv");

    if (qApp->instance()->property("MarkerPointFeedback") == QVariant())
        qApp->instance()->setProperty("MarkerPointFeedback", 0);

    if (qApp->instance()->property("PointFeedback") == QVariant())
        qApp->instance()->setProperty("PointFeedback", false);

    if (qApp->instance()->property("ModuloPointFeedback") == QVariant())
        qApp->instance()->setProperty("ModuloPointFeedback", 0);

    if (qApp->instance()->property("lastSize") == QVariant())
        qApp->instance()->setProperty("lastSize", 2);

    if (qApp->instance()->property("ScriptTimeout") == QVariant())
        qApp->instance()->setProperty("ScriptTimeout", 500);

    if (qApp->instance()->property("MaxSeriesPoints") == QVariant() || qApp->instance()->property("MaxSeriesPoints").toInt() == 0)
        qApp->instance()->setProperty("MaxSeriesPoints", 200);

    qApp->instance()->setProperty("lastDir", getDir());
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
    QMessageBox::about(this, tr("About this application"), aboutSFHtml());
}

void SupraFitGui::FirstStart()
{
    QString info;
    info += "<p>Welcome to SupraFit, a non-linear fitting tool for supramolecular NMR titrations and ITC experiments.< /p>";
    info += "<p>The SupraFit User Interface is divided into three parts:<li>The <strong>Project List </strong> on the left side,</li> <li> the <strong>Workspace </strong> in the middle and</li> <li> the <strong>Chart Widget </strong> the left hand side!</li></p>";
    info += "<p>Hitting <strong>F4</strong> or the light bulb button in the upper part toggles a message box in the part below the mainwindows.</p>";
    info += "<p>Sometimes critical information are printed out there. If there are unread critical messages, the light bulb glows red.</p>";
    info += "<p>Uncritical information are indicated by a green light blub.</p>";
    info += "<p><strong>All</strong> tooltips can globally disabled in the config dialog.</p>";
    QMessageBox::about(this, tr("First Start Information"), info);
}

void SupraFitGui::closeEvent(QCloseEvent* event)
{
    if (qApp->instance()->property("ask_on_exit").toBool()) {
        QCheckBox* checkbox = new QCheckBox;
        checkbox->setText(tr("Don't ask this again!"));
        QMessageBox question(QMessageBox::Question, tr("About to close"), tr("Do you really want to close this window?"), QMessageBox::Yes | QMessageBox::No, this);
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
    m_project_tree->setActiveIndex(widget);
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
    if (!index.isValid()) {
        Info(tr("Sorry, but the current selection is invalid. You may have mist the tree item you wanted."));
        return;
    }

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
    QMessageBox question(QMessageBox::Question, tr("About to close all projects"), tr("Do you really want to close all open projects?"), QMessageBox::Yes | QMessageBox::No, this);
    if (question.exec() == QMessageBox::No) {
        return;
    }

    Waiter wait;
    for(int i = m_data_list.size() -1; i >= 0; --i)
    {
        MainWindow* mainwindow = m_project_list.takeAt(i);
        m_stack_widget->removeWidget(mainwindow);
        m_data_list.remove(i);
        m_hashed_data.remove(mainwindow->Data()->UUID());
        delete mainwindow;
        m_project_tree->UpdateStructure();
    }
    for (int i = m_meta_models.size() - 1; i >= 0; --i)
        m_meta_models.remove(i);

    m_supr_file.clear();
    m_filename_line->clear();

    m_project_tree->UpdateStructure();
    UpdateRecentList();
}

void SupraFitGui::SaveData(const QModelIndex& index)
{
    if (!index.isValid()) {
        Info(tr("Sorry, but the current selection is invalid. You may have missed the tree item you wanted."));
        return;
    }

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

void SupraFitGui::Duplicate(const QModelIndex& index)
{
    // qDebug() << m_project_view->currentIndex() << index << m_project_view->selectionModel()->selectedIndexes();
    if (!index.isValid()) {
        Info(tr("Sorry, but the current selection is invalid. You may have missed the tree item you wanted. (List is empty, no project loaded?)"));
        return;
    }

    if (index.parent().isValid()) {
        Info(tr("Sorry, but the current selection is invalid. You can only add up or substract data to projects, not to models in projects."));
        return;
    }

    if (index.row() >= m_project_list.size()) {
        Info(tr("This is something, that should be impossbile."));
        return;
    }

    if (m_project_list[index.row()]->isMetaModel()) {
        Info(tr("Tables of MetaModels can not be manipulated. Sorry for that."));
        return;
    }

    QJsonObject d;
    QJsonObject project = m_data_list[index.row()].toStrongRef().data()->ExportData();
    project["title"] = QString(project["title"].toString() + " - Copy");
    QUuid uuid;
    project["uuid"] = uuid.createUuid().toString();

    d["data"] = project;
    LoadJson(d);
}

void SupraFitGui::AddUpData(const QModelIndex& index, bool sign)
{
    // qDebug() << m_project_view->currentIndex() << index << m_project_view->selectionModel()->selectedIndexes();
    if (!index.isValid()) {
        Info(tr("Sorry, but the current selection is invalid. You may have missed the tree item you wanted. (List is empty, no project loaded?)"));
        return;
    }

    if (index.parent().isValid()) {
        Info(tr("Sorry, but the current selection is invalid. You can only add up or substract data to projects, not to models in projects."));
        return;
    }

    if (index.row() >= m_project_list.size()) {
        Info(tr("This is something, that should be impossbile."));
        return;
    }

    if (m_project_list[index.row()]->isMetaModel()) {
        Info(tr("Tables of MetaModels can not be manipulated. Sorry for that."));
        return;
    }
    QString filetypes = m_supported_files;
    filetypes.remove("*.ITC").remove("*.itc");
    QString str = QFileDialog::getOpenFileName(this, tr("Open File"), getDir(), filetypes);
    if (str.isEmpty() || str.isNull())
        return;

    FileHandler* handler = new FileHandler(str);

    handler->LoadFile();
    QJsonObject blob;
    DataTable* table;
    if (handler->Type() == FileHandler::SupraFit) {
        blob = handler->getJsonData();
        table = new DataTable(blob["data"].toObject()["dependent"].toObject());

    } else if (handler->Type() == FileHandler::dH) {
        blob = handler->getJsonData();
        table = new DataTable(blob["dependent"].toObject());
    } else {
        blob = handler->getJsonData();
        table = new DataTable(blob["dependent"].toObject());
    }
    delete handler;

    if (table->rowCount() == m_data_list[index.row()].toStrongRef().data()->DependentModel()->rowCount() && table->columnCount() == m_data_list[index.row()].toStrongRef().data()->DependentModel()->columnCount()) {
        if (sign)
            m_data_list[index.row()].toStrongRef().data()->DependentModel()->Table() += table->Table();
        else
            m_data_list[index.row()].toStrongRef().data()->DependentModel()->Table() -= table->Table();

        m_data_list[index.row()].toStrongRef().data()->Updated();
    } else
        Info(tr("Sorry, the table you just loaded and the target table do not fit."));

    delete table;
}

void SupraFitGui::CopySystemParameter(const QModelIndex& source, int position)
{
    if (position >= m_data_list.size())
        return;

    QStringList uuids = m_project_tree->UUID(source).split("|");
    if (uuids.size() != 1)
        return;

    QPointer<DataClass> data = m_hashed_data[uuids[0]].toStrongRef().data();

    for (int i = 0; i < m_meta_models.size(); ++i) {
        if (m_meta_models[i].toStrongRef().data()->UUID() == data->UUID())
            return;
    }
    m_data_list[position].toStrongRef().data()->OverrideSystemParameter(data->SysPar());
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
        //    m_project_list[data]->Model(model)->Calculate();
    } else
        qDebug() << "not found" << data << model;
}

void SupraFitGui::ExportAllPlain()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose directory"), getDir());
    if (dir.isEmpty() || dir.isNull())
        return;

    for (int i = 0; i < m_data_list.size(); ++i) {
        QString input, name = m_data_list[i].toStrongRef().data()->ProjectTitle();
        QPointer<DataTable> indep_model = m_data_list[i].toStrongRef().data()->IndependentModel();
        QPointer<DataTable> dep_model = m_data_list[i].toStrongRef().data()->DependentModel();

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
        QString name = m_data_list[i].toStrongRef().data()->ProjectTitle();
        QJsonObject data;
        data["data"] = m_data_list[i].toStrongRef().data()->ExportData();

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
        m_data_list[i].toStrongRef().data()->DependentModel()->Table() += table->Table();
        m_data_list[i].toStrongRef().data()->Updated();
    }
    delete table;
}

void SupraFitGui::LicenseInfo()
{
    QTabWidget* licence_tab = new QTabWidget;
    QPlainTextEdit* text = new QPlainTextEdit;
    text->setReadOnly(true);

    QString content;
    QFile file(":/LICENSE.md");
    if (file.open(QIODevice::ReadOnly)) {
        content += QString(file.readAll());
    }

    text->setPlainText(content);
    licence_tab->addTab(text, tr("Licence for SupraFit"));

    text = new QPlainTextEdit;
    text->setReadOnly(true);

    file.close();
    content.clear();
    file.setFileName(":/BSD.md");
    if (file.open(QIODevice::ReadOnly)) {
        content += QString(file.readAll());
    }

    text->setPlainText(content);
    licence_tab->addTab(text, tr("Licence for FlowLayout"));

#ifdef noto_font
    file.close();
    content.clear();
    text = new QPlainTextEdit;
    text->setReadOnly(true);

    file.setFileName(":/fonts/LICENSE");
    if (file.open(QIODevice::ReadOnly)) {
        content += QString(file.readAll());
    } else
        qDebug() << file.errorString() << file.fileName();
    text->setPlainText(content);
    licence_tab->addTab(text, tr("Licence for NotoFons"));
#endif

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(licence_tab);
    QDialog dialog;
    dialog.setWindowTitle(tr("About the Licences used in SupraFit"));
    dialog.setLayout(layout);
    dialog.resize(800, 600);
    dialog.exec();
}

void SupraFitGui::resizeEvent(QResizeEvent* event)
{
    m_logolabel->UpdatePixmap();
    QMainWindow::resizeEvent(event);
}
