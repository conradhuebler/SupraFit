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
#include "src/core/toolset.h"

#include <QtMath>

#include <QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <cfloat>
#include <cmath>
#include <iostream>

#include "itc_2_1_Model.h"

itc_IItoI_Model::itc_IItoI_Model(DataClass* data)
    : AbstractItcModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

itc_IItoI_Model::itc_IItoI_Model(AbstractItcModel* model)
    : AbstractItcModel(model)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

itc_IItoI_Model::~itc_IItoI_Model()
{
}

void itc_IItoI_Model::DeclareOptions()
{
    QStringList cooperativity = QStringList() << "full"
                                              << "noncooperative";
    //   << "additive"
    //   << "statistical";
    addOption(Cooperativity, "Cooperativity", cooperativity);

    AbstractItcModel::DeclareOptions();
}

void itc_IItoI_Model::EvaluateOptions()
{
    QString cooperativitiy = getOption(Cooperativity);
    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * K11 = 4*K21 | K21 = 0.25 K11
     * valid for statistical and noncooperative systems
     */
    auto global_coop = [this]() {
        (*this->GlobalTable())[0] = log10(double(0.25) * qPow(10, (*this->GlobalTable())[1]));
    };
    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * Y(A2B) = 2Y(AB)
     * valid for statistical and additive systems
     * We first have to subtract the Host_0 Shift and afterwards calculate the new Signal
     */
    auto local_coop = [this]() {
        for (int i = 0; i < this->SeriesCount(); ++i)
            this->LocalTable()->data(1, i) = 2 * (this->LocalTable()->data(2, i) - this->LocalTable()->data(0, i)) + this->LocalTable()->data(0, i);
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

void itc_IItoI_Model::InitialGuess_Private()
{
    (*GlobalTable())[1] = 6;

    LocalTable()->data(0, 0) = GuessdH() / 10.0;
    LocalTable()->data(1, 0) = GuessdH();
    LocalTable()->data(2, 0) = -1000;
    LocalTable()->data(3, 0) = 1;
    LocalTable()->data(4, 0) = GuessFx();

    qreal K = GuessK(1);

    (*GlobalTable())[0] = K / 2.0;
    (*GlobalTable())[1] = K;

    AbstractModel::Calculate();
}

QVector<qreal> itc_IItoI_Model::OptimizeParameters_Private()
{
    QString coop21 = getOption(Cooperativity);

    if (coop21 == "additive" || coop21 == "full")
        addGlobalParameter(0);

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

void itc_IItoI_Model::CalculateVariables()
{

    QString more_info = QString("Inject\tq(A2B)\tq(AB)\tDilution\tSum\n");

    QString dil = getOption(Dilution);

    qreal dH1 = LocalTable()->data(1, 0);
    qreal dH2 = LocalTable()->data(0, 0) + dH1;

    qreal dil_heat = LocalTable()->data(2, 0);
    qreal dil_inter = LocalTable()->data(3, 0);
    qreal fx = LocalTable()->data(4, 0);

    qreal V = m_V;

    qreal K21 = qPow(10, GlobalParameter(0));
    qreal K11 = qPow(10, GlobalParameter(1));

    qreal complex_21_prev = 0, complex_11_prev = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);

        host_0 *= fx;

        qreal dilution = 0;
        if (dil == "auto") {
            dilution = (guest_0 * dil_heat + dil_inter);
        }

        qreal v = IndependentModel()->data(0, i);

        qreal host = IItoI_ItoI::HostConcentration(host_0, guest_0, QList<qreal>() << K21 << K11);
        qreal guest = guest_0 / (K11 * host + K11 * K21 * host * host + 1);
        qreal complex_11 = K11 * host * guest;
        qreal complex_21 = K11 * K21 * host * host * guest;

        Vector vector(5);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        vector(3) = complex_21;
        vector(4) = complex_11;
        if (!m_fast)
            SetConcentration(i, vector);
        qreal q_a2b = (complex_21 - complex_21_prev * (1 - v / V)) * dH2 * V;
        qreal q_ab = (complex_11 - complex_11_prev * (1 - v / V)) * dH1 * V;
        qreal value = q_a2b + q_ab;
        more_info += Print::printDouble(PrintOutIndependent(i, 0)) + "\t" + Print::printDouble(q_a2b) + "\t" + Print::printDouble(q_ab) + "\t" + Print::printDouble(dilution) + "\t" + Print::printDouble(value) + "\n";

        SetValue(i, 0, value + dilution);
        complex_11_prev = complex_11;
        complex_21_prev = complex_21;
    }
    m_more_info = more_info;
}

QSharedPointer<AbstractModel> itc_IItoI_Model::Clone()
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_IItoI_Model>(new itc_IItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return model;
}

#include "itc_2_1_Model.moc"
