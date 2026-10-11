/*
 * SupraFit — ToolSet statistics (histograms, entropy, confidence, boxplots,
 *            model-parameter statistics)
 * Copyright (C) 2017 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

/* Claude Generated — facade-preserving split out of toolset.cpp.
 * The statistics cluster: Normalise / List2Histogram, Shannon entropy,
 * Confidence bars, Box2Object boxplot serialisation, the model-parameter
 * statistics (Model2Parameter / Parameter2Statistic / fromModelsList),
 * finv and MinMax. namespace ToolSet + toolset.h facade unchanged;
 * verbatim move, behaviour-identical. */

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

typedef Eigen::VectorXd Vector;

namespace ToolSet {

void Normalise(QVector<QPair<qreal, qreal>>& hist)
{
    qreal max = 0;
    for (int i = 0; i < hist.size(); ++i)
        max = qMax(max, double(hist[i].second));

    for (int i = 0; i < hist.size(); ++i)
        hist[i].second = hist[i].second / max;
}

QVector<QPair<qreal, qreal>> List2Histogram(const QVector<qreal>& vector, int& bins, qreal min, qreal max)
{
    if (vector.size() == 0)
        return QVector<QPair<qreal, qreal>>() << QPair<qreal, qreal>(0, 0);

    if (qFuzzyCompare(min, max)) {
        min = vector.first();
        max = vector.first();

        for (int i = 0; i < vector.size(); ++i) {
            min = qMin(min, vector[i]);
            max = qMax(max, vector[i]);
        }
    }

    if (bins == 0) {
        if (vector.size() > 1e5)
            bins = vector.size() / 1e4;
        else if (vector.size() < 1e2)
            bins = vector.size() / 1e2;
        else
            bins = 10;
    }
    //    if(bins > vector.size())
    //        bins = vector.size()/2;
    /* I have forgotten where I found that piece of code ... - so no link here*/

    QVector<QPair<qreal, int>> hist;
    double h = (max - min) / bins;
    QVector<double> x(bins, 0);

    for (int j = 0; j < bins; j++) {
        x[j] = min + h / 2. + j * h;
        QPair<qreal, int> bin;
        bin.second = 0;
        bin.first = min + h / 2. + j * h;
        hist << bin;
    }
    for (int i = 0; i < vector.size(); ++i) {
        int jStar = std::floor((vector[i] - min) / h); // use the floor function to get j* for the nearest point to x_j* to phi
        if (jStar >= bins || jStar < 0)
            continue; // if you are outside the grid continue
        hist[jStar].second++;
    }
    QVector<QPair<qreal, qreal>> histogram;

    for (int i = 0; i < hist.size(); ++i)
            histogram << QPair<qreal, qreal>(hist[i].first, hist[i].second);
    return histogram;
}

QPair<qreal, qreal> Entropy(const QVector<QPair<qreal, qreal>>& histogram)
{
    qreal entropy = 0.0;
    qreal sum = 0.0;

    for (int i = 0; i < histogram.size(); ++i) {
        sum += histogram[i].second ;
    }

    /* Since we approximate the real integral
     * int_(-inf)^(+inf) f(x) log2(f(x)) dx
     * with sum, I think, we need the d
     * sum_i^n f(x_i) log2(f(x_i)) d
     */
    qreal d = (histogram.last().first - histogram.first().first) / double(histogram.size());
    qreal lower = 1 / double(histogram.size());

    for (int i = 0; i < histogram.size(); ++i) {
        if (histogram[i].second < lower)
            continue;
        const qreal value = histogram[i].second / sum * d;
        entropy +=  value* log2(value);
    }
    bool fullshannon = qApp->instance()->property("FullShannon").toBool();

    /*
 * There is the chance to calculate the entropy with the additional term (-ld(d))
 * which reverses the results. Higher values for better fitting parameters
 * To test it, change the config in the dialog
 */
    if (fullshannon)
        return QPair<qreal, qreal>(-1 * entropy - log2(d), log2(d));
    else
        return QPair<qreal, qreal>(-1 * entropy, log2(d));
}

QJsonObject CalculateShannonEntropy(const QVector<QPair<qreal, qreal>>& histogram)
{
    QJsonObject entropy;
    qreal h0 = 0.0;
    qreal sum = 0.0;
    qreal dh0 = 0.0;
    QVector<qreal> ordered_histogram, final_histogram;

    for (int i = 0; i < histogram.size(); ++i) {
        sum += histogram[i].second;
        ordered_histogram << histogram[i].second;
    }
    std::sort(ordered_histogram.begin(), ordered_histogram.end(), std::greater<double>());
    int i = 1;
    for (double value : ordered_histogram) {
        if (i % 2 == 0)
            final_histogram.push_back(value);
        else
            final_histogram.push_front(value);
        i++;
    }

    /* Since we approximate the real integral
     * int_(-inf)^(+inf) f(x) log2(f(x)) dx
     * with sum, I think, we need the d
     * sum_i^n f(x_i) log2(f(x_i)) d
     */
    qreal d = (histogram.last().first - histogram.first().first) / double(histogram.size());
    qreal lower = 1 / double(histogram.size());

    for (int i = 0; i < histogram.size(); ++i) {
        qreal value = histogram[i].second / sum;
        qreal tmp = value * log2(value) * d;

        if (histogram[i].second > lower)
            h0 += tmp;

        qreal value2 = final_histogram[i] / sum;
        qreal tmp2 = 0;
        if (value2 > lower)
            tmp2 = value2 * log2(value2) * d;

        // qDebug() << value << histogram[i].second;
        if (!std::isnan(tmp))
            dh0 += sqrt((tmp - tmp2) * (tmp - tmp2));
        if (std::isnan(dh0)) {
            qDebug() << "breakpoint";
        }
    }
    double H0 = -1 * h0;
    double ld = log2(d);
    double H = H0 - ld;
    double max0 = -log2(1 / double(histogram.size()));
    double max = max0 - ld;
    double dH0 = dh0;
    double dH = dH0 - ld;

    entropy["H0"] = H0;
    entropy["ld"] = ld;
    entropy["H"] = H;
    entropy["dH0"] = dH0;
    entropy["dH"] = dH;

    entropy["max0"] = max0;
    entropy["max"] = max;
    entropy["ratio"] = H / dH;
    entropy["diff"] = qAbs(H0 - dH0);

    // qDebug() << entropy << d << lower;
    return entropy;
}
QJsonObject CalculateShannonEntropy(const QVector<qreal>& vector, int& bins, qreal min, qreal max)
{

    return CalculateShannonEntropy(List2Histogram(vector, bins, min, max));
}

SupraFit::ConfidenceBar Confidence(const QList<qreal>& list, qreal error)
{
    SupraFit::ConfidenceBar result;
    if (list.isEmpty())
        return result;
    error /= 2;

    if (qFuzzyCompare(error, 0)) {
        result.lower = list.first();
        result.upper = list.last();
    } else if (qFuzzyCompare(error, 100)) {
        int max = list.size();
        int pos = max / 2;
        if (max % 2 == 1) {
            result.lower = list[pos];
            result.upper = list[pos + 1];
        } else {
            result.lower = list[pos];
            result.upper = list[pos];
        }
    } else {
        int max = list.size();
        int pos_upper = round(max * (1 - error / 100));
        int pos_lower = round(max * (error / 100));
        if (pos_lower == 0)
            pos_lower = 1;
        if (pos_upper == 0)
            pos_upper = 1;
        /* returns now identical values compared to octaves quantile functions
         * at least for the tested 10, 100, 1000 step mc simulations
         */
        if (max >= 1000) {
            result.lower = (list[pos_lower - 1] + list[pos_lower]) / 2.0;
            result.upper = (list[pos_upper - 1] + list[pos_upper]) / 2.0;
        } else {
            result.lower = list[pos_lower - 1];
            result.upper = list[pos_upper - 1];
        }
    }
    return result;
}

BoxWhisker BoxWhiskerPlot(const QList<qreal>& list)
{
    BoxWhisker bw;
    int count = list.size();
    if (count == 0)
        return bw;
    /* inspired by qt docs: Box and Whiskers Example
         * https://doc.qt.io/qt-5.10/qtcharts-boxplotchart-example.html
         */
    auto Median = [](const QList<qreal>& list, int begin, int end) {
        int count = end - begin;
        if (count % 2) {
            return list.at(count / 2 + begin);
        } else {
            qreal right = list.at(count / 2 + begin);
            qreal left = list.at(count / 2 - 1 + begin);
            return (right + left) / 2.0;
        }
    };

    bw.median = Median(list, 0, count);
    bw.lower_quantile = Median(list, 0, count / 2);
    bw.upper_quantile = Median(list, count / 2 + (count % 2), count);
    bw.lower_whisker = bw.lower_quantile;
    bw.upper_whisker = bw.upper_quantile;
    /*
         * plagiate stopped
         */
    bw.count = count;
    qreal iqd = bw.upper_quantile - bw.lower_quantile;

    for (int i = 0; i < count; ++i) {
        bw.mean += list[i];
        if (list[i] < bw.median - 3 * iqd || list[i] > bw.median + 3 * iqd)
            bw.extreme_outliers << list[i];
        else if (list[i] < bw.median - 1.5 * iqd || list[i] > bw.median + 1.5 * iqd)
            bw.mild_outliers << list[i];
        else {
            bw.lower_whisker = qMin(list[i], bw.lower_whisker);
            bw.upper_whisker = qMax(list[i], bw.upper_whisker);
        }
    }
    bw.mean /= double(count);

    bw.stddev = Stddev(list.toVector());

    return bw;
}

QJsonObject Box2Object(const BoxWhisker& box)
{
    QJsonObject object;
    object["lower_whisker"] = box.lower_whisker;
    object["upper_whisker"] = box.upper_whisker;
    object["lower_quantile"] = box.lower_quantile;
    object["upper_quantile"] = box.upper_quantile;
    object["median"] = box.median;
    object["mean"] = box.mean;
    object["stddev"] = box.stddev;
    object["count"] = box.count;
    object["extreme_outliers"] = DoubleList2String(box.extreme_outliers);
    object["mild_outliers"] = DoubleList2String(box.mild_outliers);
    return object;
}

BoxWhisker Object2Whisker(const QJsonObject& object)
{
    BoxWhisker box;
    box.lower_whisker = object["lower_whisker"].toDouble();
    box.upper_whisker = object["upper_whisker"].toDouble();
    box.lower_quantile = object["lower_quantile"].toDouble();
    box.upper_quantile = object["upper_quantile"].toDouble();
    box.median = object["median"].toDouble();
    box.stddev = object["stddev"].toDouble(0);
    box.mean = object["mean"].toDouble();
    box.count = object["count"].toDouble();
    box.extreme_outliers = String2DoubleList(object["extreme_outliers"].toString());
    box.mild_outliers = String2DoubleList(object["mild_outliers"].toString());
    return box;
}

QList<QJsonObject> Model2Parameter(const QList<QJsonObject>& models, bool sort)
{
    int globalcount = 0, localcount = 0, each_local = 0;
    QJsonObject model = models.first()["data"].toObject();
    globalcount = model["globalParameter"].toObject()["cols"].toInt();
    localcount = model["localParameter"].toObject()["rows"].toInt();
    QStringList local_names, global_names;
    local_names = model["localParameter"].toObject()["header"].toString().split("|");
    each_local = model["localParameter"].toObject()["cols"].toInt();
    global_names = model["globalParameter"].toObject()["header"].toString().split("|");

    QVector<QVector<qreal>> global(globalcount);
    QVector<QVector<qreal>> local(localcount * each_local);
    QVector<int> int_keys; // We need this keys as int, since single "signals" could have been omitted and therefore there can be gaps in the list

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = models[i]["data"].toObject();

        QJsonObject globalObject = model["globalParameter"].toObject();
        QJsonObject localObject = model["localParameter"].toObject();
        QVector<qreal> vector = String2DoubleVec(globalObject["data"].toObject()["0"].toString());
        for (int j = 0; j < globalcount; ++j)
            global[j] << vector[j];

        /* Series free models may have no entry or an empty entry for active_series
         * Therefor we have fill the list, if it is smaller
         * Lets fill it with ones, otherwise no local parameter will be evaluated */
        QList<int> active = String2IntList(model["active_series"].toString());
        while (active.size() < localcount)
            active << 1;

        for (int j = 0; j < localcount; ++j) {
            QList<qreal> values = String2DoubleList(localObject["data"].toObject()[QString::number(j)].toString());
            QList<qreal> checked = String2DoubleList(localObject["checked"].toObject()[QString::number(j)].toString());
            if (i == 0)
                int_keys << j; //active[j];

            if (active[j] == 0)
                continue;

            for (int k = 0; k < each_local; ++k) {
                if (checked[k])
                    local[each_local * j + k] << values[k];
            }
        }
    }

    if (sort) {
        for (int i = 0; i < global.size(); ++i) {
            QVector<double> vector = global[i];
            std::sort(vector.begin(), vector.end());
            global[i] = vector;
        }
        for (int i = 0; i < local.size(); ++i) {
            QVector<double> vector = local[i];
            std::sort(vector.begin(), vector.end());
            local[i] = vector;
        }
    }

    QList<QJsonObject> parameter;

    for (int i = 0; i < globalcount; ++i) {
        QJsonObject object;
        QJsonObject data;
        data["raw"] = DoubleVec2String(global[i]);
        object["data"] = data;
        object["name"] = global_names[i];
        object["type"] = "Global Parameter";
        object["index"] = QString::number(i);
        parameter << object;
    }

    for (int i = 0; i < int_keys.size(); ++i) {
        for (int j = 0; j < each_local; ++j) {
            QJsonObject object;
            QJsonObject data;
            if (local[each_local * i + j].size() == 0)
                continue;
            data["raw"] = DoubleVec2String(local[each_local * i + j]);
            object["data"] = data;
            object["name"] = local_names[j];
            object["type"] = "Local Parameter";
            object["index"] = QString::number(j) + "|" + QString::number(int_keys[i]); /* j - local parameter within series, i - ascending series */
            parameter << object;
        }
    }
    return parameter;
}

void Parameter2Statistic(QList<QJsonObject>& parameter, const QPointer<AbstractModel> model)
{
    for (int i = 0; i < parameter.size(); ++i) {
        QList<qreal> list = ToolSet::String2DoubleList(parameter[i]["data"].toObject()["raw"].toString());
        if (parameter[i]["type"].toString() == "Global Parameter")
            parameter[i]["value"] = model->GlobalParameter(parameter[i]["index"].toString().toInt());
        else {
            QStringList index = parameter[i]["index"].toString().split("|");
            parameter[i]["value"] = model->LocalParameter(index[0].toInt(), index[1].toInt());
        }
        parameter[i]["boxplot"] = ToolSet::Box2Object(ToolSet::BoxWhiskerPlot(list));
    }
}

QList<QPointF> fromModelsList(const QList<QJsonObject>& models, const QString& str)
{
    QList<QPointF> series;
    for (const QJsonObject& obj : qAsConst(models)) {
        QJsonObject constants = obj["data"].toObject()[str].toObject();
        series << QPointF(constants[QString::number(0)].toString().toDouble(), constants[QString::number(1)].toString().toDouble());
    }
    return series;
}

qreal finv(qreal p, int m, int n)
{
    qreal f_value(1);
    try {
        f_value = Fisher_Dist::finv(p, m, n);
    } catch (int i) {
        /*
        if (i == -1)
            qDebug() << "p value out of range";
        else if (i == -2)
            qDebug() << "m or n are below 0";
            */
    }
    return f_value;
}
QPair<qreal, qreal> MinMax(const QList<qreal>& vector)
{
    if (vector.size() == 0)
        return QPair<qreal, qreal>(0, 0);

    qreal min = vector.first();
    qreal max = vector.first();

    for (int i = 0; i < vector.size(); ++i) {
        min = qMin(min, vector[i]);
        max = qMax(max, vector[i]);
    }
    return QPair<qreal, qreal>(min, max);
}
} // namespace ToolSet
