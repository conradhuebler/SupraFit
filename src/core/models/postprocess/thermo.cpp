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

#include "src/core/models/models.h"
#include "src/core/toolset.h"

#include "src/global.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonObject>

#include "thermo.h"

namespace Thermo{

EnergyUnit CurrentEnergyUnit()
{
    EnergyUnit unit; // defaults to SI (kJ/mol, J/(mol·K))
    const bool kcal = qApp && qApp->instance()->property("energy_unit_kcal").toBool();
    if (kcal) {
        unit.energyDivisor = 1000.0 * cal2joule; // J/mol -> kcal/mol
        unit.entropyDivisor = cal2joule; // J/(mol·K) -> cal/(mol·K)
        unit.energyLabel = "kcal/mol";
        unit.entropyLabel = "cal/(molK)";
    }
    return unit;
}

QString FormatThermo(qreal K, qreal T, qreal H)
{
    const EnergyUnit unit = CurrentEnergyUnit();
    QString result;
    qreal dG = ToolSet::K2G(K, T);
    qreal dS = ToolSet::GHE(dG, H, T);

    result += "<table>";
    result += "<tr><td><b>Complexation Constant K </b></td><td>" + Print::printDouble(qPow(10, K)) + "</td>";
    result += "<td> M</td></tr>";
    result += "<tr><td><b>Free Enthalpy of Complexation &Delta;G </b></td><td>" + Print::printDouble(dG / unit.energyDivisor, 3) + "</td>";
    result += "<td>" + unit.energyLabel + "</td></tr>";

    if (H != 0.0) {
        result += "<tr><td><b>Enthalpy of Complexation &Delta;H</b></td><td>" + Print::printDouble(H / unit.energyDivisor) + "</td>";
        result += "<td>" + unit.energyLabel + "</td></tr>";

        result += "<tr><td><b>Entropy of Complexation &Delta;S</b></td><td>" + Print::printDouble(dS / unit.entropyDivisor) + "</td>";
        result += "<td>" + unit.entropyLabel + "</td></tr>";

        // -TΔS (entropy term of ΔG = ΔH − TΔS); matches the NanoAnalyze convention where the
        // entropy contribution is reported as −TΔS, resolving the apparent entropy sign mismatch.
        result += "<tr><td><b>Entropy Term -T&Delta;S</b></td><td>" + Print::printDouble(-T * dS / unit.energyDivisor) + "</td>";
        result += "<td>" + unit.energyLabel + "</td></tr>";
    }

    result += "</table>";
    return result;
}

}
