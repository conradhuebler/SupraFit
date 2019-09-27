/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include <QtCore/QRandomGenerator>
#include <QtCore/QVector>

#include "modelcomparison.h"

void MCThread::run()
{
    QVector<QVector<qreal>> applied_parameters;
    QVector<qreal> parameters = m_model.data()->OptimizeParameters();
    applied_parameters << parameters;

    m_ParameterIndex = m_controller["ParameterIndex"].toInt(0);
    m_MaxParameter = m_controller["MaxParameter"].toDouble();

    QList<int> global_param, local_param, param;
    global_param = ToolSet::String2IntList(m_controller["GlobalParameterList"].toString());
    local_param = ToolSet::String2IntList(m_controller["LocalParameterList"].toString());

    param << global_param << local_param;
    qint64 t0 = QDateTime::currentMSecsSinceEpoch();

    int max = 0;

    for (int i = 0; i < param.size(); ++i)
        max += param[i];
    bool appr = true;

    if (max > 4)
        appr = false;

    /* Lets have two different approaches
     * The first one for only a few parameter and a second for many parameters */

    // The first one
    if (appr) {
        for (int step = 0; step < m_maxsteps; ++step) {
            if (m_interrupt)
                return;

            QVector<qreal> consts = parameters;

            int j = 0;
            for (int i = 0; i < param.size(); ++i) {
                if (param[i])
                    consts[j] = QRandomGenerator::global()->generateDouble() * (m_box[j][1] - m_box[j][0]) + m_box[j][0];
                j++;
            }

            m_model->setParameter(consts);
            m_model->Calculate();
            if (m_model->StatisticVector()[m_ParameterIndex] <= m_MaxParameter) {
                m_results << m_model->ExportModel(false);
            }
            m_steps++;
            if (step % update_intervall == 0) {
                emit IncrementProgress(QDateTime::currentMSecsSinceEpoch() - t0);
                t0 = QDateTime::currentMSecsSinceEpoch();
            }
        }
    } else { // The second one

        auto GenerateRandom = [](const QVector<int>& indicies, const QList<int>& param, int max) -> int {
            int j = QRandomGenerator::global()->bounded(0, param.size());
            while ((indicies.contains(j) || param[j] == 0) && indicies.size() < max)
                j = QRandomGenerator::global()->bounded(0, param.size());
            if (max == indicies.size())
                return -1;
            return j;
        };

        auto GenerateParameter = [=](int j, QVector<qreal>& consts) -> qreal {
            consts[j] = QRandomGenerator::global()->generateDouble() * (m_box[j][1] - m_box[j][0]) + m_box[j][0];
            m_model->setParameter(consts);
            m_model->Calculate();
            return m_model->StatisticVector()[m_ParameterIndex];
        };

        QVector<qreal> consts = applied_parameters[QRandomGenerator::global()->bounded(0, applied_parameters.size())];

        for (int step = 0; step < m_maxsteps; ++step) {
            if (m_interrupt)
                return;
            bool allow_loop = true;

            QVector<int> indicies;

            QVector<qreal> oldconsts = consts;
            int j = GenerateRandom(indicies, param, max);
            int current = 0;
            while ((current < max && allow_loop && indicies.size() < parameters.size())) {

                j = GenerateRandom(indicies, param, max);
                if (j == -1)
                    break;

                indicies << j;

                if (m_interrupt)
                    return;

                if (param[j]) {
                    int attemps = 0;
                    while (attemps < 10) {
                        oldconsts = consts;
                        qreal SSE = GenerateParameter(j, consts);
                        if (SSE <= m_MaxParameter) {
                            oldconsts = consts;
                            current++;
                            attemps = 10;
                        } else
                            attemps++;
                    }
                }
            }
            m_model->setParameter(consts);
            m_model->Calculate();
            if (m_model->StatisticVector()[m_ParameterIndex] <= m_MaxParameter) {
                m_results << m_model->ExportModel(false);
                applied_parameters << consts;
                consts = applied_parameters[QRandomGenerator::global()->bounded(0, applied_parameters.size())];
            } else {
                consts = parameters;
            }
            m_steps++;
            if (step % update_intervall == 0) {
                emit IncrementProgress(QDateTime::currentMSecsSinceEpoch() - t0);
                t0 = QDateTime::currentMSecsSinceEpoch();
            }
        }
    }
}

qreal FCThread::SingleLimit(int parameter_id, int direction)
{
    QVector<double> parameter = m_model.data()->OptimizeParameters();
    double param = parameter[parameter_id];
    double old_param = param;
    int iter = 0;

    int maxiter = m_controller["MaxStepsFastConfidence"].toInt();
    double step = qPow(10, ceil(log10(qAbs(param))) + m_controller["FastConfidenceScaling"].toDouble());

    int ParameterIndex = m_controller["ParameterIndex"].toInt(0);
    qreal MaxParameter = m_controller["MaxParameter"].toDouble();

    param += direction * step;
    double error = m_model.data()->StatisticVector()[ParameterIndex];

    int shrink = 0;
    while (qAbs(error - MaxParameter) > 1e-7) {
        parameter[parameter_id] = param;
        m_model.data()->setParameter(parameter);
        m_model.data()->Calculate();
        error = m_model.data()->StatisticVector()[ParameterIndex];
        if (error < MaxParameter) {
            old_param = param;
            param += step * direction;
            shrink = 0;

        } else {
            if (shrink == 10) {
                step = qAbs(param - old_param) * 0.0005;
                shrink = 0;
            } else {
                step = qAbs(param - old_param) * 0.5;
                shrink++;
            }
            param = old_param + step * direction;
            old_param -= step * direction;
        }
        parameter[parameter_id] = param;
        m_model.data()->setParameter(parameter);
        m_model.data()->Calculate();
        error = m_model.data()->StatisticVector()[ParameterIndex];

        if (error < MaxParameter)
            m_list_points[param] = error;

        iter++;

        if (iter >= maxiter) {
#ifdef _DEBUG
            qDebug() << "fast confidence not converged for parameter" << parameter_id << " going " << direction << " value: " << parameter[parameter_id] << qAbs(error - MaxParameter);
#endif
            break;
        }
    }
#ifdef _DEBUG
    qDebug() << "Final Error " << error << " after " << iter << " steps, value:" << parameter[parameter_id];
#endif

    return param;
}

void FCThread::run()
{
    int ParameterIndex = m_controller["ParameterIndex"].toInt(0);

    QVector<double> parameter = m_model.data()->OptimizeParameters();
    double param = parameter[m_parameter];
    m_list_points[param] = m_model.data()->StatisticVector()[ParameterIndex];

    m_upper = SingleLimit(m_parameter, +1);
    m_model.data()->setParameter(parameter);
    m_model.data()->Calculate();
    m_lower = SingleLimit(m_parameter, -1);
    auto i = m_list_points.constBegin();
    while (i != m_list_points.constEnd()) {
        m_points << QPointF(i.key(), i.value()); // << endl;
        ++i;
    }
}

ModelComparison::ModelComparison(QObject* parent)
    : AbstractSearchClass(parent)
    , m_fast_finished(false)
{
}

ModelComparison::~ModelComparison()
{
}

bool ModelComparison::Run()
{
    if (static_cast<SupraFit::Method>(m_controller["method"].toInt()) == SupraFit::Method::FastConfidence)
        return FastConfidence();
    else
        return Confidence();
}

bool ModelComparison::FastConfidence()
{
    if (!m_model)
        return false;
    for (int i = 0; i < m_series.size(); ++i)
        m_series[i].clear();
    m_series.clear();
    m_results.clear();

    bool series = m_controller["IncludeSeries"].toBool();

    QVector<double> parameter = m_model.data()->OptimizeParameters();
    QVector<QPointer<FCThread>> threads;
    qint64 t0 = QDateTime::currentMSecsSinceEpoch();

    for (int i = 0; i < parameter.size(); ++i) {

        if (!series && i >= m_model->GlobalParameterSize() && m_model->SupportSeries())
            continue;

        QPointer<FCThread> thread = new FCThread(i);
        thread->setController(m_controller);
        thread->setModel(m_model);
        // connect(this, &ModelComparison::Interrupt, thread, &FCThread::Interrupt);

        if (!m_model.data()->SupportThreads())
            m_threadpool->start(thread);
        else
            thread->run();
        threads << thread;
    }
    if (!m_model.data()->SupportThreads()) {
        while (m_threadpool->activeThreadCount())
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    m_multicore_time = QDateTime::currentMSecsSinceEpoch() - t0;

    for (int i = 0; i < threads.size(); ++i) {
        QJsonObject result;
        QPair<int, int> pair = m_model.data()->IndexParameters(threads[i]->Parameter());
        if (pair.second == 0) {
            result["name"] = m_model.data()->GlobalParameterName(pair.first);
            result["type"] = "Global Parameter";
        } else if (pair.second == 1) {
            result["name"] = m_model.data()->LocalParameterName(pair.first);
            result["type"] = "Local Parameter";
        }
        result["value"] = parameter[i];
        result["points"] = ToolSet::Points2String(threads[i]->Points());

        QJsonObject confidence;
        confidence["upper"] = threads[i]->Upper();
        confidence["lower"] = threads[i]->Lower();
        confidence["error"] = m_controller["confidence"].toDouble();
        result["confidence"] = confidence;
        delete threads[i];
        m_results << result;
    }
    return true;
}

QVector<QVector<qreal>> ModelComparison::MakeBox()
{
    QVector<double> global_scaling = ToolSet::String2DoubleVec(m_controller["GlobalParameterScalingList"].toString());
    QVector<double> local_scaling = ToolSet::String2DoubleVec(m_controller["LocalParameterScalingList"].toString());

    QVector<double> param_list = m_model->AllParameter();
    double BoxScalingFactor = m_controller["BoxScalingFactor"].toDouble();

    while (global_scaling.size() < m_model.data()->GlobalParameterSize())
        global_scaling << BoxScalingFactor;

    while (local_scaling.size() < m_model.data()->LocalParameterSize() * m_model.data()->SeriesCount())
        local_scaling << BoxScalingFactor;

    QVector<double> scaling = QVector<double>() << global_scaling << local_scaling;

    QList<QJsonObject> constant_results = Results();
    QVector<QVector<qreal>> parameter;
    m_box_area = 1.0;
    for (const QJsonObject& object : qAsConst(constant_results)) {
        QVector<qreal> constant;
        qreal lower = object["confidence"].toObject()["lower"].toDouble();
        qreal upper = object["confidence"].toObject()["upper"].toDouble();
        qreal value = object["value"].toDouble();
        double BoxScalingFactor = scaling[param_list.indexOf(value)];
        qDebug() << param_list.indexOf(value);
        constant << value - BoxScalingFactor * (value - lower);
        constant << value + BoxScalingFactor * (upper - value);
        parameter << constant;
        m_box_area *= double(BoxScalingFactor * (upper - value) + BoxScalingFactor * (value - lower));
        QList<QPointF> points;
        points << QPointF(lower, upper);
        m_series.append(points);
    }

    for (int i = 0; i < m_series.size(); ++i)
        m_box[QString::number(i)] = ToolSet::Points2String(m_series[i]);
    qDebug() << m_box;
    return parameter;
}

bool ModelComparison::Confidence()
{
    if (!m_model)
        return false;
    // We make an initial guess to estimate the dimension
    m_model.data()->Calculate();
    // m_effective_error =     double MaxError = m_controller["MaxError"].toDouble();

    //if(!m_fast_finished)
    //{
    //   qDebug() << "automatic perform guess";
    m_controller["IncludeSeries"] = true;
    m_controller["FastConfidenceSteps"] = qApp->instance()->property("FastConfidenceSteps").toInt();
    m_controller["FastConfidenceScaling"] = qApp->instance()->property("FastConfidenceScaling").toInt();
    FastConfidence();
    //}

    QVector<QVector<qreal>> box = MakeBox();
    if (box.size() > 1) {
        MCSearch(box);
        return true;
    } else
        return false;
}

void ModelComparison::MCSearch(const QVector<QVector<qreal>>& box)
{
    QVector<QPointer<MCThread>> threads;

    int maxsteps = m_controller["MaxSteps"].toInt();
    emit setMaximumSteps(maxsteps / update_intervall);
    int thread_count = qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(thread_count);
    qint64 t0 = QDateTime::currentMSecsSinceEpoch();

    for (int i = 0; i < thread_count; ++i) {
        MCThread* thread = new MCThread();
        connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
        connect(this, &ModelComparison::StopSubThreads, thread, &MCThread::Interrupt);

        thread->setModel(m_model);
        thread->setController(m_controller);
        thread->setError(m_effective_error);
        thread->setMaxSteps(maxsteps / thread_count);
        thread->setBox(box);
        threads << thread;
        m_threadpool->start(thread);
    }
    while (m_threadpool->activeThreadCount()) {
        QCoreApplication::processEvents();
    }
    m_multicore_time = QDateTime::currentMSecsSinceEpoch() - t0;

    QList<QJsonObject> results;
    for (int i = 0; i < threads.size(); ++i) {
        if (threads[i]) {
            m_steps += threads[i]->Steps();
            results << threads[i]->Results();
            delete threads[i];
        }
    }
    StripResults(results);
}

void ModelComparison::StripResults(const QList<QJsonObject>& results)
{
    /* This can one day be made faster, but not today
     * Actually just move this to the individual thread and collect only the vectors */

    QVector<QPair<qreal, qreal>> confidence_global(m_model->GlobalParameterSize(), QPair<qreal, qreal>(0, 0));
    QVector<QPair<qreal, qreal>> confidence_local(m_model->LocalParameterSize() * m_model->SeriesCount(), QPair<qreal, qreal>(0, 0));

    QVector<QList<qreal>> data_global = QVector<QList<qreal>>(m_model->GlobalParameterSize());
    QVector<QList<qreal>> data_local = QVector<QList<qreal>>(m_model->LocalParameterSize() * m_model->SeriesCount());
    QVector<QPair<qreal, qreal>> local_values = QVector<QPair<qreal, qreal>>(m_model->LocalParameterSize() * m_model->SeriesCount());
    for (const QJsonObject& object : qAsConst(results)) {
            m_models << object;
            QJsonObject constants;
            constants = object["data"].toObject()["globalParameter"].toObject();
            QVector<qreal> param = ToolSet::String2DoubleVec(constants["data"].toObject()["0"].toString());
            for (int i = 0; i < m_model->GlobalParameterSize(); ++i) {
                data_global[i] << param[i];
            }

            constants = object["data"].toObject()["localParameter"].toObject();
            if (m_model->LocalParameterSize()) {
                for (int i = 0; i < m_model->SeriesCount(); ++i) {
                    QVector<qreal> param_raw = ToolSet::String2DoubleVec(constants["data"].toObject()[QString::number(i)].toString());
                    QVector<qreal> checked = ToolSet::String2DoubleVec(constants["checked"].toObject()[QString::number(i)].toString());
                    QVector<qreal> param;
                    for (int x = 0; x < checked.size(); ++x)
                        if (checked[x])
                            param << param_raw[x];
                    for (int j = 0; j < param.size(); ++j) {
                        data_local[i + m_model->SeriesCount() * j] << param[j];
                        local_values[i + m_model->SeriesCount() * j] = QPair<int, int>(j, i);
                    }
                }
            }
            QCoreApplication::processEvents();
    }

    m_ellipsoid_area = double(results.size()) / double(m_steps) * m_box_area;
    m_results.clear();

    QList<int> global_param, local_param;
    global_param = ToolSet::String2IntList(m_controller["GlobalParameterList"].toString());
    local_param = ToolSet::String2IntList(m_controller["LocalParameterList"].toString());

    for (int i = 0; i < confidence_global.size(); ++i) {

        if (!global_param[i])
            continue;
        QJsonObject conf;
        QPair<qreal, qreal> minmax = ToolSet::MinMax(data_global[i]);
        conf["lower"] = minmax.first;
        conf["upper"] = minmax.second;
        conf["error"] = m_controller["confidence"].toDouble(); //m_config.confidence;

        QJsonObject result;
        result["confidence"] = conf;

        result["value"] = m_model->GlobalParameter(i);
        result["name"] = m_model->GlobalParameterName(i);
        result["type"] = "Global Parameter";
        result["data"] = ToolSet::DoubleList2String(data_global[i]);

        m_results << result;
    }

    for (int i = 0; i < confidence_local.size(); ++i) {

        if (!local_param[i])
            continue;

        QJsonObject conf;
        QPair<qreal, qreal> minmax = ToolSet::MinMax(data_local[i]);

        conf["lower"] = minmax.first;
        conf["upper"] = minmax.second;
        conf["error"] = m_controller["confidence"].toDouble();

        QJsonObject result;
        result["confidence"] = conf;

        result["value"] = m_model->LocalParameter(local_values[i].first, local_values[i].second);
        result["name"] = m_model->LocalParameterName(local_values[i].first);
        result["type"] = "Local Parameter";
        result["data"] = ToolSet::DoubleList2String(data_local[i]);

        m_results << result;
    }
    m_controller["steps_taken"] = m_steps;
    m_controller["moco_area"] = m_ellipsoid_area;
}

void ModelComparison::Interrupt()
{
    emit StopSubThreads();
    m_interrupt = true;
    m_threadpool->clear();
}
