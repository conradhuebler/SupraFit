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



#include "src/core/AbstractModel.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"

#include <QtMath>

#include <QtCore/QCollator>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QDebug>
#include <QtCore/QDateTime>
#include <cmath>
#include <cfloat>
#include <iostream>

#include "first_order_model.h"

Kinetic_First_Order_Model::Kinetic_First_Order_Model(const DataClass *data) : AbstractModel(data)
{
    setName(tr("First Order Kinetics"));
    m_local_parameter = new DataTable(1, SeriesCount(), this);
    InitialGuess();
}

Kinetic_First_Order_Model::Kinetic_First_Order_Model(const AbstractModel* model) : AbstractModel(model)
{
    setName(tr("First Order Kinetics"));
    m_local_parameter = new DataTable(1, SeriesCount(), this);
    InitialGuess();
}


Kinetic_First_Order_Model::~Kinetic_First_Order_Model() 
{
    
}

void Kinetic_First_Order_Model::InitialGuess()
{
    m_k = 5;
    m_global_parameter = QList<qreal>() << m_k;
    setOptParamater(m_global_parameter);
    
    AbstractModel::Calculate();
}

QVector<qreal> Kinetic_First_Order_Model::OptimizeParameters_Private(OptimizationType type)
{    
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
        setOptParamater(m_global_parameter);

    if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
         
        if((type & OptimizationType::UnconstrainedShifts) == OptimizationType::UnconstrainedShifts)
        { 
            if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
                addLocalParameter(0);
        }
    }
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void Kinetic_First_Order_Model::CalculateVariables(const QList<qreal > &constants)
{  
    m_corrupt = false;
    
    m_sum_absolute = 0;
    m_sum_squares = 0;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal k = GlobalParameter(0);
        qreal t = IndependentModel()->data(0,i);
        for(int j = 0; j < SeriesCount(); ++j)
        {
            qreal A_0 = LocalParameter(0, j);
            qreal value =A_0*qExp(-k*t);
            SetValue(i, j, value);    
        }
    }
    emit Recalculated();
}


QSharedPointer<AbstractModel > Kinetic_First_Order_Model::Clone() const
{
    QSharedPointer<Kinetic_First_Order_Model > model = QSharedPointer<Kinetic_First_Order_Model>(new Kinetic_First_Order_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "first_order_model.moc"
