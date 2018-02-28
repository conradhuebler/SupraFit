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

#include "src/core/AbstractModel.h"
#include "src/core/toolset.h"

#include <QtCore/QCoreApplication>

#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QVector>

#include "modelcomparison.h"


void MCThread::run()
{
    QVector<std::uniform_int_distribution<int> > dist;
    QVector<double > factors(m_model.data()->GlobalParameterSize(), 1);
    for(int i = 0; i < m_model.data()->GlobalParameterSize(); ++i)
    {
        factors[i] = qPow(10,log10(qAbs(1e7/m_model.data()->GlobalParameter(i))));
        int lower = factors[i]*m_box[i][0];
        int upper = factors[i]*m_box[i][1];
        dist << std::uniform_int_distribution<int>(lower, upper);
    }
    quint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng;
    rng.seed(seed);
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    for(int step = 0; step < m_maxsteps; ++step)
    {
        if(m_interrupt)
            return;

        QList<qreal > consts = m_model.data()->GlobalParameter();
        for(int i = 0; i < m_model.data()->GlobalParameterSize(); ++i) // FIXME dont want that really, but it should work for now
        {
            consts[i] = dist[i](rng)/factors[i];
        }
        m_model->setGlobalParameter(consts);
        m_model->Calculate();
        m_results << m_model->ExportModel(false);

        if(step%update_intervall == 0)
        {
            emit IncrementProgress(QDateTime::currentMSecsSinceEpoch()-t0);
            t0 = QDateTime::currentMSecsSinceEpoch();
        }
    }
}



ModelComparison::ModelComparison(MoCoConfig config, QObject *parent) : AbstractSearchClass(parent), m_config(config)
{
    
}

ModelComparison::~ModelComparison()
{
    
}

bool ModelComparison::FastConfidence()
{
     if(!m_model)
        return false;
    for(int i = 0; i < m_series.size(); ++i)
        m_series[i].clear();
    m_series.clear();
    m_results.clear();
    
    
    m_controller["method"] = SupraFit::Statistic::FastConfidence;
    m_controller["fisher"] = m_config.fisher_statistic;
    m_controller["maxerror"] = m_config.maxerror;
    m_controller["f-value"] = m_config.f_value;
    
    QJsonObject optimized = m_model.data()->ExportModel(false);
    QList<double > parameter = m_model.data()->OptimizeParameters(OptimizationType::ComplexationConstants | ~OptimizationType::OptimizeShifts).toList();
    for(int i = 0; i < parameter.size(); ++i)
    {
        m_model.data()->ImportModel(optimized);
        double upper = SingleLimit(i, +1);
        m_model.data()->ImportModel(optimized);
        double lower = SingleLimit(i, -1);
        
        QJsonObject result;
        result["name"] = m_model.data()->GlobalParameterName(i);
        result["value"] = parameter[i];
        result["type"] = "Global Parameter";
        QJsonObject confidence;
        confidence["upper"] = upper;
        m_model.data()->ImportModel(optimized);
        confidence["lower"] = lower;
        confidence["error"] = m_config.confidence;
        result["confidence"] = confidence;
        m_results << result;
    }
    return true;
}

double ModelComparison::SingleLimit(int parameter_id, int direction)
{
    QVector<double > parameter = m_model.data()->OptimizeParameters(OptimizationType::ComplexationConstants | ~OptimizationType::OptimizeShifts);
    double param = parameter[parameter_id];
    double old_param = param;
    int iter = 0;
    int maxiter = 100;
    double step = 0.5;
    param += direction*step;
    double error = m_model.data()->SumofSquares();
    while(qAbs(error-m_config.maxerror) > 1e-7)
    {
        parameter[parameter_id] = param;
        m_model.data()->setParameter(parameter);
        m_model.data()->Calculate();
        error = m_model.data()->SumofSquares();
        if( error < m_config.maxerror )
        {
            old_param = param;
            param += step*direction;
            
        }else
        {
            step = qAbs(param - old_param)*0.5;
            param = old_param + step*direction;
            old_param -= step*direction;
        }
        parameter[parameter_id] = param;
        m_model.data()->setParameter(parameter);
        m_model.data()->Calculate();
        error = m_model.data()->SumofSquares();
        iter++;
        
        if(iter >= maxiter)
            break;
    }
    return param;
}


QVector<QVector<qreal> > ModelComparison::MakeBox()
{    
    QList<QJsonObject > constant_results = Results();
    QVector<QVector< qreal > > parameter;
    m_box_area = 1.0;
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
        m_box_area *= double(m_config.box_multi*(upper-value)+m_config.box_multi*(value-lower));
        QList<QPointF> points;
        points << QPointF(lower, upper);
        m_series.append( points );
    }

    for(int i = 0; i < m_series.size(); ++i)
        m_box[QString::number(i)] = ToolSet::Points2String(m_series[i]);

    return parameter;
}

bool ModelComparison::Confidence()
{
    if(!m_model)
        return false;
    // We make an initial guess to estimate the dimension
    m_model.data()->Calculate();
    m_effective_error = m_config.maxerror;
    
    FastConfidence();
    
    if(m_model.data()->GlobalParameterSize() == 1)
        return true;
    
    QVector<QVector<qreal> > box = MakeBox();
    if(box.size() > 1)
    {
        MCSearch(box);
        return true;
    }else
        return false;
}


void ModelComparison::MCSearch(const QVector<QVector<qreal> >& box)
{
    QVector<QPointer<MCThread> > threads;
    
    int maxsteps = m_config.mc_steps;
    emit setMaximumSteps(1+maxsteps/update_intervall);
    int thread_count =  qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(thread_count);
    for(int i = 0; i < thread_count; ++i)
    {
        MCThread *thread = new MCThread;
        connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
        thread->setModel(m_model);
        thread->setMaxSteps(maxsteps/thread_count);
        thread->setBox(box);
        threads << thread;
        m_threadpool->start(thread);
    }
    while(m_threadpool->activeThreadCount())
    {
        QCoreApplication::processEvents();
    }
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
    QVector<QPair<qreal, qreal> > confidence(m_model->GlobalParameterSize(), QPair<qreal, qreal>(0,0));
    int inner = 0;
    int all = 0;
    m_data_vec = QVector<QList<qreal > >(m_model->GlobalParameterSize());
    for(const QJsonObject &object : qAsConst(results))
    {
        all++;
        if(object["sum_of_squares"].toDouble() <= m_effective_error)
        {
            inner++;
            m_models << object;
            QJsonObject constants = object["data"].toObject()["globalParameter"].toObject();
            for(int i = 0; i < m_model->GlobalParameterSize(); ++i)
            {
                m_data_vec[i] << constants[QString::number(i)].toString().toDouble();
                qreal min = confidence[i].first;
                qreal max = confidence[i].second;
                
                if(min != 0)
                    confidence[i].first = qMin(min, constants[QString::number(i)].toString().toDouble());
                else
                    confidence[i].first = constants[QString::number(i)].toString().toDouble();

                if(max != 0)
                    confidence[i].second = qMax(max, constants[QString::number(i)].toString().toDouble());
                else
                    confidence[i].second = constants[QString::number(i)].toString().toDouble();
            }
        }
        QCoreApplication::processEvents();
    }
    emit IncrementProgress(1);
    m_ellipsoid_area = double(inner)/double(all)*m_box_area;
    m_results.clear();
    
    for(int i = 0; i < confidence.size(); ++i)
    { 
        QJsonObject conf;
        conf["lower"] = confidence[i].first;
        conf["upper"] = confidence[i].second;
        conf["error"] = m_config.confidence;
        
        QJsonObject result;
        result["confidence"] = conf;
        result["value"] = m_model->GlobalParameter(i);
        result["name"] = m_model->GlobalParameterName(i);
        result["type"] = "Global Parameter";
        m_results << result;
    }

    m_controller["runtype"] = m_config.runtype;
    m_controller["steps"] = m_config.mc_steps;
    m_controller["fisher"] = m_config.fisher_statistic;
    m_controller["maxerror"] = m_config.maxerror;
    m_controller["f-value"] = m_config.f_value;
    m_controller["method"] = SupraFit::Statistic::ModelComparison;
    m_controller["moco_area"] = m_ellipsoid_area;
    QJsonObject data;

    for(int i = 0; i < m_data_vec.size(); ++i)
        data["global_" + QString::number(i)] = ToolSet::DoubleList2String(m_data_vec[i]);

    m_controller["data"] = data;
    m_controller["box"] = m_box;
}

QJsonObject ModelComparison::Controller() const
{
    return m_controller;
}
