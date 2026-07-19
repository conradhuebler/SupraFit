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

#pragma once

#include <QtCore/QChar>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

/*! \brief Central home for SupraFit's physical units. Claude Generated
 *
 * Conventions:
 *  - Internally SupraFit works in SI: concentrations mol/L, energies J/mol,
 *    entropies J/(mol·K), temperature K.
 *  - Thermodynamic energies are formatted through EnergyUnit so the whole app can be
 *    switched between SI (kJ/mol, J/(mol·K)) and calorie-based units (kcal/mol,
 *    cal/(mol·K)) — see currentEnergy() and the "energy_unit_kcal" setting.
 *  - ITC syringe/cell concentrations are conventionally entered in mmol/L; raw ITC heat
 *    keeps the calorimeter's own unit until it is scaled to Joule at import (cal → J,
 *    factor cal2joule), which is why an unscaled ΔH comes out in calories.
 *
 * The label helpers below are the single source of truth for on-screen unit strings. */
namespace Units {

//! Temperature label ("K").
inline QString temperature() { return QStringLiteral("K"); }
//! Molar concentration label ("mol/L").
inline QString concentration() { return QStringLiteral("mol/L"); }
//! Millimolar concentration label ("mmol/L") — ITC syringe/cell convention.
inline QString concentrationMilli() { return QStringLiteral("mmol/L"); }
//! Micro-litre volume label ("µL").
inline QString volumeMicroLitre() { return QString(QChar(0x00B5)) + QStringLiteral("L"); }
//! Milli-litre volume label ("mL").
inline QString volumeMilliLitre() { return QStringLiteral("mL"); }
//! NMR chemical-shift label ("ppm").
inline QString chemicalShift() { return QStringLiteral("ppm"); }
//! Concentration axis title used by the titration concentration charts ("c [mol/L]").
inline QString concentrationAxis() { return QStringLiteral("c [mol/L]"); }

/*! \brief Runtime-selectable display unit for thermodynamic energies. */
struct EnergyUnit {
    qreal energyDivisor = 1000.0; //!< divide a J/mol value by this (kJ: 1000, kcal: 1000·cal2joule)
    qreal entropyDivisor = 1.0; //!< divide a J/(mol·K) value by this (J: 1, cal: cal2joule)
    QString energyLabel = QStringLiteral("kJ/mol"); //!< label for ΔG/ΔH/−TΔS
    QString entropyLabel = QStringLiteral("J/(molK)"); //!< label for ΔS
};

/*! \brief Current energy-unit selection, read from the global qApp property
 * "energy_unit_kcal" (bool); defaults to SI (kJ/mol). */
EnergyUnit currentEnergy();

}
