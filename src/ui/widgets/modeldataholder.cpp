
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

#include "src/core/jsonhandler.h"
#include "src/core/models.h"
#include "src/ui/widgets/datawidget.h"
#include "src/ui/widgets/modelwidget.h"

#include <QApplication>

#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QCheckBox>

#include "modeldataholder.h"
#include "chartwidget.h"

#include <iostream>

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
    hide->resize(20,20);
    hide->setChecked(true);
    hide->setToolTip(tr("Toggle Series in Charts"));
    
    connect(hide, SIGNAL(stateChanged(int)), modelwidget, SIGNAL(ToggleSeries(int)));
    setCurrentWidget(scroll);
    tabBar()->setTabButton(currentIndex(), QTabBar::LeftSide, hide);
}

void TabWidget::setDataTab(QPointer<DataWidget> datawidget)
{
    addTab(datawidget, "Overview: " + qApp->instance()->property("projectname").toString());
    tabBar()->setTabButton(0, QTabBar::RightSide, 0);
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
    
    m_add = new QPushButton(tr("Add Titration\n Model"));
    m_add->setFlat(true);
    m_add->setDisabled(true);

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
    
    m_add->setMenu(menu);
    
    layout->addWidget(m_add, 0, 0);
    layout->addWidget(m_modelsWidget, 1, 0, 1, 2);
    
}

ModelDataHolder::~ModelDataHolder()
{
    
}

QSharedPointer<DataClass> ModelDataHolder::setData(QPointer<DataClass> dataclass)
{
    m_data = QSharedPointer<DataClass>(new DataClass((dataclass))); 
    m_datawidget->setData(m_data);
    m_add->setEnabled(true);
    m_modelsWidget->setDataTab(m_datawidget); 
    return m_data;
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
//     t->Minimize();
    m_history = false;
    ActiveModel(t);
    
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
    t->ImportJSON(object);
    ActiveModel(t);
}

void ModelDataHolder::ActiveModel(QSharedPointer<AbstractTitrationModel> t)
{
    
    m_datawidget->setData(m_data);
    Charts charts = m_charts->addModel(t);
    ModelWidget *modelwidget = new ModelWidget(t, charts);
    
    t->setOptimizerConfig(m_config);
    connect(modelwidget, SIGNAL(AddModel(const QJsonObject)), this, SLOT(AddToWorkspace(const QJsonObject)));
    connect(modelwidget->getMinimizer().data(), SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);
    connect(modelwidget, SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);

    connect(modelwidget->getMinimizer().data(), SIGNAL(RequestCrashFile()), this, SLOT(CreateCrashFile()), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(RequestRemoveCrashFile()), this, SLOT(RemoveCrashFile()), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(InsertModel(ModelHistoryElement)), this, SIGNAL(InsertModel(ModelHistoryElement)), Qt::DirectConnection);
    

    m_modelsWidget->addModelsTab(modelwidget);
    m_models << t;
    /*
     * Some models are loaded from history, this should no be added again
     * after not added them, we allow the next models to be added to history again
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
}

void ModelDataHolder::setSettings(const OptimizerConfig &config)
{
    for(int i = 0; i < m_modelsWidget->count(); ++i)
    {
        ModelWidget *w = qobject_cast<ModelWidget *>(m_modelsWidget->widget(i));
        if(w != 0)
        {
            w->setMaxIter(config.MaxIter);
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
            QJsonObject obj = m_models[i].data()->ExportJSON();
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
            QJsonObject obj = m_models[i].data()->ExportJSON();
            toplevel["model_" + QString::number(i)] = obj;
            
        }
    }   
    JsonHandler::WriteJsonFile(toplevel, file);   
}

void ModelDataHolder::SaveWorkspace(const QString &file)
{
        QJsonObject toplevel;
        toplevel["data"] = m_data->ExportJSON();
        
        for(int i = 0; i < m_models.size(); ++i)
        {    
            if(!m_models[i].isNull())
            {
                QJsonObject obj = m_models[i].data()->ExportJSON();
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
        QScrollArea *scroll = qobject_cast<QScrollArea *>(m_modelsWidget->currentWidget());
        ModelWidget *model = qobject_cast<ModelWidget *>(scroll->widget());
        model->LoadJson(object);
    }
}


#include "modeldataholder.moc"
