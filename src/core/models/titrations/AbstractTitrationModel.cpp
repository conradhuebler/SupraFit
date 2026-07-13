/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/models/postprocess/thermo.h"

#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "src/core/models/dataclass.h"

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include <cfloat>
#include <cmath>
#include <iostream>

#include <libpeakpick/mathhelper.h>
#include <libpeakpick/peakpick.h>

#include "AbstractTitrationModel.h"

AbstractTitrationModel::AbstractTitrationModel(DataClass* data)
    : AbstractModel(data)
{
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "Host (A)", Qt::DisplayRole);
    IndependentModel()->setHeaderData(1, Qt::Horizontal, "Guest (B)", Qt::DisplayRole);
    LoadSystemParameter();
    connect(this, &AbstractModel::Recalculated, this, [this]() {
        emit this->ChartUpdated("Concentration Chart");
    });
}

AbstractTitrationModel::AbstractTitrationModel(AbstractTitrationModel* other)
    : AbstractModel(other)
{
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "Host (A)", Qt::DisplayRole);
    IndependentModel()->setHeaderData(1, Qt::Horizontal, "Guest (B)", Qt::DisplayRole);
    m_T = other->m_T;
    connect(this, &AbstractModel::Recalculated, this, [this]() {
        emit this->ChartUpdated("Concentration Chart");
    });
}

AbstractTitrationModel::~AbstractTitrationModel()
{
}

void AbstractTitrationModel::DeclareSystemParameter()
{
    const QString sub_char = QChar(0x2080);

    addSystemParameter(Temperature, "Temperature", "Temperature in K", SystemParameter::Scalar);
    setSystemParameterValue(Temperature, 298);

    addSystemParameter(PlotMode, "Plot Mode", "x-Axis Plot Mode", SystemParameter::List);
    m_plotmode = QStringList() << QString("[G%1]/[H%2]").arg(sub_char).arg(sub_char)
                               << QString("[G%1]").arg(sub_char)
                               << "Number";
    setSystemParameterList(PlotMode, m_plotmode);
    setSystemParameterValue(PlotMode, m_plotmode[0]);

    addSystemParameter(HostGuestAssignment, "Host Assignment", "Assign host concentration to", SystemParameter::List);
    m_HostAssignmentList = QStringList() << "First column"
                                         << "Second column";
    setSystemParameterList(HostGuestAssignment, m_HostAssignmentList);
    setSystemParameterValue(HostGuestAssignment, m_HostAssignmentList[0]);
}

void AbstractTitrationModel::UpdateParameter()
{
    m_T = getSystemParameter(Temperature).Double();
    m_plotMode = getSystemParameter(PlotMode).getString();
    m_HostAssignment = m_HostAssignmentList.indexOf(getSystemParameter(HostGuestAssignment).getString());
    UpdateChart("concentration", m_plotMode, "c [mol/L]");
}

void AbstractTitrationModel::DeclareOptions()
{
    QStringList host = QStringList() << "yes"
                                     << "no";
    addOption(Host, "Silent Host", host);
    setOption(Host, "no");

    QStringList guest = QStringList() << "yes"
                                      << "no";
    addOption(Guest, "Silent Guest", host);
    setOption(Guest, "yes");
}

void AbstractTitrationModel::EvaluateOptions()
{
}

QPair<bool, bool> AbstractTitrationModel::getHostGuestPair() const
{
    bool host = getOption(Host) == "no";
    bool guest = getOption(Guest) == "no";
    return QPair<bool, bool>(host, guest);
}

void AbstractTitrationModel::SetConcentration(int i, const Vector& equilibrium)
{
    if (!m_concentrations) {
        m_concentrations = new DataTable(DataPoints(), equilibrium.rows(), this);
        m_concentrations->setHeaderData(0, Qt::Horizontal, "Exp.", Qt::DisplayRole);
        m_concentrations->setHeaderData(1, Qt::Horizontal, "Host (A)", Qt::DisplayRole);
        m_concentrations->setHeaderData(2, Qt::Horizontal, "Guest (B)", Qt::DisplayRole);
        for (int i = 0; i < GlobalParameterSize(); ++i)
            m_concentrations->setHeaderData(3 + i, Qt::Horizontal, SpeciesName(i), Qt::DisplayRole);
    }

    m_concentrations->setRow(equilibrium, i);
    QStringList names = m_concentrations->header();
    names.removeFirst();
    addPoints("Concentration Chart", PrintOutIndependent(i), equilibrium.tail(equilibrium.size() - 1), names);
    UpdateChart("Concentration Chart", m_plotMode, "c [mol/L]");
}

MassResults AbstractTitrationModel::MassBalance(qreal A, qreal B)
{
    Q_UNUSED(A)
    Q_UNUSED(B)
    MassResults result;
    Vector values(1);
    values(0) = 0;
    result.MassBalance = values;
    return result;
}

QString AbstractTitrationModel::Model2Text_Private() const
{
    QString text;
    if (m_concentrations) {
        text += "Equilibrium concentration calculated with complexation constants:\n";
        for (int i = 0; i < getConcentrations()->columnCount(); ++i) {
            text += " " + getConcentrations()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
        }
        text += "\n";
        text += getConcentrations()->ExportAsString();
        text += "\n\n";
        text += "Equilibrium Model Signal calculated with complexation constants:\n";
        for (int i = 0; i < DependentModel()->columnCount(); ++i)
            text += " " + DependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    }
    return text;
}

qreal AbstractTitrationModel::PrintOutIndependent(int i) const
{
    QString plotmode = getPlotMode();

    if (plotmode == m_plotmode[0])
        return InitialGuestConcentration(i) / InitialHostConcentration(i);
    else if (plotmode == m_plotmode[1])
        return InitialGuestConcentration(i);
    else
        return i;
}

QString AbstractTitrationModel::ModelInfo() const
{
    QString result;

    result += tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += "<h4>... without statistical data ...</h4>";

    for (int i = 0; i < GlobalParameterSize(); ++i) {
        result += tr("<p>%1</p>").arg(ParameterComment(i));
        result += Thermo::FormatThermo(GlobalParameter(i), getT());
    }

    result += CitationBlock();
    return result;
}

QString AbstractTitrationModel::AdditionalOutput() const
{
    QString result = ModelInfo() + "\n";
    result += "Individual contributions to the signal";

    for (int i = 0; i < SeriesCount(); ++i) {
        result += QString("Series %1 ... Individual contributions ...\n").arg(i + 1);
        for (int j = 0; j < DataPoints(); ++j) {
            QVector<qreal> vector = DeCompose(j, i);
            result += QString("%1\t%2").arg(j + 1).arg(ModelTable()->data(j, i));
            for (int k = 0; k < vector.size(); ++k)
                result += QString("\t%1").arg(vector[k]);
            result += "\n";
        }
        result += QString("Series %1 ... Individual contributions done\n\n\n").arg(i);
    }
    result += Statistic::PseudoANOVA(this);
    return result;
}

QString AbstractTitrationModel::AnalyseStatistic(bool forceAll) const
{
    QString result;

    result += tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += AbstractModel::AnalyseStatistic(forceAll);

    return result;
}

QString AbstractTitrationModel::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{

    QString result;

    result += tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());

    auto conf2therm = [&result, this](int i, const QJsonObject& object = QJsonObject()) {
        result += tr("<p>%1</p>").arg(ParameterComment(i));
        result += Statistic::MonteCarlo2Thermo(i, getT(), object, false);
    };

    for (int i = 0; i < GlobalParameterSize(); ++i)
        conf2therm(i, object);

    result += AbstractModel::AnalyseMonteCarlo(object, forceAll);

    return result;
}

QString AbstractTitrationModel::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{

    QString result;

    result += tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());

    auto conf2therm = [&result, this](int i, const QJsonObject& object = QJsonObject()) {
        result += tr("<p>%1</p>").arg(ParameterComment(i));
        result += Statistic::GridSearch2Thermo(i, getT(), object, false);
    };

    for (int i = 0; i < GlobalParameterSize(); ++i)
        conf2therm(i, object);

    result += AbstractModel::AnalyseGridSearch(object, forceAll);

    return result;
}

/*
QVector<QJsonObject> AbstractTitrationModel::PostGridSearch(const QList<QJsonObject> &models) const
{

}*/

qreal AbstractTitrationModel::InitialGuestConcentration(int i) const
{
    return IndependentModel()->data(i, !m_HostAssignment);
}

qreal AbstractTitrationModel::InitialHostConcentration(int i) const
{
    return IndependentModel()->data(i, m_HostAssignment);
}

qreal AbstractTitrationModel::InitialConcentration(int i, int component) const
{
    // Two-component systems keep the classic host/guest column assignment; an N-component reaction
    // system maps component index straight to the independent column (order = first appearance). CG.
    if (m_component_count <= 2)
        return (component == 0) ? InitialHostConcentration(i) : InitialGuestConcentration(i);
    return IndependentModel()->data(i, component);
}

bool AbstractTitrationModel::BuildSpeciationFromReactions()
{
    m_reaction_component_mismatch = 0;
    const QString text = m_defined_model.value("Reactions")["value"].toString().trimmed();
    if (text.isEmpty())
        return false;
    if (!m_speciation.setReactions(text))
        return false; // parse error: caller keeps the legacy MaxA/MaxB grid

    // The data must provide one concentration column per component; a mismatch would make
    // PrepareParameter throw. Record it and fall back to the legacy grid rather than crashing; the
    // GUI reports the mismatch to the user (see ModelDataHolder::AddModel). Claude Generated.
    const int nComp = m_speciation.ComponentCount();
    if (nComp < 1 || (IndependentModel() && nComp != IndependentModel()->columnCount())) {
        m_reaction_component_mismatch = nComp;
        return false;
    }

    m_component_names = m_speciation.ComponentNames();
    m_component_count = nComp;
    m_speciation.setMaxIter(1000);
    m_speciation.setConvergeThreshold(1e-12);
    UpdateComponentHeaders();
    return true;
}

void AbstractTitrationModel::UpdateComponentHeaders()
{
    for (int c = 0; c < m_component_names.size(); ++c)
        IndependentModel()->setHeaderData(c, Qt::Horizontal, m_component_names[c], Qt::DisplayRole);
}

void AbstractTitrationModel::SolveLinearMasked(const Eigen::MatrixXd& design)
{
    const int nLinear = static_cast<int>(design.cols());
    const int series = SeriesCount();
    if (nLinear == 0 || series == 0)
        return;

    const Eigen::MatrixXd Y = DependentModel()->Table(); // DataPoints × series
    const Eigen::MatrixXd current = LocalParameter()->Table(); // SeriesCount × nLinear (kept for inactive)
    Eigen::MatrixXd L(nLinear, series); // solved locals per series (column j)

    auto keepCurrent = [&](int j) {
        for (int p = 0; p < nLinear; ++p)
            L(p, j) = (j < current.rows() && p < current.cols()) ? current(j, p) : 0.0;
    };

    for (int j = 0; j < series; ++j) {
        if (!ActiveSignals(j)) {
            keepCurrent(j);
            continue;
        }
        // Gather the active rows of this series (same mask the residual uses in SetValue).
        std::vector<int> rows;
        rows.reserve(DataEnd() - DataBegin());
        for (int i = DataBegin(); i < DataEnd(); ++i)
            if (DependentModel()->isChecked(i, j))
                rows.push_back(i);
        if (rows.empty()) {
            keepCurrent(j);
            continue;
        }
        Eigen::MatrixXd Dm(static_cast<int>(rows.size()), nLinear);
        Eigen::VectorXd ym(static_cast<int>(rows.size()));
        for (int r = 0; r < static_cast<int>(rows.size()); ++r) {
            Dm.row(r) = design.row(rows[r]);
            ym(r) = Y(rows[r], j);
        }
        // colPivHouseholderQr handles over- and (rank-deficient) under-determined systems gracefully.
        L.col(j) = Dm.colPivHouseholderQr().solve(ym);
    }
    LocalParameter()->setTable(L.transpose());
}

double AbstractTitrationModel::GuessLgBeta(int speciesIndex) const
{
    const Eigen::VectorXi stoich = m_speciation.SpeciesStoichiometry(speciesIndex);
    const int order = stoich.size() ? stoich.sum() : 2;

    // c_ref = geometric mean of the per-component maximum total concentration in the data.
    double logsum = 0.0;
    int nc = 0;
    for (int c = 0; c < m_component_count; ++c) {
        double maxTotal = 0.0;
        for (int i = DataBegin(); i < DataEnd(); ++i)
            maxTotal = std::max(maxTotal, InitialConcentration(i, c));
        if (maxTotal > 0.0) {
            logsum += std::log10(maxTotal);
            ++nc;
        }
    }
    const double logcref = nc ? logsum / nc : -3.0; // fallback ~1e-3
    return std::max(1.0, (order - 1) * (-logcref));
}

qreal AbstractTitrationModel::GuessK(int index, double min, double max)
{
    QSharedPointer<AbstractModel> test = Clone();
    qreal K = NewtonRoot(test, index, min, max);
    return K;
}

#include "AbstractTitrationModel.moc"
