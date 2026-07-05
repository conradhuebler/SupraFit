/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

double String2Double(QString str)
{
    double result = 0;
    bool ok;
    str.replace(",", ".");
    result = str.toDouble(&ok);
    return result;
}

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

Vector String2DoubleEigVec(const QString& str)
{
    Vector vector;
    std::vector<double> v;
    if (str.isEmpty())
        return vector;
    QStringList nums = str.split(" ");
    for (const QString& string : qAsConst(nums))
        v.push_back(string.toDouble());

    vector = Vector::Map(&v[0], v.size());
    return vector;
}

Vector QVector2DoubleEigVec(const QVector<qreal>& in_vector)
{
    Vector vector;
    std::vector<double> v;

    for (double value : in_vector)
        v.push_back(value);

    // Claude Generated - Fix crash when input vector is empty
    if (v.empty()) {
        vector = Vector(0);  // Create empty Vector instead of crashing
    } else {
        vector = Vector::Map(&v[0], v.size());
    }
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

QList<int> String2IntList(const QString& str)
{
    QList<int> vector;
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

QList<QPointF> String2PointsList(const QString& str)
{
    QList<QPointF> points;
    QStringList list = str.split(" ");
    for (const QString& point : qAsConst(list)) {
        double x = 0, y = 0;
        QStringList l = point.split(";");
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
        string += "(" + QString::number(points[i].x()) + ";" + QString::number(points[i].y()) + ") ";
    return string;
}

QVector<QPointF> String2PointsVector(const QString& str)
{
    QVector<QPointF> points;
    QStringList list = str.split(" ");
    for (const QString& point : qAsConst(list)) {
        double x = 0, y = 0;
        QStringList l = point.split(";");
        if (l.size() != 2)
            continue;
        x = l[0].remove("(").toDouble();
        y = l[1].remove(")").toDouble();
        points << QPointF(x, y);
    }
    return points;
}

QPair<qreal, qreal> QString2QPair(const QString& str)
{
    QPair<qreal, qreal> pair(0, 0);
    QStringList elements = str.simplified().split(" ");
    if (elements.size() == 2) {
        pair.first = elements[0].replace("[", "").toDouble();
        pair.second = elements[1].replace("]", "").toDouble();
    } else if (elements.size() == 4) {
        pair.first = elements[1].replace("[", "").toDouble();
        pair.second = elements[2].replace("]", "").toDouble();
    }
    return pair;
}
QString QPair2QString(const QPair<qreal, qreal>& pair)
{
    return QString("[%1 %2]").arg(pair.first).arg(pair.second);
}

QString bool2YesNo(bool var)
{
    return (var ? QString("Yes") : QString("No"));
}

int NiceRows(int elements, int prefered_rows)
{
    int rows = 1;
    for (int i = prefered_rows; i < prefered_rows + 1; i--) {
        if (i <= 0)
            continue;
        if (elements % i == 0)
            return i;
    }
    return rows;
}

qreal scale(qreal value)
{
    qreal pot;
    return scale(value, pot);
}

qreal scale(qreal value, qreal& pow)
{
    int iter = 0;
    if (qAbs(value) < 1 && value) {
        while (qAbs(value) < 1 && iter < 10) {
            pow /= 10;
            value *= 10;
            iter++;
        }
    } else if (qAbs(value) > 10) {
        while (qAbs(value) > 10 && iter < 10) {
            pow *= 10;
            value /= 10;
            iter++;
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

QList<int> InvertLockedList(const QList<int>& locked)
{
    QList<int> lock;
    for (int i = 0; i < locked.size(); ++i)
        lock << !locked[i];
    return lock;
}

qreal K2G(qreal logK, qreal T)
{
    return -R * T * logK * log2ln;
}

qreal GHE(qreal G, qreal H, qreal T)
{
    return -(G - H) / T;
}
QString UnicodeLowerInteger(const QString& str)
{
    QString tmp = str;
    tmp = tmp.replace("1", Unicode_Sub_1);
    tmp = tmp.replace("2", Unicode_Sub_2);
    tmp = tmp.replace("3", Unicode_Sub_3);
    tmp = tmp.replace("4", Unicode_Sub_4);
    tmp = tmp.replace("5", Unicode_Sub_5);
    tmp = tmp.replace("6", Unicode_Sub_6);
    tmp = tmp.replace("7", Unicode_Sub_7);
    tmp = tmp.replace("8", Unicode_Sub_8);
    tmp = tmp.replace("9", Unicode_Sub_9);
    tmp = tmp.replace("0", Unicode_Sub_0);
    return tmp;
}
}
