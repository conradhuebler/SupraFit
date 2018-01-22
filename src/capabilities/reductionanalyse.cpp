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
#include "src/capabilities/montecarlostatistics.h"

#include <iostream>

#include "reductionanalyse.h"

ReductionAnalyse::ReductionAnalyse(OptimizerConfig config, OptimizationType type): m_config(config, type)
{
}

ReductionAnalyse::~ReductionAnalyse()
{
    if(m_loo_table)
        delete m_loo_table;
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

    switch(type){
        case CVType::LeaveOneOut:
            emit MaximumSteps(m_model->DataPoints());
            m_loo_table = new DataTable(m_model->DependentModel());
            for(int i = m_model->DataPoints() - 1; i >= 0; --i)
            {
                QPointer<MonteCarloThread > thread = new MonteCarloThread(config);
                connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
                QSharedPointer<AbstractModel> model = m_model->Clone();
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
                    model->DependentModel()->CheckRow(i);
                    model->DependentModel()->CheckRow(j);
                    thread->setModel(model);
                    addThread(thread);
                }
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
    m_model->DependentModel()->EnableAllRows();
}

void ReductionAnalyse::PlainReduction()
{
    MCConfig config;
    config.runtype = m_config.runtype;
    config.optimizer_config = m_config.optimizer_config;
    m_model->detach();

    for(int j = 0; j < m_model->GlobalParameterSize(); ++j)
        m_series << QList<QPointF>();

    for(int i = m_model->DataPoints() - 1; i >= 0; --i)
    {
        QPointer<MonteCarloThread > thread = new MonteCarloThread(config);
        m_model->DependentModel()->CheckRow(i);
        thread->setModel(m_model);
        addThread(thread);
    }
    
    while(Pending()) { }
    
    for(int i = 0; i < m_threads.size(); ++i)
    {
        if(m_threads[i])
        {
            m_models << m_threads[i]->Model();
            QList<qreal > constants = m_threads[i]->Constants();
            for(int j = 0; j < constants.size(); ++j)
                m_series[j].append(QPointF(m_model->PrintOutIndependent(i), constants[j]));
            delete m_threads[i];
        }
    }
    m_model->DependentModel()->EnableAllRows();
}
