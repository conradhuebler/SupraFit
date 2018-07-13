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

#include "itc_1_2_Model.h"

itc_ItoII_Model::itc_ItoII_Model(DataClass* data)
    : AbstractItcModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

itc_ItoII_Model::itc_ItoII_Model(AbstractItcModel* model)
    : AbstractItcModel(model)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

itc_ItoII_Model::~itc_ItoII_Model()
{
}

void itc_ItoII_Model::DeclareOptions()
{
    QStringList cooperativity = QStringList() << "full"
                                              << "noncooperative";
    // << "additive"
    // << "statistical";
    addOption(Cooperativity, "Cooperativity", cooperativity);

    AbstractItcModel::DeclareOptions();
}

void itc_ItoII_Model::EvaluateOptions()
{
    QString cooperativitiy = getOption(Cooperativity);
    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * K11 = 4*K12 | K12 = 0.25 K11
     * valid for statistical and noncooperative systems
     */
    auto global_coop = [this]() {
        (*this->GlobalTable())[1] = log10(double(0.25) * qPow(10, (*this->GlobalTable())[0]));
    };

    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * Y(AB2) = 2Y(AB)
     * valid for statistical and additive systems
     * We first have to subtract the Host_0 Shift and afterwards calculate the new Signal
     */
    auto local_coop = [this]() {
        for (int i = 0; i < this->SeriesCount(); ++i)
            this->LocalTable()->data(2, i) = 2 * (this->LocalTable()->data(1, i) - this->LocalTable()->data(0, i)) + this->LocalTable()->data(0, i);
    };

    if (cooperativitiy == "noncooperative") {
        global_coop();
    } else if (cooperativitiy == "additive") {
        local_coop();
    } else if (cooperativitiy == "statistical") {
        local_coop();
        global_coop();
    }
    AbstractItcModel::EvaluateOptions();
}

void itc_ItoII_Model::InitialGuess_Private()
{
    LocalTable()->data(0, 0) = GuessdH();
    LocalTable()->data(1, 0) = GuessdH() / 10.0;
    LocalTable()->data(2, 0) = -1000;
    LocalTable()->data(3, 0) = 1;
    LocalTable()->data(4, 0) = GuessFx();

    qreal K = GuessK();

    (*GlobalTable())[0] = K;
    (*GlobalTable())[1] = K / 2.0;

    AbstractModel::Calculate();
}

QVector<qreal> itc_ItoII_Model::OptimizeParameters_Private()
{
    QString coop12 = getOption(Cooperativity);

    addGlobalParameter(0);

    if (coop12 == "additive" || coop12 == "full")
        addGlobalParameter(1);

        addLocalParameter(0);
        addLocalParameter(1);

        QString dilution = getOption(Dilution);

        if (dilution == "auto") {
            addLocalParameter(2);
            addLocalParameter(3);
        }

        addLocalParameter(4);

    QVector<qreal> parameter;
    for (int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void itc_ItoII_Model::CalculateVariables()
{

    QString dil = getOption(Dilution);

    qreal dil_heat = LocalTable()->data(2, 0);
    qreal dil_inter = LocalTable()->data(3, 0);
    qreal fx = LocalTable()->data(4, 0);
    qreal V = m_V;

    qreal K11 = qPow(10, GlobalParameter(0));
    qreal K12 = qPow(10, GlobalParameter(1));

    qreal dH1 = LocalTable()->data(0, 0);
    qreal dH2 = LocalTable()->data(1, 0) + dH1;

    qreal complex_11_prev = 0, complex_12_prev = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);

        qreal v = IndependentModel()->data(0, i);

        host_0 *= fx;

        qreal dilution = 0;
        if (dil == "auto") {
            dilution = (guest_0 * dil_heat + dil_inter);
        }

        qreal host = ItoI_ItoII::HostConcentration(host_0, guest_0, QList<qreal>() << K11 << K12);
        qreal guest = ItoI_ItoII::GuestConcentration(host_0, guest_0, QList<qreal>() << K11 << K12);
        qreal complex_11 = K11 * host * guest;
        qreal complex_12 = K11 * K12 * host * guest * guest;

        Vector vector(5);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        vector(3) = complex_11;
        vector(4) = complex_12;

        if (!m_fast)
            SetConcentration(i, vector);

        qreal value = V * ((complex_11 - complex_11_prev * (1 - v / V)) * dH1 + (complex_12 - complex_12_prev * (1 - v / V)) * (dH2));
        SetValue(i, 0, value + dilution);
        complex_11_prev = complex_11;
        complex_12_prev = complex_12;
    }
}

QSharedPointer<AbstractModel> itc_ItoII_Model::Clone()
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_ItoII_Model>(new itc_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return model;
}


#include "itc_1_2_Model.moc"
