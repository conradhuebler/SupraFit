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

#include "globalsearch.h"

#include "src/core/AbstractModel.h"
#include "src/core/minimizer.h"

#include <QCoreApplication>

#include <QtCore/QDateTime>

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QThreadPool>
#include <QtCore/QWeakPointer>

#include <random>
#include <iostream>

#include "weakenedgridsearch.h"

WeakenedGridSearchThread::WeakenedGridSearchThread(const WGSConfig &config, bool check_convergence) : m_config(config), m_check_convergence(check_convergence), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(false, this), &QObject::deleteLater)), m_converged(true)
{
    
}

WeakenedGridSearchThread::~WeakenedGridSearchThread()
{
}

void WeakenedGridSearchThread::run()
{
    m_minimizer->setModel(m_model);
    m_model.data()->Calculate();
    if(m_config.optimizer_config.error_potenz == 2)
        m_error = m_model.data()->SumofSquares();
    else
        m_error = m_model.data()->SumofAbsolute();
    QList<QPointF> series;
    QJsonObject optimized = m_model.data()->ExportModel(false);
    QVector<double > parameter = m_model.data()->OptimizeParameters(m_config.runtype);
    QJsonObject controller;
    controller["runtype"] = m_config.runtype;
    controller["steps"] = m_config.maxsteps;
    controller["increment"] = m_config.increment;
    controller["maxerror"] = m_config.maxerror;
    controller["fisher"] = m_config.fisher_statistic;
    controller["f-value"] = m_config.f_value;
    m_result["controller"] = controller;
    m_result["name"] = m_model.data()->ConstantNames()[m_parameter_id];
    m_result["value"] = parameter[m_parameter_id];
    m_result["type"] = "Complexation Constant";
    m_result["error"] = m_config.confidence;
    double integ_5 = 0;
    double integ_1 = 0;
    QJsonObject confidence;
    confidence["upper"] = SumErrors(1, integ_5, integ_1, series);
    m_model.data()->ImportModel(optimized);
    confidence["lower"] = SumErrors(0, integ_5, integ_1, series);
    m_result["confidence"] = confidence;
    
    m_series = series;
    m_result["integ_5"] = integ_5/m_error;
    m_result["integ_1"] = integ_1/m_error;
}

void WeakenedGridSearchThread::setParameter(const QJsonObject& json)
{
    m_model.data()->ImportModel(json);
}


qreal WeakenedGridSearchThread::SumErrors(bool direction, double& integ_5, double& integ_1, QList<QPointF> &series)
{
    double increment = m_config.increment;
    if(!direction)
        increment *= -1;
    qreal old_error = m_error;
    int counter = 0;
    QList<int> locked; 
    for(int i = 0; i <  m_model.data()->OptimizeParameters(m_config.runtype).size(); ++i)
        locked << 1;
    locked[m_parameter_id] = 0;
    QList<qreal > consts = m_model.data()->GlobalParameter();
    double constant_ = consts[m_parameter_id];
    double par = 0;
    for(int m = 0; m < m_config.maxsteps; ++m)
    {
        
        par = constant_ + double(m)*increment;
        
        consts[m_parameter_id] = par;
        m_model.data()->setGlobalParameter(consts);
        
        if(m_config.relax)
        {
            m_minimizer->Minimize(m_config.runtype, locked);
            QJsonObject json_exp = m_minimizer->Parameter();
            m_model.data()->ImportModel(json_exp);
        }
        
        m_model.data()->Calculate();
        
        qreal new_error;
        
        if(m_config.optimizer_config.error_potenz == 2)
            new_error = m_model.data()->SumofSquares();
        else
            new_error = m_model.data()->SumofAbsolute();
        if(new_error < m_error && m_check_convergence)
            counter++;
        qreal rect = qMin(new_error,old_error)*qAbs(increment);
        qreal triangle = 0.5*increment*qAbs(new_error-old_error);
        qreal integ = rect+triangle; 
        integ_5 += integ;
        
        if(new_error/m_error <= double(1.005) && new_error > m_error)
            integ_1 += integ;
        
        if(new_error > m_config.maxerror)
        {
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
        if(m_interrupt)
            break;
        emit IncrementProgress(0);
    }
    return par;
}



WeakenedGridSearch::WeakenedGridSearch(const WGSConfig &config, QObject *parent) : AbstractSearchClass(parent), m_config(config), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(false, this), &QObject::deleteLater))
{
    
    
    
}

WeakenedGridSearch::~WeakenedGridSearch()
{
    
    
    
}

QHash<QString, QList<qreal> > WeakenedGridSearch::ConstantsFromThreads(QList<QPointer<WeakenedGridSearchThread> >& threads, bool store)
{
    QHash<QString, QList<qreal> > constants;
    for(int i = 0; i < threads.size(); ++i)
    {
        if(store)
            m_results << threads[i]->Result();
        
        QList<qreal > vars;
        if(!threads[i])
            continue;
        m_models << threads[i]->Model();
        vars << threads[i]->Result()["confidence"].toObject()["lower"].toDouble() << threads[i]->Result()["confidence"].toObject()["upper"].toDouble();
        constants[threads[i]->Result()["name"].toString()].append( vars );
        delete threads[i];
    }
    return constants;
}


bool WeakenedGridSearch::ConfidenceAssesment()
{
    m_cv = true;
    if(!m_model)
        return false;
    for(int i = 0; i < m_series.size(); ++i)
        m_series[i].clear();
    m_series.clear();
    
    m_minimizer->setModel(m_model);
    QJsonObject optimized = m_model.data()->ExportModel(false);
    QList<double > parameter = m_model.data()->OptimizeParameters(OptimizationType::ComplexationConstants | ~OptimizationType::OptimizeShifts).toList();
    
    m_model.data()->Calculate();
    QList<QPointer <WeakenedGridSearchThread > > threads;
    int maxthreads =qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);
    for(int i = 0; i < parameter.size(); ++i)
    {
        QPointer<WeakenedGridSearchThread >thread = new WeakenedGridSearchThread(m_config);
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
            m_threadpool->start(thread);
        threads << thread;
        if(m_interrupt)
            break;
    }
    
    if(!m_model.data()->SupportThreads())
    {
        while(m_threadpool->activeThreadCount())
        {
            QCoreApplication::processEvents();
        }
    }
    
    bool converged = true;
    for(int i = 0; i < threads.size(); ++i)
    {
        m_models << threads[i]->Model();
        m_results << threads[i]->Result();
        m_series << threads[i]->Series();
        converged = converged && threads[i]->Converged();
        delete threads[i];
    }
    qDeleteAll(threads);
    return converged;
}


void WeakenedGridSearch::setParameter(const QJsonObject& json)
{
    m_model.data()->ImportModel(json);
}


void WeakenedGridSearch::Interrupt()
{
    emit StopSubThreads();
    m_interrupt = true;
    m_threadpool->clear();
}

#include "weakenedgridsearch.moc"
