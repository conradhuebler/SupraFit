/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QApplication>

#include "src/core/AbstractModel.h"
#include "src/core/minimizer.h"
#include "src/core/toolset.h"

#include "src/capabilities/montecarlostatistics.h"

#include <iostream>

#include "reductionanalyse.h"

ReductionAnalyse::ReductionAnalyse(OptimizerConfig config, OptimizationType type): m_config(config, type)
{
}

ReductionAnalyse::~ReductionAnalyse()
{

}

void ReductionAnalyse::addThread(QPointer<MonteCarloThread> thread)
{
    m_threads << thread;
    m_threadpool->start(thread);
}

bool ReductionAnalyse::Pending() const
{
    return m_threadpool->activeThreadCount();
}

void ReductionAnalyse::CrossValidation(CVType type)
{
    MCConfig config;
    config.runtype = m_config.runtype;
    config.optimizer_config = m_config.optimizer_config;
    
    m_controller["runtype"] = m_config.runtype;
    m_controller["method"] = SupraFit::Statistic::CrossValidation;
    m_controller["CVType"] = type;
    
    switch(type){
        case CVType::LeaveOneOut:
            emit MaximumSteps(m_model->DataPoints());
            for(int i = m_model->DataPoints() - 1; i >= 0; --i)
            {
                QPointer<MonteCarloThread > thread = new MonteCarloThread(config);
                connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
                QSharedPointer<AbstractModel> model = m_model->Clone();
                model->detach();
                model->DependentModel()->CheckRow(i);
                thread->setModel(model);
                addThread(thread);
                QApplication::processEvents();
            }
            break;
        case CVType::LeaveTwoOut:
            emit MaximumSteps(m_model->DataPoints()*(m_model->DataPoints() - 1)/2);
            for(int i = 0; i < m_model->DataPoints(); ++i)
                for(int j = i + 1; j < m_model->DataPoints(); ++j)
                {
                    QApplication::processEvents();
                    QPointer<MonteCarloThread > thread = new MonteCarloThread(config);
                    connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
                    QSharedPointer<AbstractModel> model = m_model->Clone();
                    model->detach();
                    model->DependentModel()->CheckRow(i);
                    model->DependentModel()->CheckRow(j);
                    thread->setModel(model);
                    addThread(thread);
                    QApplication::processEvents();
                }
            break;
            
        case CVType::LeaveManyOut:
            // This will come sometimes
            
            break;
    }
    while(Pending()) { QApplication::processEvents(); }
    for(int i = 0; i < m_threads.size(); ++i)
    {
        if(m_threads[i])
        {
            m_models << m_threads[i]->Model();
            delete m_threads[i];
        }
    }
    m_results = ToolSet::Model2Parameter(m_models);
    ToolSet::Parameter2Statistic(m_results, m_model.data());
    emit AnalyseFinished();
}

void ReductionAnalyse::PlainReduction()
{
    MCConfig config;
    config.runtype = m_config.runtype;
    config.optimizer_config = m_config.optimizer_config;
    
    m_controller["runtype"] = m_config.runtype;
    m_controller["method"] = SupraFit::Statistic::Reduction;
    
    emit MaximumSteps(m_model->DataPoints());
    
//     m_models << m_model.data()->ExportData();
    
    QPointer<DataTable > table = m_model->DependentModel();
    for(int i = m_model->DataPoints() - 1; i > 3; --i)
    {
        QPointer<MonteCarloThread > thread = new MonteCarloThread(config);
        connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
        thread->setIndex(i);
        QSharedPointer<AbstractModel> model = m_model->Clone();
        table->CheckRow(i); 
        model->setDependentTable(table);
        model->detach();
        thread->setModel(model);
        addThread(thread);
    }
    while(Pending()) { QApplication::processEvents(); }
    QList<qreal> x;
    for(int i = 0; i < m_threads.size(); ++i)
    {
        if(m_threads[i])
        {
            m_models << m_threads[i]->Model();
            x << m_model->PrintOutIndependent(m_threads[i]->Index() - 1); 
            delete m_threads[i];
        }
    }
    if(table)
        delete table;
    m_results = ToolSet::Model2Parameter(m_models, false);
    ToolSet::Parameter2Statistic(m_results, m_model.data());
    m_controller["x"] = ToolSet::DoubleList2String(x);
    emit AnalyseFinished();
}

