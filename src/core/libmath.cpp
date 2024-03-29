/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global_config.h"

#include <QtCore/QDebug>
#include <QtCore/QPair>
#include <QtCore/QtGlobal>
#include <QtCore/QtMath>

#include <cmath>
#include <iostream>

#include <libpeakpick/mathhelper.h>
#include <libpeakpick/nxlinregress.h>
#include <libpeakpick/peakpick.h>

#include "src/core/models/models.h"

#include "libmath.h"

#pragma omp declare reduction(vec_double_plus                                                                                          \
                              : std::vector <double>                                                                                   \
                              : std::transform(omp_out.begin(), omp_out.end(), omp_in.begin(), omp_out.begin(), std::plus <double>())) \
    initializer(omp_priv = omp_orig)

namespace Cubic {
qreal f(qreal x, qreal a, qreal b, qreal c, qreal d)
{
    return (x * x * x * a + x * x * b + x * c + d);
}

qreal df(qreal x, qreal a, qreal b, qreal c)
{
    return (3 * x * x * a + 2 * x * b + c);
}
}

PeakPick::LinearRegression LeastSquares(const QVector<qreal>& x, const QVector<qreal>& y)
{
    if (x.size() != y.size())
        throw 404;

    Vector _x;
    Vector _y;

    _x.resize(x.size());
    _y.resize(y.size());

    for (int i = 0; i < x.size(); ++i) {
        _x(i) = x[i];
        _y(i) = y[i];
    }

    return PeakPick::LeastSquares(_x, _y);
}

QMap<qreal, PeakPick::MultiRegression> LeastSquares(const QVector<qreal>& x, const QVector<qreal>& y, int functions)
{
    if (x.size() != y.size())
        return QMap<qreal, PeakPick::MultiRegression>();

    Vector _x;
    Vector _y;

    _x.resize(x.size());
    _y.resize(y.size());
    for (int i = 0; i < x.size(); ++i) {
        _x(i) = x[i];
        _y(i) = y[i];
    }

    QMap<double, PeakPick::MultiRegression> regressions(PeakPick::LeastSquares(_x, _y, functions));

    return regressions;
}

long double MinQuadraticRoot(long double a, long double b, long double c)
{
    return (-b - sqrt(b * b - 4 * a * c)) / (2 * a);
}

long double MaxQuadraticRoot(long double a, long double b, long double c)
{
    return (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
}

qreal Stddev(const QVector<qreal>& vector, int end, double average)
{
    if (!end || end == 1)
        end = vector.size();
    double size = end;
    if (qFuzzyCompare(average, 0)) {
        for (int i = 0; i < end; ++i)
            average += vector[i];
        average /= double(end);
        size = end - 1;
    }
    qreal stdev = 0;
    for (int i = 0; i < end; ++i)
        stdev += (vector[i] - average) * (vector[i] - average);
    return sqrt(stdev / size);
}

QPair<long double, long double> QuadraticRoot(long double a, long double b, long double c)
{
    QPair<long double, long double> pair(0, 0);
    if ((qPow(b, 2) - 4 * a * c) < 0)
        return pair;
    pair.first = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);
    pair.second = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
    return pair;
}

qreal MinCubicRoot(qreal a, qreal b, qreal c, qreal d)
{
    qreal root1 = 0;
    qreal root2 = 0;
    qreal root3 = 0;

    qreal m_epsilon = 1e-8;
    int m_maxiter = 100;

    double guess_0; // = -p/2+qSqrt(p*p-q);
    double guess_1; // = -p/2-qSqrt(p*p-q);

    QPair<qreal, qreal> pair = QuadraticRoot(3 * a, 2 * b, c);
    guess_0 = pair.first;
    guess_1 = pair.second;

    //      qDebug() << guess_0 << " " << guess_1;

    double x = guess_0 + 1;
    double y = Cubic::f(x, a, b, c, d);
    int i = 0;
    while (qAbs(y) > m_epsilon) {
        double dy = Cubic::df(x, a, b, c);
        x = x - y / dy;
        y = Cubic::f(x, a, b, c, d);
        //  qDebug() << "x " << x << " y " << y;
        ++i;
        if (i > m_maxiter)
            break;
    }
    root1 = x;
    //     qDebug() << i << "iterations for root 11111";
    x = (guess_0 + guess_1) / 2;
    y = Cubic::f(x, a, b, c, d);
    i = 0;
    while (qAbs(y) > m_epsilon) {
        double dy = Cubic::df(x, a, b, c);
        x = x - y / dy;
        y = Cubic::f(x, a, b, c, d);
        //     qDebug() <<"x " << x << " y " << y;
        ++i;
        if (i > m_maxiter)
            break;
    }
    root2 = x;

    x = guess_1 - 1;
    y = Cubic::df(x, a, b, c);
    //     qDebug() << i << "iterations for root 22222";
    i = 0;
    while (qAbs(y) > m_epsilon) {
        double dy = Cubic::df(x, a, b, c);
        x = x - y / dy;
        y = Cubic::df(x, a, b, c);
        //     qDebug() <<"x " << x << " y " << y;
        ++i;
        if (i > m_maxiter)
            break;
    }
    root3 = x;
    //     qDebug() << i << "iterations for root 33333";
    //     qDebug() << root1 << root2 << root3;
    if (root1 < 0) {
        if (root2 < 0)
            return root3;
        else
            return root2;
    } else if (root2 < 0) {
        if (root1 < 0)
            return root3;
        else
            return root1;
    } else {
        if (root1 < 0)
            return root2;
        else
            return root1;
    }
}

qreal SimpsonIntegrate(qreal lower, qreal upper, std::function<qreal(qreal, const QVector<qreal>)> function, const QVector<qreal>& parameter, qreal delta)
{
    qreal integ = 0;
    int increments = (upper - lower) / delta + 1;

#ifdef openMP
    omp_set_num_threads(qApp->instance()->property("threads").toInt());
#endif

#pragma omp parallel for reduction(+ \
                                   : integ)
    for (int i = 0; i < increments - 1; ++i) {
        double x = lower + i / double(increments);
        qreal b = x + delta;
        integ += (b - x) / 6.0 * (function(x, parameter) + 4 * function((x + b) / 2.0, parameter) + function(b, parameter));
    }
    return integ;
}

std::vector<double> SimpsonIntegrate(qreal lower, qreal upper, const std::vector<std::function<qreal(qreal, const QVector<qreal>)>*>& functions, const QVector<qreal>& parameter, qreal delta)
{
    std::vector<double> integs;
    for (unsigned int k = 0; k < functions.size(); ++k)
        integs.push_back(0);

    int increments = (upper - lower) / delta + 1;
#ifdef openMP
    omp_set_num_threads(qApp->instance()->property("threads").toInt());
#endif

#pragma omp parallel for reduction(vec_double_plus \
                                   : integs)
    for (int i = 0; i < increments - 1; ++i) {
        double x = lower + i / double(increments);
        qreal b = x + delta;
        for (unsigned int k = 0; k < functions.size(); ++k) {
            integs[k] += (b - x) / 6.0 * ((*functions[k])(x, parameter) + 4 * (*functions[k])((x + b) / 2, parameter) + (*functions[k])(b, parameter));
        }
    }
    return integs;
}

qreal DiscreteIntegrate(const QVector<qreal>& x, const QVector<qreal>& y)
{
    if (x.size() == 0 || x.size() != y.size())
        return 0;

    qreal integral = 0;

    std::vector<double> _x;
    std::vector<double> _y;

    for (int i = 0; i < x.size(); ++i) {
        _x.push_back(x[i]);
        _y.push_back(y[i]);
    }

    integral = PeakPick::IntegrateNumerical(_x, _y);

    return integral;
}

qint64 Factorial(qint64 n)
{
    if (n == 0)
        return 1;
    return n * Factorial(n - 1);
}

double LowerLogFermi(double x, double x0, double k, double beta)
{
    return k * log(1 + exp(-beta * (x - x0)));
}

double UpperLogFermi(double x, double x0, double k, double beta)
{
    return k * log(1 + exp(-beta * (x0 - x)));
}

qreal BisectParameter(QWeakPointer<AbstractModel> model, int index, qreal start, qreal end, double epsilon, int maxiter)
{
    QVector<qreal> param = model.toStrongRef()->OptimizeParameters();

    qreal mean = end;
    if (param.size() == 0)
        return mean;
    for (int i = 0; i < maxiter; ++i) {

        if (qAbs(start - end) < epsilon)
            break;

        param[index] = start;
        model.toStrongRef()->setParameter(param);
        model.toStrongRef()->Calculate();
        qreal SSE_0 = model.toStrongRef()->SSE();

        param[index] = end;
        model.toStrongRef()->setParameter(param);
        model.toStrongRef()->Calculate();
        qreal SSE_2 = model.toStrongRef()->SSE();

        mean = (start + end) / 2;
        param[index] = mean;
        model.toStrongRef()->setParameter(param);
        model.toStrongRef()->Calculate();
        qreal SSE_1 = model.toStrongRef()->SSE();

#ifdef _DEBUG
        qDebug() << SSE_0 << SSE_1 << start << end << mean;
#endif
        if (SSE_0 > SSE_1 && SSE_0 > SSE_2) {
            start = mean;
        } else if (SSE_2 > SSE_0 && SSE_2 > SSE_1) {
            end = mean;
        } else {
            start = start + 1;
            end = end + 1;
        }
    }
    return mean;
}
double NewtonRoot(QWeakPointer<AbstractModel> model, int index, qreal min, qreal max, double epsilon, int maxiter)
{
    double x = (min + max) / 2.0;
    double dx = 1e-4;
    QVector<qreal> param = model.toStrongRef()->OptimizeParameters();

    for (int iter = 0; iter < maxiter; ++iter) {
        param[index] = x;
        model.toStrongRef()->setParameter(param);
        model.toStrongRef()->Calculate();
        qreal y = model.toStrongRef()->SSE();

        param[index] = x + dx;
        model.toStrongRef()->setParameter(param);
        model.toStrongRef()->Calculate();
        qreal y1 = model.toStrongRef()->SSE();

        param[index] = x - dx;
        model.toStrongRef()->setParameter(param);
        model.toStrongRef()->Calculate();
        qreal y2 = model.toStrongRef()->SSE();

        double dy = (y2 - y1) / (2 * dx);
        if (std::abs(dy) < epsilon)
            break;
        int inner = 0;
        double x2 = x;
        bool loop = true;
        while (loop) {
            x2 = x + dy / (y * pow(10, inner));
            if ((x2 < min || x2 > max))
                inner++;
            else
                loop = false;

            if (inner == 10)
                return x;
        }
        x = x2;
    }
    return x;
}
