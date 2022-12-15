/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2021 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QMutexLocker>
#include <QtCore/QRandomGenerator>

#include "src/core/models/models.h"

#include "src/core/libmath.h"
#include "src/core/minimizer.h"
#include "src/core/toolset.h"

#include "src/capabilities/montecarlostatistics.h"

#include <cmath>
#include <iostream>

#include "resampleanalyse.h"

ResampleAnalyse::ResampleAnalyse()
{
}

ResampleAnalyse::~ResampleAnalyse()
{
    // m_model.clear();
}

bool ResampleAnalyse::Run()
{
    if (static_cast<SupraFit::Method>(AccessCI(m_controller, "Method").toInt()) == SupraFit::Method::Reduction) {
        PlainReduction();
        return true;
    } else {
        CrossValidation();
        return true;
    }
}

void ResampleAnalyse::addThread(QPointer<MonteCarloThread> thread)
{
    m_threads << thread;
    m_threadpool->start(thread);
}

bool ResampleAnalyse::Pending() const
{
    return m_threadpool->activeThreadCount();
}

void ResampleAnalyse::CrossValidation()
{
    int type = m_controller["CXO"].toInt();
    QHash<int, Pair> block;
    int blocksize;
    int maxthreads = qApp->instance()->property("threads").toInt();
    QPointer<DataTable> table = new DataTable(m_model->DependentModel());
    QVector<QPointer<MonteCarloBatch>> threads;

    QList<qreal> x;

    int index = 0;
    int real_points = 0;

    /* I will keep the x-vector in correct length, single rows could be disabled */
    for (int i = m_model->DataBegin(); i < m_model->DataEnd(); ++i) {
        // for (int i = 0; i < m_model->DataEnd(); ++i) {
        x << m_model->PrintOutIndependent(i);
        real_points += m_model->DependentModel()->isRowChecked(i);
    }
    qint64 t0 = QDateTime::currentMSecsSinceEpoch();

    bool more_message = true;
    switch (type) {
    case 1:
        blocksize = 1;
        // for (int i = 0; i < m_model->DataEnd(); ++i) {
        for (int i = m_model->DataBegin(); i < m_model->DataEnd(); ++i) {
            if (!m_model->DependentModel()->isRowChecked(i))
                continue;

            QPointer<DataTable> dep_table = new DataTable(table);
            QVector<int> indicies = QVector<int>() << i;
            Vector vector = dep_table->DisableRow(i);

            block.insert(index, Pair(m_model->IndependentModel(), dep_table));
            m_job.insert(index, indicies);
            if (block.size() == blocksize) {
                m_batch.enqueue(block);
                block.clear();
            }
            index++;
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        break;
    case 2:
        blocksize = 1;
        for (int i = m_model->DataBegin(); i < m_model->DataEnd(); ++i) {
            // for (int i = 0; i < m_model->DataEnd(); ++i) {
            if (!m_model->DependentModel()->isRowChecked(i))
                continue;
            for (int j = i + 1; i < m_model->DataEnd(); ++i) {
                // for (int j = i + 1; j < m_model->DataEnd(); ++j) {
                if (!m_model->DependentModel()->isRowChecked(j))
                    continue;

                QPointer<DataTable> dep_table = new DataTable(table);
                dep_table->DisableRow(i);
                dep_table->DisableRow(j);
                QVector<int> indicies = QVector<int>() << i << j;

                block.insert(index, Pair(m_model->IndependentModel(), dep_table));
                m_job.insert(index, indicies);

                if (block.size() == blocksize) {
                    m_batch.enqueue(block);
                    block.clear();
                }
                index++;
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            }
        }
        break;

    case 3:

        blocksize = 1;

        int X = m_controller["X"].toInt();
        int steps = m_controller["MaxSteps"].toInt();
        int algorithm = m_controller["Algorithm"].toInt();

        int points = m_model->DataEnd();

        int end = X * (points - 1);
        long double maxsteps = tgammal(points + 1) / (tgammal(X + 1) * tgammal(points - X + 1));

        double ratio = double(steps) / double(maxsteps);

        /* There is an ongoing process here, in the end, an ideal algorithm selection should be placed here
         *
         * The short start is:
         *  We can either precompute all combinations, which of course grows exponentionally and will for values bigger than 5e4 not usefull
         *  Or we perform a random vector filling with the according condition, which might be good for a certain range
         * For now, lets use the random algorithm for maxsteps >1e5 and steps/maxsteps < 0.75 */

        if (X >= real_points - 1) {
            emit m_model->Info()->Warning(tr("LXO exceeds DataPoints! You probably disabled some rows in the Overview."));
            more_message = false;
            break;
        }
        QVector<QVector<int>> vector_block;
        QVector<int> used_indicies;

        QVector<int> vector(X, 0);
        for (int i = 0; i < X; ++i)
            vector[i] = i;

        emit Message(tr("Map generation!"));

        if (algorithm == 2) {
            if ((maxsteps > 1e5 && ratio < 0.75) || X > 10)
                algorithm = 3;
            else
                algorithm = 1;
        }

        m_controller["Algorithm"] = algorithm;

        if (algorithm == 3) {
#ifdef _DEBUG
            qDebug() << "Random filling method";
            qint64 t0 = QDateTime::currentMSecsSinceEpoch();
#endif
            int run = 0;
            while (run < steps && qAbs(run - maxsteps) > 1e-5) {
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

                QVector<int> vector;
                while (vector.size() < X) {
                    int index = QRandomGenerator::global()->bounded(0, points);
                    if (vector.contains(index))
                        continue;
                    vector << index;
                }
                std::sort(vector.begin(), vector.end());
                if (vector_block.contains(vector))
                    continue;
                vector_block << vector;

                run++;
            }
            //#ifdef _DEBUG
            qint64 t1 = QDateTime::currentMSecsSinceEpoch();
            qDebug() << t1 - t0 << " msecs" << vector_block.size();
            //#endif

            for (int i = 0; i < vector_block.size(); ++i) {
                emit Message(tr("Running %1 jobs!").arg(steps));

                QVector<int> vector = vector_block[i];

                QPointer<DataTable> dep_table = new DataTable(table);
                QVector<int> indicies;
                int checked = 0;
                for (int i = 0; i < vector.size(); ++i) {
                    checked += bool(m_model->DependentModel()->isRowChecked(vector[i]));
                    dep_table->DisableRow(vector[i]);
                    indicies << vector[i];
                }

                if (checked != vector.size()) {
                    delete dep_table;
                    continue;
                }
                block.insert(index, Pair(m_model->IndependentModel(), dep_table));
                m_job.insert(index, indicies);

                if (block.size() == blocksize) {
                    m_batch.enqueue(block);
                    block.clear();
                }
                index++;
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            }

        } else if (algorithm == 1) {
            //#ifdef _DEBUG
            qDebug() << "Full-Precomputing";
            qint64 t0 = QDateTime::currentMSecsSinceEpoch();
            //#endif
            auto IncreaseVector = [this](QVector<int>& vector, int points) {
                for (int i = vector.size() - 1; i >= 0; --i) {
                    if (vector[i] < points - 1) {
                        int value = vector[i];
                        while (vector.contains(value) && value < points - 1)
                            value++;
                        vector[i] = value;
                        break;
                    } else if (vector[i] == points - 1) {
                        if (i) {
                            int back = 1;
                            while (vector[i - back] == points - 1)
                                back++;
                            int value = vector[i - back];
                            while (vector.contains(value))
                                value++;
                            if (value + 1 < points)
                                vector[i] = value + 1;
                        } else
                            vector[i] = 0;
                    }
                }
            };

            bool loop = true;
            while (loop) {
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

                if (QSet<int>(vector.begin(), vector.end()).size() == vector.size()) {
                    vector_block << vector;
                }

                IncreaseVector(vector, points);

                int sum = 0;
                for (int i = 0; i < vector.size(); ++i)
                    sum += vector[i];
                loop = sum < end;
            }
            //#ifdef _DEBUG
            qint64 t1 = QDateTime::currentMSecsSinceEpoch();
            qDebug() << t1 - t0 << " msecs" << vector_block.size();
            //#endif

            if (vector_block.size() < steps) {
                emit Message(tr("Running all jobs!"));

                for (int i = 0; i < vector_block.size(); ++i) {
                    QVector<int> vector = vector_block[i];

                    QPointer<DataTable> dep_table = new DataTable(table);
                    QVector<int> indicies;
                    int checked = 0;
                    for (int i = 0; i < vector.size(); ++i) {
                        checked += bool(m_model->DependentModel()->isRowChecked(vector[i]));
                        dep_table->DisableRow(vector[i]);
                        indicies << vector[i];
                    }

                    if (checked != vector.size()) {
                        delete dep_table;
                        continue;
                    }

                    block.insert(index, Pair(m_model->IndependentModel(), dep_table));
                    m_job.insert(index, indicies);

                    if (block.size() == blocksize) {
                        m_batch.enqueue(block);
                        block.clear();
                    }
                    index++;
                    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                }
            } else {
                emit Message(tr("Running %1 jobs!").arg(steps));

                while (used_indicies.size() < steps) {
                    int index = QRandomGenerator::global()->bounded(0, vector_block.size());

                    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

                    if (used_indicies.contains(index))
                        continue;
                    used_indicies << index;
                    QVector<int> vector = vector_block[index];

                    QPointer<DataTable> dep_table = new DataTable(table);
                    QVector<int> indicies;
                    int checked = 0;
                    for (int i = 0; i < vector.size(); ++i) {
                        checked += bool(m_model->DependentModel()->isRowChecked(vector[i]));
                        dep_table->DisableRow(vector[i]);
                        indicies << vector[i];
                    }

                    if (checked != vector.size()) {
                        delete dep_table;
                        continue;
                    }

                    block.insert(index, Pair(m_model->IndependentModel(), dep_table));
                    m_job.insert(index, indicies);

                    if (block.size() == blocksize) {
                        m_batch.enqueue(block);
                        block.clear();
                    }
                    index++;
                }
            }
        }
        break;
    }
    emit setMaximumSteps(m_batch.size());
    m_controller["MaxSteps"] = m_batch.size();
    for (int i = 0; i < maxthreads; ++i) {
        QPointer<MonteCarloBatch> thread = new MonteCarloBatch(this);
        thread->setChecked(true);
        connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);
        connect(this, &ResampleAnalyse::InterruptAll, thread, &MonteCarloBatch::Interrupt);
        thread->setModel(m_model);
        threads << thread;
        m_threadpool->start(thread);
    }

    while (Pending()) {
        QCoreApplication::processEvents();
    }
    m_multicore_time = QDateTime::currentMSecsSinceEpoch() - t0;

    if (more_message)
        emit Message(tr("Final evaluation in progress! Sorry, but this is done in serial mode - no parallelisation right now!"));
    bool left_out_points = m_controller["LeftOutPoints"].toBool();
    // TODO some times, I will parallise it at all
    QSharedPointer<AbstractModel> calc_model = m_model->Clone();
    QJsonObject chart_block;
    int calculation = 0;

    for (int i = 0; i < threads.size(); ++i) {
        if (threads[i]) {
            QHash<int, QJsonObject> models = threads[i]->Models();
            std::cout << "Thread " << i << " performed " << threads[i]->Counter() << " calculation in " << threads[i]->Timer() << " msecs." << std::endl;
            calculation += threads[i]->Counter();

            for (const QJsonObject& model : models) {
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                if (left_out_points) {
                    int index = models.key(model);
                    QVector<int> indicies = m_job.value(index);

                    calc_model->ImportModel(model);
                    calc_model->Calculate();
                    QString points = QString();
                    for (int j : indicies) {
                        if (m_model->DependentModel()->isRowChecked(j))
                            points = points + ToolSet::DoubleList2String(m_model->ModelTable()->Row(j)) + "|";
                    }
                    points.truncate(points.size() - 1);
                    chart_block[ToolSet::IntVec2String(indicies)] = points;
                }
                m_models << model;
            }
            delete threads[i];
        }
    }
    std::cout << calculation << " in total" << std::endl;

    calc_model.clear();
    if (more_message && left_out_points) // this will be set to false, if LXO was apported
    {
        m_controller["chart"] = chart_block;
        m_controller["xlabel"] = m_model.data()->XLabel();
        m_controller["ylabel"] = m_model.data()->YLabel();
        m_controller["series_count"] = m_model->SeriesCount();
        m_controller["x"] = ToolSet::DoubleList2String(x);
        m_controller["DependentModel"] = m_model->DependentModel()->ExportTable(true);
    }

    if (m_models.size()) {
        m_results = ToolSet::Model2Parameter(m_models);
        ToolSet::Parameter2Statistic(m_results, m_model.data());
    }
    if (table)
        delete table;
    emit AnalyseFinished();
}

void ResampleAnalyse::PlainReduction()
{
    m_controller["xlabel"] = m_model.data()->XLabel();
    m_controller["Cutoff"] = m_model.data()->ReductionCutOff();
    int maxthreads = qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);
    QPointer<DataTable> table = m_model->DependentModel();
    emit setMaximumSteps(m_model->DataEnd() - 4);
    int mind_points = m_model->DataBegin() + 3;
    qint64 t0 = QDateTime::currentMSecsSinceEpoch();

    if (m_controller["ReductionRuntype"].toInt() == 1 || m_controller["ReductionRuntype"].toInt() == 3) {
        for (int i = m_model->DataEnd() - 1; i > mind_points && table->EnabledRows() >= mind_points; --i) {
            QPointer<MonteCarloThread> thread = new MonteCarloThread();
            connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
            thread->setIndex(i);
            QSharedPointer<AbstractModel> model = m_model->Clone();
            table->DisableRow(i);
#pragma message("have a look at here, while restructureing stuff")
            model->setDependentTable(table);
            model->detach();
            thread->setModel(model);
            addThread(thread);
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    } else if (m_controller["ReductionRuntype"].toInt() == 2 || m_controller["runtype"].toInt() == 3) {
        for (int i = 1; i < m_model->DataEnd() - 3 && table->EnabledRows() >= mind_points; ++i) {
            QPointer<MonteCarloThread> thread = new MonteCarloThread();
            connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
            thread->setIndex(i);
            QSharedPointer<AbstractModel> model = m_model->Clone();
            table->DisableRow(i);
#pragma message("have a look at here, while restructureing stuff")
            model->setDependentTable(table);
            model->detach();
            thread->setModel(model);
            addThread(thread);
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }

    while (Pending()) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    m_multicore_time = QDateTime::currentMSecsSinceEpoch() - t0;

    QList<qreal> x;
    for (int i = 0; i < m_threads.size(); ++i) {
        if (m_threads[i]) {
            m_models << m_threads[i]->Model();
            x << m_model->PrintOutIndependent(m_threads[i]->Index() - 1);
            delete m_threads[i];
        }
    }
    if (table)
        delete table;
    m_results = ToolSet::Model2Parameter(m_models, false);
    ToolSet::Parameter2Statistic(m_results, m_model.data());
    m_controller["x"] = ToolSet::DoubleList2String(x);
    emit AnalyseFinished();
}

void ResampleAnalyse::Interrupt()
{
    emit InterruptAll();
}
