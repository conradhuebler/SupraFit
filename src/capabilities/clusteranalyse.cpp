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

#include <Eigen/Dense>

#include "src/core/AbstractModel.h"
#include "src/core/toolset.h"

#include "clusteranalyse.h"

ClusterAnalyse::ClusterAnalyse(QObject* parent)
    : QObject(parent)
{
}

void ClusterAnalyse::GenerateMatrix()
{
    QSharedPointer<AbstractModel> model = m_model.data()->Clone();

    for (const QJsonObject& m : m_models) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        model->ImportModel(m);
        Vector vector = ToolSet::QVector2Vector(QVector<qreal>() << model->SumofSquares() << model->SEy() << model->AllParameter());
        m_vector_block << vector;
    }
    model.clear();
}

QPair<int, int> ClusterAnalyse::MinDistance()
{
    double distance = 1e10;
    QPair<int, int> indicies;

    for (int i = 0; i < m_vector_block.size(); ++i) {
        if (m_blacklist.contains(i))
            continue;
        for (int j = i; j < m_vector_block.size(); ++j) {
            double d = (m_vector_block[i] * m_vector_block[j]).norm();
            if (d < distance) {
                distance = d;
                indicies = QPair<int, int>(i, j);
            }
        }
    }
    return indicies;
}

void ClusterAnalyse::PerformClusterAnalyse()
{
    while (m_blacklist.size() < m_vector_block.size()) {
        QPair<int, int> pair = MinDistance();
        m_blacklist << pair.second;
        m_connections << pair;
        Vector mean = ToolSet::MeanVector(m_vector_block[pair.first], m_vector_block[pair.second]);
        m_vector_block[pair.first] = mean;
        m_vector_block[pair.second] = mean;
    }
    qDebug() << m_connections;
}
