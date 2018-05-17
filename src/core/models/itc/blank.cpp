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

#include "src/core/AbstractItcModel.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"

#include <QtMath>

#include <QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <cfloat>
#include <cmath>
#include <iostream>

#include "blank.h"

Blank::Blank(DataClass* data)
    : AbstractItcModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    RemoveOption(Binding);
    RemoveOption(Dilution);
    RemoveSystemParameter(CellConcentration);
}

Blank::Blank(AbstractItcModel* model)
    : AbstractItcModel(model)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    RemoveOption(Binding);
    RemoveOption(Dilution);
    RemoveSystemParameter(CellConcentration);
}

Blank::~Blank()
{
}

void Blank::InitialGuess()
{

    m_local_parameter->data(0, 0) = 1;
    m_local_parameter->data(1, 0) = 1;

    AbstractModel::Calculate();
}

QVector<qreal> Blank::OptimizeParameters_Private(OptimizationType type)
{
    addLocalParameter(0);
    addLocalParameter(1);

    QVector<qreal> parameter;
    for (int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void Blank::CalculateVariables()
{
    m_sum_absolute = 0;
    m_sum_squares = 0;

    qreal dil_heat = m_local_parameter->data(0, 0);
    qreal dil_inter = m_local_parameter->data(1, 0);

    for (int i = 0; i < DataPoints(); ++i) {

        qreal guest_0 = InitialGuestConcentration(i);
        qreal dilution = 0;

        dilution = (guest_0 * dil_heat + dil_inter);

        SetValue(i, 0, dilution);
    }
}

QSharedPointer<AbstractModel> Blank::Clone()
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<Blank>(new Blank(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return model;
}

qreal Blank::PrintOutIndependent(int i, int format) const
{
    Q_UNUSED(format)
    qreal val = i;
    if (m_c0) {
        val = InitialGuestConcentration(i);
        if (std::isnan(val))
            val = i;
    }
    return val;
}

#include "blank.moc"
