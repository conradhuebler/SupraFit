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

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>

#include "modeldataholder.h"

ModelDataHolder::ModelDataHolder()
{
    QGridLayout *layout = new QGridLayout;
    
    setLayout(layout);
    
    
    m_datawidget = new DataWidget;
    m_models = new QTabWidget;
    m_add = new QPushButton(tr("Add"));
    
    connect(m_add, SIGNAL(clicked()), this, SLOT(AddModel()));
    
    
    
    layout->addWidget(m_add, 0, 0);
    layout->addWidget(m_datawidget,1, 0);
    layout->addWidget(m_models, 2, 0);
        
}

ModelDataHolder::~ModelDataHolder()
{
    
}

void ModelDataHolder::setData(DataClass data)
{
    m_data = new DataClass(data); 
    m_datawidget->setData(m_data);
}

void ModelDataHolder::AddModel()
{
    ItoIModel *t = new ItoIModel(m_data);
    ModelWidget *model = new ModelWidget(t);
    m_models->addTab(model, "1:1");
    connect(m_datawidget, SIGNAL(recalculate()), model, SLOT(recalulate()));
    connect(model, SIGNAL(Fit(QVector<QPointer<QtCharts::QLineSeries> >)), this, SIGNAL(PlotChart(QVector<QPointer<QtCharts::QLineSeries> >)));
    connect(model, SIGNAL(Error(QVector<QPointer<QtCharts::QLineSeries> >)), this, SIGNAL(PlotErrorChart(QVector<QPointer<QtCharts::QLineSeries> >)));
}


#include "modeldataholder.moc"
