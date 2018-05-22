/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/ui/mainwindow/modeldataholder.h"
#include "src/ui/mainwindow/modelwidget.h"

#include <QDebug>

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
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>

#include <stdio.h>

#include "suprafit.h"

MainWindow::MainWindow()
{
    m_instance = new Instance;
    Instance::setInstance(m_instance);

    ReadSettings();

    m_model_dataholder = new ModelDataHolder;
    m_modeldock = new QDockWidget(tr("Workspace"), this);
    m_modeldock->setObjectName(tr("data_and_models"));
    m_modeldock->setToolTip(tr("This <strong>workspace widget</strong> contains all open models and allows them to be manipulated!"));
    m_modeldock->setWidget(m_model_dataholder);
    m_modeldock->setTitleBarWidget(m_model_dataholder->TitleBarWidget());
    m_modeldock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);

    connect(m_model_dataholder, SIGNAL(Message(QString, int)), this, SLOT(WriteMessages(QString, int)), Qt::DirectConnection);
    connect(m_model_dataholder, SIGNAL(MessageBox(QString, int)), this, SLOT(MessageBox(QString, int)), Qt::DirectConnection);

    m_charts = new ChartWidget;
    m_model_dataholder->setChartWidget(m_charts);
    m_chartdock = new QDockWidget(tr("Charts"), this);
    m_chartdock->setObjectName(tr("charts"));
    m_chartdock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    m_chartdock->setWidget(m_charts);
    m_chartdock->setTitleBarWidget(m_charts->TitleBarWidget());
    m_chartdock->setToolTip(tr("This <strong>chart widget</strong> contains the charts for the calculated models and the model errors!"));
    connect(m_charts->TitleBarWidget(), &ChartDockTitleBar::close, m_chartdock, &QDockWidget::close);

    m_logdock = new QDockWidget(tr("Logging output"), this);
    m_logdock->setObjectName(tr("logging"));
    m_logWidget = new QPlainTextEdit(this);
    m_logdock->setWidget(m_logWidget);
    connect(this, SIGNAL(AppendPlainText(QString)), m_logWidget, SLOT(appendPlainText(QString)));
    m_logdock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    m_logdock->setToolTip(tr("The <strong>log widget</strong> contains the log output of SupraFit!"));

    m_logdock->hide();

    m_stdout.open(stdout, QIODevice::WriteOnly);

    m_historywidget = new ModelHistory(this);
    QScrollArea* history_scroll = new QScrollArea(this);
    history_scroll->setWidget(m_historywidget);
    history_scroll->setWidgetResizable(true);
    history_scroll->setAlignment(Qt::AlignTop);
    m_history_dock = new QDockWidget("Models Stack");
    m_history_dock->setObjectName(tr("history"));
    m_history_dock->setWidget(history_scroll);
    m_history_dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    m_history_dock->setMaximumWidth(240);
    m_history_dock->setMinimumWidth(240);
    m_history_dock->setToolTip(tr("This widget contains the <strong>stack</strong>, where <strong>models</strong> appear!"));

    addDockWidget(Qt::LeftDockWidgetArea, m_history_dock, Qt::Horizontal);
    addDockWidget(Qt::LeftDockWidgetArea, m_modeldock, Qt::Horizontal);
    addDockWidget(Qt::RightDockWidgetArea, m_chartdock);
    addDockWidget(Qt::BottomDockWidgetArea, m_logdock);

    connect(m_model_dataholder, SIGNAL(InsertModel(QJsonObject, int)), this, SLOT(InsertHistoryElement(QJsonObject, int)), Qt::DirectConnection);
    connect(m_model_dataholder, SIGNAL(nameChanged()), this, SLOT(setWindowTitle()));
    connect(m_model_dataholder, SIGNAL(InsertModel(QJsonObject)), this, SLOT(InsertHistoryElement(QJsonObject)), Qt::DirectConnection);
    connect(m_historywidget, SIGNAL(AddJson(QJsonObject)), m_model_dataholder, SLOT(AddToWorkspace(QJsonObject)));
    connect(m_historywidget, SIGNAL(LoadJson(QJsonObject)), m_model_dataholder, SLOT(LoadCurrentProject(QJsonObject)));
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::AnimatedDocks | QMainWindow::VerticalTabs);

    m_new = new QAction(Icon("document-new"), tr("New Table"));
    connect(m_new, SIGNAL(triggered(bool)), this, SLOT(NewTable()));

    m_load = new QAction(Icon("document-open"), tr("Open Project"));
    connect(m_load, SIGNAL(triggered(bool)), this, SLOT(OpenFile()));

    m_save = new QAction(Icon("document-save-all"), tr("Save Project"));
    connect(m_save, SIGNAL(triggered(bool)), this, SLOT(SaveProjectAction()));

    m_edit = new QAction(Icon("document-edit"), tr("Edit Data"));
    //m_edit->setCheckable(true);
    //m_edit->setChecked(false);
    //connect(m_edit, SIGNAL(toggled(bool)), m_model_dataholder, SLOT(EditTableAction(bool)));
    connect(m_edit, &QAction::triggered, this, &MainWindow::EditData);

    m_importmodel = new QAction(Icon("document-import"), tr("Import Models"));
    connect(m_importmodel, SIGNAL(triggered(bool)), this, SLOT(ImportModelAction()));

    m_export = new QAction(Icon("document-export"), tr("Export Models"));
    connect(m_export, SIGNAL(triggered(bool)), this, SLOT(ExportModelAction()));

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
    m_main_toolbar->addAction(m_new);
    m_main_toolbar->addAction(m_load);
    m_main_toolbar->addAction(m_save);
    m_main_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    addToolBar(m_main_toolbar);

    m_model_toolbar = new QToolBar;
    m_model_toolbar->setObjectName(tr("model_toolbar"));
    m_model_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_model_toolbar->addAction(m_edit);
    m_model_toolbar->addAction(m_importmodel);
    m_model_toolbar->addAction(m_export);
    addToolBar(m_model_toolbar);

    m_system_toolbar = new QToolBar;
    m_system_toolbar->setObjectName(tr("system_toolbar"));
    QAction* toggleView;

    toggleView = m_modeldock->toggleViewAction();
    toggleView->setIcon(QIcon(":/icons/workspace.png"));
    m_system_toolbar->addAction(toggleView);

    toggleView = m_chartdock->toggleViewAction();
    toggleView->setIcon(QIcon(":/icons/charts.png"));
    m_system_toolbar->addAction(toggleView);

    toggleView = m_logdock->toggleViewAction();
    toggleView->setIcon(Icon("text-field"));
    m_system_toolbar->addAction(toggleView);

    toggleView = m_history_dock->toggleViewAction();
    toggleView->setIcon(Icon("view-list-text"));
    m_system_toolbar->addAction(toggleView);
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
}

MainWindow::~MainWindow()
{
    if (m_instance)
        delete m_instance;
}

QIcon MainWindow::Icon(const QString& str)
{
#ifdef _Theme
    return QIcon::fromTheme(str);
#else
    return QIcon(":/icons/" + str + ".png");
#endif
}

void MainWindow::LoadFile(const QString& file)
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

void MainWindow::setActionEnabled(bool enabled)
{
    m_save->setEnabled(enabled);
    m_export->setEnabled(enabled);
    m_edit->setEnabled(enabled);
    m_importmodel->setEnabled(enabled);
}

bool MainWindow::SetData(QPointer<const DataClass> dataclass, const QString& str, const QString& colors)
{
    if (!m_titration_data.isNull()) {
        MainWindow* mainwindow = new MainWindow;
        mainwindow->SetData(dataclass, str);
        mainwindow->show();
        return false;
    } else {
        QFileInfo info(str);
        qApp->instance()->setProperty("projectpath", str);
        qApp->instance()->setProperty("projectname", info.baseName());
        m_titration_data = QSharedPointer<DataClass>(new DataClass((dataclass)));
        QSharedPointer<ChartWrapper> wrapper = m_charts->setRawData(m_titration_data);
        if (!colors.isEmpty() && !colors.isNull())
            wrapper->setColorList(colors);
        m_model_dataholder->setData(m_titration_data, wrapper);

        setActionEnabled(true);

        QJsonObject toplevel;
        if (JsonHandler::ReadJsonFile(toplevel, str)) {
            m_model_dataholder->AddToWorkspace(toplevel);
        }

        if (m_model_dataholder->CheckCrashFile()) {
            QMessageBox::StandardButton replay;
            QString app_name = QString(qApp->instance()->applicationName());
            replay = QMessageBox::information(this, tr("Old Models found."), tr("It seems %1 crashed (unfortunately)!\nShall I recover the last models?").arg(app_name), QMessageBox::Yes | QMessageBox::No);
            if (replay == QMessageBox::Yes) {
                QJsonObject toplevel;
                if (JsonHandler::ReadJsonFile(toplevel, qApp->instance()->property("projectpath").toString() + ".crashsave.suprafit"))
                    m_model_dataholder->AddToWorkspace(toplevel);
            }
        }
        setWindowTitle();
        return true;
    }
}

void MainWindow::NewTable()
{
    ImportData dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        if (dialog.ProjectFile().isEmpty())
            SetData(new DataClass(dialog.getStoredData()));
        else
            LoadProject(dialog.ProjectFile());
    }
}

void MainWindow::OpenFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", getDir(), tr("Supported files (*.suprafit *.json *.jdat *.txt *.dat, *.itc, *.ITC);;Json File (*.json);;SupraFit Project File  (*.suprafit);;Table Files (*.dat *.txt, *.itc);;All files (*.*)"));
    if (filename.isEmpty())
        return;
    setLastDir(filename);
    LoadFile(filename);
}

void MainWindow::ImportTable(const QString& file)
{
    ImportData dialog(file, this);

    if (dialog.exec() == QDialog::Accepted) {
        if (dialog.ProjectFile().isEmpty())
            SetData(new DataClass(dialog.getStoredData()), file);
        else
            LoadProject(dialog.ProjectFile());
    }
}

bool MainWindow::LoadProject(const QString& filename)
{
    QJsonObject toplevel;
    if (JsonHandler::ReadJsonFile(toplevel, filename)) {
        QPointer<const DataClass> data = new DataClass(toplevel["data"].toObject());
        if (data->DataPoints() != 0) {
            SetData(data, filename, toplevel["data"].toObject()["colors"].toString());
            return true;
        } else
            return false;
    }
    return false;
}

void MainWindow::SaveProjectAction()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("SupraFit Project File  (*.suprafit);;Json File (*.json);;All files (*.*)"));
    if (!str.isEmpty()) {
        setLastDir(str);
        m_model_dataholder->SaveWorkspace(str);
    }
}

void MainWindow::ImportModelAction()
{
    QString str = QFileDialog::getOpenFileName(this, tr("Open File"), getDir(), tr("SupraFit Project File  (*.suprafit *.jdat);;Json File (*.json);;All files (*.*)"));
    if (!str.isEmpty()) {
        setLastDir(str);
        QJsonObject toplevel;
        if (JsonHandler::ReadJsonFile(toplevel, str)) {
            m_model_dataholder->AddToWorkspace(toplevel);
        }
    }
}

void MainWindow::ExportModelAction()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("SupraFit Project File (*.suprafit);;Json File (*.json);;All files (*.*)"));
    if (!str.isEmpty()) {
        setLastDir(str);
        m_model_dataholder->SaveCurrentModels(str);
    }
}

void MainWindow::SettingsDialog()
{
    ConfigDialog dialog(m_opt_config, m_printlevel, m_logfile, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_opt_config = dialog.Config();
        m_model_dataholder->setSettings(m_opt_config);
        m_printlevel = dialog.PrintLevel();
        m_logfile = dialog.LogFile();
        WriteSettings();
    }
}

void MainWindow::LogFile()
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

void MainWindow::WriteMessages(const QString& message, int priority)
{
    //     QTextStream stdout_stream(&m_stdout);
    //      stdout_stream << message << "\n";
    emit AppendPlainText(message);
    return;
    if (priority <= m_printlevel) {
        QTextStream fileout_stream(&m_file);
        fileout_stream << message << "\n";

        QTimer::singleShot(0, m_logWidget, SLOT(appendPlainText(message)));
    }
}

void MainWindow::setWindowTitle()
{
    QMainWindow::setWindowTitle(QString("*" + qApp->instance()->property("projectname").toString() + "*"));
}

void MainWindow::MessageBox(const QString& str, int priority)
{
    if (priority == 0) {
        QMessageBox::critical(this, tr("Optimizer Error."), str, QMessageBox::Ok | QMessageBox::Default);
    } else if (priority == 1) {
        QMessageBox::warning(this, tr("Optimizer warning."), str, QMessageBox::Ok | QMessageBox::Default);
    }
}

void MainWindow::ReadSettings()
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

void MainWindow::ReadGeometry()
{
    QSettings _settings;
    _settings.beginGroup("window");
    restoreGeometry(_settings.value("geometry").toByteArray());
    restoreState(_settings.value("state").toByteArray());
    _settings.endGroup();
}

void MainWindow::WriteSettings(bool ignore_window_state)
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

void MainWindow::InsertHistoryElement(const QJsonObject& model)
{
    m_historywidget->InsertElement(model);
}

void MainWindow::InsertHistoryElement(const QJsonObject& model, int active)
{
    m_historywidget->InsertElement(model, active);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About this application"), SupraFit::aboutHtml());
}

void MainWindow::FirstStart()
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

void MainWindow::closeEvent(QCloseEvent* event)
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
        m_model_dataholder->SaveWorkspace(filename);
    }

    m_stdout.close();

    WriteSettings(false);

    if (m_model_dataholder)
        delete m_model_dataholder;
    if (m_charts)
        delete m_charts;
    if (m_historywidget)
        delete m_historywidget;

    QMainWindow::closeEvent(event);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::ToolTip) {
        return !qApp->instance()->property("tooltips").toBool();
    } else
        return QMainWindow::eventFilter(obj, event);
}

void MainWindow::EditData()
{
    int version = m_titration_data->ExportData()["SupraFit"].toInt();
    if (version < 1602) {
        QMessageBox::information(this, tr("Old SupraFit file"), tr("This is an older SupraFit file, you can only edit the table in Workspace!"));

        m_edit->setCheckable(true);
        m_model_dataholder->EditTableAction(!m_edit->isChecked());
        m_edit->setChecked(!m_edit->isChecked());
    } else {
        ImportData dialog(m_titration_data);
        if (dialog.exec() == QDialog::Accepted) { // I dont like this either ....
            {
                if (m_titration_data->DataType() == DataClassPrivate::Thermogram)
                    m_titration_data->ImportData(dialog.getStoredData().ExportData());
            }
        }
    }
}

#include "suprafit.moc"
