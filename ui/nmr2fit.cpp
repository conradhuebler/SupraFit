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

#include "core/data/dataclass.h"
#include "core/data/modelclass.h"
#include "ui/dialogs/configdialog.h"

#include "ui/dialogs/importdata.h"
#include "ui/widgets/modelwidget.h"
#include "ui/widgets/datawidget.h"
#include "ui/widgets/chartwidget.h"
#include "ui/widgets/modeldataholder.h"
#include <QtGui/QApplication>
#include <QtCore/QSharedPointer>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>
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
#include <QDebug>

#include <stdio.h>

#include "nmr2fit.h"

MainWindow::MainWindow() :m_hasData(false)
{
    
    m_logdock = new QDockWidget(tr("Logging output"), this);
    m_logWidget = new QPlainTextEdit(this);
    m_logdock->setWidget(m_logWidget);
    addDockWidget(Qt::BottomDockWidgetArea, m_logdock);
    
    m_stdout.open(stdout, QIODevice::WriteOnly);
    
    
//     m_mainsplitter = new QSplitter(Qt::Horizontal);
    
    
    
//     setCentralWidget(m_mainsplitter);
    
    
    
    m_model_dataholder = new ModelDataHolder;
    m_modeldock = new QDockWidget(tr("Data and Models"), this);
    m_modeldock->setWidget(m_model_dataholder);
    addDockWidget(Qt::LeftDockWidgetArea, m_modeldock);
    
    connect(m_model_dataholder, SIGNAL(Message(QString, int)), this, SLOT(WriteMessages(QString, int)));
    connect(m_model_dataholder, SIGNAL(MessageBox(QString, int)), this, SLOT(MessageBox(QString, int)));
    
    m_charts = new ChartWidget;
    m_model_dataholder->setChartWidget(m_charts);
    m_chartdock = new QDockWidget(tr("Charts"), this);
    m_chartdock->setWidget(m_charts);
    addDockWidget(Qt::RightDockWidgetArea, m_chartdock);
    
    m_new = new QAction(QIcon::fromTheme("document-new"), tr("New Table"));
    connect(m_new, SIGNAL(triggered(bool)), this, SLOT(NewTable()));
    
    m_import = new QAction(QIcon::fromTheme("document-open"), tr("Import Table"));
    connect(m_import, SIGNAL(triggered(bool)), this, SLOT(ImportAction()));
    
    m_edit = new QAction(QIcon::fromTheme("document-edit"), tr("Edit Data"));
    connect(m_edit, SIGNAL(triggered(bool)), this, SLOT(EditAction()));   
    
    m_config = new QAction(QIcon::fromTheme("applications-system"), tr("Settings"));
    connect(m_config, SIGNAL(triggered()), SLOT(SettingsDialog()) );
    
    //     m_about = new QAction(QIcon::fromTheme("help-about"), tr("About"));
    
    m_close= new QAction(QIcon::fromTheme("application-exit"), tr("Quit"));
    connect(m_close, SIGNAL(triggered()), SLOT(close()) );
    
    
    m_main_toolbar = new QToolBar;
    m_main_toolbar->addAction(m_new);
    m_main_toolbar->addAction(m_import);
    m_main_toolbar->addAction(m_edit);
    m_main_toolbar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    addToolBar(m_main_toolbar);
    
    m_model_toolbar = new QToolBar;
    m_model_toolbar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    addToolBar(m_model_toolbar);
    m_system_toolbar = new QToolBar;
    m_system_toolbar->addAction(m_config);
    //     m_system_toolbar->addAction(QIcon::fromTheme("application-exit"), tr("Quit"), qApp, &QApplication::aboutQt);
    m_system_toolbar->addAction(m_close);
    m_system_toolbar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    addToolBar(m_system_toolbar);
    
    ReadSettings();
    LogFile();
}

MainWindow::~MainWindow()
{
    
    m_stdout.close();
    QSettings _settings;
    
    _settings.beginGroup("window");
    _settings.setValue("geometry", size());
    _settings.endGroup();
    
}

void MainWindow::NewTable()
{
}


void MainWindow::LoadData()
{
    
    
    
    
}

void MainWindow::ImportAction(const QString& file)
{
    ImportData dialog(file, this);
    
    if(dialog.exec() == QDialog::Accepted)
    {
        m_data = QSharedPointer<DataClass>(new DataClass(dialog.getStoredData()));
        m_model_dataholder->setData(m_data.data());
        m_charts->setRawData(m_data.data());
        m_hasData = true;
    }else
        destroy();
}


void MainWindow::ImportAction()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", ".");
    if(filename.isEmpty())
        return;
    if(!m_hasData)
    {
        ImportData dialog(filename, this);
        
        if(dialog.exec() == QDialog::Accepted)
        {
            m_data = QSharedPointer<DataClass>(new DataClass(dialog.getStoredData()));
            m_model_dataholder->setData(m_data.data());
            m_charts->setRawData(m_data.data());
            m_hasData = true;
        }
    }else
    {
        MainWindow *nw = new MainWindow;
        nw->show();
        nw->ImportAction(filename);
    }
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
}

void MainWindow::WriteMessages(const QString &message, int priority)
{

    QTextStream stdout_stream(&m_stdout);
    stdout_stream << message << "\n";
    
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
            QMessageBox::critical(this, tr("Optimizer Error."),
                                     str,
                                     QMessageBox::Ok | QMessageBox::Default);
    }else if(priority == 1)
    {
        QMessageBox::warning(this, tr("Optimizer warning."),
                                     str,                            QMessageBox::Ok | QMessageBox::Default);
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
    resize(_settings.value("geometry", sizeHint()).toSize());
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
#include "nmr2fit.moc"
