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

BFGSConcentrationSolver::Method BFGSConcentrationSolver::MethodFromString(const QString& name)
{
    return name.trimmed().compare(QStringLiteral("BFGS"), Qt::CaseInsensitive) == 0
        ? Method::BFGS
        : Method::LevenbergMarquardt; // "LevMar"/"Newton"/"LM"/"" -> default
}

QString BFGSConcentrationSolver::MethodToString(Method method)
{
    return method == Method::BFGS ? QStringLiteral("BFGS") : QStringLiteral("LevMar");
}

void BFGSConcentrationSolver::setStoichiometry(const Eigen::MatrixXi& M)
{
    m_M = M;
    m_has_guess = false;
}

void BFGSConcentrationSolver::setStabilityConstants(const std::vector<double>& beta)
{
    m_beta = beta;
    // Precompute log(beta) once instead of in every Objective() call. Deactivated complexes
    // (beta == 0) are skipped in Objective(), so their log value is never read. Claude Generated.
    m_logbeta.resize(beta.size());
    for (std::size_t j = 0; j < beta.size(); ++j)
        m_logbeta[j] = beta[j] > 0.0 ? std::log(beta[j]) : 0.0;
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

double BFGSConcentrationSolver::Objective(const Eigen::VectorXd& x, Eigen::VectorXd* gradient, Eigen::MatrixXd* hessian) const
{
    // x holds the log free concentrations of the active components. The active-variable <-> component
    // maps (m_comp_of_var / m_var_of_comp) and log(beta) are precomputed by the caller / setters, so
    // this hot function allocates nothing. Claude Generated.
    const int m = static_cast<int>(m_M.cols());
    const std::vector<int>& comp_of_var = m_comp_of_var;
    const std::vector<int>& var_of_comp = m_var_of_comp;
    const int nv = static_cast<int>(x.size());

    double G = 0.0;
    if (gradient)
        gradient->setZero(nv);
    if (hessian)
        hessian->setZero(nv, nv);

    // free-component contribution: G += exp(x_v); grad_v += s_v; Hess_vv += s_v
    for (int v = 0; v < nv; ++v) {
        const double s = std::exp(x(v));
        G += s;
        if (gradient)
            (*gradient)(v) += s;
        if (hessian)
            (*hessian)(v, v) += s;
    }

    // complexes: c_j = exp(log beta_j + sum_i M_ij x_i); grad_a += M_aj c_j; Hess_ab += M_aj M_bj c_j
    for (int j = 0; j < m; ++j) {
        if (!(m_beta[j] > 0))
            continue; // deactivated complex (beta == 0)
        bool formable = true;
        double exponent = m_logbeta[j];
        for (int a = 0; a < nv && formable; ++a) {
            const int coeff = m_M(comp_of_var[a], j);
            if (coeff)
                exponent += coeff * x(a);
        }
        // a complex needing an absent component cannot form
        for (int i = 0; i < static_cast<int>(m_active.size()) && formable; ++i)
            if (m_M(i, j) != 0 && !m_active[i])
                formable = false;
        if (!formable)
            continue;
        const double c = std::exp(exponent);
        G += c;
        if (gradient || hessian) {
            for (int a = 0; a < nv; ++a) {
                const int ca = m_M(comp_of_var[a], j);
                if (ca == 0)
                    continue;
                if (gradient)
                    (*gradient)(a) += ca * c;
                if (hessian) {
                    (*hessian)(a, a) += double(ca) * ca * c;
                    for (int b = a + 1; b < nv; ++b) {
                        const int cb = m_M(comp_of_var[b], j);
                        if (cb) {
                            const double h = double(ca) * cb * c;
                            (*hessian)(a, b) += h;
                            (*hessian)(b, a) += h;
                        }
                    }
                }
            }
        }
    }

    // linear term  - sum t_i x_i (does not affect the Hessian)
    for (int v = 0; v < nv; ++v) {
        const int i = comp_of_var[v];
        G -= m_totals[i] * x(v);
        if (gradient)
            (*gradient)(v) -= m_totals[i];
    }
    (void)var_of_comp;
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

    // Active-variable <-> component maps, built once here and reused (allocation-free) by Objective().
    m_comp_of_var.clear();
    m_var_of_comp.assign(n, -1);
    for (int i = 0; i < n; ++i) {
        if (m_active[i]) {
            m_var_of_comp[i] = static_cast<int>(m_comp_of_var.size());
            m_comp_of_var.push_back(i);
        }
    }
    const int nv = static_cast<int>(m_comp_of_var.size());

    if (nv == 0) {
        m_converged = true;
        m_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t_start).count();
        return m_free;
    }

    Eigen::VectorXd x(nv);
    for (int v = 0; v < nv; ++v) {
        const double s = m_free[m_comp_of_var[v]];
        x(v) = std::log(s > 0 ? s : 1e-30);
    }

    Eigen::VectorXd g(nv);
    Eigen::MatrixXd H(nv, nv);
    double f = Objective(x, &g, &H);

    if (m_method == Method::BFGS) {
        // Legacy quasi-Newton: BFGS inverse-Hessian update + Armijo line search. Kept as a selectable
        // alternative (setMethod) for comparison; Levenberg-Marquardt Newton (below) is the faster and
        // more robust default. Claude Generated.
        Eigen::MatrixXd Hinv = Eigen::MatrixXd::Identity(nv, nv);
        bool hIsIdentity = true;
        const double c1 = 1e-4, rho = 0.5;
        for (m_lastIter = 0; m_lastIter < m_maxiter; ++m_lastIter) {
            double conv = 0.0;
            for (int v = 0; v < nv; ++v) {
                const double t = m_totals[m_comp_of_var[v]];
                conv = std::max(conv, std::abs(g(v)) / (t > 0 ? t : 1.0));
            }
            m_lastConv = conv;
            if (conv < m_converge) {
                m_converged = true;
                break;
            }
            Eigen::VectorXd p = -Hinv * g;
            double gp = g.dot(p);
            if (gp >= 0) { // not a descent direction -> steepest descent
                Hinv.setIdentity();
                hIsIdentity = true;
                p = -g;
                gp = g.dot(p);
            }
            double alpha = 1.0, f_new = f;
            Eigen::VectorXd x_new = x, g_new = g;
            bool ls_ok = false;
            for (int ls = 0; ls < 60; ++ls) {
                x_new = x + alpha * p;
                f_new = Objective(x_new, &g_new, nullptr);
                if (f_new <= f + c1 * alpha * gp) {
                    ls_ok = true;
                    break;
                }
                alpha *= rho;
            }
            if (!ls_ok) { // line search stalled: restart steepest, else stop
                if (!hIsIdentity) {
                    Hinv.setIdentity();
                    hIsIdentity = true;
                    continue;
                }
                break;
            }
            const Eigen::VectorXd s_vec = x_new - x;
            const Eigen::VectorXd y_vec = g_new - g;
            const double sy = s_vec.dot(y_vec);
            if (sy > 1e-14) {
                const double r = 1.0 / sy;
                const Eigen::MatrixXd I = Eigen::MatrixXd::Identity(nv, nv);
                Hinv = (I - r * s_vec * y_vec.transpose()) * Hinv * (I - r * y_vec * s_vec.transpose())
                    + r * s_vec * s_vec.transpose();
                hIsIdentity = false;
            }
            x = x_new;
            f = f_new;
            g = g_new;
        }
    } else {
        // Levenberg-Marquardt damping: solve (H + lambda*I) d = -g. lambda self-tunes per system and
        // per iteration -> shrinks toward 0 (full Newton, quadratic convergence) when steps succeed,
        // grows (small, gradient-like, guaranteed-descent steps) when the Hessian is ill-conditioned
        // or we are far from the solution. Robust across weak/strong/competitive binding without any
        // fragile fixed step cap, and preserves Newton's quadratic tail near the optimum. Claude Generated.
        double lambda = 1e-6;
        for (m_lastIter = 0; m_lastIter < m_maxiter; ++m_lastIter) {
            // convergence: relative mass-balance residual (gradient == residual)
            double conv = 0.0;
            for (int v = 0; v < nv; ++v) {
                const double t = m_totals[m_comp_of_var[v]];
                conv = std::max(conv, std::abs(g(v)) / (t > 0 ? t : 1.0));
            }
            m_lastConv = conv;
            if (conv < m_converge) {
                m_converged = true;
                break;
            }

            const double hscale = H.diagonal().maxCoeff() + 1e-300;
            const double gsq = g.squaredNorm();
            Eigen::VectorXd g_try(nv);
            bool stepTaken = false;
            for (int tries = 0; tries < 50; ++tries) {
                Eigen::MatrixXd Hr = H;
                const double damp = lambda * hscale;
                for (int k = 0; k < nv; ++k)
                    Hr(k, k) += damp;
                Eigen::LLT<Eigen::MatrixXd> llt(Hr);
                const Eigen::VectorXd d = (llt.info() == Eigen::Success)
                    ? Eigen::VectorXd(llt.solve(-g))
                    : Eigen::VectorXd(-g / hscale);
                const double f_new = Objective(x + d, &g_try, nullptr);
                // Accept on a decrease of the objective (globalisation) OR of the gradient norm
                // (root-finding). The gradient branch lets undamped Newton steps drive the mass-balance
                // residual to machine precision even where the convex objective is already flat, which
                // pure f-decrease could not resolve below ~sqrt(eps). Claude Generated.
                if (f_new < f || g_try.squaredNorm() < gsq) {
                    x += d;
                    f = f_new;
                    g = g_try;
                    lambda = std::max(lambda * 0.1, 1e-14); // reward -> quickly toward pure Newton
                    stepTaken = true;
                    break;
                }
                lambda *= 4.0; // reject -> damp harder (smaller, safer step)
                if (lambda > 1e12)
                    break; // damped to a standstill -> numerical optimum
            }
            if (!stepTaken)
                break;

            Objective(x, &g, &H); // refresh gradient + Hessian at the accepted point for the next step
        }
    }

    m_free.assign(n, 0.0);
    for (int v = 0; v < nv; ++v)
        m_free[m_comp_of_var[v]] = std::exp(x(v));

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
