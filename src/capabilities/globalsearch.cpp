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

#include "src/capabilities/abstractsearchclass.h"
#include "src/capabilities/montecarlostatistics.h"

#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/models.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QThreadPool>
#include <QtCore/QVector>

#include <iostream>

#include "globalsearch.h"

SearchBatch::SearchBatch(const GSConfig& config, QPointer<GlobalSearch> parent)
    : m_config(config)
    , m_finished(false)
    , m_parent(parent)
    , m_checked(false)
{
}

void SearchBatch::run()
{
    m_thread = new NonLinearFitThread(false);
    while (true) {
        GSResult result;
        if (m_interrupt)
            break;
        QVector<double> parameter = m_parent->DemandParameter();
        if (parameter.isEmpty())
            break;
        result.initial = parameter;
        for (int i = 0; i < parameter.size(); ++i)
            m_model->SetSingleParameter(parameter[i], i);
        optimise();
        result.optimised = m_model->AllParameter();
        result.model = m_model->ExportModel(false, false);
        result.SumError = m_model->SumofSquares();
        m_result << result;
    }
    delete m_thread;
}

void SearchBatch::optimise()
{
    if (!m_model || m_interrupt) {
        qDebug() << "no model set";
        return;
    }
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
#ifdef _DEBUG
//         qDebug() <<  "started!";
#endif
    m_model->setLockedParameter(m_config.ignored_parameter);
    m_thread->setModel(m_model, false);
    m_thread->run();
    m_model->ImportModel(m_thread->ConvergedParameter());
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit IncrementProgress(t1 - t0);
#ifdef _DEBUG
//         qDebug() <<  "finished after " << t1-t0 << "msecs!";
#endif
}

GlobalSearch::GlobalSearch( QObject* parent)
    : AbstractSearchClass(parent)
{
}


GlobalSearch::~GlobalSearch()
{
}
void GlobalSearch::Interrupt()
{
    emit InterruptAll();
}

QVector<QVector<double>> GlobalSearch::ParamList()
{
    m_max_count = 1;

    m_time = 0;
    QVector<QVector<double>> full_list;
    for (int i = 0; i < m_config.parameter.size(); ++i) {
        QVector<double> list;
        double min = 0, max = 0, step = 0;
        min = m_config.parameter[i][0];
        max = m_config.parameter[i][1];
        step = m_config.parameter[i][2];
        m_max_count *= (max + step - min) / step;
        for (double s = min; s <= max; s += step)
            list << s;
        full_list << list;
    }
    emit setMaximumSteps(m_max_count);
    return full_list;
}

QList<QJsonObject> GlobalSearch::SearchGlobal()
{
    QVector<QVector<double>> full_list = ParamList();
    m_models.clear();
    QVector<double> error;
    std::cout << "starting the scanning ..." << m_max_count << " steps approx." << std::endl;
    int t0 = QDateTime::currentMSecsSinceEpoch();
    ConvertList(full_list);
    int t1 = QDateTime::currentMSecsSinceEpoch();
    std::cout << "time for scanning: " << t1 - t0 << " msecs." << std::endl;
    return m_models;
}

void GlobalSearch::ConvertList(const QVector<QVector<double>>& full_list)
{
    m_full_list.clear();
    QVector<int> position(full_list.size(), 0);
    int maxthreads = qApp->instance()->property("threads").toInt();
    m_allow_break = false;

    if (m_config.initial_guess)
        m_model->InitialGuess();

    QVector<double> parameter = m_model->AllParameter();

    while (!m_allow_break) {
        QCoreApplication::processEvents();
        for (int j = 0; j < position.size(); ++j) {
            parameter[j] = full_list[j][position[j]];
        }
        bool temporary = true;
        for (int i = 0; i < position.size(); ++i) {
            temporary = temporary && (position[i] == full_list[i].size() - 1);
        }
        if (temporary)
            m_allow_break = true;

        m_input.enqueue(parameter);

        for (int k = position.size() - 1; k >= 0; --k) {
            if (position[k] == (full_list[k].size() - 1))
                position[k] = 0;

            else {
                position[k]++;
                if (position[k] == full_list[k].size()) {
                    position[k] = 0;
                    if (k > 0)
                        position[k - 1]++;
                }
                break;
            }
        }
    }

    emit setMaximumSteps(m_input.size());

    QVector<QPointer<SearchBatch>> threads;

    for (int i = 0; i < maxthreads; ++i) {
        QPointer<SearchBatch> thread = new SearchBatch(m_config, this);
        connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
        connect(this, &GlobalSearch::InterruptAll, thread, &SearchBatch::Interrupt);
        thread->setModel(m_model);
        threads << thread;
        m_threadpool->start(thread);
    }

    while (m_threadpool->activeThreadCount())
        QCoreApplication::processEvents();

    for (int i = 0; i < threads.size(); ++i) {
        QCoreApplication::processEvents();
        m_result << threads[i]->Result();
        delete threads[i];
    }

    return;
}

/*
void GlobalSearch::Scan(const QVector<QVector<double>>& list)
{
    for (int i = 0; i < m_series.size(); ++i)
        m_series[i].clear();
    m_series.clear();
    QVector<double> error;
    for (int j = 0; j < list.size(); ++j) {
        QList<QPointF> series;
        for (int i = 0; i < list[j].size(); ++i) {
            m_model->setGlobalParameter(QList<qreal>() << list[j][i]);
            m_model->Calculate();
            error << m_model->ModelError();
            series.append(QPointF(list[j][i], m_model->ModelError()));
        }
        m_series << series;
    }
}*/

void GlobalSearch::ExportResults(const QString& filename, double threshold, bool allow_invalid)
{
    QJsonObject toplevel;
    int i = 0;
    for (const QJsonObject& obj : qAsConst(m_models)) {
        QJsonObject constants = obj["data"].toObject()["constants"].toObject();
        QStringList keys = constants.keys();
        bool valid = true;
        for (const QString& str : qAsConst(keys)) {
            double var = constants[str].toString().toDouble();
            valid = valid && (var > 0);
        }
        double error = obj["sse"].toDouble();
        if (error < threshold && (valid || allow_invalid))
            toplevel["model_" + QString::number(i++)] = obj;
    }
    JsonHandler::WriteJsonFile(toplevel, filename);
}

QJsonObject GlobalSearch::Controller() const
{
#warning adopt and complete

    QJsonObject controller;
    controller["method"] = "Global Search";

    return controller;
}

QVector<qreal> GlobalSearch::DemandParameter()
{
    QMutexLocker lock(&mutex);
    if (m_input.isEmpty())
        return QVector<qreal>();
    else
        return m_input.dequeue();
}

#include "globalsearch.moc"
