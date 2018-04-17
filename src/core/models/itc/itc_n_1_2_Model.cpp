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

void itc_n_ItoII_Model::InitialGuess()
{
    m_global_parameter = QList<qreal>() << 6 << 6;

    m_local_parameter->data(0, 0) = -4000;
    m_local_parameter->data(1, 0) = 1;
    m_local_parameter->data(2, 0) = -4000;
    m_local_parameter->data(3, 0) = 1;
    m_local_parameter->data(4, 0) = -400;
    m_local_parameter->data(5, 0) = 1;

    AbstractModel::Calculate();
}

QVector<qreal> itc_n_ItoII_Model::OptimizeParameters_Private(OptimizationType type)
{
    if ((OptimizationType::GlobalParameter & type) == OptimizationType::GlobalParameter) {
        addGlobalParameter(0);
        addGlobalParameter(1);
    }
    if ((OptimizationType::LocalParameter & type) == OptimizationType::LocalParameter) {
        addLocalParameter(0);
        addLocalParameter(1);
        addLocalParameter(2);
        addLocalParameter(3);
        QString binding = getOption(Binding);
        QString dilution = getOption(Dilution);
        if (dilution == "auto") {
            addLocalParameter(4);
            addLocalParameter(5);
        }
        /*if (binding == "pytc" || binding == "multiple") {
            addLocalParameter(3);
        }*/
    }
    QVector<qreal> parameter;
    for (int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void itc_n_ItoII_Model::CalculateVariables()
{
    m_sum_absolute = 0;
    m_sum_squares = 0;

    QString binding = getOption(Binding);
    QString dil = getOption(Dilution);

    qreal dH1 = m_local_parameter->data(0, 0);
    qreal n1 = m_local_parameter->data(1, 0);
    qreal dH2 = m_local_parameter->data(2, 0);
    qreal n2 = m_local_parameter->data(3, 0);
    qreal dil_heat = m_local_parameter->data(4, 0);
    qreal dil_inter = m_local_parameter->data(5, 0);
    qreal V = m_V;
    qreal K1 = qPow(10, GlobalParameter(0));
    qreal K2 = qPow(10, GlobalParameter(1));

    qreal phi1_prev = 0, phi2_prev = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);

        qreal guest_0 = InitialGuestConcentration(i);
        qreal dilution = 0;
        qreal v = IndependentModel()->data(0, i);
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

        qreal p = 1 / K1 + 1 / K2 + (n1 + n2) * host_0 - guest_0;
        qreal q = (n1 / K2 + n2 / K1) * host_0 - (1 / K1 + 1 / K2) * guest_0 + 1 / K1 / K2;
        qreal r = -guest_0 / K1 / K2;

        qreal guest = MinCubicRoot(1, p, q, r);

        qreal phi1 = K1 * guest / (1 + K1 * guest);
        qreal phi2 = K2 * guest / (1 + K2 * guest);

        qreal value = V * ((phi1 - phi1_prev * (1 - v / V)) * dH1 + (phi2 - phi2_prev * (1 - v / V)) * dH2) * host_0;

        SetValue(i, 0, value + dilution);
        phi1_prev = phi1;
        phi2_prev = phi2;
    }
}

QSharedPointer<AbstractModel> itc_n_ItoII_Model::Clone()
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_n_ItoII_Model>(new itc_n_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return model;
}

#include "itc_n_1_2_Model.moc"
