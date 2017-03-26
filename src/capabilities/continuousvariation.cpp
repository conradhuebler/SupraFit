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


#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QThreadPool>
#include <QtCore/QWeakPointer>


#include <iostream>

#include "continuousvariation.h"

ContinuousVariationThread::ContinuousVariationThread(const CVConfig &config, bool check_convergence) : m_config(config), m_check_convergence(check_convergence), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(false, this), &QObject::deleteLater)), m_converged(true)
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
    m_model.data()->Calculate();
    if(m_config.optimizer_config.error_potenz == 2)
        m_error = m_model.data()->SumofSquares();
    else
        m_error = m_model.data()->SumofAbsolute();
    QList<QPointF> series;
    QJsonObject optimized = m_model.data()->ExportJSON(false);
    QVector<double > parameter = m_model.data()->OptimizeParameters(m_config.runtype);
    QJsonObject controller;
    controller["runtype"] = m_config.runtype;
    controller["steps"] = m_config.maxsteps;
    controller["increment"] = m_config.increment;
    controller["maxerror"] = m_config.maxerror;
    m_result["controller"] = controller;
    m_result["name"] = m_model.data()->ConstantNames()[m_parameter_id];
    m_result["value"] = parameter[m_parameter_id];
    m_result["type"] = "Complexation Constant";
    double integ_5 = 0;
    double integ_1 = 0;
    QJsonObject confidence;
    //FIXME this must be adopted to F-statistics
    confidence["upper_5"] = SumErrors(1, integ_5, integ_1, series);
    m_model.data()->ImportJSON(optimized);
    confidence["lower_5"] = SumErrors(0, integ_5, integ_1, series);
    m_result["confidence"] = confidence;
    
    m_series = series;
    m_result["integ_5"] = integ_5/m_error;
    m_result["integ_1"] = integ_1/m_error;
}

void ContinuousVariationThread::setParameter(const QJsonObject& json)
{
    m_model.data()->ImportJSON(json);
}


qreal ContinuousVariationThread::SumErrors(bool direction, double& integ_5, double& integ_1, QList<QPointF> &series)
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
    QList<qreal > consts = m_model.data()->Constants();
    double constant_ = consts[m_parameter_id];
    allow_break = false;
    double par;
    for(int m = 0; m < m_config.maxsteps; ++m)
    {
        
        par = constant_ + double(m)*increment;
        
        consts[m_parameter_id] = par;
        m_model.data()->setConstants(consts);
        
        if(m_config.relax)
        {
            m_minimizer->Minimize(m_config.runtype, locked);
            QJsonObject json_exp = m_minimizer->Parameter();
            m_model.data()->ImportJSON(json_exp);
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
        if(allow_break)
            break;
        emit IncrementProgress(0);
    }
    return par;
}

void ContinuousVariationThread::Interrupt()
{
    allow_break = true;
}

ContinuousVariation::ContinuousVariation(const CVConfig &config, QObject *parent) : QObject(parent), m_config(config), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(false, this), &QObject::deleteLater))
{
    
    
    
}

ContinuousVariation::~ContinuousVariation()
{
    
    
    
}

bool ContinuousVariation::FastConfidence()
{
    if(!m_model)
        return false;
    for(int i = 0; i < m_series.size(); ++i)
        m_series[i].clear();
    m_series.clear();
    m_result.clear();
    m_minimizer->setModel(m_model);
    QJsonObject optimized = m_model.data()->ExportJSON();
    QList<double > parameter = m_model.data()->OptimizeParameters(OptimizationType::ComplexationConstants | ~OptimizationType::OptimizeShifts).toList();
    
    m_model.data()->Calculate();
    QThreadPool *threadpool = QThreadPool::globalInstance();
    QList<QPointer <ContinuousVariationThread > > threads;
    int maxthreads =qApp->instance()->property("threads").toInt();
    threadpool->setMaxThreadCount(maxthreads);
    allow_break = false;
    
    for(int i = 0; i < parameter.size(); ++i)
    {
        QPointer<ContinuousVariationThread >thread = new ContinuousVariationThread(m_config, false);
        connect(this, SIGNAL(StopSubThreads()), thread, SLOT(Interrupt()), Qt::DirectConnection);
        //         connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
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
    ConstantsFromThreads(threads, true);
    qDeleteAll(threads);
    return true;
}

QVector<QVector<qreal> > ContinuousVariation::MakeBox() const
{    
    QList<QJsonObject > constant_results = Results();
    QVector<QVector< qreal > > parameter;
    for(const QJsonObject &object : qAsConst(constant_results))
    {
        QVector<qreal> constant;
        qreal lower = object["confidence"].toObject()["lower_5"].toDouble();
        qreal upper = object["confidence"].toObject()["upper_5"].toDouble();
        qreal value = object["value"].toDouble();
        constant << value-2*(value-lower);
        constant << value+2*(upper-value);
        constant << 0.0015;
        parameter << constant;
    }
    return parameter;
}


bool ContinuousVariation::EllipsoideConfidence()
{
    m_cv = false;
    if(!m_model)
        return false;
    // We make an initial guess to estimate the dimension
    m_model.data()->Calculate();
    CVConfig config;
    config.relax = false;
    config.increment = qApp->instance()->property("fast_increment").toDouble();
    qreal error = m_model.data()->SumofSquares();
    config.maxerror = error+error*0.05;
    m_config = config;
    m_config.runtype = m_model.data()->LastOptimzationRun();
    FastConfidence();
    
    
    QVector<QVector<qreal> > box = MakeBox();
    qDebug() << box;
    Search(box);
    return true;
}

void ContinuousVariation::Search(const QVector<QVector<qreal> >& box)
{
    GlobalSearch *globalsearch = new GlobalSearch(this);
    globalsearch->setModel(m_model); 
    globalsearch->setParameter(box);
    
    globalsearch->setInitialGuess(false);
    globalsearch->setOptimize(false);
    
    //     connect(globalsearch, SIGNAL(SingeStepFinished(int)), this, SIGNAL(SingeStepFinished(int)));
    //     connect(globalsearch, SIGNAL(setMaximumSteps(int)), this, SIGNAL(setMaximumSteps(int)));
    connect(globalsearch, SIGNAL(SingeStepFinished(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);
    connect(globalsearch, SIGNAL(setMaximumSteps(int)), this, SIGNAL(setMaximumSteps(int)), Qt::DirectConnection);
    QList<QJsonObject > results = globalsearch->SearchGlobal();
    qDebug() << "done";
    StripResults(results);
    delete globalsearch;
    qDebug() << "collected";
}

void ContinuousVariation::MCSearch(const QVector<QVector<qreal> >& box)
{
}

void ContinuousVariation::StripResults(const QList<QJsonObject>& results)
{ 
    for(const QJsonObject &object : qAsConst(results))
    {
        if(object["sum_of_squares"].toDouble() <= m_config.maxerror)
            m_models << object;
    }
}


QHash<QString, QList<qreal> > ContinuousVariation::ConstantsFromThreads(QList<QPointer<ContinuousVariationThread> >& threads, bool store)
{
    QHash<QString, QList<qreal> > constants;
    for(int i = 0; i < threads.size(); ++i)
    {
        if(store)
            m_result << threads[i]->getResult();
        
        QList<qreal > vars;
        if(!threads[i])
            continue;
        m_models << threads[i]->Model();
        vars << threads[i]->getResult()["confidence"].toObject()["lower_5"].toDouble() << threads[i]->getResult()["confidence"].toObject()["upper_5"].toDouble();
        constants[threads[i]->getResult()["name"].toString()].append( vars );
        delete threads[i];
    }
    return constants;
}


bool ContinuousVariation::ConfidenceAssesment()
{
    m_cv = true;
    if(!m_model)
        return false;
    for(int i = 0; i < m_series.size(); ++i)
        m_series[i].clear();
    m_series.clear();
    
    m_minimizer->setModel(m_model);
    QJsonObject optimized = m_model.data()->ExportJSON(false);
    QList<double > parameter = m_model.data()->OptimizeParameters(OptimizationType::ComplexationConstants | ~OptimizationType::OptimizeShifts).toList();
    
    m_model.data()->Calculate();
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
        m_models << threads[i]->Model();
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
