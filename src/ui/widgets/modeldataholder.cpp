
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

#include "modeldataholder.h"
#include "chartwidget.h"

#include <iostream>

ModelDataHolder::ModelDataHolder()
{
    QGridLayout *layout = new QGridLayout;
    
    setLayout(layout);
    
    
    m_datawidget = new DataWidget;
    
    m_modelsWidget = new QTabWidget;
    m_modelsWidget->addTab(m_datawidget, tr("Data"));
    m_modelsWidget->setTabsClosable(true);
    connect(m_modelsWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(RemoveTab(int)));
        
    m_add = new QPushButton(tr("Add Titration\n Model"));
    m_add->setFlat(true);
    m_add->setDisabled(true);
    
    m_simulate = new QPushButton(tr("Simulate"));
    m_simulate->setFlat(true);
    
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
        
//           QAction *t2 = new QAction(this);
//      t2->setText(tr("2:1/1:1-test-Model"));
//          connect(t2, SIGNAL(triggered()), this, SLOT(AddModel21_t()));
//          menu->addAction(t2);
//         
     m_add->setMenu(menu);
        menu = new QMenu;
    ItoI_action = new QAction(this);
    ItoI_action->setText(tr("1:1-Model"));
        connect(ItoI_action, SIGNAL(triggered()), this, SLOT(SimulateModel11()));
        menu->addAction(ItoI_action);
    IItoI_ItoI_action = new QAction(this);
    IItoI_ItoI_action->setText(tr("2:1/1:1-Model"));
        connect(IItoI_ItoI_action, SIGNAL(triggered()), this, SLOT(SimulateModel21()));
        menu->addAction(IItoI_ItoI_action);
        
        
    ItoI_ItoII_action = new QAction(this);
    ItoI_ItoII_action->setText(tr("1:1/1:2-Model"));
        connect(ItoI_ItoII_action, SIGNAL(triggered()), this, SLOT(SimulateModel12()));
        menu->addAction(ItoI_ItoII_action);
        
    m_simulate->setMenu(menu);
    layout->addWidget(m_add, 0, 0);
//     layout->addWidget(m_simulate, 0,1);
    layout->addWidget(m_modelsWidget, 1, 0, 1, 2);
        
}

ModelDataHolder::~ModelDataHolder()
{
    
}

void ModelDataHolder::setData(DataClass *dataclass)
{
    m_data = new DataClass(dataclass); 
    m_datawidget->setData(m_data);
    m_add->setEnabled(true);
}

void ModelDataHolder::AddModel(int model)
{
    QPointer<AbstractTitrationModel > t;
    
    switch(model){
        case 1:
            t = new ItoI_Model(m_data);
            break;
        case 2:
            t = new IItoI_ItoI_Model(m_data);
            break;
        case 3:
            t = new ItoI_ItoII_Model(m_data);
            break;
//         case 4:
//              t = new test_II_ItoI_Model(m_data);
//              break;
        default:
            delete t;
           return; 
        
    };
    t->Minimize();
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

void ModelDataHolder::AddModel21_t()
{
    AddModel(ModelDataHolder::IItoI_ItoI_t);
}

void ModelDataHolder::SimulateModel(int model)
{
    QPointer<AbstractTitrationModel > t;
    
    m_data = new DataClass(DataClass::EmptyData);
    switch(model){
        case 1:
            t = new ItoI_Model(m_data);
            break;
        case 2:
            t = new IItoI_ItoI_Model(m_data);
            break;
        case 3:
            t = new ItoI_ItoII_Model(m_data);
            break;
        default:
            delete t;
           return; 
        
    };
    ActiveModel(t);
}

void ModelDataHolder::Json2Model(const QJsonObject &object, const QString &str)
{

    QPointer<AbstractTitrationModel > t;
    /*
     * WARNING and FIXME I dont like this!
     */
    if(str == "1:1-Model")
    {
        t = new ItoI_Model(m_data);
    }
    else if(str == "2:1/1:1-Model")
    {
         t = new IItoI_ItoI_Model(m_data);
    }
    else if(str == "1:1/1:2-Model"){
        t = new ItoI_ItoII_Model(m_data);
    }else
    {
        delete t;
        return; 
    }
    t->ImportJSON(object);
    ActiveModel(t);
}

void ModelDataHolder::ActiveModel(QPointer<AbstractTitrationModel> t)
{
   
    m_datawidget->setData(m_data);
    m_charts->addModel(t);
    ModelWidget *modelwidget = new ModelWidget(t);
    m_modelsWidget->addTab(modelwidget, t->Name());
    
    t->setOptimizerConfig(m_config);
    connect(t, SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)));
    connect(t, SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)));
    
    connect(modelwidget, SIGNAL(RequestCrashFile()), this, SLOT(CreateCrashFile()));
    connect(modelwidget, SIGNAL(RequestRemoveCrashFile()), this, SLOT(RemoveCrashFile()));
    connect(modelwidget, SIGNAL(AddModel(QJsonObject)), this, SLOT(LoadProject(QJsonObject)));
    connect(modelwidget, SIGNAL(InsertModel(ModelHistoryElement)), this, SIGNAL(InsertModel(ModelHistoryElement)));
    
    QScrollArea *scroll = new QScrollArea;
    scroll->setBackgroundRole(QPalette::Midlight);
      scroll->setWidget(modelwidget);
      scroll->setWidgetResizable(true);
      scroll->setAlignment(Qt::AlignHCenter);
    m_modelsWidget->addTab(scroll, t->Name());
    m_models << t;
    modelwidget->addToHistory();
}

void ModelDataHolder::SimulateModel11()
{
    SimulateModel(ModelDataHolder::ItoI);
}

void ModelDataHolder::SimulateModel21()
{
    SimulateModel(ModelDataHolder::IItoI_ItoI);
}

void ModelDataHolder::SimulateModel12()
{
    SimulateModel(ModelDataHolder::ItoI_ItoII);
}


void ModelDataHolder::RemoveTab(int i)
{
    if(i)
    {
        QScrollArea *scroll = qobject_cast<QScrollArea *>(m_modelsWidget->widget(i));
        ModelWidget *model = qobject_cast<ModelWidget *>(scroll->widget());
        m_modelsWidget->removeTab(i);
        delete model;
        delete scroll;
    }else
    {
    QMessageBox msgBox;
    msgBox.setText("Lieber nicht ...");
    msgBox.exec();
    }
}

// void ModelDataHolder::addLogEntry(const QString& str)
// {
//     m_logWidget->appendPlainText(str);
// }

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
    QString filename = qApp->instance()->property("projectname").toString() + ".crashsave.json";
    return QFile::exists(filename);
}

void ModelDataHolder::CreateCrashFile()
{
    RemoveCrashFile();
    QString filename = qApp->instance()->property("projectname").toString() + ".crashsave.json";
    for(int i = 0; i < m_models.size(); ++i)
    {
        if(m_models[i])
        {
            QJsonObject obj = m_models[i]->ExportJSON();
            JsonHandler::AppendJsonFile(obj, filename);        
        }
    }   
}

void ModelDataHolder::RemoveCrashFile()
{
    if(CheckCrashFile())
    {
        QString filename = qApp->instance()->property("projectname").toString() + ".crashsave.json";
        QFile::remove(filename);
    }
}

void ModelDataHolder::SaveProject(const QString &str)
{
        QJsonObject toplevel;
        toplevel["data"] = m_data->ExportJSON();
        
        for(int i = 0; i < m_models.size(); ++i)
        {    
            if(m_models[i])
            {
                QJsonObject obj = m_models[i]->ExportJSON();
                toplevel["model_" + QString::number(i)] = obj;       
            }
        }   
        JsonHandler::WriteJsonFile(toplevel  , str);
}


void ModelDataHolder::Export(const QString &str)
{
    QJsonObject toplevel;
    for(int i = 0; i < m_models.size(); ++i)
    {
        
        if(m_models[i])
        {
            QJsonObject obj = m_models[i]->ExportJSON();
            toplevel["model_" + QString::number(i)] = obj;
            
        }
    }   
    JsonHandler::WriteJsonFile(toplevel, str);   
}

void ModelDataHolder::LoadProject(const QJsonObject &object)
{
    QStringList keys = object.keys();
    foreach(const QString &str, keys)
    {
        QJsonObject model = object[str].toObject();
        Json2Model(model, model["model"].toString());
    }
}


#include "modeldataholder.moc"
