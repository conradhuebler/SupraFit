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

// The energy-unit selection lives centrally in Units (src/core/units.h) — see Units::EnergyUnit
// and Units::currentEnergy(). FormatThermo formats in whatever unit that returns.
QString FormatThermo(qreal K, qreal T, qreal H = 0);

}
