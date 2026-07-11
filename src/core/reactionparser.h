/*
 * SupraFit - live parser for chemical reaction equations
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

#include <Eigen/Core>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

/**
 * @brief One equilibrium species (complex) produced from the free components.
 *
 * @c stoich holds the stoichiometric coefficient of every component (in the order of
 * ReactionSystem::components), so a homo-dimer A2 over components [A,B] is @f$(2,0)^T@f$ and a
 * mixed A2B is @f$(2,1)^T@f$.  Claude Generated.
 */
struct ReactionSpecies {
    QString label; ///< human-readable label with unicode subscripts, e.g. "A₂B"
    Eigen::VectorXi stoich; ///< coefficient per component (length = component count)
};

/** @brief Per-line parse result, used by the GUI for live feedback. Claude Generated. */
struct ReactionDiagnostic {
    int line = 0; ///< 1-based line number in the source text
    bool ok = false; ///< true = parsed (or intentionally skipped), false = error
    QString message; ///< derived label / skip reason / error description
};

/**
 * @brief Result of parsing a block of reaction equations.
 *
 * @c stoich is the (n_components × n_species) integer matrix ready to hand to
 * BFGSConcentrationSolver::setStoichiometry (rows = components, columns = complexes). Component
 * order is the order of first appearance in the text — it also defines the mapping of components to
 * the independent concentration columns of the data set. Claude Generated.
 */
struct ReactionSystem {
    QStringList components; ///< ordered free-component symbols, e.g. ["A","B","C"]
    QVector<ReactionSpecies> species; ///< formed complexes
    Eigen::MatrixXi stoich; ///< components × species stoichiometry matrix
    QVector<ReactionDiagnostic> diagnostics; ///< one entry per non-empty source line
    bool valid = false; ///< at least one species and no fatal per-line error

    /** @brief Encode as the legacy "a,b|a,b" species string (2-component systems only, else ""). */
    QString toLegacySpeciesString() const;
};

/**
 * @brief Parses free-text reaction equations into a component/species stoichiometry system.
 *
 * Syntax: one reaction per line, @c "[coeff] Comp [+ [coeff] Comp ...] <=> Product", where the
 * left-hand side lists the free components consumed and the right-hand side names the complex. The
 * complex stoichiometry is taken from the left-hand side; the product token is only a label hint.
 * Accepted arrows: @c <=> , @c = , @c <-> , @c -> , @c => , @c ⇌. Coefficients are integer prefixes
 * (@c "2 A" or @c "2A"). Blank lines and lines starting with @c '#' or @c "//" are ignored. Free
 * monomers (coefficient sum &lt; 2, e.g. @c "A <=> A") are implicit and skipped.
 *
 * The same parser feeds the live GUI editor (ReactionEditorWidget) and the models' DefineModel().
 * Claude Generated.
 */
class ReactionParser {
public:
    /** @brief Parse @p text into a ReactionSystem (never throws; errors land in diagnostics). */
    static ReactionSystem Parse(const QString& text);

    /** @brief Canonical species label from a stoichiometry vector, e.g. (2,1)/[A,B] -> "A₂B". */
    static QString SpeciesLabel(const QStringList& components, const Eigen::VectorXi& stoich);
};
