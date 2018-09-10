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

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include <cfloat>
#include <cmath>
#include <iostream>

#include <libpeakpick/mathhelper.h>
#include <libpeakpick/peakpick.h>

#include "blank.h"

Blank::Blank(DataClass* data)
    : AbstractItcModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    RemoveOption(Dilution);
    RemoveSystemParameter(CellConcentration);
}

Blank::Blank(AbstractItcModel* model)
    : AbstractItcModel(model)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    RemoveOption(Dilution);
    RemoveSystemParameter(CellConcentration);
}

Blank::~Blank()
{
}

void Blank::InitialGuess_Private()
{
    QVector<qreal> x, y;

    for (int i = 1; i < DataPoints(); ++i) {
        x << PrintOutIndependent(i);
        y << DependentModel()->data(0, i);
    }
    QMap<qreal, PeakPick::MultiRegression> result = LeastSquares(x, y, 1);
    PeakPick::MultiRegression regression = result.first();

    LocalTable()->data(0, 0) = regression.regressions[0].m;
    LocalTable()->data(1, 0) = regression.regressions[0].n;

    AbstractModel::Calculate();
}

QVector<qreal> Blank::OptimizeParameters_Private()
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

    qreal dil_heat = LocalTable()->data(0, 0);
    qreal dil_inter = LocalTable()->data(1, 0);

    for (int i = 0; i < DataPoints(); ++i) {

        qreal guest_0 = InitialGuestConcentration(i);
        SetValue(i, 0, guest_0 * dil_heat + dil_inter);
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

qreal Blank::PrintOutIndependent(int i) const
{
    QString plotmode = getPlotMode();

    if (m_c0) {
        if (plotmode == "[G<sub>0</sub>]/[H<sub>0</sub>]" || plotmode == "[G<sub>0</sub>]")
            return InitialGuestConcentration(i);
        else //if (plotmode == "Number")
            return i;
    } else
        return i;
}

#include "blank.moc"
