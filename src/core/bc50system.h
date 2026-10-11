/*
 * SupraFit - equilibrium-system descriptor a model hands to the BC50 machinery
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

#include <QtCore/QVector>

namespace BC50 {

/*! \brief A model's equilibrium system, in the single form the BC50 dispatch consumes.
 *
 * Every titration model — the fixed-stoichiometry ones and the reaction-driven `*_any` ones —
 * describes its binding system to the BC50 machinery through this one struct, so there is a single
 * code path (`BC50::FromSpeciation`, `Statistic::*BC50_Speciation`) instead of one hand-written
 * formula call per stoichiometry. A default-constructed (empty) value means "no BC50 defined for
 * this model", and every entry point treats it as such.
 *
 * \a lgBeta holds CUMULATIVE lg beta, one per species, in the column order of \a stoich — the
 * convention the reaction-driven models already use. The fixed models store STEPWISE binding
 * constants and convert when they build this descriptor. Claude Generated (2026). */
struct ModelSystem {
    Eigen::MatrixXi stoich; //!< components x species; empty => no BC50
    QVector<qreal> lgBeta; //!< cumulative lg beta per species, in the column order of stoich

    bool isEmpty() const { return stoich.size() == 0 || lgBeta.isEmpty(); }
};

} // namespace BC50
