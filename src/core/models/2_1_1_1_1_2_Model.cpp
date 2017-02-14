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

#include "src/core/models.h"
#include "src/core/libmath.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/NonLinearOptimization>

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

template<typename _Scalar, int NX = Eigen::Dynamic, int NY = Eigen::Dynamic>
struct EqualSystem
{
    typedef _Scalar Scalar;
    enum {
        InputsAtCompileTime = NX,
        ValuesAtCompileTime = NY
    };
    typedef Eigen::Matrix<Scalar,InputsAtCompileTime,1> InputType;
    typedef Eigen::Matrix<Scalar,ValuesAtCompileTime,1> ValueType;
    typedef Eigen::Matrix<Scalar,ValuesAtCompileTime,InputsAtCompileTime> JacobianType;
    
    int m_inputs, m_values;
    
    EqualSystem(int inputs, int values) : m_inputs(inputs), m_values(values) {}
    
    int inputs() const { return m_inputs; }
    int values() const { return m_values; }
    
};

struct MyEqualSystem : EqualSystem<double>
{
    MyEqualSystem(int inputs, int values) : EqualSystem(inputs, values), no_parameter(inputs),  no_points(values) 
    {
        
    }
    int operator()(const Eigen::VectorXd &parameter, Eigen::VectorXd &fvec) const
    {
        qreal host = parameter(0);
        qreal guest = parameter(1); 
        
        qreal complex_21 = K11*K21*host*host*guest;
        qreal complex_11 = K11*host*guest;
        qreal complex_12 = K11*K12*host*guest*guest;
        
        fvec(0) = (host + 2*complex_21 + complex_11 + complex_12) - Concen_0(0);
        fvec(1) = (guest + complex_21 + complex_11 + 2*complex_12) - Concen_0(1);
        return 0;
    }
    Eigen::VectorXd Concen_0;
    double A_0, B_0, K21, K11, K12; 
    int no_parameter;
    int no_points;
    int inputs() const { return no_parameter; } // There are two parameters of the model
    int values() const { return no_points; } // The number of observations
};

struct MyEqualSystemNumericalDiff : Eigen::NumericalDiff<MyEqualSystem> {};



inline int SolveEqualSystem(double A_0, double B_0, const QList<qreal> &constants, QList<double > &concentration)
{
       
    if(A_0 == 0 || B_0 == 0)
    {
        concentration<<  A_0 << B_0;
        return 1;
    }
    Eigen::VectorXd parameter(2);
    parameter(0) = A_0;
    parameter(1) = B_0;
    
    Eigen::VectorXd Concen_0(2);
        Concen_0(0) = A_0;
        Concen_0(1) = B_0;
    MyEqualSystem functor(2, 2);
    functor.Concen_0 = Concen_0;
     functor.K21 = qPow(10, constants[0]);
     functor.K11 = qPow(10, constants[1]);
     functor.K12 = qPow(10, constants[2]);
    Eigen::NumericalDiff<MyEqualSystem> numDiff(functor);
    Eigen::LevenbergMarquardt<Eigen::NumericalDiff<MyEqualSystem> > lm(numDiff);
    int iter = 0;
    Eigen::LevenbergMarquardtSpace::Status status = lm.minimizeInit(parameter);
      do {
          for(int i = 0; i < 2; ++i)
            if(parameter(i) < 0)
            {
                std::cout << "numeric error (below zero): " << i << std::endl;
                parameter(i) = qAbs(parameter(i));
            }else if(parameter(i) > Concen_0(i))
            {
                std::cout << "numeric error (above init): " << i << std::endl;
                qreal diff = (parameter(i) -Concen_0(i));
                parameter(i) = diff;
            }
         status = lm.minimizeOneStep(parameter);
         iter++;
      } while (status == -1);
    for(int i = 0; i < 2; ++i)
        if(parameter(i) < 0 || parameter(i) > Concen_0(i))
            std::cout << "final numeric error " << i << " " << parameter(i) << " " << Concen_0(i) << std::endl;
    concentration << double(parameter(0)) << double(parameter(1));
    return iter;
}

ConcentrationSolver::ConcentrationSolver()
{
    setAutoDelete(false);
}

ConcentrationSolver::~ConcentrationSolver()
{
}

void ConcentrationSolver::setInput(double A_0, double B_0, const QList<qreal> &constants)
{
    m_A_0 = A_0;
    m_B_0 = B_0;
    m_constants = constants;
}

void ConcentrationSolver::run()
{
    SolveEqualSystem(m_A_0, m_B_0, m_constants, m_concentration);
}


IItoI_ItoI_ItoII_Model::IItoI_ItoI_ItoII_Model(const DataClass* data): AbstractTitrationModel(data)
{
    setName(tr("2:1/1:1/1:2-Model"));
    for(int i = 0; i < DataPoints(); ++i)
        m_solvers << new ConcentrationSolver();
    InitialGuess();
    setOptParamater(m_complex_constants);
    AbstractTitrationModel::CalculateSignal();
    m_constant_names = QStringList() << tr("2:1") << tr("1:1") << tr("1:2"); 

}

IItoI_ItoI_ItoII_Model::~IItoI_ItoI_ItoII_Model()
{
    qDeleteAll(m_solvers);
}

void IItoI_ItoI_ItoII_Model::CalculateSignal(const QList<qreal> &constants)
{
    m_corrupt = false;
    if(constants.size() == 0)
        return;
    
    QThreadPool *threadpool = new QThreadPool;
    int maxthreads =qApp->instance()->property("threads").toInt();
    threadpool->setMaxThreadCount(maxthreads);
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        
        
        m_solvers[i]->setInput(host_0, guest_0, constants);
        threadpool->start(m_solvers[i]);
    }
    
    threadpool->waitForDone();
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        
        qreal K21= qPow(10, constants.first());
        qreal K11 =qPow(10, constants[1]);
        qreal K12= qPow(10, constants.last());
        QList<double > concentration = m_solvers[i]->Concentrations();
        qreal host = concentration[0];
        qreal guest = concentration[1]; 
        
        qreal complex_11 = K11*host*guest;
        qreal complex_21 = K11*K21*host*host*guest;
        qreal complex_12 = K11*K12*host*guest*guest;         
        for(int j = 0; j < SignalCount(); ++j)
        {
            qreal value = host/host_0*m_pure_signals[j] + complex_11/host_0*m_ItoI_signals[j]+ 2*complex_21/host_0*m_IItoI_signals[j] + complex_12/host_0*m_ItoII_signals[j];
            SetSignal(i, j, value);
        }
    }
    emit Recalculated();
}

QSharedPointer<AbstractTitrationModel> IItoI_ItoI_ItoII_Model::Clone() const
{
    QSharedPointer<AbstractTitrationModel > model = QSharedPointer<IItoI_ItoI_ItoII_Model>(new IItoI_ItoI_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportJSON(ExportJSON());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

void IItoI_ItoI_ItoII_Model::InitialGuess()
{
    ItoI_Model *model = new ItoI_Model(m_data);
    m_K12 = model->Constants()[model->ConstantSize() -1];
    m_K21 = m_K12/2;
    m_K11 = m_K12/2;
    delete model;
    
    m_complex_constants = QList<qreal>() << m_K21 << m_K11 << m_K12;
    setOptParamater(m_complex_constants);
    for(int i = 0; i < SignalCount(); ++i)
    {
        m_ItoII_signals << SignalModel()->lastRow()[i];
        m_ItoI_signals  << SignalModel()->lastRow()[i];
        m_IItoI_signals << SignalModel()->firstRow()[i];
    }
    
    QVector<qreal * > line1, line2, line3, line4;
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        line1 << &m_pure_signals[i];
        line2 << &m_IItoI_signals[i];
        line3 << &m_ItoI_signals[i];
        line4 << &m_ItoII_signals[i];
    }
    m_lim_para = QVector<QVector<qreal * > >() << line1 << line4;
    
    AbstractTitrationModel::CalculateSignal();
}

QVector<qreal> IItoI_ItoI_ItoII_Model::OptimizeParameters_Private(OptimizationType type)
{
    clearOptParameter();
    if(OptimizationType::ComplexationConstants & type)
    {
        addOptParameter(m_complex_constants);
    }
    if((type & ~OptimizationType::IgnoreAllShifts) > (OptimizationType::IgnoreAllShifts))
    {
        if((type & OptimizationType::UnconstrainedShifts))
        {
            addOptParameter(m_IItoI_signals);
            addOptParameter(m_ItoI_signals);
            addOptParameter(m_ItoII_signals);
            if(type & ~(OptimizationType::IgnoreZeroConcentrations))
                addOptParameter(m_pure_signals);
        }
        if(type & ~OptimizationType::UnconstrainedShifts || type & OptimizationType::IntermediateShifts)
        {
            addOptParameter(m_IItoI_signals);
            addOptParameter(m_ItoI_signals);
        }
    }
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void IItoI_ItoI_ItoII_Model::setComplexSignals(const QList<qreal> &list, int i)
{
    for(int j = 0; j < list.size(); ++j)
    {
        if(i == 0 && j < m_IItoI_signals.size())
            m_IItoI_signals[j] = list[j];
        if(i == 1 && j < m_ItoI_signals.size())
            m_ItoI_signals[j] = list[j];
        if(i == 2 && j < m_ItoII_signals.size())
            m_ItoII_signals[j] = list[j];
    }
}


void IItoI_ItoI_ItoII_Model::setPureSignals(const QList<qreal>& list)
{
    for(int i = 0; i < list.size(); ++i)
        if(i < m_pure_signals.size())
            m_pure_signals[i] = list[i];
}

QPair<qreal, qreal> IItoI_ItoI_ItoII_Model::Pair(int i, int j) const
{
    if(i == 0)
    {
        if(j < m_IItoI_signals.size()) 
        {
            return QPair<qreal, qreal>(Constants()[i], m_IItoI_signals[j]);
        } 
        
    }else if(i == 1)
    {
        if(j < m_ItoI_signals.size()) 
        {
            return QPair<qreal, qreal>(Constants()[i], m_ItoI_signals[j]);
        }          
    }
    else if(i == 2)
    {
       if(j < m_ItoII_signals.size()) 
        {
            return QPair<qreal, qreal>(Constants()[i], m_ItoII_signals[j]);
        }   
    }
    return QPair<qreal, qreal>(0, 0);
}



