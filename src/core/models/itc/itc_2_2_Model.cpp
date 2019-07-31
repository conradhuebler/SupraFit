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
#include "src/core/models/postprocess/statistic.h"

#include "src/core/AbstractItcModel.h"
#include "src/core/bc50.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"
#include "src/core/toolset.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QThreadPool>

#include <cfloat>
#include <cmath>
#include <iostream>

#include "itc_2_2_Model.h"

itc_IItoII_Model::itc_IItoII_Model(DataClass* data)
    : AbstractItcModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());

    m_threadpool = new QThreadPool(this);
    for (int i = 0; i < DataPoints(); ++i)
        m_solvers << new IItoI_ItoI_ItoII_Solver();
}

itc_IItoII_Model::itc_IItoII_Model(AbstractItcModel* model)
    : AbstractItcModel(model)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    m_threadpool = new QThreadPool(this);
    for (int i = 0; i < DataPoints(); ++i)
        m_solvers << new IItoI_ItoI_ItoII_Solver();
}

itc_IItoII_Model::~itc_IItoII_Model()
{
    for (int i = 0; i < m_solvers.size(); ++i)
        if (m_solvers[i])
            delete m_solvers[i];
}

void itc_IItoII_Model::DeclareOptions()
{
    QStringList cooperativity = QStringList() << "full"
                                              << "noncooperative";
    //                                          << "additive"
    //                                          << "statistical";

    addOption(Cooperativity2_1, "Cooperativity 2:1", cooperativity);
    cooperativity = QStringList() << "full"
                                  << "noncooperative";
    //   << "additive"
    //   << "statistical";
    addOption(Cooperativity1_2, "Cooperativity 1:2", cooperativity);

    AbstractItcModel::DeclareOptions();
}

void itc_IItoII_Model::EvaluateOptions()
{
    QString coop21 = getOption(Cooperativity2_1);

    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * K11 = 4*K21 | K21 = 0.25 K11
     * valid for statistical and noncooperative systems
     */
    auto global_coop21 = [this]() {
        (*this->GlobalTable())[0] = log10(double(0.25) * qPow(10, (*this->GlobalTable())[1]));
    };

    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * Y(A2B) = 2Y(AB)
     * valid for statistical and additive systems
     * We first have to subtract the Host_0 Shift and afterwards calculate the new Signal
     */
    auto local_coop21 = [this]() {
        for (int i = 0; i < this->SeriesCount(); ++i)
            this->LocalTable()->data(1, i) = 2 * (this->LocalTable()->data(2, i) - this->LocalTable()->data(0, i)) + this->LocalTable()->data(0, i);
    };

    if (coop21 == "noncooperative") {
        global_coop21();
    } else if (coop21 == "additive") {
        local_coop21();
    } else if (coop21 == "statistical") {
        local_coop21();
        global_coop21();
    }

    QString coop12 = getOption(Cooperativity1_2);

    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * K11 = 4*K12 | K12 = 0.25 K11
     * valid for statistical and noncooperative systems
     */
    auto global_coop12 = [this]() {
        (*this->GlobalTable())[2] = log10(double(0.25) * qPow(10, (*this->GlobalTable())[1]));
    };
    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * Y(AB2) = 2Y(AB)
     * valid for statistical and additive systems
     * We first have to subtract the Host_0 Shift and afterwards calculate the new Signal
     */
    auto local_coop12 = [this]() {
        for (int i = 0; i < this->SeriesCount(); ++i)
            this->LocalTable()->data(3, i) = 2 * (this->LocalTable()->data(2, i) - this->LocalTable()->data(0, i)) + this->LocalTable()->data(0, i);
    };

    if (coop12 == "noncooperative") {
        global_coop12();
    } else if (coop12 == "additive") {
        local_coop12();
    } else if (coop12 == "statistical") {
        local_coop12();
        global_coop12();
    }
    AbstractItcModel::EvaluateOptions();
}

void itc_IItoII_Model::InitialGuess_Private()
{
    LocalTable()->data(0, 0) = GuessdH() / 10.0;
    LocalTable()->data(1, 0) = GuessdH();
    LocalTable()->data(2, 0) = GuessdH() / 10.0;
    LocalTable()->data(3, 0) = -1000;
    LocalTable()->data(4, 0) = 1;
    LocalTable()->data(5, 0) = GuessFx();

    qreal K = GuessK(1);

    (*GlobalTable())[0] = K / 2.0;
    (*GlobalTable())[1] = K;
    (*GlobalTable())[2] = K / 2.0;

    AbstractModel::Calculate();
}

void itc_IItoII_Model::OptimizeParameters_Private()
{
    QString coop21 = getOption(Cooperativity2_1);
    QString coop12 = getOption(Cooperativity1_2);

    if (coop21 == "additive" || coop21 == "full")
        addGlobalParameter(0);

    addGlobalParameter(1);

    if (coop12 == "additive" || coop12 == "full")
        addGlobalParameter(2);

    addLocalParameter(0);
    addLocalParameter(1);
    addLocalParameter(2);

    QString dilution = getOption(Dilution);

    if (dilution == "auto") {
        addLocalParameter(3);
        addLocalParameter(4);
    }

    addLocalParameter(5);
}

void itc_IItoII_Model::CalculateVariables()
{
    if (!m_threadpool)
        return;

    bool skip = m_opt_config["Skip_not_Converged_Concentrations"].toBool();

    QString more_info = QString("Inject\t" + qA2B + "\t" + qAB + "\t" + qAB2 + "\t" + qsolv + "\t" + q + "\n");
    QString more_info_2 = QString("\nInject\t" + qA2B_ + "\t" + qAB_ + "\t" + qAB2_ + "\t" + qsolv + "\t" + q + "\n");

    QString dil = getOption(Dilution);

    qreal dH11 = LocalTable()->data(1, 0);
    qreal dH21 = LocalTable()->data(0, 0) + dH11;
    qreal dH21_ = LocalTable()->data(0, 0);
    qreal dH12 = LocalTable()->data(2, 0) + dH11;
    qreal dH12_ = LocalTable()->data(2, 0);

    qreal dil_heat = LocalTable()->data(3, 0);
    qreal dil_inter = LocalTable()->data(4, 0);
    qreal fx = LocalTable()->data(5, 0);
    qreal V = m_V;

    qreal K21 = qPow(10, GlobalParameter(0));
    qreal K11 = qPow(10, GlobalParameter(1));
    qreal K12 = qPow(10, GlobalParameter(2));

    qreal complex_21_prev = 0, complex_11_prev = 0, complex_12_prev = 0;

    QList<qreal> constants_pow;
    constants_pow << K21 << K11 << K12;

    int maxthreads = qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        host_0 *= fx;

        qreal guest_0 = InitialGuestConcentration(i);
        m_solvers[i]->setInput(host_0, guest_0);
        m_solvers[i]->setConfig(m_opt_config);
        m_solvers[i]->setConstants(constants_pow);
        //  if(QThreadPool::globalInstance()->activeThreadCount())
        //      m_solvers[i]->run();
        // else
        m_threadpool->start(m_solvers[i]);
    }
    m_threadpool->waitForDone();

    for (int i = 0; i < DataPoints(); ++i) {
        if (!m_solvers[i]->Ok()) {
#ifdef _DEBUG
            qDebug() << "Numeric didn't work out well, mark model as corrupt! - Dont panic. Not everything is lost ...";
            qDebug() << m_solvers[i]->Ok() << InitialHostConcentration(i) << InitialGuestConcentration(i);
#endif
            m_corrupt = true;
            if (skip) {
#ifdef _DEBUG
                qDebug() << "Ok, I skip the current result ...";
#endif
                continue;
            }
        }

        qreal guest_0 = InitialGuestConcentration(i);
        qreal dilution = 0;
        if (dil == "auto") {
            dilution = (guest_0 * dil_heat + dil_inter);
        }

        QPair<double, double> concentration = m_solvers[i]->Concentrations();

        qreal host = concentration.first;
        qreal guest = concentration.second;

        qreal complex_11 = K11 * host * guest;
        qreal complex_21 = K11 * K21 * host * host * guest;
        qreal complex_12 = K11 * K12 * host * guest * guest;

        Vector vector(6);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        vector(3) = complex_21;
        vector(4) = complex_11;
        vector(5) = complex_12;

        qreal v = IndependentModel()->data(0, i);
        qreal dv = (1 - v / V);

        qreal q_a2b = (complex_21 - complex_21_prev * dv) * dH21 * V;
        qreal q_ab = (complex_11 - complex_11_prev * dv) * dH11 * V;
        qreal q_ab2 = (complex_12 - complex_12_prev * dv) * dH12 * V;

        qreal q_ab_ = ((complex_21 - complex_21_prev * dv) + (complex_11 - complex_11_prev * dv) + (complex_12 - complex_12_prev * dv)) * dH11 * V;
        qreal q_ab2_ = (complex_12 - complex_12_prev * dv) * dH12_ * V;
        qreal q_a2b_ = (complex_21 - complex_21_prev * dv) * dH21_ * V;

        //qreal value = V * ((complex_21 - complex_21_prev * (1 - v / V)) * dH21 + ((complex_11 - complex_11_prev * (1 - v / V)) * dH11) + ((complex_12 - complex_12_prev * (1 - v / V)) * dH12));
        qreal value = q_a2b + q_ab + q_ab2;
        more_info += Print::printDouble(PrintOutIndependent(i)) + "\t" + Print::printDouble(q_a2b) + "\t" + Print::printDouble(q_ab) + "\t" + Print::printDouble(q_ab2) + "\t" + Print::printDouble(dilution) + "\t" + Print::printDouble(value) + "\n";
        more_info_2 += Print::printDouble(PrintOutIndependent(i)) + "\t" + Print::printDouble(q_a2b_) + "\t" + Print::printDouble(q_ab_) + "\t" + Print::printDouble(q_ab2_) + "\t" + Print::printDouble(dilution) + "\t" + Print::printDouble(value) + "\n";

        bool usage = SetValue(i, 0, value + dilution);

        if (!m_fast && usage) {
            SetConcentration(i, vector);

            QStringList header_1 = QStringList() << qA2B << qAB << qAB2 << qsolv << q;
            QStringList header_2 = QStringList() << qA2B_ << qAB_ << qAB2_ << qsolv << q;

            vector = Vector(5);
            vector(0) = q_a2b;
            vector(1) = q_ab;
            vector(2) = q_ab2;
            vector(3) = dilution;
            vector(4) = value + dilution;
            addPoints("Heat Chart I", PrintOutIndependent(i), vector, header_1);
            vector(0) = q_a2b_;
            vector(1) = q_ab_;
            vector(2) = q_ab2_;
            addPoints("Heat Chart II", PrintOutIndependent(i), vector, header_2);
        }
        complex_21_prev = complex_21;
        complex_11_prev = complex_11;
        complex_12_prev = complex_12;
    }
    m_more_info = more_info + "\n" + more_info_2;
}

QSharedPointer<AbstractModel> itc_IItoII_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_IItoII_Model>(new itc_IItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return model;
}

QString itc_IItoII_Model::AdditionalOutput() const
{
    QString result = tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += "<h4>without statistical data:</h4>";
    /*
    auto conf2therm = [&result, this](const QJsonObject& object = QJsonObject()) {
        result += "<p>Reaction: A + B &#8652; AB</p>";
        result += Statistic::MonteCarlo2Thermo(GlobalParameter(1), LocalTable()->data(1, 0), getT(), object);
        result += "<p>Reaction: AB + A &#8652; A<sub>2</sub>B</p>";
        result += Statistic::MonteCarlo2Thermo(GlobalParameter(0), LocalTable()->data(0, 0), getT(), object);
        result += "<p>Reaction: AB + B &#8652; AB<sub>2</sub></p>";
        result += Statistic::MonteCarlo2Thermo(GlobalParameter(2), LocalTable()->data(2, 0), getT(), object);
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

QString itc_IItoII_Model::ParameterComment(int parameter) const
{
    if (parameter == 0)
        return QString("Reaction: AB + A &#8652; A<sub>2</sub>B");
    else if (parameter == 1)
        return QString("Reaction: A + B &#8652; AB");
    else
        return QString("Reaction: AB + B &#8652; AB<sub>2</sub>");
}

QString itc_IItoII_Model::ModelInfo() const
{
    QString result = AbstractItcModel::ModelInfo();
    result += BC50::IItoII::Format_BC50(GlobalParameter(0), GlobalParameter(1), GlobalParameter(2));

    return result;
}

QString itc_IItoII_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{
    QString result = AbstractItcModel::AnalyseMonteCarlo(object);

    if (!forceAll)
        return result;

    QString bc = Statistic::MonteCarlo2BC50_2_2(GlobalParameter(0), GlobalParameter(1), GlobalParameter(2), object);

    return bc + result;
}

#include "itc_2_2_Model.moc"
