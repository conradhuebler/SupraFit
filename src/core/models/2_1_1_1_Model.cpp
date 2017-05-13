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
    m_complex_signal_parameter = Eigen::MatrixXd::Zero(SignalCount(), 2);
    InitialGuess();   
    
    AbstractTitrationModel::Calculate();
    
    m_constant_names = QStringList() << tr("2:1") << tr("1:1");
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
        
    m_complex_signal_parameter.col(0) = DependentModel()->firstRow();
    m_complex_signal_parameter.col(1) = DependentModel()->lastRow();
    m_pure_signals_parameter = DependentModel()->firstRow();
    
    QVector<qreal * > line1, line2;
    for(int i = 0; i < m_pure_signals_parameter.size(); ++i)
    {
        line1 << &m_pure_signals_parameter(i);
        line2 << &m_complex_signal_parameter(i,1);
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

void IItoI_ItoI_Model::CalculateVariables(const QList<qreal > &constants)
{
    m_corrupt = false;

    if(constants.size() == 0)
        return;
    
    m_sum_absolute = 0;
    m_sum_squares = 0;
    
    qreal K21= qPow(10, constants.first());
    qreal K11 = qPow(10, constants.last());
    
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = HostConcentration(host_0, guest_0, constants);
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
        
        for(int j = 0; j < SignalCount(); ++j)
        {
            qreal value = host/host_0*m_pure_signals_parameter(j, 0) + 2*complex_21/host_0*m_complex_signal_parameter(j, 0) + complex_11/host_0*m_complex_signal_parameter(j,1);
            SetValue(i, j, value);
        }
        
    }
    emit Recalculated();
}

QVector<qreal> IItoI_ItoI_Model::OptimizeParameters_Private(OptimizationType type)
{    
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
    {
        addOptParameter(m_global_parameter);
    }
    if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
        if((type & OptimizationType::UnconstrainedShifts) == OptimizationType::UnconstrainedShifts)
        {
            addOptParameterList_fromConstant(0);
            addOptParameterList_fromConstant(1);
            if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
                addOptParameterList_fromPure(0);
        }
        if(((type & OptimizationType::ConstrainedShifts) == OptimizationType::ConstrainedShifts) && ((type & OptimizationType::IntermediateShifts) == OptimizationType::IntermediateShifts))
        {
            addOptParameterList_fromConstant(0);
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
