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

#include <QtCore/QFile>
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
    m_complex_signal_parameter = Eigen::MatrixXd::Zero(SignalCount(), 1);
    InitialGuess();
    m_complex_constants = QList<qreal>() << m_K11;
    
    m_constant_names = QStringList() << tr("1:1");
}

ItoI_Model::ItoI_Model(const AbstractTitrationModel* model) : AbstractTitrationModel(model)
{
    setName(tr("1:1-Model"));
    m_complex_signal_parameter = Eigen::MatrixXd::Zero(SignalCount(), 1);
    InitialGuess();
    m_complex_constants = QList<qreal>() << m_K11;
    
    m_constant_names = QStringList() << tr("1:1");
}


ItoI_Model::~ItoI_Model() 
{
    
}

void ItoI_Model::InitialGuess()
{
    m_K11 = 4;
    
    m_complex_signal_parameter.col(0) = SignalModel()->lastRow();
    
    setOptParamater(m_K11);
    QVector<qreal * > line1, line2;
    for(int i = 0; i < m_pure_signals_parameter.size(); ++i)
    {
        line1 << &m_pure_signals_parameter(i, 0);
        line2 << &m_complex_signal_parameter(i,0);
    }
    m_lim_para = QVector<QVector<qreal * > >()  << line1 << line2;
    
    AbstractTitrationModel::CalculateSignal();
}

QVector<qreal> ItoI_Model::OptimizeParameters_Private(OptimizationType type)
{    
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
        setOptParamater(m_complex_constants);

    if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
         
        if((type & OptimizationType::UnconstrainedShifts) == OptimizationType::UnconstrainedShifts)
        {
            addOptParameterList_fromConstant(0);
            if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
                addOptParameterList_fromPure(0);
        }
    }
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


qreal ItoI_Model::HostConcentration(qreal host_0, qreal guest_0, const QList< qreal > &constants)
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

void ItoI_Model::CalculateSignal(const QList<qreal > &constants)
{  
    m_corrupt = false;
    
    m_sum_absolute = 0;
    m_sum_squares = 0;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = HostConcentration(host_0, guest_0, constants);
        qreal complex = host_0 -host;
        Vector vector(4);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest_0 - complex;
        vector(3) = complex;
        SetConcentration(i, vector);
        for(int j = 0; j < SignalCount(); ++j)
        {
            qreal value = host/host_0*m_pure_signals_parameter(j, 0)+ complex/host_0*m_complex_signal_parameter(j,0);
            SetSignal(i, j, value);    
        }
    }
    emit Recalculated();
}


QSharedPointer<AbstractTitrationModel > ItoI_Model::Clone() const
{
    QSharedPointer<AbstractTitrationModel > model = QSharedPointer<ItoI_Model>(new ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportJSON(ExportJSON());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
    
}

qreal ItoI_Model::BC50()
{
    return 1/qPow(10,Constants()[0]); 
}

#include "1_1_Model.moc"
