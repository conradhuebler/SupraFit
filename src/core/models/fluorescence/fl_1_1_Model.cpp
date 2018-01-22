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

#include "fl_1_1_Model.h"

fl_ItoI_Model::fl_ItoI_Model(DataClass *data) : AbstractTitrationModel(data)
{
    setName(tr("fl_1:1-Model"));
    m_local_parameter = new DataTable(3, SeriesCount(), this);
//     m_complex_signal_parameter = Eigen::MatrixXd::Zero(SeriesCount(), 1);
    DeclareOptions();
    InitialGuess();
}

fl_ItoI_Model::fl_ItoI_Model(AbstractTitrationModel* model) : AbstractTitrationModel(model)
{
    setName(tr("fl_1:1-Model"));
    m_local_parameter = new DataTable(3, SeriesCount(), this);
    DeclareOptions();
    InitialGuess();
}


fl_ItoI_Model::~fl_ItoI_Model() 
{
    
}

void fl_ItoI_Model::InitialGuess()
{
    m_K11 = 4;
    m_global_parameter = QList<qreal>() << m_K11;

    qreal factor = 1; ///InitialHostConcentration(0);
    

    m_local_parameter->setColumn(DependentModel()->firstRow()*factor, 0);
    m_local_parameter->setColumn(DependentModel()->lastRow()*factor, 1);
    m_local_parameter->setColumn(DependentModel()->lastRow()*factor, 2);
    QVector<qreal * > line1, line2;

    for(int i = 0; i < SeriesCount(); ++i)
    {
        line1 << &m_local_parameter->data(0, i);
        line2 << &m_local_parameter->data(1, i);
        line2 << &m_local_parameter->data(2, i);
    }

    setOptParamater(m_global_parameter);
    m_lim_para = QVector<QVector<qreal * > >()  << line1 << line2;
    
    AbstractTitrationModel::Calculate();
}

void fl_ItoI_Model::DeclareOptions()
{
     QStringList method = QStringList() << "Host" << "no Host";
     addOption("Host", method);
}

void fl_ItoI_Model::EvaluateOptions()
{
    /*
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


qreal fl_ItoI_Model::HostConcentration(qreal host_0, qreal guest_0, const QList< qreal > &constants)
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
        qreal host = HostConcentration(host_0, guest_0, GlobalParameter());
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
            
            SetValue(i, j, value*1e5);    
        }
    }
    emit Recalculated();
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
