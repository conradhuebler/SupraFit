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

#include "itc_1_1_Model.h"

itc_ItoI_Model::itc_ItoI_Model(DataClass* data)
    : AbstractItcModel(data)
{

    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    m_random_local[0][0] = QPair<qreal, qreal>(0, -100000);
}

itc_ItoI_Model::itc_ItoI_Model(AbstractItcModel* model)
    : AbstractItcModel(model)
{

    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    m_random_local[0][0] = QPair<qreal, qreal>(0, -100000);
}

itc_ItoI_Model::~itc_ItoI_Model()
{
}

void itc_ItoI_Model::InitialGuess_Private()
{
    LocalTable()->data(0, 0) = GuessdH();
    LocalTable()->data(0, 1) = -1000;
    LocalTable()->data(0, 2) = 1;
    LocalTable()->data(0, 3) = GuessFx();

    (*GlobalTable())[0] = GuessK();

    AbstractModel::Calculate();
}

void itc_ItoI_Model::OptimizeParameters_Private()
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

void itc_ItoI_Model::CalculateVariables()
{
    qDebug() << "from within" << OptimizeParameters();
    QString more_info = QString("Inject\t" + qAB + "\t" + qsolv + "\t" + q + "\n");
    QString dil = getOption(Dilution);

    qreal dH = LocalTable()->data(0, 0);
    qreal dil_heat = LocalTable()->data(0, 1);
    qreal dil_inter = LocalTable()->data(0, 2);
    qreal fx = LocalTable()->data(0, 3);
    qreal V = m_V;

    qreal complex_prev = 0;
    bool reservior = m_reservior;

    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);

        host_0 *= fx;
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
        V += IndependentModel()->data(i) * !reservior;
        qreal dv = (1 - v / V);

        qreal q_ab = V * (complex - complex_prev * dv) * dH;
        // std::cout << i<< " " << q_ab << std::endl;
        qreal value = q_ab;
        more_info += Print::printDouble(PrintOutIndependent(i)) + "\t" + Print::printDouble(q_ab) + "\t" + Print::printDouble(dilution) + "\t" + Print::printDouble(value) + "\n";

        bool usage = SetValue(i, 0, value + dilution);
        if (!m_fast && usage) {
            SetConcentration(i, vector);
            QStringList header = QStringList() << qAB << qsolv << q;
            vector = Vector(3);
            vector(0) = q_ab;
            vector(1) = dilution;
            vector(2) = value + dilution;
            addPoints("Heat Chart I", PrintOutIndependent(i), vector, header);
        }
        complex_prev = complex;
    }
    m_more_info = more_info;
    qDebug() << SSE();
}

QSharedPointer<AbstractModel> itc_ItoI_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_ItoI_Model>(new itc_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return std::move(model);
}

QString itc_ItoI_Model::AdditionalOutput() const
{
    QString result = tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += "<h4>without statistical data:</h4>";

    auto conf2therm = [&result, this](const QJsonObject& object = QJsonObject()) {
        result += "<p>Reaction: A + B &#8652; AB</p>";
        result += Statistic::MonteCarlo2Thermo(0, getT(), object);
    };

    conf2therm();
    /*
    if (!m_fast_confidence.isEmpty()) {
        result += "<h4>Statistics from Fast Confidence Calculation:</h4>";
        conf2therm(m_fast_confidence);
    }

    for (int i = 0; i < getMCStatisticResult(); ++i) {
        if (static_cast<SupraFit::Statistic>(AccessCI(getStatistic(SupraFit::Statistic::MonteCarlo, i)["controller"].toObject(),"Method").toInt()) == SupraFit::Statistic::MonteCarlo) {
            result += tr("<h4>Monte Carlo Simulation %1:</h4>").arg(i);
            conf2therm(getStatistic(SupraFit::Statistic::MonteCarlo, i));
        }
    }*/
    /*
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

QString itc_ItoI_Model::ModelInfo() const
{
    QString result = AbstractItcModel::ModelInfo();
    result += BC50::ItoI::Format_BC50(GlobalParameter(0));

    /*
    result += tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += "<h4>without statistical data:</h4>";
    result += "<p>Reaction: A + B &#8652; AB</p>";
    result += Statistic::MonteCarlo2Thermo(0, getT(), QJsonObject());*/

    return result;
}

QString itc_ItoI_Model::ParameterComment(int parameter) const
{
    Q_UNUSED(parameter)
    return QString("Reaction: A + B &#8652; AB");
}

QString itc_ItoI_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractItcModel::AnalyseMonteCarlo(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::MonteCarlo2BC50_1(GlobalParameter(0), object);
    return bc + result;
}

QString itc_ItoI_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractItcModel::AnalyseGridSearch(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::GridSearch2BC50_1(GlobalParameter(0), object);
    return bc + result;
}

#include "itc_1_1_Model.moc"
