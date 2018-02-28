/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"

#include <QtMath>
#include <QDebug>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "fl_1_1_Model.h"

fl_ItoI_Model::fl_ItoI_Model(DataClass *data) : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}



fl_ItoI_Model::~fl_ItoI_Model() 
{
    
}

void fl_ItoI_Model::InitialGuess()
{
    m_global_parameter[0] = Guess_1_1();

    qreal factor = InitialHostConcentration(0);
    

    m_local_parameter->setColumn(DependentModel()->firstRow()/factor/1e3, 0);
    m_local_parameter->setColumn(DependentModel()->lastRow()/factor/1e3, 1);
    m_local_parameter->setColumn(DependentModel()->lastRow()/factor/1e3, 2);

    Calculate();
}

void fl_ItoI_Model::DeclareOptions()
{
     //QStringList method = QStringList() << "Host" << "no Host";
     //addOption("Host", method);
}

void fl_ItoI_Model::EvaluateOptions()
{
    /*
    QString host = getOption("Host");
    if(host != "Host")
    {
         for(int i = 0; i < SeriesCount(); ++i)
         {
            this->m_local_parameter->data(0,i) = 0;
            this->m_local_parameter->data(1,i) = 0;
         }
    }*/
}

QVector<qreal> fl_ItoI_Model::OptimizeParameters_Private(OptimizationType type)
{    
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
        setOptParamater(m_global_parameter);

    if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
        if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
            addLocalParameter(0);
        addLocalParameter(1);
    }
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void fl_ItoI_Model::CalculateVariables()
{  
    m_corrupt = false;
    m_sum_absolute = 0;
    m_sum_squares = 0;
    qreal value;
    QVector<qreal > F0(SeriesCount());
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        qreal complex = host_0 - host;
        Vector vector(4);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest_0 - complex;
        vector(3) = complex;
        SetConcentration(i, vector);

        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(i == 0)
            {
                F0[j] = host_0*m_local_parameter->data(0, j);
                value = F0[j];
            }else
                value = (host*m_local_parameter->data(1, j) + complex*m_local_parameter->data(2, j)); 
            
            SetValue(i, j, value*1e3);
        }
    }
}


QSharedPointer<AbstractModel > fl_ItoI_Model::Clone()
{
    QSharedPointer<AbstractModel > model = QSharedPointer<fl_ItoI_Model>(new fl_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

qreal fl_ItoI_Model::BC50() const
{
    return 1/qPow(10,GlobalParameter()[0]); 
}

#include "fl_1_1_Model.moc"
