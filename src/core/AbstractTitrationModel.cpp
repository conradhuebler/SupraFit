/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "libmath.h"
#include "src/core/dataclass.h"
#include "src/core/toolset.h"

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
    m_ylabel = "&delta; [ppm]";
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
    m_ylabel = "&delta; [ppm]";
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
    addSystemParameter(Temperature, "Temperature", "Temperature in K", SystemParameter::Scalar);
    setSystemParameterValue(Temperature, 298);

    addSystemParameter(PlotMode, "Plot Mode", "x-Axis Plot Mode", SystemParameter::List);
    QStringList plotmode = QStringList() << "[G<sub>0</sub>]/[H<sub>0</sub>]"
                                         << "[G<sub>0</sub>]"
                                         << "Number";
    setSystemParameterList(PlotMode, plotmode);
    setSystemParameterValue(PlotMode, "[G<sub>0</sub>]/[H<sub>0</sub>]");
}

void AbstractTitrationModel::UpdateParameter()
{
    m_T = getSystemParameter(Temperature).Double();
    m_plotMode = getSystemParameter(PlotMode).getString();
    UpdateChart("concentration", m_plotMode, "c [mol/L]");
}

void AbstractTitrationModel::DeclareOptions()
{
    QStringList host = QStringList() << "yes"
                                     << "no";
    addOption(Host, "Fix Host Signal", host);
    setOption(Host, "no");

    QStringList method = QStringList() << "NMR"
                                       << "UV/VIS";
    addOption(Method, "Method", method);
    setOption(Method, "NMR");
}

void AbstractTitrationModel::EvaluateOptions()
{
    if (getOption(Method) == "UV/VIS")
        m_ylabel = "I";
    else
        m_ylabel = "&delta; [ppm]";
}

void AbstractTitrationModel::SetConcentration(int i, const Vector& equilibrium)
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

    if (plotmode == "[G<sub>0</sub>]/[H<sub>0</sub>]")
        return InitialGuestConcentration(i) / InitialHostConcentration(i);
    else if (plotmode == "[G<sub>0</sub>]")
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

    return result;
}

QString AbstractTitrationModel::AdditionalOutput() const
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

qreal AbstractTitrationModel::Guess_1_1() const
{
    qreal K11 = 0;
    QVector<qreal> x;
    QVector<QVector<qreal>> y(SeriesCount());
    for (int i = 1; i < DataPoints(); ++i) {
        if (!(InitialHostConcentration(i) && InitialGuestConcentration(i)))
            continue;
        x << (1 / InitialHostConcentration(i) / InitialGuestConcentration(i));
        for (int j = 0; j < SeriesCount(); ++j) {
            y[j] << 1 / (DependentModel()->data(j, i) - DependentModel()->data(j, 0));
        }
    }
    for (int i = 0; i < SeriesCount(); ++i) {
        PeakPick::LinearRegression regress = LeastSquares(x, y[i]);
        K11 += qLn(qAbs(1 / regress.m)) / 2.3;
    }
    K11 /= double(SeriesCount());
    return K11;
}

#include "AbstractTitrationModel.moc"
