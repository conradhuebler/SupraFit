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




#include "ui/widgets/datawidget.h"
#include "ui/widgets/modelwidget.h"

#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include "modeldataholder.h"
#include "chartwidget.h"

ModelDataHolder::ModelDataHolder()
{
    QGridLayout *layout = new QGridLayout;
    
    setLayout(layout);
    
    
    m_datawidget = new DataWidget;
    m_logWidget = new QPlainTextEdit;
    m_modelsWidget = new QTabWidget;
    m_modelsWidget->addTab(m_datawidget, tr("Data"));
//         m_modelsWidget->setMovable(true);
        m_modelsWidget->setTabsClosable(true);
        connect(m_modelsWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(RemoveTab(int)));
    m_add = new QPushButton(tr("Add Titration\n Model"));
        m_add->setFlat(true);
        m_add->setDisabled(true);
    m_simulate = new QPushButton(tr("Simulate"));
        m_add->setFlat(true);
    QMenu *menu = new QMenu;
    QAction *ItoI = new QAction(this);
    ItoI->setText(tr("1:1-Model"));
        connect(ItoI, SIGNAL(triggered()), this, SLOT(AddModel11()));
        menu->addAction(ItoI);
    QAction *IItoI_ItoI = new QAction(this);
    IItoI_ItoI->setText(tr("2:1/1:1-Model"));
        connect(IItoI_ItoI, SIGNAL(triggered()), this, SLOT(AddModel21()));
        menu->addAction(IItoI_ItoI);
        
   
        
    QAction *ItoI_ItoII = new QAction(this);
    ItoI_ItoII->setText(tr("1:1/1:2-Model"));
        connect(ItoI_ItoII, SIGNAL(triggered()), this, SLOT(AddModel12()));
        menu->addAction(ItoI_ItoII);
     m_add->setMenu(menu);
        menu = new QMenu;
    ItoI = new QAction(this);
    ItoI->setText(tr("1:1-Model"));
        connect(ItoI, SIGNAL(triggered()), this, SLOT(SimulateModel11()));
        menu->addAction(ItoI);
    IItoI_ItoI = new QAction(this);
    IItoI_ItoI->setText(tr("2:1/1:1-Model"));
        connect(IItoI_ItoI, SIGNAL(triggered()), this, SLOT(SimulateModel21()));
        menu->addAction(IItoI_ItoI);
        
        
    ItoI_ItoII = new QAction(this);
    ItoI_ItoII->setText(tr("1:1/1:2-Model"));
        connect(ItoI_ItoII, SIGNAL(triggered()), this, SLOT(SimulateModel12()));
        menu->addAction(ItoI_ItoII);
        
    m_simulate->setMenu(menu);
    layout->addWidget(m_add, 0, 0);
//     layout->addWidget(m_simulate, 0,1);
    layout->addWidget(m_modelsWidget, 1, 0, 1, 2);
    layout->addWidget(m_logWidget, 2, 0, 1, 2);
        
}

ModelDataHolder::~ModelDataHolder()
{
    
}

void ModelDataHolder::setData(DataClass *data)
{
    m_data = new DataClass(data); 
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
        default:
           return; 
        
    };
    ModelWidget *modelwidget = new ModelWidget(t);
    m_modelsWidget->addTab(modelwidget, t->Name());
    m_charts->addModel(t);
    connect(t, SIGNAL(Message(QString)), this, SLOT(addLogEntry(QString)));

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
           return; 
        
    };
    ModelWidget *modelwidget = new ModelWidget(t);
    m_modelsWidget->addTab(modelwidget, t->Name());
    m_datawidget->setData(m_data);
    m_charts->addModel(t);
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
        ModelWidget *w = qobject_cast<ModelWidget *>(m_modelsWidget->widget(i));
        m_modelsWidget->removeTab(i);
        delete w;
    }else
    {
    QMessageBox msgBox;
    msgBox.setText("Lieber nicht ...");
    msgBox.exec();
    }
}

void ModelDataHolder::addLogEntry(const QString& str)
{
    m_logWidget->appendPlainText(str);
}


#include "modeldataholder.moc"
