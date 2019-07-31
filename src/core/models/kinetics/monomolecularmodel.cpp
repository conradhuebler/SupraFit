/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/minimizer.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "monomolecularmodel.h"

MonoMolecularModel::MonoMolecularModel(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "t", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "c(A)", Qt::DisplayRole);
}

MonoMolecularModel::MonoMolecularModel(AbstractModel* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "t", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "c(A)", Qt::DisplayRole);
}

MonoMolecularModel::~MonoMolecularModel()
{
}

void MonoMolecularModel::DeclareOptions()
{
    QStringList order = QStringList() << "First"
                                      << "Second"
                                      << "Mixed"
                                      << "Free";
    addOption(Order, "Order", order);
    QStringList component = QStringList() << "A"
                                          << "B";
    addOption(Component, "Component detected", component);
}

void MonoMolecularModel::DeclareSystemParameter()
{
    addSystemParameter(ConcentrationA, "Initial Concentration A", "Initial concentration of component A", SystemParameter::Scalar);
    addSystemParameter(ConcentrationB, "Initial Concentration B", "Initial concentration of component B", SystemParameter::Scalar);
    setSystemParameterValue(ConcentrationB, 0);
}

void MonoMolecularModel::InitialGuess_Private()
{
    (*GlobalTable())[0] = 1;
    (*GlobalTable())[1] = 1;
    Calculate();
}

void MonoMolecularModel::OptimizeParameters_Private()
{
    QString order = getOption(Order);
        if (order == "First") // || order == "Mixed")
            addGlobalParameter(0);
        if (order == "Second") // || order == "Mixed")
            addGlobalParameter(1);
}

void MonoMolecularModel::CalculateVariables()
{
    UpdateParameter();
    qreal A0 = m_A0;
    qreal B0 = m_B0;
    QString order = getOption(Order);
    QString component = getOption(Component);
    qreal k1 = GlobalParameter(0);
    qreal k2 = GlobalParameter(1);
    for (int i = 0; i < DataPoints(); ++i) {
        qreal t = IndependentModel()->data(0, i) * getScaling()[0]; ///1000.0/60.0;
        for (int j = 0; j < SeriesCount(); ++j) {
            qreal value = 0;
            value += A0 * qExp(-k1 * t) * (order == "First"); // || order == "Mixed");
            value += 1 / (2 * k2 * t + (1 / A0)) * (order == "Second"); // || order == "Mixed");

            if (component == "A")
                SetValue(i, j, value);
            else if (component == "B")
                SetValue(i, j, A0 - value + B0);
        }
    }
}

QSharedPointer<AbstractModel> MonoMolecularModel::Clone(bool statistics)
{
    QSharedPointer<MonoMolecularModel> model = QSharedPointer<MonoMolecularModel>(new MonoMolecularModel(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->UpdateParameter();
    return model;
}

void MonoMolecularModel::UpdateParameter()
{
    m_A0 = getSystemParameter(ConcentrationA).Double();
    m_B0 = getSystemParameter(ConcentrationB).Double();
}

#include "monomolecularmodel.moc"
