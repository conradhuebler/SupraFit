/*
 * <one line to give the library's name and an idea of what it does.>
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

#include <QtCore/QString>

namespace Thermo {

/*! \brief Runtime-selectable display unit for thermodynamic energies. Claude Generated
 *
 * Internally every energy is J/mol and every entropy J/(mol·K). This struct carries the
 * divisor + label needed to present them either in SI units (kJ/mol, J/(mol·K)) or in the
 * calorie-based units used e.g. by TA NanoAnalyze (kcal/mol, cal/(mol·K)), so results can be
 * compared without hand conversion. */
struct EnergyUnit {
    qreal energyDivisor = 1000.0; //!< divide a J/mol value by this for display (kJ: 1000, kcal: 1000·cal2joule)
    qreal entropyDivisor = 1.0; //!< divide a J/(mol·K) value by this for display (J: 1, cal: cal2joule)
    QString energyLabel = "kJ/mol"; //!< label for ΔG/ΔH/−TΔS
    QString entropyLabel = "J/(molK)"; //!< label for ΔS
};

/*! \brief Current energy-unit selection, read from the global qApp property
 * "energy_unit_kcal" (bool); defaults to SI (kJ/mol). Claude Generated */
EnergyUnit CurrentEnergyUnit();

QString FormatThermo(qreal K, qreal T, qreal H = 0);

}
