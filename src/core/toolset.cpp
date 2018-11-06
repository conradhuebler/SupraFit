/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "jsonhandler.h"
#include "libmath.h"

#include <QtCore/QDebug>
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

QString DoubleVec2String(const QVector<qreal>& vector, const QString& str)
{
    QString string;
    for (int i = 0; i < vector.size(); ++i)
        string += QString::number(vector[i]) + str;

    return string.left(string.length() - 1);
}

QString IntVec2String(const QVector<int>& vector, const QString& str)
{
    QString string;
    for (int i = 0; i < vector.size(); ++i)
        string += QString::number(vector[i]) + str;

    return string.left(string.length() - 1);
}

QString IntList2String(const QList<int>& vector, const QString& str)
{
    QString string;
    for (int i = 0; i < vector.size(); ++i)
        string += QString::number(vector[i]) + str;

    return string.left(string.length() - 1);
}

QVector<int> VecAndVec(const QVector<int>& a, const QVector<int>& b)
{
    QVector<int> c;

    if (a.size() != b.size())
        return a;

    for (int i = 0; i < a.size(); ++i)
        c << a[i] * b[i];

    return c;
}

QString Int2DVec2String(const QVector<QVector<int>>& vector)
{
    QString string("");
    for (int i = 0; i < vector.size(); ++i)
        string += IntVec2String(vector[i], " ") + "|";

    return string.left(string.length() - 1);
}

QVector<QVector<int>> String2Int2DVec(const QString& str)
{
    QStringList input = str.split("|");
    QVector<QVector<int>> vector;
    for (const QString& string : qAsConst(input))
        vector << String2IntVec(string);

    return vector;
}

QVector<int> VecAndVec(const Vector& a, const QVector<int>& b)
{
    QVector<int> c;

    if (a.size() != b.size()) {
        for (int i = 0; i < a.size(); ++i)
            c << a[i];
    } else {
        for (int i = 0; i < a.size(); ++i)
            c << a[i] * b[i];
    }
    return c;
}

QString DoubleList2String(const QList<qreal>& vector, const QString& str)
{
    QString string;
    for (int i = 0; i < vector.size(); ++i)
        string += QString::number(vector[i]) + str;

    return string.left(string.length() - 1);
}

QString DoubleList2String(const Vector& vector, const QString& str)
{
    QString string;
    for (int i = 0; i < vector.rows(); ++i)
        string += QString::number(vector(i)) + str;

    return string.left(string.length() - 1);
}
QStringList DoubleList2StringList(const Vector& vector)
{
    QStringList list;
    for (int i = 0; i < vector.rows(); ++i)
        list << QString::number(vector(i));
    return list;
}

QVector<qreal> String2DoubleVec(const QString& str)
{
    QVector<qreal> vector;
    if (str.isEmpty())
        return vector;
    QStringList nums = str.split(" ");
    for (const QString& string : qAsConst(nums))
        vector << string.toDouble();
    return vector;
}

QVector<int> String2IntVec(const QString& str)
{
    QVector<int> vector;
    if (str.isEmpty())
        return vector;
    QStringList nums = str.split(" ");
    for (const QString& string : qAsConst(nums))
        vector << string.toInt();
    return vector;
}

QList<qreal> String2DoubleList(const QString& str)
{
    QList<qreal> vector;
    if (str.isEmpty())
        return vector;
    QStringList nums = str.split(" ");
    for (const QString& string : qAsConst(nums))
        vector << string.toDouble();
    return vector;
}

QList<QPointF> String2Points(const QString& str)
{
    QList<QPointF> points;
    QStringList list = str.split(" ");
    for (const QString& point : qAsConst(list)) {
        double x = 0, y = 0;
        QStringList l = point.split(",");
        if (l.size() != 2)
            continue;
        x = l[0].remove("(").toDouble();
        y = l[1].remove(")").toDouble();
        points << QPointF(x, y);
    }
    return points;
}
QString Points2String(const QList<QPointF>& points)
{
    QString string;
    for (int i = 0; i < points.size(); ++i)
        string += "(" + Print::printDouble(points[i].x()) + "," + Print::printDouble(points[i].y()) + ") ";
    return string;
}

QString bool2YesNo(bool var)
{
    return (var ? QString("Yes") : QString("No"));
}

qreal scale(qreal value)
{
    qreal pot;
    return scale(value, pot);
}

qreal scale(qreal value, qreal& pow)
{
    if (qAbs(value) < 1 && value) {
        while (qAbs(value) < 1) {
            pow /= 10;
            value *= 10;
        }
    } else if (qAbs(value) > 10) {
        while (qAbs(value) > 10) {
            pow *= 10;
            value /= 10;
        }
    }
    return value;
}

qreal ceil(qreal value)
{
    if (1 < qAbs(value) && qAbs(value) < 10)
        return std::ceil(value);

    double pot = 1;
    value = scale(value, pot);

    int integer = int(value) + 1;
    if (value < 0)
        integer -= 1;
    return qreal(integer) * pot;
}

qreal floor(qreal value)
{
    if (1 < qAbs(value) && qAbs(value) < 10)
        return std::floor(value);

    double pot = 1;
    value = scale(value, pot);

    int integer = int(value);

    if (value < 0)
        integer -= 1;
    return qreal(integer) * pot;
}

void Normalise(QVector<QPair<qreal, qreal>>& hist)
{
    qreal max = 0;
    for (int i = 0; i < hist.size(); ++i)
        max = qMax(max, double(hist[i].second));

    for (int i = 0; i < hist.size(); ++i)
        hist[i].second = hist[i].second / max;
}

QVector<QPair<qreal, qreal>> List2Histogram(const QVector<qreal>& vector, int bins, qreal min, qreal max)
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

    return QPair<qreal, qreal>(-1 * entropy, log2(histogram.size()));
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

SupraFit::BoxWhisker BoxWhiskerPlot(const QList<qreal>& list)
{
    SupraFit::BoxWhisker bw;
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

QJsonObject Box2Object(const SupraFit::BoxWhisker& box)
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

SupraFit::BoxWhisker Object2Whisker(const QJsonObject& object)
{
    SupraFit::BoxWhisker box;
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

        // QStringList keys = localObject.keys();
        for (int j = 0; j < localcount; ++j) {
            QList<qreal> values = String2DoubleList(localObject["data"].toObject()[QString::number(j)].toString());
            QList<qreal> checked = String2DoubleList(localObject["checked"].toObject()[QString::number(j)].toString());

            for (int k = 0; k < each_local; ++k) {
                if (checked[k])
                    local[each_local * j + k] << values[k];
            }
            if (i == 0)
                int_keys << j;
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
            object["index"] = QString::number(j) + "|" + QString::number(int_keys[i]);
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
        if (i == -1)
            qDebug() << "p value out of range";
        else if (i == -2)
            qDebug() << "m or n are below 0";
    }
    return f_value;
}

QList<int> InvertLockedList(const QList<int>& locked)
{
    QList<int> lock;
    for (int i = 0; i < locked.size(); ++i)
        lock << !locked[i];
    return lock;
}
void ExportResults(const QString& filename, const QList<QJsonObject>& models)
{
    QJsonObject toplevel;
    int i = 0;
    for (const QJsonObject& obj : qAsConst(models)) {
        QJsonObject constants = obj["data"].toObject()["constants"].toObject();
        QStringList keys = constants.keys();
        bool valid = true;
        for (const QString& str : qAsConst(keys)) {
            double var = constants[str].toString().toDouble();
            valid = valid && (var > 0);
        }
        toplevel["model_" + QString::number(i++)] = obj;
    }
    JsonHandler::WriteJsonFile(toplevel, filename);
}

QPair<Vector, Vector> LoadXYFile(const QString& filename)
{
    Vector x, y;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << file.errorString();
        return QPair<Vector, Vector>(x, y);
    }

    QStringList filecontent = QString(file.readAll()).split("\n");

    std::vector<double> entries_x, entries_y;
    for (const QString& str : filecontent) {
        if (!str.contains("#")) {
            QStringList elements = str.simplified().split(" ");
            if (elements.size() == 2) {
                entries_x.push_back(elements[0].toDouble());
                entries_y.push_back(elements[1].toDouble());
            }
        }
    }

    x = Vector::Map(&entries_x[0], entries_x.size());
    y = Vector::Map(&entries_y[0], entries_y.size());

    return QPair<Vector, Vector>(x, y);
}

PeakPick::spectrum LoadITCFile(QString& filename, std::vector<PeakPick::Peak>* peaks, qreal& offset, qreal& freq, QVector<qreal>& inject)
{

    Vector x, y;

    QFileInfo info(filename);
    if (!info.exists()) {
        QStringList list = filename.split("/");
        QString file = list.last();
        QString lastdir = qApp->instance()->property("lastdir").toString();
        QFileInfo inf(lastdir + "/" + file);
        if (inf.exists())
            filename = lastdir + "/" + file;
        else {
            qDebug() << QString(lastdir + "/" + file) << "doesnt work";
            throw 404;
        }
    }
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << file.errorString();
        throw 404;
    }

    QStringList filecontent = QString(file.readAll()).split("\n");

    std::vector<double> entries_x, entries_y;
    bool start_peak = false, skip = false;
    qreal last_x = 0;
    offset = 0;
    PeakPick::Peak floating_peak;
    int i_offset = 0;
    for (const QString& str : filecontent) {
        if (str.contains("$") || str.contains("#") || str.contains("?") || str.contains("%")) {
            if (str.contains("%")) {
            }
            if (str.contains("$")) {
                if (str.simplified().split(" ").size() == 8) {
                    double val = str.simplified().split(" ").last().toDouble();
                    freq = val;
                }
            }
        } else if (str.contains("@")) {
            skip = (str.contains("@0"));
            if (skip)
                continue;
            start_peak = true;
            floating_peak.end = last_x;
            if (last_x && floating_peak.start)
                peaks->push_back(floating_peak);
            inject << str.split(",")[1].toDouble();
        } else {
            QStringList elements = str.simplified().split(",");
            if (elements.size() > 2) {
                if (skip) {
                    offset += elements[1].toDouble();
                    i_offset++;
                    last_x = elements[0].toDouble() / freq;
                }
                entries_x.push_back(elements[0].toDouble());
                entries_y.push_back(elements[1].toDouble());
                if (start_peak) {
                    floating_peak.start = elements[0].toDouble() / freq;
                    start_peak = false;
                }
                last_x = elements[0].toDouble() / freq;
            }
        }
    }

    floating_peak.end = last_x;
    peaks->push_back(floating_peak);

    offset /= double(i_offset);
    if (entries_x.size() < 1 || entries_y.size() < 1 || entries_x.size() != entries_y.size())
        throw 101;

    x = Vector::Map(&entries_x[0], entries_x.size());
    y = Vector::Map(&entries_y[0], entries_y.size());

    return PeakPick::spectrum(y, x[0], x[x.size() - 1]);
}

qreal K2G(qreal logK, qreal T)
{
    return -R * T * logK * log2ln;
}

qreal GHE(qreal G, qreal H, qreal T)
{
    return -(G - H) / T;
}
}

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
    int type = controller["method"].toInt();
    int bins = controller["bins"].toInt(500);

    qreal value = result["value"].toDouble();

    QString text = QString("<p> --- statistic block --- </p><p>%1</p>").arg(result["name"].toString() + " of type " + result["type"].toString() + ": optimal value = " + Print::printDouble(value));

    QJsonObject confidence = result["confidence"].toObject();
    qreal upper = confidence["upper"].toDouble();
    qreal lower = confidence["lower"].toDouble();
    qreal conf = confidence["error"].toDouble();

    QString const_name;
    //text += "<table><tr><th colspan='3'> " + result["name"].toString() + " of type " + result["type"].toString() + ": optimal value = " + Print::printDouble(value) + "</th></tr>";
    if (type == SupraFit::Statistic::MonteCarlo || type == SupraFit::Statistic::ModelComparison || type == SupraFit::Statistic::WeakenedGridSearch || type == SupraFit::Statistic::FastConfidence) {

        text += "<tr><td><b>" + result["name"].toString() + const_name + ":</b></td><td>" + QString::number(value) + " [+ " + QString::number(upper - value, 'g', 3) + " / " + QString::number(lower - value, 'g', 3) + "]</td></tr>";
        text += "<tr><td>" + QString::number(conf, 'f', 2) + "% Confidence Intervall: </td><td>[" + QString::number(lower) + " - " + QString::number(upper) + "]</td></tr>";
    }
    if (type == SupraFit::Statistic::MonteCarlo || type == SupraFit::Statistic::CrossValidation) {

        QVector<qreal> list = ToolSet::String2DoubleVec(result["data"].toObject()["raw"].toString());
        QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(list, bins);
        ToolSet::Normalise(histogram);
        QPair<qreal, qreal> pair = ToolSet::Entropy(histogram);

        SupraFit::BoxWhisker box;

        /* If old results are used, that don't contain stddev in their json object, recalculate box-plot */
        if (result["boxplot"].toObject().contains("stddev"))
            box = ToolSet::Object2Whisker(result["boxplot"].toObject());
        else
            box = ToolSet::BoxWhiskerPlot(list.toList());

        text += "<tr><td colspan='2'>Analyse of the Monte Carlo Histogram</td></tr>\n";
        text += "<tr><td>Median: </td><td> <b>" + QString::number(box.median, 'f', 4) + "</b></td></tr>\n";
        text += "<tr><td>Notches: </td><td> <b>" + QString::number(box.LowerNotch(), 'f', 4) + " - " + QString::number(box.UpperNotch(), 'f', 4) + "</b></td></tr>";
        text += QString("<tr><td>Entropy H(X):</td><td>%1<td></tr>").arg(pair.first);
        text += QString("<tr><td>Standard deviation from mean:</td><td>%1<td></tr>").arg(box.stddev);

        if (value > box.UpperNotch() || value < box.LowerNotch()) {
            text += "<tr><th colspan=2><font color='red'>Estimated value exceeds notch of BoxPlot!</font></th></tr>";
        }

        text += "<tr><td colspan=2></th></tr>";
        text += QString("<tr><td colspan='2'>%9 & %1 & \\ce{^{+%2}_{%3}} & %4 & %5 & %6  & %7 & %8\\\\[2mm]</td>").arg(Print::printDouble(value, 4)).arg(Print::printDouble(upper - value, 4)).arg(Print::printDouble(lower - value, 4)).arg(Print::printDouble(lower, 4)).arg(Print::printDouble(upper, 4)).arg(Print::printDouble(box.median, 4)).arg(Print::printDouble(box.stddev, 6)).arg(Print::printDouble(pair.first, 6)).arg(Html2Tex(result["name"].toString()));
        text += "<tr><td colspan=2></th></tr>";
    }

    if (type == SupraFit::Statistic::WeakenedGridSearch)
    {
        bool converged = result["converged"].toBool();
        bool finished = result["finished"].toBool();
        bool stationary = result["stationary"].toBool();
        bool fine = converged&&finished&&!stationary;

        QString color_start, color_end;
        if(!fine)
        {
            color_start = "<font color='red'>";
            color_end = "</font>";
        }

        text += "<tr><td colspan='2'>Analyse of the Grid Search Outcome</td></tr>\n";
        text += QString("<tr><td>%1 Steps: %3</td><td> %1 <b>%2</b>%3</td></tr>\n").arg(color_start).arg( result["steps"].toInt() ).arg(color_end);
        text += QString("<tr><td>%1 Converged: %3</td><td> %1 <b>%2</b>%3</td></tr>\n").arg(color_start).arg(ToolSet::bool2YesNo(converged)).arg(color_end);
        text += QString("<tr><td>%1 Stationary: %3</td><td> %1 <b>%2</b>%3</td></tr>\n").arg(color_start).arg(ToolSet::bool2YesNo(stationary)).arg(color_end);
        text += QString("<tr><td>%1 Finished: %3</td><td> %1 <b>%2</b>%3</td></tr>\n").arg(color_start).arg(ToolSet::bool2YesNo(finished)).arg(color_end);
        text += QString("<tr><td>%1 All fine: %3</td><td> %1 <b>%2</b>%3</td></tr>\n").arg(color_start).arg(ToolSet::bool2YesNo(fine)).arg(color_end);

        /*
        text += "<tr><td>Finished: </td><td> <b>" + Bool2YesNo(finished) + "</b></td></tr>\n";
        text += "<tr><td>Stationary: </td><td> <b>" + Bool2YesNo(stationary) + "</b></td></tr>\n";
        text += "<tr><td>Fine: </td><td> <b>" + Bool2YesNo(fine) + "</b></td></tr>\n";*/
    }

    if (type == SupraFit::Statistic::Reduction) {
        qreal value = 0, sum_err = 0, max_err = 0, aver_err = 0, aver = 0, stdev = 0;
        value = result["value"].toDouble();
        QVector<qreal> vector = ToolSet::String2DoubleVec(result["data"].toObject()["raw"].toString());
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
    }
    text += "<tr><td></td></tr></table><p> ... done ...</p>";
    return text;
}

QString TextFromStatistic(const QJsonObject& result, const QJsonObject& controller)
{

    int converged = 0;
    int invalid = 0;

    double min_error = 1e10, max_error = 0;

    int size = controller["size"].toInt();
    for (int i = 0; i < size; ++i) {
        QJsonObject local_model = result[QString::number(i)].toObject();
        min_error = qMin(min_error, local_model["SSE"].toDouble());
        max_error = qMax(max_error, local_model["SSE"].toDouble());

        converged += local_model["converged"].toBool();
        invalid += local_model["valid"].toBool();
    }
    QString text = QString("<html><h3>Global Search Result Overview</h3>");
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
    result = QString("%1 %2M").arg(concentration * 1e6, prec).arg(QChar(956));
    /* else if (concentration < 1e-1)
        result = QString("%1 mM").arg(concentration * 1e3, prec);
    else
        result = QString("%1 M").arg(concentration, prec);
    */
    return result;
}
}
