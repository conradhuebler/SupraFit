/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#pragma once

#include <Eigen/Dense>

#include "src/core/AbstractModel.h"
#include "src/global_config.h"
#include <QtCore/QPair>

class AbstractModel;
struct OptimizerConfig;

struct LinearRegression {
    QVector<qreal> y, x, y_head;
    qreal m = 0;
    qreal n = 0;
    qreal R = 0;
    qreal sum_err = 0;
};

struct MultiRegression {
    QVector<LinearRegression> regressions;
    qreal sum_err = 0;
    QVector<int> start;
};

long double MinQuadraticRoot(long double a, long double b, long double c);
long double MaxQuadraticRoot(long double a, long double b, long double c);

QPair<long double, long double> QuadraticRoots(long double a, long double b, long double c);
qreal MinCubicRoot(qreal a, qreal b, qreal c, qreal d);

LinearRegression LeastSquares(const QVector<qreal>& x, const QVector<qreal>& y);
QMap<qreal, MultiRegression> LeastSquares(const QVector<qreal>& x, const QVector<qreal>& y, int functions);

qreal SimpsonIntegrate(qreal lower, qreal upper, std::function<qreal(qreal, const QVector<qreal>)> function, const QVector<qreal>& parameter);

qreal Stddev(const QVector<qreal>& vector);

namespace Cubic {
qreal f(qreal x, qreal a, qreal b, qreal c, qreal d);
qreal df(qreal x, qreal a, qreal b, qreal c);
}

int NonlinearFit(QWeakPointer<AbstractModel> model, QVector<qreal>& param);
