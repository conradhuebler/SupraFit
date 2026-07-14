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
 */

#pragma once

#include <vector>

#include <Eigen/Dense>

#include <QtCore/QString>

/**
 * @brief General equilibrium speciation solver based on quasi-Newton (BFGS) minimisation.
 *
 * Computes the free concentrations of the @em components of an arbitrary
 * multi-equilibrium system from their total (analytical) concentrations and the
 * cumulative stability constants of the formed complexes.  Unlike the grid based
 * ConcentrationalPolynomial / EqnConc_2x solvers (which enumerate A_aB_b complexes
 * with @f$a,b\ge 1@f$), the stoichiometry here is supplied as an explicit matrix, so
 * self-aggregation (A2, A3, B2, ...) and mixed complexes of any order are handled
 * uniformly — a homo-dimer A2 is simply a column @f$(2,0)^T@f$.
 *
 * @par Method
 * The optimisation variables are the logarithms of the free component concentrations
 * @f$x_i = \ln s_i@f$ (which enforces @f$s_i>0@f$ automatically).  The complex
 * concentrations are @f$c_j = \beta_j \prod_k s_k^{M_{kj}}@f$.  Mass balance is the
 * stationary point of the strictly convex potential
 * @f[ G(x) = \sum_i \exp(x_i) + \sum_j c_j(x) - \sum_i t_i\,x_i , @f]
 * whose gradient @f$\partial G/\partial x_i = s_i + \sum_j M_{ij} c_j - t_i@f$ is exactly
 * the mass-balance residual and whose Hessian @f$\mathrm{diag}(s_i) + \sum_j c_j\, m_j m_j^{T}@f$ is
 * symmetric positive definite.  Convexity guarantees a single global minimum for any system.
 *
 * @f$G@f$ is minimised with a @b damped @b Newton method: the analytic Hessian above is cheap for the
 * small component counts of titration systems, so each step solves @f$H\,d = -g@f$ (Cholesky) and
 * takes an Armijo-backtracked step. Newton converges quadratically, reaching machine-precision
 * mass balance in a handful of iterations (the earlier BFGS inverse-Hessian update needed dozens and
 * could stall on host-excess points). The class name is kept for compatibility. Claude Generated.
 *
 * @par Reference
 * Method after Daniil O. Soloviev and Christopher A. Hunter, "Musketeer: a software tool
 * for the analysis of titration data", Chem. Sci., 2024, 15, 15299-15310,
 * DOI: 10.1039/d4sc03354j.  The Musketeer implementation uses bounded L-BFGS-B on the free
 * concentrations; the log-space reformulation used here removes the bounds and yields an
 * unconstrained convex problem.
 *
 * Claude Generated.
 */
class BFGSConcentrationSolver {
public:
    BFGSConcentrationSolver() = default;

    /**
     * @brief Set the stoichiometry matrix of the formed complexes.
     * @param M integer matrix, rows = components, columns = complexes.  @f$M_{ij}@f$ is the
     *          stoichiometric coefficient of component @f$i@f$ in complex @f$j@f$.  The free
     *          components are implicit and must @em not be listed as columns.
     */
    void setStoichiometry(const Eigen::MatrixXi& M);

    /**
     * @brief Set the cumulative (overall) stability constants @f$\beta_j@f$ of the complexes.
     * @param beta linear (not log10) constants, one per column of @f$M@f$, same order.
     */
    void setStabilityConstants(const std::vector<double>& beta);

    /** @brief Set the total (analytical) concentration @f$t_i@f$ of every component. */
    void setTotalConcentrations(const std::vector<double>& totals);

    /** @brief Convergence threshold on the relative mass-balance residual (default 1e-10). */
    inline void setConvergeThreshold(double converge) { m_converge = converge; }
    /** @brief Maximum number of iterations (default 200). */
    inline void setMaxIter(int maxiter) { m_maxiter = maxiter; }

    /** @brief Minimisation method used by solve(). Claude Generated. */
    enum class Method {
        LevenbergMarquardt, ///< damped Newton with the analytic Hessian (default: fast + robust)
        BFGS ///< legacy quasi-Newton inverse-Hessian update (slower; may stall when ill-conditioned)
    };
    /** @brief Select the minimisation method (default LevenbergMarquardt). */
    inline void setMethod(Method method) { m_method = method; }
    inline Method method() const { return m_method; }
    /** @brief Parse a method name ("levmar"/"newton"/"lm" or "bfgs"); defaults to LevenbergMarquardt. */
    static Method MethodFromString(const QString& name);
    static QString MethodToString(Method method);

    /** @brief Build an initial guess for the free concentrations from the totals. */
    void Guess();

    /** @brief Cold-start free-concentration magnitude from the totals (used by Guess() and, in solve(),
     * to seed components that just became active during a warm-started sweep). Claude Generated. */
    double GuessStart() const;

    /** @brief Concentration of complex @p j from the current free concentrations: c_j = β_j ∏_i s_i^{M_ij};
     * 0 if the complex is disabled (β ≤ 0) or a required free component is non-positive. Claude Generated. */
    double speciesConcentration(int j) const;

    /**
     * @brief Solve for the free component concentrations.
     * @return vector of length n_components with the free concentrations @f$s_i@f$.
     *
     * Warm-starts from the current free concentrations (previous solve or Guess()), which
     * makes sweeps along a titration cheap.
     */
    std::vector<double> solve();

    /** @brief The free component concentrations of the last solve. */
    inline std::vector<double> currentConcentration() const { return m_free; }

    /** @brief Inject a warm start (e.g. this data point's solution from the previous outer iteration).
     * Ignored if the size does not match the current totals (a stale/structural mismatch). Must be called
     * after setTotalConcentrations(). Claude Generated. */
    void setWarmStart(const std::vector<double>& free);

    /**
     * @brief All species concentrations: the n free components followed by the m complexes
     *        (in column order of @f$M@f$).
     */
    std::vector<double> AllConcentrations() const;

    /**
     * @brief Analytic parameter sensitivities of the free (log-)concentrations, @f$S_{ij} =
     *        \partial x_i / \partial \ln\beta_j@f$ (components × species), from the implicit-function
     *        theorem on the converged mass balance, reusing the solution Hessian. Only exact for the
     *        LevenbergMarquardt (Newton) method. Underpins the analytic outer fit Jacobian (VarPro).
     *        Claude Generated.
     */
    Eigen::MatrixXd sensitivityMatrix() const;

    inline int LastIterations() const { return m_lastIter; }
    inline double LastConvergency() const { return m_lastConv; }
    inline bool Converged() const { return m_converged; }
    /** @brief Wall-clock time of the last solve() in microseconds. */
    inline long long Timer() const { return m_time; }

private:
    /** @brief Objective @f$G(x)@f$ and, optionally, its gradient and (analytic) Hessian at the
     * log-concentrations @p x. The Hessian @f$\mathrm{diag}(s_i) + \sum_j c_j m_j m_j^T@f$ is used for
     * the damped-Newton step. Claude Generated. */
    double Objective(const Eigen::VectorXd& x, Eigen::VectorXd* gradient, Eigen::MatrixXd* hessian = nullptr) const;

    Eigen::MatrixXi m_M; ///< stoichiometry (components x complexes)
    std::vector<double> m_beta; ///< stability constants per complex
    std::vector<double> m_logbeta; ///< log(beta_j), precomputed (only valid where beta_j > 0)
    std::vector<double> m_totals; ///< total component concentrations
    std::vector<double> m_free; ///< free component concentrations (result / warm start)
    std::vector<bool> m_active; ///< component present (total > 0)
    std::vector<int> m_comp_of_var; ///< active-variable index -> component row (per solve)
    std::vector<int> m_var_of_comp; ///< component row -> active-variable index, -1 if inactive
    Eigen::MatrixXd m_H; ///< mass-balance Hessian at the solution (active dims); exact for Newton only

    Method m_method = Method::LevenbergMarquardt;
    double m_converge = 1e-10;
    double m_lastConv = 0.0;
    int m_maxiter = 200;
    int m_lastIter = 0;
    long long m_time = 0;
    bool m_converged = false;
    bool m_has_guess = false;
};
