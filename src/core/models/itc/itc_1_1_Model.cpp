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



#include "src/core/AbstractTitrationModel.h"
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

#include "itc_1_1_Model.h"

itc_ItoI_Model::itc_ItoI_Model(DataClass *data) : AbstractTitrationModel(data)
{
    setName(tr("itc_1:1-Model"));
    m_local_parameter = new DataTable(1, SeriesCount(), this);
    InitialGuess();
    DeclearSystemParameter();
}

itc_ItoI_Model::itc_ItoI_Model(AbstractTitrationModel* model) : AbstractTitrationModel(model)
{
    setName(tr("itc_1:1-Model"));
    m_local_parameter = new DataTable(1, SeriesCount(), this);
    InitialGuess();
    DeclearSystemParameter();
}


itc_ItoI_Model::~itc_ItoI_Model() 
{
    
}

void itc_ItoI_Model::DeclearSystemParameter()
{
    m_data->addSystemParameter("Cell Volume", "Volume of the cell", SystemParameter::Scalar);
    m_data->addSystemParameter("Inject Volume", "Inject Volume per step", SystemParameter::Scalar);
    m_data->LoadSystemParameter();
}


void itc_ItoI_Model::InitialGuess()
{
    m_K11 = 4;
    m_global_parameter = QList<qreal>() << m_K11;
    
    
    m_local_parameter->setColumn(DependentModel()->firstRow(), 0);
    
    QVector<qreal * > line1, line2;
    for(int i = 0; i < SeriesCount(); ++i)
        line1 << &m_local_parameter->data(0, i); 

    setOptParamater(m_global_parameter);
    m_lim_para = QVector<QVector<qreal * > >()  << line1 << line2;
    
    AbstractTitrationModel::Calculate();
}

QVector<qreal> itc_ItoI_Model::OptimizeParameters_Private(OptimizationType type)
{    
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
        setOptParamater(m_global_parameter);

    if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
         
        if((type & OptimizationType::UnconstrainedShifts) == OptimizationType::UnconstrainedShifts)
        {
            addLocalParameter(0);
        }
    }
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


qreal itc_ItoI_Model::HostConcentration(qreal host_0, qreal guest_0, const QList< qreal > &constants)
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

void itc_ItoI_Model::CalculateVariables()
{  
    m_corrupt = false;
    
    m_sum_absolute = 0;
    m_sum_squares = 0;
    qreal V = m_data->getSystemParameter("Cell Volume").Double();
    qreal inject = m_data->getSystemParameter("Inject Volume").Double();
    qreal heat  = 0;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal V_1 = V+((1+i)*inject);
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = HostConcentration(host_0, guest_0, GlobalParameter());
        qreal complex = (host_0 -host);
        Vector vector(4);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest_0 - complex;
        vector(3) = complex;
        SetConcentration(i, vector);
        qreal value = V_1*complex*m_local_parameter->data(0, 0);
        SetValue(i, 0, value-heat*(1-inject/V));    
        heat = value;
    }
    emit Recalculated();
}


QSharedPointer<AbstractModel > itc_ItoI_Model::Clone()
{
    QSharedPointer<AbstractModel > model = QSharedPointer<itc_ItoI_Model>(new itc_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
    
}

qreal itc_ItoI_Model::BC50()
{
    return 1/qPow(10,GlobalParameter()[0]); 
}

#include "itc_1_1_Model.moc"
