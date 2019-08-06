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

#include <QDebug>

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
    QHash<QString, qreal> parameters, parameters_corr;
    QHash<QString, int> parameters_count;

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
#pragma message("let us analyse more reduction, later ")
        QJsonObject reduction;
        for (const QString& key : model.first["data"].toObject()["methods"].toObject().keys()) {
            if (model.first["data"].toObject()["methods"].toObject()[key].toObject()["controller"].toObject()["method"].toInt() == SupraFit::Method::Reduction) {
                reduction = model.first["data"].toObject()["methods"].toObject()[key].toObject();
                break;
            }
        }

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
        result += "<tr><th>Analysing Model " + QString::number(index) + " - " + model.first["name"].toString() + "</th></tr>";

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

            if (parameters.contains(element["name"].toString())) {
                parameters[element["name"].toString()] += stdev;
                parameters_corr[element["name"].toString()] += stdev_corr;

                parameters_count[element["name"].toString()]++;
            } else {
                parameters.insert(element["name"].toString(), stdev);
                parameters_corr.insert(element["name"].toString(), stdev_corr);

                parameters_count.insert(element["name"].toString(), 1);
            }

            all_partial_std += stdev;
            all_partial_std_corr += stdev_corr;

            result += "<tr><td>Standard deviation : " + Print::printDouble(stdev) + "</td><td> Average Parameter : " + Print::printDouble(aver) + "  </td><td>    </td></tr>";
            result += "<tr><td>Average Error = " + Print::printDouble(aver_err) + "</td><td> Sum of Errors: " + Print::printDouble(sum_err) + "  </td><td>  Max Error = " + Print::printDouble(max_err) + " </td></tr>";
            result += "<tr><td></td></tr>";
            concl.insert(stdev, QString("<p> " + model.first["name"].toString() + " Parameter: " + element["name"].toString() + " of type " + element["type"].toString() + " &sigma;<sub>pt</sub>: " + Print::printDouble(stdev)) + "</p>");
            concl_corr.insert(stdev_corr, QString("<p> " + model.first["name"].toString() + " Parameter: " + element["name"].toString() + " of type " + element["type"].toString() + " &sigma;<sub>ptc</sub>: " + Print::printDouble(stdev_corr)) + "</p>");
        }

        mean_std /= double(parameter.size());
        mean_corr_std /= double(parameter.size());

        mean_std_orderd.insert(mean_std, model.first["name"].toString());
        mean_std_corr_orderd.insert(mean_corr_std, model.first["name"].toString());

        result += "<tr><td></td></tr>";
        result += "<tr><td></td></tr>";
    }

    QMap<qreal, QString> parameters_orderd, parameters_orderd_corr;

    for (const QString& str : parameters.keys()) {
        parameters_orderd.insert(parameters[str] / double(parameters_count[str]), str);
        parameters_orderd_corr.insert(parameters_corr[str] / double(parameters_count[str]), str);
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

    result += "<p> Best fitting parameters according to the partial standard deviation (&sigma;<sub>pt</sub>) </p>";

    auto param = parameters_orderd.constBegin();
    while (param != parameters_orderd.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(param.value()).arg(param.key());
        ++param;
    }

    result += "</br>\n\n";
    result += "<p> Best fitting parameters according to the corrected partial standard deviation (&sigma;<sub>pt</sub>) </p>";
    param = parameters_orderd_corr.constBegin();
    while (param != parameters_orderd_corr.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(param.value()).arg(param.key());
        ++param;
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

QString CompareCV(const QVector<QJsonObject> models, int cvtype, bool local, int cv_x, int bins)
{

    QMultiMap<qreal, QString> individual, model_wise;

    QMultiMap<qreal, QString> concl_corr;
    QMap<qreal, QString> mean_std_orderd, mean_std_corr_orderd;
    QHash<QString, qreal> parameters, parameters_corr;
    QHash<QString, int> parameters_count;

    QString result = QString("<table>");
    for (const auto& model : models) {

        QJsonObject statistics = model["data"].toObject()["methods"].toObject();
        QStringList keys = statistics.keys();

        for (const QString& str : qAsConst(keys)) {
            QJsonObject obj = model["data"].toObject()["methods"].toObject()[str].toObject();
            QJsonObject controller = model["data"].toObject()["methods"].toObject()[str].toObject()["controller"].toObject();
            int method = controller["method"].toInt();
            int type = controller["CXO"].toInt();
            int x = controller["X"].toInt();
            if (method != 4 || type != cvtype)
                continue;

            if (type == 3 && x != cv_x)
                continue;

            QStringList k = obj.keys();
            qreal hx = 0;
            int counter = 0;
            for (const QString& element : qAsConst(k)) {
                if (element == "controller")
                    continue;

                QJsonObject result = obj[element].toObject();

                QVector<qreal> list = ToolSet::String2DoubleVec(result["data"].toObject()["raw"].toString());
                QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(list, bins);
                ToolSet::Normalise(histogram);
                QPair<qreal, qreal> pair = ToolSet::Entropy(histogram);

                QString name = result["name"].toString() + " - " + model["name"].toString();

                if ((result["type"].toString() == "Local Parameter" && local) || result["type"].toString() == "Global Parameter") {
                    hx += qAbs(pair.first);
                    individual.insert(qAbs(pair.first), name);
                    counter++;
                }
            }
            model_wise.insert(hx / double(counter), model["name"].toString());
        }
    }

    {
        auto i = model_wise.begin();
        qreal first = 0;
        while (i != model_wise.constEnd()) {

            if (i == model_wise.begin())
                first = i.key();
            result += "<p>" + i.value() + ":  H(x) :" + Print::printDouble(i.key()) + "</p>";
            ++i;
        }
    }
    {
        auto i = individual.begin();
        qreal first = 0;
        while (i != individual.constEnd()) {
            if (i == individual.begin())
                first = i.key();
            result += "<p>" + i.value() + ":  H(x) :" + Print::printDouble(i.key()) + "</p>";
            ++i;
        }
    }
    return result;
}

QString CompareMC(const QVector<QJsonObject> models, bool local, int bins)
{

    QMultiMap<qreal, QString> individual, model_wise;

    QMultiMap<qreal, QString> concl_corr;
    QMap<qreal, QString> mean_std_orderd, mean_std_corr_orderd;
    QHash<QString, qreal> parameters, parameters_corr;
    QHash<QString, int> parameters_count;

    int MaxSteps = -1;
    int sigma = -1;
    QString result = QString("<table>");
    for (const auto& model : models) {

        QJsonObject statistics = model["data"].toObject()["methods"].toObject();
        QStringList keys = statistics.keys();

        for (const QString& str : qAsConst(keys)) {
            QJsonObject obj = model["data"].toObject()["methods"].toObject()[str].toObject();
            QJsonObject controller = model["data"].toObject()["methods"].toObject()[str].toObject()["controller"].toObject();
            int method = controller["method"].toInt();
            if (method != SupraFit::Method::MonteCarlo)
                continue;

            if (MaxSteps == -1) {
                MaxSteps = controller["MaxSteps"].toInt();
                sigma = controller["VarianceSource"].toInt();
            }

            if (MaxSteps != controller["MaxSteps"].toInt() || sigma != controller["VarianceSource"].toInt())
                continue;

            QStringList k = obj.keys();
            qreal hx = 0;
            int counter = 0;
            for (const QString& element : qAsConst(k)) {
                if (element == "controller")
                    continue;

                QJsonObject result = obj[element].toObject();

                QVector<qreal> list = ToolSet::String2DoubleVec(result["data"].toObject()["raw"].toString());
                QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(list, bins);
                ToolSet::Normalise(histogram);
                QPair<qreal, qreal> pair = ToolSet::Entropy(histogram);

                QString name = result["name"].toString() + " - " + model["name"].toString();

                if ((result["type"].toString() == "Local Parameter" && local) || result["type"].toString() == "Global Parameter") {
                    hx += qAbs(pair.first);
                    individual.insert(qAbs(pair.first), name);
                    counter++;
                }
            }
            model_wise.insert(hx / double(counter), model["name"].toString());
        }
    }

    {
        auto i = model_wise.begin();
        qreal first = 0;
        while (i != model_wise.constEnd()) {

            if (i == model_wise.begin())
                first = i.key();
            result += "<p>" + i.value() + ":  H(x) :" + Print::printDouble(i.key()) + "</p>";
            ++i;
        }
    }
    {
        auto i = individual.begin();
        qreal first = 0;
        while (i != individual.constEnd()) {
            if (i == individual.begin())
                first = i.key();
            result += "<p>" + i.value() + ":  H(x) :" + Print::printDouble(i.key()) + "</p>";
            ++i;
        }
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
