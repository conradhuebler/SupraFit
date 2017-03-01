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

#include <QtCore/QCollator>
#include <QtCore/QObject>
#include <QtCore/QCoreApplication>
#include <QtCore/QThreadPool>
#include <QtCore/QVector>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include <iostream>
#include <cmath>
#include <random>

#include "montecarlostatistics.h"

MonteCarloThread::MonteCarloThread(const MCConfig &config, QObject* parent): QObject(parent), m_config(config),  m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater))
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
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    m_minimizer->setModel(m_model);
    m_minimizer->Minimize(m_config.runtype);
    
    m_optimized = m_minimizer->Parameter();
    m_model->ImportJSON(m_optimized);
    m_model->Calculate();
    m_constants = m_model->Constants();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit IncrementProgress(t1-t0);
}


MonteCarloStatistics::MonteCarloStatistics(const MCConfig &config, QObject *parent): QObject(parent), m_config(config)
{
    quint64 seed = QDateTime::currentMSecsSinceEpoch();
    rng.seed(seed);
    Phi = std::normal_distribution<double>(0,m_config.varianz);
    m_threadpool = QThreadPool::globalInstance();
}

MonteCarloStatistics::~MonteCarloStatistics()
{
    
}

void MonteCarloStatistics::Evaluate()
{
    m_constant_list.clear();
    m_shift_list.clear();
    m_constants.clear();
    m_shifts.clear();
    m_models.clear();
    QVector<QPointer <MonteCarloThread > > threads = GenerateData();
    while(m_threadpool->activeThreadCount())
    {
        QCoreApplication::processEvents();
    }
    Collect(threads);
    AnalyseData();
}


QVector<QPointer <MonteCarloThread > > MonteCarloStatistics::GenerateData()
{    
    
    int maxthreads =qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);
    m_table = new DataTable(m_model->ModelTable());
    QVector<QPointer <MonteCarloThread > > threads;
    for(int step = 0; step < m_config.maxsteps; ++step)
    {
        QPointer<MonteCarloThread > thread = new MonteCarloThread(m_config, this);
        connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
        thread->setModel(m_model);
        thread->setDataTable(m_model->SignalModel()->PrepareMC(Phi, rng));
        m_threadpool->start(thread);
        threads << thread; 
        QCoreApplication::processEvents();
        if(step % 100 == 0)
            emit IncrementProgress(0);
    }
    return threads;
    
}

void MonteCarloStatistics::Collect(const QVector<QPointer<MonteCarloThread> >& threads)
{ 
    QVector<QVector<qreal > > m_constants_list(m_model->Constants().size());
    for(int i = 0; i < threads.size(); ++i)
    {
        if(threads[i])
        {
            QList<qreal > constants = threads[i]->Constants();
            for(int j = 0; j < constants.size(); ++j)
                m_constants_list[j] << constants[j];
            m_models << threads[i]->Model()->ExportJSON();
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

void MonteCarloStatistics::AnalyseData()
{
    m_constant_list.resize(m_model->ConstantSize());
    m_shift_list.resize(m_model->ConstantSize()*m_model->SignalCount());
    for(int i = 0; i < m_models.size(); ++i)
    {
        //         for(int j = 0; j < m_constant_list.size(); ++j)
        //         {
        
        QJsonObject constants = m_models[i]["data"].toObject()["constants"].toObject();
        QStringList keys = constants.keys();
        
        if(keys.size() > 10)
        {
            QCollator collator;
            collator.setNumericMode(true);
            std::sort(
                keys.begin(),
                      keys.end(),
                      [&collator](const QString &key1, const QString &key2)
                      {
                          return collator.compare(key1, key2) < 0;
                      });
        }
        
        QString consts;
        int j = 0;
        for(const QString &str : qAsConst(keys))
        {
            if(str.contains("statistic"))
                continue;
            QString element = constants[str].toString();
            m_constant_list[j] << element.toDouble();
            j++;
        }
        
        
        
        for(int j = 0; j < m_shift_list.size(); ++j)
        {
            //             m_shift_list[j] << m_models[j].
        }
        
    }
    
    for(int i = 0; i < m_constant_list.size(); ++i)
    {
        QList<qreal > list = m_constant_list[i];
        ConfidenceBar bar = ToolSet::Confidence(list);
        StatisticResult result;
        result.bar = bar;
        result.optim = m_model->Constant(i);
        m_constants << result;
    }
}


void MonteCarloStatistics::Interrupt()
{
    QThreadPool::globalInstance()->clear();
}

#include "montecarlostatistics.moc"
