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

#include "src/core/equal_system.h"
#include "src/core/models.h"
#include "src/core/libmath.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/NonLinearOptimization>

#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>
#include <QtCore/QThreadPool>
#include <QtCore/QRunnable>
#include <QtMath>
#include <QtCore/QJsonObject>
#include <QDebug>
#include <cmath>
#include <cfloat>
#include <iostream>
#include "2_1_1_1_1_2_Model.h"

IItoI_ItoI_ItoII_Model::IItoI_ItoI_ItoII_Model(DataClass* data): AbstractTitrationModel(data)
{
    setName(tr("2:1/1:1/1:2-Model"));
    for(int i = 0; i < DataPoints(); ++i)
        m_solvers << new ConcentrationSolver(this);
    m_local_parameter = new DataTable(4, SeriesCount(), this);
    InitialGuess();
    setOptParamater(m_global_parameter);
    AbstractTitrationModel::Calculate();    
}

IItoI_ItoI_ItoII_Model::~IItoI_ItoI_ItoII_Model()
{
    qDeleteAll(m_solvers);
}

void IItoI_ItoI_ItoII_Model::CalculateVariables()
{
    m_corrupt = false;
    
    m_sum_absolute = 0;
    m_sum_squares = 0;
    
    qreal K21= qPow(10, GlobalParameter().first());
    qreal K11 =qPow(10, GlobalParameter()[1]);
    qreal K12= qPow(10, GlobalParameter().last());
    m_constants_pow = QList<qreal >() << K21 << K11 << K12;
    QThreadPool *threadpool = new QThreadPool;
    int maxthreads =qApp->instance()->property("threads").toInt();
    threadpool->setMaxThreadCount(maxthreads);
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        
        m_solvers[i]->setInput(host_0, guest_0);
        threadpool->start(m_solvers[i]);
    }
    
    threadpool->waitForDone();
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        
        
        QList<double > concentration = m_solvers[i]->Concentrations();
        qreal host = concentration[0];
        qreal guest = concentration[1]; 
        
        qreal complex_11 = K11*host*guest;
        qreal complex_21 = K11*K21*host*host*guest;
        qreal complex_12 = K11*K12*host*guest*guest;       
        
        Vector vector(6);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        vector(3) = complex_21;
        vector(4) = complex_11;
        vector(5) = complex_12;
        SetConcentration(i, vector);
        
        for(int j = 0; j < SeriesCount(); ++j)
        {
            qreal value = host/host_0*m_local_parameter->data(0, j) + 2*complex_21/host_0*m_local_parameter->data(1, j) + complex_11/host_0*m_local_parameter->data(2, j) + complex_12/host_0*m_local_parameter->data(3, j);
            SetValue(i, j, value);
        }
    }
    
    emit Recalculated();
}

QSharedPointer<AbstractModel> IItoI_ItoI_ItoII_Model::Clone()
{
    QSharedPointer<IItoI_ItoI_ItoII_Model > model = QSharedPointer<IItoI_ItoI_ItoII_Model>(new IItoI_ItoI_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

void IItoI_ItoI_ItoII_Model::InitialGuess()
{
    ItoI_Model *model = new ItoI_Model(m_data);
    m_K12 = model->GlobalParameter()[model->GlobalParameterSize() -1];
    m_K21 = m_K12/2;
    m_K11 = m_K12/2;
    delete model;
    
    m_global_parameter = QList<qreal>() << m_K21 << m_K11 << m_K12;
    setOptParamater(m_global_parameter);
    
    m_local_parameter->setColumn(DependentModel()->firstRow(), 0);
    m_local_parameter->setColumn(DependentModel()->firstRow(), 1);
    m_local_parameter->setColumn(DependentModel()->lastRow(), 2);
    m_local_parameter->setColumn(DependentModel()->lastRow(), 3);
    
    QVector<qreal * > line1, line2;
    for(int i = 0; i < SeriesCount(); ++i)
    {
        line1 << &m_local_parameter->data(0, i); 
        line2 << &m_local_parameter->data(3, i); 
    }

    m_lim_para = QVector<QVector<qreal * > >() << line1 << line2;
    
    AbstractTitrationModel::Calculate();
}

QVector<qreal> IItoI_ItoI_ItoII_Model::OptimizeParameters_Private(OptimizationType type)
{
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
    {
        addGlobalParameter(m_global_parameter);
    }
     if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
        if((type & OptimizationType::UnconstrainedShifts) == OptimizationType::UnconstrainedShifts)
        {
            addLocalParameter(1);
            addLocalParameter(2);
            addLocalParameter(3);
            if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
                addLocalParameter(0);
        }
        if(((type & OptimizationType::ConstrainedShifts) == OptimizationType::ConstrainedShifts) && ((type & OptimizationType::IntermediateShifts) == OptimizationType::IntermediateShifts))
        {
            addLocalParameter(1);
            addLocalParameter(2);
        }
    }
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


MassResults IItoI_ItoI_ItoII_Model::MassBalance(qreal A, qreal B)
{
    //     QMutexLocker (&mutex);
    MassResults result;
    qreal K11 = m_constants_pow[0];
    qreal K21 = m_constants_pow[1];
    qreal K12 = m_constants_pow[2];
    
    Vector values(2);
    
    qreal complex_21 = K11*K21*A*A*B;
    qreal complex_11 = K11*A*B;
    qreal complex_12 = K11*K12*A*B*B;
    
    values(0) = (2*complex_21 + complex_11 + complex_12) ;
    values(1) = (complex_21 + complex_11 + 2*complex_12) ;
    result.MassBalance = values;
    return result;
}

