/*
 * <one line to give the library's name and an idea of what it does.>
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

#pragma once

#include <QtCore/QJsonObject>

#include "src/core/models.h"

namespace Statistic {

QString MonteCarlo2Thermo(int index, qreal T, const QJsonObject& object = QJsonObject(), bool heat = false);
QString MonteCarlo2BC50_1(const qreal logK11, const QJsonObject& object);
QString MonteCarlo2BC50_1_2(const qreal logK11, const qreal logK12, const QJsonObject& object);
QString MonteCarlo2BC50_2_1(const qreal logK21, const qreal logK11, const QJsonObject& object);
QString MonteCarlo2BC50_2_2(const qreal logK21, const qreal logK11, const qreal logK12, const QJsonObject& object);

QString GridSearch2Thermo(int index, qreal T, const QJsonObject& object = QJsonObject(), bool heat = false);
QJsonObject PostGridSearch(const QList<QJsonObject>& models, qreal K, qreal T, int index, qreal H = 0);
QString GridSearch2BC50_1(const qreal logK11, const QJsonObject& object);
QString GridSearch2BC50_1_2(const qreal logK11, const qreal logK12, const QJsonObject& object);
QString GridSearch2BC50_2_1(const qreal logK21, const qreal logK11, const QJsonObject& object);
QString GridSearch2BC50_2_2(const qreal logK21, const qreal logK11, const qreal logK12, const QJsonObject& object);
}
