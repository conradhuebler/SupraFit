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


#include "src/core/toolset.h"
#include "src/core/models.h"
#include "src/core/libmath.h"
#include <QtMath>
#include <QtCore/QJsonObject>
#include <QDebug>
#include <QtCore/QDateTime>

#include <cmath>
#include <cfloat>
#include <iostream>
#include <functional>

#include "1_1_1_2_Model.h"

ItoI_ItoII_Model::ItoI_ItoII_Model(DataClass* data) : AbstractTitrationModel(data)
{
    setName(tr("1:1/1:2-Model"));
    m_local_parameter = new DataTable(3, SeriesCount(), this);
    InitialGuess();
    AbstractTitrationModel::Calculate();
}

ItoI_ItoII_Model::~ItoI_ItoII_Model()
{
    
}

void ItoI_ItoII_Model::InitialGuess()
{
    ItoI_Model *model = new ItoI_Model(m_data);
    m_K12 = model->GlobalParameter()[model->GlobalParameterSize() -1];
    m_K11 = m_K12/2;
    delete model;
    
    m_global_parameter = QList<qreal>() << m_K11 << m_K12;
    setOptParamater(m_global_parameter);
    
    m_local_parameter->setColumn(DependentModel()->firstRow(), 0);
    m_local_parameter->setColumn(DependentModel()->firstRow(), 1);
    m_local_parameter->setColumn(DependentModel()->lastRow(), 2);
    
    QVector<qreal * > line1, line2;
    for(int i = 0; i < SeriesCount(); ++i)
    {
        line1 << &m_local_parameter->data(0, i); 
        line2 << &m_local_parameter->data(2, i); 
    }
    m_lim_para = QVector<QVector<qreal * > >() << line1 << line2;
    
    AbstractTitrationModel::Calculate();
}

QVector<qreal> ItoI_ItoII_Model::OptimizeParameters_Private(OptimizationType type)
{
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
    {
        setOptParamater(m_global_parameter);
    }
    if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
        if((type & OptimizationType::UnconstrainedShifts) == OptimizationType::UnconstrainedShifts)
        {
            addLocalParameter(1);
            addLocalParameter(2);
            if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
                addLocalParameter(1);
        }
        if(((type & OptimizationType::ConstrainedShifts) == OptimizationType::ConstrainedShifts) && ((type & OptimizationType::IntermediateShifts) == OptimizationType::IntermediateShifts))
        {
            addLocalParameter(1);
        }
        
    } 
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


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

void ItoI_ItoII_Model::CalculateVariables()
{
    m_corrupt = false;

    m_sum_absolute = 0;
    m_sum_squares = 0;
    
    qreal K12= qPow(10, GlobalParameter().last());
    qreal K11 = qPow(10, GlobalParameter().first());
    
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        
        qreal host = HostConcentration(host_0, guest_0, GlobalParameter());
        qreal guest = GuestConcentration(host_0, guest_0, GlobalParameter());
        qreal complex_11 = K11*host*guest;
        qreal complex_12 = K11*K12*host*guest*guest;
                
        Vector vector(5);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        vector(3) = complex_11;
        vector(4) = complex_12;
        SetConcentration(i, vector);
        
        for(int j = 0; j < SeriesCount(); ++j)
        {
            qreal value = host/host_0*m_local_parameter->data(0, j) + complex_11/host_0*m_local_parameter->data(1, j)+ complex_12/host_0*m_local_parameter->data(2, j);
            SetValue(i, j, value);
        }
        
    }
    emit Recalculated();

}

QSharedPointer<AbstractModel > ItoI_ItoII_Model::Clone()
{
    QSharedPointer<ItoI_ItoII_Model > model = QSharedPointer<ItoI_ItoII_Model>(new ItoI_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

qreal ItoI_ItoII_Model::Y(qreal x, const QVector<qreal> &parameter)
{
    if(2 != parameter.size())
        return 0;
    qreal b11 = parameter[0];
    qreal b12 = parameter[1];
    qreal B = -b11/2/b12 + sqrt((qPow(b11,2))/(4*qPow(b12,2))+((x/(1-x))/b12));
    qreal A = 1/(b11+2*b12*B);
    return 1./(A + b11*A*B+b12*A*qPow(B,2));
}


qreal ItoI_ItoII_Model::BC50()
{
    qreal b11 = qPow(10,GlobalParameter(0));
    qreal b12 = qPow(10,GlobalParameter(0)+GlobalParameter(1));
    QVector<qreal> parameter;
    parameter << b11 << b12;
    std::function<qreal(qreal, const QVector<qreal> &)> function = Y;
    qreal integ = ToolSet::SimpsonIntegrate(0, 1, function, parameter);
    return double(1)/double(2)/integ;
}

#include "1_1_1_2_Model.moc"
