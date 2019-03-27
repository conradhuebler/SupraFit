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

#include <iostream>

#include "weakenedgridsearch.h"

WGSearchThread::WGSearchThread(const QJsonObject& controller)
    : m_controller(controller)
    , m_stationary(false)
    , m_finished(false)
    , m_converged(false)
{
    m_MaxSteps = m_controller["MaxSteps"].toInt();
    m_MaxOvershotCounter = m_controller["OverShotCounter"].toInt();
    m_MaxErrorDecreaseCounter = m_controller["ErrorDecreaseCounter"].toInt();
    m_MaxErrorConvergencyCounter = m_controller["ErrorConvergencyCounter"].toInt();
    m_ScalingFactor = m_controller["StepScalingFactor"].toInt();
    m_MaxError = m_controller["MaxError"].toInt();

    setUp();
}

WGSearchThread::~WGSearchThread()
{
}

void WGSearchThread::run()
{
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    m_model.data()->Calculate();
    m_ModelError = m_model.data()->SumofSquares();

    QList<QPointF> series;
    QVector<double> parameter = m_model.data()->OptimizeParameters();
    m_result["value"] = parameter[m_index];
    Calculate();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit IncrementProgress(t1 - t0);
}

void WGSearchThread::Calculate()
{
    bool interrupt = false;

    QList<int> locked = m_model->LockedParameters();
    QVector<qreal> param = m_model->OptimizeParameters();

    locked[m_index] = 0;

    qreal error = m_ModelError;
    m_steps = 0;
    double value = param[m_index];

    double increment = qPow(10, ceil(log10(qAbs(value))) + m_ScalingFactor);

    m_last = value;
    NonLinearFitThread* thread = new NonLinearFitThread(false);

    while (m_steps < m_MaxSteps && m_ErrorDecreaseCounter < m_MaxErrorDecreaseCounter && m_OvershotCounter < m_MaxOvershotCounter && m_ErrorConvergencyCounter < m_MaxErrorConvergencyCounter) {

        m_steps++;

        value += (increment * m_direction);
        param[m_index] = value;

        m_model.data()->SetSingleParameter(value, m_index);
        m_model->setLockedParameter(locked);
        thread->setModel(m_model, false);
        thread->run();
        bool converged = thread->Converged();

        QJsonObject model;

        if (converged)
            model = thread->ConvergedParameter();
        else
            model = thread->BestIntermediateParameter();
        qreal new_error = thread->SumOfError();

        if (new_error > m_MaxError) {
            m_OvershotCounter++;
            m_finished = true;
        } else {
            m_models << model;
            m_last = value;

            if (m_direction == 1) {
                m_x.append(value);
                m_y.append(new_error);
            } else {
                m_x.prepend(value);
                m_y.prepend(new_error);
            }
        }

        if (new_error < m_ModelError)
            m_ErrorDecreaseCounter++;

        if (qAbs(new_error - error) < m_ErrorConvergency)
            m_ErrorConvergencyCounter++;

        error = new_error;
        if (m_interrupt)
        {
            interrupt = true;
            break;
        }
    }
    if(interrupt)
        m_finished = false;

    m_converged = m_ErrorDecreaseCounter < m_MaxErrorDecreaseCounter;
    m_stationary = m_ErrorConvergencyCounter > m_MaxErrorConvergencyCounter;

    delete thread;
}

WeakenedGridSearch::WeakenedGridSearch(QObject* parent)
    : AbstractSearchClass(parent)
{
}

WeakenedGridSearch::~WeakenedGridSearch()
{
}

QPointer<WGSearchThread> WeakenedGridSearch::CreateThread(int index, bool direction)
{
    QPointer<WGSearchThread> thread = new WGSearchThread(m_controller);
    connect(this, SIGNAL(StopSubThreads()), thread, SLOT(Interrupt()), Qt::DirectConnection);
    connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
    thread->setModel(m_model);
    thread->setParameterId(index);
    if (!direction)
        thread->setDown();

    if (m_model.data()->SupportThreads()) {
        thread->run();
    } else
        m_threadpool->start(thread);
    return thread;
}

bool WeakenedGridSearch::ConfidenceAssesment()
{
    m_cv = true;
    if (!m_model)
        return false;
    for (int i = 0; i < m_series.size(); ++i)
        m_series[i].clear();
    m_series.clear();

    QVector<double> parameter = m_model.data()->OptimizeParameters();

    m_model.data()->Calculate();
    QList<QPair<QPointer<WGSearchThread>, QPointer<WGSearchThread>>> threads;
    int maxthreads = qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);

    QList<int> global_param, local_param;
    global_param = ToolSet::String2IntList(m_controller["GlobalParameterList"].toString());
    local_param = ToolSet::String2IntList(m_controller["LocalParameterList"].toString());

    for (int i = 0; i < parameter.size(); ++i) {
        bool step = true;

        QPair<int, int> index_pair = m_model.data()->IndexParameters(i);
        if (index_pair.second == 0) {
            step = global_param[index_pair.first];
        } else if (index_pair.second == 1) {
            step = local_param[index_pair.first];
        }

        if (!step)
            continue;

        QPair<QPointer<WGSearchThread>, QPointer<WGSearchThread>> pair;
        pair.first = CreateThread(i, 1);
        pair.second = CreateThread(i, 0);
        threads << pair;
        if (m_interrupt)
            break;
    }

    if (!m_model.data()->SupportThreads()) {
        while (m_threadpool->activeThreadCount()) {
            QCoreApplication::processEvents();
        }
    }

    bool converged = true;
    for (int i = 0; i < threads.size(); ++i) {
        QPair<QPointer<WGSearchThread>, QPointer<WGSearchThread>> pair = threads[i];
        int index = pair.first->ParameterId();

        m_models << pair.second->IntermediateResults();
        m_models << pair.first->IntermediateResults();

        QList<qreal> x, y;
        x << pair.second->XSeries() << pair.first->XSeries();
        y << pair.second->YSeries() << pair.first->YSeries();

        qreal upper = pair.first->Last();
        qreal lower = pair.second->Last();

        QJsonObject result;
        QPair<int, int> index_pair = m_model.data()->IndexParameters(index);
        if (index_pair.second == 0) {
            result["name"] = m_model.data()->GlobalParameterName(index);
            result["type"] = "Global Parameter";
        } else if (index_pair.second == 1) {
            result["name"] = m_model.data()->LocalParameterName(index_pair.first);
            result["type"] = "Local Parameter";
            result["index"] = QString::number(0) + "|" + QString::number(index_pair.first);
        }
        result["value"] = parameter[index];

        result["converged"] = pair.first->Converged() && pair.second->Converged();
        result["stationary"] = pair.first->Stationary() && pair.second->Stationary();
        result["finished"] = pair.first->Finished() && pair.second->Finished();
        result["steps"] = pair.first->Steps() + pair.second->Steps();
        result["OvershotCounter"] = pair.first->OvershotCounter() + pair.second->OvershotCounter();
        result["ErrorDecreaseCounter"] = pair.first->ErrorDecreaseCounter() + pair.second->ErrorDecreaseCounter();
        result["ErrorConvergencyCounter"] = pair.first->ErrorConvergencyCounter() + pair.second->ErrorConvergencyCounter();

        bool local = pair.first->Converged() && pair.second->Converged() && pair.first->Finished() && pair.second->Finished() && !(pair.first->Stationary() && pair.second->Stationary());

        QJsonObject confidence;
        confidence["upper"] = upper;
        confidence["lower"] = lower;
        confidence["error"] = m_controller["confidence"].toDouble();

        result["confidence"] = confidence;
        QJsonObject data;
        data["x"] = ToolSet::DoubleList2String(x);
        data["y"] = ToolSet::DoubleList2String(y);
        result["data"] = data;

        m_results << result;
        converged = converged && local;

        delete pair.first;
        delete pair.second;
    }

    if (!m_StoreIntermediate)
        m_models.clear();

    return converged;
}

void WeakenedGridSearch::setParameter(const QJsonObject& json)
{
    m_model.data()->ImportModel(json);
}

QJsonObject WeakenedGridSearch::Controller() const
{
    /*
    QJsonObject controller;
    controller["steps"] = m_config.maxsteps;
    controller["increment"] = m_config.increment;
    controller["maxerror"] = m_config.maxerror;
    controller["fisher"] = m_config.fisher_statistic;
    controller["f-value"] = m_config.f_value;
    controller["method"] = SupraFit::Statistic::WeakenedGridSearch;
    controller["ErrorConvergency"] = m_config.ErrorConvergency;
    controller["OverShotCounter"] = m_config.OvershotCounter;
    controller["ErrorDecreaseCounter"] = m_config.ErrorDecreaseCounter;
    controller["ErrorConvergencyCounter"] = m_config.ErrorConvergencyCounter;
    return controller;*/

    return m_controller;
}

void WeakenedGridSearch::Interrupt()
{
    emit StopSubThreads();
    m_interrupt = true;
    m_threadpool->clear();
}

#include "weakenedgridsearch.moc"
