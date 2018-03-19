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

#include "monomolecularmodel.h"

MonoMolecularModel::MonoMolecularModel(DataClass *data) : AbstractModel(data)
{
    connect(this, &DataClass::SystemParameterChanged, this, &MonoMolecularModel::UpdateParameter);
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

MonoMolecularModel::~MonoMolecularModel()
{
    
}

void MonoMolecularModel::DeclareOptions()
{
    QStringList order = QStringList() << "First" << "Second";
    addOption(Order, "Order", order);
}

void MonoMolecularModel::DeclareSystemParameter()
{
    addSystemParameter(Concentration, "Initial Concentration", "Initial concentration of component A", SystemParameter::Scalar);
}


void MonoMolecularModel::InitialGuess()
{
    m_global_parameter[0] = 1;
    AbstractModel::Calculate();
}

QVector<qreal> MonoMolecularModel::OptimizeParameters_Private(OptimizationType type)
{    
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
        addGlobalParameter(0);

    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void MonoMolecularModel::CalculateVariables()
{      
    m_sum_absolute = 0;
    m_sum_squares = 0;
    qreal C0 = m_C0;

    QString order = getOption(Order);

    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal k = GlobalParameter(0);
        qreal t = IndependentModel()->data(0,i);
        for(int j = 0; j < SeriesCount(); ++j)
        {
            qreal value;

            if(order == "First")
                value = C0 * qExp(-k*t);
            else if(order == "Second")
                value = 1/(2*k*t + (1/C0) );
            else
                value = 0;
            SetValue(i, j, value);
        }
    }
}


QSharedPointer<AbstractModel > MonoMolecularModel::Clone()
{
    QSharedPointer<MonoMolecularModel > model = QSharedPointer<MonoMolecularModel>(new MonoMolecularModel(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

void MonoMolecularModel::UpdateParameter()
{
    m_C0 = getSystemParameter(Concentration).Double();
}

#include "monomolecularmodel.moc"
