/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/global_config.h"

#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/models.h"
#include "src/core/toolset.h"

#include <QtCore/QCollator>
#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QPointF>
#include <QtCore/QThreadPool>
#include <QtCore/QTimer>
#include <QtCore/QVector>

#include <iostream>
#include <cmath>
#include <random>

#include "montecarlostatistics.h"

MonteCarloThread::MonteCarloThread(const MCConfig &config): m_minimizer(QSharedPointer<Minimizer>(new Minimizer(false, this), &QObject::deleteLater)), m_config(config), m_finished(false)
{
}

MonteCarloThread::~MonteCarloThread()
{
}

void MonteCarloThread::setDataTable(DataTable* table)
{
    m_model->OverrideDependentTable(table);
}

void MonteCarloThread::run()
{
    if(!m_model)
    {
        qDebug() << "no model set";
        return;
    }
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
#ifdef _DEBUG
        qDebug() <<  "started!";
#endif
    m_minimizer->setModel(m_model);
    m_finished = m_minimizer->Minimize(m_config.runtype);
    
    m_optimized = m_minimizer->Parameter();
    m_model->ImportModel(m_optimized);
    m_constants = m_model->GlobalParameter();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit IncrementProgress(t1-t0);
#ifdef _DEBUG
        qDebug() <<  "finished after " << t1-t0 << "msecs!";
#endif
}


MonteCarloStatistics::MonteCarloStatistics(const MCConfig &config, QObject *parent): AbstractSearchClass(parent), m_config(config)
{
    quint64 seed = QDateTime::currentMSecsSinceEpoch();
    rng.seed(seed);
    Phi = std::normal_distribution<double>(0,m_config.variance);
}

MonteCarloStatistics::~MonteCarloStatistics()
{
    
}

void MonteCarloStatistics::Evaluate()
{
    m_models.clear();
    QVector<QPointer <MonteCarloThread > > threads = GenerateData();
    while(m_threadpool->activeThreadCount())
        QCoreApplication::processEvents();
    
    Collect(threads);
    m_results = ToolSet::Model2Parameter(m_models);
    QJsonObject controller;
    controller["runtype"] = m_config.runtype;
    controller["steps"] = m_steps;
    controller["variance"] = m_config.variance;
    controller["original"] = m_config.original;
    controller["bootstrap"] = m_config.bootstrap;
    ToolSet::Parameter2Statistic(m_results, m_model.data(), controller);
}


QVector<QPointer <MonteCarloThread > > MonteCarloStatistics::GenerateData()
{    
    int maxthreads =qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);
    m_model->Calculate();
    m_table = new DataTable(m_model->ModelTable());
    QVector<QPointer <MonteCarloThread > > threads;
    m_generate = true;
    QVector<qreal> vector = m_model->ErrorTable()->toList();
    Uni = std::uniform_int_distribution<int>(0,vector.size()-1);
#ifdef _DEBUG
    qDebug() << "Starting MC Simulation with" << m_config.maxsteps << "steps";
#endif
    for(int step = 0; step < m_config.maxsteps; ++step)
    {
        QPointer<MonteCarloThread > thread = new MonteCarloThread(m_config);
        connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
        thread->setModel(m_model);
        DataTable *table;
        if(m_config.original)
            table = m_model->DependentModel();
        else
            table = m_model->ModelTable();

        if(m_config.bootstrap)
            thread->setDataTable(table->PrepareBootStrap(Uni, rng, vector));
        else
            thread->setDataTable(table->PrepareMC(Phi, rng));
        m_threadpool->start(thread);
#ifdef _DEBUG
        qDebug() << "Thread added to queue!" << thread;
#endif
        threads << thread; 
        QCoreApplication::processEvents();
        if(step % 100 == 0)
            emit IncrementProgress(0);
        if(!m_generate)
            break;
    }
    return threads;
}

void MonteCarloStatistics::Collect(const QVector<QPointer<MonteCarloThread> >& threads)
{ 
    m_steps = 0;
    for(int i = 0; i < threads.size(); ++i)
    {
        if(threads[i])
        {
            if(!threads[i]->Finished())
            {
                delete threads[i];
                continue;
            }
            m_models << threads[i]->Model();
            m_steps++;
            delete threads[i];
        }
    }
}


void MonteCarloStatistics::Interrupt()
{
    m_generate = false;
    m_threadpool->clear();
}

#include "montecarlostatistics.moc"
