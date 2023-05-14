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

#include "tian.h"

TIANModel::TIANModel(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "t", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "c(A)", Qt::DisplayRole);
}

TIANModel::TIANModel(AbstractModel* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "t", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "c(A)", Qt::DisplayRole);
}

TIANModel::~TIANModel()
{
}

void TIANModel::DeclareOptions()
{
}

void TIANModel::DeclareSystemParameter()
{
    addSystemParameter(Temperature, "Temperature", "Temperature in K", SystemParameter::Scalar);
    setSystemParameterValue(Temperature, 298);

    addSystemParameter(C0, "C0", "Constant calibration coefficient", SystemParameter::Scalar);
    setSystemParameterValue(C0, 1);

    addSystemParameter(C1, "C1", "Linear calibration coefficient", SystemParameter::Scalar);
    setSystemParameterValue(C1, 1);

    addSystemParameter(C2, "C2", "Quadratic calibration coefficient", SystemParameter::Scalar);
    setSystemParameterValue(C2, 1);

    addSystemParameter(C3, "C3", "Cubic calibration coefficient", SystemParameter::Scalar);
    setSystemParameterValue(C3, 0);

    addSystemParameter(C4, "C4", "Quartic calibration coefficient", SystemParameter::Scalar);
    setSystemParameterValue(C4, 0);
}

void TIANModel::InitialGuess_Private()
{
    (*GlobalTable())[0] = 0.002;
    (*GlobalTable())[1] = DependentModel()->data(DataBegin());

    //    QSharedPointer<AbstractModel> test = Clone();
    //    (*GlobalTable())[0] = NewtonRoot(test, 0, 0, 0.1, 1e-5);

    Calculate();
}

void TIANModel::OptimizeParameters_Private()
{
    addGlobalParameter(0);
    addGlobalParameter(1);
}

void TIANModel::CalculateVariables()
{
    UpdateParameter();

    qreal A = GlobalParameter(0);
    qreal k = GlobalParameter(1) / 1e5;
    double T = getSystemParameter(Temperature).Double();
    double tau = getSystemParameter(C0).Double()
        + T * getSystemParameter(C1).Double()
        + T * T * getSystemParameter(C2).Double()
        + T * T * T * getSystemParameter(C3).Double()
        + T * T * T * T * getSystemParameter(C4).Double();

    for (int i = DataBegin(); i < DataEnd(); ++i) {
        qreal t = IndependentModel()->data(i);
        qreal value = (A) * (exp(-(t)*k) - exp(-t / tau));
        SetValue(i, AppliedSeries(), value);
    }
}

QSharedPointer<AbstractModel> TIANModel::Clone(bool statistics)
{
    QSharedPointer<TIANModel> model = QSharedPointer<TIANModel>(new TIANModel(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->UpdateParameter();
    return model;
}

void TIANModel::UpdateParameter()
{
}

#include "tian.moc"
