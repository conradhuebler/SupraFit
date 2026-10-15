/*
 * Appearance (light / dark / system colours) of the SupraFit GUI.
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
 * Claude Generated (UI redesign WP5).
 */

#pragma once

#include <QtGui/QColor>
#include <QtGui/QPalette>

class QChart;

/*! \brief Application colour scheme: one QPalette for all widgets (Fusion style) and palette-derived
 * colours for the charts.
 *
 * The mode is the persisted setting "appearance" (0 = follow the system, 1 = light, 2 = dark). Widgets
 * should take their colours from the palette (QPalette roles or "palette(...)" in style sheets); colours
 * with a meaning (included / excluded parameters, data-series colours) keep their colour and get a text
 * colour from ContrastText(). Charts keep their series colours; only background, axes, grid, title and
 * legend follow the palette, so a dark window no longer shows white chart cards. */
namespace Appearance {

enum Mode {
    System = 0,
    Light = 1,
    Dark = 2
};

/*! \brief Remember the platform palette (call once, before the first Apply) and start restyling
 * every chart view that is shown from now on. */
void Install();

/*! \brief Switch the application palette to \p mode and restyle all existing charts. */
void Apply(int mode);

/*! \brief True if the current application palette is dark (window colour lightness < 128). */
bool IsDark();

/*! \brief Colour the background, axes, grid, title and legend of \p chart from the application palette;
 * series colours are left untouched. */
void ApplyToChart(QChart* chart);

/*! \brief Black or white, whichever is readable on \p background. */
QColor ContrastText(const QColor& background);
}
