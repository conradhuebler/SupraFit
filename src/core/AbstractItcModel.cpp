/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/AbstractModel.h"
#include "src/core/libmath.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QMutexLocker>

#include <iostream>

#include <libpeakpick/mathhelper.h>
#include <libpeakpick/nxlinregress.h>
#include <libpeakpick/peakpick.h>

#include "AbstractItcModel.h"


AbstractItcModel::AbstractItcModel(DataClass* data)
    : AbstractModel(data)
    , m_lock_concentrations(false)
{
    m_c0 = new DataTable(3, DataPoints(), this);
    m_c0->setHeaderData(0, Qt::Horizontal, "V (cell)", Qt::DisplayRole);
    m_c0->setHeaderData(1, Qt::Horizontal, "Host (A)", Qt::DisplayRole);
    m_c0->setHeaderData(2, Qt::Horizontal, "Guest (B)", Qt::DisplayRole);
    LoadSystemParameter();
}

AbstractItcModel::AbstractItcModel(AbstractItcModel* data)
    : AbstractModel(data)
    , m_lock_concentrations(false)
{
    m_c0 = new DataTable(3, DataPoints(), this);
    m_c0->setHeaderData(0, Qt::Horizontal, "V (cell)", Qt::DisplayRole);
    m_c0->setHeaderData(1, Qt::Horizontal, "Host (A)", Qt::DisplayRole);
    m_c0->setHeaderData(2, Qt::Horizontal, "Guest (B)", Qt::DisplayRole);

    m_V = data->m_V;
    m_cell_concentration = data->m_cell_concentration;
    m_syringe_concentration = data->m_syringe_concentration;
    m_T = data->m_T;
}

AbstractItcModel::~AbstractItcModel()
{
    if (m_c0)
        delete m_c0;
    if (m_concentrations)
        delete m_concentrations;
}

void AbstractItcModel::DeclareSystemParameter()
{
    QChar mu = QChar(956);

    addSystemParameter(CellVolume, "Cell Volume", "Volume of the cell in " + QString(mu) + "L", SystemParameter::Scalar);
    addSystemParameter(Temperature, "Temperature", "Temperature in K", SystemParameter::Scalar);
    addSystemParameter(CellConcentration, "Cell concentration", "Concentration in cell in mmol/L", SystemParameter::Scalar);

    addSystemParameter(SyringeConcentration, "Syringe concentration", "Concentration in syringe in mmol/L", SystemParameter::Scalar);
    setSystemParameterValue(Temperature, 298);

    addSystemParameter(Reservoir, "Cell Volume constant", "Keep the volume in cell constant", SystemParameter::Boolean);
    setSystemParameterValue(Reservoir, true);

    addSystemParameter(InptUnit, "Unit", "Observed heat in", SystemParameter::List);
    QStringList units = QStringList() << QString(mu) + " cal" << QString(mu) + "J"
                                      << "mcal"
                                      << "mJ";
    setSystemParameterList(InptUnit, units);
    setSystemParameterValue(InptUnit, 0);
}

void AbstractItcModel::DeclareOptions()
{
    QStringList method = QStringList() << "auto"
                                       << "none";
    addOption(Dilution, "Dilution", method);
    setOption(Dilution, "none");
}

qreal AbstractItcModel::GuessdH()
{
    if (!m_V || !m_cell_concentration || !m_syringe_concentration) {
        m_guess_failed = true;
        return -4000;
    }

    /* Lets calculate the dH guess by using the heat change from point 2 -> 3 (first are omitted since they are offen incorrect
     * No complexation constant is used, it is asumed, that all guest will form the new complex
     * and there for dH = q/(dc*V)
     */

    m_guess_failed = false;
    qreal c0 = m_c0->data(2, 2);
    qreal c1 = m_c0->data(2, 3);
    qreal dn = (c1 - c0) * m_V;
    qreal q = DependentModel()->data(0, 3);
    return q / dn;
}

qreal AbstractItcModel::GuessFx()
{

    if (!m_V || !m_cell_concentration || !m_syringe_concentration) {
        m_guess_failed = true;
        return 1;
    }
    m_guess_failed = false;

    /* The inflection point can easily be obtained through 3-x-linear regression and using the
     * the mean x value of the intersections of 1-2 and 2-3
     */

    QVector<qreal> x, y;

    for (int i = 1; i < DataPoints(); ++i) {
        x << PrintOutIndependent(i, 0);
        y << DependentModel()->data(0, i);
    }
    QMap<qreal, PeakPick::MultiRegression> result = LeastSquares(x, y, 3);
    PeakPick::MultiRegression regression = result.first();

    qreal x1 = 0, x2 = 0;
    int m = 0;
    x1 = (regression.regressions[m].n - regression.regressions[m + 1].n) / (regression.regressions[m + 1].m - regression.regressions[m].m);
    m = 1;

    x2 = (regression.regressions[m].n - regression.regressions[m + 1].n) / (regression.regressions[m + 1].m - regression.regressions[m].m);

    return (x1 + x2) / 2;
}

qreal AbstractItcModel::GuessK(int index)
{

    /* For now, this is really a crude method
     */

    QMap<qreal, qreal> vals;

    for (double i = 1; i < 10; i += 0.5) {
        (*GlobalTable())[index] = i;
        Calculate();
        vals[SumofSquares()] = i;
    }
    return vals.first();
}

void AbstractItcModel::CalculateConcentrations()
{
    if (m_lock_concentrations)
        return;

    if ((!m_V || !m_cell_concentration || !m_syringe_concentration) && (SFModel() != SupraFit::itc_blank))
        return;

    /*
     * Conversation from L to mL
     * */
    qreal convers = 1e-3;

    /*
     * Cell Volumen is given in muL since each inject is given in
     * muL as well
     * */
    qreal V_cell = m_V;

    bool reservoir = m_reservior;

    /*
     * Initial concentration is given in mol/L
     * therefore we want to convert into mol/mL
     * */
    qreal cell = m_cell_concentration * convers;
    qreal gun = m_syringe_concentration * convers;

    qreal prod = 1;
    qreal cell_0 = V_cell * cell;
    qreal cumulative_shot = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        qreal shot_vol = IndependentModel()->data(0, i);

        /*
         * Assuming constant cell volume, the absolute amount of host changes upon injection
         * The equation is taken from Freire, E., Schön, A., & Velazquez‐Campoy, A. (2009). Isothermal titration calorimetry: ..., 455, 127-155. Page 134 eq. 5.14
         * If the cell volume is not constant, we just add up
         * The correct value is set through the boolean reservoir value to avoid branching in loops
         * */
        V_cell += shot_vol;
        cumulative_shot += shot_vol;

        prod *= (1 - shot_vol / m_V);
        cell *= (1 - shot_vol / m_V);
        qreal host_0 = cell * reservoir + !reservoir * cell_0 / V_cell;
        qreal guest_0 = gun * (1 - prod) * reservoir + !reservoir * (gun * cumulative_shot) / V_cell;

        Vector vector(3);
        vector(0) = reservoir * m_V + !reservoir * V_cell;
        vector(1) = host_0;
        vector(2) = guest_0;

        if (std::isnan(host_0) || std::isinf(host_0) || std::isnan(guest_0) || std::isinf(guest_0))
            m_corrupt = true;

        m_c0->setRow(vector, i);
    }
}

void AbstractItcModel::SetConcentration(int i, const Vector& equilibrium)
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
}

QString AbstractItcModel::Model2Text_Private() const
{
    QString text;
    if (m_c0) {
        text += "Initial concentration calculated from ITC Experiment:\n";
        for (int i = 0; i < m_c0->columnCount(); ++i) {
            text += " " + m_c0->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
        }
        text += "\n";
        text += m_c0->ExportAsString();
        text += "\n";
    }
    if (m_concentrations) {
        text += "Equilibrium concentration calculated with complexation constants:\n";
        for (int i = 0; i < m_concentrations->columnCount(); ++i) {
            text += " " + m_concentrations->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
        }
        text += "\n";
        text += m_concentrations->ExportAsString();
        text += "\n\n";
    }
    text += "Equilibrium Model Signal calculated with complexation constants:\n";
    for (int i = 0; i < DependentModel()->columnCount(); ++i)
        text += " " + DependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";

    text += "\n\nEquilibrium Model Signal consisting of:\n";
    text += m_more_info;

    return text;
}

qreal AbstractItcModel::PrintOutIndependent(int i, int format) const
{
    Q_UNUSED(format)
    qreal val = i;
    if (m_c0) {
        val = InitialGuestConcentration(i) / InitialHostConcentration(i);
        if (std::isnan(val))
            val = i;
    }
    return val;
}

void AbstractItcModel::UpdateParameter()
{
    m_V = getSystemParameter(CellVolume).Double();
    m_cell_concentration = getSystemParameter(CellConcentration).Double();
    m_syringe_concentration = getSystemParameter(SyringeConcentration).Double();
    m_T = getSystemParameter(Temperature).Double();
    m_reservior = getSystemParameter(Reservoir).Bool();
    Concentration();
    if (m_guess_failed && m_demand_guess)
        InitialGuess();
}

QString AbstractItcModel::ModelInfo() const
{
    return QString();
}

QString AbstractItcModel::RandomInput(const QVector<double>& indep, const QVector<double>& dep) const
{
    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng(seed);

    QString input;
    QPointer<DataTable> indep_model = IndependentModel()->PrepareMC(indep, rng);
    QPointer<DataTable> dep_model = ModelTable()->PrepareMC(dep, rng);

    QStringList x = indep_model->ExportAsStringList();
    QStringList y = dep_model->ExportAsStringList();

    delete indep_model;
    delete dep_model;

    input = "10\n";
    input += "0," + QString::number(DataPoints()) + ",0,0,0\n";
    input += QString::number(getT() - 273) + "," + QString::number(getCellConcentration()) + "," + QString::number(getSyringeConcentration()) + "," + QString::number(getV() / 1000.0) + ",0\n";
    input += "0\n";
    input += "0\n";

    if (x.size() == y.size()) {
        for (int i = 0; i < x.size(); ++i)
            input += (x[i].simplified().replace(",", ".") + "," + y[i].simplified().replace(",", ".")).simplified() + "\n";
    }

    return input;
}

void AbstractItcModel::UpdateOption(int index, const QString& str)
{
    Q_UNUSED(index)
    Q_UNUSED(str)
    /*if (index == Reservoir)
        Concentration();*/
}

#include "AbstractItcModel.moc"
