/*
 * SupraFit - general BFGS equilibrium speciation solver
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * Method after Soloviev & Hunter, "Musketeer", Chem. Sci., 2024, 15, 15299-15310,
 * DOI: 10.1039/d4sc03354j. Claude Generated.
 */

#include <algorithm>
#include <chrono>
#include <cmath>

#include "bfgsconcentrationsolver.h"

void BFGSConcentrationSolver::setStoichiometry(const Eigen::MatrixXi& M)
{
    m_M = M;
    m_has_guess = false;
}

void BFGSConcentrationSolver::setStabilityConstants(const std::vector<double>& beta)
{
    m_beta = beta;
}

void BFGSConcentrationSolver::setTotalConcentrations(const std::vector<double>& totals)
{
    m_totals = totals;
    m_active.assign(totals.size(), true);
    for (std::size_t i = 0; i < totals.size(); ++i)
        m_active[i] = totals[i] > 0.0;
    if (m_free.size() != totals.size())
        m_free.assign(totals.size(), 0.0);
    m_has_guess = false;
}

void BFGSConcentrationSolver::Guess()
{
    const int n = static_cast<int>(m_totals.size());
    m_free.assign(n, 0.0);

    // Smallest positive total, scaled down by the largest complex order, gives a robust
    // low starting free concentration (Musketeer: within ~2 orders of the optimum).
    double min_total = 0.0;
    for (int i = 0; i < n; ++i) {
        if (!m_active[i])
            continue;
        if (min_total == 0.0 || m_totals[i] < min_total)
            min_total = m_totals[i];
    }
    int max_order = 1;
    for (int j = 0; j < m_M.cols(); ++j) {
        int order = 0;
        for (int i = 0; i < m_M.rows(); ++i)
            order += m_M(i, j);
        max_order = std::max(max_order, order);
    }
    const double start = min_total / (10.0 * (max_order + 1));
    for (int i = 0; i < n; ++i)
        m_free[i] = m_active[i] ? start : 0.0;
    m_has_guess = true;
}

double BFGSConcentrationSolver::Objective(const Eigen::VectorXd& x, Eigen::VectorXd* gradient) const
{
    // x holds the log free concentrations of the active components (indexed by their position
    // in x, mapped back to component rows via the caller's active list).
    const int n = static_cast<int>(m_totals.size());
    const int m = static_cast<int>(m_M.cols());

    // Map active variable index -> component row and back.
    // (kept as a small helper array here so Objective stays self-contained)
    std::vector<int> comp_of_var;
    comp_of_var.reserve(x.size());
    std::vector<int> var_of_comp(n, -1);
    for (int i = 0; i < n; ++i) {
        if (m_active[i]) {
            var_of_comp[i] = static_cast<int>(comp_of_var.size());
            comp_of_var.push_back(i);
        }
    }

    double G = 0.0;
    if (gradient) {
        gradient->setZero(x.size());
        // free-component contribution: sum exp(x_i) with gradient exp(x_i)
        for (int v = 0; v < x.size(); ++v) {
            const double s = std::exp(x(v));
            G += s;
            (*gradient)(v) += s;
        }
    } else {
        for (int v = 0; v < x.size(); ++v)
            G += std::exp(x(v));
    }

    // complexes
    for (int j = 0; j < m; ++j) {
        if (!(m_beta[j] > 0))
            continue; // deactivated complex (beta == 0)
        bool formable = true;
        double exponent = std::log(m_beta[j]);
        for (int i = 0; i < n && formable; ++i) {
            const int coeff = m_M(i, j);
            if (coeff == 0)
                continue;
            if (!m_active[i]) {
                formable = false; // needs an absent component -> cannot form
                break;
            }
            exponent += coeff * x(var_of_comp[i]);
        }
        if (!formable)
            continue;
        const double c = std::exp(exponent);
        G += c;
        if (gradient) {
            for (int i = 0; i < n; ++i) {
                const int coeff = m_M(i, j);
                if (coeff != 0)
                    (*gradient)(var_of_comp[i]) += coeff * c;
            }
        }
    }

    // linear term  - sum t_i x_i
    for (int v = 0; v < x.size(); ++v) {
        const int i = comp_of_var[v];
        G -= m_totals[i] * x(v);
        if (gradient)
            (*gradient)(v) -= m_totals[i];
    }
    return G;
}

std::vector<double> BFGSConcentrationSolver::solve()
{
    const auto t_start = std::chrono::steady_clock::now();
    const int n = static_cast<int>(m_totals.size());
    m_converged = false;
    m_lastIter = 0;
    m_lastConv = 0.0;

    if (!m_has_guess)
        Guess();

    // Build the active-variable index map and the starting point x = ln(s).
    std::vector<int> comp_of_var;
    for (int i = 0; i < n; ++i)
        if (m_active[i])
            comp_of_var.push_back(i);
    const int nv = static_cast<int>(comp_of_var.size());

    if (nv == 0) {
        m_converged = true;
        m_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t_start).count();
        return m_free;
    }

    Eigen::VectorXd x(nv);
    for (int v = 0; v < nv; ++v) {
        const double s = m_free[comp_of_var[v]];
        x(v) = std::log(s > 0 ? s : 1e-30);
    }

    Eigen::MatrixXd H = Eigen::MatrixXd::Identity(nv, nv); // inverse-Hessian approximation
    Eigen::VectorXd g(nv);
    double f = Objective(x, &g);

    const double c1 = 1e-4; // Armijo constant
    const double rho = 0.5; // backtracking factor

    for (m_lastIter = 0; m_lastIter < m_maxiter; ++m_lastIter) {
        // convergence: relative mass-balance residual (gradient == residual)
        double conv = 0.0;
        for (int v = 0; v < nv; ++v) {
            const double t = m_totals[comp_of_var[v]];
            conv = std::max(conv, std::abs(g(v)) / (t > 0 ? t : 1.0));
        }
        m_lastConv = conv;
        if (conv < m_converge) {
            m_converged = true;
            break;
        }

        Eigen::VectorXd p = -H * g; // search direction
        double gp = g.dot(p);
        if (gp >= 0) { // not a descent direction -> reset to steepest descent
            H.setIdentity();
            p = -g;
            gp = g.dot(p);
        }

        double alpha = 1.0;
        double f_new = f;
        Eigen::VectorXd x_new = x;
        Eigen::VectorXd g_new = g;
        for (int ls = 0; ls < 60; ++ls) {
            x_new = x + alpha * p;
            f_new = Objective(x_new, &g_new);
            if (f_new <= f + c1 * alpha * gp)
                break;
            alpha *= rho;
        }

        const Eigen::VectorXd s_vec = x_new - x;
        const Eigen::VectorXd y_vec = g_new - g;
        const double sy = s_vec.dot(y_vec);
        if (sy > 1e-14) { // BFGS inverse-Hessian update (skip on tiny curvature)
            const double r = 1.0 / sy;
            const Eigen::MatrixXd I = Eigen::MatrixXd::Identity(nv, nv);
            H = (I - r * s_vec * y_vec.transpose()) * H * (I - r * y_vec * s_vec.transpose())
                + r * s_vec * s_vec.transpose();
        }

        x = x_new;
        f = f_new;
        g = g_new;
    }

    m_free.assign(n, 0.0);
    for (int v = 0; v < nv; ++v)
        m_free[comp_of_var[v]] = std::exp(x(v));

    m_has_guess = true; // keep result as warm start for the next solve
    m_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t_start).count();
    return m_free;
}

std::vector<double> BFGSConcentrationSolver::AllConcentrations() const
{
    const int n = static_cast<int>(m_totals.size());
    const int m = static_cast<int>(m_M.cols());
    std::vector<double> all(n + m, 0.0);
    for (int i = 0; i < n; ++i)
        all[i] = m_free[i];
    for (int j = 0; j < m; ++j) {
        if (!(m_beta[j] > 0)) {
            all[n + j] = 0.0;
            continue;
        }
        double c = m_beta[j];
        bool formable = true;
        for (int i = 0; i < n; ++i) {
            const int coeff = m_M(i, j);
            if (coeff == 0)
                continue;
            if (m_free[i] <= 0.0 && coeff > 0) {
                formable = false;
                break;
            }
            c *= std::pow(m_free[i], coeff);
        }
        all[n + j] = formable ? c : 0.0;
    }
    return all;
}
