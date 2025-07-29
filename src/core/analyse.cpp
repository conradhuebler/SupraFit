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

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMultiMap>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>

#include <limits>

#include "src/core/models/AbstractModel.h"

#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "analyse.h"

namespace StatisticTool {

QString AnalyseReductionAnalysis(const QVector<QJsonObject> models, bool local, double cutoff)
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
        double parameter = 0;

        bool skip = false;
#pragma message("let us analyse more reduction, later ")
        QJsonObject reduction;
        for (const QString& key : model["data"].toObject()["methods"].toObject().keys()) {
            if (AccessCI(model["data"].toObject()["methods"].toObject()[key].toObject()["controller"].toObject(), "Method").toInt() == SupraFit::Method::Reduction) {
                reduction = model["data"].toObject()["methods"].toObject()[key].toObject();
                break;
            }
        }

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
        result += "<tr><th>Analysing Model " + QString::number(index) + " - " + model["name"].toString() + "</th></tr>";

        for (const QString& key : reduction.keys()) {
            if (key == "controller")
                continue;

            //            if (key == -1)
            //                continue;

            QJsonObject element = reduction[key].toObject();
            if (element.isEmpty())
                continue;
            if (!local && element["type"].toString() == "Local Parameter")
                continue;

            parameter += 1;

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
            concl.insert(stdev, QString("<p> " + model["name"].toString() + " Parameter: " + element["name"].toString() + " of type " + element["type"].toString() + QString(" %1<sub>pt</sub>: ").arg(Unicode_sigma) + Print::printDouble(stdev)) + "</p>");
            concl_corr.insert(stdev_corr, QString("<p> " + model["name"].toString() + " Parameter: " + element["name"].toString() + " of type " + element["type"].toString() + QString(" %1<sub>ptc</sub>: ").arg(Unicode_sigma) + Print::printDouble(stdev_corr)) + "</p>");
        }

        mean_std /= double(parameter);
        mean_corr_std /= double(parameter);

        mean_std_orderd.insert(mean_std, model["name"].toString());
        mean_std_corr_orderd.insert(mean_corr_std, model["name"].toString());

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

    result += QString("<p> Best fitting models according to the partial standard deviation (%1<sub>pt</sub>) </p>").arg(Unicode_sigma);

    auto l = mean_std_orderd.constBegin();
    while (l != mean_std_orderd.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(l.value()).arg(l.key());
        ++l;
    }

    result += "</br>\n\n";
    result += QString("<p> Best fitting models according to the corrected partial standard deviation (%1<sub>pt</sub>) </p>").arg(Unicode_sigma);
    l = mean_std_corr_orderd.constBegin();
    while (l != mean_std_corr_orderd.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(l.value()).arg(l.key());
        ++l;
    }
    result += "</br>\n\n";
    result += "</br>\n\n";

    result += QString("<p> Best fitting parameters according to the partial standard deviation (%1<sub>pt</sub>) </p>").arg(Unicode_sigma);

    auto param = parameters_orderd.constBegin();
    while (param != parameters_orderd.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(param.value()).arg(param.key());
        ++param;
    }

    result += "</br>\n\n";
    result += QString("<p> Best fitting parameters according to the corrected partial standard deviation (%1<sub>pt</sub>) </p>").arg(Unicode_sigma);
    param = parameters_orderd_corr.constBegin();
    while (param != parameters_orderd_corr.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(param.value()).arg(param.key());
        ++param;
    }
    result += "</br>\n\n";
    result += "</br>\n\n";

    result += "<p> Individual parameters </p>";
    result += QString("<p> Ordered list of all partial standard deviations for each parameter (%1<sub>pt</sub>) </p>").arg(Unicode_sigma);
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
    result += QString("<p> Ordered list of all corrected partial standard deviations for each parameter (%1<sub>ptc</sub>)</p>").arg(Unicode_sigma);
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

QString CompareCV(const QVector<QJsonObject> models, int cvtype, bool local, int cv_x)
{
    QMultiMap<qreal, QString> individual_entropy, model_wise_entropy;
    QMultiMap<qreal, QString> individual_stdev, model_wise_stdev;
    QHash<QString, qreal> parameters_stdev, parameters_h;
    QHash<QString, int> parameters_count;

    QString CV;

    if (cvtype == 1)
        CV = "L0O";
    else if (cvtype == 2)
        CV = "L2O";
    else
        CV = "CXO";

    QString result = QString("<table>");
    QString method_line = QString();
    QString bin_info = QString();
    for (const auto& model : models) {
        QJsonObject statistics = model["data"].toObject()["methods"].toObject();
        QStringList keys = statistics.keys();

        for (const QString& str : qAsConst(keys)) {
            QJsonObject obj = model["data"].toObject()["methods"].toObject()[str].toObject();
            QJsonObject controller = model["data"].toObject()["methods"].toObject()[str].toObject()["controller"].toObject();
            int method = AccessCI(controller, "Method").toInt();
            int type = controller["CXO"].toInt();
            int x = controller["X"].toInt();
            if (method != 4 || type != cvtype)
                continue;

            if (type == 3 && x != cv_x)
                continue;

            int bins;
            if (qApp->instance()->property("OverwriteBins").toBool())
                bins = qApp->instance()->property("EntropyBins").toInt();
            else
                bins = controller["EntropyBins"].toInt(qApp->instance()->property("EntropyBins").toInt());

            int EntropyBins = bins;
            if (cvtype == 3)
                method_line = QString("Cross Validation %1; X = %2, Bins for Histogram calculation = %3, MaxSteps = %4").arg(CV).arg(x).arg(bins).arg(controller["MaxSteps"].toInt());
            else
                method_line = QString("Cross Validation %1, Bins for Histogram calculation = %2, MaxSteps = %3").arg(CV).arg(bins).arg(controller["MaxSteps"].toInt());

            QStringList k = obj.keys();
            qreal hx = 0;
            qreal stdev = 0;
            int counter = 0;
            for (const QString& element : qAsConst(k)) {
                if (element == "controller")
                    continue;

                QJsonObject result = obj[element].toObject();
                QJsonObject box = result["boxplot"].toObject();
                QVector<qreal> list = ToolSet::String2DoubleVec(result["data"].toObject()["raw"].toString());
                QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(list, bins);
                ToolSet::Normalise(histogram);
                QPair<qreal, qreal> pair = ToolSet::Entropy(histogram);

                if (bins != EntropyBins)
                    bin_info = QString("Bins had been set to %1").arg(bins);
                /* If old results are used, that don't contain stddev in their json object, recalculate box-plot */

                QString name = result["name"].toString() + " - " + model["name"].toString() + "   " + bin_info;

                if ((result["type"].toString() == "Local Parameter" && local) || result["type"].toString() == "Global Parameter") {
                    hx += qAbs(pair.first);
                    stdev += box["stddev"].toDouble();
                    individual_entropy.insert(qAbs(pair.first), name);
                    individual_stdev.insert(box["stddev"].toDouble(), name);
                    counter++;
                    if (parameters_stdev.contains(result["name"].toString())) {
                        parameters_stdev[result["name"].toString()] += box["stddev"].toDouble();
                        parameters_h[result["name"].toString()] += qAbs(pair.first);
                        parameters_count[result["name"].toString()]++;
                    } else {
                        parameters_stdev.insert(result["name"].toString(), box["stddev"].toDouble());
                        parameters_h.insert(result["name"].toString(), qAbs(pair.first));
                        parameters_count.insert(result["name"].toString(), 1);
                    }
                }
            }
            model_wise_entropy.insert(hx / double(counter), model["name"].toString());
            model_wise_stdev.insert(stdev / double(counter), model["name"].toString());
        }
    }

    QMap<qreal, QString> parameters_stdev_orderd, parameters_h_orderd;

    for (const QString& str : parameters_stdev.keys()) {
        parameters_stdev_orderd.insert(parameters_stdev[str] / double(parameters_count[str]), str);
        parameters_h_orderd.insert(parameters_h[str] / double(parameters_count[str]), str);
    }

    {
        result += "<tr><th colspan='2'>Initialising comparison of Cross Validation!</th></tr>";
        result += "<tr><td colspan='2'>" + method_line + "</td></tr>";
        if (!model_wise_entropy.isEmpty()) {
            result += "<tr><th colspan='2'>modelwise entropy list</th></tr>";

            auto i = model_wise_entropy.begin();
            //qreal first = 0;
            while (i != model_wise_entropy.constEnd()) {
                //if (i == model_wise_entropy.begin())
                //first = i.key();
                result += "<p>" + i.value() + ":  H(x) :" + Print::printDouble(i.key()) + "</p>";

                ++i;
            }
        }
    }
    {
        result += "<tr><th colspan='2'>parameterwise entropy list</th></tr>";
        if (!individual_entropy.isEmpty()) {
            auto i = individual_entropy.begin();
            //qreal first = 0;
            while (i != individual_entropy.constEnd()) {
                //if (i == individual_entropy.begin())
                //first = i.key();
                result += "<p>" + i.value() + ":  H(x) :" + Print::printDouble(i.key()) + "</p>";
                ++i;
            }
        }
    }
    result += QString("<p>Best fitting parameters according to the Shannon entropy H(x) </p>");

    auto param = parameters_h_orderd.constBegin();
    while (param != parameters_h_orderd.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(param.value()).arg(param.key());
        ++param;
    }
    {
        result += "<tr><th colspan='2'>modelwise " + Unicode_sigma + " list</th></tr>";

        if (!model_wise_stdev.isEmpty()) {
            auto i = model_wise_stdev.begin();
            //qreal first = 0;
            while (i != model_wise_stdev.constEnd()) {
                //if (i == model_wise_stdev.begin())
                //  first = i.key();
                result += "<p>" + i.value() + ":  " + Unicode_sigma + " :" + Print::printDouble(i.key()) + "</p>";
                ++i;
            }
        }
    }
    {
        result += "<tr><th colspan='2'>parameterwise " + Unicode_sigma + " list</th></tr>";
        if (!individual_stdev.isEmpty()) {
            auto i = individual_stdev.begin();
            //qreal first = 0;
            while (i != individual_stdev.constEnd()) {
                //  if (i == individual_stdev.begin())
                //   first = i.key();
                result += "<p>" + i.value() + ":  " + Unicode_sigma + " :" + Print::printDouble(i.key()) + "</p>";
                ++i;
            }
        }
    }
    result += QString("<p>Best fitting parameters according to %1 </p>").arg(Unicode_sigma);

    param = parameters_stdev_orderd.constBegin();
    while (param != parameters_stdev_orderd.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(param.value()).arg(param.key());
        ++param;
    }
    return result;
}

QString CompareMC(const QVector<QJsonObject> models, bool local, int index)
{

    QMultiMap<qreal, QString> individual_entropy, model_wise_entropy;
    QMultiMap<qreal, QString> individual_stdev, model_wise_stdev;
    QHash<QString, qreal> parameters_stdev, parameters_h;
    QHash<QString, int> parameters_count;

    int MaxSteps = -1;
    int sigma = -1;
    int idx = 1;
    double SEy = 0.0;
    QString result = QString("<table>");
    QString method_line = QString();
    QString bin_info = QString();
    bool fullshannon = qApp->instance()->property("FullShannon").toBool();

    for (const auto& model : models) {
        QJsonObject statistics = model["data"].toObject()["methods"].toObject();
        QStringList keys = statistics.keys();

        for (const QString& str : qAsConst(keys)) {
            QJsonObject obj = model["data"].toObject()["methods"].toObject()[str].toObject();
            QJsonObject controller = model["data"].toObject()["methods"].toObject()[str].toObject()["controller"].toObject();
            int method = AccessCI(controller, "Method").toInt();
            if (method != SupraFit::Method::MonteCarlo)
                continue;

            int bins;
            if (qApp->instance()->property("OverwriteBins").toBool())
                bins = qApp->instance()->property("EntropyBins").toInt();
            else
                bins = controller["EntropyBins"].toInt(qApp->instance()->property("EntropyBins").toInt());

            int EntropyBins = bins;

            if (MaxSteps == -1 && idx == index) {
                MaxSteps = controller["MaxSteps"].toInt();
                sigma = controller["VarianceSource"].toInt();
                SEy = controller["Variance"].toDouble();
                if (fullshannon)
                    method_line = QString("<font color=\"red\"><p>Monte Carlo Simulation with %1; %2 = %3, Bins for Histogram calculation = %4;</p><p>Shannon entropy is calculated with the additional term for discrete distribution.</p><p>This will diverge for a large number of binds and reorders the entropy values.</p></font>").arg(MaxSteps).arg(Unicode_sigma).arg(controller["Variance"].toDouble()).arg(bins);
                else
                    method_line = QString("Monte Carlo Simulation with %1; %2 = %3, Bins for Histogram calculation = %4").arg(MaxSteps).arg(Unicode_sigma).arg(controller["Variance"].toDouble()).arg(bins);
            }

            idx++;

            if (MaxSteps != controller["MaxSteps"].toInt() || (sigma != controller["VarianceSource"].toInt() || (1 == controller["VarianceSource"].toInt() && SEy != controller["Variance"].toDouble())))
                continue;

            QStringList k = obj.keys();
            qreal hx = 0;
            qreal stdev = 0;
            int counter = 0;
            for (const QString& element : qAsConst(k)) {
                if (element == "controller")
                    continue;

                QJsonObject result = obj[element].toObject();
                QJsonObject box = result["boxplot"].toObject();
                QVector<qreal> list = ToolSet::String2DoubleVec(result["data"].toObject()["raw"].toString());
                QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(list, bins);
                ToolSet::Normalise(histogram);
                QPair<qreal, qreal> pair = ToolSet::Entropy(histogram);
                // QJsonObject entropy = ToolSet::CalculateShannonEntropy(histogram);
                if (bins != EntropyBins)
                    bin_info = QString("Bins had been set to %1").arg(bins);

                QString name = result["name"].toString() + " - " + model["name"].toString() + "   " + bin_info;

                if ((result["type"].toString() == "Local Parameter" && local) || result["type"].toString() == "Global Parameter") {
                    double h = qAbs(pair.first); // entropy["diff"].toDouble(); ///(0.5*(1+log2(2*pi*box["stddev"].toDouble()*box["stddev"].toDouble())));
                    // qDebug() << entropy;
                    //  hx += entropy["ratio"].toDouble();//qAbs(pair.first);
                    // hx += h;
                    stdev += box["stddev"].toDouble();
                    // individual_entropy.insert(/* qAbs(pair.first) */ entropy["ratio"].toDouble(), name);
                    individual_entropy.insert(h, name);
                    individual_stdev.insert(box["stddev"].toDouble(), name);
                    counter++;

                    if (parameters_stdev.contains(result["name"].toString())) {
                        parameters_stdev[result["name"].toString()] += box["stddev"].toDouble();
                        // parameters_h[result["name"].toString()] += entropy["ratio"].toDouble();//qAbs(pair.first);
                        parameters_h[result["name"].toString()] += h;
                        parameters_count[result["name"].toString()]++;
                    } else {
                        parameters_stdev.insert(result["name"].toString(), box["stddev"].toDouble());
                        // parameters_h.insert(result["name"].toString(), entropy["ratio"].toDouble() /*qAbs(pair.first) */);
                        parameters_h.insert(result["name"].toString(), h);
                        parameters_count.insert(result["name"].toString(), 1);
                    }
                }
            }
            model_wise_entropy.insert(hx / double(counter), model["name"].toString());
            model_wise_stdev.insert(stdev / double(counter), model["name"].toString());
        }
    }
    QMap<qreal, QString> parameters_stdev_orderd, parameters_h_orderd;

    for (const QString& str : parameters_stdev.keys()) {
        parameters_stdev_orderd.insert(parameters_stdev[str] / double(parameters_count[str]), str);
        parameters_h_orderd.insert(parameters_h[str] / double(parameters_count[str]), str);
    }
    {
        result += "<tr><th colspan='2'>Initialising comparison of Monte Carlo Simulation!</th></tr>";
        result += "<tr><td colspan='2'>" + method_line + "</td></tr>";
        if (!model_wise_entropy.isEmpty()) {
            result += "<tr><th colspan='2'>modelwise entropy list</th></tr>";

            auto i = model_wise_entropy.begin();
            //qreal first = 0;
            while (i != model_wise_entropy.constEnd()) {
                //if (i == model_wise_entropy.begin())
                //    first = i.key();
                result += "<p>" + i.value() + ":  H(x) :" + Print::printDouble(i.key()) + "</p>";
                ++i;
            }
        }
    }
    {
        if (!individual_entropy.isEmpty()) {
            result += "<tr><th colspan='2'>parameterwise entropy list</th></tr>";

            auto i = individual_entropy.begin();
            //qreal first = 0;
            while (i != individual_entropy.constEnd()) {
                //  if (i == individual_entropy.begin())
                //     first = i.key();
                result += "<p>" + i.value() + ":  H(x) :" + Print::printDouble(i.key()) + "</p>";
                ++i;
            }
        }
    }
    result += QString("<p>Best fitting parameters according to the Shannon entropy H(x) </p>");

    auto param = parameters_h_orderd.constBegin();
    while (param != parameters_h_orderd.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(param.value()).arg(param.key());
        ++param;
    }

    {
        if (!model_wise_stdev.isEmpty()) {
            result += "<tr><th colspan='2'>modelwise " + Unicode_sigma + " list</th></tr>";

            auto i = model_wise_stdev.begin();
            //qreal first = 0;
            while (i != model_wise_stdev.constEnd()) {
                //  if (i == model_wise_stdev.begin())
                //      first = i.key();
                result += "<p>" + i.value() + ": " + Unicode_sigma + " :" + Print::printDouble(i.key()) + "</p>";
                ++i;
            }
        }
    }
    {
        if (!individual_stdev.isEmpty()) {
            result += "<tr><th colspan='2'>parameterwise " + Unicode_sigma + " list</th></tr>";

            auto i = individual_stdev.begin();
            // qreal first = 0;
            while (i != individual_stdev.constEnd()) {
                //  if (i == individual_stdev.begin())
                //      first = i.key();
                result += "<p>" + i.value() + ": " + Unicode_sigma + " :" + Print::printDouble(i.key()) + "</p>";
                ++i;
            }
        }
    }

    result += QString("<p>Best fitting parameters according to %1 </p>").arg(Unicode_sigma);

    param = parameters_stdev_orderd.constBegin();
    while (param != parameters_stdev_orderd.constEnd()) {
        result += QString("<p> %1 : %2</p>").arg(param.value()).arg(param.key());
        ++param;
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
        QString str(QString::number(i) + " - " + models[i].toStrongRef()->Name());
        list_second.insert(models[i].toStrongRef()->AICc(), str);
        list_first.insert(models[i].toStrongRef()->AIC(), str);
        result += "<tr><td>" + str + ":</td><td>" + Print::printDouble(models[i].toStrongRef()->AICc()) + "</td><td>" + Print::printDouble(models[i].toStrongRef()->AIC()) + "</td></tr>";
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

// JSON-based statistical calculation functions (Claude Generated)

QJsonObject CalculateAICMetrics(const QVector<QWeakPointer<AbstractModel>>& models)
{
    QJsonObject result;
    QJsonArray modelMetrics;
    QMap<qreal, QString> aicRanking;

    qreal minAIC = std::numeric_limits<qreal>::max();

    // Calculate AIC for each model
    for (int i = 0; i < models.size(); ++i) {
        if (auto model = models[i].toStrongRef()) {
            QJsonObject modelData;
            modelData["index"] = i;
            modelData["name"] = model->Name();
            modelData["aic"] = model->AIC();
            modelData["aicc"] = model->AICc();
            modelData["parameters"] = model->GlobalParameterSize() + model->LocalParameterSize();
            modelData["datapoints"] = model->DataPoints();

            modelMetrics.append(modelData);

            aicRanking.insert(model->AIC(), QString::number(i) + " - " + model->Name());

            if (model->AIC() < minAIC)
                minAIC = model->AIC();
        }
    }

    // Calculate evidence ratios and rankings
    QJsonArray aicRankingArray;
    int rank = 1;

    for (auto it = aicRanking.begin(); it != aicRanking.end(); ++it, ++rank) {
        QJsonObject rankData;
        rankData["rank"] = rank;
        rankData["model"] = it.value();
        rankData["aic"] = it.key();
        rankData["evidence_ratio"] = 1.0 / exp(-0.5 * (minAIC - it.key()));
        aicRankingArray.append(rankData);
    }

    result["models"] = modelMetrics;
    result["aic_ranking"] = aicRankingArray;
    result["best_aic"] = minAIC;

    return result;
}

QJsonObject CalculateCVMetrics(const QVector<QJsonObject>& models, int cvtype, int cv_x)
{
    QJsonObject result;
    QJsonArray modelResults;
    QMultiMap<qreal, QString> entropyRanking, stdevRanking;
    QHash<QString, qreal> parameterStdev, parameterEntropy;
    QHash<QString, int> parameterCount;

    QString cvTypeStr = (cvtype == 1) ? "L0O" : (cvtype == 2) ? "L2O"
                                                              : "CXO";

    for (const auto& model : models) {
        QJsonObject modelData;
        modelData["name"] = model["name"].toString();
        QJsonArray parameterResults;

        QJsonObject statistics = model["data"].toObject()["methods"].toObject();
        for (const QString& methodKey : statistics.keys()) {
            QJsonObject method = statistics[methodKey].toObject();
            QJsonObject controller = method["controller"].toObject();

            if (AccessCI(controller, "Method").toInt() != 4 || controller["CXO"].toInt() != cvtype)
                continue;

            if (cvtype == 3 && controller["X"].toInt() != cv_x)
                continue;

            int bins = controller["EntropyBins"].toInt(50);

            for (const QString& paramKey : method.keys()) {
                if (paramKey == "controller")
                    continue;

                QJsonObject param = method[paramKey].toObject();
                QJsonObject box = param["boxplot"].toObject();

                QVector<qreal> rawData = ToolSet::String2DoubleVec(param["data"].toObject()["raw"].toString());
                QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(rawData, bins);
                ToolSet::Normalise(histogram);
                QPair<qreal, qreal> entropy = ToolSet::Entropy(histogram);

                QJsonObject paramResult;
                paramResult["name"] = param["name"].toString();
                paramResult["type"] = param["type"].toString();
                paramResult["entropy"] = qAbs(entropy.first);
                paramResult["stddev"] = box["stddev"].toDouble();
                paramResult["mean"] = box["mean"].toDouble();
                paramResult["median"] = box["median"].toDouble();
                paramResult["bins"] = bins;

                parameterResults.append(paramResult);

                QString paramName = param["name"].toString();
                entropyRanking.insert(qAbs(entropy.first), paramName + " - " + model["name"].toString());
                stdevRanking.insert(box["stddev"].toDouble(), paramName + " - " + model["name"].toString());

                if (parameterStdev.contains(paramName)) {
                    parameterStdev[paramName] += box["stddev"].toDouble();
                    parameterEntropy[paramName] += qAbs(entropy.first);
                    parameterCount[paramName]++;
                } else {
                    parameterStdev[paramName] = box["stddev"].toDouble();
                    parameterEntropy[paramName] = qAbs(entropy.first);
                    parameterCount[paramName] = 1;
                }
            }
        }

        modelData["parameters"] = parameterResults;
        modelResults.append(modelData);
    }

    // Calculate average parameter statistics
    QJsonArray parameterAverages;
    for (auto it = parameterStdev.begin(); it != parameterStdev.end(); ++it) {
        QJsonObject avg;
        avg["parameter"] = it.key();
        avg["avg_stddev"] = it.value() / parameterCount[it.key()];
        avg["avg_entropy"] = parameterEntropy[it.key()] / parameterCount[it.key()];
        avg["model_count"] = parameterCount[it.key()];
        parameterAverages.append(avg);
    }

    result["cv_type"] = cvTypeStr;
    result["cv_x"] = cv_x;
    result["models"] = modelResults;
    result["parameter_averages"] = parameterAverages;

    return result;
}

QJsonObject CalculateMCMetrics(const QVector<QJsonObject>& models, int index)
{
    QJsonObject result;
    QJsonArray modelResults;
    QMultiMap<qreal, QString> entropyRanking, stdevRanking;
    QHash<QString, qreal> parameterStdev, parameterEntropy;
    QHash<QString, int> parameterCount;

    int maxSteps = -1;
    double variance = 0.0;

    for (const auto& model : models) {
        QJsonObject modelData;
        modelData["name"] = model["name"].toString();
        QJsonArray parameterResults;

        QJsonObject statistics = model["data"].toObject()["methods"].toObject();
        for (const QString& methodKey : statistics.keys()) {
            QJsonObject method = statistics[methodKey].toObject();
            QJsonObject controller = method["controller"].toObject();

            if (AccessCI(controller, "Method").toInt() != SupraFit::Method::MonteCarlo)
                continue;

            if (maxSteps == -1) {
                maxSteps = controller["MaxSteps"].toInt();
                variance = controller["Variance"].toDouble();
            }

            if (maxSteps != controller["MaxSteps"].toInt())
                continue;

            int bins = controller["EntropyBins"].toInt(50);

            for (const QString& paramKey : method.keys()) {
                if (paramKey == "controller")
                    continue;

                QJsonObject param = method[paramKey].toObject();
                QJsonObject box = param["boxplot"].toObject();

                QVector<qreal> rawData = ToolSet::String2DoubleVec(param["data"].toObject()["raw"].toString());
                QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(rawData, bins);
                ToolSet::Normalise(histogram);
                QPair<qreal, qreal> entropy = ToolSet::Entropy(histogram);

                QJsonObject paramResult;
                paramResult["name"] = param["name"].toString();
                paramResult["type"] = param["type"].toString();
                paramResult["entropy"] = qAbs(entropy.first);
                paramResult["stddev"] = box["stddev"].toDouble();
                paramResult["mean"] = box["mean"].toDouble();
                paramResult["median"] = box["median"].toDouble();

                // Use percentile-based confidence intervals from existing data - Claude Generated
                if (param.contains("confidence")) {
                    QJsonObject confidence = param["confidence"].toObject();
                    paramResult["confidence_lower"] = confidence["lower"].toDouble();
                    paramResult["confidence_upper"] = confidence["upper"].toDouble();
                    paramResult["confidence_error"] = confidence["error"].toDouble();
                    paramResult["confidence_range"] = confidence["upper"].toDouble() - confidence["lower"].toDouble();
                    paramResult["relative_uncertainty"] = (confidence["error"].toDouble() / param["value"].toDouble()) * 100.0;
                }

                // Boxplot quartiles for additional uncertainty measures - Claude Generated
                paramResult["lower_quartile"] = box["lower_quantile"].toDouble();
                paramResult["upper_quartile"] = box["upper_quantile"].toDouble();
                paramResult["iqr"] = box["upper_quantile"].toDouble() - box["lower_quantile"].toDouble();
                paramResult["bins"] = bins;

                parameterResults.append(paramResult);

                QString paramName = param["name"].toString();
                entropyRanking.insert(qAbs(entropy.first), paramName + " - " + model["name"].toString());
                stdevRanking.insert(box["stddev"].toDouble(), paramName + " - " + model["name"].toString());

                if (parameterStdev.contains(paramName)) {
                    parameterStdev[paramName] += box["stddev"].toDouble();
                    parameterEntropy[paramName] += qAbs(entropy.first);
                    parameterCount[paramName]++;
                } else {
                    parameterStdev[paramName] = box["stddev"].toDouble();
                    parameterEntropy[paramName] = qAbs(entropy.first);
                    parameterCount[paramName] = 1;
                }
            }
        }

        modelData["parameters"] = parameterResults;
        modelResults.append(modelData);
    }

    // Calculate parameter correlations and averages
    QJsonArray parameterAverages;
    for (auto it = parameterStdev.begin(); it != parameterStdev.end(); ++it) {
        QJsonObject avg;
        avg["parameter"] = it.key();
        avg["avg_stddev"] = it.value() / parameterCount[it.key()];
        avg["avg_entropy"] = parameterEntropy[it.key()] / parameterCount[it.key()];
        avg["relative_uncertainty"] = (it.value() / parameterCount[it.key()]) * 100.0; // Percentage
        avg["model_count"] = parameterCount[it.key()];
        parameterAverages.append(avg);
    }

    result["max_steps"] = maxSteps;
    result["variance"] = variance;
    result["models"] = modelResults;
    result["parameter_averages"] = parameterAverages;

    return result;
}

QJsonObject CalculateReductionMetrics(const QVector<QJsonObject>& models, double cutoff)
{
    QJsonObject result;
    QJsonArray modelResults;
    QMultiMap<qreal, QString> significanceRanking;
    QHash<QString, qreal> parameterSignificance;
    QHash<QString, int> parameterCount;

    QVector<qreal> cutoffValues;

    for (const auto& model : models) {
        QJsonObject modelData;
        modelData["name"] = model["name"].toString();
        QJsonArray parameterResults;

        QJsonObject methods = model["data"].toObject()["methods"].toObject();
        for (const QString& methodKey : methods.keys()) {
            QJsonObject method = methods[methodKey].toObject();
            QJsonObject controller = method["controller"].toObject();

            if (AccessCI(controller, "Method").toInt() != SupraFit::Method::Reduction)
                continue;

            if (cutoffValues.isEmpty()) {
                cutoffValues = ToolSet::String2DoubleVec(controller["x"].toString());
            }

            for (const QString& paramKey : method.keys()) {
                if (paramKey == "controller")
                    continue;

                QJsonObject param = method[paramKey].toObject();
                QVector<qreal> yValues = ToolSet::String2DoubleVec(param["y"].toString());
                QVector<qreal> yValuesCorr = ToolSet::String2DoubleVec(param["y_corr"].toString());

                QJsonObject paramResult;
                paramResult["name"] = param["name"].toString();
                paramResult["x_values"] = QJsonArray::fromVariantList(
                    QVariantList(cutoffValues.begin(), cutoffValues.end()));
                paramResult["y_values"] = QJsonArray::fromVariantList(
                    QVariantList(yValues.begin(), yValues.end()));
                paramResult["y_corrected"] = QJsonArray::fromVariantList(
                    QVariantList(yValuesCorr.begin(), yValuesCorr.end()));

                // Find significance at cutoff
                double significance = 0.0;
                for (int i = 0; i < cutoffValues.size(); ++i) {
                    if (cutoffValues[i] >= cutoff) {
                        significance = (i < yValues.size()) ? yValues[i] : 0.0;
                        break;
                    }
                }

                paramResult["significance_at_cutoff"] = significance;
                paramResult["is_significant"] = significance > cutoff;

                parameterResults.append(paramResult);

                QString paramName = param["name"].toString();
                significanceRanking.insert(significance, paramName + " - " + model["name"].toString());

                if (parameterSignificance.contains(paramName)) {
                    parameterSignificance[paramName] += significance;
                    parameterCount[paramName]++;
                } else {
                    parameterSignificance[paramName] = significance;
                    parameterCount[paramName] = 1;
                }
            }
        }

        modelData["parameters"] = parameterResults;
        modelResults.append(modelData);
    }

    // Calculate parameter importance ranking
    QJsonArray parameterRanking;
    QMultiMap<qreal, QString> avgSignificance;
    for (auto it = parameterSignificance.begin(); it != parameterSignificance.end(); ++it) {
        double avg = it.value() / parameterCount[it.key()];
        avgSignificance.insert(avg, it.key());
    }

    int rank = 1;
    auto reverseIt = avgSignificance.end();
    while (reverseIt != avgSignificance.begin()) {
        --reverseIt;
        QJsonObject rankData;
        rankData["rank"] = rank;
        rankData["parameter"] = reverseIt.value();
        rankData["avg_significance"] = reverseIt.key();
        rankData["is_important"] = reverseIt.key() > cutoff;
        parameterRanking.append(rankData);
        ++rank;
    }

    result["cutoff"] = cutoff;
    result["models"] = modelResults;
    result["parameter_ranking"] = parameterRanking;

    return result;
}

QJsonObject ExtractModelMLFeatures(QSharedPointer<AbstractModel> model)
{
    QJsonObject features;

    if (!model) {
        return features;
    }

    // Basic model characteristics
    features["model_name"] = model->Name();
    features["model_id"] = model->SFModel();
    features["global_parameters"] = model->GlobalParameterSize();
    features["local_parameters"] = model->LocalParameterSize();
    features["total_parameters"] = model->GlobalParameterSize() + model->LocalParameterSize();
    features["datapoints"] = model->DataPoints();
    features["series_count"] = model->SeriesCount();

    // Fit quality metrics
    features["aic"] = model->GetAIC();
    features["aicc"] = model->GetAICc();
    features["sse"] = model->SSE();
    features["rmse"] = std::sqrt(model->SSE() / model->DataPoints());
    features["sigma"] = model->sigma();
    features["converged"] = model->isConverged();

    // Degrees of freedom and complexity
    int dof = model->DataPoints() - (model->GlobalParameterSize() + model->LocalParameterSize());
    features["degrees_of_freedom"] = dof;
    features["complexity_ratio"] = double(model->GlobalParameterSize() + model->LocalParameterSize()) / model->DataPoints();

    // Parameter values for ML training
    QJsonArray globalParams, localParams;
    for (int i = 0; i < model->GlobalParameterSize(); ++i) {
        globalParams.append(model->GlobalParameter(i));
    }

    for (int i = 0; i < model->LocalParameterSize(); ++i) {
        QJsonArray seriesParams;
        for (int j = 0; j < model->SeriesCount(); ++j) {
            seriesParams.append(model->LocalParameter(i, j));
        }
        localParams.append(seriesParams);
    }

    features["global_parameter_values"] = globalParams;
    features["local_parameter_values"] = localParams;

    // Statistical indicators
    if (dof > 0) {
        features["reduced_chi_squared"] = model->SSE() / dof;
        features["normalized_rmse"] = std::sqrt(model->SSE() / model->DataPoints()) / model->sigma();
    }

    return features;
}

QString FormatStatisticsString(const QJsonObject& statisticsJson, const QString& analysisType)
{
    QString result;

    if (analysisType == "AIC") {
        result = "<h3>Akaike Information Criterion Analysis</h3><table>";
        result += "<tr><th>Model</th><th>AIC</th><th>Evidence Ratio</th></tr>";

        QJsonArray ranking = statisticsJson["aic_ranking"].toArray();
        for (const auto& entry : ranking) {
            QJsonObject rank = entry.toObject();
            result += QString("<tr><td>%1</td><td>%2</td><td>%3</td></tr>")
                          .arg(rank["model"].toString())
                          .arg(Print::printDouble(rank["aic"].toDouble()))
                          .arg(Print::printDouble(rank["evidence_ratio"].toDouble()));
        }
        result += "</table>";

    } else if (analysisType == "MC") {
        result = "<h3>Monte Carlo Parameter Uncertainty Analysis</h3>";
        result += QString("<p>Simulation Steps: %1, Variance: %2</p>")
                      .arg(statisticsJson["max_steps"].toInt())
                      .arg(statisticsJson["variance"].toDouble());

        QJsonArray models = statisticsJson["models"].toArray();
        for (const auto& modelEntry : models) {
            QJsonObject model = modelEntry.toObject();
            result += QString("<h4>Model: %1</h4><table>").arg(model["name"].toString());
            result += "<tr><th>Parameter</th><th>Mean</th><th>Std Dev</th><th>Confidence Interval</th><th>Entropy</th></tr>";

            QJsonArray params = model["parameters"].toArray();
            for (const auto& paramEntry : params) {
                QJsonObject param = paramEntry.toObject();
                QString confInterval = QString("[%1, %2]")
                                           .arg(Print::printDouble(param["confidence_lower"].toDouble()))
                                           .arg(Print::printDouble(param["confidence_upper"].toDouble()));

                result += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                              .arg(param["name"].toString())
                              .arg(Print::printDouble(param["mean"].toDouble()))
                              .arg(Print::printDouble(param["stddev"].toDouble()))
                              .arg(confInterval)
                              .arg(Print::printDouble(param["entropy"].toDouble()));
            }
            result += "</table>";
        }

    } else if (analysisType == "CV") {
        result = QString("<h3>Cross Validation Analysis (%1)</h3>").arg(statisticsJson["cv_type"].toString());

        QJsonArray models = statisticsJson["models"].toArray();
        for (const auto& modelEntry : models) {
            QJsonObject model = modelEntry.toObject();
            result += QString("<h4>Model: %1</h4><table>").arg(model["name"].toString());
            result += "<tr><th>Parameter</th><th>Type</th><th>Std Dev</th><th>Entropy</th></tr>";

            QJsonArray params = model["parameters"].toArray();
            for (const auto& paramEntry : params) {
                QJsonObject param = paramEntry.toObject();
                result += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                              .arg(param["name"].toString())
                              .arg(param["type"].toString())
                              .arg(Print::printDouble(param["stddev"].toDouble()))
                              .arg(Print::printDouble(param["entropy"].toDouble()));
            }
            result += "</table>";
        }

    } else if (analysisType == "Reduction") {
        result = QString("<h3>Parameter Reduction Analysis (Cutoff: %1)</h3>")
                     .arg(statisticsJson["cutoff"].toDouble());

        result += "<table><tr><th>Rank</th><th>Parameter</th><th>Significance</th><th>Important</th></tr>";
        QJsonArray ranking = statisticsJson["parameter_ranking"].toArray();
        for (const auto& entry : ranking) {
            QJsonObject rank = entry.toObject();
            result += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                          .arg(rank["rank"].toInt())
                          .arg(rank["parameter"].toString())
                          .arg(Print::printDouble(rank["avg_significance"].toDouble()))
                          .arg(rank["is_important"].toBool() ? "Yes" : "No");
        }
        result += "</table>";

    } else if (analysisType == "MLFeatures") {
        result = "<h3>ML Feature Extraction</h3><table>";
        result += QString("<tr><td>Model</td><td>%1</td></tr>").arg(statisticsJson["model_name"].toString());
        result += QString("<tr><td>Parameters</td><td>%1</td></tr>").arg(statisticsJson["total_parameters"].toInt());
        result += QString("<tr><td>Data Points</td><td>%1</td></tr>").arg(statisticsJson["datapoints"].toInt());
        result += QString("<tr><td>AIC</td><td>%1</td></tr>").arg(Print::printDouble(statisticsJson["aic"].toDouble()));
        result += QString("<tr><td>RMSE</td><td>%1</td></tr>").arg(Print::printDouble(statisticsJson["rmse"].toDouble()));
        result += QString("<tr><td>Converged</td><td>%1</td></tr>").arg(statisticsJson["converged"].toBool() ? "Yes" : "No");
        result += "</table>";

    } else if (analysisType == "PostFit") {
        result = QString("<h3>Post-Fit Analysis Results</h3>");
        result += QString("<p><strong>Analysis Timestamp:</strong> %1</p>").arg(statisticsJson["analysis_timestamp"].toString());
        result += QString("<p><strong>Model:</strong> %1 (ID: %2)</p>")
                      .arg(statisticsJson["model_name"].toString())
                      .arg(statisticsJson["model_id"].toInt());

        QJsonObject methods = statisticsJson["methods"].toObject();

        // Format Monte Carlo results
        if (methods.contains("MonteCarlo")) {
            QJsonObject mcResult = methods["MonteCarlo"].toObject();
            if (mcResult["execution_status"].toString() == "success") {
                result += FormatStatisticsString(mcResult, "MC");
            } else {
                result += QString("<h4>Monte Carlo Analysis</h4><p>❌ %1</p>").arg(mcResult["error"].toString());
            }
        }

        // Format Cross-Validation results
        if (methods.contains("CrossValidation")) {
            QJsonObject cvResult = methods["CrossValidation"].toObject();
            if (cvResult["execution_status"].toString() == "success") {
                result += FormatStatisticsString(cvResult, "CV");
            } else {
                result += QString("<h4>Cross-Validation Analysis</h4><p>❌ %1</p>").arg(cvResult["error"].toString());
            }
        }

        // Format Reduction results
        if (methods.contains("Reduction")) {
            QJsonObject reductionResult = methods["Reduction"].toObject();
            if (reductionResult["execution_status"].toString() == "success") {
                result += FormatStatisticsString(reductionResult, "Reduction");
            } else {
                result += QString("<h4>Parameter Reduction Analysis</h4><p>❌ %1</p>").arg(reductionResult["error"].toString());
            }
        }

        // Format Model Comparison results
        if (methods.contains("ModelComparison")) {
            QJsonObject compResult = methods["ModelComparison"].toObject();
            if (compResult["execution_status"].toString() == "success") {
                result += FormatStatisticsString(compResult, "AIC");
                result += QString("<p><strong>Confidence Level:</strong> %1%</p>").arg(compResult["confidence_level"].toDouble());
            } else {
                result += QString("<h4>Model Comparison Analysis</h4><p>❌ %1</p>").arg(compResult["error"].toString());
            }
        }
    }

    return result;
}

} // namespace StatisticTool
