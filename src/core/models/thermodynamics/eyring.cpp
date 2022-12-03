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

#include "eyring.h"

EyringFit::EyringFit(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "T", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "r", Qt::DisplayRole);
}

EyringFit::EyringFit(AbstractModel* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "T", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "r", Qt::DisplayRole);
}

EyringFit::~EyringFit()
{
}

void EyringFit::InitialGuess_Private()
{
    QVector<qreal> x, y;

    for (int i = 1; i < DataPoints(); ++i) {
        x << 1 / IndependentModel()->data(i);
        y << 1 / DependentModel()->data(i);
    }

    PeakPick::LinearRegression regress = LeastSquares(x, y);
    m_vmax = 1 / regress.n;
    m_Km = regress.m * m_vmax;
    (*GlobalTable())[0] = m_vmax;
    (*GlobalTable())[1] = m_Km;
    Calculate();
}

void EyringFit::CalculateVariables()
{
    for (int i = DataBegin(); i < DataEnd(); ++i) {
        // for (int i = 0; i < DataPoints(); ++i) {
        qreal vmax = GlobalParameter(0);
        qreal Km = GlobalParameter(1);
        qreal S_0 = IndependentModel()->data(i);
        // for (int j = 0; j < SeriesCount(); ++j) {
        qreal value = vmax * S_0 / (Km + S_0);
        SetValue(i, AppliedSeries(), value);
        //}
    }
}

QSharedPointer<AbstractModel> EyringFit::Clone(bool statistics)
{
    QSharedPointer<EyringFit> model = QSharedPointer<EyringFit>(new EyringFit(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "eyring.moc"
