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
#include "src/core/models/titrations/AbstractTitrationModel.h" // MaxA_Json/MaxB_Json (was transitive via models.h)

#include "EqnConc_2.h"

#include "src/core/models/postprocess/statistic.h"

#include "src/core/bc50.h"
#include "src/core/bfgsconcentrationsolver.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "src/core/models/models.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "nmr_any_Model.h"

// Human-readable species label with unicode subscripts, e.g. (2,0) -> "A₂", (1,1) -> "AB",
// (2,1) -> "A₂B". A zero coefficient drops the component; a coefficient of 1 drops the subscript.
// Claude Generated.
static QString SpeciesLabel(int a, int b)
{
    auto part = [](const QString& symbol, int n) -> QString {
        if (n == 0)
            return QString();
        if (n == 1)
            return symbol;
        return symbol + ToolSet::UnicodeLowerInteger(QString::number(n));
    };
    return part("A", a) + part("B", b);
}

nmr_any_Model::nmr_any_Model(DataClass* data)
    : AbstractNMRModel(data)
{
    m_pre_input = { MaxA_Json, MaxB_Json, MaxSelfA_Json, Species_Json };
    m_complete = false;
}

nmr_any_Model::nmr_any_Model(AbstractNMRModel* data)
    : AbstractNMRModel(data)
{
    DefineModel();

    // DefineModel(m_model_definition);
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
}

nmr_any_Model::~nmr_any_Model()
{
    qDeleteAll(m_ext_solvers);
    delete m_bfgs;
}

bool nmr_any_Model::DefineModel()
{
    QJsonObject object = m_defined_model.value("MaxA");
    m_maxA = object["value"].toInt();

    object = m_defined_model.value("MaxB");
    m_maxB = object["value"].toInt();

    object = m_defined_model.value("MaxSelfA");
    m_maxSelfA = object["value"].toInt();

    // sane floors so an empty definition still yields the classic 1:1 grid
    if (m_maxA < 1)
        m_maxA = 1;
    if (m_maxB < 1)
        m_maxB = 1;

    // Build the species list: the classic A_aB_b grid first (a,b >= 1), so existing projects keep
    // their global-parameter indices, then the pure host oligomers A_n (b = 0) for self-aggregation.
    m_species.clear();
    for (int a = 1; a <= m_maxA; ++a)
        for (int b = 1; b <= m_maxB; ++b)
            m_species << qMakePair(a, b);
    for (int n = 2; n <= m_maxSelfA; ++n)
        m_species << qMakePair(n, 0);
    m_has_selfagg = (m_maxSelfA >= 2);

    // Optional explicit species list from the species editor: "a,b|a,b|..." overrides the grid,
    // allowing an arbitrary selection of complexes and self-aggregates (e.g. "2,0|1,1" = {A2, AB}).
    // Free monomers (1,0)/(0,1) are implicit and skipped. Claude Generated.
    const QString speciesDef = m_defined_model.value("Species")["value"].toString().trimmed();
    if (!speciesDef.isEmpty()) {
        m_species.clear();
        m_has_selfagg = false;
        const QStringList tokens = speciesDef.split("|", Qt::SkipEmptyParts);
        for (const QString& token : tokens) {
            const QStringList ab = token.split(",");
            if (ab.size() != 2)
                continue;
            bool okA = false, okB = false;
            const int a = ab[0].trimmed().toInt(&okA);
            const int b = ab[1].trimmed().toInt(&okB);
            if (!okA || !okB || a < 0 || b < 0 || a + b < 2)
                continue; // skip invalid tokens and the implicit free monomers
            m_species << qMakePair(a, b);
            if (a == 0 || b == 0)
                m_has_selfagg = true; // pure oligomer -> the grid EqnConc solver cannot represent it
        }
    }

    m_global_names.clear();
    m_species_names.clear();
    m_stoich = Eigen::MatrixXi(2, m_species.size());
    for (int k = 0; k < m_species.size(); ++k) {
        const int a = m_species[k].first;
        const int b = m_species[k].second;
        m_stoich(0, k) = a;
        m_stoich(1, k) = b;

        const QString sname = SpeciesLabel(a, b);
        m_species_names << sname;
        m_global_names << QString("lg %1%2%3").arg(Unicode_beta)
                              .arg(ToolSet::UnicodeLowerInteger(QString::number(a)))
                              .arg(ToolSet::UnicodeLowerInteger(QString::number(b)));

        addOption(Host + 1 + k, sname, QStringList() << "yes" << "no");
    }

    m_global_parametersize = m_species.size();
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
    addOption(SolverChoice, "Concentration solver", QStringList() << "BFGS" << "Polynomial");
    for (int k = 0; k < GlobalParameterSize(); ++k)
        setOption(Host + 1 + k, "yes");

    CollectOptimizationParameters_Private();
    m_complete = true;

    // General BFGS speciation solver (handles self-aggregation and any stoichiometry).
    delete m_bfgs;
    m_bfgs = new BFGSConcentrationSolver;
    m_bfgs->setStoichiometry(m_stoich);
    m_bfgs->setMaxIter(1000);
    m_bfgs->setConvergeThreshold(1e-12);

    // The grid polynomial solver (EqnConc_2x) is only valid for the pure A_aB_b grid; keep it as a
    // per-datapoint alternative when no self-aggregation is requested.
    qDeleteAll(m_ext_solvers);
    m_ext_solvers.clear();
    if (!m_has_selfagg) {
        for (int i = 0; i < DataPoints(); ++i) {
            qreal host_0 = InitialHostConcentration(i);
            qreal guest_0 = InitialGuestConcentration(i);

            EqnConc_2x* solver_ext = new EqnConc_2x;
            m_ext_solvers << solver_ext;
            solver_ext->setStoichiometry({ m_maxA, m_maxB });
            solver_ext->setInitialConcentrations({ host_0, guest_0 });
            solver_ext->Guess();
            solver_ext->setMaxIter(m_maxA * m_maxB * 100);
            solver_ext->setConvergeThreshold(1e-16);
        }
    }

    return true;
}

void nmr_any_Model::InitialGuess_Private()
{
    LocalTable()->setColumn(DependentModel()->firstRow(), 0);
    QVector<double> ratios;
    double factor = double(m_maxA * m_maxB);
    double last = 1 / double(InitialGuestConcentration(DataPoints() - 1) / InitialHostConcentration(DataPoints() - 1));
    for (int i = DataBegin(); i < DataEnd(); ++i) {
        // for (int i = 0; i < DataPoints(); ++i) {
        ratios << InitialGuestConcentration(i) / InitialHostConcentration(i) * last * factor;
    }
    // qDebug() << ratios;
    double guess_K = 4;
    (*GlobalTable())[0] = guess_K;
    for (int k = 0; k < m_species.size(); ++k) {
        const int a = m_species[k].first;
        const int b = m_species[k].second;

        double ratio = a > 0 ? double(b) / a : 0;
        int best_index = 0;
        double diff = ratios.last();
        for (int index = 0; index < ratios.size(); ++index) {
            if (abs(ratio - ratios[index]) < diff) {
                best_index = index;
                diff = abs(ratio - ratios[index]);
            }
        }
        (*GlobalTable())[k] = guess_K + a + b;
        LocalTable()->setColumn(DependentModel()->Row(best_index), 1 + k);
    }
    // UpdateShifts();
    Calculate();
}

void nmr_any_Model::CollectOptimizationParameters_Private()
{
    for (int k = 0; k < m_species.size(); ++k) {
        if (getOption(Host + 1 + k) == "yes") {
            addGlobalParameter(k);
            addLocalParameter(1 + k);
        }
    }
    QString host = getOption(Host);

    if (host == "no")
        addLocalParameter(0);
}

void nmr_any_Model::CalculateConcentrations()
{
    const int nSpecies = m_species.size();

    // Linear stability constants; a deactivated (option "no") or unchecked species contributes 0.
    std::vector<double> constants(nSpecies);
    for (int k = 0; k < nSpecies; ++k) {
        if (getOption(Host + 1 + k) == "yes" && GlobalTable()->isChecked(0, k))
            constants[k] = pow(10, GlobalParameter(k));
        else
            constants[k] = 0;
    }

    m_concentrations = Eigen::MatrixXd(DataPoints(), 2 + nSpecies);
    m_molar_ratios = Eigen::MatrixXd(DataPoints(), 1 + nSpecies);

    // The EqnConc grid solver cannot represent self-aggregation, so route through the BFGS solver
    // whenever self-aggregation is present or the user selected it (the default).
    const bool use_bfgs = m_has_selfagg || m_ext_solvers.isEmpty()
        || getOption(SolverChoice) != "Polynomial";
    if (use_bfgs)
        m_bfgs->setStabilityConstants(constants);

    for (int i = DataBegin(); i < DataEnd(); ++i) {
        const qreal host_0 = InitialHostConcentration(i);
        const qreal guest_0 = InitialGuestConcentration(i);

        std::vector<double> result;
        if (use_bfgs) {
            m_bfgs->setTotalConcentrations({ host_0, guest_0 });
            result = m_bfgs->solve();
        } else {
            m_ext_solvers[i]->setStabilityConstants(constants);
            result = m_ext_solvers[i]->solver();
        }

        const double host = result[0];
        const double guest = result[1];
        m_concentrations(i, 0) = host;
        m_concentrations(i, 1) = guest;
        m_molar_ratios(i, 0) = host / host_0;

        Vector vector(nSpecies + 3);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;

        for (int k = 0; k < nSpecies; ++k) {
            const int a = m_species[k].first;
            const int b = m_species[k].second;
            const double beta = constants[k];
            // moles of host A bound in species k = a * [A_a B_b]; free A + sum_k this = A_0
            const double c = a * beta * pow(host, a) * pow(guest, b);
            m_concentrations(i, 2 + k) = c;
            m_molar_ratios(i, 1 + k) = c / host_0;
            vector(3 + k) = c;
        }
        if (!m_fast)
            SetConcentration(i, vector);
    }
}

void nmr_any_Model::UpdateShifts()
{
    CalculateConcentrations();
    // Solve the linear response problem: fit the per-species chemical shifts (local parameters)
    // to the observed signal given the species mole fractions (m_molar_ratios).
    Eigen::MatrixXd dep = DependentModel()->Table();
    Eigen::MatrixXd x = m_molar_ratios.colPivHouseholderQr().solve(dep);
    LocalParameter()->setTable(x.transpose());
}

void nmr_any_Model::CalculateVariables()
{
    CalculateConcentrations();
    // Model signal = species mole fractions weighted by the per-species chemical shifts.
    Eigen::MatrixXd m = m_molar_ratios * LocalParameter()->Table().transpose();
    for (int i = 0; i < DataPoints(); ++i)
        for (int j = 0; j < SeriesCount(); ++j)
            SetValue(i, j, m(i, j));
}
/*
QVector<qreal> nmr_any_Model::DeCompose(int datapoint, int series) const
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
QSharedPointer<AbstractModel> nmr_any_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractModel> model = QSharedPointer<nmr_any_Model>(new nmr_any_Model(this), &QObject::deleteLater);
    finishClone(model, statistics);
    return model;
}

QString nmr_any_Model::ModelInfo() const
{
    QString result = AbstractNMRModel::ModelInfo();
    result += BC50::ItoI::Format_BC50(GlobalParameter(0));

    return result;
}

QString nmr_any_Model::AdditionalOutput() const
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
}

QString nmr_any_Model::ParameterComment(int parameter) const
{
    if (parameter >= 0 && parameter < m_species.size()) {
        const int a = m_species[parameter].first;
        const int b = m_species[parameter].second;
        if (b == 0)
            return QString("Self-aggregation: %1 A &#8652; %2").arg(a).arg(m_species_names.value(parameter));
        return QString("Reaction: %1 A + %2 B &#8652; %3").arg(a).arg(b).arg(m_species_names.value(parameter));
    }
    return QString("Reaction: A + B &#8652; AB");
}

QString nmr_any_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{
    return prependBC50(AbstractNMRModel::AnalyseMonteCarlo(object, forceAll), forceAll, Statistic::MonteCarlo2BC50_1(GlobalParameter(0), object));
}

QString nmr_any_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{
    return prependBC50(AbstractNMRModel::AnalyseGridSearch(object, forceAll), forceAll, Statistic::GridSearch2BC50_1(GlobalParameter(0), object));
}

#include "nmr_any_Model.moc"
