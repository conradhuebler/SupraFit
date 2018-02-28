/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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



#include "src/core/AbstractItcModel.h"
#include "src/core/equil.h"
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

itc_ItoI_Model::itc_ItoI_Model(DataClass *data) : AbstractItcModel(data)
{
   PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

itc_ItoI_Model::~itc_ItoI_Model() 
{
    
}

void itc_ItoI_Model::InitialGuess()
{
    m_global_parameter = QList<qreal>() << 3;

    m_local_parameter->data(0, 0) = -4000;
    m_local_parameter->data(1, 0) = -1000;
    m_local_parameter->data(2, 0) = 1;
    m_local_parameter->data(3, 0) = 1;

    setOptParamater(m_global_parameter);
    
    AbstractModel::Calculate();
}

QVector<qreal> itc_ItoI_Model::OptimizeParameters_Private(OptimizationType type)
{    
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
        setOptParamater(m_global_parameter);

    addLocalParameter(0);
    QString binding = getOption("Binding");
    QString dilution = getOption("Dilution");
    if(dilution == "auto")
    {
        addLocalParameter(1);
        addLocalParameter(2);
    }
    if(binding == "pytc" || binding == "multiple")
    {
            addLocalParameter(3);
    }

    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


void itc_ItoI_Model::CalculateVariables()
{  
    /*
     * It seems, we have to recalculate the concentrations from within the child class
     * and not from the parent ??
     */
    Concentration();


    m_corrupt = false;
    m_sum_absolute = 0;
    m_sum_squares = 0;

    qreal V = m_data->getSystemParameter(CellVolume).Double();

    QString binding = getOption("Binding");
    QString dil = getOption("Dilution");

    qreal dH = m_local_parameter->data(0, 0);
    qreal dil_heat = m_local_parameter->data(1, 0);
    qreal dil_inter = m_local_parameter->data(2, 0);
    qreal fx = m_local_parameter->data(3, 0);
    qreal complex_prev = 0;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);

        if(binding == "pytc")
        {
            host_0 *= fx;
        }
        qreal guest_0 = InitialGuestConcentration(i);
        qreal dilution = 0;
        if(dil == "auto")
        {
            dilution= (guest_0*dil_heat+dil_inter);
        }
        qreal host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        qreal complex = (host_0 - host);
        Vector vector(4);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest_0 - complex;
        vector(3) = complex;

        SetConcentration(i, vector);
        qreal value = V*(complex-complex_prev)*dH;
        if(binding == "multiple")
            value *= fx;
        SetValue(i, 0, value+dilution);
        complex_prev = complex;
    }
}


QSharedPointer<AbstractModel > itc_ItoI_Model::Clone()
{
    QSharedPointer<AbstractModel > model = QSharedPointer<itc_ItoI_Model>(new itc_ItoI_Model(this), &QObject::deleteLater);
    model.data()->setData( m_data );
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

qreal itc_ItoI_Model::BC50() const
{
    return 1/qPow(10,GlobalParameter()[0]); 
}

#include "itc_1_1_Model.moc"
