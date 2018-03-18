/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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


#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/models.h"
#include "src/core/minimizer.h"

#include <QtMath>
#include <QDebug>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "1_1_Model.h"

ItoI_Model::ItoI_Model(DataClass *data) : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
}

ItoI_Model::~ItoI_Model() 
{
    
}

void ItoI_Model::InitialGuess()
{
    m_global_parameter[0] = Guess_1_1();

    qreal factor = 1;
    if(getOption(Method) == "UV/VIS")
    {
        factor = 1/InitialHostConcentration(0);
    }

    m_local_parameter->setColumn(DependentModel()->firstRow()*factor, 0);
    m_local_parameter->setColumn(DependentModel()->lastRow()*factor, 1);
    
    Calculate();
}

void ItoI_Model::DeclareOptions()
{
    QStringList method = QStringList() << "NMR" << "UV/VIS";
    addOption(Method, "Method", method);
}

void ItoI_Model::EvaluateOptions()
{

}

QVector<qreal> ItoI_Model::OptimizeParameters_Private(OptimizationType type)
{    
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
        addGlobalParameter(0);

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


void ItoI_Model::CalculateVariables()
{  
    QString method = getOption(Method);
    m_sum_absolute = 0;
    m_sum_squares = 0;
    qreal value;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        qreal complex = host_0 -host;
        Vector vector(4);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest_0 - complex;
        vector(3) = complex;

        if(!m_fast)
            SetConcentration(i, vector);

        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(method == "NMR")
                value = host/host_0*m_local_parameter->data(0, j) + complex/host_0*m_local_parameter->data(1, j);
            else if(method == "UV/VIS")
                value = host*m_local_parameter->data(0, j) + complex*m_local_parameter->data(1, j);
            SetValue(i, j, value);    
        }
    }
}


QSharedPointer<AbstractModel > ItoI_Model::Clone()
{
    QSharedPointer<AbstractModel > model = QSharedPointer<ItoI_Model>(new ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

qreal ItoI_Model::BC50() const
{
    return 1/qPow(10,GlobalParameter()[0]); 
}

#include "1_1_Model.moc"
