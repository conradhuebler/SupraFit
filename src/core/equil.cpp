/*
  * Tools to calculate Equilibrium Concentrations for different models
  * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QDateTime>
#include <QtCore/QThread>

#include <iostream>

#include "src/global_config.h"
#include "equil.h"


#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/NonLinearOptimization>

int MyScripteEqualSystem::operator()(const Eigen::VectorXd& parameter, Eigen::VectorXd& fvec) const
{
    qreal A = parameter(0);
    qreal B = parameter(1);

    Vector balance(2);

    qreal complex_21 = K11 * K21 * A * A * B;
    qreal complex_11 = K11 * A * B;
    qreal complex_12 = K11 * K12 * A * B * B;

    balance(0) = (2 * complex_21 + complex_11 + complex_12);
    balance(1) = (complex_21 + complex_11 + 2 * complex_12);

    fvec = parameter + balance - Concen_0;

    return 0;
}


IItoI_ItoI_ItoII_Solver::IItoI_ItoI_ItoII_Solver()
    : m_ok(false)
{
    setAutoDelete(false);
}

IItoI_ItoI_ItoII_Solver::~IItoI_ItoI_ItoII_Solver()
{
}

void IItoI_ItoI_ItoII_Solver::setInput(double A0, double B0)
{
    m_A0 = A0;
    m_B0 = B0;
    m_concentration = QPair<double, double>(A0, B0);
}

void IItoI_ItoI_ItoII_Solver::run()
{
    if (m_A0 && m_B0)
#ifndef legacy
        m_concentration = HostConcentration(m_A0, m_B0);
#else
        m_concentration = LegacyHostConcentration(m_A0, m_B0);
#endif
    else
        m_ok = true;
}

void IItoI_ItoI_ItoII_Solver::RunTest()
{
    m_concentration = HostConcentration(m_A0, m_B0);

    m_concentration_legacy = LegacyHostConcentration(m_A0, m_B0);
}

QPair<double, double> IItoI_ItoI_ItoII_Solver::HostConcentration(double a0, double b0)
{
    if (!a0 || !b0)
        return QPair<double, double>(a0, b0);

    qint64 t0 = QDateTime::currentMSecsSinceEpoch();
    qreal K21 = m_parameter[0];
    qreal K11 = m_parameter[1];
    qreal K12 = m_parameter[2];
    qreal b12 = K11 * K12;
    qreal b21 = K11 * K21;

    auto calc_a = [](double a0, long double b, double K11, double b21, double b12) {
        long double x1 = 2 * b21 * b;
        long double x2 = b12 * b * b + K11 * b + 1;
        long double x3 = -a0;
        long double a = MaxQuadraticRoot(x1, x2, x3);
        if (a < a0)
            return a;
        else {
#ifdef _DEBUG
        //  std::cout << "a: " << a << " a0 " << a0 << std::endl;
#endif
            return MinQuadraticRoot(x1, x2, x3);
        }
    };

    auto calc_b = [](double b0, long double a, double K11, double b21, double b12) {
        long double x1 = 2 * b12 * a;
        long double x2 = b21 * a * a + K11 * a + 1;
        long double x3 = -b0;
        long double b = MaxQuadraticRoot(x1, x2, x3);
        if (b < b0)
            return b;
        else {
#ifdef _DEBUG
        //   std::cout << "b: " << b << " b0: " << b0 << std::endl;
#endif
            return MinQuadraticRoot(x1, x2, x3);
        }
    };
    long double epsilon = m_opt_config.concen_convergency;
    long double a = qMin(a0, b0) / K11 * 10;
    long double b = 0;
    long double a_1 = 0, b_1 = 0;
    int i = 0;
    for (i = 0; i < m_opt_config.single_iter; ++i) {
        a_1 = a;
        b_1 = b;
        b = calc_b(b0, a, K11, b21, b12);
        if (b < 0)
            b *= -1;

        a = calc_a(a0, b, K11, b21, b12);
        if (a < 0)
            a *= -1;
        if (qAbs(b21 * a_1 * a_1 * b_1 - b21 * a * a * b) < epsilon && qAbs(b12 * a_1 * b_1 * b_1 - b12 * a * b * b) < epsilon && qAbs(K11 * a_1 * b_1 - K11 * a * b) < epsilon)
            break;
    }
#ifdef _DEBUG
/*
    std::cout << std::endl
              << std::endl;
    std::cout << a_1 << " " << b_1 << " " << K11 * a_1 * b_1 << " " << b21 * a_1 * a_1 * b_1 << " " << b12 * a_1 * b_1 * b_1 << std::endl;
    std::cout << a << " " << b << " " << K11 * a * b << " " << b21 * a * a * b << " " << b12 * a * b * b << std::endl;
    std::cout << "Guess A: " << qMin(a0, b0) / K11 * 100 << " .. Final A: " << a << " .. Iterations:" << i << std::endl;
    */
#endif
    m_ok = (a < m_A0) && (b < m_B0) && (a > 0) && (b > 0) && i < m_opt_config.single_iter;
    m_t += QDateTime::currentMSecsSinceEpoch() - t0;

    return QPair<double, double>(a, b);
}

QPair<double, double> IItoI_ItoI_ItoII_Solver::LegacyHostConcentration(double a0, double b0)
{
    if (!a0 || !b0)
        return QPair<double, double>(a0, b0);

    qint64 t0 = QDateTime::currentMSecsSinceEpoch();

    qreal K21 = m_parameter[0];
    qreal K11 = m_parameter[1];
    qreal K12 = m_parameter[2];

    Eigen::VectorXd parameter(2);
    parameter(0) = a0;
    parameter(1) = b0;

    Eigen::VectorXd Concen_0(2);
    Concen_0(0) = a0;
    Concen_0(1) = b0;

    MyScripteEqualSystem functor;

    functor.Concen_0 = Concen_0;

    functor.K11 = K11;
    functor.K21 = K21;
    functor.K12 = K12;

    Eigen::NumericalDiff<MyScripteEqualSystem> numDiff(functor);
    Eigen::LevenbergMarquardt<Eigen::NumericalDiff<MyScripteEqualSystem>> lm(numDiff);
    int iter = 0;
    Eigen::LevenbergMarquardtSpace::Status status = lm.minimizeInit(parameter);
    do {
        for (int i = 0; i < 2; ++i)
            if (parameter(i) < 0) {
                //  std::cout << "numeric error (below zero): " << i << std::endl;
                parameter(i) = qAbs(parameter(i));
            } else if (parameter(i) > Concen_0(i)) {
                //  std::cout << "numeric error (above init): " << i << std::endl;
                qreal diff = (parameter(i) - Concen_0(i));
                parameter(i) = diff;
            }
        status = lm.minimizeOneStep(parameter);
        iter++;
    } while (status == -1);
    for (int i = 0; i < 2; ++i)
        if (parameter(i) < 0 || parameter(i) > Concen_0(i))
            m_lok = false;
    m_lt += QDateTime::currentMSecsSinceEpoch() - t0;

    return QPair<qreal,qreal>(double(parameter(0)), double(parameter(1)));
}

