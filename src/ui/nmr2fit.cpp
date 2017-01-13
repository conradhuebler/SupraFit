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

#include <QtWidgets/QApplication>
#include <QtCore/QSharedPointer>
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
#include <QtCore/QSettings>
#include <QtCore/QJsonObject>
#include <QDebug>

#include <stdio.h>

#include "nmr2fit.h"

MainWindow::MainWindow() :m_hasData(false)
{
    
    
    
    m_model_dataholder = new ModelDataHolder;
    m_modeldock = new QDockWidget(tr("Data and Models"), this);
    m_modeldock->setObjectName(tr("data_and_models"));
    m_modeldock->setWidget(m_model_dataholder);
    m_modeldock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, m_modeldock);
    
    connect(m_model_dataholder, SIGNAL(Message(QString, int)), this, SLOT(WriteMessages(QString, int)));
    connect(m_model_dataholder, SIGNAL(MessageBox(QString, int)), this, SLOT(MessageBox(QString, int)));

    
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
    m_logdock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::BottomDockWidgetArea, m_logdock);
    
    m_stdout.open(stdout, QIODevice::WriteOnly);
    
    m_historywidget = new ModelHistory(&m_history);
    m_history_dock = new QDockWidget("Stored Models");
    m_history_dock->setObjectName(tr("history"));
    m_history_dock->setWidget(m_historywidget);
    m_history_dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, m_history_dock);
    connect(m_model_dataholder, SIGNAL(InsertModel(ModelHistoryElement)), this, SLOT(InsertHistoryElement(ModelHistoryElement)));
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
    
    m_stdout.close();
    QSettings _settings;
    
    _settings.beginGroup("window");
//     _settings.setValue("geometry", size());
        _settings.setValue("geometry", saveGeometry());
    _settings.setValue("state", saveState());
//     _settings.setValue("model_dock", m_modeldock->geometry());
//     _settings.setValue("log_dock", m_logdock->geometry());
//     _settings.setValue("chart_dock", m_chartdock->geometry());
//     _settings.setValue("history_dock", m_history_dock->geometry());
    _settings.endGroup();
    
}

void MainWindow::setActionEnabled(bool enabled)
{
    m_save->setEnabled(enabled);
    m_export->setEnabled(enabled);
    m_edit->setEnabled(enabled);
    m_importmodel->setEnabled(enabled);
}

void MainWindow::NewTableAction()
{
    
      
    ImportData dialog(this);
    if(dialog.exec() == QDialog::Accepted)
    {
        m_titration_data = QSharedPointer<DataClass>(new DataClass(dialog.getStoredData()));
        m_model_dataholder->setData(m_titration_data.data());
        m_charts->setRawData(m_titration_data.data());
        m_hasData = true;
        setActionEnabled(true);
    }
    
}

void MainWindow::ImportTableAction()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", ".");
    if(filename.isEmpty())
        return;
    if(!m_hasData)
    {
        ImportData dialog(filename, this);
        
        if(dialog.exec() == QDialog::Accepted)
        {
            m_titration_data = QSharedPointer<DataClass>(new DataClass(dialog.getStoredData()));
            LoadData(filename);
        }
    }else
    {
        MainWindow *nw = new MainWindow;
        nw->show();
        nw->ImportAction(filename);
    }
}

void MainWindow::ImportAction(const QString& file)
{
    ImportData dialog(file, this);
    
    if(dialog.exec() == QDialog::Accepted)
    {
        m_titration_data = QSharedPointer<DataClass>(new DataClass(dialog.getStoredData()));
        LoadData(file);
    }else
        destroy();
}

void MainWindow::LoadProjectAction()
{
    QString str = QFileDialog::getOpenFileName(this, tr("Load File"), ".", tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(!str.isEmpty())
    {
        QJsonObject toplevel;
        if(JsonHandler::ReadJsonFile(toplevel, str))
        {
            m_titration_data = QSharedPointer<DataClass>(new DataClass(toplevel));  
            if(m_titration_data->DataPoints() != 0)
            {
                LoadData(str);
                m_model_dataholder->AddToWorkspace(toplevel);
            }
            else
            {
                m_titration_data.clear();
                QMessageBox::warning(this, tr("Loading Datas."),  tr("Sorry, but this doesn't contain any titration tables!"),  QMessageBox::Ok | QMessageBox::Default);
            }
        }
    }
}

void MainWindow::LoadData(const QString &file)
{
    m_model_dataholder->setData(m_titration_data.data());
    m_charts->setRawData(m_titration_data.data());
    m_hasData = true;
    qApp->instance()->setProperty("projectname", file);
    setActionEnabled(true);

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

void MainWindow::resizeEvent(QResizeEvent* event)
{
    

    
//     m_charts->resize(3*event->size().width()/4, m_charts->height());
//     m_model_dataholder->resize(event->size().width()/4, m_model_dataholder->height());
//     
    QWidget::resizeEvent(event);
    /*
    m_modeldock->setMaximumHeight(4/5*event->size().height());
    m_chartdock->setMaximumHeight(4/5*event->size().height());
    m_logdock->setMaximumHeight(1/5*event->size().height());
    */
}

void MainWindow::WriteMessages(const QString &message, int priority)
{

//     QTextStream stdout_stream(&m_stdout);
//      stdout_stream << message << "\n";
    
    if(priority <= m_printlevel)
    {
        QTextStream fileout_stream(&m_file);
        fileout_stream << message << "\n";
        
        m_logWidget->appendPlainText(message);
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
    _settings.endGroup();
    
    _settings.beginGroup("window");
        restoreGeometry(_settings.value("geometry").toByteArray());
    restoreState(_settings.value("state").toByteArray());
//     resize(_settings.value("geometry", sizeHint()).toSize());
//     m_logdock->setGeometry(_settings.value("log_dock").toRect());
//     m_history_dock->setGeometry(_settings.value("history_dock").toRect());
//     m_chartdock->setGeometry(_settings.value("chart_dock").toRect());
//     m_modeldock->setGeometry(_settings.value("model_dock").toRect());
    _settings.endGroup();
    
}

void MainWindow::WriteSettings()
{
   QSettings _settings;
    _settings.beginGroup("main"); 
    _settings.setValue("logfile", m_logfile);
    _settings.setValue("printlevel", m_printlevel);
    _settings.endGroup();
}


void MainWindow::InsertHistoryElement(const ModelHistoryElement &element)
{
    int nr = m_history.size();
    m_history[nr] = element;
    m_historywidget->InsertElement(&m_history[nr]);
}

#include "nmr2fit.moc"
