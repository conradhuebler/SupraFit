/*
 * SupraFit - flexible NMR titration model (arbitrary N-component equilibrium via BFGS speciation)
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * The speciation (free component + species concentrations) is delegated to the shared
 * SpeciationEngine on AbstractTitrationModel; this model only maps those concentrations to the NMR
 * signal (fast-exchange population-weighted average of the observed component's chemical shifts).
 * The equilibrium system is defined solely through the free-text Reactions field. Claude Generated.
 */

#include <Eigen/SVD>

#include "src/core/models/titrations/AbstractTitrationModel.h" // Reactions/MaxA/MaxB JSON templates

#include "src/core/models/postprocess/statistic.h"

#include "src/core/bc50.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/reactionparser.h"
#include "src/core/toolset.h"

#include "src/core/models/models.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "nmr_any_Model.h"

nmr_any_Model::nmr_any_Model(DataClass* data)
    : AbstractNMRModel(data)
{
    m_pre_input = { Reactions_Json };
    m_complete = false;
}

nmr_any_Model::nmr_any_Model(AbstractNMRModel* data)
    : AbstractNMRModel(data)
{
    DefineModel();

    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
}

nmr_any_Model::~nmr_any_Model()
{
}

bool nmr_any_Model::DefineModel()
{
    m_observed = 0; // NMR observes the first component (host A) by default

    // The N-component reaction editor is the sole definition path. Without a valid reaction system
    // the model stays undefined (the GUI/CLI must supply reactions before it can be fitted).
    if (!BuildSpeciationFromReactions()) {
        m_complete = false;
        return false;
    }
    m_uses_bfgs = true;

    const ReactionSystem& sys = m_speciation.System();
    const int nSpecies = sys.species.size();

    m_global_names.clear();
    m_species_names.clear();
    for (int k = 0; k < nSpecies; ++k) {
        const QString sname = sys.species[k].label;
        m_species_names << sname;
        m_global_names << QString("lg %1(%2)").arg(Unicode_beta).arg(sname);
        addOption(Host + 1 + k, sname, QStringList() << "yes" << "no");
    }

    m_global_parametersize = nSpecies;
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
    for (int k = 0; k < GlobalParameterSize(); ++k)
        setOption(Host + 1 + k, "yes");

    CollectOptimizationParameters_Private();
    m_complete = true;
    return true;
}

void nmr_any_Model::InitialGuess_Private()
{
    LocalTable()->setColumn(DependentModel()->firstRow(), 0);
    const int nSpecies = m_speciation.SpeciesCount();
    for (int k = 0; k < nSpecies; ++k) {
        // data-derived guess (scaled to the concentration range) instead of a fixed constant, so
        // higher-order complexes (AB2, ...) do not start far too high and send the optimiser into a
        // flat runaway direction. Claude Generated.
        (*GlobalTable())[k] = GuessLgBeta(k);
        LocalTable()->setColumn(DependentModel()->Row(DataPoints() - 1), 1 + k);
    }
    Calculate();
}

void nmr_any_Model::CollectOptimizationParameters_Private()
{
    const int nSpecies = m_speciation.SpeciesCount();
    for (int k = 0; k < nSpecies; ++k) {
        if (getOption(Host + 1 + k) == "yes") {
            addGlobalParameter(k);
            addLocalParameter(1 + k);
        }
    }
    if (getOption(Host) == "no")
        addLocalParameter(0);
}

void nmr_any_Model::CalculateConcentrations()
{
    const int nComp = m_component_count;
    const int nSpecies = m_speciation.SpeciesCount();
    const ReactionSystem& sys = m_speciation.System();

    // Linear stability constants; a deactivated (option "no") or unchecked species contributes 0.
    std::vector<double> constants(nSpecies);
    for (int k = 0; k < nSpecies; ++k) {
        if (getOption(Host + 1 + k) == "yes" && GlobalTable()->isChecked(0, k))
            constants[k] = pow(10, GlobalParameter(k));
        else
            constants[k] = 0;
    }
    m_speciation.setStabilityConstants(constants);

    m_concentrations = Eigen::MatrixXd(DataPoints(), nComp + nSpecies);
    m_molar_ratios = Eigen::MatrixXd(DataPoints(), 1 + nSpecies);

    std::vector<double> totals(nComp);
    for (int i = DataBegin(); i < DataEnd(); ++i) {
        for (int c = 0; c < nComp; ++c)
            totals[c] = InitialConcentration(i, c);

        m_speciation.solve(totals, i);
        const std::vector<double>& freeConc = m_speciation.FreeConcentrations();
        const std::vector<double>& speciesConc = m_speciation.SpeciesConcentrations();

        const double obs_total = totals[m_observed];

        // free component concentrations
        for (int c = 0; c < nComp; ++c)
            m_concentrations(i, c) = freeConc[c];
        m_molar_ratios(i, 0) = freeConc[m_observed] / obs_total;

        // stored concentration vector: [idx, free_0..free_{n-1}, obsBound_0..obsBound_{m-1}]
        Vector vector(nComp + 1 + nSpecies);
        vector(0) = i + 1;
        for (int c = 0; c < nComp; ++c)
            vector(1 + c) = freeConc[c];

        for (int k = 0; k < nSpecies; ++k) {
            // moles of the observed component bound in species k = M(observed,k) * [species_k]
            const int obsCoeff = sys.species[k].stoich(m_observed);
            const double bound = obsCoeff * speciesConc[k];
            m_concentrations(i, nComp + k) = bound;
            m_molar_ratios(i, 1 + k) = bound / obs_total;
            vector(1 + nComp + k) = bound;
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

void nmr_any_Model::ProjectLinearParameters()
{
    // VarPro linear projection: build the design matrix (mole fractions) for the current global
    // parameters, then solve the shifts by masked least-squares (unlike UpdateShifts(), which is the
    // unmasked initial-guess variant, this honours the active-point mask for correct CV/RA).
    CalculateConcentrations();
    SolveLinearMasked(m_molar_ratios);
}

bool nmr_any_Model::AnalyticVarProJacobian(const std::vector<int>& gidx, Eigen::MatrixXd& J)
{
    // Exact only for the Newton speciation method (its stored Hessian is the solution Hessian).
    if (!m_speciation.isValid()
        || m_speciation.method() != ConcentrationSolver::Method::LevenbergMarquardt)
        return false;

    const int nSpecies = m_speciation.SpeciesCount();
    const int nComp = m_component_count;
    const int series = SeriesCount();
    const int nG = static_cast<int>(gidx.size());
    const int nData = DataPoints();
    if (nSpecies == 0 || series == 0 || nG == 0)
        return false;
    const int P = 1 + nSpecies; // design columns: free observed + one per species

    // SolveLinearMasked (and the residual) run over [DataBegin, DataEnd); Cross-Validation / Reduction
    // mask by *un-checking rows* inside that window (isChecked), which is handled below. A shrunk window
    // itself is not the CV/RA mechanism, so fall back to FD for that rare case. Claude Generated.
    if (DataBegin() != 0 || DataEnd() != nData)
        return false;

    const Eigen::MatrixXi& Mst = m_speciation.Stoichiometry(); // components x species
    const double ln10 = std::log(10.0);

    std::vector<double> constants(nSpecies);
    for (int k = 0; k < nSpecies; ++k)
        constants[k] = std::pow(10.0, GlobalParameter(k));
    m_speciation.setStabilityConstants(constants);
    CalculateConcentrations(); // refresh m_molar_ratios (the design D) at the current beta

    // ∂(design row)/∂ln(β_j) per enabled global, D²  matrices (nData × P). Uses the analytic speciation
    // sensitivities S_i = ∂x/∂ln(β). Then combine with the *projection* derivative ∂φ/∂β so J is the
    // FULL (Golub–Pereyra) Jacobian of the projected residual - not the Kaufman (φ-fixed) one, which
    // leaves a rank-deficient Gauss–Newton Hessian and stalls the outer LM. Claude Generated.
    std::vector<Eigen::MatrixXd> dD(nG, Eigen::MatrixXd::Zero(nData, P));
    std::vector<double> totals(nComp);
    for (int i = 0; i < nData; ++i) {
        for (int c = 0; c < nComp; ++c)
            totals[c] = InitialConcentration(i, c);
        m_speciation.solve(totals, i);
        const std::vector<double>& freeConc = m_speciation.FreeConcentrations();
        const std::vector<double>& speciesConc = m_speciation.SpeciesConcentrations();
        const Eigen::MatrixXd S = m_speciation.sensitivityMatrix(); // nComp × nSpecies
        const double obs_total = totals[m_observed];
        const double s_obs = freeConc[m_observed];
        for (int jj = 0; jj < nG; ++jj) {
            const int j = gidx[jj]; // enabled global index == species index
            dD[jj](i, 0) = (s_obs / obs_total) * S(m_observed, j); // D[0] = s_obs / T_obs
            for (int k = 0; k < nSpecies; ++k) {
                double dlnck = (k == j) ? 1.0 : 0.0; // d ln c_k / d ln β_j = δ_kj + Σ_c M(c,k) S(c,j)
                for (int c = 0; c < nComp; ++c)
                    dlnck += static_cast<double>(Mst(c, k)) * S(c, j);
                dD[jj](i, 1 + k) = (static_cast<double>(Mst(m_observed, k)) / obs_total) * speciesConc[k] * dlnck;
            }
        }
    }

    const Eigen::MatrixXd& D = m_molar_ratios; // nData × P
    const Eigen::MatrixXd phi = LocalParameter()->Table(); // series × P
    const Eigen::MatrixXd Y = DependentModel()->Table(); // nData × series

    // Row map matching the residual list: SetValue appends i-major, j-minor, ONLY for active+checked
    // (i,l). Build the same mapping so J's rows line up with getCalculatedAbsoluteErrors(). CV/RA mask
    // by un-checking rows, so this is also the per-series least-squares mask of SolveLinearMasked.
    std::vector<std::vector<int>> rowOf(nData, std::vector<int>(series, -1));
    int nRows = 0;
    for (int i = 0; i < nData; ++i)
        for (int l = 0; l < series; ++l)
            if (ActiveSignals(l) && DependentModel()->isChecked(i, l))
                rowOf[i][l] = nRows++;

    J.setZero(nRows, nG);
    for (int l = 0; l < series; ++l) {
        if (!ActiveSignals(l))
            continue;
        std::vector<int> rows; // checked rows of series l (the projection + residual mask)
        for (int i = 0; i < nData; ++i)
            if (DependentModel()->isChecked(i, l))
                rows.push_back(i);
        const int na = static_cast<int>(rows.size());
        if (na == 0)
            continue;

        const Eigen::VectorXd phil = phi.row(l).transpose();
        Eigen::MatrixXd Dm(na, P); // design over the masked rows
        Eigen::VectorXd Rm(na); // residual (model − data) over the masked rows
        for (int r = 0; r < na; ++r) {
            Dm.row(r) = D.row(rows[r]);
            Rm(r) = D.row(rows[r]).dot(phil) - Y(rows[r], l);
        }
        const Eigen::LDLT<Eigen::MatrixXd> DtD = (Dm.transpose() * Dm).ldlt();

        for (int jj = 0; jj < nG; ++jj) {
            Eigen::MatrixXd dDm(na, P);
            for (int r = 0; r < na; ++r)
                dDm.row(r) = dD[jj].row(rows[r]);
            // ∂φ_l/∂ln(β_j) = −(DmᵀDm)⁻¹ [ dDmᵀ R_l + (Dmᵀ dDm) φ_l ]  over the masked rows
            const Eigen::VectorXd dphil = -DtD.solve(dDm.transpose() * Rm + (Dm.transpose() * dDm) * phil);
            for (int r = 0; r < na; ++r) {
                // ∂(model)/∂ln(β_j) = dDm φ_l + Dm ∂φ_l/∂ln(β_j); residual = model − data, ×ln10 for log10.
                const double dmodel = dDm.row(r).dot(phil) + Dm.row(r).dot(dphil);
                J(rowOf[rows[r]][l], jj) = ln10 * dmodel;
            }
        }
    }
    return true;
}

void nmr_any_Model::CalculateVariables()
{
    CalculateConcentrations();
    // Model signal = observed-component mole fractions weighted by the per-species chemical shifts.
    Eigen::MatrixXd m = m_molar_ratios * LocalParameter()->Table().transpose();
    for (int i = 0; i < DataPoints(); ++i)
        for (int j = 0; j < SeriesCount(); ++j)
            SetValue(i, j, m(i, j));
}

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
}

QString nmr_any_Model::ParameterComment(int parameter) const
{
    const ReactionSystem& sys = m_speciation.System();
    if (parameter >= 0 && parameter < sys.species.size()) {
        const Eigen::VectorXi& v = sys.species[parameter].stoich;
        QStringList lhs;
        for (int c = 0; c < sys.components.size() && c < v.size(); ++c) {
            if (v(c) <= 0)
                continue;
            lhs << (v(c) == 1 ? sys.components[c] : QString("%1 %2").arg(v(c)).arg(sys.components[c]));
        }
        const bool selfAgg = lhs.size() == 1; // a single component on the left -> self-aggregation
        return QString("%1: %2 &#8652; %3")
            .arg(selfAgg ? "Self-aggregation" : "Reaction")
            .arg(lhs.join(" + "))
            .arg(sys.species[parameter].label);
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
