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



#include "src/core/models.h"

#include "src/core/libmath.h"
#include <QtMath>
#include <QtCore/QJsonObject>
#include <QDebug>
#include <QtCore/QDateTime>
#include <cmath>
#include <cfloat>
#include <iostream>
#include "2_1_1_1_Model.h"

IItoI_ItoI_Model::IItoI_ItoI_Model(const DataClass* data) : AbstractTitrationModel(data)
{
    setName(tr("2:1/1:1-Model"));

    InitialGuess();   
    
    setOptParamater(m_complex_constants);
    AbstractTitrationModel::CalculateSignal();
    
    m_constant_names = QStringList() << tr("2:1") << tr("1:1");
}
IItoI_ItoI_Model::~IItoI_ItoI_Model()
{
    
}

void IItoI_ItoI_Model::InitialGuess()
{
    ItoI_Model *model = new ItoI_Model(m_data);
    m_K11 = model->Constants()[model->ConstantSize() -1];
    m_K21 = m_K11/2;
    delete model;
    
    m_complex_constants = QList<qreal>() << m_K21 << m_K11;
    setOptParamater(m_complex_constants);
    for(int i = 0; i < SignalCount(); ++i)
    {
        m_ItoI_signals <<SignalModel()->lastRow()[i];
        m_IItoI_signals << SignalModel()->firstRow()[i];
    }
    
    QVector<qreal * > line1, line2;
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        line1 << &m_pure_signals[i];
        line2 << &m_ItoI_signals[i];
    }
    m_lim_para = QVector<QVector<qreal * > >() << line1 << line2;
    /*
    Minimizer *mini = new Minimizer;
    QSharedPointer<AbstractTitrationModel > model = QSharedPointer<ItoI_Model>(this, &QObject::deleteLater);
    model.data()->ImportJSON(ExportJSON());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    mini->setModel(model);
    mini->Minimize(OptimizationType::ComplexationConstants);
    ImportJSON(mini->Parameter());
    
    delete mini;*/
    AbstractTitrationModel::CalculateSignal();
}

void IItoI_ItoI_Model::setComplexSignals(const QList< qreal > &list, int i)
{
    for(int j = 0; j < list.size(); ++j)
    {
        
        if(i == 0 && j < m_IItoI_signals.size())
            m_IItoI_signals[j] = list[j];
        if(i == 1 && j < m_ItoI_signals.size())
            m_ItoI_signals[j] = list[j];
    }
}

qreal IItoI_ItoI_Model::HostConcentration(qreal host_0, qreal guest_0, const QList<qreal > &constants)
{
    
    if(constants.size() < 2)
        return host_0;
    qreal K21= qPow(10, constants.first());
    qreal K11 = qPow(10, constants.last());
    qreal host;
    qreal a, b, c;
    a = K11*K21;
    b = K11*(2*K21*guest_0-K21*host_0+1);
    c = K11*(guest_0-host_0)+1;
    host = MinCubicRoot(a,b,c, -host_0);
    return host;
}

void IItoI_ItoI_Model::CalculateSignal(const QList<qreal > &constants)
{
    m_corrupt = false;
    if(constants.size() == 0)
        return;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        
        qreal K21= qPow(10, constants.first());
        qreal K11 = qPow(10, constants.last());
        qreal host = HostConcentration(host_0, guest_0, constants);
        qreal guest = guest_0/(K11*host+K11*K21*host*host+1);
        qreal complex_11 = K11*host*guest;
        qreal complex_21 = K11*K21*host*host*guest;
        
        
        for(int j = 0; j < SignalCount(); ++j)
        {
            qreal value = host/host_0*m_pure_signals[j] + complex_11/host_0*m_ItoI_signals[j]+ 2*complex_21/host_0*m_IItoI_signals[j];
            SetSignal(i, j, value);
        }
        
    }
    emit Recalculated();
}

void IItoI_ItoI_Model::setPureSignals(const QList< qreal > &list)
{
    for(int i = 0; i < list.size(); ++i)
        if(i < m_pure_signals.size())
            m_pure_signals[i] = list[i];
}


QPair< qreal, qreal > IItoI_ItoI_Model::Pair(int i, int j) const
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
    return QPair<qreal, qreal>(0, 0);
}

QVector<qreal> IItoI_ItoI_Model::OptimizeParameters_Private(OptimizationType type)
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
            if(type & ~(OptimizationType::IgnoreZeroConcentrations))
                addOptParameter(m_pure_signals);
        }
        if(type & ~OptimizationType::UnconstrainedShifts || type & OptimizationType::IntermediateShifts)
        {
            addOptParameter(m_IItoI_signals);
        }
    }
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


QSharedPointer<AbstractTitrationModel > IItoI_ItoI_Model::Clone() const
{
    QSharedPointer<AbstractTitrationModel > model = QSharedPointer<IItoI_ItoI_Model>(new IItoI_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportJSON(ExportJSON());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    return model;
    
}

#include "2_1_1_1_Model.moc"
