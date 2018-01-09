/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifndef TOOLSET_H
#define TOOLSET_H

#include "src/core/AbstractModel.h"

#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <Eigen/Dense>

#include <fisher/fisher_dist.h>

#include <functional>

typedef Eigen::VectorXd Vector;

namespace ToolSet{
    
    QString DoubleVec2String(const QVector<qreal > &vector, const QString &str = " ");
    QString DoubleList2String(const QList<qreal > &vector, const QString &str = " ");
    QString DoubleList2String(const Vector &vector, const QString &str = " ");
    QStringList DoubleList2StringList(const Vector &vector);
    
    QVector<qreal > String2DoubleVec(const QString &str);
    QList<qreal > String2DoubleList(const QString &str);
    QString bool2YesNo(bool var) ;
    
    qreal ceil(qreal value);
    qreal floor(qreal value);
    qreal scale(qreal value, qreal &pow);
    qreal scale(qreal value);
    
    QVector<QPair<qreal, int > > List2Histogram(const QVector<qreal> &vector, int bins = 0, qreal min = 0, qreal max = 0);
    ConfidenceBar Confidence(const QList<qreal > &list, qreal error);
    BoxWhisker BoxWhiskerPlot(const QList<qreal> &list);
    QJsonObject Box2Object(const BoxWhisker &box);
    BoxWhisker Object2Whisker(const QJsonObject& object);
//     ConfidenceBar Confidence(QList<qreal > &list);
    
    QList<QPointF> fromModelsList(const QList<QJsonObject> &models, const QString &str);
    qreal SimpsonIntegrate(qreal lower, qreal upper, std::function<qreal(qreal, const QVector<qreal >)> function, const QVector<qreal > &parameter);
    
    qreal finv(qreal p, int m, int n);
    QList<int> InvertLockedList(const QList<int> &locked);
}

namespace Print{
    QString TextFromConfidence(const QJsonObject &result, const QPointer<AbstractModel> model);
}
#endif // TOOLSET_H
