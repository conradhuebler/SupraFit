/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/libmath.h"
#include "src/core/minimizer.h"

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include <cmath>
#include <iostream>

#include <libpeakpick/mathhelper.h>
#include <libpeakpick/peakpick.h>

#include "arrhenius.h"

ArrheniusFit::ArrheniusFit(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "T", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "r", Qt::DisplayRole);
}

ArrheniusFit::ArrheniusFit(AbstractModel* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "T", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "r", Qt::DisplayRole);
}

ArrheniusFit::~ArrheniusFit()
{
}

void ArrheniusFit::InitialGuess_Private()
{
    QVector<qreal> x, y;

    for (int i = 1; i < DataPoints(); ++i) {
        x << 1 / IndependentModel()->data(i);
        y << log(DependentModel()->data(i));
    }
    double A = 0.0, EA = 0.0;
    PeakPick::LinearRegression regress = LeastSquares(x, y);
    A = exp(regress.n);
    EA = -1 * regress.m * R;
    (*GlobalTable())[0] = A;
    (*GlobalTable())[1] = EA;
    Calculate();
}

void ArrheniusFit::CalculateVariables()
{
    for (int i = DataBegin(); i < DataEnd(); ++i) {
        // for (int i = 0; i < DataPoints(); ++i) {
        qreal A = GlobalParameter(0);
        qreal EA = GlobalParameter(1);
        qreal T = IndependentModel()->data(i);
        for (int j = 0; j < SeriesCount(); ++j) {
            qreal value = A * exp(-EA / R / T);
            SetValue(i, j, value);
        }
    }
}

QSharedPointer<AbstractModel> ArrheniusFit::Clone(bool statistics)
{
    QSharedPointer<ArrheniusFit> model = QSharedPointer<ArrheniusFit>(new ArrheniusFit(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "arrhenius.moc"
