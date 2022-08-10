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

#include "src/core/models/postprocess/statistic.h"

#include "src/core/models/titrations/AbstractItcModel.h"

#include "src/core/bc50.h"
#include "src/core/concentrationalpolynomial.h"
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

#include "itc_any_Model.h"

itc_any_Model::itc_any_Model(DataClass* data)
    : AbstractItcModel(data)
{
    m_pre_input = { MaxA_Json, MaxB_Json };
    m_complete = false;
}

itc_any_Model::itc_any_Model(AbstractItcModel* model)
    : AbstractItcModel(model)
{
    DefineModel();
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
}

itc_any_Model::~itc_any_Model()
{
    qDeleteAll(m_solvers);
}

bool itc_any_Model::DefineModel()
{
    QJsonObject object = m_defined_model.value("MaxA");
    m_maxA = object["value"].toInt();

    object = m_defined_model.value("MaxB");
    m_maxB = object["value"].toInt();

    m_global_names.clear();
    m_species_names.clear();
    for (int i = 1; i <= m_maxA; ++i) {
        QString name_i = QString::number(i);
        QString name_i_short = QString::number(i);
        if (i == 1)
            name_i_short.clear();
        name_i = ToolSet::UnicodeLowerInteger(name_i);
        name_i_short = ToolSet::UnicodeLowerInteger(name_i_short);

        for (int j = 1; j <= m_maxB; ++j) {
            QString name_j = QString::number(j);
            QString name_j_short = QString::number(j);

            if (j == 1)
                name_j_short.clear();
            name_j = ToolSet::UnicodeLowerInteger(name_j);
            name_j_short = ToolSet::UnicodeLowerInteger(name_j_short);

            m_global_names << QString("lg %1%2%3").arg(Unicode_beta).arg(name_i).arg(name_j);
            m_species_names << QString("A%1B%2").arg(name_i_short).arg(name_j_short);
            m_local_names << QString("%1 H (A%2B%3)").arg(Unicode_delta).arg(name_i_short).arg(name_j_short);

            QStringList host = QStringList() << "yes"
                                             << "no";
            addOption(Dilution + 1 + Index(i, j), QString("A%1B%2").arg(name_i_short).arg(name_j_short), host);
        }
    }
    m_local_names << "m (solv H)"
                  << "n (solv H)"
                  << "fx";
    m_global_parametersize = m_maxA * m_maxB;
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
    for (int i = 0; i < GlobalParameterSize(); ++i)
        setOption(Dilution + 1 + i, "yes");

    OptimizeParameters_Private();
    m_complete = true;

    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);

        ConcentrationalPolynomial* solver = new ConcentrationalPolynomial;
        m_solvers << solver;
        solver->setStoichiometry(m_maxA, m_maxB);
        solver->setInitialConcentrations(host_0, guest_0);
        solver->Guess();
        solver->setMaxIter(m_maxA * m_maxB * 200);
        solver->setConvergeThreshold(1e-19);
    }
    // std::cout << QDateTime::currentMSecsSinceEpoch() - t0 << std::endl;

    return true;
}

void itc_any_Model::InitialGuess_Private()
{
    double heat = GuessdH();
    LocalTable()->data(0, 0) = heat;
    for (int i = 1; i < m_global_parametersize; ++i)
        LocalTable()->data(0, i) = heat + heat / 10;
    LocalTable()->data(0, m_global_parametersize) = -1000;
    LocalTable()->data(0, m_global_parametersize + 1) = 1;

    LocalTable()->data(0, m_global_parametersize + 2) = GuessFx();

    double K = GuessK();
    (*GlobalTable())[0] = K;

    for (int i = 1; i < GlobalParameterSize(); ++i)
        (*GlobalTable())[i] = K + K;

    AbstractModel::Calculate();
}

void itc_any_Model::OptimizeParameters_Private()
{
    /*
    for (int i = 0; i < GlobalParameterSize(); ++i) {

        addGlobalParameter(i);
        addLocalParameter(i);
    }
    */
    for (int a = 1; a <= m_maxA; ++a)
        for (int b = 1; b <= m_maxB; ++b) {
            if (getOption(Dilution + 1 + Index(a, b)) == "yes") {
                addGlobalParameter(Index(a, b));
                addLocalParameter(Index(a, b));
            }
        }
    QString dilution = getOption(Dilution);
    if (dilution == "auto") {
        addLocalParameter(m_global_parametersize);
        addLocalParameter(m_global_parametersize + 1);
    }
    addLocalParameter(m_global_parametersize + 2);
}

void itc_any_Model::CalculateVariables()
{
    std::vector<double> constants(GlobalParameterSize());
    Vector vector_prev(m_species_names.size() + 3);
    vector_prev(0) = 0;
    vector_prev(1) = 0;
    vector_prev(2) = 0;
    Vector heats(GlobalParameterSize());

    for (int a = 1; a <= m_maxA; ++a)
        for (int b = 1; b <= m_maxB; ++b) {
            if (getOption(Dilution + 1 + Index(a, b)) == "yes") {
                constants[Index(a, b)] = pow(10, GlobalParameter(Index(a, b)));
                heats(Index(a, b)) = LocalTable()->data(0, Index(a, b));
                //     addGlobalParameter(Index(a, b));
                //     addLocalParameter(Index(a, b));
            } else {
                constants[Index(a, b)] = 0;
                heats(Index(a, b)) = 0;
            }
            vector_prev(Index(a, b) + 3) = 0;
        }
    /*
        for (int i = 0; i < GlobalParameterSize(); ++i) {
            if (GlobalTable()->isChecked(0, i))
            {
                constants[i] = pow(10, GlobalParameter(i));
                heats(i) = LocalTable()->data(0, i);
            }
            else
            {
                constants[i] = 0;
                heats(i) = 0;
            }
            vector_prev(i + 3) = 0;
        }
    */
    QString more_info = QString("Inject\t" + qAB + "\t" + qsolv + "\t" + q + "\n");
    QString dil = getOption(Dilution);

    qreal dil_heat = LocalTable()->data(0, heats.size());
    qreal dil_inter = LocalTable()->data(0, heats.size() + 1);
    qreal fx = LocalTable()->data(0, heats.size() + 2);
    qreal V = m_V;
    bool reservior = m_reservior;
    double diff_h = 0, diff_g = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);

        host_0 *= fx;
        m_solvers[i]->setInitialConcentrations(host_0, InitialGuestConcentration(i));
        // m_solvers[i]->Guess();
        m_solvers[i]->setStabilityConstants(constants);

        std::vector<double> result;
        result = m_solvers[i]->solver();
        /*
        {
            qreal guest_0 = InitialGuestConcentration(i);
            qreal host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
            qreal complex = (host_0 - host);
            qreal guest = guest_0 - complex;
            diff_h += host - result[0];
            diff_g +=  guest - result[1];
           // std::cout << "diff host: " <<host - result[0] << "     diff guest: " << guest - result[1] << std::endl;
        }*/
        // std::cout << std::setprecision(20) << m_solvers[i]->LastIterations() << " " << m_solvers[i]->LastConvergency() << std::endl;
        //  timer += m_solvers[i]->Timer();
        double host = result[0];
        double guest = result[1];

        Vector vector(m_species_names.size() + 3);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;

        int index = 3;
        for (int a = 1; a <= m_maxA; ++a) {
            double powA = a * pow(host, a);
            for (int b = 1; b <= m_maxB; b++) {
                double beta = constants[Index(a, b)];
                const double c = (beta * pow(guest, b) * powA);
                vector(index++) = c;
            }
        }

        // std::cout << vector.transpose() << std::endl;

        qreal guest_0 = InitialGuestConcentration(i);
        qreal dilution = 0;
        qreal v = IndependentModel()->data(i);
        if (dil == "auto") {
            dilution = (guest_0 * dil_heat + dil_inter);
        }
        /*
        qreal host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        qreal complex = (host_0 - host);
        Vector vector(4);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest_0 - complex;
        vector(3) = complex;*/
        V += IndependentModel()->data(i) * !reservior;
        qreal dv = (1 - v / V);
        Vector qvector(GlobalParameterSize() + 2);
        double value = 0;
        //  std::cout << i << " ";
        for (int species = 3; species < vector.size(); ++species) {
            const double q = V * (vector(species) - vector_prev(species) * dv) * heats(species - 3);
            // std::cout <<i<< " " <<  q << std::endl;
            // std::cout << q << " ";
            value += q;
            qvector(species - 3) = q;
        }
        //  std::cout << " = " << value << std::endl;
        qvector(GlobalParameterSize()) = dilution;
        qvector(GlobalParameterSize() + 1) = value + dilution;
        // std::cout << qvector.transpose() << std::endl;

        // qreal q_ab = V * (complex - complex_prev * dv) * dH;
        // qreal value = q_ab;
        QString more = QString();
        more += Print::printDouble(PrintOutIndependent(i)) + "\t";

        for (double& d : qvector)
            more += Print::printDouble(d) + "\t";
        // more += Print::printDouble(dilution) + "\t" + Print::printDouble(value) + "\n";
        // qDebug() << more;
        more_info += more + "\n";
        bool usage = SetValue(i, 0, value + dilution);

        if (!m_fast && usage) {
            SetConcentration(i, vector);
            QStringList header;
            for (const QString& species : m_species_names)
                header << QString("q%1").arg(species);
            header << qsolv << q;
            addPoints("Heat Chart I", PrintOutIndependent(i), qvector, header);
        }
        vector_prev = vector;
    }
    m_more_info = more_info;
    // qDebug() << SSE() << diff_h << diff_g;
}

QSharedPointer<AbstractModel> itc_any_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_any_Model>(new itc_any_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return std::move(model);
}

QString itc_any_Model::AdditionalOutput() const
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

QString itc_any_Model::ModelInfo() const
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

QString itc_any_Model::ParameterComment(int parameter) const
{
    Q_UNUSED(parameter)
    return QString("Reaction: A + B &#8652; AB");
}

QString itc_any_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractItcModel::AnalyseMonteCarlo(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::MonteCarlo2BC50_1(GlobalParameter(0), object);
    return bc + result;
}

QString itc_any_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractItcModel::AnalyseGridSearch(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::GridSearch2BC50_1(GlobalParameter(0), object);
    return bc + result;
}

#include "itc_any_Model.moc"
