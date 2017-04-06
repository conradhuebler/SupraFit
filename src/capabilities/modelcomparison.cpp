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

#include "abstractsearchclass.h"
#include "globalsearch.h"
#include "continuousvariation.h"

#include "src/core/AbstractModel.h"

#include <QtCore/QCoreApplication>

#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QVector>

#include "modelcomparison.h"

const int update_intervall = 10;

void MCThread::run()
{
    QVector<std::uniform_int_distribution<int> > dist;
    double mult = 1e6;
    for(int i = 0; i < m_box.size(); ++i)
    {
        int lower = mult*m_box[i][0];
        int upper = mult*m_box[i][1];
        dist << std::uniform_int_distribution<int>(lower, upper);
    }
    
    quint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng;
    rng.seed(seed);
    
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();

    for(int step = 0; step < m_maxsteps; ++step)
    {
        QList<qreal > consts = m_model.data()->Constants();
        for(int i = 0; i < consts.size(); ++i)
        {
            consts[i] = dist[i](rng)/mult;
        }
        m_model->setConstants(consts);
        m_model->Calculate();
        m_results << m_model->ExportJSON();
        if( step % update_intervall == 0 )
        {
            t1 = QDateTime::currentMSecsSinceEpoch();
            emit IncrementProgress(t1-t0);
            t0 = QDateTime::currentMSecsSinceEpoch();
            QCoreApplication::processEvents();
        }
    }
}



ModelComparison::ModelComparison(MoCoConfig config, QObject *parent) : AbstractSearchClass(parent), m_config(config)
{
    
}

ModelComparison::~ModelComparison()
{
    
}

QVector<QVector<qreal> > ModelComparison::MakeBox() const
{    
    QList<QJsonObject > constant_results = Results();
    QVector<QVector< qreal > > parameter;
    for(const QJsonObject &object : qAsConst(constant_results))
    {
        QVector<qreal> constant;
        qreal lower = object["confidence"].toObject()["lower"].toDouble();
        qreal upper = object["confidence"].toObject()["upper"].toDouble();
        qreal value = object["value"].toDouble();
        constant << value-m_config.box_multi*(value-lower);
        constant << value+m_config.box_multi*(upper-value);
        constant << m_config.cv_config.increment;
        parameter << constant;
    }
    return parameter;
}

bool ModelComparison::EllipsoideConfidence()
{

    if(!m_model)
        return false;
    // We make an initial guess to estimate the dimension
    m_model.data()->Calculate();
    CVConfig cv_config = m_config.cv_config;
    cv_config.relax = false;
    cv_config.increment = 10e-4; //qApp->instance()->property("fast_increment").toDouble();
    qreal error = m_model.data()->SumofSquares();
    m_effective_error = error+error*m_config.maxerror;
    cv_config.maxerror = m_effective_error;
    cv_config.runtype = m_model.data()->LastOptimzationRun();
    
    ContinuousVariation *cv = new ContinuousVariation(cv_config, this);
    cv->setModel(m_model);
    cv->FastConfidence();
    m_results = cv->Results();
    delete cv;
    QVector<QVector<qreal> > box = MakeBox();
    
    if(m_config.method == 1)
        MCSearch(box);
    else
        Search(box);
    
    return true;
}

void ModelComparison::Search(const QVector<QVector<qreal> >& box)
{
    GSConfig config;
    config.optimize = false;
    config.initial_guess = false;
    config.parameter = box;
    GlobalSearch *globalsearch = new GlobalSearch(config, this);
    globalsearch->setModel(m_model); 
    connect(globalsearch, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);
    connect(globalsearch, SIGNAL(setMaximumSteps(int)), this, SIGNAL(setMaximumSteps(int)), Qt::DirectConnection);
    QList<QJsonObject > results = globalsearch->SearchGlobal();
    StripResults(results);
    delete globalsearch;
}

void ModelComparison::MCSearch(const QVector<QVector<qreal> >& box)
{
    
    QVector<QPointer<MCThread> > threads;
    
    int maxsteps = m_config.mc_steps;
    emit setMaximumSteps(maxsteps/update_intervall);
    int thread_count =  qApp->instance()->property("threads").toInt();
    for(int i = 0; i < thread_count; ++i)
    {
        MCThread *thread = new MCThread;
        connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);
        thread->setModel(m_model);
        thread->setMaxSteps(maxsteps/thread_count);
        thread->setBox(box);
        threads << thread;
        m_threadpool->start(thread);
    }
    m_threadpool->waitForDone();
    QList<QJsonObject> results;
    for(int i = 0; i < threads.size(); ++i)
    {
        if(threads[i])
        {
            results << threads[i]->Results();
            delete threads[i];
        }
    }
    StripResults(results);
}


void ModelComparison::StripResults(const QList<QJsonObject>& results)
{ 
    
    QVector<QPair<qreal, qreal> > confidence(m_model->ConstantSize(), QPair<qreal, qreal>(0,0));
    for(const QJsonObject &object : qAsConst(results))
    {
        if(object["sum_of_squares"].toDouble() <= m_effective_error)
        {
            m_models << object;
            QJsonObject constants = object["data"].toObject()["constants"].toObject();

            for(int i = 0; i < m_model->ConstantSize(); ++i)
            {
                qreal min = confidence[i].first;
                qreal max = confidence[i].second;
                
                if(min != 0)
                    confidence[i].first = qMin(min, constants[QString::number(i)].toString().toDouble());
                else
                    confidence[i].first = constants[QString::number(i)].toString().toDouble();
                
                confidence[i].second = qMax(max, constants[QString::number(i)].toString().toDouble());  
            }
        }
    }
    m_results.clear();
    for(int i = 0; i < confidence.size(); ++i)
    { 
        QJsonObject result;
        
        QJsonObject conf;
        conf["lower"] = confidence[i].first;
        conf["upper"] = confidence[i].second;
        result["confidence"] = conf;

        QJsonObject controller;
        controller["runtype"] = m_config.runtype;
        controller["steps"] = m_config.mc_steps;
        result["controller"] = controller;
        result["value"] = m_model->Constant(i);
        result["name"] = m_model->ConstantNames()[i];
        result["type"] = "Complexation Constant";
        m_results << result;
    }
}
