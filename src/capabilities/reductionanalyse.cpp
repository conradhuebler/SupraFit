/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

void ReductionAnalyse::CrossValidation(CVType type)
{
    MCConfig config;
    config.runtype = m_config.runtype;
    config.optimizer_config = m_config.optimizer_config;

    switch(type){
        case CVType::LeaveOnOut:
            m_loo_table = new DataTable(m_model->DependentModel());
            for(int i = m_model->DataPoints() - 1; i >= 0; --i)
            {
                QPointer<MonteCarloThread > thread = new MonteCarloThread(config);
                QSharedPointer<AbstractModel> model = m_model->Clone();
                model->DependentModel()->CheckRow(i);
                thread->setModel(model);
                thread->run();
                model->ImportModel(thread->Model());
//                 std::cout << model->ModelTable()->Row(i) << std::endl;
                model->DependentModel()->CheckRow(i);
                model->Calculate();
//                 std::cout << model->ModelTable()->Row(i) << std::endl;
                m_loo_table->setRow(model->ModelTable()->Row(i), i);
                m_models << model->ExportModel();
                delete thread;
            }
            m_model->setDependentTable(m_loo_table);
//             m_loo_table->Debug();
            m_model_data = m_model->ExportModel(false, true);
            break;
        case CVType::LeaveTwoOut:
            
            break;
    }
    
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
        thread->run();
        QList<qreal > constants = thread->Constants();
        for(int j = 0; j < constants.size(); ++j)
            m_series[j].append(QPointF(m_model->PrintOutIndependent(i), constants[j]));
        delete thread;
    }
}
