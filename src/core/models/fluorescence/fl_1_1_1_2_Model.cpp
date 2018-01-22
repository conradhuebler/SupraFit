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

#include "fl_1_1_1_2_Model.h"

fl_ItoI_ItoII_Model::fl_ItoI_ItoII_Model(DataClass* data) : AbstractTitrationModel(data)
{
    setName(tr("fl_1:1/1:2-Model"));
    m_local_parameter = new DataTable(4, SeriesCount(), this);
    InitialGuess();
    DeclareOptions();
    AbstractTitrationModel::Calculate();
}

fl_ItoI_ItoII_Model::~fl_ItoI_ItoII_Model()
{
    
}

void fl_ItoI_ItoII_Model::DeclareOptions()
{
//     QStringList method = QStringList() << "NMR" << "UV/VIS";
//     addOption("Method", method);
//     QStringList cooperativity = QStringList() << "full" << "noncooperative" << "additive" << "statistical";
//     addOption("Cooperativity", cooperativity);
    
     QStringList method = QStringList() << "Host" << "no Host";
     addOption("Host", method);
     
}

void fl_ItoI_ItoII_Model::EvaluateOptions()
{
    /*
    QString cooperativitiy = getOption("Cooperativity");
    
    auto global_coop = [this]()
    {
        this->m_global_parameter[1] = log10(double(0.25)*qPow(10,this->m_global_parameter[0]));
    };
    
    auto local_coop = [this]()
    {
        for(int i = 0; i < this->SeriesCount(); ++i)
            this->m_local_parameter->data(2,i) = 2*(this->m_local_parameter->data(1,i)-this->m_local_parameter->data(0,i))+this->m_local_parameter->data(0,i);
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
    }*/
    
    QString host = getOption("Host");
    if(host != "Host")
    {
         for(int i = 0; i < SeriesCount(); ++i)
         {
            this->m_local_parameter->data(0,i) = 0;
            this->m_local_parameter->data(1,i) = 0;
         }
    }
    
}

void fl_ItoI_ItoII_Model::InitialGuess()
{   
    m_global_parameter = QList<qreal>() << 4 << 2;
    setOptParamater(m_global_parameter);

    qreal factor = 1; ///InitialHostConcentration(0);
    

    m_local_parameter->setColumn(DependentModel()->firstRow()*factor, 0);
    m_local_parameter->setColumn(DependentModel()->firstRow()*factor, 1);
    m_local_parameter->setColumn(DependentModel()->lastRow()*factor, 2);
    m_local_parameter->setColumn(DependentModel()->lastRow()*factor, 3);

    QVector<qreal * > line1, line2;
    for(int i = 0; i < SeriesCount(); ++i)
    {
        line1 << &m_local_parameter->data(0, i); 
        line1 << &m_local_parameter->data(1, i); 
        line2 << &m_local_parameter->data(2, i); 
        line2 << &m_local_parameter->data(3, i);
    }
    m_lim_para = QVector<QVector<qreal * > >() << line1 << line2;
    AbstractTitrationModel::Calculate();
}

QVector<qreal> fl_ItoI_ItoII_Model::OptimizeParameters_Private(OptimizationType type)
{
    QString cooperativity = getOption("Cooperativity");
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
    {
        addGlobalParameter(m_global_parameter);
        if(cooperativity == "statistical" || cooperativity == "noncooperative")
            m_opt_para.removeLast();
    }

    if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
         if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
             addLocalParameter(0);
         addLocalParameter(1);
         if(!(cooperativity == "additive" || cooperativity == "statistical"))
             addLocalParameter(2);
    } 
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


qreal fl_ItoI_ItoII_Model::HostConcentration(qreal host_0, qreal guest_0, const QList<qreal > &constants)
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

qreal fl_ItoI_ItoII_Model::GuestConcentration(qreal host_0, qreal guest_0, const QList< qreal > &constants)
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

void fl_ItoI_ItoII_Model::CalculateVariables()
{
    m_corrupt = false;
    QString method = getOption("Method");
    m_sum_absolute = 0;
    m_sum_squares = 0;
    qreal K12= qPow(10, GlobalParameter().last());
    qreal K11 = qPow(10, GlobalParameter().first());

    qreal host_zero, guest_zero;
    QVector<qreal > F0(SeriesCount());

    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);

        qreal host = HostConcentration(host_0, guest_0, GlobalParameter());
        qreal guest = GuestConcentration(host_0, guest_0, GlobalParameter());
        qreal complex_11 = K11*host*guest;
        qreal complex_12 = K11*K12*host*guest*guest;

        if(i == 0)
        {
            host_zero = host;
            guest_zero = guest;
        }

        Vector vector(5);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        vector(3) = complex_11;
        vector(4) = complex_12;
        SetConcentration(i, vector);

        qreal value = 0;
        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(i == 0)
            {
                F0[j] = host_0*m_local_parameter->data(0, j);
                value = F0[j];
            }else
                value = (host*m_local_parameter->data(1, j) + complex_11*m_local_parameter->data(2, j) + complex_12*m_local_parameter->data(3, j));
            

            SetValue(i, j, value);
        }
    }
    emit Recalculated();
}

QSharedPointer<AbstractModel > fl_ItoI_ItoII_Model::Clone()
{
    QSharedPointer<fl_ItoI_ItoII_Model > model = QSharedPointer<fl_ItoI_ItoII_Model>(new fl_ItoI_ItoII_Model(this), &QObject::deleteLater);    
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->ImportModel(ExportModel());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

qreal fl_ItoI_ItoII_Model::Y(qreal x, const QVector<qreal> &parameter)
{
    if(2 != parameter.size())
        return 0;
    qreal b11 = parameter[0];
    qreal b12 = parameter[1];
    qreal B = -b11/2/b12 + sqrt((qPow(b11,2))/(4*qPow(b12,2))+((x/(1-x))/b12));
    qreal A = 1/(b11+2*b12*B);
    return 1./(A + b11*A*B+b12*A*qPow(B,2));
}


qreal fl_ItoI_ItoII_Model::BC50() const
{
    qreal b11 = qPow(10,GlobalParameter(0));
    qreal b12 = qPow(10,GlobalParameter(0)+GlobalParameter(1));
    QVector<qreal> parameter;
    parameter << b11 << b12;
    std::function<qreal(qreal, const QVector<qreal> &)> function = Y;
    qreal integ = ToolSet::SimpsonIntegrate(0, 1, function, parameter);
    return double(1)/double(2)/integ;
}

#include "fl_1_1_1_2_Model.moc"
