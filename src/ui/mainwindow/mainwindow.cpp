/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/jsonhandler.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

#include "src/ui/dialogs/configdialog.h"
#include "src/ui/dialogs/importdata.h"

#include "src/ui/guitools/guitools.h"

#include "src/ui/mainwindow/chartwidget.h"
#include "src/ui/mainwindow/datawidget.h"
#include "src/ui/mainwindow/modeldataholder.h"
#include "src/ui/mainwindow/modelwidget.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtCore/QSettings>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>
#include <QtCore/QWeakPointer>

#include <QtWidgets/QAction>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>

#include <stdio.h>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
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

    m_mainsplitter = new QSplitter(Qt::Horizontal);
    m_mainsplitter->setObjectName("model_splitter");
    m_mainsplitter->addWidget(m_modeldock);
    m_mainsplitter->addWidget(m_chartdock);
    m_mainsplitter->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    /*
    QGridLayout* layout = new QGridLayout;

    layout->addWidget(m_mainsplitter, 0, 0);
    setLayout(layout);
    */
    setCentralWidget(m_mainsplitter);

    // connect(m_model_dataholder, SIGNAL(nameChanged()), this, SLOT(setWindowTitle()));
    connect(m_model_dataholder, &ModelDataHolder::ModelAdded, this, &MainWindow::ModelsChanged);
    connect(m_model_dataholder, &ModelDataHolder::ModelRemoved, this, &MainWindow::ModelsChanged);
    /*
    setStyleSheet("QSplitter::handle:vertical {"
                  "background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
                  "stop:0 #eee, stop:1 #ccc);"
                  "border: 1px solid #777;"
                  "height: 1px;"
                  "margin-top: 2px;"
                  "margin-bottom: 2px;"
                  "border-radius: 4px;"
                  "}");*/
    setWindowFlag(Qt::Widget);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    ReadGeometry();
}

MainWindow::~MainWindow()
{
    disconnect(m_data.data());
    disconnect(m_model_dataholder);
    m_model_dataholder->CloseAllForced();
    m_data.clear();
    WriteSettings();
}

QSharedPointer<DataClass> MainWindow::SetData(const QJsonObject& object)
{
    QString colors = object["data"].toObject()["colors"].toString();
    m_data = QSharedPointer<DataClass>(new DataClass());

    connect(m_data.data(), &DataClass::Message, this, &MainWindow::Message);
    connect(m_data.data(), &DataClass::Warning, this, &MainWindow::Warning);

    if (!m_data->LegacyImportData(object["data"].toObject()))
        return m_data;

    QSharedPointer<ChartWrapper> wrapper = m_charts->setRawData(m_data);
    if (!colors.isEmpty() && !colors.isNull())
        wrapper->setColorList(colors);
    m_model_dataholder->setData(m_data, wrapper);

    QJsonObject toplevel;
    m_model_dataholder->AddToWorkspace(object);

    return m_data;
}

QSharedPointer<AbstractModel> MainWindow::CreateMetaModel(const QWeakPointer<ChartWrapper>& wrapper)
{
    m_data = QSharedPointer<DataClass>(new DataClass(this));

    m_data->setProjectTitle("MetaModel");
    QSharedPointer<AbstractModel> model = CreateModel(SupraFit::MetaModel, m_data);
    QSharedPointer<ChartWrapper> w = m_charts->setRawWrapper(wrapper);

    m_model_dataholder->setData(model, w);

    connect(qobject_cast<MetaModel*>(model.data()), &MetaModel::ModelAdded, m_model_dataholder, &ModelDataHolder::addMetaModel);

    /* lets connect the meta model, that when ever a new model is added 
     * when then connect the associated model widget after the color changed with something about changing the scater series color */

    connect(qobject_cast<MetaModel*>(model.data()), &MetaModel::ModelAdded, this, [this]() {
        int series = this->m_model_dataholder->getChartWrapper().toStrongRef()->SeriesSize();
        if (qobject_cast<ScatterSeries*>(this->m_model_dataholder->getChartWrapper().toStrongRef()->Series(series - 1))) {
            ScatterSeries* serie = qobject_cast<ScatterSeries*>(this->m_model_dataholder->getChartWrapper().toStrongRef()->Series(series - 1));
            connect(this->m_model_dataholder->RecentModel(), &ModelWidget::ColorChanged, serie, &ScatterSeries::setColor);
            connect(this->m_model_dataholder->RecentModel(), &ModelWidget::ToggleSeries, serie, [serie]() {
                serie->setVisible(!serie->isVisible());
            });
            serie->setColor(this->m_model_dataholder->RecentModel()->ActiveColor());
        }
    });
    m_meta_model = true;

    return model;
}

void MainWindow::setCurrentTab(int index)
{
    m_model_dataholder->setCurrentTab(index);
}

void MainWindow::ReadGeometry()
{
    QSettings _settings;
    _settings.beginGroup("model");
    restoreGeometry(_settings.value("geometry").toByteArray());
    restoreState(_settings.value("state").toByteArray());
    _settings.endGroup();
}

void MainWindow::WriteSettings()
{
    QSettings _settings;
    _settings.beginGroup("model");
    _settings.setValue("geometry", saveGeometry());
    _settings.setValue("state", saveState());
    _settings.endGroup();
}

#include "mainwindow.moc"
