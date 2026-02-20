/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include <QtCore/QVector>
#include <QtCore/QPair>

namespace BC50 {

namespace ItoI {
    qreal BC50(const qreal logK11);
    QString Format_BC50(const qreal logK11);
}

namespace IItoI {
    qreal BC50(const qreal logK21, const qreal logK11);
    QString Format_BC50(const qreal logK21, const qreal logK11);
}

namespace ItoII {
    qreal BC50(const qreal logK11, const qreal logK12);
    QString Format_BC50(const qreal logK11, const qreal logK12);
}

namespace IItoII {
    qreal BC50_A0(const qreal logK21, const qreal logK11, const qreal logK12);
    qreal BC50_A(const qreal logK21, const qreal logK11, const qreal logK12);
    qreal BC50_B0(const qreal logK21, const qreal logK11, const qreal logK12);
    QString Format_BC50(const qreal logK21, const qreal logK11, const qreal logK12);
}

}