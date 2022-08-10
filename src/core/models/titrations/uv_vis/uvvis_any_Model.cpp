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

#include <Eigen/SVD>

#include "EqnConc_2.h"

#include "src/core/models/postprocess/statistic.h"

#include "src/core/bc50.h"
#include "src/core/concentrationalpolynomial.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "src/core/models/models.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "uvvis_any_Model.h"

uvvis_any_Model::uvvis_any_Model(DataClass* data)
    : AbstractTitrationModel(data)
{
    m_pre_input = { MaxA_Json, MaxB_Json };
    m_complete = false;
}

uvvis_any_Model::uvvis_any_Model(AbstractTitrationModel* data)
    : AbstractTitrationModel(data)
{
    DefineModel();

    // DefineModel(m_model_definition);
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
}

uvvis_any_Model::~uvvis_any_Model()
{
    qDeleteAll(m_solvers);
    qDeleteAll(m_ext_solvers);
}

bool uvvis_any_Model::DefineModel()
{
    // qint64 t0 = QDateTime::currentMSecsSinceEpoch();

    QJsonObject object = m_defined_model.value("MaxA");
    m_maxA = object["value"].toInt();

    object = m_defined_model.value("MaxB");
    m_maxB = object["value"].toInt();

    m_global_names.clear();
    m_species_names.clear();
    m_species_names << "B";
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

            QStringList host = QStringList() << "yes"
                                             << "no";
            addOption(Host + 1 + Index(i, j), QString("A%1B%2").arg(name_i_short).arg(name_j_short), host);
        }
    }

    m_global_parametersize = m_maxA * m_maxB;
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
    for (int i = 0; i < GlobalParameterSize(); ++i)
        setOption(Host + 1 + i, "yes");

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
        solver->setMaxIter(m_maxA * m_maxB * 100);
        solver->setConvergeThreshold(1e-15);

        EqnConc_2x* solver_ext = new EqnConc_2x;
        m_ext_solvers << solver_ext;
        solver_ext->setStoichiometry({ m_maxA, m_maxB });
        solver_ext->setInitialConcentrations({ host_0, guest_0 });
        solver_ext->Guess();
        solver_ext->setMaxIter(m_maxA * m_maxB * 100);
        solver_ext->setConvergeThreshold(1e-16);
    }
    // std::cout << QDateTime::currentMSecsSinceEpoch() - t0 << std::endl;

    return true;
}

void uvvis_any_Model::InitialGuess_Private()
{
    LocalTable()->setColumn(DependentModel()->firstRow(), 0);
    QVector<double> ratios;
    double factor = double(m_maxA * m_maxB);
    double last = 1 / double(InitialGuestConcentration(DataPoints() - 1) / InitialHostConcentration(DataPoints() - 1));
    for (int i = 0; i < DataPoints(); ++i) {
        ratios << InitialGuestConcentration(i) / InitialHostConcentration(i) * last * factor;
    }

    double guess_K = 4;
    (*GlobalTable())[0] = guess_K;
    for (int a = 1; a <= m_maxA; ++a)
        for (int b = 1; b <= m_maxB; ++b) {

            double ratio = b / a;
            int best_index = 0;
            double diff = ratios.last();
            for (int index = 0; index < ratios.size(); ++index) {
                if (abs(ratio - ratios[index]) < diff) {
                    best_index = index;
                    diff = abs(ratio - ratios[index]);
                }
            }
            //       qDebug() << b/a << best_index;
            (*GlobalTable())[(Index(a, b))] = guess_K + a + b;
            LocalTable()->setColumn(DependentModel()->Row(best_index), 1 + Index(a, b));
        }
    UpdateShifts();
    Calculate();
}

void uvvis_any_Model::OptimizeParameters_Private()
{
    for (int a = 1; a <= m_maxA; ++a) {
        for (int b = 1; b <= m_maxB; ++b) {
            if (getOption(Host + 1 + Index(a, b)) == "yes") {
                addGlobalParameter(Index(a, b));
                addLocalParameter(2 + Index(a, b));
            }
        }
    }
    // QString host = getOption(Host);

    // if (host == "no")
    addLocalParameter(0);
    addLocalParameter(1);
}

void uvvis_any_Model::CalculateConcentrations()
{
    std::vector<double> constants(GlobalParameterSize());
    for (int i = 0; i < GlobalParameterSize(); ++i) {
        if (GlobalTable()->isChecked(0, i))
            constants[i] = pow(10, GlobalParameter(i));
        else
            constants[i] = 0;
    }

    for (int a = 1; a <= m_maxA; ++a)
        for (int b = 1; b <= m_maxB; ++b) {
            if (getOption(Host + 1 + Index(a, b)) == "yes") {
                constants[Index(a, b)] = pow(10, GlobalParameter(Index(a, b)));
            } else
                constants[Index(a, b)] = 0;
        }
    m_concentrations = Eigen::MatrixXd(DataPoints(), 1 + m_species_names.size());
    m_molar_ratios = Eigen::MatrixXd(DataPoints(), 1 + m_species_names.size());

    qreal value = 0;
    // int timer = 0;
    // qint64 t0 = QDateTime::currentMSecsSinceEpoch();
    bool failed = false;
    for (int i = 0; i < DataPoints(); ++i) {
        // m_solvers[i]->Guess();
        // m_solvers[i]->setStabilityConstants(constants);
        m_ext_solvers[i]->setStabilityConstants(constants);
        qreal host_0 = InitialHostConcentration(i);
        std::vector<double> result;
        // result = m_solvers[i]->solver();
        // failed = failed || m_solvers[i]->LastIterations() == (m_solvers[i]->MaxIter());

        result = m_ext_solvers[i]->solver();

        // std::cout << m_solvers[i]->LastIterations() << " " << m_solvers[i]->LastConvergency() << std::endl;

        //  timer += m_solvers[i]->Timer();
        double host = result[0];
        double guest = result[1];
        m_concentrations(i, 0) = host;
        m_concentrations(i, 1) = 0;

        m_molar_ratios(i, 0) = host;

        Vector vector(m_species_names.size() + 3);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;

        int index = 2;
        for (int a = 1; a <= m_maxA; ++a) {
            double powA = a * pow(host, a);
            for (int b = 1; b <= m_maxB; b++) {
                double beta = constants[Index(a, b)];
                const double c = (beta * pow(guest, b) * powA);
                m_concentrations(i, index) = c;
                m_molar_ratios(i, index - 1) = c;

                vector(index++) = c;
            }
        }
        if (!m_fast)
            SetConcentration(i, vector);
    }
}

void uvvis_any_Model::UpdateShifts()
{
    CalculateConcentrations();
    // std::cout << m_concentrations << std::endl <<m_molar_ratios << std::endl;
    Eigen::MatrixXd dep = DependentModel()->Table();
    // std::cout << dep << std::endl;
    // Eigen::MatrixXd x = m_molar_ratios.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(dep);// m_molar_ratios.llt().solve(dep);
    Eigen::MatrixXd x = m_concentrations.colPivHouseholderQr().solve(dep); // m_molar_ratios.llt().solve(dep);

    // std::cout << x << std::endl;
    qreal value = 0;
    // int timer = 0;
    // qint64 t0 = QDateTime::currentMSecsSinceEpoch();
    bool failed = false;
    LocalParameter()->setTable(x.transpose());
}

void uvvis_any_Model::CalculateVariables()
{
    /*
    std::vector<double> constants(GlobalParameterSize());
    for (int i = 0; i < GlobalParameterSize(); ++i) {
        if (GlobalTable()->isChecked(0, i))
            constants[i] = pow(10, GlobalParameter(i));
        else
            constants[i] = 0;
    }

    for (int a = 1; a <= m_maxA; ++a)
        for (int b = 1; b <= m_maxB; ++b) {
            if (getOption(Host + 1 + Index(a, b)) == "yes") {
                constants[Index(a, b)] = pow(10, GlobalParameter(Index(a, b)));
            } else
                constants[Index(a, b)] = 0;
        }
    */
    // UpdateShifts();
    // std::cout << x << std::endl;
    qreal value = 0;
    // int timer = 0;
    // qint64 t0 = QDateTime::currentMSecsSinceEpoch();
    bool failed = false;
    CalculateConcentrations();
    Eigen::MatrixXd m = m_concentrations * LocalParameter()->Table().transpose();
    for (int i = 0; i < DataPoints(); ++i)
        for (int j = 0; j < SeriesCount(); ++j)
            SetValue(i, j, m(i, j));
    /*
        for (int i = 0; i < DataPoints(); ++i) {
            // m_solvers[i]->Guess();
             m_solvers[i]->setStabilityConstants(constants);
            m_ext_solvers[i]->setStabilityConstants(constants);
            qreal host_0 = InitialHostConcentration(i);
            std::vector<double> result;
            // result = m_solvers[i]->solver();
            // failed = failed || m_solvers[i]->LastIterations() == (m_solvers[i]->MaxIter());

            result = m_ext_solvers[i]->solver();

            // std::cout << m_solvers[i]->LastIterations() << " " << m_solvers[i]->LastConvergency() << std::endl;

            //  timer += m_solvers[i]->Timer();
            double host = result[0];
            double guest = result[1];

            Vector vector(m_species_names.size() + 3);
            vector(0) = i + 1;
            vector(1) = host;
            vector(2) = guest;

            int index = 3;
            for (int a = 1; a <= m_maxA; ++a) {
                double powA = a*pow(host, a);
                for (int b = 1; b <= m_maxB; b++) {
                    double beta = constants[Index(a, b)];
                    const double c = (beta * pow(guest, b)*powA);
                    vector(index++) = c;
                }
            }

            for (int j = 0; j < SeriesCount(); ++j) {
                value = host / host_0 * LocalTable()->data(j, 0);
                for (int a = 1; a <= m_maxA; ++a) {
                    double powA = a * pow(host, a);
                    for (int b = 1; b <= m_maxB; b++) {
                        double beta = constants[Index(a, b)];
                        const double c = (beta * pow(guest, b) * powA);
                        value += (a * c / host_0 * LocalTable()->data(j, Index(a, b) + 1)) * GlobalTable()->isChecked(0, Index(a, b));
                    }
                }

                //+ complex / host_0 * LocalTable()->data(j, 1);
                SetValue(i, j, value);
            }
            if (!m_fast)
                SetConcentration(i, vector);
        }*/
    if (failed)
        m_corrupt = true;
    //  std::cout << timer << "  " << QDateTime::currentMSecsSinceEpoch() - t0 << std::endl;
}
/*
QVector<qreal> uvvis_any_Model::DeCompose(int datapoint, int series) const
{
    QString method = getOption(Method);

    QVector<qreal> vector;
    qreal host_0 = InitialHostConcentration(datapoint);

    Vector concentration = getConcentration(datapoint);

    qreal host = concentration(1);

    qreal complex = concentration(3);
    ;

    vector << host / host_0 * LocalTable()->data(series, 0);
    vector << complex / host_0 * LocalTable()->data(series, 1);

    return vector;
}
*/
QSharedPointer<AbstractModel> uvvis_any_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractModel> model = QSharedPointer<uvvis_any_Model>(new uvvis_any_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

QString uvvis_any_Model::ModelInfo() const
{
    QString result = AbstractTitrationModel::ModelInfo();
    result += BC50::ItoI::Format_BC50(GlobalParameter(0));

    return result;
}

QString uvvis_any_Model::AdditionalOutput() const
{
    QString result;
    return result;

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
        // std::cout << host << " "
        //          << " " << guest_0 - complex << " " << complex << std::endl;
        // std::cout << integral.transpose() << std::endl;
    }
    integral(0) /= end;
    integral(1) /= end;
    integral(2) /= end;
    std::cout << integral.transpose() << std::endl;
}

QString uvvis_any_Model::ParameterComment(int parameter) const
{
    Q_UNUSED(parameter)
    return QString("Reaction: A + B &#8652; AB");
}

QString uvvis_any_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractTitrationModel::AnalyseMonteCarlo(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::MonteCarlo2BC50_1(GlobalParameter(0), object);
    return bc + result;
}

QString uvvis_any_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractTitrationModel::AnalyseGridSearch(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::GridSearch2BC50_1(GlobalParameter(0), object);
    return bc + result;
}

#include "uvvis_any_Model.moc"
