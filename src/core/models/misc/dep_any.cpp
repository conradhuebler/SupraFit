/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/AbstractModel.h"

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include <cmath>
#include <iostream>

#include "dep_any.h"

Dep_Any::Dep_Any(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "x", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "y", Qt::DisplayRole);
}

Dep_Any::Dep_Any(AbstractModel* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "x", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "y", Qt::DisplayRole);
}

Dep_Any::~Dep_Any()
{
}

void Dep_Any::InitialGuess_Private()
{
    m_a = 2;
    m_b = 2;
    (*GlobalTable())[0] = m_a;
    (*GlobalTable())[1] = m_b;
    Calculate();
}

void Dep_Any::CalculateVariables()
{
    for (int i = DataBegin(); i < DataEnd(); ++i) {
        // for (int i = 0; i < DataPoints(); ++i) {
        qreal a = GlobalParameter(0);
        qreal b = GlobalParameter(1);
        qreal x = IndependentModel()->data(i);
        qreal value = log2(a) * log2(b) * x * x + log2(a) / x + log2(b) * x;
        SetValue(i, 0, value);
    }
}

QSharedPointer<AbstractModel> Dep_Any::Clone(bool statistics)
{
    QSharedPointer<Dep_Any> model = QSharedPointer<Dep_Any>(new Dep_Any(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "dep_any.moc"
