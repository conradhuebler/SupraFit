/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "AbstractNMRModel.h"

AbstractNMRModel::AbstractNMRModel(DataClass* data)
    : AbstractModel(data)
{
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "Host (A)", Qt::DisplayRole);
    IndependentModel()->setHeaderData(1, Qt::Horizontal, "Guest (B)", Qt::DisplayRole);
    m_ylabel = "&delta; /ppm";
    LoadSystemParameter();
    connect(this, &AbstractModel::Recalculated, this, [this]() {
        emit this->ChartUpdated("Concentration Chart");
    });
}

AbstractNMRModel::AbstractNMRModel(AbstractNMRModel* other)
    : AbstractModel(other)
{
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "Host (A)", Qt::DisplayRole);
    IndependentModel()->setHeaderData(1, Qt::Horizontal, "Guest (B)", Qt::DisplayRole);
    m_ylabel = "&delta; /ppm";
    m_T = other->m_T;
    connect(this, &AbstractModel::Recalculated, this, [this]() {
        emit this->ChartUpdated("Concentration Chart");
    });
}

AbstractNMRModel::~AbstractNMRModel()
{
}

void AbstractNMRModel::DeclareSystemParameter()
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
}

void AbstractNMRModel::UpdateParameter()
{
    m_T = getSystemParameter(Temperature).Double();
    m_plotMode = getSystemParameter(PlotMode).getString();
    UpdateChart("concentration", m_plotMode, "c [mol/L]");
}

void AbstractNMRModel::DeclareOptions()
{
    QStringList host = QStringList() << "yes"
                                     << "no";
    addOption(Host, "Fix Host Signal", host);
    setOption(Host, "no");

    QStringList method = QStringList() << "NMR"
                                       << "UV/VIS";
    addOption(Method, "Method", QStringList());
    setOption(Method, "NMR");
}

void AbstractNMRModel::EvaluateOptions()
{
    if (getOption(Method) == "UV/VIS") {
        m_ylabel = "I";
        m_localParameterSuffix = "";
        m_localParameterName = Unicode_epsilion;
        m_localParameterDescription = "Absorption";
        emit DataClass::Message("UV/VIS Model option in NMR Models is only available to maintain compatibility with older SupraFit projects. Please use the UV/VIS Models instead!", 0);
    } else {
        m_ylabel = "&delta; [ppm]";
        m_localParameterSuffix = " ppm";
        m_localParameterName = Unicode_delta;
        m_localParameterDescription = "chemical shift";
    }
}

void AbstractNMRModel::SetConcentration(int i, const Vector& equilibrium)
{
    if (!m_concentrations) {
        m_concentrations = new DataTable(equilibrium.rows(), DataPoints(), this);
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

MassResults AbstractNMRModel::MassBalance(qreal A, qreal B)
{
    Q_UNUSED(A)
    Q_UNUSED(B)
    MassResults result;
    Vector values(1);
    values(0) = 0;
    result.MassBalance = values;
    return result;
}

QString AbstractNMRModel::Model2Text_Private() const
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

qreal AbstractNMRModel::PrintOutIndependent(int i) const
{
    QString plotmode = getPlotMode();

    if (plotmode == m_plotmode[0])
        return InitialGuestConcentration(i) / InitialHostConcentration(i);
    else if (plotmode == m_plotmode[1])
        return InitialGuestConcentration(i);
    else
        return i;
}

QString AbstractNMRModel::ModelInfo() const
{
    QString result;

    result += tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += "<h4>... without statistical data ...</h4>";

    for (int i = 0; i < GlobalParameterSize(); ++i) {
        result += tr("<p>%1</p>").arg(ParameterComment(i));
        result += Thermo::FormatThermo(GlobalParameter(i), getT());
    }

    return result;
}

QString AbstractNMRModel::AdditionalOutput() const
{
    QString result = ModelInfo() + "\n";
    result += "Idividual contribution to the signal";

    for (int i = 0; i < SeriesCount(); ++i) {
        result += QString("Series %1 ... Individual contributions ...\n").arg(i + 1);
        for (int j = 0; j < DataPoints(); ++j) {
            QVector<qreal> vector = DeCompose(j, i);
            result += QString("%1\t%2").arg(j + 1).arg(ModelTable()->data(i, j));
            for (int k = 0; k < vector.size(); ++k)
                result += QString("\t%1").arg(vector[k]);
            result += "\n";
        }
        result += QString("Series %1 ... Individual contributions done\n\n\n").arg(i);
    }
    result += Statistic::PseudoANOVA(this);
    return result;
}

QString AbstractNMRModel::AnalyseStatistic(bool forceAll) const
{
    QString result;

    result += tr("<h4>Thermodynamic Output for T = %1 K:</h4>").arg(getT());
    result += AbstractModel::AnalyseStatistic(forceAll);

    return result;
}

QString AbstractNMRModel::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
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

QString AbstractNMRModel::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
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
QVector<QJsonObject> AbstractNMRModel::PostGridSearch(const QList<QJsonObject> &models) const
{

}*/

qreal AbstractNMRModel::InitialGuestConcentration(int i) const
{
#pragma message("have a look at here, while restructureing stuff, make that NMR stuff specific, by JSON?")
    return d->m_independent_model->data(!HostAssignment(), i) * d->m_scaling[!HostAssignment()];
}

qreal AbstractNMRModel::InitialHostConcentration(int i) const
{
#pragma message("have a look at here, while restructureing stuff, make that NMR stuff specific, by JSON?")
    return d->m_independent_model->data(HostAssignment(), i) * d->m_scaling[HostAssignment()];
}

qreal AbstractNMRModel::GuessK(int index)
{

    QSharedPointer<AbstractModel> test = Clone();
    qreal K = BisectParameter(test, index, 1, 5);
    return K;
}

#include "AbstractNMRModel.moc"
