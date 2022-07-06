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

#pragma once

#include <QObject>

#include "src/core/libmath.h"
#include "src/core/toolset.h"

class ConcentrationalPolynomial : public QObject {
    Q_OBJECT
public:
    ConcentrationalPolynomial(QObject* parent = nullptr);

    inline void setMaxIter(int maxiter) { m_maxiter = maxiter; }
    inline void setStabilityConstants(const std::vector<double>& vector)
    {
        m_stability_constants = vector;
    }

    inline void setConvergeThreshold(double converge) { m_converge = converge; }
    inline void setStoichiometry(int A, int B)
    {
        m_A = A;
        m_B = B;
    }

    inline void setInitialConcentrations(double A, double B)
    {
        m_A0 = A;
        m_B0 = B;
    }

    inline int LastIterations() const { return m_lastIter; }
    inline double LastConvergency() const { return m_lastConv; }
    std::vector<double> currentConcentration() const { return std::vector<double>() = { m_cA, m_cB }; }
    Vector FillAVector();
    Vector FillBVector();
    void Guess();
    inline bool Converged() const { return m_converged; }
    std::vector<double> solver();

    inline int Index(int a, int b) const { return (a - 1) * m_B + (b - 1); }

    void printConcentration(int iter) const
    {
        std::cout << iter << " " << m_cA << " " << m_cB << std::endl;
    }
    inline void powA()
    {
        double tmp = m_cA;
        for (int a = 0; a < m_A; ++a) {
            m_powA[a] = tmp;
            tmp *= m_cA;
        }
    }

    inline void powB()
    {
        double tmp = m_cB;
        for (int b = 0; b < m_B; ++b) {
            m_powB[b] = tmp;
            tmp *= m_cB;
        }
    }
    inline int Timer() const { return m_time; }
signals:

private:
    double Bisection(double min, double max, const std::vector<double>& polynom);
    double fPolynom(const std::vector<double>& polynom, double x);

    double m_A0 = 0, m_B0 = 0;
    double m_cA = 0, m_cB = 0;
    double m_converge = 1e-10;
    int m_A = 0, m_B = 0;
    int m_maxiter = 1000;
    int m_time = 0;
    int m_lastIter = 0;
    double m_lastConv = 0.0;
    std::vector<double> m_stability_constants;
    std::vector<double> m_current_concentration;
    std::vector<double> m_powA, m_powB;
    bool m_converged = false;
};
