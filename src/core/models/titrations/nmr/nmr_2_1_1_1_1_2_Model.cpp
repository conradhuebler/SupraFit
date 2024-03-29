/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/bc50.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "src/core/models/models.h"

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

#include "nmr_2_1_1_1_1_2_Model.h"

nmr_IItoI_ItoI_ItoII_Model::nmr_IItoI_ItoI_ItoII_Model(DataClass* data)
    : AbstractNMRModel(data)
{
    m_threadpool = new QThreadPool(this);
    for (int i = 0; i < DataPoints(); ++i)
        m_solvers << new IItoI_ItoI_ItoII_Solver();

    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

nmr_IItoI_ItoI_ItoII_Model::nmr_IItoI_ItoI_ItoII_Model(AbstractNMRModel* data)
    : AbstractNMRModel(data)
{
    m_threadpool = new QThreadPool(this);
    for (int i = 0; i < DataPoints(); ++i)
        m_solvers << new IItoI_ItoI_ItoII_Solver();

    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

nmr_IItoI_ItoI_ItoII_Model::~nmr_IItoI_ItoI_ItoII_Model()
{
    qDeleteAll(m_solvers);
    qDebug() << "time for calculation" << m_time << "in" << Description();
}

void nmr_IItoI_ItoI_ItoII_Model::DeclareOptions()
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

    AbstractNMRModel::DeclareOptions();
}

void nmr_IItoI_ItoI_ItoII_Model::EvaluateOptions()
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
            this->LocalTable()->data(i, 1) = 2 * (this->LocalTable()->data(i, 2) - this->LocalTable()->data(i, 0)) + this->LocalTable()->data(i, 0);
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
            this->LocalTable()->data(i, 3) = 2 * (this->LocalTable()->data(i, 2) - this->LocalTable()->data(i, 0)) + this->LocalTable()->data(i, 0);
    };

    if (coop12 == "noncooperative") {
        global_coop12();
    } else if (coop12 == "additive") {
        local_coop12();
    } else if (coop12 == "statistical") {
        local_coop12();
        global_coop12();
    }
    AbstractNMRModel::EvaluateOptions();
}

void nmr_IItoI_ItoI_ItoII_Model::InitialGuess_Private()
{
    int index_21 = 0, index_11 = 0;

    for (int i = 0; i < DataPoints(); ++i) {
        if (XValue(i) <= 0.5)
            index_21 = i;
        if (XValue(i) <= 1)
            index_11 = i;
    }

    LocalTable()->setColumn(DependentModel()->firstRow(), 0);
    LocalTable()->setColumn(DependentModel()->Row(index_21), 1);
    LocalTable()->setColumn(DependentModel()->Row(index_11), 2);
    LocalTable()->setColumn(DependentModel()->lastRow(), 3);

    qreal K = GuessK(1);

    (*GlobalTable())[1] = K * 0.8;
    (*GlobalTable())[0] = K / 2.0;

    (*GlobalTable())[2] = K / 2.0;

    Calculate();
}

void nmr_IItoI_ItoI_ItoII_Model::CalculateVariables()
{
    qreal K21 = qPow(10, GlobalParameter(0));
    qreal K11 = qPow(10, GlobalParameter(1));
    qreal K12 = qPow(10, GlobalParameter(2));
    m_constants_pow = QList<qreal>() << K21 << K11 << K12;

    int maxthreads = qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);

    bool skip = m_opt_config["Skip_not_Converged_Concentrations"].toBool();

    for (int i = DataBegin(); i < DataEnd(); ++i) {
        // for (int i = 0; i < DataPoints(); ++i) {
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

    for (int i = DataBegin(); i < DataEnd(); ++i) {
        // for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        m_time += m_solvers[i]->Time() + m_solvers[i]->LTime();
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

        if (!m_fast) {
            SetConcentration(i, vector);
            // qDebug() << log10(complex_12/complex_11/guest) << log10(K12) << log10(complex_21/complex_11/host) << log10(K21) << log10(complex_11/host/guest) << log10(K11);
        }
        qreal value = 0;
        for (int j = 0; j < SeriesCount(); ++j) {
#pragma message("things got removed, because they seem to be old")
            //            if (method == "NMR")
            value = host / host_0 * LocalTable()->data(j, 0) + 2 * complex_21 / host_0 * LocalTable()->data(j, 1) + complex_11 / host_0 * LocalTable()->data(j, 2) + complex_12 / host_0 * LocalTable()->data(j, 3);
            //            else if (method == "UV/VIS")
            //                value = host * LocalTable()->data(0, j) + 2 * complex_21 * LocalTable()->data(1, j) + complex_11 * LocalTable()->data(2, j) + complex_12 * LocalTable()->data(3, j);

            SetValue(i, j, value);
        }
    }
}

QVector<qreal> nmr_IItoI_ItoI_ItoII_Model::DeCompose(int datapoint, int series) const
{
    qreal host_0 = InitialHostConcentration(datapoint);

    Vector concentration = getConcentration(datapoint);

    qreal host = concentration(1);

    qreal complex_21 = concentration(3);
    qreal complex_11 = concentration(4);
    qreal complex_12 = concentration(5);

    QVector<qreal> vector;

    vector << host / host_0 * LocalTable()->data(series, 0);
    vector << 2 * complex_21 / host_0 * LocalTable()->data(series, 1);
    vector << complex_11 / host_0 * LocalTable()->data(series, 2);
    vector << complex_12 / host_0 * LocalTable()->data(series, 3);

    return vector;
}

QSharedPointer<AbstractModel> nmr_IItoI_ItoI_ItoII_Model::Clone(bool statistics)
{
    QSharedPointer<nmr_IItoI_ItoI_ItoII_Model> model = QSharedPointer<nmr_IItoI_ItoI_ItoII_Model>(new nmr_IItoI_ItoI_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

void nmr_IItoI_ItoI_ItoII_Model::OptimizeParameters_Private()
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
}

MassResults nmr_IItoI_ItoI_ItoII_Model::MassBalance(qreal A, qreal B)
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

QString nmr_IItoI_ItoI_ItoII_Model::ModelInfo() const
{
    QString result = AbstractNMRModel::ModelInfo();
    result += BC50::IItoII::Format_BC50(GlobalParameter(0), GlobalParameter(1), GlobalParameter(2));

    return result;
}

QString nmr_IItoI_ItoI_ItoII_Model::ParameterComment(int parameter) const
{
    if (parameter == 0)
        return QString("Reaction: AB + A &#8652; A<sub>2</sub>B");
    else if (parameter == 1)
        return QString("Reaction: A + B &#8652; AB");
    else
        return QString("Reaction: AB + B &#8652; AB<sub>2</sub>");
}

QString nmr_IItoI_ItoI_ItoII_Model::AdditionalOutput() const
{
    QString result;
    /*
    // double max = 1e3;
    double delta = 1e-3;
    qreal host_0 = 1.0;
    qreal host = 0;
    qreal diff = host_0 - host;
    Vector integral(3);
    qreal end = delta;
    for (end = delta; diff > 1e-5; end += delta) {
        qreal guest_0 = end;
        host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        qreal complex = host_0 - host;

        integral(0) += host * delta;
        integral(1) += (guest_0 - complex) * delta;
        integral(2) += complex * delta;

        diff = host_0 - complex;
        // std::cout << end << " " << diff << " " << host << " " << " " << guest_0 - complex << " " << complex << std::endl;
        std::cout << host << " "
                  << " " << guest_0 - complex << " " << complex << std::endl;
        //std::cout << integral.transpose() << std::endl;
    }
    integral(0) /= end;
    integral(1) /= end;
    integral(2) /= end;
    std::cout << integral.transpose() << std::endl;
    */
    return result;
}

QString nmr_IItoI_ItoI_ItoII_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{
    QString result = AbstractNMRModel::AnalyseMonteCarlo(object);

    if (!forceAll)
        return result;

    QString bc = Statistic::MonteCarlo2BC50_2_2(GlobalParameter(0), GlobalParameter(1), GlobalParameter(2), object);

    return bc + result;
}

QString nmr_IItoI_ItoI_ItoII_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{
    QString result = AbstractNMRModel::AnalyseGridSearch(object);

    if (!forceAll)
        return result;

    QString bc = Statistic::GridSearch2BC50_2_2(GlobalParameter(0), GlobalParameter(1), GlobalParameter(2), object);

    return bc + result;
}

#include "nmr_2_1_1_1_1_2_Model.moc"
