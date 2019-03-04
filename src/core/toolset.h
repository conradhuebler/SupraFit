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

#pragma once

#include "src/core/models.h"

#include <fisher/fisher_dist.h>
#include <libpeakpick/libpeakpick/peakpick.h>

#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <Eigen/Dense>

#include <functional>

typedef Eigen::VectorXd Vector;

namespace ToolSet {

QString DoubleVec2String(const QVector<qreal>& vector, const QString& str = " ");
QString IntVec2String(const QVector<int>& vector, const QString& str = " ");
QString IntList2String(const QList<int>& vector, const QString& str = " ");

QString DoubleList2String(const QList<qreal>& vector, const QString& str = " ");
QString DoubleList2String(const Vector& vector, const QString& str = " ");
QString Points2String(const QList<QPointF>& points);
QList<QPointF> String2Points(const QString& str);
QStringList DoubleList2StringList(const Vector& vector);

QString Int2DVec2String(const QVector<QVector<int>>& vector);

QVector<qreal> String2DoubleVec(const QString& str);
QVector<int> String2IntVec(const QString& str);

QVector<QVector<int>> String2Int2DVec(const QString& str);

QVector<int> VecAndVec(const QVector<int>& a, const QVector<int>& b);
QVector<int> VecAndVec(const Vector& a, const QVector<int>& b);

QList<qreal> String2DoubleList(const QString& str);
QString bool2YesNo(bool var);

qreal ceil(qreal value);
qreal floor(qreal value);
qreal scale(qreal value, qreal& pow);
qreal scale(qreal value);

QPair<qreal, qreal> MinMax(const QList<qreal>& vector);

void Normalise(QVector<QPair<qreal, qreal>>& hist);
QVector<QPair<qreal, qreal>> List2Histogram(const QVector<qreal>& vector, int bins = 0, qreal min = 0, qreal max = 0);
/*! \brief Calculate the Entropy of a histogram, bin with is already defined */
QPair<qreal, qreal> Entropy(const QVector<QPair<qreal, qreal>>& histogram);

SupraFit::ConfidenceBar Confidence(const QList<qreal>& list, qreal error);
SupraFit::BoxWhisker BoxWhiskerPlot(const QList<qreal>& list);
QJsonObject Box2Object(const SupraFit::BoxWhisker& box);
SupraFit::BoxWhisker Object2Whisker(const QJsonObject& object);
QList<QJsonObject> Model2Parameter(const QList<QJsonObject>& models, bool sort = true);
void Parameter2Statistic(QList<QJsonObject>& parameter, const QPointer<AbstractModel> model);

QList<QPointF> fromModelsList(const QList<QJsonObject>& models, const QString& str);

qreal finv(qreal p, int m, int n);
QList<int> InvertLockedList(const QList<int>& locked);
void ExportResults(const QString& filename, const QList<QJsonObject>& models);
QPair<Vector, Vector> LoadXYFile(const QString& filename);
QPair<PeakPick::spectrum, QJsonObject> LoadITCFile(QString& filename, std::vector<PeakPick::Peak>* peaks, qreal& offset, qreal& freq, QVector<qreal>& inject);

/*! \brief Convert log K (complexation constant) to free Enthalpy for given T */
qreal K2G(qreal logK, qreal T);

/*! \brief Use Gibbs-Helmholtz equation to calculate Entropy */
qreal GHE(qreal G, qreal H, qreal T);
}

namespace Print {
QString Html2Tex(const QString& str);
QString TextFromConfidence(const QJsonObject& result, const QJsonObject& controller);
QString TextFromStatistic(const QJsonObject& result, const QJsonObject& controller);

QString printDouble(double number, int prec = -1);
QString printConcentration(double concentration, int prec = 3);
}
