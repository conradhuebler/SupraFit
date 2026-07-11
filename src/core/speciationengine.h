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

#pragma once

#include <vector>

#include <QtCore/QString>
#include <QtCore/QStringList>

#include "src/core/bfgsconcentrationsolver.h"
#include "src/core/reactionparser.h"

/**
 * @brief Bridges a parsed reaction system to the general BFGS speciation solver.
 *
 * The engine is observable-agnostic: it turns reaction text (or an explicit ReactionSystem) into the
 * component/species stoichiometry, then, per data point, solves the mass balance for the free
 * component concentrations and derives every species concentration
 * @f$c_j = \beta_j \prod_k s_k^{M_{kj}}@f$. Titration and ITC models embed one and add only their
 * observable mapping on top. Claude Generated.
 */
class SpeciationEngine {
public:
    SpeciationEngine();

    /** @brief Configure from reaction text; returns true if a valid system was parsed. */
    bool setReactions(const QString& text);
    /** @brief Configure from an already-parsed reaction system. */
    void setSystem(const ReactionSystem& system);

    const ReactionSystem& System() const { return m_system; }
    bool isValid() const { return m_system.valid; }
    int ComponentCount() const { return m_system.components.size(); }
    int SpeciesCount() const { return m_system.species.size(); }
    const QStringList& ComponentNames() const { return m_system.components; }
    QString SpeciesLabel(int j) const;
    /** @brief Stoichiometry vector of species @p j over the components (length ComponentCount()). */
    Eigen::VectorXi SpeciesStoichiometry(int j) const;
    const Eigen::MatrixXi& Stoichiometry() const { return m_system.stoich; }

    /** @brief Linear (not log10) cumulative stability constants, one per species; 0 disables one. */
    void setStabilityConstants(const std::vector<double>& beta);
    void setMaxIter(int maxiter);
    void setConvergeThreshold(double converge);

    /**
     * @brief Solve one data point given the total concentration of every component.
     * @param totals length must equal ComponentCount().
     * @return free component concentrations; species concentrations via SpeciesConcentrations().
     */
    std::vector<double> solve(const std::vector<double>& totals);

    const std::vector<double>& FreeConcentrations() const { return m_free; }
    const std::vector<double>& SpeciesConcentrations() const { return m_species_conc; }
    bool Converged() const { return m_solver.Converged(); }

private:
    ReactionSystem m_system;
    BFGSConcentrationSolver m_solver;
    std::vector<double> m_free; ///< free component concentrations of the last solve
    std::vector<double> m_species_conc; ///< species concentrations of the last solve
};
