/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include <QtCore/QMutexLocker>

#include <QtWidgets/QApplication>

#include "src/core/AbstractModel.h"
#include "src/core/minimizer.h"
#include "src/core/toolset.h"

#include "src/capabilities/montecarlostatistics.h"

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
    if (static_cast<SupraFit::Method>(m_controller["method"].toInt()) == SupraFit::Method::Reduction) {
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
    QVector<Pair> block;
    int blocksize;
    int maxthreads = qApp->instance()->property("threads").toInt();
    QPointer<DataTable> table = new DataTable(m_model->DependentModel());
    QVector<QPointer<MonteCarloBatch>> threads;

    switch (type) {
    case 1:
        emit setMaximumSteps(m_model->DataPoints());
        blocksize = 1;
        for (int i = m_model->DataPoints() - 1; i >= 0; --i) {
            QPointer<DataTable> dep_table = new DataTable(table);
            dep_table->DisableRow(i);

            block << Pair(m_model->IndependentModel(), dep_table);
            if (block.size() == blocksize) {
                m_batch.enqueue(block);
                block.clear();
            }
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        break;
    case 2:
        emit setMaximumSteps(m_model->DataPoints() * (m_model->DataPoints() - 1) / 2);
        blocksize = 25;
        for (int i = 0; i < m_model->DataPoints(); ++i)
            for (int j = i + 1; j < m_model->DataPoints(); ++j) {
                QPointer<DataTable> dep_table = new DataTable(table);
                dep_table->DisableRow(i);
                dep_table->DisableRow(j);
                block << Pair(m_model->IndependentModel(), dep_table);
                if (block.size() == blocksize) {
                    m_batch.enqueue(block);
                    block.clear();
                }
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            }
        break;

        // case CVType::LeaveManyOut:
        // This will come sometimes

        //     break;
    }
    qDebug() << m_batch.size();
    //emit setMaximumSteps(m_batch.size());

    for (int i = 0; i < maxthreads; ++i) {
        QPointer<MonteCarloBatch> thread = new MonteCarloBatch(this);
        thread->setChecked(true);
        connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
        connect(this, &ResampleAnalyse::InterruptAll, thread, &MonteCarloBatch::Interrupt);
        thread->setModel(m_model);
        threads << thread;
        m_threadpool->start(thread);
    }

    while (Pending()) {
        QApplication::processEvents();
    }
    for (int i = 0; i < threads.size(); ++i) {
        if (threads[i]) {
            m_models << threads[i]->Models();
            delete threads[i];
        }
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
    m_controller["cutoff"] = m_model.data()->ReductionCutOff();
    int maxthreads = qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);
    QPointer<DataTable> table = m_model->DependentModel();
    emit setMaximumSteps(m_model->DataPoints() - 4);

    if (m_controller["ReductionRuntype"].toInt() == 1 || m_controller["ReductionRuntype"].toInt() == 3) {
        for (int i = m_model->DataPoints() - 1; i > 3; --i) {
            QPointer<MonteCarloThread> thread = new MonteCarloThread();
            connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
            thread->setIndex(i);
            QSharedPointer<AbstractModel> model = m_model->Clone();
            table->DisableRow(i);
            model->setDependentTable(table);
            model->detach();
            thread->setModel(model);
            addThread(thread);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    } else if (m_controller["ReductionRuntype"].toInt() == 2 || m_controller["runtype"].toInt() == 3) {
        for (int i = 1; i < m_model->DataPoints() - 3; ++i) {
            QPointer<MonteCarloThread> thread = new MonteCarloThread();
            connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
            thread->setIndex(i);
            QSharedPointer<AbstractModel> model = m_model->Clone();
            table->DisableRow(i);
            model->setDependentTable(table);
            model->detach();
            thread->setModel(model);
            addThread(thread);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }

    while (Pending()) {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
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

void ResampleAnalyse::clear()
{
    m_models.clear();
    m_model.clear();
}
