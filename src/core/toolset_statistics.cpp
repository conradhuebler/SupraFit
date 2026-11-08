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
#include <QtCore/QHashFunctions>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QJsonObject>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <cmath>
#include <limits>
#include <unordered_map>

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

/*! \brief Continued fraction of the regularised incomplete beta function (modified Lentz, DLMF 8.17.22).
 *
 * I_x(a, b) = x^a (1-x)^b / (a B(a, b)) * cf; converges quickly for x < (a + 1)/(a + b + 2). Claude Generated. */
static long double IncompleteBetaFraction(long double x, long double a, long double b)
{
    const long double tiny = 1e-4000L;
    const long double eps = 4 * std::numeric_limits<long double>::epsilon(); // a few ulp: the product d c settles there
    long double c = 1;
    long double d = 1 - (a + b) * x / (a + 1);
    if (fabsl(d) < tiny)
        d = tiny;
    d = 1 / d;
    long double h = d;
    for (int k = 1; k < 100000; ++k) {
        const long double m2 = 2 * k;
        long double aa = k * (b - k) * x / ((a + m2 - 1) * (a + m2)); // even step
        d = 1 + aa * d;
        if (fabsl(d) < tiny)
            d = tiny;
        c = 1 + aa / c;
        if (fabsl(c) < tiny)
            c = tiny;
        d = 1 / d;
        h *= d * c;
        aa = -(a + k) * (a + b + k) * x / ((a + m2) * (a + m2 + 1)); // odd step
        d = 1 + aa * d;
        if (fabsl(d) < tiny)
            d = tiny;
        c = 1 + aa / c;
        if (fabsl(c) < tiny)
            c = tiny;
        d = 1 / d;
        const long double delta = d * c;
        h *= delta;
        if (fabsl(delta - 1) < eps)
            break;
    }
    return h;
}

/*! \brief Both tails of the regularised incomplete beta function, I_x(a, b) and 1 - I_x(a, b), each without cancellation where it
 * is the small one (the fraction is evaluated on the side where it converges). Long double: the prefactor x^a (1-x)^b / B(a, b) is
 * the difference of log-gamma values of size ~(a + b) log(a + b), which would cost ~1e-13 in double for a + b ~ 1000.
 * Claude Generated. */
void IncompleteBetaTails(long double x, long double a, long double b, long double& lower, long double& upper)
{
    if (x <= 0) {
        lower = 0;
        upper = 1;
        return;
    }
    if (x >= 1) {
        lower = 1;
        upper = 0;
        return;
    }
    const long double log_prefactor = lgammal(a + b) - lgammal(a) - lgammal(b) + a * logl(x) + b * log1pl(-x);
    const long double prefactor = expl(log_prefactor);
    if (x < (a + 1) / (a + b + 2)) {
        lower = prefactor * IncompleteBetaFraction(x, a, b) / a;
        upper = 1 - lower;
    } else {
        upper = prefactor * IncompleteBetaFraction(1 - x, b, a) / b;
        lower = 1 - upper;
    }
}

/*! \brief Solves I_t(a, b) = target for t (target <= 0.5, so the tail needed is the small one and is computed without cancellation).
 *
 * Halley iteration with the analytic derivative (the beta density) and its logarithmic derivative, started from the
 * Abramowitz-Stegun 26.5.22 guess (as in Numerical Recipes' invbetai), kept inside a bracket that every residual shrinks; a step
 * that leaves the bracket is replaced by bisection. Stops when the step is below 1e-19 relative to t and to 1 - t (the quantile
 * needs 1 - t when t is close to 1, as for m = 1 and large n). Claude Generated. */
static long double InverseIncompleteBeta(long double target, long double a, long double b)
{
    // Initial guess (AS 26.5.22 for a, b >= 1; power-law tails otherwise)
    long double t;
    if (a >= 1 && b >= 1) {
        const long double s = sqrtl(-2 * logl(target));
        const long double z = s - (2.30753L + s * 0.27061L) / (1 + s * (0.99229L + s * 0.04481L)); // normal quantile of target
        const long double al = (z * z - 3) / 6;
        const long double h = 2 / (1 / (2 * a - 1) + 1 / (2 * b - 1));
        const long double w = z * sqrtl(al + h) / h - (1 / (2 * b - 1) - 1 / (2 * a - 1)) * (al + 5.0L / 6 - 2 / (3 * h));
        t = a / (a + b * expl(2 * w));
    } else {
        const long double lna = logl(a / (a + b)), lnb = logl(b / (a + b));
        const long double ta = expl(a * lna) / a, tb = expl(b * lnb) / b, w = ta + tb;
        if (target < ta / w)
            t = powl(a * w * target, 1 / a);
        else
            t = 1 - powl(b * w * (1 - target), 1 / b);
    }
    const long double log_beta = lgammal(a) + lgammal(b) - lgammal(a + b);
    long double lo = 0, hi = 1;
    if (!(t > 0 && t < 1))
        t = 0.5;
    for (int iter = 0; iter < 200; ++iter) {
        long double lower, upper;
        IncompleteBetaTails(t, a, b, lower, upper);
        const long double residual = lower - target;
        if (residual == 0)
            return t;
        if (residual < 0)
            lo = t;
        else
            hi = t;
        const long double density = expl((a - 1) * logl(t) + (b - 1) * log1pl(-t) - log_beta);
        long double next;
        if (density > 0 && std::isfinite(double(density))) {
            const long double newton = residual / density;
            const long double curvature = (a - 1) / t - (b - 1) / (1 - t); // (d density/dt) / density
            const long double denominator = 1 - 0.5L * newton * curvature;
            const long double step = (denominator > 0.5L && denominator < 2) ? newton / denominator : newton;
            next = t - step;
        } else
            next = -1;
        if (!(next > lo && next < hi)) // outside the bracket: bisect (geometrically when the bracket spans decades)
            next = (lo > 0 && hi / lo > 4) ? sqrtl(lo * hi) : 0.5L * (lo + hi);
        if (fabsl(next - t) <= 1e-19L * fminl(t, 1 - t)) // long double resolution, also of 1 - t
            return next;
        t = next;
        if (hi - lo <= 1e-19L * fminl(t, 1 - t))
            return t;
    }
    return t;
}

/*! \brief Quantile of the F distribution with \p m and \p n degrees of freedom at \p p, without the memo of finv.
 *
 * P(F <= x) = I_y(m/2, n/2) with y = m x / (m x + n). For p <= 0.5 the lower tail is solved for y, for p > 0.5 the upper tail
 * I_z(n/2, m/2) = 1 - p for z = 1 - y = n / (m x + n) (1 - p is exact in double for p >= 0.5), so the small variable is the one
 * solved for and the quantile keeps its relative accuracy far into the tail. Accuracy over p in {0.5 .. 0.999}, m 1..40, n 1..2000
 * against an independent series reference: see test_finv. Returns 0 for p = 0, infinity for p = 1, NaN outside the domain.
 * Claude Generated (replaces the Simpson stepping of Fisher_Dist::finv, which was accurate to ~1e-4 and capped m, n at 250). */
qreal FisherQuantile(qreal p, qreal m, qreal n)
{
    if (!(p >= 0 && p <= 1) || !(m > 0) || !(n > 0))
        return std::numeric_limits<qreal>::quiet_NaN();
    if (p == 0)
        return 0;
    if (p == 1)
        return std::numeric_limits<qreal>::infinity();
    const long double a = 0.5L * m, b = 0.5L * n;
    if (p <= 0.5) {
        const long double y = InverseIncompleteBeta(p, a, b);
        return qreal(n * y / (m * (1 - y)));
    }
    const long double z = InverseIncompleteBeta(1.0L - p, b, a);
    return qreal(n * (1 - z) / (m * z));
}

/*! \brief Quantile of the F distribution with \p m and \p n degrees of freedom at \p p (the f value of an F test).
 *
 * Computed by FisherQuantile (some microseconds). The value depends only on (p, m, n), and the models of a project share them
 * (the same confidence, mostly the same number of parameters and points): every value computed is kept. Thread-safe (statistics
 * jobs call it from worker threads). Out of the domain (p outside [0, 1], m or n <= 0) it returns 1, as the Fisher_Dist version
 * did. Claude Generated (memo, FisherQuantile). */
qreal finv(qreal p, int m, int n)
{
    struct Key {
        qreal p;
        int m, n;
        bool operator==(const Key& other) const { return p == other.p && m == other.m && n == other.n; }
    };
    struct KeyHash {
        size_t operator()(const Key& key, size_t seed = 0) const { return qHashMulti(seed, key.p, key.m, key.n); }
    };
    static QMutex mutex;
    static std::unordered_map<Key, qreal, KeyHash> known;
    {
        QMutexLocker locker(&mutex);
        const auto it = known.find(Key{ p, m, n });
        if (it != known.end())
            return it->second;
    }
    qreal f_value = FisherQuantile(p, m, n);
    if (std::isnan(f_value))
        f_value = 1;
    QMutexLocker locker(&mutex);
    known.emplace(Key{ p, m, n }, f_value);
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
