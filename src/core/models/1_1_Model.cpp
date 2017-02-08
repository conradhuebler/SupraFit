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



#include "src/core/AbstractModel.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"

#include <QtMath>

#include <QtCore/QJsonObject>
#include <QDebug>
#include <QtCore/QDateTime>
#include <cmath>
#include <cfloat>
#include <iostream>

#include "1_1_Model.h"

ItoI_Model::ItoI_Model(const DataClass *data) : AbstractTitrationModel(data)
{
    setName(tr("1:1-Model"));
    InitialGuess();
    m_constant_names = QStringList() << tr("1:1");
}

ItoI_Model::ItoI_Model(const AbstractTitrationModel* model) : AbstractTitrationModel(model)
{
    setName(tr("1:1-Model"));
    InitialGuess();
    m_constant_names = QStringList() << tr("1:1");
}


ItoI_Model::~ItoI_Model() 
{
    
}

void ItoI_Model::InitialGuess()
{
    m_K11 = 4;
    m_ItoI_signals = SignalModel()->lastRow();
    m_pure_signals = SignalModel()->firstRow();
    
    setOptParamater(m_K11);
    QVector<qreal * > line1, line2;
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        line1 << &m_pure_signals[i];
        line2 << &m_ItoI_signals[i];
    }
    m_lim_para = QVector<QVector<qreal * > >()  << line1 << line2;
    
    CalculateSignal();
}

QVector<qreal> ItoI_Model::OptimizeParameters_Private(OptimizationType type)
{    
    if(OptimizationType::ComplexationConstants & type)
        setOptParamater(m_K11);

    if((type & ~OptimizationType::IgnoreAllShifts) > (OptimizationType::IgnoreAllShifts))
    {
        if(type & OptimizationType::UnconstrainedShifts)
        {
            addOptParameter(m_ItoI_signals);
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


qreal ItoI_Model::HostConcentration(qreal host_0, qreal guest_0, QVector< qreal > constants)
{
    if(constants.size() == 0)
        return host_0;
    qreal K11 = qPow(10, constants.first());
    qreal a, b, c;
    qreal complex;
    a = K11;
    b = -1*(K11*host_0+K11*guest_0+1);
    c = K11*guest_0*host_0;
    complex = MinQuadraticRoot(a,b,c);
    return host_0 - complex;
}

void ItoI_Model::CalculateSignal(QVector<qreal > constants)
{  
    m_corrupt = false;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0, guest_0;
        if(*ptr_concentrations)
        {
            host_0 = ConcentrationModel()->data(0,i);
            guest_0 = ConcentrationModel()->data(1,i);
        }else
        {
            host_0 = ConcentrationModel()->data(1,i);
            guest_0 = ConcentrationModel()->data(0,i);
        }
        qreal host = HostConcentration(host_0, guest_0, constants);
        qreal complex = host_0 -host;
        for(int j = 0; j < SignalCount(); ++j)
        {
            qreal value = host/host_0*m_pure_signals[j] + complex/host_0*m_ItoI_signals[j];
            SetSignal(i, j, value);    
        }
    }
    emit Recalculated();
}

// void ItoI_Model::setConstants(QVector< qreal > list)
// {
//     if(list.isEmpty())
//         return;
//     m_K11 = list.first();
// }

void ItoI_Model::setPureSignals(const QVector< qreal > &list)
{
    for(int i = 0; i < list.size(); ++i)
        if(i < m_pure_signals.size())
            m_pure_signals[i] = list[i];
        else if(Type() == 3)
            m_pure_signals<<list[i];
}

void ItoI_Model::setComplexSignals(QVector< qreal > list, int i)
{
    Q_UNUSED(i)
    if(list.size() << m_ItoI_signals.size())
    {
        for(int j = 0; j < list.size(); ++j)
            m_ItoI_signals[j] = list[j];
    }
}


QPair< qreal, qreal > ItoI_Model::Pair(int i, int j) const
{
    Q_UNUSED(i);
    if(j < m_ItoI_signals.size()) 
    {
        return QPair<qreal, qreal>(m_K11, m_ItoI_signals[j]);
    }
    return QPair<qreal, qreal>(0, 0);
}

QSharedPointer<AbstractTitrationModel > ItoI_Model::Clone() const
{
    QSharedPointer<AbstractTitrationModel > model = QSharedPointer<ItoI_Model>(new ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportJSON(ExportJSON());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    return model;
    
}

#include "1_1_Model.moc"
