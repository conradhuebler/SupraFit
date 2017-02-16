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
#include "src/core/libmath.h"
#include "src/core/minimizer.h"

#include <QtMath>

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
    
    ParseJson();
    InitializeCupofTea();
    InitialGuess();
    
}


ScriptModel::~ScriptModel() 
{
    
}

void ScriptModel::ParseJson()
{
    QJsonObject object = m_json["model_definition"].toObject();
    
    setName(object["name"].toString());
    QString constant_names = object["constant_names"].toString().simplified();
    m_constant_names = constant_names.split(" ");
    for(int i = 0; i < m_constant_names.size(); ++i)
        m_complex_constants << 4/m_constant_names.size();
    
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
    m_corrupt = false;
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
