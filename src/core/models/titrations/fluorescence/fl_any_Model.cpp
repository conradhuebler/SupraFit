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
#include "src/core/models/postprocess/thermo.h"

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

    m_species_count = nSpecies;
    m_global_names << QString("lg K%1").arg(QStringLiteral("(SV)")); // dynamic quenching, fitted only if the option asks for it
    m_global_parametersize = nSpecies + 1;
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
    for (int k = 0; k < nSpecies; ++k)
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
    (*GlobalTable())[nSpecies] = 3; // lg K_SV: a quencher of 1e3 M^-1, only used if the option asks for it
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
    if (Quencher() >= 0)
        addGlobalParameter(nSpecies);
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

    // The signal factor of every point (inner filter, quenching) scales the rows of the design matrix: the signal stays linear in the coefficients.
    m_signal_factor = Eigen::VectorXd::Ones(DataPoints());
    if (InnerFilterOn()) {
        // absorbance of the sample at excitation and emission: sum over the components of absorptivity x total concentration x path length
        // (the absorbance of a complex is taken as the sum of its components, see docs/fluorescence_models.md)
        const double path = getSystemParameter(PathLength).Double();
        for (int i = 0; i < DataPoints(); ++i) {
            double absorbance = 0;
            for (int c = 0; c < nComp; ++c)
                absorbance += (getSystemParameter(EpsilonExcitation + c).Double() + getSystemParameter(EpsilonEmission + c).Double()) * InitialConcentration(i, c);
            m_signal_factor[i] *= std::pow(10.0, -0.5 * path * absorbance);
        }
    }
    const int quencher = Quencher();
    if (quencher >= 0) {
        const double ksv = std::pow(10.0, QuenchingLg());
        for (int i = 0; i < DataPoints(); ++i)
            m_signal_factor[i] /= 1.0 + ksv * m_concentrations(i, quencher);
    }
    m_design = m_signal_factor.asDiagonal() * m_concentrations;
}

void fl_any_Model::ProjectLinearParameters()
{
    // VarPro linear projection: build the design matrix for the current constants, then solve the
    // fluorescence coefficients by masked least-squares (honours the active-point mask for correct
    // CV/RA, unlike the unmasked initial-guess UpdateShifts()). Claude Generated.
    CalculateConcentrations();
    SolveLinearMasked(m_design);
}

void fl_any_Model::UpdateShifts()
{
    CalculateConcentrations();
    // Fit the fluorescence coefficients (local parameters) linearly to the signal given the absolute
    // species concentrations (design matrix).
    Eigen::MatrixXd dep = DependentModel()->Table();
    Eigen::MatrixXd x = m_design.colPivHouseholderQr().solve(dep);
    LocalParameter()->setTable(x.transpose());
}

void fl_any_Model::CalculateVariables()
{
    CalculateConcentrations();
    if (LocalParameter()->Table().cols() != m_design.cols() || LocalParameter()->Table().rows() < SeriesCount()) {
        m_corrupt = true; // the coefficients of a file with another layout (the fixed models before the rebuild): AdaptStoredModel says so
        for (int i = 0; i < DataPoints(); ++i)
            for (int j = 0; j < SeriesCount(); ++j)
                SetValue(i, j, 0);
        return;
    }
    // Signal = sum over free components and species of concentration * fluorescence coefficient, times the signal factor of the point.
    Eigen::MatrixXd m = m_design * LocalParameter()->Table().transpose();
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

BC50::ModelSystem fl_any_Model::BC50System() const
{
    BC50::ModelSystem sys;
    sys.stoich = m_speciation.Stoichiometry();
    sys.lgBeta = GlobalParameterVector().mid(0, m_species_count); // not lg K_SV
    return sys;
}

QString fl_any_Model::AdditionalOutput() const
{
    QString result;
    return result;
}

QString fl_any_Model::ParameterComment(int parameter) const
{
    if (parameter == m_species_count)
        return tr("Dynamic quenching: F0/F = 1 + K<sub>SV</sub> [Q] (Stern-Volmer), K<sub>SV</sub> in L/mol; fitted only if the option \"Dynamic quenching\" names a component. AI-implemented.");
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

void fl_any_Model::DeclareOptions()
{
    AbstractTitrationModel::DeclareOptions();
    // AI-implemented (Claude, 2026), see docs/fluorescence_models.md
    addOption(InnerFilter, tr("Inner filter correction (AI-implemented)"), QStringList() << "off" << "on");
    setOption(InnerFilter, "off");
    QStringList quencher;
    quencher << "none";
    for (int c = 0; c < m_component_count; ++c)
        quencher << (c < m_component_names.size() ? m_component_names[c] : QString(QChar('A' + c)));
    addOption(Quenching, tr("Dynamic quenching by free (AI-implemented)"), quencher);
    setOption(Quenching, "none");
}

void fl_any_Model::DeclareSystemParameter()
{
    AbstractTitrationModel::DeclareSystemParameter();
    // inputs of the inner filter correction; 0 = the component does not absorb (a value that is already set, e.g. from a file, stays)
    auto declare = [this](int index, const QString& name, const QString& description, double value) {
        if (getSystemParameterList().contains(index))
            return;
        addSystemParameter(index, name, description, SystemParameter::Scalar);
        setSystemParameterValue(index, value);
    };
    declare(PathLength, tr("Path length"), tr("Path length of the cuvette in cm (inner filter correction)"), 1.0);
    for (int c = 0; c < m_component_count; ++c) {
        const QString comp = c < m_component_names.size() ? m_component_names[c] : QString(QChar('A' + c));
        declare(EpsilonExcitation + c, tr("%1 absorptivity (excitation)").arg(comp), tr("Molar absorptivity of %1 at the excitation wavelength in L/(mol cm) (inner filter correction)").arg(comp), 0.0);
        declare(EpsilonEmission + c, tr("%1 absorptivity (emission)").arg(comp), tr("Molar absorptivity of %1 at the emission wavelength in L/(mol cm) (inner filter correction)").arg(comp), 0.0);
    }
}

int fl_any_Model::Quencher() const
{
    const QStringList values = getSingleOptionValues(Quenching);
    return values.indexOf(getOption(Quenching)) - 1; // "none" is the first value
}

QString fl_any_Model::ModelInfo() const
{
    QString result;
    result += tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += "<h4>... without statistical data ...</h4>";
    for (int i = 0; i < m_species_count; ++i) {
        result += tr("<p>%1</p>").arg(ParameterComment(i));
        result += Thermo::FormatThermo(GlobalParameter(i), getT());
    }
    if (Quencher() >= 0)
        result += tr("<p>%1</p><p>K<sub>SV</sub> = %2 L/mol</p>").arg(ParameterComment(m_species_count)).arg(std::pow(10.0, QuenchingLg()), 0, 'g', 4);
    if (InnerFilterOn() || Quencher() >= 0)
        result += tr("<p><i>The inner filter and quenching terms of this model are AI-implemented (Claude); the equations are in docs/fluorescence_models.md and have not been checked against the literature by the operator yet.</i></p>");
    result += CitationBlock();
    const BC50::ModelSystem sys = BC50System();
    result += BC50::Format_FromSpeciation(sys.stoich, sys.lgBeta);
    return result;
}

void fl_any_Model::DefinePreset(const QString& reactions)
{
    QJsonObject descriptor;
    descriptor["name"] = QStringLiteral("Reactions");
    descriptor["type"] = 6;
    descriptor["value"] = reactions;
    QHash<QString, QJsonObject> elements;
    elements[QStringLiteral("Reactions")] = descriptor;
    setModelDefinition(elements);
    m_preset = true;
    AbstractModel::DefineModel(QJsonObject());
}

void fl_any_Model::AdaptStoredModel(const QJsonObject& block)
{
    Q_UNUSED(block)
    const int locals = m_species_count + m_component_count;
    const int series = SeriesCount();
    const bool globalsFit = GlobalTable() && GlobalTable()->columnCount() >= m_species_count;
    if (LocalParameter()->Table().cols() == locals && GlobalTable()->columnCount() == GlobalParameterSize())
        return;
    if (globalsFit && LocalParameter()->Table().cols() == locals) {
        // a file before the quenching constant: the table gets its column, lg K_SV starts at its default
        Eigen::MatrixXd table = Eigen::MatrixXd::Zero(1, GlobalParameterSize());
        Eigen::MatrixXd checked = Eigen::MatrixXd::Ones(1, GlobalParameterSize());
        const Eigen::MatrixXd old = GlobalTable()->Table();
        const Eigen::MatrixXd oldChecked = GlobalTable()->CheckedTable();
        for (int k = 0; k < old.cols() && k < table.cols(); ++k) {
            table(0, k) = old(0, k);
            if (k < oldChecked.cols())
                checked(0, k) = oldChecked(0, k);
        }
        table(0, m_species_count) = 3;
        GlobalTable()->setTable(table);
        GlobalTable()->setCheckedTable(checked);
        Calculate();
        return;
    }
    // another layout (the fixed fluorescence models before they became presets of this one): the numbers of the file do not apply
    emit Info()->Warning(tr("The parameters stored for this fluorescence model have another layout (an older SupraFit) and cannot be used; they are reset to the initial guess. Fit the model again."));
    Eigen::MatrixXd globals = Eigen::MatrixXd::Zero(1, GlobalParameterSize()), locs = Eigen::MatrixXd::Zero(series, locals);
    GlobalTable()->setTable(globals);
    GlobalTable()->setCheckedTable(Eigen::MatrixXd::Ones(1, GlobalParameterSize()));
    LocalParameter()->setTable(locs);
    LocalParameter()->setCheckedTable(Eigen::MatrixXd::Ones(series, locals));
    InitialGuess();
}

#include "fl_any_Model.moc"