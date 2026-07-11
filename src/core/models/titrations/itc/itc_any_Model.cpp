/*
 * SupraFit - flexible ITC model (arbitrary equilibrium species via BFGS speciation)
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
 * ITC derives the host/guest totals from the cell/syringe protocol (2 components), so unlike
 * nmr_any/uvvis_any it stays 2-component; the shared SpeciationEngine still lets it use an arbitrary
 * species list (self-aggregation, higher complexes) via the BFGS solver. The incremental-heat model
 * (sequential injection loop, dilution, stoichiometry factor fx) is unchanged. Classic MaxA/MaxB
 * remain a backward-compatible fallback. Claude Generated.
 */

#include "src/core/models/postprocess/statistic.h"

#include "src/core/models/titrations/AbstractItcModel.h"
#include "src/core/models/titrations/AbstractTitrationModel.h" // Reactions/MaxA/MaxB JSON templates

#include "src/core/bc50.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/reactionparser.h"
#include "src/core/toolset.h"

#include <QtMath>

#include <QDebug>
#include <QtCore/QJsonObject>
#include <cmath>

#include "itc_any_Model.h"

itc_any_Model::itc_any_Model(DataClass* data)
    : AbstractItcModel(data)
{
    m_pre_input = { Reactions_Json, MaxA_Json, MaxB_Json, MaxSelfA_Json, Species_Json };
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
}

ReactionSystem itc_any_Model::buildLegacySystem() const
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
                continue;
            add(a, b);
        }
    } else {
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

bool itc_any_Model::DefineModel()
{
    m_maxA = m_defined_model.value("MaxA")["value"].toInt();
    m_maxB = m_defined_model.value("MaxB")["value"].toInt();
    m_maxSelfA = m_defined_model.value("MaxSelfA")["value"].toInt();

    // Only a 2-component reaction system is meaningful for ITC (host + guest totals from the
    // protocol); a larger system falls back to the classic grid.
    bool reactionMode = false;
    const QString reactions = m_defined_model.value("Reactions")["value"].toString().trimmed();
    if (!reactions.isEmpty()) {
        ReactionSystem sys = ReactionParser::Parse(reactions);
        if (sys.valid && sys.components.size() <= 2) {
            m_speciation.setSystem(sys);
            reactionMode = true;
        }
    }
    if (!reactionMode)
        m_speciation.setSystem(buildLegacySystem());
    m_speciation.setMaxIter(1000);
    m_speciation.setConvergeThreshold(1e-12);

    const ReactionSystem& sys = m_speciation.System();
    const int nSpecies = sys.species.size();

    m_global_names.clear();
    m_species_names.clear();
    m_local_names.clear();
    for (int k = 0; k < nSpecies; ++k) {
        const QString sname = sys.species[k].label;
        m_species_names << sname;
        m_global_names << QString("lg %1(%2)").arg(Unicode_beta).arg(sname);
        m_local_names << QString("%1 H (%2)").arg(Unicode_delta).arg(sname);
        addOption(Dilution + 1 + k, sname, QStringList() << "yes" << "no");
    }
    m_local_names << "m (solv H)"
                  << "n (solv H)"
                  << "fx";

    m_global_parametersize = nSpecies;
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
    for (int k = 0; k < GlobalParameterSize(); ++k)
        setOption(Dilution + 1 + k, "yes");

    CollectOptimizationParameters_Private();
    m_complete = true;
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

void itc_any_Model::CollectOptimizationParameters_Private()
{
    const int nSpecies = m_speciation.SpeciesCount();
    for (int k = 0; k < nSpecies; ++k) {
        if (getOption(Dilution + 1 + k) == "yes") {
            addGlobalParameter(k);
            addLocalParameter(k);
        }
    }
    QString dilution = getOption(Dilution);
    if (dilution == "auto") {
        addLocalParameter(nSpecies);
        addLocalParameter(nSpecies + 1);
    }
    addLocalParameter(nSpecies + 2);
}

void itc_any_Model::CalculateVariables()
{
    const int nSpecies = m_speciation.SpeciesCount();
    const ReactionSystem& sys = m_speciation.System();

    // linear stability constants and per-species reaction enthalpies (deactivated species -> 0)
    std::vector<double> constants(nSpecies);
    Vector heats(nSpecies);
    for (int k = 0; k < nSpecies; ++k) {
        if (getOption(Dilution + 1 + k) == "yes") {
            constants[k] = pow(10, GlobalParameter(k));
            heats(k) = LocalTable()->data(0, k);
        } else {
            constants[k] = 0;
            heats(k) = 0;
        }
    }
    m_speciation.setStabilityConstants(constants);

    QString more_info = QString("Inject\t" + qAB + "\t" + qsolv + "\t" + q + "\n");
    QString dil = getOption(Dilution);

    const qreal dil_heat = LocalTable()->data(0, nSpecies);
    const qreal dil_inter = LocalTable()->data(0, nSpecies + 1);
    const qreal fx = LocalTable()->data(0, nSpecies + 2);
    qreal V = m_V;
    bool reservior = m_reservior;

    Vector vector_prev(nSpecies + 3);
    vector_prev.setZero();

    /* The incremental heat of injection i depends on the concentrations of the previous point, so
     * the loop MUST run over all data points in sequence. */
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i) * fx;
        qreal guest_0 = InitialGuestConcentration(i);

        m_speciation.solve({ host_0, guest_0 });
        const std::vector<double>& freeConc = m_speciation.FreeConcentrations();
        const std::vector<double>& speciesConc = m_speciation.SpeciesConcentrations();
        const double host = freeConc.empty() ? 0.0 : freeConc[0];
        const double guest = freeConc.size() > 1 ? freeConc[1] : 0.0;

        // stored vector: [idx, free host, free guest, hostBound_0 .. hostBound_{m-1}]
        // hostBound_k = M(host,k) * [species_k] preserves the previous a*[complex] convention.
        Vector vector(nSpecies + 3);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        for (int k = 0; k < nSpecies; ++k) {
            const int hostCoeff = sys.species[k].stoich(0); // component 0 = host A
            vector(3 + k) = hostCoeff * speciesConc[k];
        }

        qreal dilution = 0;
        qreal v = IndependentModel()->data(i);
        if (dil == "auto")
            dilution = (guest_0 * dil_heat + dil_inter);
        V += IndependentModel()->data(i) * !reservior;
        qreal dv = (1 - v / V);

        Vector qvector(nSpecies + 2);
        double value = 0;
        for (int species = 3; species < vector.size(); ++species) {
            const double q_species = V * (vector(species) - vector_prev(species) * dv) * heats(species - 3);
            value += q_species;
            qvector(species - 3) = q_species;
        }
        qvector(nSpecies) = dilution;
        qvector(nSpecies + 1) = value + dilution;

        QString more = QString();
        more += Print::printDouble(PrintOutIndependent(i)) + "\t";
        for (double& d : qvector)
            more += Print::printDouble(d) + "\t";
        more_info += more + "\n";

        bool usage = SetValue(i, AppliedSeries(), value + dilution);

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
}

QSharedPointer<AbstractModel> itc_any_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_any_Model>(new itc_any_Model(this), &QObject::deleteLater);
    finishClone(model, statistics);
    model.data()->setConcentrations(ConcentrationTable());
    return std::move(model);
}

QString itc_any_Model::AdditionalOutput() const
{
    QString result = tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += "<h4>without statistical data:</h4>";

    auto conf2therm = [&result, this](const QJsonObject& object = QJsonObject()) {
        result += tr("<p>%1</p>").arg(ParameterComment(0));
        result += Statistic::MonteCarlo2Thermo(0, getT(), object);
    };

    conf2therm();
    return result;
}

QString itc_any_Model::ModelInfo() const
{
    QString result = AbstractItcModel::ModelInfo();
    result += BC50::ItoI::Format_BC50(GlobalParameter(0));
    result += CitationBlock();

    return result;
}

QString itc_any_Model::ParameterComment(int parameter) const
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

QString itc_any_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{
    return prependBC50(AbstractItcModel::AnalyseMonteCarlo(object, forceAll), forceAll, Statistic::MonteCarlo2BC50_1(GlobalParameter(0), object));
}

QString itc_any_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{
    return prependBC50(AbstractItcModel::AnalyseGridSearch(object, forceAll), forceAll, Statistic::GridSearch2BC50_1(GlobalParameter(0), object));
}

#include "itc_any_Model.moc"
