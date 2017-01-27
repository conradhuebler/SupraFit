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

#ifndef STATISTIC_H
#define STATISTIC_H

#include "src/global.h"
#include "src/core/AbstractModel.h"

#include <QtCore/QWeakPointer>
class Minimizer;

struct StatisticResult
{
    double optim;
    double max;
    double min;
    double error;
    QList<QPointF > points;  
};

class Statistic : public QObject
{
    Q_OBJECT
public:
    Statistic(QObject *parent = 0);
    ~Statistic();
    void setModel(QSharedPointer<AbstractTitrationModel> model) { m_model = model->Clone(); }
    inline void setOptimizationRun(OptimizationType runtype) { m_type = runtype; }
    void ConfidenceAssesment();
    void setParameter(const QJsonObject &json);
private:
    QSharedPointer<AbstractTitrationModel> m_model;
    QSharedPointer<Minimizer> m_minimizer;
    OptimizationType m_type;
};

#endif // STATISTIC_H
