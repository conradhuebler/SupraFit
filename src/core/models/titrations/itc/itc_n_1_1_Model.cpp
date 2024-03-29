/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/titrations/AbstractItcModel.h"

#include "src/core/bc50.h"
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

#include "itc_n_1_1_Model.h"

itc_n_ItoI_Model::itc_n_ItoI_Model(DataClass* data)
    : AbstractItcModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

itc_n_ItoI_Model::itc_n_ItoI_Model(AbstractItcModel* model)
    : AbstractItcModel(model)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

itc_n_ItoI_Model::~itc_n_ItoI_Model()
{
}

void itc_n_ItoI_Model::InitialGuess_Private()
{
    LocalTable()->data(0, 0) = GuessdH();
    LocalTable()->data(0, 1) = -1000;
    LocalTable()->data(0, 2) = 1;
    LocalTable()->data(0, 3) = 1;

    (*GlobalTable())[1] = GuessK();

    AbstractModel::Calculate();
}

void itc_n_ItoI_Model::OptimizeParameters_Private()
{
    addGlobalParameter(0);

    addLocalParameter(0);

    QString dilution = getOption(Dilution);
    if (dilution == "auto") {
        addLocalParameter(1);
        addLocalParameter(2);
    }
    addLocalParameter(3);
}

void itc_n_ItoI_Model::CalculateVariables()
{
    QString dil = getOption(Dilution);

    qreal dH = LocalTable()->data(0, 0);
    qreal dil_heat = LocalTable()->data(0, 1);
    qreal dil_inter = LocalTable()->data(0, 2);
    qreal fx = LocalTable()->data(0, 3);
    qreal V = m_V;

    qreal phi_prev = 0;

    /* One note for ITC Models and the "faster" iteration of inlcuded points!
     * The results depend on the previously calculated concentrations of the complex, hence the loop MUST be complete */

    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);

        qreal guest_0 = InitialGuestConcentration(i);
        qreal dilution = 0;
        qreal v = IndependentModel()->data(i);
        if (dil == "auto") {
            dilution = (guest_0 * dil_heat + dil_inter);
        }
        qreal host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        qreal complex = (host_0 - host);
        Vector vector(4);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest_0 - complex;
        vector(3) = complex;

        if (!m_fast)
            SetConcentration(i, vector);
        qreal sum = 1 + guest_0 / (fx * host_0) + 1 / (fx * host_0 * qPow(10, GlobalParameter(0)));
        qreal phi = (sum - qSqrt((sum * sum) - 4 * guest_0 / (fx * host_0))) * host_0;

        qreal value = V * (phi - phi_prev * (1 - v / V)) * dH;

        SetValue(i, AppliedSeries(), value + dilution);
        phi_prev = phi;
    }
}

QSharedPointer<AbstractModel> itc_n_ItoI_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_n_ItoI_Model>(new itc_n_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return std::move(model);
}

#include "itc_n_1_1_Model.moc"
