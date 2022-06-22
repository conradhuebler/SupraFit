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

#include <unsupported/Eigen/Polynomials>

#include <QtCore/QDebug>

#include "concentrationalpolynomial.h"

ConcentrationalPolynomial::ConcentrationalPolynomial(QObject* parent)
    : QObject{ parent }
{
    m_current_concentration = Vector(2);
}

void ConcentrationalPolynomial::Guess()
{
    m_cA = std::min(m_A0, m_B0) / 10.0;
    m_cB = m_cA;
    m_current_concentration(0) = m_cA;
    m_current_concentration(1) = m_cB;

    // std::cout << "## 00 ## " << m_A0 << " " << m_B0 << std::endl;
    // std::cout << "## 11 ## " << m_cA << " " << m_cB << std::endl;
}

Vector ConcentrationalPolynomial::FillAVector()
{
    Vector coeffs(m_A + 1);
    for (int a = 1; a <= m_A; ++a) {
        coeffs(a) = 0;
        for (int b = 1; b <= m_B; b++) {
            double beta = m_stability_constants(Index(a, b));
            coeffs(a) += a * beta * pow(m_cB, b);
        }
    }
    coeffs(0) = -m_A0;
    coeffs(1) += 1;
    return coeffs;
}

Vector ConcentrationalPolynomial::FillBVector()
{
    Vector coeffs(m_B + 1);
    for (int b = 1; b <= m_B; b++) {
        coeffs(b) = 0;
        for (int a = 1; a <= m_A; ++a) {

            double beta = m_stability_constants(Index(a, b));
            coeffs(b) += b * beta * pow(m_cA, a);
        }
    }
    coeffs(0) = -m_B0;
    coeffs(1) += 1;
    return coeffs;
}

Vector ConcentrationalPolynomial::solver()
{
    if (m_A0 < m_converge || m_B0 < m_converge)
        return Eigen::Vector2d(m_A0, m_B0);
    Guess();

    Vector coeffs_a, coeffs_b;
    // Eigen::PolynomialSolver<double, Eigen::Dynamic> solver;
    // bool hasit;
    // double thresh;
    for (int iter = 0; iter < m_maxiter; ++iter) {
        // printConcentration(iter);
        coeffs_a = FillAVector();

        // solver.compute(coeffs_a);

        // const Eigen::PolynomialSolver<double, Eigen::Dynamic>::RootsType& r1 = solver.roots();

        // std::cout << solver.absSmallestRealRoot(hasit, thresh) << std::endl;

        m_cA = Bisection(0, m_A0, coeffs_a); // solver.greatestRealRoot(hasit, thresh);

        // qDebug() << Bisection(0, m_A0, coeffs_a);
        // qDebug() << m_cA;

        coeffs_b = FillBVector();
        // solver.compute(coeffs_b);
        //  std::cout << coeffs_a.transpose() << "\n"
        //            << coeffs_b.transpose() << std::endl;

        // const Eigen::PolynomialSolver<double, Eigen::Dynamic>::RootsType& r2 = solver.roots();
        // std::cout << r1 << r2 << std::endl;

        // std::cout << solver.absSmallestRealRoot(hasit, thresh) << std::endl;
        m_cB = Bisection(0, m_B0, coeffs_b); // solver.greatestRealRoot(hasit, thresh);

        if ((abs(m_current_concentration(0) - m_cA) + abs(m_current_concentration(1) - m_cB)) < m_converge) {
            return currentConcentration();
        }
        m_current_concentration(0) = m_cA;
        m_current_concentration(1) = m_cB;
    }
    return m_current_concentration;
}

double ConcentrationalPolynomial::Bisection(double min, double max, const Vector& polynom)
{
    double mean = (min + max) / 2.0;
    double y_min = 0;
    double y_max = 0;
    double result = mean;

    // std::cout << mean << " ... " <<polynom.transpose() << std::endl;
    // std::cout << fPolynom(polynom, mean) << std::endl;
    // y_min = fPolynom(polynom, min);

    // y_max = fPolynom(polynom, max);
    // std::cout << y_min << " --- "  << y_max << std::endl;
    for (int i = 0; i < 20; ++i) {
        mean = (min + max) / 2.0;
        y_min = fPolynom(polynom, min);

        y_max = fPolynom(polynom, max);
        double bisect = fPolynom(polynom, mean);
        if (std::signbit(y_min) == std::signbit(bisect))
            min = mean;
        else if (std::signbit(y_max) == std::signbit(bisect))
            max = mean;
        // if(std::abs(result - mean) < m_converge)
        //     return result;
        result = mean;
        // std::cout << mean << " ";
    }
    // std::cout << std::endl;
    return result;
}

double ConcentrationalPolynomial::fPolynom(const Vector& polynom, double x)
{
    // std::cout << x << " " << polynom.transpose() << std::endl;

    double result = 0.0;
    for (int i = 0; i < polynom.size(); ++i)
        result += polynom(i) * pow(x, i);
    return result;
}
