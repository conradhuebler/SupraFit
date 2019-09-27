/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include <cmath>
#include <iostream>

#include "indep_quadrat.h"

Indep_Quadrat::Indep_Quadrat(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "x", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "y", Qt::DisplayRole);
}

Indep_Quadrat::Indep_Quadrat(AbstractModel* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "x", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "y", Qt::DisplayRole);
}

Indep_Quadrat::~Indep_Quadrat()
{
}

void Indep_Quadrat::DeclareOptions()
{
    QStringList order = QStringList() << "Linear"
                                      << "Quadrat";
    addOption(Order, "Order", order);
    setOption(Order, order[1]);
}

void Indep_Quadrat::InitialGuess_Private()
{
    m_a = 2;
    m_b = 2;
    (*GlobalTable())[0] = m_a;
    (*GlobalTable())[1] = m_b;
    Calculate();
}

void Indep_Quadrat::CalculateVariables()
{
    QString order = getOption(Order);
    qreal value = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        qreal a = GlobalParameter(0);
        qreal b = GlobalParameter(1);
        qreal x = IndependentModel()->data(0, i);
        if (order == "Linear")
            value = a * x * (i < DataPoints() / 2) + b * x * !(i < DataPoints() / 2);
        else
            value = a * x * x * (i < DataPoints() / 2) + b * x * x * !(i < DataPoints() / 2);

        SetValue(i, 0, value);
    }
}

QSharedPointer<AbstractModel> Indep_Quadrat::Clone(bool statistics)
{
    QSharedPointer<Indep_Quadrat> model = QSharedPointer<Indep_Quadrat>(new Indep_Quadrat(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "indep_quadrat.moc"
