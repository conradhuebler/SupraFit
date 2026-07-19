/*
 * SupraFit - equilibrium speciation engine (reaction system + BFGS solver)
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

#include "speciationengine.h"

SpeciationEngine::SpeciationEngine()
{
    m_solver.setMaxIter(1000);
    m_solver.setConvergeThreshold(1e-12);
}

bool SpeciationEngine::setReactions(const QString& text)
{
    setSystem(ReactionParser::Parse(text));
    return m_system.valid;
}

void SpeciationEngine::setSystem(const ReactionSystem& system)
{
    m_system = system;
    // rows = components, cols = species; the solver treats the free components implicitly
    m_solver.setStoichiometry(m_system.stoich);
    m_point_cache.clear(); // component count may have changed -> old per-point warm starts are invalid
}

QString SpeciationEngine::SpeciesLabel(int j) const
{
    return (j >= 0 && j < m_system.species.size()) ? m_system.species[j].label : QString();
}

Eigen::VectorXi SpeciationEngine::SpeciesStoichiometry(int j) const
{
    if (j >= 0 && j < m_system.species.size())
        return m_system.species[j].stoich;
    return Eigen::VectorXi();
}

void SpeciationEngine::setStabilityConstants(const std::vector<double>& beta)
{
    m_solver.setStabilityConstants(beta);
}

void SpeciationEngine::setMaxIter(int maxiter)
{
    m_solver.setMaxIter(maxiter);
}

void SpeciationEngine::setConvergeThreshold(double converge)
{
    m_solver.setConvergeThreshold(converge);
}

std::vector<double> SpeciationEngine::solve(const std::vector<double>& totals)
{
    return solve(totals, -1);
}

std::vector<double> SpeciationEngine::solve(const std::vector<double>& totals, int pointIndex)
{
    // The per-point cache only helps the convergent Newton (LevenbergMarquardt) method, whose solution
    // is independent of the start (strictly convex): each point then seeds from its own previous
    // converged solution - a nearer start than the neighbouring swept point. The legacy BFGS method does
    // NOT fully converge, so caching a stalled point and reusing it just moves (and can slow) the stall;
    // it keeps the mild point-to-point warm start instead. Claude Generated.
    const bool cache = pointIndex >= 0
        && m_solver.method() == ConcentrationSolver::Method::LevenbergMarquardt;

    m_solver.setTotalConcentrations(totals);
    if (cache && pointIndex < static_cast<int>(m_point_cache.size())
        && m_point_cache[pointIndex].size() == totals.size())
        m_solver.setWarmStart(m_point_cache[pointIndex]);

    m_free = m_solver.solve();

    if (cache) {
        if (static_cast<int>(m_point_cache.size()) <= pointIndex)
            m_point_cache.resize(pointIndex + 1);
        m_point_cache[pointIndex] = m_free;
    }

    // AllConcentrations() returns the free components followed by the species (column order of M).
    const std::vector<double> all = m_solver.AllConcentrations();
    const int n = ComponentCount();
    const int m = SpeciesCount();
    m_species_conc.assign(m, 0.0);
    for (int j = 0; j < m; ++j)
        m_species_conc[j] = all[n + j];
    return m_free;
}
