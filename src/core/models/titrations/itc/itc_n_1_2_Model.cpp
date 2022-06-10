/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/models/postprocess/statistic.h"

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

#include "itc_n_1_2_Model.h"

itc_n_ItoII_Model::itc_n_ItoII_Model(DataClass* data)
    : AbstractItcModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

itc_n_ItoII_Model::itc_n_ItoII_Model(AbstractItcModel* model)
    : AbstractItcModel(model)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

itc_n_ItoII_Model::~itc_n_ItoII_Model()
{
}

void itc_n_ItoII_Model::InitialGuess_Private()
{
    (*GlobalTable())[0] = 6;
    (*GlobalTable())[1] = 6;

    LocalTable()->data(0, 0) = -4000;
    LocalTable()->data(0, 1) = 1;
    LocalTable()->data(0, 2) = -4000;
    LocalTable()->data(0, 3) = 1;
    LocalTable()->data(0, 4) = -400;
    LocalTable()->data(0, 5) = 1;

    /*
    LocalTable()->data(0, 0) = GuessdH();
    LocalTable()->data(1, 0) = GuessFx();
    LocalTable()->data(2, 0) = GuessdH();
    LocalTable()->data(3, 0) = 2*GuessFx();
    LocalTable()->data(4, 0) = -400;
    LocalTable()->data(5, 0) = 1;
    */

    AbstractModel::Calculate();
}

void itc_n_ItoII_Model::OptimizeParameters_Private()
{
    addGlobalParameter(0);
    addGlobalParameter(1);

    addLocalParameter(0);
    addLocalParameter(1);
    addLocalParameter(2);
    addLocalParameter(3);
    QString dilution = getOption(Dilution);
    if (dilution == "auto") {
        addLocalParameter(4);
        addLocalParameter(5);
    }
}

void itc_n_ItoII_Model::CalculateVariables()
{
    QString dil = getOption(Dilution);

    qreal dH1 = LocalTable()->data(0, 0);
    qreal n1 = LocalTable()->data(0, 1);
    qreal dH2 = LocalTable()->data(0, 2);
    qreal n2 = LocalTable()->data(0, 3);
    qreal dil_heat = LocalTable()->data(0, 4);
    qreal dil_inter = LocalTable()->data(0, 5);
    qreal V = m_V / 1e6;
    qreal K1 = qPow(10, GlobalParameter(0));
    qreal K2 = qPow(10, GlobalParameter(1));

    bool reservior = m_reservior;
    qreal oldq = 0;
    qreal phi1_prev = 0, phi2_prev = 0;
    for (int i = 0; i < DataPoints(); ++i) {

        qreal v = IndependentModel()->data(i) / 1e6;

        V += v * !reservior;
        qreal dv = (1 - v / V);

        qreal host_0 = InitialHostConcentration(i);

        qreal guest_0 = InitialGuestConcentration(i);
        qreal dilution = 0;

        if (dil == "auto") {
            dilution = (guest_0 * dil_heat + dil_inter);
        }

        /*  if (!m_fast)
            SetConcentration(i, vector);
*/
        oldq *= dv;
        /*
        qreal p = 1 / K1 + 1 / K2 + (n1 + n2) * host_0 - guest_0;
        qreal q = (n1 / K2 + n2 / K1) * host_0 - (1 / K1 + 1 / K2) * guest_0 + 1 / K1 / K2;
        qreal r = -guest_0 / K1 / K2;
        */
        qreal a = K1 * K2;
        qreal b = K1 + K2 + a * (host_0 * (n1 + n2) - guest_0);
        qreal c = 1 + host_0 * (n1 * K1 + n2 * K2) - guest_0 * (K1 + K2);
        qreal d = -guest_0;
        //qreal guest = MinCubicRoot(1, p, q, r);
        qreal guest = MinCubicRoot(a, b, c, d);

        qreal phi1 = n1 * K1 * guest / (1 + K1 * guest);
        qreal phi2 = n2 * K2 * guest / (1 + K2 * guest);

        // qreal value = V * ((phi1 - phi1_prev * dv) * dH1 + (phi2 - phi2_prev * dv) * dH2) * host_0;
        qreal value = V * host_0 * (n1 * dH1 * K1 * guest / (1 + K1 * guest) + n2 * dH2 * K2 * guest / (1 + K2 * guest)) * 1e6;

        SetValue(i, 0, value + dilution - oldq);
        phi1_prev = phi1;
        phi2_prev = phi2;
        oldq = value;
    }
}

QSharedPointer<AbstractModel> itc_n_ItoII_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_n_ItoII_Model>(new itc_n_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return std::move(model);
}

#include "itc_n_1_2_Model.moc"
