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
#include "src/core/models.h"
#include "src/core/toolset.h"


#include "src/core/libmath.h"
#include <QtMath>
#include <QtCore/QJsonObject>
#include <QDebug>
#include <QtCore/QDateTime>
#include <cmath>
#include <cfloat>
#include <iostream>
#include "2_1_1_1_Model.h"

IItoI_ItoI_Model::IItoI_ItoI_Model(DataClass* data) : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}
    
IItoI_ItoI_Model::~IItoI_ItoI_Model()
{
    
}

void IItoI_ItoI_Model::DeclareOptions()
{
    QStringList method = QStringList() << "NMR" << "UV/VIS";
    addOption("Method", method);
    QStringList cooperativity = QStringList() << "full" << "noncooperative" << "additive" << "statistical";
    addOption("Cooperativity", cooperativity);
}

void IItoI_ItoI_Model::EvaluateOptions()
{
    QString cooperativitiy = getOption("Cooperativity");
    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * K11 = 4*K21 | K21 = 0.25 K11
     * valid for statistical and noncooperative systems
     */
    auto global_coop = [this](){
        this->m_global_parameter[0] = log10(double(0.25)*qPow(10,this->m_global_parameter[1]));
    };
    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * Y(A2B) = 2Y(AB)
     * valid for statistical and additive systems
     * We first have to subtract the Host_0 Shift and afterwards calculate the new Signal
     */
    auto local_coop = [this]()
    {
        for(int i = 0; i < this->SeriesCount(); ++i)
            this->m_local_parameter->data(1,i) = 2*(this->m_local_parameter->data(2,i)-this->m_local_parameter->data(0,i))+this->m_local_parameter->data(0,i);
    };
    
    if(cooperativitiy == "noncooperative")
    {
        global_coop();
    }else if(cooperativitiy == "additive")
    {
        local_coop();
    }else if(cooperativitiy == "statistical")
    {
        local_coop();
        global_coop();
    }
    
}

void IItoI_ItoI_Model::InitialGuess()
{
    m_global_parameter[1] = Guess_1_1();
    m_global_parameter[0] = m_global_parameter[1]/2;

    qreal factor = 1;
    if(getOption("Method") == "UV/VIS")
    {
        factor = 1/InitialHostConcentration(0);
    }

    m_local_parameter->setColumn(DependentModel()->firstRow()*factor, 0);
    m_local_parameter->setColumn(DependentModel()->firstRow()*factor, 1);
    m_local_parameter->setColumn(DependentModel()->lastRow()*factor, 2);
    Calculate();
}

void IItoI_ItoI_Model::CalculateVariables()
{
    QString method = getOption("Method");
    m_sum_absolute = 0;
    m_sum_squares = 0;
    
    qreal K21 = qPow(10, GlobalParameter().first());
    qreal K11 = qPow(10, GlobalParameter().last());
    
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = IItoI_ItoI::HostConcentration(host_0, guest_0, QList<qreal>() << K21 << K11);
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
        qreal value = 0;
        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(method == "NMR")
                value = host/host_0*m_local_parameter->data(0, j) + 2*complex_21/host_0*m_local_parameter->data(1, j) + complex_11/host_0*m_local_parameter->data(2, j);
            else if(method == "UV/VIS")
                value = host*m_local_parameter->data(0, j) + 2*complex_21*m_local_parameter->data(1, j) + complex_11*m_local_parameter->data(2, j);
            SetValue(i, j, value);
        }
        
    }
}

QVector<qreal> IItoI_ItoI_Model::OptimizeParameters_Private(OptimizationType type)
{    
    QString coop21 = getOption("Cooperativity");
   
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
    {
        addGlobalParameter(1);
        if(coop21 == "additive" || coop21 == "full")
            addGlobalParameter(0);
    }
    
    if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
        if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
            addLocalParameter(0);
        if(coop21 != "additive" && coop21 != "statistical")
            addLocalParameter(1);
        addLocalParameter(2);
    } 
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


QSharedPointer<AbstractModel > IItoI_ItoI_Model::Clone()
{
    QSharedPointer<IItoI_ItoI_Model > model = QSharedPointer<IItoI_ItoI_Model>(new IItoI_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;    
}

qreal IItoI_ItoI_Model::BC50() const
{
    qreal b11 = qPow(10,GlobalParameter()[1]);
    qreal b21 = qPow(10,(GlobalParameter(0)+GlobalParameter(1)));
    qreal bc50 = -b11/b21/double(2) + sqrt(qPow(b11/double(2)/b21,2)+1/b21);
    return bc50;
}


#include "2_1_1_1_Model.moc"
