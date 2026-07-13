/*
 * SupraFit - flexible fluorescence titration model (arbitrary N-component equilibrium via BFGS speciation)
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
 * The speciation is delegated to the shared SpeciationEngine on AbstractTitrationModel; this model
 * maps the absolute concentrations to the fluorescence signal as a linear combination of per-species
 * fluorescence coefficients (Σ c · φ), fitted linearly — the same linear form the fixed
 * fl_1_1_1_2 / fl_2_1_1_1 models use, generalised to N components. The equilibrium system is defined
 * solely through the free-text Reactions field. Claude Generated.
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

#include <QtCore/QJsonObject>

#include "fl_any_Model.h"

fl_any_Model::fl_any_Model(DataClass* data)
    : AbstractTitrationModel(data)
{
    m_pre_input = { Reactions_Json };
    m_complete = false;
}

fl_any_Model::fl_any_Model(AbstractTitrationModel* data)
    : AbstractTitrationModel(data)
{
    DefineModel();

    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
}

fl_any_Model::~fl_any_Model()
{
}

QString fl_any_Model::LocalParameterName(int i) const
{
    if (i < m_component_count) {
        const QString comp = (i < m_component_names.size()) ? m_component_names[i] : QString(QChar('A' + i));
        return QString("%1 %2").arg(Unicode_Phi).arg(comp);
    }
    return QString("%1 %2").arg(Unicode_Phi).arg(SpeciesName(i - m_component_count));
}

bool fl_any_Model::DefineModel()
{
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

void fl_any_Model::InitialGuess_Private()
{
    // seed the free-component fluorescence coefficients from the first spectrum, the complexes from the last
    for (int c = 0; c < m_component_count; ++c)
        LocalTable()->setColumn(DependentModel()->firstRow(), c);

    const int nSpecies = m_speciation.SpeciesCount();
    for (int k = 0; k < nSpecies; ++k) {
        (*GlobalTable())[k] = GuessLgBeta(k); // data-derived guess (see AbstractTitrationModel)
        LocalTable()->setColumn(DependentModel()->Row(DataPoints() - 1), m_component_count + k);
    }
    UpdateShifts();
    Calculate();
}

void fl_any_Model::CollectOptimizationParameters_Private()
{
    const int nSpecies = m_speciation.SpeciesCount();
    for (int k = 0; k < nSpecies; ++k) {
        if (getOption(Host + 1 + k) == "yes") {
            addGlobalParameter(k);
            addLocalParameter(m_component_count + k);
        }
    }
    for (int c = 0; c < m_component_count; ++c)
        addLocalParameter(c);
}

void fl_any_Model::CalculateConcentrations()
{
    const int nComp = m_component_count;
    const int nSpecies = m_speciation.SpeciesCount();

    std::vector<double> constants(nSpecies);
    for (int k = 0; k < nSpecies; ++k) {
        if (getOption(Host + 1 + k) == "yes" && GlobalTable()->isChecked(0, k))
            constants[k] = pow(10, GlobalParameter(k));
        else
            constants[k] = 0;
    }
    m_speciation.setStabilityConstants(constants);

    // design matrix: absolute concentrations of free components then species
    m_concentrations = Eigen::MatrixXd(DataPoints(), nComp + nSpecies);

    std::vector<double> totals(nComp);
    for (int i = 0; i < DataPoints(); ++i) {
        for (int c = 0; c < nComp; ++c)
            totals[c] = InitialConcentration(i, c);

        m_speciation.solve(totals, i);
        const std::vector<double>& freeConc = m_speciation.FreeConcentrations();
        const std::vector<double>& speciesConc = m_speciation.SpeciesConcentrations();

        Vector vector(nComp + 1 + nSpecies);
        vector(0) = i + 1;
        for (int c = 0; c < nComp; ++c) {
            m_concentrations(i, c) = freeConc[c];
            vector(1 + c) = freeConc[c];
        }
        for (int k = 0; k < nSpecies; ++k) {
            m_concentrations(i, nComp + k) = speciesConc[k];
            vector(1 + nComp + k) = speciesConc[k];
        }
        if (!m_fast)
            SetConcentration(i, vector);
    }
}

void fl_any_Model::ProjectLinearParameters()
{
    // VarPro linear projection: build the design matrix for the current constants, then solve the
    // fluorescence coefficients by masked least-squares (honours the active-point mask for correct
    // CV/RA, unlike the unmasked initial-guess UpdateShifts()). Claude Generated.
    CalculateConcentrations();
    SolveLinearMasked(m_concentrations);
}

void fl_any_Model::UpdateShifts()
{
    CalculateConcentrations();
    // Fit the fluorescence coefficients (local parameters) linearly to the signal given the absolute
    // species concentrations (design matrix).
    Eigen::MatrixXd dep = DependentModel()->Table();
    Eigen::MatrixXd x = m_concentrations.colPivHouseholderQr().solve(dep);
    LocalParameter()->setTable(x.transpose());
}

void fl_any_Model::CalculateVariables()
{
    CalculateConcentrations();
    // Signal = sum over free components and species of concentration * fluorescence coefficient.
    Eigen::MatrixXd m = m_concentrations * LocalParameter()->Table().transpose();
    for (int i = 0; i < DataPoints(); ++i)
        for (int j = 0; j < SeriesCount(); ++j)
            SetValue(i, j, m(i, j));
}

QSharedPointer<AbstractModel> fl_any_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractModel> model = QSharedPointer<fl_any_Model>(new fl_any_Model(this), &QObject::deleteLater);
    finishClone(model, statistics);
    return model;
}

QString fl_any_Model::ModelInfo() const
{
    QString result = AbstractTitrationModel::ModelInfo();
    result += BC50::ItoI::Format_BC50(GlobalParameter(0));

    return result;
}

QString fl_any_Model::AdditionalOutput() const
{
    QString result;
    return result;
}

QString fl_any_Model::ParameterComment(int parameter) const
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
        const bool selfAgg = lhs.size() == 1;
        return QString("%1: %2 &#8652; %3")
            .arg(selfAgg ? "Self-aggregation" : "Reaction")
            .arg(lhs.join(" + "))
            .arg(sys.species[parameter].label);
    }
    return QString("Reaction: A + B &#8652; AB");
}

QString fl_any_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{
    return prependBC50(AbstractTitrationModel::AnalyseMonteCarlo(object, forceAll), forceAll, Statistic::MonteCarlo2BC50_1(GlobalParameter(0), object));
}

QString fl_any_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{
    return prependBC50(AbstractTitrationModel::AnalyseGridSearch(object, forceAll), forceAll, Statistic::GridSearch2BC50_1(GlobalParameter(0), object));
}

#include "fl_any_Model.moc"