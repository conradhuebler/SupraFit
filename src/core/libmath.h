/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QPair>

#include <libpeakpick/mathhelper.h>
#include <libpeakpick/nxlinregress.h>
#include <libpeakpick/peakpick.h>

#include "src/core/models/AbstractModel.h"

#include "src/global_config.h"

class AbstractModel;
struct OptimizerConfig;

long double MinQuadraticRoot(long double a, long double b, long double c);
long double MaxQuadraticRoot(long double a, long double b, long double c);

QPair<long double, long double> QuadraticRoots(long double a, long double b, long double c);

/**
 * @brief Selectable implementations behind MinCubicRoot(). Claude Generated.
 *
 * @c Newton is the historical behaviour and stays the DEFAULT so existing fit results are bit-for-bit
 * reproducible. @c Analytic is the closed-form (Cardano / trigonometric) solution: no iteration, and
 * it returns the smallest non-negative root, which is the physically meaningful one for a free
 * concentration.
 *
 * The two do NOT agree in every case, by design — the Newton path has known defects (its third root
 * iterates on the derivative instead of the function, so it is not a root at all, and two branches of
 * its root selection are unreachable). Opt in per run, compare, then decide.
 */
namespace CubicSolver {
enum class Method {
    Newton, ///< historical three-Newton search (default)
    Analytic ///< closed-form Cardano / trigonometric solution
};
void setMethod(Method method);
Method method();
}

/*! \brief Closed-form real root of a x^3 + b x^2 + c x + d, smallest non-negative one. CG. */
qreal AnalyticCubicRoot(qreal a, qreal b, qreal c, qreal d);

qreal MinCubicRoot(qreal a, qreal b, qreal c, qreal d);

PeakPick::LinearRegression LeastSquares(const QVector<qreal>& x, const QVector<qreal>& y);
QMap<qreal, PeakPick::MultiRegression> LeastSquares(const QVector<qreal>& x, const QVector<qreal>& y, int functions);

qreal SimpsonIntegrate(qreal lower, qreal upper, std::function<qreal(qreal, const QVector<qreal>)> function, const QVector<qreal>& parameter, qreal delta = 1e-4);
std::vector<qreal> SimpsonIntegrate(qreal lower, qreal upper, const std::vector<std::function<qreal(qreal, const QVector<qreal>)>*>& functions, const QVector<qreal>& parameter, qreal delta = 1e-4);

qreal DiscreteIntegrate(const QVector<qreal>& x, const QVector<qreal>& y);
qreal Stddev(const QVector<qreal>& vector, int end = 0, double average = 0);

double NewtonRoot(QWeakPointer<AbstractModel> model, int index, qreal min, qreal max, double epsilon = 1e-4, int maxiter = 30);

double LowerLogFermi(double x, double x0, double k, double beta);
double UpperLogFermi(double x, double x0, double k, double beta);

qint64 Factorial(qint64 n);

namespace Cubic {
qreal f(qreal x, qreal a, qreal b, qreal c, qreal d);
qreal df(qreal x, qreal a, qreal b, qreal c);
}

int NonlinearFit(QWeakPointer<AbstractModel> model, QVector<qreal>& param, QVector<double>& rmsd, QVector<QVector<double>>& parameter);

/*! \brief Opt-in variable-projection (VarPro) fit: a self-contained damped Levenberg-Marquardt over
 * only the non-linear global parameters; the linear local parameters are projected out by masked
 * least-squares (AbstractModel::ProjectLinearParameters()) at each residual evaluation. For models
 * with SupportsVarPro(); selected via the "FitSolver" optimizer-config key. Claude Generated. */
int VarProFit(QWeakPointer<AbstractModel> model, QVector<double>& rmsd, QVector<QVector<double>>& parameter);
