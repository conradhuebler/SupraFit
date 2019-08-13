/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/AbstractModel.h"

#include <Eigen/Dense>

#include <QtCore/QObject>
#include <QtCore/QWeakPointer>

typedef Eigen::VectorXd Vector;

class ClusterAnalyse : public QObject {
    Q_OBJECT
public:
    explicit ClusterAnalyse(QObject* parent = nullptr);
    virtual ~ClusterAnalyse() {}

    void setModels(const QList<QJsonObject>& models) { m_models = models; }
    void setModel(const QWeakPointer<AbstractModel> model) { m_model = model; }
signals:

public slots:

private:
    void GenerateMatrix();
    void PerformClusterAnalyse();
    QPair<int, int> MinDistance();

    QList<QJsonObject> m_models;

    QWeakPointer<AbstractModel> m_model;
    QVector<Vector> m_vector_block;
    QVector<QPair<int, int>> m_connections;
    QVector<int> m_blacklist;
};
