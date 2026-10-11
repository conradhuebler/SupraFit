/*
 * SupraFit central unit definitions
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
 */

#include <QtCore/QCoreApplication>

#include "src/global.h" // cal2joule

#include "units.h"

namespace Units {

EnergyUnit currentEnergy()
{
    EnergyUnit unit; // defaults to SI (kJ/mol, J/(mol·K))
    const bool kcal = qApp && qApp->instance()->property("energy_unit_kcal").toBool();
    if (kcal) {
        unit.energyDivisor = 1000.0 * cal2joule; // J/mol -> kcal/mol
        unit.entropyDivisor = cal2joule; // J/(mol·K) -> cal/(mol·K)
        unit.energyLabel = QStringLiteral("kcal/mol");
        unit.entropyLabel = QStringLiteral("cal/(molK)");
    }
    return unit;
}

}
