/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QJsonObject>

#include "thermo.h"

namespace Thermo{

QString FormatThermo(qreal K, qreal T, qreal H)
{
    QString result;
    qreal dG = ToolSet::K2G(K, T);
    qreal dS = ToolSet::GHE(dG, H, T);

    result += "<table>";
    result += "<tr><td><b>Complexation Constant K </b></td><td>" + Print::printDouble(qPow(10, K)) + "</td>";
    result += "<td> M</td></tr>";
    result += "<tr><td><b>Free Enthalpy of Complexation &Delta;G </b></td><td>" + Print::printDouble(dG / 1000.0, 3) + "</td>";
    result += "<td>kJ/mol</td></tr>";

    if (H != 0.0) {
        result += "<tr><td><b>Enthalpy of Complexation &Delta;H</b></td><td>" + Print::printDouble(H / 1000.0) + "</td>";
        result += "<td>kJ/mol</td></tr>";

        result += "<tr><td><b>Entropy of Complexation &Delta;S</b></td><td>" + Print::printDouble(dS) + "</td>";
        result += "<td>J/(molK)</td></tr>";

    }

    result += "</table>";
    return result;
}

}
