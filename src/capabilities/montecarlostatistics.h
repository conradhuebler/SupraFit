/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#ifndef MONTECARLOSTATISTICS_H
#define MONTECARLOSTATISTICS_H
#include "src/core/models.h"

#include <QtCore/QJsonObject>
#include <QtCore/QObject>

class Minimizer;


class MonteCarloThread : public QObject, public QRunnable
{
 Q_OBJECT
 
public:
    MonteCarloThread(QObject *parent = 0);
    ~MonteCarloThread();
    inline void setModel(const QSharedPointer<AbstractTitrationModel> model) { m_model = model->Clone(); }
    virtual void run();
    inline void setOptimizationRun(OptimizationType runtype) { m_runtype = runtype; }
    inline QJsonObject OptimizedParameter() const { return m_optimized; }
    inline QList<qreal > Constants() const { return m_constants; }
    void setDataTable(DataTable *table);
private:
    QSharedPointer<Minimizer> m_minimizer;
    QSharedPointer<AbstractTitrationModel> m_model;
    QJsonObject m_optimized;
    OptimizationType m_runtype;
    QList<qreal > m_constants;
};

class MonteCarloStatistics : public QObject
{
    Q_OBJECT
public:
    MonteCarloStatistics(QObject *parent = 0);
    ~MonteCarloStatistics();
    
    inline void setModel(const QSharedPointer<AbstractTitrationModel> model) { m_model = model->Clone(); }
    inline void setMaxSteps(int steps) { m_maxsteps = steps; }
    void Evaluate();
    inline QVector<StatisticResult >getResult() const { return m_result; }
    inline QList<QList<QPointF> > getSeries() const { return m_series; }
    inline void setOptimizationRun(OptimizationType runtype) { m_runtype = runtype; }
private:
    OptimizationType m_runtype;
    QSharedPointer<AbstractTitrationModel> m_model;
    int m_maxsteps;
    QList<QList<QPointF> > m_series;
    QVector<StatisticResult > m_result;
    OptimizerConfig m_config;
    std::mt19937 rng;
    std::normal_distribution<double> Phi;
    DataTable *m_table;
};

#endif // MONTECARLOSTATISTICS_H
