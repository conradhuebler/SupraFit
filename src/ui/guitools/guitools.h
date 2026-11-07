/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2019 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#pragma once

#include <QtGui/QIcon>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

inline QIcon Icon(const QString& str)
{
#ifdef _Theme
    return QIcon::fromTheme(str);
#else
    return QIcon(":/icons/" + str + ".png");
#endif
}

/*! \brief Set the style sheet of \p widget only when it differs: every setStyleSheet() polishes the widget (and its children)
 * again, also for the sheet it already has; the parameter boxes are told their colour after every recalculation.
 * Claude Generated. */
inline void SetStyleSheetIfChanged(QWidget* widget, const QString& sheet)
{
    if (widget && widget->styleSheet() != sheet)
        widget->setStyleSheet(sheet);
}
