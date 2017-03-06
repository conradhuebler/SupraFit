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
    Phi = std::normal_distribution<double>(0,m_config.variance);
    m_threadpool = QThreadPool::globalInstance();
}

MonteCarloStatistics::~MonteCarloStatistics()
{
    
}

void MonteCarloStatistics::Evaluate()
{
    m_constant_list.clear();
    m_shift_list.clear();
    m_mc_results.clear();
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
    m_generate = true;
    for(int step = 0; step < m_config.maxsteps || m_generate; ++step)
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
    m_shift_list.resize(m_model->ConstantSize()*m_model->SignalCount()+m_model->SignalCount());
    for(int i = 0; i < m_models.size(); ++i)
    {       
        ExtractFromJson(i, "constants");
        ExtractFromJson(i, "pureShift");
        
        for(int k = 0; k < m_model->ConstantSize(); ++k)
        {
            ExtractFromJson(i, "shift_" + QString::number(k));
        }
    }
    
    for(int i = 0; i < m_constant_list.size(); ++i)
    {
        QList<qreal > list = m_constant_list[i];
        QJsonObject result = MakeJson(list);
        result["value"] = m_model->Constant(i);
        result["name"] = m_model->ConstantNames()[i];
        result["type"] = "Complexation Constant";
        m_mc_results << result;
    }
    
    for(int i = 0; i < m_shift_list.size(); ++i)
    {
        QList<qreal > list = m_shift_list[i];
        QJsonObject result = MakeJson(list);
        /*
         * Some fun goes here, since our data are one long vector
         * the 0 - SignalCount() Datas are one block
         */
        int nr = ceil(i/m_model->SignalCount()); // nr = 0 -> pure shifts, after SignalCount() runs, it gets incremented
        int mod = i%m_model->SignalCount(); // this is the modulo, which says what index the parameter is
        if(nr == 0)
        {
            result["value"] = m_model->PureParameter()(i,0);
            result["name"] = "Host Shift: " + m_model->SignalModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        }else
        {
            result["value"] = m_model->ComplexParameter()(mod,nr-1);
            result["name"] = m_model->ConstantNames()[nr-1] + " Component Shift: " + m_model->SignalModel()->headerData(mod, Qt::Horizontal, Qt::DisplayRole).toString();
        }
        result["type"] = "Shift";
        m_mc_results << result;
    }
    
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
        /*
         * Thank to json, we know what is to find where
         * constants as constants
         * pureShifts are the first n = SignalCount() Entries 
         * shift_0 are the next n +1 - 2n entries
         * and so on
         * it really is not the best solution
         */
        if(string == "constants")
            m_constant_list[j] << element.toDouble();
        else if(string == "pureShift")
            m_shift_list[j] << element.toDouble();
        else
        {
            int add = QString(string).remove("shift_").toInt() + 1;
            m_shift_list[j + add*m_model->SignalCount()] << element.toDouble();
        }
        j++;
    }  
    
}

QJsonObject MonteCarloStatistics::MakeJson(QList<qreal>& list)
{
    ConfidenceBar bar = ToolSet::Confidence(list);
    QJsonObject result;
    
    QJsonObject confidence;
    confidence["upper_5"] = bar.upper_5;
    confidence["upper_2_5"] = bar.upper_2_5;
    confidence["lower_2_5"] = bar.lower_2_5;
    confidence["lower_5"] = bar.lower_5;
    result["confidence"] = confidence;
    
    QJsonObject controller;
    controller["runtype"] = m_config.runtype;
    controller["steps"] = m_config.maxsteps;
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
