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
#include "fl_2_1_1_1_Model.h"

fl_IItoI_ItoI_Model::fl_IItoI_ItoI_Model(DataClass* data) : AbstractTitrationModel(data)
{
    m_local_parameter = new DataTable(4, SeriesCount(), this); 
    m_global_parameter << 5 << 5;
    DeclareOptions();
//     InitialGuess();   
//     AbstractTitrationModel::Calculate();
}
    
fl_IItoI_ItoI_Model::~fl_IItoI_ItoI_Model()
{
    
}

void fl_IItoI_ItoI_Model::DeclareOptions()
{
     QStringList cooperativity = QStringList() << "full" << "noncooperative" << "additive" << "statistical";
     addOption("Cooperativity", cooperativity);
    
     // QStringList host = QStringList() << "Host" << "no Host";
     // addOption("Host", host);
     // setOption("Host", "Host");
     
}

void fl_IItoI_ItoI_Model::EvaluateOptions()
{
    
     QString cooperativitiy = getOption("Cooperativity");

    auto global_coop = [this](){
        this->m_global_parameter[0] = log10(double(0.25)*qPow(10,this->m_global_parameter[1]));
    };
    
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
    
    QString host = getOption("Host");
    /*qDebug() << host;
    if(host != "Host")
    {
         for(int i = 0; i < SeriesCount(); ++i)
         {
             qDebug() << "bang";
            this->m_local_parameter->data(0,i) = 0;
            this->m_local_parameter->data(1,i) = 0;
         }
    }*/
    
}

void fl_IItoI_ItoI_Model::InitialGuess()
{
    setOptParamater(m_global_parameter);

    qreal factor = InitialHostConcentration(0);
    

    m_local_parameter->setColumn(DependentModel()->firstRow()/factor/1e3, 0);
    m_local_parameter->setColumn(DependentModel()->firstRow()/factor/1e3, 1);
    m_local_parameter->setColumn(DependentModel()->lastRow()/factor/1e4, 2);
    m_local_parameter->setColumn(DependentModel()->lastRow()/factor/1e4, 3);
    
    QVector<qreal * > line1, line2;
    for(int i = 0; i < SeriesCount(); ++i)
    {
        line1 << &m_local_parameter->data(0, i);
        line1 << &m_local_parameter->data(1, i);
        line2 << &m_local_parameter->data(2, i);
        line1 << &m_local_parameter->data(3, i);
    }

    Calculate();
}

qreal fl_IItoI_ItoI_Model::HostConcentration(qreal host_0, qreal guest_0, const QList<qreal > &constants)
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

void fl_IItoI_ItoI_Model::CalculateVariables()
{
    m_corrupt = false;
    m_sum_absolute = 0;
    m_sum_squares = 0;
    
    qreal K21 = qPow(10, GlobalParameter().first());
    qreal K11 = qPow(10, GlobalParameter().last());

    QVector<qreal > F0(SeriesCount());

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
        qreal value = 0;
        for(int j = 0; j < SeriesCount(); ++j)
        {
           if(i == 0)
            {
                F0[j] = host_0*m_local_parameter->data(0, j);
                value = F0[j];
            }else
                value = (host*m_local_parameter->data(1, j) + 2*complex_21*m_local_parameter->data(2, j) + complex_11*m_local_parameter->data(3, j)); 
            
            SetValue(i, j, value*1e3);
        }
        
    }
}

QVector<qreal> fl_IItoI_ItoI_Model::OptimizeParameters_Private(OptimizationType type)
{    
    QString cooperativity = getOption("Cooperativity");
   
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
    {
        addGlobalParameter(m_global_parameter);
        if(cooperativity == "statistical" || cooperativity == "noncooperative")
            m_opt_para.removeFirst();
    }
    
    if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
        if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
            addLocalParameter(0);
        if(cooperativity != "additive" && cooperativity != "statistical")
            addLocalParameter(1);
        addLocalParameter(2);
    } 
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


QSharedPointer<AbstractModel > fl_IItoI_ItoI_Model::Clone()
{
    QSharedPointer<fl_IItoI_ItoI_Model > model = QSharedPointer<fl_IItoI_ItoI_Model>(new fl_IItoI_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;    
}

qreal fl_IItoI_ItoI_Model::BC50() const
{
    qreal b11 = qPow(10,GlobalParameter()[1]);
    qreal b21 = qPow(10,(GlobalParameter()[0]+GlobalParameter()[1]));
    qreal bc50 = -b11/b21/double(2) + sqrt(qPow(b11/double(2)/b21,2)+1/b21);
    return bc50;
}


#include "fl_2_1_1_1_Model.moc"
