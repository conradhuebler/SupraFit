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

#include <QtMath>

#include <QDebug>
#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include <cmath>
#include <iostream>

#include "mm_model.h"

Michaelis_Menten_Model::Michaelis_Menten_Model(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "S0", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "v", Qt::DisplayRole);
}

Michaelis_Menten_Model::~Michaelis_Menten_Model()
{
}

void Michaelis_Menten_Model::InitialGuess_Private()
{
    QVector<qreal> x, y;

    for (int i = 1; i < DataPoints(); ++i) {
        x << 1 / IndependentModel()->data(0, i);
        y << 1 / DependentModel()->data(0, i);
    }

    LinearRegression regress = LeastSquares(x, y);
    m_vmax = 1 / regress.n;
    m_Km = regress.m * m_vmax;
    (*GlobalTable())[0] = m_vmax;
    (*GlobalTable())[1] = m_Km;
    Calculate();
}

QVector<qreal> Michaelis_Menten_Model::OptimizeParameters_Private()
{
        addGlobalParameter(0);
        addGlobalParameter(1);

    QVector<qreal> parameter;
    for (int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void Michaelis_Menten_Model::CalculateVariables()
{
    m_sum_absolute = 0;
    m_sum_squares = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        qreal vmax = GlobalParameter(0);
        qreal Km = GlobalParameter(1);
        qreal S_0 = IndependentModel()->data(0, i);
        for (int j = 0; j < SeriesCount(); ++j) {
            qreal value = vmax * S_0 / (Km + S_0);
            SetValue(i, j, value);
        }
    }
}

QSharedPointer<AbstractModel> Michaelis_Menten_Model::Clone()
{
    QSharedPointer<Michaelis_Menten_Model> model = QSharedPointer<Michaelis_Menten_Model>(new Michaelis_Menten_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "mm_model.moc"
