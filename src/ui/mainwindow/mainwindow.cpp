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

#include "mainwindow.h"

MainWindow::MainWindow()
{
    m_model_dataholder = new ModelDataHolder;
    m_modeldock = new QDockWidget(tr("Workspace"), this);
    m_modeldock->setObjectName(tr("data_and_models"));
    m_modeldock->setToolTip(tr("This <strong>workspace widget</strong> contains all open models and allows them to be manipulated!"));
    m_modeldock->setWidget(m_model_dataholder);
    m_modeldock->setTitleBarWidget(m_model_dataholder->TitleBarWidget());
    m_modeldock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);

    m_charts = new ChartWidget;
    m_model_dataholder->setChartWidget(m_charts);
    m_chartdock = new QDockWidget(tr("Charts"), this);
    m_chartdock->setObjectName(tr("charts"));
    m_chartdock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    m_chartdock->setWidget(m_charts);
    m_chartdock->setTitleBarWidget(m_charts->TitleBarWidget());
    m_chartdock->setToolTip(tr("This <strong>chart widget</strong> contains the charts for the calculated models and the model errors!"));
    connect(m_charts->TitleBarWidget(), &ChartDockTitleBar::close, m_chartdock, &QDockWidget::close);

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

    connect(m_model_dataholder, SIGNAL(InsertModel(QJsonObject, int)), this, SLOT(InsertHistoryElement(QJsonObject, int)), Qt::DirectConnection);
    // connect(m_model_dataholder, SIGNAL(nameChanged()), this, SLOT(setWindowTitle()));
    connect(m_model_dataholder, SIGNAL(InsertModel(QJsonObject)), this, SLOT(InsertHistoryElement(QJsonObject)), Qt::DirectConnection);
    connect(m_historywidget, SIGNAL(AddJson(QJsonObject)), m_model_dataholder, SLOT(AddToWorkspace(QJsonObject)));
    connect(m_historywidget, SIGNAL(LoadJson(QJsonObject)), m_model_dataholder, SLOT(LoadCurrentProject(QJsonObject)));
    connect(m_model_dataholder, &ModelDataHolder::ModelAdded, this, &MainWindow::ModelAdded);

    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::AnimatedDocks | QMainWindow::VerticalTabs);

    ReadGeometry();
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

QSharedPointer<DataClass> MainWindow::SetData(const QJsonObject& object)
{
    QString colors = object["data"].toObject()["colors"].toString();
    m_data = QSharedPointer<DataClass>(new DataClass(object["data"].toObject()));
    QSharedPointer<ChartWrapper> wrapper = m_charts->setRawData(m_data);
    if (!colors.isEmpty() && !colors.isNull())
        wrapper->setColorList(colors);
    m_model_dataholder->setData(m_data, wrapper);

    QJsonObject toplevel;
    m_model_dataholder->AddToWorkspace(object);

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
    return m_data;
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

void MainWindow::EditData()
{
    int version = m_data->ExportData()["SupraFit"].toInt();
    if (version < 1602) {
        QMessageBox::information(this, tr("Old SupraFit file"), tr("This is an older SupraFit file, you can only edit the table in Workspace!"));

        //m_edit->setCheckable(true);
        //m_model_dataholder->EditTableAction(!m_edit->isChecked());
        //m_edit->setChecked(!m_edit->isChecked());
    } else {
        ImportData dialog(m_data);
        if (dialog.exec() == QDialog::Accepted) { // I dont like this either ....
            {
                if (m_data->DataType() == DataClassPrivate::Thermogram)
                    m_data->ImportData(dialog.getStoredData().ExportData());
            }
        }
    }
}

void MainWindow::setCurrentTab(int index)
{
    m_model_dataholder->setCurrentTab(index);
}

#include "mainwindow.moc"
