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

#include <Eigen/Dense>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <fisher/fisher_dist.h>

#include <functional>

typedef Eigen::VectorXd Vector;

namespace ToolSet {

QString DoubleVec2String(const QVector<qreal>& vector, const QString& str = " ");
QString DoubleList2String(const QList<qreal>& vector, const QString& str = " ");
QString DoubleList2String(const Vector& vector, const QString& str = " ");
QString Points2String(const QList<QPointF>& points);
QList<QPointF> String2Points(const QString& str);
QStringList DoubleList2StringList(const Vector& vector);

QVector<qreal> String2DoubleVec(const QString& str);
QList<qreal> String2DoubleList(const QString& str);
QString bool2YesNo(bool var);

qreal ceil(qreal value);
qreal floor(qreal value);
qreal scale(qreal value, qreal& pow);
qreal scale(qreal value);

void Normalise(const QVector<QPair<qreal, qreal>>& hist);
QVector<QPair<qreal, qreal>> List2Histogram(const QVector<qreal>& vector, int bins = 0, qreal min = 0, qreal max = 0);
SupraFit::ConfidenceBar Confidence(const QList<qreal>& list, qreal error);
SupraFit::BoxWhisker BoxWhiskerPlot(const QList<qreal>& list);
QJsonObject Box2Object(const SupraFit::BoxWhisker& box);
SupraFit::BoxWhisker Object2Whisker(const QJsonObject& object);
QList<QJsonObject> Model2Parameter(const QList<QJsonObject>& models, bool sort = true);
void Parameter2Statistic(QList<QJsonObject>& parameter, const QPointer<AbstractModel> model);

QList<QPointF> fromModelsList(const QList<QJsonObject>& models, const QString& str);
qreal SimpsonIntegrate(qreal lower, qreal upper, std::function<qreal(qreal, const QVector<qreal>)> function, const QVector<qreal>& parameter);

qreal finv(qreal p, int m, int n);
QList<int> InvertLockedList(const QList<int>& locked);
void ExportResults(const QString& filename, const QList<QJsonObject>& models);
}

namespace Print {
QString TextFromConfidence(const QJsonObject& result, const QPointer<AbstractModel> model, const QJsonObject& controller);
QString printDouble(double number);
}
