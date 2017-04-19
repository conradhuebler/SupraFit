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

#ifndef ABSTRACTSEARCHCLASS_H
#define ABSTRACTSEARCHCLASS_H

#include "src/core/AbstractModel.h"

#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QPointF>
#include <QtCore/QSharedPointer>
#include <QtCore/QThreadPool>

class AbstractTitrationModel;

class AbstractConfig
{
    
public:
    inline AbstractConfig(OptimizerConfig config = OptimizerConfig(), OptimizationType type = static_cast<OptimizationType>(OptimizationType::ComplexationConstants | OptimizationType::OptimizeShifts | OptimizationType::UnconstrainedShifts)) : optimizer_config(config), runtype(type){ }
    inline ~AbstractConfig() {}
    OptimizerConfig optimizer_config;
    OptimizationType runtype;
};

class AbstractSearchThread : public QObject, public QRunnable
{
  Q_OBJECT
  
public:
    inline AbstractSearchThread(QObject *parent = 0) : QObject(parent) { setAutoDelete(false); }
    inline ~AbstractSearchThread() { }
    inline void setModel(const QSharedPointer<AbstractTitrationModel> model) { m_model = model->Clone(); }
    
    
protected:
    QSharedPointer<AbstractTitrationModel> m_model;
    
signals:
    void IncrementProgress(int msecs);
};

class AbstractSearchClass : public QObject
{
    Q_OBJECT
    
public:
    AbstractSearchClass(QObject *parent = 0);
    ~AbstractSearchClass();
    inline void setModel(const QSharedPointer<AbstractTitrationModel> model) { m_model = model->Clone(); }
    
    inline QList<QList<QPointF> > Series() const { return m_series; }
    inline QList<QJsonObject > Models() const { return m_models; }
    inline QList<QJsonObject > Results() const { return m_results; }
    
    void ExportResults(const QString& filename);
    
public slots:
    void Interrupt();
    
private:
    
protected:
    QSharedPointer<AbstractTitrationModel> m_model;
    QThreadPool *m_threadpool;
    QList<QList<QPointF> > m_series;
    QList<QJsonObject > m_models;
    QList<QJsonObject > m_results;
    
signals:
    void IncrementProgress(int msecs);
    void AnalyseFinished();
    void setMaximumSteps(int maxsteps);
};

#endif // ABSTRACTSEARCHCLASS_H
