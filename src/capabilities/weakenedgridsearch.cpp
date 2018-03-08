/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/toolset.h"

#include <QCoreApplication>

#include <QtCore/QDateTime>

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QThreadPool>
#include <QtCore/QWeakPointer>

#include <random>
#include <iostream>

#include "weakenedgridsearch.h"


WGSearchThread::WGSearchThread(const WGSConfig &config) : m_config(config),
    m_minimizer(QSharedPointer<Minimizer>(new Minimizer(false, this), &QObject::deleteLater)),
    m_stationary(false),
    m_finished(false),
    m_converged(false)
{

}

WGSearchThread::~WGSearchThread()
{

}

void WGSearchThread::run()
{
    m_minimizer->setModel(m_model);
    m_model.data()->Calculate();
    m_error = m_model.data()->SumofSquares();

    QList<QPointF> series;
    QJsonObject optimized = m_model.data()->ExportModel(false);
    QVector<double > parameter = m_model.data()->OptimizeParameters();
    m_result["value"] = parameter[m_index];
    Calculate();
}


void WGSearchThread::Calculate()
{
    double increment = m_config.increment;
    QList<int >locked = m_model->LockedParamters();
    QVector<qreal > param = m_model->OptimizeParameters();

    locked[m_index] = 0;

    qreal error = 0;
    m_steps = 0;
    int counter = 0;
    double value = m_start;
    bool converged = false;
    while(!converged)
    {
        if(m_start < m_end)
        {
            converged = value > m_end;
            value += increment;
        }else
        {
            converged = value < m_end;
            value -= increment;
        }
        param[m_index] = value;
        m_steps++;
        m_model.data()->setParameter(param);

        m_minimizer->Minimize(m_config.runtype, locked);
        QJsonObject model = m_minimizer->Parameter();
        m_model.data()->ImportModel(model);


        m_model.data()->Calculate();

        qreal new_error = m_model.data()->SumofSquares();
        m_models[new_error] = model;

        if(new_error > m_config.maxerror || qAbs(new_error - error) < m_config.error_conv)
        {
            m_finished = new_error > m_config.maxerror;
            m_stationary = qAbs(new_error - error) < m_config.error_conv;
            break;
        }


        error = new_error;

        m_x.append(value);
        m_y.append(new_error);
        if(new_error < m_error)
            counter++;
        // QCoreApplication::processEvents();
        if(m_interrupt)
            break;
        emit IncrementProgress(0);
    }
    m_converged = counter > 50;

}

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
    m_error = m_model.data()->SumofSquares();

    QList<QPointF> series;
    QJsonObject optimized = m_model.data()->ExportModel(false);
    QVector<double > parameter = m_model.data()->OptimizeParameters();
    m_result["name"] = m_model.data()->GlobalParameterName(m_parameter_id);
    m_result["value"] = parameter[m_parameter_id];
    m_result["type"] = "Global Parameter";
    
    double integ_5 = 0;
    double integ_1 = 0;
    QJsonObject confidence;
    confidence["upper"] = SumErrors(1, integ_5, integ_1, series);
    m_model.data()->ImportModel(optimized);
    confidence["lower"] = SumErrors(0, integ_5, integ_1, series);
    confidence["error"] = m_config.confidence;

    m_result["confidence"] = confidence;
    QJsonObject data;
    data["x"] = ToolSet::DoubleList2String(m_x);
    data["y"] = ToolSet::DoubleList2String(m_y);
    m_result["data"] = data;
    
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
    for(int i = 0; i <  m_model.data()->OptimizeParameters().size(); ++i)
        locked << 1;
    locked[m_parameter_id] = 0;
    QVector<qreal > consts = m_model.data()->OptimizeParameters();
    double constant_ = consts[m_parameter_id];
    double par = 0;
    for(int m = 0; m < m_config.maxsteps; ++m)
    {
        
        par = constant_ + double(m)*increment;
        
        consts[m_parameter_id] = par;
        m_model.data()->setParameter(consts);
        
        if(m_config.relax)
        {
            m_minimizer->Minimize(m_config.runtype, locked);
            QJsonObject json_exp = m_minimizer->Parameter();
            m_model.data()->ImportModel(json_exp);
        }
        
        m_model.data()->Calculate();
        
        qreal new_error = m_model.data()->SumofSquares();

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
        {
//             series.append(QPointF(par,new_error));
            m_x.append(par);
            m_y.append(new_error);
        }
        else
        {
//             series.prepend(QPointF(par,new_error));
            m_x.prepend(par);
            m_y.prepend(new_error);
        }
        old_error = new_error;
        
        QCoreApplication::processEvents();
        if(m_interrupt)
            break;
        emit IncrementProgress(0);
    }
    if(counter > 50)
    {
            m_converged = false;
            qDebug() << "not converged " << direction;
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
    QVector<double > parameter = m_model.data()->OptimizeParameters();
    
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

QJsonObject WeakenedGridSearch::Controller() const
{
    QJsonObject controller;
    controller["runtype"] = m_config.runtype;
    controller["steps"] = m_config.maxsteps;
    controller["increment"] = m_config.increment;
    controller["maxerror"] = m_config.maxerror;
    controller["fisher"] = m_config.fisher_statistic;
    controller["f-value"] = m_config.f_value;
    controller["method"] = SupraFit::Statistic::WeakenedGridSearch;
    
    return controller;
}

void WeakenedGridSearch::Interrupt()
{
    emit StopSubThreads();
    m_interrupt = true;
    m_threadpool->clear();
}

#include "weakenedgridsearch.moc"
