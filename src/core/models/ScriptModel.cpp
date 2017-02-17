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
#include "src/core/AbstractModel.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/NonLinearOptimization>

#include <QtMath>

#include <QCoreApplication>
#include <QtCore/QMutexLocker>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QDebug>
#include <QtCore/QDateTime>

#include <cmath>
#include <cfloat>
#include <iostream>

#include <chaiscript/chaiscript.hpp>


#include "ScriptModel.h"

/*

template<typename _Scalar, int NX = Eigen::Dynamic, int NY = Eigen::Dynamic>
struct ScriptedEqualSystem
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
    
    ScriptedEqualSystem(int inputs, int values) : m_inputs(inputs), m_values(values) {}
    
    int inputs() const { return m_inputs; }
    int values() const { return m_values; }
    
};

struct MyScripteEqualSystem : ScriptedEqualSystem<double>
{
    MyScripteEqualSystem(int inputs, int values, QPointer<ScriptModel> model) : ScriptedEqualSystem(inputs, values), no_parameter(inputs),  no_points(values), m_model(model) 
    {
        
    }
    int operator()(const Eigen::VectorXd &parameter, Eigen::VectorXd &fvec) const
    {
        qreal A = parameter(0);
        qreal B = parameter(1); 
        
        QList<qreal > balance = m_model.data()->MassBalance(A, B);
        
        fvec(0) = (A + balance[0]) - Concen_0(0);
        fvec(1) = (B + balance[1]) - Concen_0(1);
        return 0;
    }
    QPointer<ScriptModel > m_model;
    Eigen::VectorXd Concen_0;
    int no_parameter;
    int no_points;
    int inputs() const { return no_parameter; } // There are two parameters of the model
    int values() const { return no_points; } // The number of observations
};

struct MyScripteEqualSystemNumericalDiff : Eigen::NumericalDiff<MyScripteEqualSystem> {};



inline int SolveScriptedEqualSystem(double A_0, double B_0, QList<double > &concentration, QPointer<ScriptModel> model)
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
        
    MyScripteEqualSystem functor(2, 2, model);
    functor.Concen_0 = Concen_0;

    Eigen::NumericalDiff<MyScripteEqualSystem> numDiff(functor);
    Eigen::LevenbergMarquardt<Eigen::NumericalDiff<MyScripteEqualSystem> > lm(numDiff);
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
*/

ScriptModel::ScriptModel(const DataClass *data, const QJsonObject &json) : AbstractTitrationModel(data), m_json(json)
{
    chai = new chaiscript::ChaiScript;
    
    for(int i = 0; i < DataPoints(); ++i)
        m_solvers << new ConcentrationSolver(this);
    
    ParseJson();
    InitializeCupofTea();
    InitialGuess();
    
}


ScriptModel::~ScriptModel() 
{
    
}

Vector ScriptModel::MassBalance(qreal A, qreal B)
{
//     QMutexLocker (&mutex);

    chai->add(chaiscript::var(A), "A");
    chai->add(chaiscript::var(B), "B");

    Vector values(2);
    for(int i = 0; i < m_mass_balance.size(); ++i)
        values(i) = chai->eval<double>(m_mass_balance[i]);
 
    return values;
}


void ScriptModel::CreateMassBalanceEquation(const QJsonObject &json)
{
    QVariantHash complex_hashed = json["complexes"].toObject().toVariantHash();
    QHash<QString, QString> mass_balance;
    for(const QString &str_component : (m_component_list))
    {
        for(const QString &str_constant : m_constant_names)
        {
            QVariantMap complex = complex_hashed[str_constant].toMap();
            mass_balance[str_component] += str_constant;
            for(const QString &str_complex : (complex.keys()))
            {
                if(complex[str_complex].toInt() != 1)
                {
                    if(str_component == str_complex)
                        mass_balance[str_component] += "*"+ QString(complex[str_complex].toByteArray() );
                    mass_balance[str_component] += "*"+("qPow(" + str_complex +",%1)" ).arg(complex[str_complex].toInt());
                }else
                    mass_balance[str_component] += "*" +  str_complex;
            }
            mass_balance[str_component] += "+";
        }
        mass_balance[str_component].chop(1);
    }
    
    for(const QString &str : mass_balance.keys())
        m_mass_balance << mass_balance[str].toStdString();
}

void ScriptModel::ParseJson()
{
    QJsonObject object = m_json["model_definition"].toObject();
    
    setName(object["name"].toString());
    QJsonObject complexes = object["complexes"].toObject();
    m_constant_names = complexes.keys();
    for(const QString &str : m_constant_names)
    {
        QJsonObject components = complexes[str].toObject();
        m_component_list << components.keys();
    }
    m_component_list.removeDuplicates();



    for(int i = 0; i < m_constant_names.size(); ++i)
        m_complex_constants << 4/m_constant_names.size();
    
    
    
    CreateMassBalanceEquation(object);
    
}

void ScriptModel::InitializeCupofTea()
{
    chai->add(chaiscript::fun(&qPow), "qPow");
    chai->add(chaiscript::fun(&qSqrt), "qSqrt");
    
    chai->add(chaiscript::base_class<DataClass, AbstractTitrationModel>());
    chai->add(chaiscript::base_class<DataClass, ScriptModel>());
    chai->add(chaiscript::base_class<AbstractTitrationModel, ScriptModel>());
    chai->add(chaiscript::fun(&ScriptModel::Constant), "Constant");
    chai->add(chaiscript::fun(&ScriptModel::InitialHostConcentration), "InitialHostConcentration");
    chai->add(chaiscript::fun(&ScriptModel::InitialGuestConcentration), "InitialGuestConcentration");
    chai->add(chaiscript::var(this), "model");
    
//     chai->eval(m_content);
    
}

void ScriptModel::InitialGuess()
{
    m_signals = SignalModel()->lastRow();
    m_pure_signals = SignalModel()->firstRow();
    
    setOptParamater(m_complex_constants);
    QVector<qreal * > line1, line2;
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        line1 << &m_pure_signals[i];
        line2 << &m_signals[i];
    }
    m_lim_para = QVector<QVector<qreal * > >()  << line1 << line2;
    
    AbstractTitrationModel::CalculateSignal();
}

QVector<qreal> ScriptModel::OptimizeParameters_Private(OptimizationType type)
{    
    if(OptimizationType::ComplexationConstants & type)
        setOptParamater(m_complex_constants);
    
    if((type & ~OptimizationType::IgnoreAllShifts) > (OptimizationType::IgnoreAllShifts))
    {
        if(type & OptimizationType::UnconstrainedShifts)
        {
            addOptParameter(m_signals);
            if(type & ~OptimizationType::IgnoreZeroConcentrations)
            {
                addOptParameter(m_pure_signals);
            }
        }
    }
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}



void ScriptModel::setPureSignals(const QList< qreal > &list)
{
    for(int i = 0; i < list.size(); ++i)
        if(i < m_pure_signals.size())
            m_pure_signals[i] = list[i];
        else if(Type() == 3)
            m_pure_signals<<list[i];
}

void ScriptModel::setComplexSignals(const QList< qreal > &list, int i)
{
    Q_UNUSED(i)
    if(list.size() << m_signals.size())
    {
        for(int j = 0; j < list.size(); ++j)
            m_signals[j] = list[j];
    }
}


QPair< qreal, qreal > ScriptModel::Pair(int i, int j) const
{
    Q_UNUSED(i);
    if(j < m_signals.size()) 
    {
        return QPair<qreal, qreal>(Constant(0), m_signals[j]);
    }
    return QPair<qreal, qreal>(0, 0);
}

void ScriptModel::CalculateSignal(const QList<qreal > &constants)
{  
 /*   m_corrupt = false;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        chai->add(chaiscript::var(i), "i");
        double host = chai->eval<double>(m_mass_balance);
        qreal complex = host_0 -host;
        for(int j = 0; j < SignalCount(); ++j)
        {
            qreal value = chai->eval<double>(m_signal_calculation);; //host/host_0*m_pure_signals[j] + complex/host_0*m_ItoI_signals[j];
            SetSignal(i, j, value);    
        }
    }
    emit Recalculated();*/
 
 m_corrupt = false;
    if(constants.size() == 0)
        return;
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    
    for(int i = 0; i < Constants().size(); ++i)
        chai->add(chaiscript::var(qPow(10,Constant(i))), m_constant_names[i].toStdString());
    QThreadPool *threadpool = new QThreadPool;
    int maxthreads =qApp->instance()->property("threads").toInt();
    threadpool->setMaxThreadCount(maxthreads);
    
    /*QThreadPool *threadpool = new QThreadPool;
    int maxthreads =qApp->instance()->property("threads").toInt();
    threadpool->setMaxThreadCount(maxthreads);*/
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        
        /*
        m_solvers[i]->setInput(host_0, guest_0, constants_pow);
        threadpool->start(m_solvers[i]);
        */
        QList<qreal > concentrations;
        
        quint64 t0 = QDateTime::currentMSecsSinceEpoch();
        m_solvers[i]->setInput(host_0, guest_0);
        m_solvers[i]->run();
//         threadpool->start(m_solvers[i]);
        quint64 t1 = QDateTime::currentMSecsSinceEpoch();
        std::cout << t1-t0 << " msecs for all signals!" << std::endl;
        
        
        qreal complex = qPow(10,Constant(0))*concentrations[0]*concentrations[1];         
        for(int j = 0; j < SignalCount(); ++j)
        {
            qreal value = concentrations[0]/host_0*m_pure_signals[j] + complex/host_0*m_signals[j];
            SetSignal(i, j, value);
        }
    }
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    std::cout << t1-t0 << " msecs for all signals!" << std::endl;
   /* threadpool->waitForDone();
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        

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
    }*/
    emit Recalculated();
 
 
 
}

QSharedPointer<AbstractTitrationModel > ScriptModel::Clone() const
{
    QSharedPointer<AbstractTitrationModel > model = QSharedPointer<ScriptModel>(new ScriptModel(this, m_json), &QObject::deleteLater);
    model.data()->ImportJSON(ExportJSON());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
    
}

qreal ScriptModel::BC50()
{
    return 0;
}





#include "ScriptModel.moc"
