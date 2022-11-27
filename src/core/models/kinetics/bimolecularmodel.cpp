/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/libmath.h"
#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "bimolecularmodel.h"

BiMolecularModel::BiMolecularModel(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "t", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "c(A)", Qt::DisplayRole);
}

BiMolecularModel::BiMolecularModel(AbstractModel* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "t", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "c(A)", Qt::DisplayRole);
}

BiMolecularModel::~BiMolecularModel()
{
}

void BiMolecularModel::DeclareOptions()
{
    QStringList cooperativity = QStringList() << "yes"
                                              << "no";
    addOption(Scale, "Scale", cooperativity);

    AbstractModel::DeclareOptions();
}

void BiMolecularModel::DeclareSystemParameter()
{
}

void BiMolecularModel::InitialGuess_Private()
{
    (*GlobalTable())[0] = 0.002;
    (*GlobalTable())[1] = DependentModel()->data(DataBegin());
    (*GlobalTable())[2] = DependentModel()->data(DataEnd());
    (*GlobalTable())[3] = DependentModel()->data(DataBegin()) / DependentModel()->data(DataEnd());
    (*GlobalTable())[4] = 1;
    QSharedPointer<AbstractModel> test = Clone();
    (*GlobalTable())[0] = BisectParameter(test, 0, 0, 0.1, 1e-5);
    Calculate();
}

void BiMolecularModel::OptimizeParameters_Private()
{
    addGlobalParameter(0);
    addGlobalParameter(1);
    addGlobalParameter(2);
    addGlobalParameter(3);

    QString scale = getOption(Scale);
    if (scale == "yes")
        addGlobalParameter(4);
}

void BiMolecularModel::CalculateVariables()
{
    UpdateParameter();
    qreal k = GlobalParameter(0);
    qreal cA0 = GlobalParameter(1);
    qreal cAeq = GlobalParameter(2);
    qreal cc0 = GlobalParameter(3);
    qreal q;
    QString scale = getOption(Scale);
    if (scale == "yes")
        q = GlobalParameter(4);
    else
        q = 1;
    for (int i = DataBegin(); i < DataEnd(); ++i) {
        qreal t = IndependentModel()->data(i);
        // for (int j = 0; j < SeriesCount(); ++j) {
        qreal value = (cA0 * cc0 - cA0 * cA0) / (cc0 * exp(cc0 * k * q * t - cA0 * k * q * t) - cA0) + cAeq;
        SetValue(i, 0, value);
        //}
    }
}

QSharedPointer<AbstractModel> BiMolecularModel::Clone(bool statistics)
{
    QSharedPointer<BiMolecularModel> model = QSharedPointer<BiMolecularModel>(new BiMolecularModel(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->UpdateParameter();
    return model;
}

void BiMolecularModel::UpdateParameter()
{
}

#include "bimolecularmodel.moc"
