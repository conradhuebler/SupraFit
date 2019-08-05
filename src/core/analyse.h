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

#include <QtCore/QJsonObject>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>

#include "src/core/AbstractModel.h"

namespace StatisticTool {

QString AnalyseReductionAnalysis(const QVector<QPair<QJsonObject, QVector<int>>> models, double cutoff = 0);
QString CompareAIC(const QVector<QWeakPointer<AbstractModel>> models);
QString CompareCV(const QVector<QJsonObject> models, int cvtype = 1, bool local = true, int cv_x = 3, int bins = 30);
QString CompareMC(const QVector<QJsonObject> models, bool local = true, int bins = 30);
}
