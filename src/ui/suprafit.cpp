/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/dataclass.h"
#include "src/core/AbstractModel.h"
#include "src/core/jsonhandler.h"

#include "src/ui/dialogs/configdialog.h"
#include "src/ui/dialogs/importdata.h"

#include "src/ui/widgets/modelwidget.h"
#include "src/ui/widgets/datawidget.h"
#include "src/ui/widgets/chartwidget.h"
#include "src/ui/widgets/modeldataholder.h"

#include <QtCore/QSharedPointer>
#include <QtCore/QWeakPointer>
#include <QtCore/QSettings>
#include <QtCore/QJsonObject>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QAction>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMessageBox>

#include <QDebug>

#include <stdio.h>

#include "suprafit.h"

MainWindow::MainWindow() : m_ask_on_exit(true)
{
    m_model_dataholder = new ModelDataHolder;
    m_modeldock = new QDockWidget(tr("Workspace"), this);
    m_modeldock->setObjectName(tr("data_and_models"));
    m_modeldock->setWidget(m_model_dataholder);
    m_modeldock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, m_modeldock);
    
    connect(m_model_dataholder, SIGNAL(Message(QString, int)), this, SLOT(WriteMessages(QString, int)), Qt::DirectConnection);
    connect(m_model_dataholder, SIGNAL(MessageBox(QString, int)), this, SLOT(MessageBox(QString, int)), Qt::DirectConnection);
    
    
    m_charts = new ChartWidget;
    m_model_dataholder->setChartWidget(m_charts);
    m_chartdock = new QDockWidget(tr("Charts"), this);
    m_chartdock->setObjectName(tr("charts"));
    m_chartdock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    m_chartdock->setWidget(m_charts);
    addDockWidget(Qt::RightDockWidgetArea, m_chartdock);
    
    m_logdock = new QDockWidget(tr("Logging output"), this);
    m_logdock->setObjectName(tr("logging"));
    m_logWidget = new QPlainTextEdit(this);
    m_logdock->setWidget(m_logWidget);
    connect(this, SIGNAL(AppendPlainText(QString)), m_logWidget, SLOT(appendPlainText(QString)));
    m_logdock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::BottomDockWidgetArea, m_logdock);
    
    m_stdout.open(stdout, QIODevice::WriteOnly);
    
    m_historywidget = new ModelHistory(&m_history);
    QScrollArea *history_scroll = new QScrollArea(this);
    history_scroll->setWidget(m_historywidget);
        history_scroll->setWidgetResizable(true);
        history_scroll->setAlignment(Qt::AlignTop);
    m_history_dock = new QDockWidget("Models Stack");
    m_history_dock->setObjectName(tr("history"));
    m_history_dock->setWidget(history_scroll);
    m_history_dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, m_history_dock);
    connect(m_model_dataholder, SIGNAL(InsertModel(ModelHistoryElement)), this, SLOT(InsertHistoryElement(ModelHistoryElement)), Qt::DirectConnection);
    connect(m_model_dataholder, SIGNAL(InsertModel(QJsonObject)), this, SLOT(InsertHistoryElement(QJsonObject)), Qt::DirectConnection);
    connect(m_historywidget, SIGNAL(AddJson(QJsonObject)), m_model_dataholder, SLOT(AddToWorkspace(QJsonObject)));
    connect(m_historywidget, SIGNAL(LoadJson(QJsonObject)), m_model_dataholder, SLOT(LoadCurrentProject(QJsonObject)));
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::AnimatedDocks | QMainWindow::VerticalTabs);
    
    m_new = new QAction(QIcon::fromTheme("document-new"), tr("New Table"));
    connect(m_new, SIGNAL(triggered(bool)), this, SLOT(NewTableAction()));
    
    m_import = new QAction(QIcon::fromTheme("document-open"), tr("Import Table"));
    connect(m_import, SIGNAL(triggered(bool)), this, SLOT(ImportTableAction()));
    
    m_load = new QAction(QIcon::fromTheme("document-open"), tr("Load Project"));
    connect(m_load, SIGNAL(triggered(bool)), this, SLOT(LoadProjectAction()));
    
    m_save = new QAction(QIcon::fromTheme("document-save"), tr("Save Project"));
    connect(m_save, SIGNAL(triggered(bool)), this, SLOT(SaveProjectAction()));
    
    m_edit = new QAction(QIcon::fromTheme("document-edit"), tr("Edit Data"));
    connect(m_edit, SIGNAL(triggered(bool)), this, SLOT(EditTableAction()));   
    
    m_importmodel = new QAction(QIcon::fromTheme("document-open"), tr("Import Models"));
    connect(m_importmodel, SIGNAL(triggered(bool)), this, SLOT(ImportModelAction()));
    
    m_export = new QAction(QIcon::fromTheme("document-edit"), tr("Export Models"));
    connect(m_export, SIGNAL(triggered(bool)), this, SLOT(ExportModelAction()));   
    
    m_config = new QAction(QIcon::fromTheme("applications-system"), tr("Settings"));
    connect(m_config, SIGNAL(triggered()), this, SLOT(SettingsDialog()) );
    
    m_close= new QAction(QIcon::fromTheme("application-exit"), tr("Quit"));
    connect(m_close, SIGNAL(triggered()), SLOT(close()) );
    
    
    m_main_toolbar = new QToolBar;
    m_main_toolbar->setObjectName(tr("main_toolbar"));
    m_main_toolbar->addAction(m_new);
    m_main_toolbar->addAction(m_import);
    m_main_toolbar->addAction(m_load);
    m_main_toolbar->addAction(m_save);
    m_main_toolbar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    addToolBar(m_main_toolbar);
    
    m_model_toolbar = new QToolBar;
    m_model_toolbar->setObjectName(tr("model_toolbar"));
    m_model_toolbar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    m_model_toolbar->addAction(m_edit);
    m_model_toolbar->addAction(m_importmodel);
    m_model_toolbar->addAction(m_export);
    addToolBar(m_model_toolbar);
    
    m_system_toolbar = new QToolBar;
    m_system_toolbar->setObjectName(tr("system_toolbar"));
    m_system_toolbar->addAction(m_modeldock->toggleViewAction());
    m_system_toolbar->addAction(m_chartdock->toggleViewAction());
    m_system_toolbar->addAction(m_logdock->toggleViewAction());
    m_system_toolbar->addAction(m_history_dock->toggleViewAction());
    m_system_toolbar->addAction(m_config);
    m_system_toolbar->addAction(m_close);
    m_system_toolbar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    addToolBar(m_system_toolbar);
    
    ReadSettings();
    LogFile();
    setActionEnabled(false);
}

MainWindow::~MainWindow()
{
    
}

void MainWindow::setActionEnabled(bool enabled)
{
    m_save->setEnabled(enabled);
    m_export->setEnabled(enabled);
    m_edit->setEnabled(enabled);
    m_importmodel->setEnabled(enabled);
}

bool MainWindow::SetData(QPointer<const DataClass> dataclass, const QString &str)
{
    if(!m_titration_data.isNull())
    {
        MainWindow *mainwindow = new MainWindow;
        mainwindow->SetData(dataclass, str);
        mainwindow->show();
        return false;
    }else
    {
        QFileInfo info(str);
        qApp->instance()->setProperty("projectpath", str);
        qApp->instance()->setProperty("projectname", info.baseName());
        m_titration_data = m_model_dataholder->setData(new DataClass(dataclass));
        setActionEnabled(true);
        m_charts->setRawData(m_titration_data.data());
        if(m_model_dataholder->CheckCrashFile())
        {
            QMessageBox::StandardButton replay;
            QString app_name = QString(qApp->instance()->applicationName());
            replay = QMessageBox::information(this, tr("Old Models found."), tr("It seems %1 crashed (unfortunately)!\nShall I recover the last models?").arg(app_name), QMessageBox::Yes | QMessageBox::No);
            if(replay == QMessageBox::Yes) 
            {
                QJsonObject toplevel;
                if(JsonHandler::ReadJsonFile(toplevel, qApp->instance()->property("projectpath").toString() + ".crashsave.json"))
                    m_model_dataholder->AddToWorkspace(toplevel);
            }
        }
        return true;
    }
}


void MainWindow::NewTableAction()
{
    ImportData dialog(this);
    if(dialog.exec() == QDialog::Accepted)
        SetData(new DataClass(dialog.getStoredData()));
    
}

void MainWindow::ImportTableAction()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", ".");
    if(filename.isEmpty())
        return;
    ImportData dialog(filename, this);
    if(dialog.exec() == QDialog::Accepted)
        SetData(new DataClass(dialog.getStoredData()), filename);
}

void MainWindow::ImportAction(const QString& file)
{
    ImportData dialog(file, this);
    
    if(dialog.exec() == QDialog::Accepted)
        SetData(new DataClass(dialog.getStoredData()), file);
}

void MainWindow::LoadProjectAction()
{
    QString str = QFileDialog::getOpenFileName(this, tr("Load File"), ".", tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(!str.isEmpty())
    {
        QJsonObject toplevel;
        if(JsonHandler::ReadJsonFile(toplevel, str))
        {
            QPointer<DataClass > data = new DataClass(toplevel);
            if(data->DataPoints() != 0)
            {
                SetData(new DataClass(new DataClass(data)), str);
                m_model_dataholder->AddToWorkspace(toplevel);
            }
            else
                QMessageBox::warning(this, tr("Loading Datas."),  tr("Sorry, but this doesn't contain any titration tables!"),  QMessageBox::Ok | QMessageBox::Default);
        }
    }
}

void MainWindow::SaveProjectAction()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), ".", tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(!str.isEmpty())
    {
        m_model_dataholder->SaveWorkspace(str);
    }
}

void MainWindow::ImportModelAction()
{
    QString str = QFileDialog::getOpenFileName(this, tr("Open File"), ".", tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(!str.isEmpty())
    {
        QJsonObject toplevel;
        if(JsonHandler::ReadJsonFile(toplevel, str))
        {
            m_model_dataholder->AddToWorkspace(toplevel);
        }
    }  
}


void MainWindow::ExportModelAction()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), ".", tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(!str.isEmpty())
    {
        m_model_dataholder->SaveCurrentModels(str);   
    }   
}

void MainWindow::EditTableAction()
{
    
}

void MainWindow::SettingsDialog()
{
    ConfigDialog dialog(m_opt_config, m_printlevel, m_logfile, this);
    if(dialog.exec() == QDialog::Accepted)
    {
        m_opt_config = dialog.Config();
        m_model_dataholder->setSettings(m_opt_config);
        m_printlevel = dialog.PrintLevel();
        m_logfile = dialog.LogFile();
        WriteSettings();
    }
}

void MainWindow::LogFile()
{
    if(m_logfile.isEmpty() && m_file.isOpen())
        m_file.close();
    else if(m_file.fileName() != m_logfile && m_file.isOpen())
    {
        m_file.close();
        m_file.setFileName(m_logfile);
        if (!m_file.open(QIODevice::WriteOnly| QIODevice::Text))
            return;
    }else if(!m_file.isOpen())
    {
        m_file.setFileName(m_logfile);
        if (!m_file.open(QIODevice::WriteOnly| QIODevice::Text))
            return;
    }
    
}

void MainWindow::WriteMessages(const QString &message, int priority)
{
    //     QTextStream stdout_stream(&m_stdout);
    //      stdout_stream << message << "\n";
    emit AppendPlainText(message);
    return;
    if(priority <= m_printlevel)
    {
        QTextStream fileout_stream(&m_file);
        fileout_stream << message << "\n";
        
        QTimer::singleShot(0,m_logWidget, SLOT(appendPlainText(message)));
    }
}


void MainWindow::MessageBox(const QString& str, int priority)
{
    if(priority == 0)
    {    
        QMessageBox::critical(this, tr("Optimizer Error."), str, QMessageBox::Ok | QMessageBox::Default);
    }else if(priority == 1)
    {
        QMessageBox::warning(this, tr("Optimizer warning."),  str,  QMessageBox::Ok | QMessageBox::Default);
    }
}

void MainWindow::ReadSettings()
{
    QSettings _settings;
    _settings.beginGroup("main");
    m_logfile = _settings.value("logfile").toString();
    m_printlevel = _settings.value("printlevel", 3).toInt();
    qApp->instance()->setProperty("threads", _settings.value("threads", QThread::idealThreadCount())); 
    _settings.endGroup();
    
    _settings.beginGroup("window");
    restoreGeometry(_settings.value("geometry").toByteArray());
    restoreState(_settings.value("state").toByteArray());
    m_ask_on_exit = _settings.value("Ask_on_exit", true).toBool();
    _settings.endGroup();
    
}

void MainWindow::WriteSettings(bool ignore_window_state)
{
    QSettings _settings;
    _settings.beginGroup("main"); 
    _settings.setValue("logfile", m_logfile);
    _settings.setValue("printlevel", m_printlevel);
    _settings.setValue("threads", qApp->instance()->property("threads")); 
    _settings.endGroup();
    
    if(!ignore_window_state)
    {
        _settings.beginGroup("window");
        _settings.setValue("geometry", saveGeometry());
        _settings.setValue("state", saveState());
        _settings.setValue("Ask_on_exit", m_ask_on_exit);
        _settings.endGroup();
    }
}


void MainWindow::InsertHistoryElement(const ModelHistoryElement &element)
{
    int nr = m_history.size();
    m_history[nr] = element;
    m_historywidget->InsertElement(&m_history[nr]);
}

void MainWindow::InsertHistoryElement(const QJsonObject &model)
{
    int nr = m_history.size();
    ModelHistoryElement element;
    element.model = model;
    element.error = model["sse"].toDouble();
    m_history[nr] = element;
    m_historywidget->InsertElement(&m_history[nr]);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(m_ask_on_exit)
    {
        QCheckBox *checkbox = new QCheckBox;
        checkbox->setText(tr("Don't ask this again!"));
        QMessageBox question(QMessageBox::Question, tr("About to close"), tr("Do yout really want to close this window?"), QMessageBox::Yes | QMessageBox::No, this);
        question.setCheckBox(checkbox);
        if(question.exec() == QMessageBox::No)
        {
            m_ask_on_exit = !question.checkBox()->isChecked();
            event->ignore();
            return;
        }
        m_ask_on_exit = !question.checkBox()->isChecked();
    }
        
    m_stdout.close();

    WriteSettings(false);
    QMainWindow::closeEvent(event);
}
#include "suprafit.moc"
