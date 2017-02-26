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
#include "src/core/minimizer.h"
#include "src/core/models.h"
#include "src/core/toolset.h"

#include <QtCore/QObject>
#include <QtCore/QCoreApplication>
#include <QtCore/QThreadPool>
#include <QtCore/QVector>
#include <QtCore/QDateTime>

#include <iostream>
#include <cmath>
#include <random>

#include "montecarlostatistics.h"

MonteCarloThread::MonteCarloThread(QObject* parent): QObject(parent), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater))
{
    setAutoDelete(false);
}

MonteCarloThread::~MonteCarloThread()
{
}

void MonteCarloThread::setDataTable(DataTable* table)
{
    m_model->OverrideSignalTable(table);
}

void MonteCarloThread::run()
{
    if(!m_model)
    {
        qDebug() << "no model set";
        return;
    }
    
    m_minimizer->setModel(m_model);
    m_minimizer->Minimize(m_runtype);
    
    m_optimized = m_minimizer->Parameter();
    m_model->ImportJSON(m_optimized);
    m_model->CalculateSignal();
    m_constants = m_model->Constants();
    
}


MonteCarloStatistics::MonteCarloStatistics(QObject *parent): QObject(parent), m_maxsteps(100)
{
    quint64 seed = QDateTime::currentMSecsSinceEpoch();
    rng.seed(seed);
    Phi = std::normal_distribution<double>(0,1e-2);
}

MonteCarloStatistics::~MonteCarloStatistics()
{
    
}

void MonteCarloStatistics::Evaluate()
{
    m_table = new DataTable(m_model->SignalModel());
    QVector<QPointer <MonteCarloThread > > threads;
    for(int step = 0; step < m_maxsteps; ++step)
    {
        QPointer<MonteCarloThread > thread = new MonteCarloThread(this);
        thread->setOptimizationRun(m_runtype);
        thread->setModel(m_model);
        thread->setDataTable(m_model->SignalModel()->PrepareMC(Phi, rng));
        thread->run();
//         QThreadPool::globalInstance()->start(thread);
        threads << thread;
//          QThreadPool::globalInstance()->waitForDone();
        m_model->OverrideSignalTable(m_table);
    }
    
    QVector<QVector<qreal > > m_constants_list(m_model->Constants().size());
    for(int i = 0; i < threads.size(); ++i)
    {
        if(threads[i])
        {
            QList<qreal > constants = threads[i]->Constants();
            for(int j = 0; j < constants.size(); ++j)
                m_constants_list[j] << constants[j];
            
            delete threads[i];
        }
    }
    for(int i = 0; i < m_constants_list.size(); ++i)
    {
        QVector<QPair<qreal, int> > histogram = ToolSet::List2Histogram(m_constants_list[i], 500);
        QList<QPointF> series;
        for(int j = 0; j < histogram.size(); ++j)
        {
            series.append(QPointF(histogram[j].first, histogram[j].second));       
        }
        m_series << series;
    }
    
}

#include "montecarlostatistics.moc"
