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

#include "src/core/AbstractModel.h"
#include "src/core/minimizer.h"

#include <QApplication>

#include <QtCore/QThreadPool>
#include <QtCore/QObject>
#include <QtCore/QWeakPointer>

#include <iostream>

#include "statistic.h"

StatisticThread::StatisticThread(RunType runtype) : m_runtype(runtype), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater))
{
    setAutoDelete(false);
}

StatisticThread::~StatisticThread()
{
}

void StatisticThread::setModel(QSharedPointer<AbstractTitrationModel> model)
{ 
    m_model = model->Clone(); 
    m_minimizer->setModel(m_model); 
}

void StatisticThread::run()
{
    
    if(m_runtype == RunType::ConfidenceByError)
        ConfidenceAssesment();
}

void StatisticThread::setParameter(const QJsonObject& json)
{
    m_model->ImportJSON(json);
}

void StatisticThread::ConfidenceAssesment()
{
    m_model.data()->CalculateSignal();
    qreal error = m_model.data()->ModelError();
    QList<QPointF> series;
    QJsonObject optimized = m_model->ExportJSON();
    QVector<double > parameter = m_model.data()->OptimizeParameters(m_type);
    QList<int> locked; 
    for(int i = 0; i < parameter.size(); ++i)
        locked << 1;
    locked[m_parameter_id] = 0;
    m_model->setLockedParameter(locked);
    
    m_result.optim = parameter[m_parameter_id];
    m_result.error = error;
    m_result.name = m_model->ConstantNames()[m_parameter_id];
    
    m_model->setLockedParameter(locked);
    QVector<double> vars = parameter;
    double increment = vars[m_parameter_id]/1000;
    
    for(int m = 0; m < 1000; ++m)
    {
        double x  = m_model->IncrementParameter(increment, m_parameter_id);
        QJsonObject json = m_model->ExportJSON();
        m_minimizer->setParameter(json);
        m_minimizer->Minimize(m_type);
        {
            QJsonObject json = m_minimizer->Parameter();
            m_model->ImportJSON(json);
            m_model->CalculateSignal();
        }
        qreal new_error = m_model->ModelError();
        
        if(new_error/error > double(1.05))
        {
            m_result.max = x;
            break;
        }
        series.append(QPointF(x,new_error));
    }
    increment *= -1;
    m_model->ImportJSON(optimized);
    for(int m = 0; m < 1000; ++m)
    {
        double x  = m_model->IncrementParameter(increment, m_parameter_id);
        QJsonObject json = m_model->ExportJSON();
        m_minimizer->setParameter(json);
        m_minimizer->Minimize(m_type);
        {
            QJsonObject json = m_minimizer->Parameter();
            m_model->ImportJSON(json);
            m_model->CalculateSignal();
        }
        qreal new_error = m_model->ModelError();
        
        
        if(new_error/error > double(1.05))
        {
            m_result.min = x;
            break;
        }
        series.prepend(QPointF(x,new_error));
    }
    m_result.points = series;
    
}

Statistic::Statistic(QObject *parent) : QObject(parent), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater))
{
    
    
    
}

Statistic::~Statistic()
{
    
    
    
}

void Statistic::ConfidenceAssesment()
{
    if(!m_model)
        return;
    for(int i = 0; i < m_series.size(); ++i)
        m_series[i].clear();
    m_series.clear();
    
    m_minimizer->setModel(m_model);
    QJsonObject optimized = m_model->ExportJSON();
    QList<double > parameter = m_model.data()->OptimizeParameters(OptimizationType::ComplexationConstants | OptimizationType::IgnoreAllShifts).toList();
    
    m_model.data()->CalculateSignal();
    QThreadPool *threadpool = QThreadPool::globalInstance();
    QList<QPointer <StatisticThread > > threads;
    int maxthreads =qApp->instance()->property("threads").toInt();
    threadpool->setMaxThreadCount(maxthreads);
    for(int i = 0; i < parameter.size(); ++i)
    {
        QPointer<StatisticThread >thread = new StatisticThread(StatisticThread::RunType::ConfidenceByError);
        thread->setModel(m_model);
        thread->SetParameterID(i);
         thread->setOptimizationRun(OptimizationType::ComplexationConstants| OptimizationType::IgnoreAllShifts);
//        thread->setOptimizationRun(m_type);
        threadpool->start(thread);
//         thread->run();
        threads << thread;
    }
    
    threadpool->waitForDone();
    for(int i = 0; i < threads.size(); ++i)
    {
        m_result << threads[i]->getResult();
        delete threads[i];
    }
    qDeleteAll(threads);
}


void Statistic::setParameter(const QJsonObject& json)
{
    m_model->ImportJSON(json);
}

#include "statistic.moc"
