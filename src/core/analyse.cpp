/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include <QtCore/QJsonObject>
#include <QtCore/QMultiMap>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>

#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "analyse.h"

namespace StatisticTool {

QString AnalyseReductionAnalysis(const QVector<QPair<QJsonObject, QVector<int>>> models, double cutoff)
{
    QMultiMap<qreal, QString> concl;
    QString result = QString("<table>");
    QVector<qreal> X;
    qreal all_std = 0;
    int cut = 0;
    int index = 0;
    int j = 0;
    for (const QPair<QJsonObject, QVector<int>>& model : models) {
        index++;
        bool skip = false;
        QJsonObject reduction = model.first["data"].toObject()["statistics"].toObject()[QString::number(SupraFit::Statistic::Reduction)].toObject();
        QVector<int> parameter = model.second;

        if (reduction.isEmpty()) {
            skip = true;
        }
        if (X.isEmpty()) {
            X = ToolSet::String2DoubleVec(reduction["controller"].toObject()["x"].toString());
            for (int i = 0; i <= X.size(); ++i) {
                if (X[i] > cutoff)
                    cut = i;
            }
            if (cut < X.size() - 1)
                cut++;
            result += "<tr><th>Initialising comparison of Reduction Analysis!</th></tr>";
            result += "<tr><td>Cutoff set to X = " + QString::number(cutoff) + " translated to index = " + QString::number(cut) + "</td></tr>";
        }

        if (skip) {
            result += "<tr><th>Skipping model " + QString::number(index) + " - Something missing?</th></tr>";
            continue;
        }
        result += "<tr><th>Analysing Model " + QString::number(index) + " - " + Model2Name((SupraFit::Model)model.first["model"].toInt()) + "</th></tr>";

        for (int key : parameter) {
            if (key == -1)
                continue;

            QJsonObject element = reduction[QString::number(key)].toObject();
            if (element.isEmpty())
                continue;

            qreal val = element["value"].toDouble();
            result += "<tr><th colspan='3'> " + element["name"].toString() + " of type " + element["type"].toString() + ": optimal value = " + Print::printDouble(val) + "</th></tr>";

            qreal value = 0, sum_err = 0, max_err = 0, aver_err = 0, aver = 0, stdev = 0;
            value = element["value"].toDouble();
            QVector<qreal> vector = ToolSet::String2DoubleVec(element["data"].toObject()["raw"].toString());
            for (int i = 0; i < cut; ++i) {
                aver += vector[i];
                sum_err += (value - vector[i]) * (value - vector[i]);
                max_err = qMax(qAbs(value - vector[i]), max_err);
            }

            aver /= double(cut);
            aver_err = sqrt(sum_err) / double(cut);
            stdev = Stddev(vector, cut);
            all_std += stdev;
            result += "<tr><td>Standard deviation : " + Print::printDouble(stdev) + "</td><td> Average Parameter : " + Print::printDouble(aver) + "  </td><td>    </td></tr>";
            result += "<tr><td>Average Error = " + Print::printDouble(aver_err) + "</td><td> Sum of Errors: " + Print::printDouble(sum_err) + "  </td><td>  Max Error = " + Print::printDouble(max_err) + " </td></tr>";
            result += "<tr><td></td></tr>";
            concl.insert(stdev, QString("<p> " + Model2Name((SupraFit::Model)model.first["model"].toInt()) + " Parameter: " + element["name"].toString() + " of type " + element["type"].toString() + " stddev: " + Print::printDouble(stdev)) + "</p>");
        }
        result += "<tr><td></td></tr>";
        result += "<tr><td></td></tr>";
        j++;
    }
    all_std /= double(j);
    result += "</table></br >";
    result += "<p> Summary: Ordered list of all partial standard deviations for each parameter </p>";
    result += "<p> Mean standard deviations " + Print::printDouble(all_std) + "</p>";
    qreal old_std = 0;
    QMap<qreal, QString>::iterator i = concl.begin();
    int indx = 0;
    while (i != concl.constEnd()) {
        if (indx - 1 < concl.size() / 2 && indx >= concl.size() / 2)
            result += "<p>-------------------------------------------------------------------------------------</p>";
        result += i.value();
        indx++;
        ++i;
    }
    return result;
}
}
