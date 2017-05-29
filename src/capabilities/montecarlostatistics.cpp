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

MonteCarloThread::MonteCarloThread(const MCConfig &config): m_config(config),  m_minimizer(QSharedPointer<Minimizer>(new Minimizer(false, this), &QObject::deleteLater))
{
    setAutoDelete(false);
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
        qDebug() << this << "started!";
#endif
    m_minimizer->setModel(m_model);
    m_minimizer->Minimize(m_config.runtype);
    
    m_optimized = m_minimizer->Parameter();
    m_model->ImportModel(m_optimized);
//     m_model->Calculate();
    m_constants = m_model->GlobalParameter();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit IncrementProgress(t1-t0);
#ifdef _DEBUG
        qDebug() <<  this << "finished after " << t1-t0 << "msecs!";
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
    m_constant_list.clear();
    m_shift_list.clear();
    
    m_models.clear();
    QVector<QPointer <MonteCarloThread > > threads = GenerateData();
    while(m_threadpool->activeThreadCount())
    {
        QCoreApplication::processEvents();
    }
    Collect(threads);
    
    m_constant_list.resize(m_model->GlobalParameterSize());
    m_shift_list.resize(m_model->GlobalParameterSize()*m_model->SeriesCount()+m_model->SeriesCount());
    for(int i = 0; i < m_models.size(); ++i)
    {       
        ExtractFromJson(i, "globalParameter");
        ExtractFromJson(i, "localParameter");
    }
    for(int i = 0; i < m_constant_list.size(); ++i)
    {
        QList<double > vector = m_constant_list[i];
        std::sort(vector.begin(), vector.end());
        m_constant_list[i] = vector;
    }
    
    for(int i = 0; i < m_shift_list.size(); ++i)
    {
        QList<double > vector = m_shift_list[i];
        std::sort(vector.begin(), vector.end());
        m_shift_list[i] = vector;
    }
    
    AnalyseData();
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
    QVector<QVector<qreal > > m_constants_list(m_model->GlobalParameterSize());
    for(int i = 0; i < threads.size(); ++i)
    {
        if(threads[i])
        {
            QList<qreal > constants = threads[i]->Constants();
            for(int j = 0; j < constants.size(); ++j)
                m_constants_list[j] << constants[j];
            m_models << threads[i]->Model();
            m_steps++;
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

void MonteCarloStatistics::AnalyseData(qreal error)
{
    m_results.clear();
    for(int i = 0; i < m_constant_list.size(); ++i)
    {
        QList<qreal > list = m_constant_list[i];
        QJsonObject result = MakeJson(list, 100-error);
        result["value"] = m_model->GlobalParameter(i);
        result["name"] = m_model->GlobalParameterNames()[i];
        result["type"] = "Global Parameter";
        result["error"] = error;
        m_results << result;
    }
for(int parameter = 0; parameter < m_model->LocalParameterSize(); ++parameter)
    
    {
        for(int series = 0; series < m_model->SeriesCount(); ++series)
        {
            QList<qreal > list = m_shift_list[series*m_model->LocalParameterSize()+parameter];
            QJsonObject result = MakeJson(list, 100-error);
            result["value"] = m_model->LocalParameter(parameter, series);
            result["name"] = m_model->LocalParameterName(parameter) + m_model->DependentModel()->headerData(parameter, Qt::Horizontal, Qt::DisplayRole).toString();

            result["type"] = "Local Parameter";
            result["error"] = error;
            qDebug() << series << parameter << m_shift_list[series*m_model->LocalParameterSize()+parameter] << result;
            m_results << result;
        }
    }
    emit AnalyseFinished();
}

void MonteCarloStatistics::ExtractFromJson(int i, const QString &string)
{
    QJsonObject object =  m_models[i]["data"].toObject()[string].toObject();
    QStringList keys = object.keys();
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
    
    int j = 0;
    for(const QString &str : qAsConst(keys))
    {
        QString element = object[str].toString();
        if(string == "globalParameter")
            m_constant_list[j] << element.toDouble();
        else if(string == "localParameter")
        {
            /*
             * We transform the tables of the local parameter to an array of vectors
             */
            QVector<double> vector = ToolSet::String2DoubleVec(element);
            for(int i = 0; i < vector.size(); ++i)
                m_shift_list[j*m_model->LocalParameterSize()+i] << vector[i];
        }
        j++;
    }  
}

QJsonObject MonteCarloStatistics::MakeJson(QList<qreal>& list, qreal error)
{
    ConfidenceBar bar = ToolSet::Confidence(list, error);
    QJsonObject result;
    
    QJsonObject confidence;
    confidence["lower"] = bar.lower;
    confidence["upper"] = bar.upper;
    result["confidence"] = confidence;
    
    QJsonObject controller;
    controller["runtype"] = m_config.runtype;
    controller["steps"] = m_steps;
    controller["variance"] = m_config.variance;
    controller["original"] = m_config.original;
    controller["bootstrap"] = m_config.bootstrap;
    result["controller"] = controller;
    return result;
}

void MonteCarloStatistics::Interrupt()
{
    m_generate = false;
    m_threadpool->clear();
}

#include "montecarlostatistics.moc"
