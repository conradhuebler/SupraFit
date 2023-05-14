/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2023 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "bet.h"

BETModel::BETModel(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "T", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "r", Qt::DisplayRole);
}

BETModel::BETModel(AbstractModel* data)
    : AbstractModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    IndependentModel()->setHeaderData(0, Qt::Horizontal, "T", Qt::DisplayRole);
    DependentModel()->setHeaderData(0, Qt::Horizontal, "r", Qt::DisplayRole);
}

BETModel::~BETModel()
{
}

void BETModel::DeclareSystemParameter()
{
    const QChar mu = QChar(956);
    const QString sub_char = QChar(0x2080);

    addSystemParameter(V1, "Volume", "Reservoir volume in mL", SystemParameter::Scalar);
    addSystemParameter(V2, "Volume", "Volume at Ambient Temperature TA mL", SystemParameter::Scalar);
    addSystemParameter(V3, "Volume", "Volume at Sample Temperature T0 mL", SystemParameter::Scalar);

    addSystemParameter(Pressure, "Pressure", "Ambient Pressure in Pascal", SystemParameter::Scalar);
    addSystemParameter(Mass, "Sample Mass", "Mass of the sample in g", SystemParameter::Scalar);

    addSystemParameter(TA, "Ambient Temperature", "Ambient temperature in K", SystemParameter::Scalar);
    setSystemParameterValue(TA, 298);
    addSystemParameter(T0, "Sample Temperature", "Temperature of sample and gas K", SystemParameter::Scalar);
    setSystemParameterValue(T0, 77);
}

void BETModel::DeclareOptions()
{
    QStringList method = QStringList() << "Volume"
                                       << "Pressure";
    addOption(InputQuantity, "Input Quantity", method);
    setOption(InputQuantity, "Volume");
}

void BETModel::InitialGuess_Private()
{
    QVector<qreal> x, y;

    for (int i = 1; i < DataPoints(); ++i) {
        x << 1 / IndependentModel()->data(i);
        y << log(DependentModel()->data(i));
    }
    double A = 0.0, EA = 0.0;
    PeakPick::LinearRegression regress = LeastSquares(x, y);
    A = exp(regress.n);
    EA = -1 * regress.m * R;
    (*GlobalTable())[0] = A;
    (*GlobalTable())[1] = EA;
    Calculate();
}

void BETModel::CalculateVariables()
{
    qreal C = GlobalParameter(0);
    qreal vm = GlobalParameter(1);
    double p0 = getSystemParameter(Pressure).Double();
    for (int i = DataBegin(); i < DataEnd(); ++i) {

        qreal p = IndependentModel()->data(i);
        qreal value = vm * C * p / ((p0 - p) * (1 + p / p0 * (C - 1)));
        SetValue(i, AppliedSeries(), value);
    }
}

void BETModel::UpdateOption(int index, const QString& str)
{
    Q_UNUSED(index)
    Q_UNUSED(str)
    CalculateVolume();
}

void BETModel::CalculateVolume()
{
    if (!m_volume) {
        m_volume = new DataTable(DataPoints(), 1, this);
    }
    QString method = getOption(InputQuantity);
    if (method == "Pressure") {
        double v = 0.0;
        double v1 = getSystemParameter(V1).Double();
        double v2 = getSystemParameter(V2).Double();
        double v3 = getSystemParameter(V3).Double();
        double Ta = getSystemParameter(TA).Double();
        double T = getSystemParameter(T0).Double();
        double R = 8.3145;
        double p_prev = 0;
        for (int i = 0; i < DataPoints(); ++i) {
            double p1 = IndependentModel()->data(i, 0);
            double p2 = DependentModel()->data(i, 0);
            double n = (p1 - p2) * (v1 / R / Ta) + (p2 - p_prev) * (v2 / R / Ta + v3 / R / T) * 1e-4;
            v += n * 22.4;
            m_volume->data(i, 0) = v;
            p_prev = p2;
        }
    } else if (method == "Volume") {
        for (int i = 0; i < DataPoints(); ++i)
            m_volume->data(i, 0) = DependentModel()->data(i, 0);
    }
}

qreal BETModel::PrintOutIndependent(int i) const
{
    if (!m_volume)
        return i;

    if (m_volume->rowCount() < i)
        return i;
    return m_volume->data(i, 0);
}

QSharedPointer<AbstractModel> BETModel::Clone(bool statistics)
{
    QSharedPointer<BETModel> model = QSharedPointer<BETModel>(new BETModel(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "bet.moc"
