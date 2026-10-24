/*
 * SupraFit - metadynamics-like sampling of a least-squares landscape
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * Claude Generated (roadmap/landscape_sampling.md, WP-A). No Qt and no model dependency: the landscape is
 * a plain function f(theta), so the sampler is testable on analytic surfaces.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

/*! \brief Settings of the landscape sampler (Claude Generated).
 *
 * SSE plays the role of the energy, so `temperature` and `biasTemperature` are in SSE units and carry no
 * physical meaning; `NaturalTemperature` gives the scale at which exp(-SSE/kT) is the likelihood.
 * Lengths (`stepSize`, `hillWidth`, `gradientStep`) are fractions of the box width of each variable.
 */
struct LandscapeSettings {
    enum class Sampler { Metropolis, Langevin };

    Sampler sampler = Sampler::Metropolis;
    double temperature = 1.0; ///< kT in SSE units
    /*! Metropolis: proposal sigma. Langevin: step of the explicit Euler-Maruyama update, which is only stable
     *  for stepSize^2 width^2 k / (2 kT) < ~1 with k the largest curvature of SSE; too large a step shows as
     *  an acceptance near zero (all moves land behind the wall). */
    double stepSize = 0.05;
    double gradientStep = 1e-3; ///< Langevin: finite-difference step of the SSE gradient
    bool centralGradient = false; ///< Langevin: central instead of forward differences (2N instead of N evaluations)

    double hillHeight = 0.5; ///< height w of a Gaussian hill in SSE units; 0 switches the bias off
    double hillWidth = 0.05; ///< sigma of the hills
    double biasTemperature = 10.0; ///< well-tempered dT in SSE units, h = w exp(-V/dT); <= 0: plain metadynamics
    int stride = 20; ///< steps between two hills
    int biasBins = 64; ///< bias grid nodes per variable (up to 3 variables; more variables use a hill list)

    double sseCut = std::numeric_limits<double>::infinity(); ///< wall: moves with SSE above it are rejected
    int minSteps = 2000; ///< no termination by patience before this
    int maxSteps = 200000; ///< hard cap
    int patience = 100; ///< hills in a row without a new cell or a better minimum that end the run
    /*! Patience only counts while the bias around the walker has converged: the hill height h = w exp(-V/dT) is
     *  below this fraction of w. Without it a basin that is fully visited but not yet filled ends the run before
     *  the walker can climb out. The bias then has to reach V = -dT ln(fraction), so dT sets the barrier height
     *  a run can take down (about 2.3 dT for 0.1). Not applicable without well-tempering (dT <= 0). */
    double minHillFraction = 0.1;
    double minimumTolerance = 1e-4; ///< relative improvement of the best SSE that counts as new

    int bins = 50; ///< resolution of coverage, profile and minimum candidates per variable
    uint64_t seed = 1;
};

/*! \brief Lowest SSE seen in a cell of the sampling box, a candidate for a minimum when no neighbour is lower. */
struct LandscapeCell {
    std::vector<double> x;
    double sse = std::numeric_limits<double>::infinity();
};

/*! \brief Outcome of one run (Claude Generated). */
struct LandscapeResult {
    enum class Stop { Patience, MaxSteps, Interrupted, StartAboveCut };

    Stop reason = Stop::MaxSteps;
    long steps = 0;
    long evaluations = 0; ///< all calls of f, gradients included
    double acceptance = 0; ///< accepted fraction of the proposed moves
    int hills = 0;
    double lastHillHeight = 0; ///< shrinks with the deposited bias for the well-tempered case
    double maxAcceptedSSE = 0; ///< never above sseCut (the wall)

    std::vector<double> best;
    double bestSSE = std::numeric_limits<double>::infinity();
    long cellsVisited = 0;

    /*! profile[i][b]: lowest SSE seen with variable i in bin b (infinity: not visited); the minimum over all
     *  other variables, which is what a likelihood-based confidence interval needs (not a marginal). */
    std::vector<std::vector<double>> profile;
    /*! Cells whose lowest SSE is below that of every visited neighbour, best first. Candidates only: polish
     *  them with the minimiser before reporting them as minima. Empty for more than 6 variables. */
    std::vector<LandscapeCell> minima;
};

/*! \brief Well-tempered metadynamics-like walker on a function f(theta) inside a box (Claude Generated).
 *
 * A history-dependent bias V (Gaussian hills at the visited points) is added to the "energy" SSE, so the walker
 * leaves basins it has filled. Moves go through Metropolis acceptance or a Langevin update with a
 * finite-difference gradient; moves outside the box or above `sseCut` are rejected (wall). The run ends
 * when new hills stop opening up new cells or a better minimum (`patience`), not at the first event, so small
 * hills do not end it early.
 */
class LandscapeSampler {
public:
    using Function = std::function<double(const std::vector<double>&)>;

    /*! \param f landscape; non-finite values count as "above the wall" */
    LandscapeSampler(Function f, std::vector<double> lower, std::vector<double> upper);
    ~LandscapeSampler();

    void setSettings(const LandscapeSettings& settings) { m_settings = settings; }
    const LandscapeSettings& Settings() const { return m_settings; }
    /*! \brief Polled once per step; returning true ends the run. */
    void setInterrupt(std::function<bool()> interrupt) { m_interrupt = std::move(interrupt); }

    /*! \brief Runs from `start` (clamped into the box). Throws std::invalid_argument for unusable settings. */
    LandscapeResult Run(const std::vector<double>& start);

    /*! \brief Bias V(x) deposited by the last run. */
    double BiasAt(const std::vector<double>& x) const;
    /*! \brief Free-energy estimate -(T + dT)/dT V(x) of the last run (-V without well-tempering). */
    double FreeEnergyAt(const std::vector<double>& x) const;

    /*! \brief kT = 2 sigma^2 with sigma^2 = SSE_min/(n - p): exp(-SSE/kT) is then the likelihood. */
    static double NaturalTemperature(double sseMin, int datapoints, int parameters);

private:
    class Bias;

    Function m_f;
    std::vector<double> m_lower, m_upper;
    LandscapeSettings m_settings;
    std::function<bool()> m_interrupt;
    std::unique_ptr<Bias> m_bias;
};
