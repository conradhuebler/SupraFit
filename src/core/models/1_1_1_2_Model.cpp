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
#include "1_1_1_2_Model.h"

ItoI_ItoII_Model::ItoI_ItoII_Model(const DataClass* data) : AbstractTitrationModel(data)
{
    setName(tr("1:1/1:2-Model"));
    m_complex_signal_parameter = Eigen::MatrixXd::Zero(SignalCount(), 2);
    InitialGuess();
    setOptParamater(m_complex_constants);
    AbstractTitrationModel::CalculateSignal();
    m_constant_names = QStringList() << tr("1:1") << tr("1:2");
}

ItoI_ItoII_Model::~ItoI_ItoII_Model()
{
    
}

void ItoI_ItoII_Model::InitialGuess()
{
    ItoI_Model *model = new ItoI_Model(m_data);
    m_K12 = model->Constants()[model->ConstantSize() -1];
    m_K11 = m_K12/2;
    delete model;
    
    m_complex_constants = QList<qreal>() << m_K11 << m_K12;
    setOptParamater(m_complex_constants);
    
//     for(int i = 0; i < SignalCount(); ++i)
//     {
//         m_ItoI_signals << ( SignalModel()->data(i,0) +  SignalModel()->data(i,SignalCount() - 1))/2;
//         m_ItoII_signals << SignalModel()->data(i,SignalCount() - 1);
//     }
    
    QVector<qreal * > line1, line2;
    for(int i = 0; i < m_pure_signals_parameter.size(); ++i)
    {
        line1 << &m_pure_signals_parameter(i);
        line2 << &m_complex_signal_parameter(i,0);
    }
    m_lim_para = QVector<QVector<qreal * > >() << line1 << line2;
    
    AbstractTitrationModel::CalculateSignal();
}

QVector<qreal> ItoI_ItoII_Model::OptimizeParameters_Private(OptimizationType type)
{
    if(OptimizationType::ComplexationConstants & type)
    {
        setOptParamater(m_complex_constants);
    }
    if((type & ~OptimizationType::IgnoreAllShifts) > (OptimizationType::IgnoreAllShifts))
    {
        if((type & OptimizationType::UnconstrainedShifts))
        {
            addOptParameterList_fromConstant(1);
            addOptParameterList_fromConstant(0);
            if(type & ~(OptimizationType::IgnoreZeroConcentrations))
                addOptParameterList_fromPure(0);
        }
        if(type & ~OptimizationType::UnconstrainedShifts || type & OptimizationType::IntermediateShifts)
        {
            addOptParameterList_fromConstant(0);
        }
        
    } 
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

/*
void ItoI_ItoII_Model::setComplexSignals(const QList< qreal > &list, int i)
{
    for(int j = 0; j < list.size(); ++j)
    {
        if(i == 0 && j < m_ItoI_signals.size())
            m_ItoI_signals[j] = list[j];        
        if(i == 1 && j < m_ItoII_signals.size())
            m_ItoII_signals[j] = list[j];
    }
}
*/
qreal ItoI_ItoII_Model::HostConcentration(qreal host_0, qreal guest_0, const QList<qreal > &constants)
{
    
    if(constants.size() < 2)
        return host_0;
    
    qreal K12 = qPow(10, constants.last());
    qreal K11 = qPow(10, constants.first());    
    qreal guest = GuestConcentration(host_0, guest_0, constants);
    qreal host;
    host = host_0/(K11*guest+K11*K12*guest*guest+1);
    return host;
}

qreal ItoI_ItoII_Model::GuestConcentration(qreal host_0, qreal guest_0, const QList< qreal > &constants)
{
    
    if(constants.size() < 2)
        return guest_0;
    
    qreal K12 = qPow(10, constants.last());
    qreal K11 = qPow(10, constants.first());
    qreal a = K11*K12;
    qreal b = K11*(2*K12*host_0-K12*guest_0+1);
    qreal c = K11*(host_0-guest_0)+1;
    qreal guest = MinCubicRoot(a,b,c, -guest_0);
    return guest;
}

void ItoI_ItoII_Model::CalculateSignal(const QList<qreal > &constants)
{
    m_corrupt = false;
    if(constants.size() == 0)
        return;        
    
    qreal K12= qPow(10, constants.last());
    qreal K11 = qPow(10, constants.first());
    
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        
        qreal host = HostConcentration(host_0, guest_0, constants);
        qreal guest = GuestConcentration(host_0, guest_0, constants);
        qreal complex_11 = K11*host*guest;
        qreal complex_12 = K11*K12*host*guest*guest;
        
        for(int j = 0; j < SignalCount(); ++j)
        {
            qreal value = host/host_0*m_pure_signals_parameter(j, 0) + complex_11/host_0*m_complex_signal_parameter(j,0)+ complex_12/host_0*m_complex_signal_parameter(j,1);
//             qreal value = host/host_0*m_pure_signals_parameter(j, 0) + complex_11/host_0*m_complex_signal_parameter(j,1)+ 2*complex_21/host_0*m_complex_signal_parameter(j, 0);
            SetSignal(i, j, value);
        }
        
    }
    emit Recalculated();

}
/*
void ItoI_ItoII_Model::setPureSignals(const QList< qreal > &list)
{
    for(int i = 0; i < list.size(); ++i)
        if(i < m_pure_signals.size())
        {
            m_pure_signals[i] = list[i];
        }
}


QPair< qreal, qreal > ItoI_ItoII_Model::Pair(int i, int j) const
{
    if(i == 0)
    {
        if(j < m_ItoI_signals.size()) 
        {
            return QPair<qreal, qreal>(Constants()[i], m_ItoI_signals[j]);
        }          
    }
    else if(i == 1)
    {
        
        
        if(j < m_ItoII_signals.size()) 
        {
            return QPair<qreal, qreal>(Constants()[i], m_ItoII_signals[j]);
        } 
        
    }
    return QPair<qreal, qreal>(0, 0);
}
*/
QSharedPointer<AbstractTitrationModel > ItoI_ItoII_Model::Clone() const
{
    QSharedPointer<ItoI_ItoII_Model > model = QSharedPointer<ItoI_ItoII_Model>(new ItoI_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportJSON(ExportJSON());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "1_1_1_2_Model.moc"
