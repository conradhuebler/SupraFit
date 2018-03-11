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

#include <QtMath>
#include <QDebug>

#include <QtCore/QFile>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QMutexLocker>

#include "AbstractItcModel.h"

#include <iostream>

AbstractItcModel::AbstractItcModel(DataClass *data) : AbstractModel(data), m_lock_concentrations(false)
{
    connect(this, &DataClass::SystemParameterChanged, this, &AbstractItcModel::UpdateParameter);
    m_c0 = new DataTable( 2, DataPoints(), this);
    m_c0->setHeaderData(0, Qt::Horizontal, "Host (A)", Qt::DisplayRole);
    m_c0->setHeaderData(1, Qt::Horizontal, "Guest (B)", Qt::DisplayRole);
    LoadSystemParameter();
}

AbstractItcModel::AbstractItcModel(AbstractItcModel *data) : AbstractModel(data), m_lock_concentrations(false)
{
    connect(this, &DataClass::SystemParameterChanged, this, &AbstractItcModel::UpdateParameter);
    m_c0 = new DataTable( 2, DataPoints(), this);
    m_c0->setHeaderData(0, Qt::Horizontal, "Host (A)", Qt::DisplayRole);
    m_c0->setHeaderData(1, Qt::Horizontal, "Guest (B)", Qt::DisplayRole);

    m_V = data->m_V;
    m_cell_concentration = data->m_cell_concentration;
    m_syringe_concentration = data->m_syringe_concentration;
    m_T = data->m_T;
}

AbstractItcModel::~AbstractItcModel()
{
    if(m_c0)
        delete m_c0;
    if(m_concentrations)
        delete m_concentrations;
}

void AbstractItcModel::DeclareSystemParameter()
{
    QChar mu = QChar(956);
    addSystemParameter(CellVolume, "Cell Volume", "Volume of the cell in " + QString(mu) + "L", SystemParameter::Scalar);
    addSystemParameter(Temperature, "Temperature", "Temperature in K", SystemParameter::Scalar);
    addSystemParameter(CellConcentration, "Cell concentration", "Concentration in cell in mol/L", SystemParameter::Scalar);
    addSystemParameter(SyringeConcentration, "Syringe concentration", "Concentration in syringe in mol/L", SystemParameter::Scalar);
}

void AbstractItcModel::DeclareOptions()
{
    QStringList method = QStringList() << "auto" << "none";
    addOption("Dilution", method);
    QStringList cooperativity = QStringList() << "pytc" /*<< "multiple"*/ << "single";
    addOption("Binding", cooperativity);
}

void AbstractItcModel::CalculateConcentrations()
{
    if(m_lock_concentrations)
        return;

    qreal emp_exp = 1e-3;

    if(!m_V || !m_cell_concentration || !m_syringe_concentration)
        return;

    qreal V_cell = m_V;

    qreal cell = m_cell_concentration*emp_exp;
    qreal gun = m_syringe_concentration*emp_exp;
    qreal prod = 1;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal shot_vol = IndependentModel()->data(0,i);
        V_cell += shot_vol;
        prod *= (1-shot_vol/m_V);
        cell *= (1-shot_vol/m_V);
        qreal host_0 = cell;
        qreal guest_0 = gun*(1-prod);
        Vector vector(2);
        vector(0) = host_0;
        vector(1) = guest_0;

        if(std::isnan(host_0) || std::isinf(host_0) || std::isnan(guest_0) || std::isinf(guest_0))
            m_corrupt = true;

        m_c0->setRow(vector, i);
    }
}

void AbstractItcModel::SetConcentration(int i, const Vector& equilibrium)
{
    if(!m_concentrations)
    {
        m_concentrations = new DataTable( equilibrium.rows(), DataPoints(), this);
        m_concentrations->setHeaderData(0, Qt::Horizontal, "Exp.", Qt::DisplayRole);
        m_concentrations->setHeaderData(1, Qt::Horizontal, "Host (A)", Qt::DisplayRole);
        m_concentrations->setHeaderData(2, Qt::Horizontal, "Guest (B)", Qt::DisplayRole);
        for(int i = 0; i < GlobalParameterSize(); ++i)
            m_concentrations->setHeaderData(3 + i, Qt::Horizontal, SpeciesName(i), Qt::DisplayRole);
    }
    m_concentrations->setRow(equilibrium, i);
}

QString AbstractItcModel::Model2Text_Private() const
{
    QString text;
    if(m_c0)
    {
        text += "Initial concentration calculated from ITC Experiment:\n";
        for(int i = 0; i < m_c0->columnCount(); ++i)
        {
            text += " " + m_c0->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
        }
        text += "\n";
        text += m_c0->ExportAsString();
        text += "\n";
    }
    if(m_concentrations)
    {
        text += "Equilibrium concentration calculated with complexation constants:\n";
        for(int i = 0; i < m_concentrations->columnCount(); ++i)
        {
            text += " " + m_concentrations->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
        }
        text += "\n";
        text += m_concentrations->ExportAsString();
        text += "\n\n";
    }
    text += "Equilibrium Model Signal calculated with complexation constants:\n";
    for(int i = 0; i < DependentModel()->columnCount(); ++i)
        text += " " + DependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";

    return text;
}

qreal AbstractItcModel::PrintOutIndependent(int i, int format) const
{
    Q_UNUSED(format)
    qreal val = i;
    if(m_c0)
    {
        val = InitialGuestConcentration(i)/InitialHostConcentration(i);
        if(std::isnan(val))
            val = i;
    }
    return val;
}


void AbstractItcModel::UpdateParameter()
{
    m_V = getSystemParameter(CellVolume).Double();
    m_cell_concentration =getSystemParameter(CellConcentration).Double();
    m_syringe_concentration = getSystemParameter(SyringeConcentration).Double();
    m_T = getSystemParameter(Temperature).Double();
    Concentration();
}
#include "AbstractItcModel.moc"
