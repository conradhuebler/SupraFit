/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/AbstractModel.h"

#include "src/core/libmath.h"
#include "src/core/minimizer.h"

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include <cmath>
#include <iostream>

#include <libpeakpick/mathhelper.h>
#include <libpeakpick/peakpick.h>

#include "decayrates.h"

DecayRates::DecayRates(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "t", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "I", Qt::DisplayRole);
}

DecayRates::DecayRates(AbstractModel* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "t", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "I", Qt::DisplayRole);
}

DecayRates::~DecayRates()
{
}

void DecayRates::DeclareOptions()
{
    QStringList functions = QStringList() << "1"
                                          << "2"
                                          << "3"
                                          << "4";
    addOption(1, "Fit Functions", functions);
}

void DecayRates::EvaluateOptions()
{
    QString str = getOption(1);
    int functions = str.toInt();

    if (functions == 1) {
        m_z = 0;
        m_d = 0;
        m_v = 0;
    } else if (functions == 2) {
        m_z = 1;
        m_d = 0;
        m_v = 0;
    } else if (functions == 3) {
        m_z = 1;
        m_d = 1;
        m_v = 0;
    } else if (functions == 4) {
        m_z = 1;
        m_d = 1;
        m_v = 1;
    }
}

void DecayRates::OptimizeParameters_Private()
{
    QString str = getOption(1);

    int functions = str.toInt();
    addLocalParameter(0);
    addGlobalParameter(0);
    addLocalParameter(1);
    addGlobalParameter(1);
    addLocalParameter(2);
    addGlobalParameter(2);
    addLocalParameter(3);
    addGlobalParameter(3);
    return;
    m_z = 1;
    m_d = 1;
    m_v = 1;
    if (functions == 1) {
        m_z = 0;
        m_d = 0;
        m_v = 0;
    } else if (functions == 2) {
        addLocalParameter(1);
        addGlobalParameter(1);

        m_z = 1;
        m_d = 0;
        m_v = 0;
    } else if (functions == 3) {
        addLocalParameter(1);
        addGlobalParameter(1);
        addLocalParameter(2);
        addGlobalParameter(2);

        m_z = 1;
        m_d = 1;
        m_v = 0;
    } else if (functions == 4) {
        addLocalParameter(1);
        addGlobalParameter(1);
        addLocalParameter(2);
        addGlobalParameter(2);
        addLocalParameter(3);
        addGlobalParameter(3);

        m_z = 1;
        m_d = 1;
        m_v = 1;
    }
    m_z = 1;
    m_d = 1;
    m_v = 1;
}

void DecayRates::InitialGuess_Private()
{
    (*GlobalTable())[0] = 1e4;
    (*GlobalTable())[1] = 1e5;
    (*GlobalTable())[2] = 1e5;
    (*GlobalTable())[3] = 1e5;
    (*LocalTable())[0] = 1e4;
    (*LocalTable())[1] = 0;
    (*LocalTable())[2] = 0;
    (*LocalTable())[3] = 0;
    Calculate();
}

void DecayRates::CalculateVariables()
{
    qreal t1 = GlobalParameter(0);
    qreal t2 = GlobalParameter(1);
    qreal t3 = GlobalParameter(2);
    qreal t4 = GlobalParameter(3);

    qreal B1 = LocalTable()->data(0);
    qreal B2 = LocalTable()->data(1);
    qreal B3 = LocalTable()->data(2);
    qreal B4 = LocalTable()->data(3);

    for (int i = 0; i < DataPoints(); ++i) {
        qreal t = IndependentModel()->data(i);
        qreal value = B1 * exp(-1 * t / t1);
        // if(m_z)
        value += B2 * exp(-1 * t / t2);
        // if(m_d)
        value += B3 * exp(-1 * t / t3);
        // if(m_v)
        value += B4 * exp(-1 * t / t4);
        SetValue(i, 0, value);
    }
}

QSharedPointer<AbstractModel> DecayRates::Clone(bool statistics)
{
    QSharedPointer<DecayRates> model = QSharedPointer<DecayRates>(new DecayRates(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "decayrates.moc"
