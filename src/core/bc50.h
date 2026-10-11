/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

// BC50 (concentration at 50 % binding) API. Declarations only — the solver/
// integration bodies were de-inlined into bc50.cpp (Claude Generated, 2026) so the
// 22 translation units that include this header no longer recompile the math.
// The BC50::{ItoI,IItoI,ItoII,IItoII} namespaces and signatures are unchanged.

#include "src/global_config.h"

#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>

namespace BC50 {

/*! \brief Host-guest systems for which a BC50 is implemented. Claude Generated (2026). */
enum class System {
    Unsupported, //!< no BC50 definition available for this reaction system
    ItoI, //!< AB
    ItoII, //!< AB, AB2
    IItoI, //!< A2B, AB
    IItoII //!< A2B, AB, AB2
};

/*! \brief Classify a reaction-defined system by its stoichiometry (components x species).
 *
 * The reaction-driven `*_any` models carry an arbitrary system, but BC50 is only defined for the
 * four host-guest patterns above. Anything else — three or more components, self-aggregation,
 * higher stoichiometries — returns Unsupported so the caller can omit the value instead of
 * reporting a wrong one. Claude Generated (2026). */
System Classify(const Eigen::MatrixXi& stoich);

/*! \brief BC50 for a reaction-defined system, or a negative value when unsupported.
 *
 * \param stoich components x species, as produced by ReactionParser/SpeciationEngine.
 * \param lgBeta CUMULATIVE lg beta per species, in the column order of \p stoich — the convention
 *        the `*_any` models use for their global parameters. The fixed-stoichiometry entry points
 *        below take STEPWISE constants, so this converts. Claude Generated (2026). */
qreal FromSpeciation(const Eigen::MatrixXi& stoich, const QVector<qreal>& lgBeta);

/*! \brief Formatted BC50 block for a reaction-defined system; empty when unsupported. CG (2026). */
QString Format_FromSpeciation(const Eigen::MatrixXi& stoich, const QVector<qreal>& lgBeta);

namespace ItoI {
    qreal BC50(const qreal logK11);
    QString Format_BC50(const qreal logK11);
}

namespace IItoI {
    qreal BC50(const qreal logK21, const qreal logK11);
    QPair<qreal, qreal> ABPair(qreal x, const QVector<qreal>& parameter);
    qreal AFunction(qreal x, const QVector<qreal>& parameter);
    qreal BFunction(qreal x, const QVector<qreal>& parameter);
    qreal A0Function(qreal x, const QVector<qreal>& parameter);
    qreal B0Function(qreal x, const QVector<qreal>& parameter);
    qreal ABfunction(qreal x, const QVector<qreal>& parameter);
    qreal A2Bfunction(qreal x, const QVector<qreal>& parameter);
    qreal BCfunction(qreal x, const QVector<qreal>& parameter);
    QString Format_BC50(const qreal logK21, const qreal logK11);
}

namespace ItoII {
    qreal BC50_Y(qreal x, const QVector<qreal>& parameter);
    qreal BC50(const qreal logK11, const qreal logK12);
    QPair<qreal, qreal> ABPair(qreal x, const QVector<qreal>& parameter);
    qreal BFunction(qreal x, const QVector<qreal>& parameter);
    qreal AFunction(qreal x, const QVector<qreal>& parameter);
    qreal ABFunction(qreal x, const QVector<qreal>& parameter);
    qreal AB2Function(qreal x, const QVector<qreal>& parameter);
    qreal A0Function(qreal x, const QVector<qreal>& parameter);
    qreal B0Function(qreal x, const QVector<qreal>& parameter);
    QString Format_BC50(const qreal logK11, const qreal logK12);
}

namespace IItoII {
    qreal BC50_A0_X(qreal x, const QVector<qreal>& parameter);
    qreal BC50_A0(const qreal logK21, const qreal logK11, const qreal logK12);
    qreal BC50_A_X(qreal x, const QVector<qreal>& parameter);
    QPair<qreal, qreal> ABConcentration(qreal x, const QVector<qreal>& parameter);
    qreal BC50_A(const qreal logK21, const qreal logK11, const qreal logK12);
    qreal BC50_B0_X(qreal x, const QVector<qreal>& parameter);
    qreal BC50_B0(const qreal logK21, const qreal logK11, const qreal logK12);
    qreal BFunction(qreal x, const QVector<qreal>& parameter);
    qreal AFunction(qreal x, const QVector<qreal>& parameter);
    qreal A2BFunction(qreal x, const QVector<qreal>& parameter);
    qreal ABFunction(qreal x, const QVector<qreal>& parameter);
    qreal AB2Function(qreal x, const QVector<qreal>& parameter);
    qreal A0Function(qreal x, const QVector<qreal>& parameter);
    qreal B0Function(qreal x, const QVector<qreal>& parameter);
    QString Format_BC50(const qreal logK21, const qreal logK11, const qreal logK12);
}
}
