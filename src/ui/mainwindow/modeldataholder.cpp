
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

#include "src/capabilities/continuousvariation.h"
#include "src/capabilities/montecarlostatistics.h"

#include "src/core/jsonhandler.h"
#include "src/core/models.h"

#include "src/ui/mainwindow/datawidget.h"
#include "src/ui/mainwindow/modelwidget.h"
#include "src/ui/dialogs/statisticdialog.h"

#include <QApplication>

#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include <QtGui/QColor>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QToolButton>

#include "modeldataholder.h"

TabWidget::TabWidget(QWidget *parent) : QTabWidget(parent)
{
    
}

void TabWidget::addModelsTab(QPointer<ModelWidget> modelwidget)
{
    QScrollArea *scroll = new QScrollArea;
    scroll->setBackgroundRole(QPalette::Midlight);
    scroll->setWidget(modelwidget);
    scroll->setWidgetResizable(true);
    scroll->setAlignment(Qt::AlignHCenter);
    addTab(scroll, modelwidget->Model()->Name());
    
    QCheckBox *hide = new QCheckBox;
    hide->setMaximumSize(20,20);
    hide->setChecked(true);
    hide->setToolTip(tr("Toggle Series in Charts"));
    
    m_color = new QToolButton;
    m_color->setMaximumSize(15,15);
    m_color->setStyleSheet("background-color: #77d740;");
    
    QWidget *tools = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(hide);
    layout->addWidget(m_color);
    tools->setLayout(layout);
    
    connect(hide, SIGNAL(stateChanged(int)), modelwidget, SIGNAL(ToggleSeries(int)));
    connect(m_color, SIGNAL(clicked()), modelwidget, SLOT(ChangeColor()));
    connect(modelwidget, SIGNAL(ColorChanged(QColor)), this, SLOT(ChangeColor(QColor)));
    
    setCurrentWidget(scroll);
    tabBar()->setTabButton(currentIndex(), QTabBar::LeftSide, tools);
}

void TabWidget::setDataTab(QPointer<DataWidget> datawidget)
{
    addTab(datawidget, "Overview: " + qApp->instance()->property("projectname").toString());
    tabBar()->setTabButton(0, QTabBar::RightSide, 0);
}

void TabWidget::ChangeColor(const QColor& color)
{
    m_color->setStyleSheet("background-color:" + color.name()+ ";");
}



ModelDataHolder::ModelDataHolder() : m_history(true)
{
    QGridLayout *layout = new QGridLayout;
    
    setLayout(layout);
    
    m_datawidget = new DataWidget;
    connect(m_datawidget, SIGNAL(NameChanged()), this, SLOT(SetProjectTabName()));
    m_modelsWidget = new TabWidget(this);
    m_modelsWidget->setTabsClosable(true);
    m_modelsWidget->setMovable(true);
    connect(m_modelsWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(RemoveTab(int)));
    connect(m_modelsWidget, SIGNAL(currentChanged(int)), this, SLOT(HideSubWindows(int)));
     
    m_statistic_dialog = new StatisticDialog(this);
    connect(m_statistic_dialog, SIGNAL(MCStatistic()), this, SLOT(MCStatistic()));
    connect(m_statistic_dialog, SIGNAL(CVStatistic()), this, SLOT(CVStatistic()));
    
    m_add = new QPushButton(tr("Add Model"));
    m_add->setFlat(true);
    m_add->setDisabled(true);
    
    m_optimize = new QPushButton(tr("Optimize All"));
    m_optimize->setFlat(true);
    m_optimize->setDisabled(true);
    connect(m_optimize, SIGNAL(clicked()), this, SLOT(OptimizeAll()));
    
    m_statistics = new QPushButton(tr("Statistics"));
    m_statistics->setFlat(true);
    m_statistics->setDisabled(true);
    connect(m_statistics, SIGNAL(clicked()), m_statistic_dialog, SLOT(show()));
    
    m_close_all = new QPushButton(tr("Close All"));
    m_close_all->setFlat(true);
    m_close_all->setDisabled(true);
    connect(m_close_all, SIGNAL(clicked()), this, SLOT(CloseAll()));
    
    QMenu *menu = new QMenu;
    QAction *ItoI_action = new QAction(this);
    ItoI_action->setText(tr("1:1-Model"));
    connect(ItoI_action, SIGNAL(triggered()), this, SLOT(AddModel11()));
    menu->addAction(ItoI_action);
    QAction *IItoI_ItoI_action = new QAction(this);
    IItoI_ItoI_action->setText(tr("2:1/1:1-Model"));
    connect(IItoI_ItoI_action, SIGNAL(triggered()), this, SLOT(AddModel21()));
    menu->addAction(IItoI_ItoI_action);
    
    QAction *ItoI_ItoII_action = new QAction(this);
    ItoI_ItoII_action->setText(tr("1:1/1:2-Model"));
    connect(ItoI_ItoII_action, SIGNAL(triggered()), this, SLOT(AddModel12()));
    menu->addAction(ItoI_ItoII_action);
    
    QAction *II_I_ItoI_ItoII_action = new QAction(this);
    II_I_ItoI_ItoII_action->setText(tr("2:1/1:1/1:2-Model"));
    connect(II_I_ItoI_ItoII_action, SIGNAL(triggered()), this, SLOT(AddModel2112()));
    menu->addAction(II_I_ItoI_ItoII_action);
    
    m_script_action = new QAction(this);
    m_script_action->setText(tr("Scripted Models"));
    ParseScriptedModels();
#ifdef experimentel
    menu->addAction(m_script_action);
#endif
    
    m_add->setMenu(menu);
    
    layout->addWidget(m_add, 0, 0);
    layout->addWidget(m_optimize, 0, 1);
    layout->addWidget(m_statistics, 0, 2);
    layout->addWidget(m_close_all, 0, 3);
    layout->addWidget(m_modelsWidget, 1, 0, 1, 4);
}

ModelDataHolder::~ModelDataHolder()
{
    
}

void ModelDataHolder::setData(QSharedPointer<DataClass> data, QSharedPointer<ChartWrapper> wrapper)
{
    m_data = data;
    m_datawidget->setData(m_data, wrapper);
    m_add->setEnabled(true);
    m_modelsWidget->setDataTab(m_datawidget); 
}

void ModelDataHolder::SetProjectTabName()
{
    m_modelsWidget->setTabText(0, "Overview: " + qApp->instance()->property("projectname").toString());
}


void ModelDataHolder::AddModel(int model)
{
    QSharedPointer<AbstractTitrationModel > t;
    
    switch(model){
        case 1:
            t =  QSharedPointer<ItoI_Model>(new ItoI_Model(m_data.data()), &QObject::deleteLater);
            break;
        case 2:
            t = QSharedPointer<IItoI_ItoI_Model>(new IItoI_ItoI_Model(m_data.data()), &QObject::deleteLater);
            break;
        case 3:
            t = QSharedPointer<ItoI_ItoII_Model>(new ItoI_ItoII_Model(m_data.data()),  &QObject::deleteLater);
            break;
        case 4:
            t = QSharedPointer<IItoI_ItoI_ItoII_Model>(new IItoI_ItoI_ItoII_Model(m_data.data()),  &QObject::deleteLater);
            break;
        default:
            t.clear();
            return; 
            
    };
    m_history = false;
    ActiveModel(t);
}

void ModelDataHolder::AddModel(const QJsonObject &json)
{
#ifdef experimental
    QSharedPointer<AbstractTitrationModel > t =  QSharedPointer<ScriptModel>(new ScriptModel(m_data.data(), json), &QObject::deleteLater);
    m_history = false;
    ActiveModel(t);
#endif
}

void ModelDataHolder::AddModel11()
{
    AddModel(ModelDataHolder::ItoI);
}

void ModelDataHolder::AddModel21()
{
    AddModel(ModelDataHolder::IItoI_ItoI);
}

void ModelDataHolder::AddModel12()
{
    AddModel(ModelDataHolder::ItoI_ItoII);
}

void ModelDataHolder::AddModel2112()
{
    AddModel(ModelDataHolder::IItoI_ItoI_ItoII);
}


void ModelDataHolder::AddModelScript()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        QString str = action->data().toString();
        
        if(!str.isEmpty())
        {
            QJsonObject object;
            if(JsonHandler::ReadJsonFile(object, str))
            {
                AddModel(object);
            }
            else
                qDebug() << "loading failed";
        }
    }
}


void ModelDataHolder::ParseScriptedModels()
{
    QMenu *menu = new QMenu;
    QDirIterator it(":/data/models/", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString name = it.next();
        
        QAction *sub_model = new QAction(this);
        sub_model->setData(name);
        sub_model->setText(name.remove(".json").remove(":/data/models/"));
        connect(sub_model, SIGNAL(triggered()), this, SLOT(AddModelScript()));
        menu->addAction(sub_model);
    }
    m_script_action->setMenu(menu);
    
}

void ModelDataHolder::Json2Model(const QJsonObject &object, const QString &str)
{
    
    QSharedPointer<AbstractTitrationModel > t;
    
    /*
     * WARNING and FIXME I dont like this!
     */
    if(str == "1:1-Model")
    {
        t = QSharedPointer<ItoI_Model>(new ItoI_Model(m_data.data()), &QObject::deleteLater);
    }
    else if(str == "2:1/1:1-Model")
    {
        t = QSharedPointer<IItoI_ItoI_Model>(new IItoI_ItoI_Model(m_data.data()), &QObject::deleteLater);
    }
    else if(str == "1:1/1:2-Model"){
        t =  QSharedPointer<ItoI_ItoII_Model>(new ItoI_ItoII_Model(m_data.data()),  &QObject::deleteLater);
    }
    else if(str == "2:1/1:1/1:2-Model"){
        t = QSharedPointer<IItoI_ItoI_ItoII_Model>(new IItoI_ItoI_ItoII_Model(m_data.data()),  &QObject::deleteLater);
    }
    else
    {
        t.clear();
        return; 
    }
    t->ImportModel(object);
    ActiveModel(t);
}

void ModelDataHolder::ActiveModel(QSharedPointer<AbstractTitrationModel> t)
{
    Charts charts = m_charts->addModel(t);
    ModelWidget *modelwidget = new ModelWidget(t, charts);
    
    t->setOptimizerConfig(m_config);
    connect(modelwidget, SIGNAL(AddModel(const QJsonObject)), this, SLOT(AddToWorkspace(const QJsonObject)));
    connect(modelwidget->getMinimizer().data(), SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);
    connect(modelwidget, SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);
    
    connect(modelwidget->getMinimizer().data(), SIGNAL(RequestCrashFile()), this, SLOT(CreateCrashFile()), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(RequestRemoveCrashFile()), this, SLOT(RemoveCrashFile()), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(InsertModel(QJsonObject, int)), this, SIGNAL(InsertModel(QJsonObject, int)), Qt::DirectConnection);
    
    connect(modelwidget, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)));
    connect(m_statistic_dialog, SIGNAL(Interrupt()), modelwidget, SIGNAL(Interrupt()));
    connect(m_statistic_dialog, SIGNAL(Interrupt()), this, SLOT(Interrupt()));
    
    m_modelsWidget->addModelsTab(modelwidget);
    m_last_tab = m_modelsWidget->currentIndex();
    m_models << t;
    m_close_all->setEnabled(true);
    m_statistics->setEnabled(true);
    m_optimize->setEnabled(true);
    
    /*
     * Some models are loaded from history, this should no be added again
     * after not adding them, we allow the next models to be added to history again
     */
    if(m_history)
        modelwidget->getMinimizer()->addToHistory();
    else
        m_history = true;
}

void ModelDataHolder::RemoveTab(int i)
{
    if(qobject_cast<QScrollArea *>(m_modelsWidget->widget(i)))
    {
        QScrollArea *scroll = qobject_cast<QScrollArea *>(m_modelsWidget->widget(i));
        ModelWidget *model = qobject_cast<ModelWidget *>(scroll->widget());
        m_modelsWidget->removeTab(i);
        delete model;
        delete scroll;
    }
    if(m_modelsWidget->count() < 2)
    {
        m_close_all->setEnabled(false);
        m_statistics->setEnabled(false);
        m_optimize->setEnabled(false);
    }
}

void ModelDataHolder::setSettings(const OptimizerConfig &config)
{
    for(int i = 0; i < m_modelsWidget->count(); ++i)
    {
        ModelWidget *w = qobject_cast<ModelWidget *>(m_modelsWidget->widget(i));
        if(w != 0)
        {
            w->Model()->setOptimizerConfig(config);
            m_config = config;
        }
    }
}

bool ModelDataHolder::CheckCrashFile()
{
    QString filename = qApp->instance()->property("projectpath").toString() + ".crashsave.json";
    return QFile::exists(filename);
}

void ModelDataHolder::CreateCrashFile()
{
    RemoveCrashFile();
    QString filename = qApp->instance()->property("projectpath").toString() + ".crashsave.json";
    for(int i = 0; i < m_models.size(); ++i)
    {
        if(!m_models[i].isNull())
        {
            QJsonObject obj = m_models[i].data()->ExportModel();
            JsonHandler::AppendJsonFile(obj, filename);        
        }
    }   
}

void ModelDataHolder::RemoveCrashFile()
{
    if(CheckCrashFile())
    {
        QString filename = qApp->instance()->property("projectpath").toString() + ".crashsave.json";
        QFile::remove(filename);
    }
}

void ModelDataHolder::SaveCurrentModels(const QString &file)
{
    QJsonObject toplevel;
    for(int i = 0; i < m_models.size(); ++i)
    {
        
        if(m_models[i].isNull())
        {
            QJsonObject obj = m_models[i].data()->ExportModel();
            toplevel["model_" + QString::number(i)] = obj;
            
        }
    }   
    JsonHandler::WriteJsonFile(toplevel, file);   
}

void ModelDataHolder::SaveWorkspace(const QString &file)
{
    QJsonObject toplevel;
    toplevel["data"] = m_data->ExportData();
    
    for(int i = 0; i < m_models.size(); ++i)
    {    
        if(!m_models[i].isNull())
        {
            QJsonObject obj = m_models[i].data()->ExportModel();
            toplevel["model_" + QString::number(i)] = obj;       
        }
    }   
    JsonHandler::WriteJsonFile(toplevel, file);
}

void ModelDataHolder::AddToWorkspace(const QJsonObject &object)
{
    QStringList keys = object.keys();
    
    /*
     * If the json contains only one model, then we have probely only "data" and "model" as keys
     * and we can load them directly
     * else we iter through all keys which may be model_x keys containing "data" and "models"
     */
    if(keys.contains("data") && keys.contains("model"))
    {
        // we don't allow this model to be added to addToHistory
        m_history = false;
        Json2Model(object, object["model"].toString());
    }
    else
    {
        int i = m_models.size();
        for(const QString &str : qAsConst(keys))
        {
            /*
             * Dont load to many models to the workspace, this is slow and confusing
             */
            QJsonObject model = object[str].toObject();
            if(i++ < 5)
                Json2Model(model, model["model"].toString());
            else
                emit InsertModel(model);
        }
    }
}

void ModelDataHolder::LoadCurrentProject(const QJsonObject& object)
{
    QStringList keys = object.keys();
    if(keys.contains("data") && keys.contains("model"))
    {
        if(m_modelsWidget->currentIndex() == 0)
        {
            if(m_modelsWidget->count() < 2)
                return;
            m_modelsWidget->setCurrentIndex(1);
        }
        QMessageBox::StandardButton replay;
        replay = QMessageBox::information(this, tr("Override Model."), tr("Do you want to override the current loaded model [Y]\nor just add to workspace [N]?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if(replay == QMessageBox::Yes) 
        {
            QScrollArea *scroll = qobject_cast<QScrollArea *>(m_modelsWidget->currentWidget());
            ModelWidget *model = qobject_cast<ModelWidget *>(scroll->widget());
            model->LoadJson(object);
        }
        else if(replay == QMessageBox::No)
        {
            AddToWorkspace(object);
        }
    }
}



void ModelDataHolder::CloseAll()
{
    QMessageBox::StandardButton replay;
    QString app_name = QString(qApp->instance()->applicationName());
    replay = QMessageBox::information(this, tr("Close All."), tr("Do you really want to close all models on the workspace?"), QMessageBox::Yes | QMessageBox::No);
    if(replay == QMessageBox::Yes) 
    {
        for(int i = m_modelsWidget->count(); i > 0; --i)
            RemoveTab(i);
    }
}

void ModelDataHolder::CVStatistic()
{
    CVConfig config = m_statistic_dialog->getCVConfig();
    for(int i = 1; i < m_modelsWidget->count(); i++)
    {
        if(qobject_cast<QScrollArea *>(m_modelsWidget->widget(i)))
        {
            QScrollArea *scroll = qobject_cast<QScrollArea *>(m_modelsWidget->widget(i));
            ModelWidget *model = qobject_cast<ModelWidget *>(scroll->widget());
            model->CVStatistic(config);
        }
    }
}

void ModelDataHolder::MCStatistic()
{
    m_statistic_dialog->setRuns(m_models.size() - 1);
    MCConfig config = m_statistic_dialog->getMCConfig();
    m_allow_loop = true;
    for(int i = 1; i < m_modelsWidget->count(); i++)
    {
        if(qobject_cast<QScrollArea *>(m_modelsWidget->widget(i)))
        {
            QScrollArea *scroll = qobject_cast<QScrollArea *>(m_modelsWidget->widget(i));
            ModelWidget *model = qobject_cast<ModelWidget *>(scroll->widget());
            config.variance = model->Model()->StdDeviation();
            model->MCStatistic(config);
        }
        if(!m_allow_loop)
            break;
    }
}

void ModelDataHolder::OptimizeAll()
{
    for(int i = 1; i < m_modelsWidget->count(); i++)
    {
        if(qobject_cast<QScrollArea *>(m_modelsWidget->widget(i)))
        {
            QScrollArea *scroll = qobject_cast<QScrollArea *>(m_modelsWidget->widget(i));
            ModelWidget *model = qobject_cast<ModelWidget *>(scroll->widget());
            model->GlobalMinimize();
        }
    } 
}

void ModelDataHolder::HideSubWindows(int index)
{
    if(qobject_cast<QScrollArea *>(m_modelsWidget->widget(m_last_tab)))
    {
        QScrollArea *scroll = qobject_cast<QScrollArea *>(m_modelsWidget->widget(m_last_tab));
        ModelWidget *model = qobject_cast<ModelWidget *>(scroll->widget());
        model->HideAllWindows();
        m_last_tab = index;
    }
}
#include "modeldataholder.moc"
