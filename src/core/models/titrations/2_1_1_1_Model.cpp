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
    m_local_parameter = new DataTable(3, SeriesCount(), this);
    InitialGuess();   
    
    AbstractTitrationModel::Calculate();
}
    
IItoI_ItoI_Model::~IItoI_ItoI_Model()
{
    
}

void IItoI_ItoI_Model::InitialGuess()
{
    ItoI_Model *model = new ItoI_Model(m_data);
    m_K11 = model->GlobalParameter()[model->GlobalParameterSize() -1];
    m_K21 = m_K11/2;
    delete model;
    
    m_global_parameter = QList<qreal>() << m_K21 << m_K11;
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
    AbstractTitrationModel::Calculate();
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

void IItoI_ItoI_Model::CalculateVariables()
{
    m_corrupt = false;
   
    m_sum_absolute = 0;
    m_sum_squares = 0;
    
    qreal K21 = qPow(10, GlobalParameter().first());
    qreal K11 = qPow(10, GlobalParameter().last());
    
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = HostConcentration(host_0, guest_0, GlobalParameter());
        qreal guest = guest_0/(K11*host+K11*K21*host*host+1);
        qreal complex_11 = K11*host*guest;
        qreal complex_21 = K11*K21*host*host*guest;
        
        Vector vector(5);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        vector(3) = complex_21;
        vector(4) = complex_11;
        
        SetConcentration(i, vector);
        
        for(int j = 0; j < SeriesCount(); ++j)
        {
            qreal value = host/host_0*m_local_parameter->data(0, j) + 2*complex_21/host_0*m_local_parameter->data(1, j) + complex_11/host_0*m_local_parameter->data(2, j);
            SetValue(i, j, value);
        }
        
    }
    emit Recalculated();
}

QVector<qreal> IItoI_ItoI_Model::OptimizeParameters_Private(OptimizationType type)
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
            if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
                addLocalParameter(0);
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


QSharedPointer<AbstractModel > IItoI_ItoI_Model::Clone() const
{
    QSharedPointer<IItoI_ItoI_Model > model = QSharedPointer<IItoI_ItoI_Model>(new IItoI_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;    
}

qreal IItoI_ItoI_Model::BC50()
{
    qreal b11 = qPow(10,GlobalParameter()[1]);
    qreal b21 = qPow(10,(GlobalParameter()[0]+GlobalParameter()[1]));
    qreal bc50 = -b11/b21/double(2) + sqrt(qPow(b11/double(2)/b21,2)+1/b21);
    return bc50;
}


#include "2_1_1_1_Model.moc"
