/*
 * <one line to give the program's name and a brief idea of what it does.>
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
 */

// Claude Generated: string/HTML formatting split out of toolset.cpp (2026). Holds the Print
// formatting helpers (Html2Tex, Html2Raw, TextFromConfidence, TextFromStatistic, printDouble,
// printConcentration); the namespace Print and toolset.h facade are unchanged.

#include <boxwhisker.h>

#include "jsonhandler.h"
#include "libmath.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <Eigen/Dense>

#include <functional>

#include "toolset.h"

namespace Print {


QString Html2Tex(const QString& str)
{
    QString result;

    result = str;
    result.replace("<sub>", "_{");
    result.replace("</sub>", "}");
    result.replace("<sup>", "^{");
    result.replace("</sup>", "}");

    return "$" + result + "$";
}

QString TextFromConfidence(const QJsonObject& result, const QJsonObject& controller)
{
    int type = AccessCI(controller, "Method").toInt();

    int bins;
    if (qApp->instance()->property("OverwriteBins").toBool())
        bins = qApp->instance()->property("EntropyBins").toInt();
    else
        bins = controller["EntropyBins"].toInt(qApp->instance()->property("EntropyBins").toInt());

    qreal value = result["value"].toDouble();

    QString text = QString("\n\n\n<p>%1</p>").arg(result["name"].toString() + " of type " + result["type"].toString() + ": optimal value = " + Print::printDouble(value, 4));

    QJsonObject confidence = result["confidence"].toObject();
    qreal upper = confidence["upper"].toDouble();
    qreal lower = confidence["lower"].toDouble();
    qreal conf = confidence["error"].toDouble();

    QString const_name = result["name"].toString();
    if (result["type"] == "Local Parameter") {
        if (result.contains("index")) {
            int index = result["index"].toString().split("|")[1].toInt();
            const_name += QString(" - Series %1").arg(index + 1);
        }
    }

    //text += "<table><tr><th colspan='3'> " + result["name"].toString() + " of type " + result["type"].toString() + ": optimal value = " + Print::printDouble(value) + "</th></tr>";
    if (type == SupraFit::Method::CrossValidation || type == SupraFit::Method::MonteCarlo || type == SupraFit::Method::ModelComparison || type == SupraFit::Method::WeakenedGridSearch || type == SupraFit::Method::FastConfidence) {
        text += "<tr><td><b>" + const_name + ":</b></td><td>" + Print::printDouble(value, 4) + " [+ " + Print::printDouble(upper - value, 4) + " /- " + Print::printDouble(value - lower, 4) + " ]</td></tr>";
        text += "<tr><td>" + QString::number(conf, 'f', 2) + "% Confidence Intervall: </td><td>[ " + Print::printDouble(lower, 4) + " - " + Print::printDouble(upper, 4) + " ]</td></tr>";
    }
    if (type == SupraFit::Method::MonteCarlo || type == SupraFit::Method::CrossValidation) {
        if (type == SupraFit::Method::CrossValidation) {
            if(controller["CVType"].toInt() == 1)
                text += "<p>Leave-One-Out Cross Validation</p>";
            else if (controller["CVType"].toInt() == 2)
                text += "<p>Leave-Two-Out Cross Validation</p>";
            else
                text += QString("<p>Leave-%1-Out Cross Validation</p>").arg(controller["X"].toInt());
        } else {
            if (controller["VarianceSource"].toInt() != 4)
                text += QString("<p>Input variance is %1</p>").arg(Print::printDouble(controller["Variance"].toDouble(), 6));
            else
                text += QString("<p>Bootstrapping has been used.</p>");
        }
        text += QString("<tr><td>Inter-percentile range for %1</td><td>%2</td></tr>").arg(result["name"].toString()).arg(Print::printDouble(upper - lower, 4));
        QVector<qreal> list = ToolSet::String2DoubleVec(result["data"].toObject()["raw"].toString());
        QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(list, bins);
        ToolSet::Normalise(histogram);
        QPair<qreal, qreal> pair = ToolSet::Entropy(histogram);
        QJsonObject entropy = ToolSet::CalculateShannonEntropy(histogram);
        // qDebug() << entropy;
        BoxWhisker box;

        /* If old results are used, that don't contain stddev in their json object, recalculate box-plot */
        if (result["boxplot"].toObject().contains("stddev"))
            box = ToolSet::Object2Whisker(result["boxplot"].toObject());
        else
            box = ToolSet::BoxWhiskerPlot(list.toList());

        text += "<tr><td colspan='2'>Analyse of the Monte Carlo Histogram</td></tr>";
        text += "<tr><td>Median: </td><td> <b>" + Print::printDouble(box.median, 4) + "</b></td></tr>";
        text += "<tr><td>Notches: </td><td> <b>" + Print::printDouble(box.LowerNotch(), 4) + " - " + Print::printDouble(box.UpperNotch(), 4) + "</b></td></tr>";
        text += QString("<tr><td>Entropy H(X):</td><td>%1 + %2<td></tr>").arg(Print::printDouble(pair.first, 6)).arg(Print::printDouble(pair.second, 6));
        text += QString("<tr><td>Standard deviation from mean:</td><td>%1<td></tr>").arg(Print::printDouble(box.stddev, 6));

        //   if (value > box.UpperNotch() || value < box.LowerNotch()) {
        //       text += "<tr><th colspan=2><font color='red'>Estimated value exceeds notch of BoxPlot!</font></th></tr>";
        //   }

        text += "<tr><td colspan=2></th></tr>";
        text += QString("<tr><td colspan='2'>%9 & %1 & \\ce{^{+%2}_{%3}} & %4 & %5 & %6  & %7 & %8\\\\[2mm]</td>").arg(Print::printDouble(value, 4)).arg(Print::printDouble(upper - value, 4)).arg(Print::printDouble(lower - value, 4)).arg(Print::printDouble(lower, 4)).arg(Print::printDouble(upper, 4)).arg(Print::printDouble(box.median, 4)).arg(Print::printDouble(box.stddev, 6)).arg(Print::printDouble(pair.first, 6)).arg(Html2Tex(result["name"].toString()));
        text += "<tr><td colspan=2></th></tr>";
        //     for(const QString &str : entropy.keys())
        //         text += QString("%1 - %2<br>").arg(str).arg(entropy[str].toDouble());
    }

    if (type == SupraFit::Method::WeakenedGridSearch) {
        bool converged = result["converged"].toBool();
        bool finished = result["finished"].toBool();
        bool stationary = result["stationary"].toBool();
        bool fine = converged&&finished&&!stationary;

        int OvershotCounter = result["OvershotCounter"].toInt();
        int ErrorDecreaseCounter = result["ErrorDecreaseCounter"].toInt();
        int ErrorConvergencyCounter = result["ErrorConvergencyCounter"].toInt();

        QString increase_OvershotCounter = QString(), increase_ErrorDecreaseCounter = QString(), increase_ErrorConvergencyCounter = QString(), increase_MaxSteps = QString();
        QString color_start, color_end;

        if(!fine)
        {
            color_start = "<font color='red'>";
            color_end = "</font>";

            if (result["StepsTaken"].toInt() > controller["MaxSteps"].toInt())
                increase_MaxSteps = "Maybe increase MaxSteps or increase ScalingFactor.";

            if (OvershotCounter > controller["OvershotCounter"].toInt())
                increase_OvershotCounter = "Maybe increase OvershotCounter.";

            if (ErrorDecreaseCounter > controller["ErrorDecreaseCounter"].toInt())
                increase_ErrorDecreaseCounter = "Maybe increase ErrorDecreaseCounter.";

            if (ErrorConvergencyCounter > controller["ErrorConvergencyCounter"].toInt())
                increase_ErrorConvergencyCounter = "Maybe increase ErrorConvergencyCounter.";
        }
        QVector<qreal> x = ToolSet::String2DoubleVec(result["data"].toObject()["x"].toString());
        QVector<qreal> y = ToolSet::String2DoubleVec(result["data"].toObject()["y"].toString());
        qreal integral = DiscreteIntegrate(x, y);
        text += QString("<tr><td>Integral below curve:</td><td> %1 <b></td></tr>").arg(Print::printDouble(integral)); //.arg( result["steps"].toInt() ).arg(color_end);

        text += "<tr><td colspan='2'>Analyse of the Grid Search Outcome</td></tr>";
        text += QString("<tr><td>%1 Steps: %3</td><td> %1 <b>%2</b>%3</td></tr>").arg(color_start).arg(result["StepsTaken"].toInt()).arg(color_end);
        text += QString("<tr><td>%1 Converged: %3</td><td> %1 <b>%2</b>%3</td></tr>").arg(color_start).arg(ToolSet::bool2YesNo(converged)).arg(color_end);
        text += QString("<tr><td>%1 Stationary: %3</td><td> %1 <b>%2</b>%3</td></tr>").arg(color_start).arg(ToolSet::bool2YesNo(stationary)).arg(color_end);
        text += QString("<tr><td>%1 Finished: %3</td><td> %1 <b>%2</b>%3</td></tr>").arg(color_start).arg(ToolSet::bool2YesNo(finished)).arg(color_end);
        text += QString("<tr><td>%1 OvershotCounter: %3</td><td> %1 <b>%2</b>%3</td></tr>").arg(color_start).arg(OvershotCounter).arg(color_end);
        text += QString("<tr><td>%1 ErrorDecreaseCounter: %3</td><td> %1 <b>%2</b>%3</td></tr>").arg(color_start).arg(ErrorDecreaseCounter).arg(color_end);
        text += QString("<tr><td>%1 ErrorConvergencyCounter: %3</td><td> %1 <b>%2</b>%3</td></tr>").arg(color_start).arg(ErrorConvergencyCounter).arg(color_end);
        if (!fine)
            text += QString("<tr><td colspan='2'>%1 - %2 - %3 - %4</td></tr>").arg(increase_MaxSteps).arg(increase_OvershotCounter).arg(increase_ErrorDecreaseCounter).arg(increase_ErrorConvergencyCounter);
        text += QString("<tr><td>%1 All fine: %3</td><td> %1 <b>%2</b>%3</td></tr>").arg(color_start).arg(ToolSet::bool2YesNo(fine)).arg(color_end);

        text += "<tr><td colspan=2></th></tr>";
        text += QString("<tr><td colspan='2'>%8 & %1 & \\ce{^{+%2}_{%3}} & %4 & %5 & %6 & %7\\\\[2mm]</td>").arg(Print::printDouble(value, 4)).arg(Print::printDouble(upper - value, 4)).arg(Print::printDouble(lower - value, 4)).arg(Print::printDouble(lower, 4)).arg(Print::printDouble(upper, 4)).arg(Print::printDouble(integral, 4)).arg(ToolSet::bool2YesNo(fine)).arg(Html2Tex(result["name"].toString()));
        text += "<tr><td colspan=2></th></tr>";
    }

    if (type == SupraFit::Method::ModelComparison || type == SupraFit::Method::FastConfidence) {
        text += "<tr><td colspan=2></th></tr>";
        text += QString("<tr><td colspan='2'>%6 & %1 & \\ce{^{+%2}_{%3}} & %4 & %5\\\\[2mm]</td>").arg(Print::printDouble(value, 4)).arg(Print::printDouble(upper - value, 4)).arg(Print::printDouble(lower - value, 4)).arg(Print::printDouble(lower, 4)).arg(Print::printDouble(upper, 4)).arg(Html2Tex(result["name"].toString()));
        text += "<tr><td colspan=2></th></tr>";
    }

    if (type == SupraFit::Method::Reduction) {
        auto CalculateReduction = [&text](qreal value, const QVector<qreal>& vector) -> qreal {
            qreal sum_err = 0, max_err = 0, aver_err = 0, aver = 0, stdev = 0;
            for (int i = 0; i < vector.size(); ++i) {
                aver += vector[i];
                sum_err += (value - vector[i]) * (value - vector[i]);
                max_err = qMax(qAbs(value - vector[i]), max_err);
            }
            aver /= double(vector.size());
            aver_err = sqrt(sum_err) / double(vector.size());
            stdev = Stddev(vector);
            text += "<tr><td>Standard deviation : " + Print::printDouble(stdev) + "</td><td> Average Parameter : " + Print::printDouble(aver) + "  </td><td>    </td></tr>";
            text += "<tr><td>Average Error = " + Print::printDouble(aver_err) + "</td><td> Sum of Errors: " + Print::printDouble(sum_err) + "  </td><td>  Max Error = " + Print::printDouble(max_err) + " </td></tr>";
            return stdev;
        };

        qreal value = 0;
        value = result["value"].toDouble();

        QVector<qreal> vector = ToolSet::String2DoubleVec(result["data"].toObject()["raw"].toString());
        text += "<tr><th colspan='2'>Post-Processing the Reduction Analysis without applied cutoff!</th></tr>";

        qreal stdev_full = CalculateReduction(value, vector);

        double cutoff = controller["cutoff"].toDouble();
        QVector<qreal> vector_1, vector_2;
        QVector<qreal> x = ToolSet::String2DoubleVec(controller["x"].toString());
        if (cutoff != -1) {
            text += QString("<tr><th colspan='2'>Post-Processing the Reduction Analysis applied cutoff of %1!</th></tr>").arg(cutoff);

            for (int i = 0; i < vector.size(); ++i) {
                if (x[i] > cutoff)
                    vector_1 << vector[i];
                else
                    vector_2 << vector[i];
            }

            text += QString("<tr><th colspan='2'>Incooperating points above %1!</th></tr>").arg(cutoff);
            qreal stdev_good = CalculateReduction(value, vector_1);

            text += QString("<tr><th colspan='2'>Incooperating points below %1!</th></tr>").arg(cutoff);
            qreal stdev_bad = CalculateReduction(value, vector_2);

            text += QString("<tr><th colspan='2'>Ratio of the Standard deviation is %1, compared to the original %2!</th></tr>").arg(stdev_good / stdev_bad).arg(stdev_full);
        }
    }
    text += "</table>";
    return text;
}

QString TextFromStatistic(const QJsonObject& result)
{

    int converged = 0;
    int invalid = 0;

    double min_error = 1e10, max_error = 0;

    /* One entry is the controller, we dont count them */
    int size = result.keys().size();

    if (result.contains("controller"))
        size--;
    QString text = QString("<html><h3>General Result Overview</h3>");
    if (size == 0) {
        text += "<p>No stored models.</p>";
        return text;
    }

    for (int i = 0; i < size; ++i) {
        QJsonObject local_model;

        if (result[QString::number(i)].toObject().contains("initial"))
            local_model = result[QString::number(i)].toObject()["model"].toObject();
        else
            local_model = result[QString::number(i)].toObject();

        qreal SSE = local_model["SSE"].toDouble();
        min_error = qMin(min_error, SSE);
        max_error = qMax(max_error, SSE);

        converged += local_model["converged"].toBool();
        invalid += local_model["valid"].toBool();
    }
    text += "<p>Models tested: <b>" + QString::number(size) + "</b> </p>";
    text += "<p>Models converged: <b>" + QString::number(converged) + "</b> </p>";
    text += "<p>Models invalid: <b>" + QString::number(size - invalid) + "</b> </p>";
    text += "<p>SSE best model: <b>" + QString::number(min_error) + "</b> </p>";
    text += "<p>SSE worst model: <b>" + QString::number(max_error) + "</b> </p>";

    return text;
}

QString printDouble(double number, int prec)
{
    QString string;
    QLocale local;

    if (number >= 0)
        string += " ";

    if (prec == -1) {
        if (qAbs(number - int(number)) < 1e-30)
            string += local.toString(number);
        else if (qAbs(number) < 1e-17)
            string += "0";
        else if (qAbs(number) < 1e-9)
            string += local.toString(number, 'e', 2);
        else if (qAbs(number) < 1e-4)
            string += local.toString(number, 'e', 3);
        else if (qAbs(number) >= 1e5)
            string += local.toString(number, 'e', 3);
        else
            string += local.toString(number, 'f', 6);
    } else {
        if (qAbs(number - int(number)) < 1e-30)
            string += local.toString(number);
        else if (qAbs(number) < 1e-17)
            string += "0";
        else if (qAbs(number) < 1e-9)
            string += local.toString(number, 'e', prec);
        else if (qAbs(number) < 1e-4)
            string += local.toString(number, 'e', prec);
        else if (qAbs(number) >= 1e5)
            string += local.toString(number, 'e', prec);
        else
            string += local.toString(number, 'f', prec);
    }
    return string;
}

QString printConcentration(double concentration, int prec)
{
    QString result;

    // if (concentration < 1e-3)
    result = QString("%1 %2M").arg(concentration * 1e6, prec).arg(Unicode_mu);
    /* else if (concentration < 1e-1)
        result = QString("%1 mM").arg(concentration * 1e3, prec);
    else
        result = QString("%1 M").arg(concentration, prec);
    */
    return result;
}

QString Html2Raw(QString text)
{
    for (const QString& str : qAsConst(tags))
        text.replace(str, "");
    text.replace("</p>", "\n");
    text.replace("</tr>", "\n");
    text.replace("</td>", "\t");
    text.replace("</th>", "\t");
    text.replace("</br>", "\n");
    text.replace("<sub>", "_{");
    text.replace("</sub>", "}");
    text.replace("&sigma;", "sigma");

    return text;
}

}
