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

struct StatisticResult
{
    double optim;
    double max;
    double min;
    double error;
    QList<QPointF > points;  
};


class StatisticThread : public QObject, public QRunnable
{
  Q_OBJECT
public:
    enum RunType{
      ConfidenceByError = 1,
      ConfidenceByFTest = 2,
      ConfidenceByCarlo = 3
    };
    StatisticThread(RunType runtype);
    ~StatisticThread();
    void setModel(QSharedPointer<AbstractTitrationModel> model); 
    inline void SetParameterID( int id ) { m_parameter_id = id; }
    inline void setOptimizationRun(OptimizationType runtype) { m_type = runtype; }
    void setParameter(const QJsonObject &json);
    virtual void run();
    RunType m_runtype;
    StatisticResult getResult() const { return m_result; }
    
private:
    QSharedPointer<AbstractTitrationModel> m_model;
    QSharedPointer<Minimizer> m_minimizer;
    OptimizationType m_type;
    void ConfidenceAssesment();
    int m_parameter_id;
    StatisticResult m_result;
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
    QList<QList<QPointF> >Series() const { return m_series; }
    QList<StatisticResult > Results() const { return m_result; }
private:
    QSharedPointer<AbstractTitrationModel> m_model;
    QSharedPointer<Minimizer> m_minimizer;
    OptimizationType m_type;
    QList<QList<QPointF> > m_series;
    QList<StatisticResult > m_result;
};

#endif // STATISTIC_H
