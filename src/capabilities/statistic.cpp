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

#include <QCoreApplication>

#include <QtCore/QThreadPool>
#include <QtCore/QObject>
#include <QtCore/QWeakPointer>

#include <iostream>

#include "statistic.h"

StatisticThread::StatisticThread(RunType runtype) : m_runtype(runtype), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater)), m_increment(1e-3), m_maxsteps(1e4), m_converged(true)
{
    setAutoDelete(false);
}

StatisticThread::~StatisticThread()
{
}

void StatisticThread::setModel(QSharedPointer<AbstractTitrationModel> model)
{ 
    m_model = model->Clone(); 
    m_minimizer->setModel(m_model);//Cloned(m_model); 
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
    if(m_config.error_potenz == 2)
        m_error = m_model.data()->SumofSquares();
    else
        m_error = m_model.data()->SumofAbsolute();
    QList<QPointF> series;
    QJsonObject optimized = m_model->ExportJSON();
    QVector<double > parameter = m_model.data()->OptimizeParameters(m_type);
    
    m_result.optim = parameter[m_parameter_id];
    m_result.name = m_model->ConstantNames()[m_parameter_id];
    
    double integ_5 = 0;
    double integ_1 = 0;
    SumErrors(1, integ_5, integ_1, series);
    m_model->ImportJSON(optimized);
    SumErrors(0, integ_5, integ_1, series);
    
    
    m_series = series;
    m_result.integ_5 = integ_5/m_error;
    m_result.integ_1 = integ_1/m_error;
}

void StatisticThread::SumErrors(bool direction, double& integ_5, double& integ_1, QList<QPointF> &series)
{
    double increment = m_increment;
    if(!direction)
        increment *= -1;
    qreal old_error = m_error;
    int counter = 0;
    QList<int> locked; 
    for(int i = 0; i <  m_model.data()->OptimizeParameters(m_type).size(); ++i)
        locked << 1;
    locked[m_parameter_id] = 0;
    QList<qreal > consts = m_model->Constants();
    double constant_ = consts[m_parameter_id];
    for(int m = 0; m < m_maxsteps; ++m)
    {
        double par = constant_ + double(m)*increment;
       
        consts[m_parameter_id] = par;
        m_model->setConstants(consts);
        m_model->setOptimizerConfig(m_config);
        m_minimizer->Minimize(m_type, locked);
        
        QJsonObject json_exp = m_minimizer->Parameter();
        m_model->ImportJSON(json_exp);
        m_model->CalculateSignal();
        
        qreal new_error;
        
        if(m_config.error_potenz == 2)
            new_error = m_model.data()->SumofSquares();
        else
            new_error = m_model.data()->SumofAbsolute();
        
        if(new_error < m_error)
            counter++;
        qreal rect = qMin(new_error,old_error)*qAbs(increment);
        qreal triangle = 0.5*increment*qAbs(new_error-old_error);
        qreal integ = rect+triangle; 
        integ_5 += integ;
        
        if(new_error/m_error <= double(1.005) && new_error > m_error)
            integ_1 += integ;
        
        if(new_error/m_error > double(1.025))
        {
            if(direction)
                m_result.max = par;
            else
                m_result.min = par;
            break;
        }
        if(direction)
            series.append(QPointF(par,new_error));
        else
            series.prepend(QPointF(par,new_error));
        old_error = new_error;
        if(counter > 50)
        {
            m_converged = false;
            qDebug() << "not converged " << direction;
            break;
        }
    }
}


Statistic::Statistic(QObject *parent) : QObject(parent), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater))
{
    
    
    
}

Statistic::~Statistic()
{
    
    
    
}

bool Statistic::ConfidenceAssesment()
{
    if(!m_model)
        return false;
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
        thread->setOptimizationConfig(m_config);
        if(m_model->SupportThreads())
        {
            thread->run();
        }
        else
            threadpool->start(thread);
        threads << thread;
    }
    
    if(!m_model->SupportThreads())
        threadpool->waitForDone();
    
    bool converged = true;
    for(int i = 0; i < threads.size(); ++i)
    {
        m_result << threads[i]->getResult();
        m_series << threads[i]->getSeries();
        converged = converged && threads[i]->Converged();
        delete threads[i];
    }
    qDeleteAll(threads);
    return converged;
}


void Statistic::setParameter(const QJsonObject& json)
{
    m_model->ImportJSON(json);
}

#include "statistic.moc"
