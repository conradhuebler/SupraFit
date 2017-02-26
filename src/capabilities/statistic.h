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
#include <QtCore/QRunnable>
#include <QtCore/QObject>
#include <QtCore/QWeakPointer>
class Minimizer;
class QPointF;

struct StatisticResult;

class StatisticThread : public QObject, public QRunnable
{
  Q_OBJECT
public:

    StatisticThread();
    ~StatisticThread();
    void setModel(QSharedPointer<AbstractTitrationModel> model); 
    inline void SetParameterID( int id ) { m_parameter_id = id; }
    inline void setOptimizationRun(OptimizationType runtype) { m_type = runtype; }
    void setParameter(const QJsonObject &json);
    virtual void run();
    inline StatisticResult getResult() const { return m_result; }
    inline void setIncerement(double increment) { m_increment = increment; }
    inline void setMaxSteps(int steps ) { m_maxsteps = steps; }
    inline bool Converged() const { return m_converged; }
    inline QList<QPointF> getSeries() const { return m_series; }
    inline void setOptimizationConfig(OptimizerConfig config) { m_config = config; }
    
private:
    void SumErrors(bool direction, double &integ_5, double &integ_1, QList<QPointF> &series);
    QSharedPointer<AbstractTitrationModel> m_model;
    QSharedPointer<Minimizer> m_minimizer;
    OptimizationType m_type;
    void ConfidenceAssesment();
    int m_parameter_id;
    StatisticResult m_result;
    qreal m_increment, m_error;
    int m_maxsteps;
    bool m_converged;
    QList<QPointF> m_series;
    OptimizerConfig m_config;
};

class Statistic : public QObject
{
    Q_OBJECT
public:
    Statistic(QObject *parent = 0);
    ~Statistic();
    void setModel(QSharedPointer<AbstractTitrationModel> model) { m_model = model->Clone(); }
    inline void setOptimizationRun(OptimizationType runtype) { m_type = runtype; }
    bool ConfidenceAssesment();
    void setParameter(const QJsonObject &json);
    QList<QList<QPointF> >Series() const { return m_series; }
    QList<StatisticResult > Results() const { return m_result; }
    inline void setOptimizationConfig(OptimizerConfig config) { m_config = config; }
    
private:
    QSharedPointer<AbstractTitrationModel> m_model;
    QSharedPointer<Minimizer> m_minimizer;
    OptimizationType m_type;
    QList<QList<QPointF> > m_series;
    QList<StatisticResult > m_result;
    OptimizerConfig m_config;
};

#endif // STATISTIC_H
