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
