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

#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/models.h"
#include "src/core/thermo.h"
#include "src/core/toolset.h"

#include <Eigen/Dense>

#include <QDebug>
#include <QtMath>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>

#include <cmath>
#include <functional>

#include "2_1_1_1_1_2_Model.h"

IItoI_ItoI_ItoII_Model::IItoI_ItoI_ItoII_Model(DataClass* data)
    : AbstractTitrationModel(data)
{
    m_threadpool = new QThreadPool(this);
    for (int i = 0; i < DataPoints(); ++i)
        m_solvers << new IItoI_ItoI_ItoII_Solver();

    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

IItoI_ItoI_ItoII_Model::~IItoI_ItoI_ItoII_Model()
{
    qDeleteAll(m_solvers);
}

void IItoI_ItoI_ItoII_Model::DeclareOptions()
{
    QStringList cooperativity = QStringList() << "full"
                                              << "noncooperative"
                                              << "additive"
                                              << "statistical";
    addOption(Cooperativity2_1, "Cooperativity 2:1", cooperativity);
    cooperativity = QStringList() << "full"
                                  << "noncooperative"
                                  << "additive"
                                  << "statistical";
    addOption(Cooperativity1_2, "Cooperativity 1:2", cooperativity);

    AbstractTitrationModel::DeclareOptions();
}

void IItoI_ItoI_ItoII_Model::EvaluateOptions()
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
    AbstractTitrationModel::EvaluateOptions();
}

void IItoI_ItoI_ItoII_Model::InitialGuess_Private()
{
    qreal K11 = Guess_1_1();
    //GlobalTable() = QList<qreal>() << 1 << K11 << 1;
    (*GlobalTable())[0] = K11 / 2;
    (*GlobalTable())[1] = Guess_1_1();
    (*GlobalTable())[2] = K11 / 2;
    qreal factor = 1;

    if (getOption(Method) == "UV/VIS") {
        factor = 1 / InitialHostConcentration(0);
    }

    LocalTable()->setColumn(DependentModel()->firstRow() * factor, 0);
    LocalTable()->setColumn(DependentModel()->firstRow() * factor, 1);
    LocalTable()->setColumn(DependentModel()->lastRow() * factor, 2);
    LocalTable()->setColumn(DependentModel()->lastRow() * factor, 3);

    Calculate();
}

void IItoI_ItoI_ItoII_Model::CalculateVariables()
{
    QString method = getOption(Method);

    qreal K21 = qPow(10, GlobalParameter(0));
    qreal K11 = qPow(10, GlobalParameter(1));
    qreal K12 = qPow(10, GlobalParameter(2));
    m_constants_pow = QList<qreal>() << K21 << K11 << K12;

    int maxthreads = qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);

        m_solvers[i]->setInput(host_0, guest_0);
        m_solvers[i]->setConfig(m_opt_config);
        m_solvers[i]->setConstants(m_constants_pow);
        //if(QThreadPool::globalInstance()->activeThreadCount())
        m_solvers[i]->run();
        //else
        //    m_threadpool->start(m_solvers[i]);
    }

    while (m_threadpool->activeThreadCount()) { /*QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);*/
    }

    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        if (!m_solvers[i]->Ok()) {
#ifdef _DEBUG
            qDebug() << "Numeric didn't work out well, mark model as corrupt! - Dont panic. Not everything is lost ...";
            qDebug() << m_solvers[i]->Ok() << InitialHostConcentration(i) << InitialGuestConcentration(i);
#endif
            m_corrupt = true;
            if (m_opt_config.skip_not_converged_concentrations) {
#ifdef _DEBUG
                qDebug() << "Ok, I skip the current result ...";
#endif
                continue;
            }
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

        if (!m_fast)
            SetConcentration(i, vector);

        qreal value = 0;
        for (int j = 0; j < SeriesCount(); ++j) {
            if (method == "NMR")
                value = host / host_0 * LocalTable()->data(0, j) + 2 * complex_21 / host_0 * LocalTable()->data(1, j) + complex_11 / host_0 * LocalTable()->data(2, j) + complex_12 / host_0 * LocalTable()->data(3, j);
            else if (method == "UV/VIS")
                value = host * LocalTable()->data(0, j) + 2 * complex_21 * LocalTable()->data(1, j) + complex_11 * LocalTable()->data(2, j) + complex_12 * LocalTable()->data(3, j);

            SetValue(i, j, value);
        }
    }
}

QSharedPointer<AbstractModel> IItoI_ItoI_ItoII_Model::Clone()
{
    QSharedPointer<IItoI_ItoI_ItoII_Model> model = QSharedPointer<IItoI_ItoI_ItoII_Model>(new IItoI_ItoI_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

QVector<qreal> IItoI_ItoI_ItoII_Model::OptimizeParameters_Private()
{
    QString coop21 = getOption(Cooperativity2_1);
    QString coop12 = getOption(Cooperativity1_2);
    QString host = getOption(Host);

    if (coop21 == "additive" || coop21 == "full")
        addGlobalParameter(0);
    addGlobalParameter(1);

    if (coop12 == "additive" || coop12 == "full")
        addGlobalParameter(2);

    if (host == "no")
        addLocalParameter(0);

    if (coop21 == "noncooperative" || coop21 == "full")
        addLocalParameter(1);

    addLocalParameter(2);
    if (coop12 == "noncooperative" || coop12 == "full")
        addLocalParameter(3);

    QVector<qreal> parameter;
    for (int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

MassResults IItoI_ItoI_ItoII_Model::MassBalance(qreal A, qreal B)
{
    //     QMutexLocker (&mutex);
    MassResults result;
    qreal K11 = m_constants_pow[0];
    qreal K21 = m_constants_pow[1];
    qreal K12 = m_constants_pow[2];

    Vector values(2);

    qreal complex_21 = K11 * K21 * A * A * B;
    qreal complex_11 = K11 * A * B;
    qreal complex_12 = K11 * K12 * A * B * B;

    values(0) = (2 * complex_21 + complex_11 + complex_12);
    values(1) = (complex_21 + complex_11 + 2 * complex_12);
    result.MassBalance = values;
    return result;
}

QString IItoI_ItoI_ItoII_Model::AdditionalOutput() const
{
    QString result = tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += "<h4>without statistical data:</h4>";

    auto conf2therm = [&result, this](const QJsonObject& object = QJsonObject()) {
        result += "<p>Reaction: A + B &#8652; AB</p>";
        result += Thermo::Statistic2Thermo(GlobalParameter(1), 0, getT(), object);
        result += "<p>Reaction: AB + A &#8652; A<sub>2</sub>B</p>";
        result += Thermo::Statistic2Thermo(GlobalParameter(0), 0, getT(), object);
        result += "<p>Reaction: AB + B &#8652; AB<sub>2</sub></p>";
        result += Thermo::Statistic2Thermo(GlobalParameter(2), 0, getT(), object);
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

#include "2_1_1_1_1_2_Model.moc"
