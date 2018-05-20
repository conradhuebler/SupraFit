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

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QMutexLocker>

#include "AbstractItcModel.h"

#include <iostream>

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
    addSystemParameter(CellConcentration, "Cell concentration", "Concentration in cell in mol/L", SystemParameter::Scalar);

    addSystemParameter(SyringeConcentration, "Syringe concentration", "Concentration in syringe in mol/L", SystemParameter::Scalar);
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
    /*QStringList cooperativity = QStringList() << "pytc" << "single";
    addOption(Binding, "Binding", cooperativity);*/

    /*QStringList reservoir = QStringList() << "constant"
                                          << "variable";
    addOption(Reservoir, "Volume", reservoir);
    setOption(Reservoir, "constant");*/
}

void AbstractItcModel::CalculateConcentrations()
{
    if (m_lock_concentrations)
        return;

    qreal emp_exp = 1e-3;

    if ((!m_V || !m_cell_concentration || !m_syringe_concentration) && (SFModel() != SupraFit::itc_blank))
        return;

    qreal V_cell = m_V;
    bool reservoir = m_reservior;

    qreal cell = m_cell_concentration * emp_exp;
    qreal gun = m_syringe_concentration * emp_exp;
    qreal prod = 1;
    qreal cell_0 = V_cell * cell;
    qreal cumulative_shot = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        qreal shot_vol = IndependentModel()->data(0, i);
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
}

QString AbstractItcModel::ModelInfo() const
{
    return QString();
}

void AbstractItcModel::UpdateOption(int index, const QString& str)
{
    /*if (index == Reservoir)
        Concentration();*/
}

#include "AbstractItcModel.moc"
