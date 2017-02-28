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

#include "continuousvariation.h"

ContinuousVariationThread::ContinuousVariationThread(const CVConfig &config) : m_config(config), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater)), m_increment(1e-3), m_maxsteps(1e4), m_converged(true)
{
    setAutoDelete(false);
}

ContinuousVariationThread::~ContinuousVariationThread()
{
}

void ContinuousVariationThread::setModel(QSharedPointer<AbstractTitrationModel> model)
{ 
    m_model = model->Clone(); 
    m_minimizer->setModel(m_model);
}

void ContinuousVariationThread::run()
{
    m_model.data()->CalculateSignal();
    if(m_config.optimizer_config.error_potenz == 2)
        m_error = m_model.data()->SumofSquares();
    else
        m_error = m_model.data()->SumofAbsolute();
    QList<QPointF> series;
    QJsonObject optimized = m_model.data()->ExportJSON();
    QVector<double > parameter = m_model.data()->OptimizeParameters(m_config.runtype);
    
    m_result.optim = parameter[m_parameter_id];
    m_result.name = m_model.data()->ConstantNames()[m_parameter_id];
    
    double integ_5 = 0;
    double integ_1 = 0;
    SumErrors(1, integ_5, integ_1, series);
    m_model.data()->ImportJSON(optimized);
    SumErrors(0, integ_5, integ_1, series);
    
    
    m_series = series;
    m_result.integ_5 = integ_5/m_error;
    m_result.integ_1 = integ_1/m_error;
}

void ContinuousVariationThread::setParameter(const QJsonObject& json)
{
    m_model.data()->ImportJSON(json);
}


void ContinuousVariationThread::SumErrors(bool direction, double& integ_5, double& integ_1, QList<QPointF> &series)
{
    double increment = m_increment;
    if(!direction)
        increment *= -1;
    qreal old_error = m_error;
    int counter = 0;
    QList<int> locked; 
    for(int i = 0; i <  m_model.data()->OptimizeParameters(m_config.runtype).size(); ++i)
        locked << 1;
    locked[m_parameter_id] = 0;
    QList<qreal > consts = m_model.data()->Constants();
    double constant_ = consts[m_parameter_id];
    allow_break = false;
    for(int m = 0; m < m_maxsteps; ++m)
    {
        
        double par = constant_ + double(m)*increment;
       
        consts[m_parameter_id] = par;
        m_model.data()->setConstants(consts);
        m_minimizer->Minimize(m_config.runtype, locked);
        
        QJsonObject json_exp = m_minimizer->Parameter();
        m_model.data()->ImportJSON(json_exp);
        m_model.data()->CalculateSignal();
        
        qreal new_error;
        
        if(m_config.optimizer_config.error_potenz == 2)
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
        
        QCoreApplication::processEvents();
        if(allow_break)
            break;
        emit IncrementProgress(0);
    }
}

void ContinuousVariationThread::Interrupt()
{
    allow_break = true;
}

ContinuousVariation::ContinuousVariation(const CVConfig &config, QObject *parent) : QObject(parent), m_config(config), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater))
{
    
    
    
}

ContinuousVariation::~ContinuousVariation()
{
    
    
    
}

bool ContinuousVariation::ConfidenceAssesment()
{
    if(!m_model)
        return false;
    for(int i = 0; i < m_series.size(); ++i)
        m_series[i].clear();
    m_series.clear();
    
    m_minimizer->setModel(m_model);
    QJsonObject optimized = m_model.data()->ExportJSON();
    QList<double > parameter = m_model.data()->OptimizeParameters(OptimizationType::ComplexationConstants | ~OptimizationType::OptimizeShifts).toList();
    
    m_model.data()->CalculateSignal();
    QThreadPool *threadpool = QThreadPool::globalInstance();
    QList<QPointer <ContinuousVariationThread > > threads;
    int maxthreads =qApp->instance()->property("threads").toInt();
    threadpool->setMaxThreadCount(maxthreads);
    allow_break = false;
    for(int i = 0; i < parameter.size(); ++i)
    {
        QPointer<ContinuousVariationThread >thread = new ContinuousVariationThread(m_config);
        connect(this, SIGNAL(StopSubThreads()), thread, SLOT(Interrupt()), Qt::DirectConnection);
        connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
        thread->setModel(m_model);
        thread->SetParameterID(i);
        thread->setOptimizationRun(OptimizationType::ComplexationConstants| ~OptimizationType::OptimizeShifts);
        if(m_model.data()->SupportThreads())
        {
            thread->run();
        }
        else
            threadpool->start(thread);
        threads << thread;
        if(allow_break)
            break;
    }
    
    if(!m_model.data()->SupportThreads())
    {
        while(threadpool->activeThreadCount())
        {
            QCoreApplication::processEvents();
        }
    }
    
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


void ContinuousVariation::setParameter(const QJsonObject& json)
{
    m_model.data()->ImportJSON(json);
}


void ContinuousVariation::Interrupt()
{
    emit StopSubThreads();
    allow_break = true;
}

#include "continuousvariation.moc"
