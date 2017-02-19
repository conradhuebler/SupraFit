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

ScriptModel::ScriptModel(const DataClass *data, const QJsonObject &json) : AbstractTitrationModel(data), m_json(json)
{
    chai = new chaiscript::ChaiScript;
    
    for(int i = 0; i < DataPoints(); ++i)
        m_solvers << new ConcentrationSolver(this);
    
    ParseJson();
    InitializeCupofTea();
    m_complex_signal_parameter = Eigen::MatrixXd::Zero(SignalCount(), m_complex_constants.size());
    InitialGuess();
    
}


ScriptModel::~ScriptModel() 
{
    
}

MassResults ScriptModel::MassBalance(qreal A, qreal B)
{
    MassResults result;
    //     QMutexLocker locker(&mutex);
    
    /*    
    
     for(int i = 0; i < Constants().size(); ++i)
        chai->add(chaiscript::var(qPow(10,Constant(i))), m_constant_names[i].toStdString());
    chai->add(chaiscript::var(A), "A");
    chai->add(chaiscript::var(B), "B");*/
    QList<qreal > variables;
    variables << A << B;
    Vector values(2);
    values(0) = 0;
    values(1) = 0;
    Vector Components(ConstantSize());
    QStringList keys = m_complex_hashed.keys();
//     qreal m = chai->eval<double>(m_mass_balance[0]);
//     std::vector<chaiscript::Boxed_Value> m = chai->eval_file< std::vector<chaiscript::Boxed_Value> >("/media/Daten/conrad/Programme/nmr2fit/src/core/models/1_1_model.chai");
    
    /*
     * Allways parsing the complexation structure
     * 
     */
    
    for(int i = 0; i < m_component_list.size(); ++i)
    {
        for(int j = 0; j < m_constant_names.size(); ++j) // b11, b21, b12 ...
        {
            qreal compound = 1;
            compound *=  qPow(10,Constant(j));
            for(int k = 0; k < m_component_list.size(); ++k)
            {
                int pow = m_complex_map[m_component_list[k]].toInt();
                if(pow == 1)
                    compound *= variables[k];
                else
                    compound *= qPow(variables[k], pow);
                
                Components(j) = compound;
                if(k == 0)
                    Components(j) *= pow;
                if(k == i)
                    compound *= pow;
            }
            values(i) += compound;
        }
        
//           values(i) = qPow(10, Constant(0))*A*B;
 //        values(i) = m;
    }
    result.Components = Components;
    result.MassBalance = values;
    return result;
}


void ScriptModel::CreateMassBalanceEquation(const QJsonObject &json)
{
    m_complex_hashed = json["complexes"].toObject().toVariantHash();
    QHash<QString, QString> mass_balance;
    for(const QString &str_component : (m_component_list))
    {
        for(const QString &str_constant : m_constant_names)
        {
            m_complex_map = m_complex_hashed[str_constant].toMap();
            mass_balance[str_component] += str_constant;
            for(const QString &str_complex : (m_complex_map.keys()))
            {
                if(m_complex_map[str_complex].toInt() != 1)
                {
                    if(str_component == str_complex)
                        mass_balance[str_component] += "*"+ QString(m_complex_map[str_complex].toByteArray() );
                    mass_balance[str_component] += "*"+("qPow(" + str_complex +",%1)" ).arg(m_complex_map[str_complex].toInt());
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
    for(int i = 0; i < m_complex_signal_parameter.cols(); ++i)
        m_complex_signal_parameter.col(i) = SignalModel()->lastRow();
    
    setOptParamater(m_complex_constants);
    QVector<qreal * > line1, line2;
    for(int i = 0; i < m_complex_signal_parameter.rows(); ++i)
    {
        line1 << &m_pure_signals_parameter(i, 0);
        line2 << &m_complex_signal_parameter(i,0);
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
            for(int i = 0; i < m_complex_signal_parameter.cols(); ++i)
                addOptParameterList_fromConstant(i);
            if(type & ~OptimizationType::IgnoreZeroConcentrations)
            {
                addOptParameterList_fromPure(0);
            }
        }
    }
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


void ScriptModel::CalculateSignal(const QList<qreal > &constants)
{  
    m_corrupt = false;
    if(constants.size() == 0)
        return;
    
    for(int i = 0; i < Constants().size(); ++i)
        chai->add(chaiscript::var(qPow(10,Constant(i))), m_constant_names[i].toStdString());
    
    QThreadPool *threadpool = new QThreadPool;
    int maxthreads =qApp->instance()->property("threads").toInt();
    threadpool->setMaxThreadCount(maxthreads);
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        
        
        m_solvers[i]->setInput(host_0, guest_0);
        //         m_solvers[i]->run();
        threadpool->start(m_solvers[i]);
    }
    threadpool->waitForDone();
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        QList<qreal > concentrations = m_solvers[i]->Concentrations();
        MassResults result = MassBalance(concentrations[0], concentrations[1]);
            
        for(int j = 0; j < SignalCount(); ++j)
        {
            qreal value = concentrations[0]/host_0*m_pure_signals_parameter(j, 0);
            for(int k = 0; k < result.Components.rows(); ++k)
                 value += result.Components[k]/host_0*m_complex_signal_parameter(j,k);
            SetSignal(i, j, value);
        }
    }
    
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
