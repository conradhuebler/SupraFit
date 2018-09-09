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
#include "src/core/thermo.h"
#include "src/core/toolset.h"

#include <QtMath>

#include <QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <cfloat>
#include <cmath>
#include <iostream>

#include "itc_1_1_Model.h"

itc_ItoI_Model::itc_ItoI_Model(DataClass* data)
    : AbstractItcModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

itc_ItoI_Model::itc_ItoI_Model(AbstractItcModel* model)
    : AbstractItcModel(model)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

itc_ItoI_Model::~itc_ItoI_Model()
{
}

void itc_ItoI_Model::InitialGuess_Private()
{
    LocalTable()->data(0, 0) = GuessdH();
    LocalTable()->data(1, 0) = -1000;
    LocalTable()->data(2, 0) = 1;
    LocalTable()->data(3, 0) = GuessFx();

    (*GlobalTable())[0] = GuessK();

    AbstractModel::Calculate();
}

QVector<qreal> itc_ItoI_Model::OptimizeParameters_Private()
{
    addGlobalParameter(0);
    addLocalParameter(0);

    QString dilution = getOption(Dilution);
    if (dilution == "auto") {
        addLocalParameter(1);
        addLocalParameter(2);
    }

    addLocalParameter(3);

    QVector<qreal> parameter;
    for (int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void itc_ItoI_Model::CalculateVariables()
{
    QString more_info = QString("Inject\t" + qAB + "\t" + qsolv + "\t" + q + "\n");
    QString dil = getOption(Dilution);

    qreal dH = LocalTable()->data(0, 0);
    qreal dil_heat = LocalTable()->data(1, 0);
    qreal dil_inter = LocalTable()->data(2, 0);
    qreal fx = LocalTable()->data(3, 0);
    qreal V = m_V;

    qreal complex_prev = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);

        host_0 *= fx;
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

        qreal q_ab = V * (complex - complex_prev * (1 - v / V)) * dH;
        qreal value = q_ab;
        more_info += Print::printDouble(PrintOutIndependent(i)) + "\t" + Print::printDouble(q_ab) + "\t" + Print::printDouble(dilution) + "\t" + Print::printDouble(value) + "\n";

        SetValue(i, 0, value + dilution);
        complex_prev = complex;
    }
    m_more_info = more_info;
}

QSharedPointer<AbstractModel> itc_ItoI_Model::Clone()
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_ItoI_Model>(new itc_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return model;
}

QString itc_ItoI_Model::AdditionalOutput() const
{
    QString result = tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += "<h4>without statistical data:</h4>";

    auto conf2therm = [&result, this](const QJsonObject& object = QJsonObject()) {
        result += "<p>Reaction: A + B &#8652; AB</p>";
        result += Thermo::Statistic2Thermo(GlobalParameter(0), LocalTable()->data(0, 0), getT(), object);
    };

    conf2therm();

    if (!m_fast_confidence.isEmpty()) {
        result += "<h4>Statistics from Fast Confidence Calculation:</h4>";
        conf2therm(m_fast_confidence);
    }

    for (int i = 0; i < getMCStatisticResult(); ++i) {
        if (static_cast<SupraFit::Statistic>(getStatistic(SupraFit::Statistic::MonteCarlo, i)["controller"].toObject()["method"].toInt()) == SupraFit::Statistic::MonteCarlo) {
            result += tr("<h4>Monte Carlo Simulation %1:</h4>").arg(i);
            conf2therm(getStatistic(SupraFit::Statistic::MonteCarlo, i));
        }
    }

    for (int i = 0; i < getMoCoStatisticResult(); ++i) {
        result += tr("<h4>Model Comparison %1:</h4>").arg(i);
        conf2therm(getStatistic(SupraFit::Statistic::ModelComparison, i));
    }

    for (int i = 0; i < getWGStatisticResult(); ++i) {
        result += tr("<h4>Weakend Grid Search %1:</h4>").arg(i);
        conf2therm(getStatistic(SupraFit::Statistic::WeakenedGridSearch, i));
    }

    return result;
}

#include "itc_1_1_Model.moc"
