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
#include <QtCore/QWeakPointer>

#include "src/core/AbstractModel.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "analyse.h"

namespace StatisticTool {

QString AnalyseReductionAnalysis(const QVector<QPair<QJsonObject, QVector<int>>> models, double cutoff)
{
    QMultiMap<qreal, QString> concl;
    QMultiMap<qreal, QString> concl_corr;
    QMap<qreal, QString> mean_std_orderd, mean_std_corr_orderd;

    QString result = QString("<table>");
    QVector<qreal> X;
    qreal all_partial_std = 0;
    qreal all_partial_std_corr = 0;
    int cut = 0;
    int index = 0;
    int j = 0;
    for (const auto& model : models) {
        index++;
        qreal mean_std = 0, mean_corr_std = 0;
        bool skip = false;
        QJsonObject reduction = model.first["data"].toObject()["statistics"].toObject()[QString::number(SupraFit::Statistic::Reduction)].toObject();
        QVector<int> parameter = model.second;

        if (reduction.isEmpty()) {
            skip = true;
        }
        if (X.isEmpty()) {
            X = ToolSet::String2DoubleVec(reduction["controller"].toObject()["x"].toString());
            if (X.isEmpty())
                return QString("No Data found. Sorry for that.");
            for (int i = 0; i < X.size(); ++i) {
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
        result += "<tr><th>Analysing Model " + QString::number(index) + " - " + Model2Name(static_cast<SupraFit::Model>(model.first["model"].toInt())) + "</th></tr>";

        for (int key : parameter) {
            if (key == -1)
                continue;

            QJsonObject element = reduction[QString::number(key)].toObject();
            if (element.isEmpty())
                continue;

            qreal val = element["value"].toDouble();
            result += "<tr><th colspan='3'> " + element["name"].toString() + " of type " + element["type"].toString() + ": optimal value = " + Print::printDouble(val) + "</th></tr>";

            qreal value = 0, sum_err = 0, max_err = 0, aver_err = 0, aver = 0, stdev = 0, stdev_corr = 0;
            value = element["value"].toDouble();
            QVector<qreal> vector = ToolSet::String2DoubleVec(element["data"].toObject()["raw"].toString());
            for (int i = 0; i < cut; ++i) {
                aver += vector[i];
                sum_err += (value - vector[i]) * (value - vector[i]);
                max_err = qMax(qAbs(value - vector[i]), max_err);
            }
            j++;
            aver /= double(cut);
            aver_err = sqrt(sum_err) / double(cut);
            stdev = Stddev(vector, cut);
            stdev_corr = Stddev(vector, cut, value);

            mean_std += stdev;
            mean_corr_std += stdev_corr;

            all_partial_std += stdev;
            all_partial_std_corr += stdev_corr;

            result += "<tr><td>Standard deviation : " + Print::printDouble(stdev) + "</td><td> Average Parameter : " + Print::printDouble(aver) + "  </td><td>    </td></tr>";
            result += "<tr><td>Average Error = " + Print::printDouble(aver_err) + "</td><td> Sum of Errors: " + Print::printDouble(sum_err) + "  </td><td>  Max Error = " + Print::printDouble(max_err) + " </td></tr>";
            result += "<tr><td></td></tr>";
            concl.insert(stdev, QString("<p> " + Model2Name(static_cast<SupraFit::Model>(model.first["model"].toInt())) + " Parameter: " + element["name"].toString() + " of type " + element["type"].toString() + " &sigma;<sub>pt</sub>: " + Print::printDouble(stdev)) + "</p>");
            concl_corr.insert(stdev_corr, QString("<p> " + Model2Name(static_cast<SupraFit::Model>(model.first["model"].toInt())) + " Parameter: " + element["name"].toString() + " of type " + element["type"].toString() + " &sigma;<sub>ptc</sub>: " + Print::printDouble(stdev_corr)) + "</p>");
        }

        mean_std /= double(parameter.size());
        mean_corr_std /= double(parameter.size());

        mean_std_orderd.insert(mean_std, Model2Name(static_cast<SupraFit::Model>(model.first["model"].toInt())));
        mean_std_corr_orderd.insert(mean_corr_std, Model2Name(static_cast<SupraFit::Model>(model.first["model"].toInt())));

        result += "<tr><td></td></tr>";
        result += "<tr><td></td></tr>";
    }
    all_partial_std /= double(j);
    all_partial_std_corr /= double(j);

    result += "</table></br >";

    result += "<p> Best fitting models according to the partial standard deviation (&sigma;<sub>pt</sub>) </p>";

    auto l = mean_std_orderd.constBegin();
    while (l != mean_std_orderd.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(l.value()).arg(l.key());
        ++l;
    }

    result += "</br>\n\n";
    result += "<p> Best fitting models according to the corrected partial standard deviation (&sigma;<sub>pt</sub>) </p>";
    l = mean_std_corr_orderd.constBegin();
    while (l != mean_std_corr_orderd.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(l.value()).arg(l.key());
        ++l;
    }
    result += "</br>\n\n";
    result += "</br>\n\n";

    result += "<p> Individual parameters </p>";
    result += "<p> Ordered list of all partial standard deviations for each parameter (&sigma;<sub>pt</sub>) </p>";
    result += "<p> Mean partial standard deviation " + Print::printDouble(all_partial_std) + "</p>";
    qreal old_std = 0;
    auto i = concl.begin();
    int indx = 0;
    while (i != concl.constEnd()) {
        if (indx - 1 < concl.size() / 2 && indx >= concl.size() / 2)
            result += "<p>------------------------- Median Line --------------------------------</p>";
        if (old_std < all_partial_std && all_partial_std < i.key())
            result += "<p>------------------------- Average Line -------------------------------</p>";
        result += i.value();
        old_std = i.key();
        indx++;
        ++i;
    }
    result += "</br>\n\n";
    result += "<p> Ordered list of all corrected partial standard deviations for each parameter (&sigma;<sub>ptc</sub>)</p>";
    result += "<p> Mean corrected partial standard deviation " + Print::printDouble(all_partial_std_corr) + "</p>";

    auto k = concl_corr.begin();
    indx = 0;
    while (k != concl_corr.constEnd()) {
        if (indx - 1 < concl.size() / 2 && indx >= concl_corr.size() / 2)
            result += "<p>------------------------- Median Line --------------------------------</p>";
        if (old_std < all_partial_std_corr && all_partial_std_corr < k.key())
            result += "<p>------------------------- Average Line -------------------------------</p>";
        result += k.value();
        old_std = k.key();
        indx++;
        ++k;
    }

    return result;
}

QString CompareAIC(const QVector<QWeakPointer<AbstractModel>> models)
{
    QString result = "Akaike's Information Criterion AIC";
    result += "<table>";
    QMultiMap<qreal, QString> list_first, list_second;
    result += "<tr><th>Model</th><th>Second Order AIC</th><th>AIC</th></tr>";
    for (int i = 0; i < models.size(); ++i) {
        QString str(QString::number(i) + " - " + models[i].data()->Name());
        list_second.insert(models[i].data()->AICc(), str);
        list_first.insert(models[i].data()->AIC(), str);
        result += "<tr><td>" + str + ":</td><td>" + Print::printDouble(models[i].data()->AICc()) + "</td><td>" + Print::printDouble(models[i].data()->AIC()) + "</td></tr>";
    }
    result += "</table>\n";
    result += "<p>List ordered by 2nd AIC Value (corrected AIC)</p>";

    auto i = list_second.begin();
    qreal first = 0;
    while (i != list_second.constEnd()) {

        if (i == list_second.begin())
            first = i.key();
        result += "<p>" + i.value() + ":  2nd Order AIC:" + Print::printDouble(i.key()) + " - Evidence Ratio:" + Print::printDouble(1 / exp(-0.5 * (first - i.key()))) + "</p>";
        ++i;
    }

    result += "\n\n<p>List ordered by uncorrected AIC Value</p>";
    i = list_first.begin();
    while (i != list_first.constEnd()) {

        if (i == list_first.begin())
            first = i.key();

        result += "<p>" + i.value() + ":  AIC:" + Print::printDouble(i.key()) + " - Evidence Ratio:" + Print::printDouble(1 / exp(-0.5 * (first - i.key()))) + "</p>";
        ++i;
    }
    result += "For more information on AIC see H. Motulsky, A. Christopoulos, Fitting Models to Biological Data using Linear and Nonlinear Regression. A practical guide to curve fitting. GraphPad Software Inc., San Diego CA, 2003";
    return result;
}
}
