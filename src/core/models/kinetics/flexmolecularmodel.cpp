/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/libmath.h"
#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "flexmolecularmodel.h"

FlexMolecularModel::FlexMolecularModel(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "t", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "c(A)", Qt::DisplayRole);
}

FlexMolecularModel::FlexMolecularModel(AbstractModel* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "t", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "c(A)", Qt::DisplayRole);
}

FlexMolecularModel::~FlexMolecularModel()
{
}

void FlexMolecularModel::DeclareOptions()
{
}

void FlexMolecularModel::DeclareSystemParameter()
{
}

void FlexMolecularModel::InitialGuess_Private()
{
    (*GlobalTable())[0] = 0.002;
    (*GlobalTable())[1] = 1.001;
    (*GlobalTable())[2] = DependentModel()->data(DataBegin());
    (*GlobalTable())[3] = DependentModel()->data(DataEnd());

    Calculate();
}

void FlexMolecularModel::OptimizeParameters_Private()
{
    addGlobalParameter(0);
    addGlobalParameter(1);
    addGlobalParameter(2);
    addGlobalParameter(3);
}

void FlexMolecularModel::CalculateVariables()
{
    UpdateParameter();
    qreal k = GlobalParameter(0);
    qreal n = GlobalParameter(1);
    qreal cA0 = GlobalParameter(2);
    qreal cAeq = GlobalParameter(3);

    for (int i = DataBegin(); i < DataEnd(); ++i) {
        qreal t = IndependentModel()->data(i);
        // for (int j = 0; j < SeriesCount(); ++j) {
        qreal inner = -k * t * (1 - n) + pow(cA0 - cAeq, 1 - n);
        qreal value = pow(inner, 1 / (1 - n)) + cAeq;
        // qreal value = (cA0 - cAeq) * pow(pow(cA0 - cAeq, (1 - n)) * k * t * (1 -n ) + 1, 1.0 / ( 1.0 - n)) + cAeq;
        SetValue(i, 0, value);
        // }
    }
}

QSharedPointer<AbstractModel> FlexMolecularModel::Clone(bool statistics)
{
    QSharedPointer<FlexMolecularModel> model = QSharedPointer<FlexMolecularModel>(new FlexMolecularModel(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->UpdateParameter();
    return model;
}

void FlexMolecularModel::UpdateParameter()
{
}

#include "flexmolecularmodel.moc"
