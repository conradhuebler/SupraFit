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
 * The classic MaxA/MaxB/MaxSelfA/Species fields remain a backward-compatible 2-component fallback.
 * Claude Generated.
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
    m_pre_input = { Reactions_Json, MaxA_Json, MaxB_Json, MaxSelfA_Json, Species_Json };
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

ReactionSystem nmr_any_Model::buildLegacySystem() const
{
    ReactionSystem sys;
    sys.components = QStringList() << "A" << "B";

    auto add = [&sys](int a, int b) {
        Eigen::VectorXi v(2);
        v << a, b;
        ReactionSpecies species;
        species.stoich = v;
        species.label = ReactionParser::SpeciesLabel(sys.components, v);
        sys.species << species;
    };

    // Optional explicit species list "a,b|a,b|..." overrides the grid (order as written).
    const QString speciesDef = m_defined_model.value("Species")["value"].toString().trimmed();
    if (!speciesDef.isEmpty()) {
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
            add(a, b);
        }
    } else {
        // Classic A_aB_b grid first (a,b >= 1) so existing projects keep their indices, then the
        // pure host oligomers A_n (b = 0) for self-aggregation.
        const int maxA = m_maxA < 1 ? 1 : m_maxA;
        const int maxB = m_maxB < 1 ? 1 : m_maxB;
        for (int a = 1; a <= maxA; ++a)
            for (int b = 1; b <= maxB; ++b)
                add(a, b);
        for (int n = 2; n <= m_maxSelfA; ++n)
            add(n, 0);
    }

    sys.stoich = Eigen::MatrixXi(2, sys.species.size());
    for (int j = 0; j < sys.species.size(); ++j)
        sys.stoich.col(j) = sys.species[j].stoich;
    sys.valid = !sys.species.isEmpty();
    return sys;
}

bool nmr_any_Model::DefineModel()
{
    m_maxA = m_defined_model.value("MaxA")["value"].toInt();
    m_maxB = m_defined_model.value("MaxB")["value"].toInt();
    m_maxSelfA = m_defined_model.value("MaxSelfA")["value"].toInt();
    m_observed = 0; // NMR observes the first component (host A) by default

    // Prefer the N-component reaction editor; otherwise fall back to the 2-component legacy grid.
    if (!BuildSpeciationFromReactions()) {
        m_component_count = 2;
        m_component_names = QStringList() << "A" << "B";
        m_speciation.setSystem(buildLegacySystem());
        m_speciation.setMaxIter(1000);
        m_speciation.setConvergeThreshold(1e-12);
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
    const double guess_K = 4;
    for (int k = 0; k < nSpecies; ++k) {
        const Eigen::VectorXi v = m_speciation.SpeciesStoichiometry(k);
        const int order = v.sum(); // higher-order complexes start from a larger lg beta guess
        (*GlobalTable())[k] = guess_K + order;
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

        m_speciation.solve(totals);
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
