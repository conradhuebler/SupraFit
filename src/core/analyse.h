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

#include <QtCore/QJsonObject>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>

#include "src/core/AbstractModel.h"

namespace StatisticTool {

/*
const QString Latex_Head = "\documentclass{standalone}"
        "\usepackage[utf8]{inputenc}"
        "\usepackage{tikz}"
        "\usepackage{pgfplots}"
        "\pgfplotsset{compat=1.10}"
        "\pgfplotsset{every axis/.append style={"
                            "label style={font=\sffamily},"
                            "tick label style={font=\sffamily}  "
                            "}}"
        "%1"
        "\begin{document}"
        "\begin{tikzpicture}[font=\sffamily\small]"
        "\begin{axis}[title={\parbox{6cm}{%2}},xmin = 0.5,xmax = %3, ymin= 0, ymax=0.25,xtick={%4},xticklabels={%5 }, ytick={%6},yticklabels={%7}, xlabel={Models}, ylabel={%8}, y label style={rotate=0,anchor=east,yshift=10pt}]"
        "%9"
        "\end{axis}"
        "%10"
        "\end{tikzpicture}"
        "\end{document}";

*/

QString AnalyseReductionAnalysis(const QVector<QPair<QJsonObject, QVector<int>>> models, double cutoff = 0);
QString CompareAIC(const QVector<QWeakPointer<AbstractModel>> models);
QString CompareCV(const QVector<QJsonObject> models, int cvtype = 1, bool local = true, int cv_x = 3);
QString CompareMC(const QVector<QJsonObject> models, bool local = true, int index = 1);
}
