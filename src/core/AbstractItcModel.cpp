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

#include "AbstractItcModel.h"

AbstractItcModel::AbstractItcModel(DataClass *data) : AbstractModel(data)
{
    connect(m_data, &DataClass::SystemParameterChanged, this, &AbstractModel::Calculate);
    m_c0 = new DataTable( 2, DataPoints(), this);
    m_c0->setHeaderData(0, Qt::Horizontal, "Host (A)", Qt::DisplayRole);
    m_c0->setHeaderData(1, Qt::Horizontal, "Guest (B)", Qt::DisplayRole);

    CalculateConcentrations();
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
    m_data->addSystemParameter(CellVolume, "Cell Volume", "Volume of the cell in " + QString(mu) + "L", SystemParameter::Scalar);
    m_data->addSystemParameter(Temperature, "Temperature", "Temperature in K", SystemParameter::Scalar);
    m_data->addSystemParameter(CellConcentration, "Cell concentration", "Concentration in cell in mol/L", SystemParameter::Scalar);
    m_data->addSystemParameter(SyringeConcentration, "Syringe concentration", "Concentration in syringe in mol/L", SystemParameter::Scalar);
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
    qreal emp_exp = 1e-3;

    qreal V = m_data->getSystemParameter(CellVolume).Double();
    qreal initial_cell = m_data->getSystemParameter(CellConcentration).Double();
    qreal initial_syringe = m_data->getSystemParameter(SyringeConcentration).Double();

    if(!V || !initial_cell || !initial_syringe)
        return;

    qreal V_cell = V;

    qreal cell = initial_cell*emp_exp;
    qreal gun = initial_syringe*emp_exp;
    qreal prod = 1;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal shot_vol = IndependentModel()->data(0,i);
        V_cell += shot_vol;
        prod *= (1-shot_vol/V);
        cell *= (1-shot_vol/V);
        qreal host_0 = cell;
        qreal guest_0 = gun*(1-prod);
        Vector vector(2);
        vector(0) = host_0;
        vector(1) = guest_0;
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
        //for(int i = 0; i < GlobalParameterSize(); ++i)
            //m_concentrations->setHeaderData(3 + i, Qt::Horizontal, SpeciesName(i), Qt::DisplayRole);
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
    if(m_c0)
        return InitialGuestConcentration(i)/InitialHostConcentration(i);
    else
        return i;
}

#include "AbstractItcModel.moc"
