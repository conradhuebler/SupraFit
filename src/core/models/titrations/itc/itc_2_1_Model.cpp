/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

    m_random_local[0][0] = QPair<qreal, qreal>(0, -100000);
    m_random_local[0][1] = m_random_local[0][0];
}

itc_IItoI_Model::itc_IItoI_Model(AbstractItcModel* model)
    : AbstractItcModel(model)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());

    m_random_local[0][0] = QPair<qreal, qreal>(0, -100000);
    m_random_local[0][1] = m_random_local[0][0];
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

void itc_IItoI_Model::OptimizeParameters_Private()
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
}

void itc_IItoI_Model::CalculateVariables()
{

    QString more_info = QString("Inject\t" + qA2B + "\t" + qAB + "\t" + qsolv + "\t" + q + "\n");
    QString more_info_2 = QString("\nInject\t" + qA2B_ + "\t" + qAB_ + "\t" + qsolv + "\t" + q + "\n");

    QString dil = getOption(Dilution);

    qreal dH1 = LocalTable()->data(1, 0);
    qreal dH2 = LocalTable()->data(0, 0) + dH1;
    qreal dH2_ = LocalTable()->data(0, 0);

    qreal dil_heat = LocalTable()->data(2, 0);
    qreal dil_inter = LocalTable()->data(3, 0);
    qreal fx = LocalTable()->data(4, 0);

    qreal V = m_V;

    qreal K21 = qPow(10, GlobalParameter(0));
    qreal K11 = qPow(10, GlobalParameter(1));

    bool reservior = m_reservior;

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
        V += IndependentModel()->data(0, i) * !reservior;
        qreal dv = (1 - v / V);

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

        qreal q_a2b = (complex_21 - complex_21_prev * dv) * dH2 * V;
        qreal q_ab = (complex_11 - complex_11_prev * dv) * dH1 * V;
        qreal q_ab_ = ((complex_21 - complex_21_prev * dv) + (complex_11 - complex_11_prev * dv)) * dH1 * V;
        qreal q_a2b_ = (complex_21 - complex_21_prev * dv) * dH2_ * V;
        qreal value = q_a2b + q_ab;

        more_info += Print::printDouble(PrintOutIndependent(i)) + "\t" + Print::printDouble(q_a2b) + "\t" + Print::printDouble(q_ab) + "\t" + Print::printDouble(dilution) + "\t" + Print::printDouble(value) + "\n";
        more_info_2 += Print::printDouble(PrintOutIndependent(i)) + "\t" + Print::printDouble(q_a2b_) + "\t" + Print::printDouble(q_ab_) + "\t" + Print::printDouble(dilution) + "\t" + Print::printDouble(value) + "\n";

        bool usage = SetValue(i, 0, value + dilution);

        if (!m_fast && usage) {
            SetConcentration(i, vector);
            QStringList header_1 = QStringList() << qA2B << qAB << qsolv << q;
            QStringList header_2 = QStringList() << qA2B_ << qAB_ << qsolv << q;

            vector = Vector(4);
            vector(0) = q_a2b;
            vector(1) = q_ab;
            vector(2) = dilution;
            vector(3) = value + dilution;
            addPoints("Heat Chart I", PrintOutIndependent(i), vector, header_1);
            vector(0) = q_a2b_;
            vector(1) = q_ab_;
            addPoints("Heat Chart II", PrintOutIndependent(i), vector, header_2);
        }
        complex_11_prev = complex_11;
        complex_21_prev = complex_21;
    }
    m_more_info = more_info + "\n" + more_info_2;
}

QSharedPointer<AbstractModel> itc_IItoI_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_IItoI_Model>(new itc_IItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return std::move(model);
}

QString itc_IItoI_Model::AdditionalOutput() const
{
    QString result = tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += "<h4>without statistical data:</h4>";
    /*
    auto conf2therm = [&result, this](const QJsonObject& object = QJsonObject()) {
        result += "<p>Reaction: A + B &#8652; AB</p>";
        result += Statistic::MonteCarlo2Thermo(i, getT(), object, true);
        result += "<p>Reaction: AB + A &#8652; A<sub>2</sub>B</p>";
        result += Statistic::MonteCarlo2Thermo(GlobalParameter(0), LocalTable()->data(0, 0), getT(), object);
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
    */
    return result;
}

QString itc_IItoI_Model::ParameterComment(int parameter) const
{
    if (parameter == 0)
        return QString("Reaction: AB + A &#8652; A<sub>2</sub>B");
    else
        return QString("Reaction: A + B &#8652; AB");
}

QString itc_IItoI_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{
    QString result = AbstractItcModel::AnalyseMonteCarlo(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::MonteCarlo2BC50_2_1(GlobalParameter(0), GlobalParameter(1), object);
    return bc + result;
}

QString itc_IItoI_Model::ModelInfo() const
{
    QString result = AbstractItcModel::ModelInfo();
    result += BC50::IItoI::Format_BC50(GlobalParameter(0), GlobalParameter(1));

    return result;
}

#include "itc_2_1_Model.moc"
