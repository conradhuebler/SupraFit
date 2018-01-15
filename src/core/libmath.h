/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global_config.h"
#include "src/core/AbstractModel.h"
#include <QtCore/QPair>


class AbstractModel;
struct OptimizerConfig;

struct LinearRegression
{
    QVector<qreal> y, x, y_head;
    qreal m = 0;
    qreal n = 0;
    qreal R = 0;
    qreal sum_err = 0;
};

qreal MinQuadraticRoot(qreal a, qreal b, qreal c);
QPair<qreal, qreal> QuadraticRoots(qreal a, qreal b, qreal c);
qreal MinCubicRoot(qreal a, qreal b, qreal c, qreal d);

LinearRegression LeastSquares(const QVector<qreal> &x, const QVector<qreal> &y);

namespace Cubic{
    qreal f(qreal x, qreal a, qreal b, qreal c, qreal d);
    qreal df(qreal x, qreal a, qreal b, qreal c);
}

int NonlinearFit(QWeakPointer<AbstractModel> model, QVector<qreal > &param);

