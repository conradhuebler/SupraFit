/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include <QtCore/QDateTime>
#include <QtCore/QMutexLocker>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QThreadPool>
#include <QtCore/QTimer>
#include <QtCore/QVector>

#include <cmath>
#include <iostream>
#include <random>

#include "montecarlostatistics.h"

MonteCarloThread::MonteCarloThread(const MCConfig& config)
    : m_minimizer(QSharedPointer<Minimizer>(new Minimizer(false, this), &QObject::deleteLater))
    , m_config(config)
    , m_finished(false)
{
}

MonteCarloThread::~MonteCarloThread()
{
}

void MonteCarloThread::setDataTable(QPointer<DataTable> table)
{
    m_model->OverrideDependentTable(table);
}

void MonteCarloThread::setIndepTable(QPointer<DataTable> table)
{
    m_model->OverrideInDependentTable(table);
}

void MonteCarloThread::run()
{
    if (!m_model || m_interrupt) {
        qDebug() << "no model set";
        return;
    }
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
#ifdef _DEBUG
//         qDebug() <<  "started!";
#endif
    m_minimizer->setModel(m_model);
    m_finished = m_minimizer->Minimize();

    m_optimized = m_minimizer->Parameter();
    m_model->ImportModel(m_optimized);
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit IncrementProgress(t1 - t0);
#ifdef _DEBUG
//         qDebug() <<  "finished after " << t1-t0 << "msecs!";
#endif
}

MonteCarloBatch::MonteCarloBatch(QPointer<AbstractSearchClass> parent)
    : m_finished(false)
    , m_parent(parent)
    , m_checked(false)
{
}

MonteCarloBatch::~MonteCarloBatch()
{
}

void MonteCarloBatch::run()
{

    m_fit_thread = new NonLinearFitThread(false);

    while (true) {
        if (m_interrupt)
            break;
        QVector<QPair<QPointer<DataTable>, QPointer<DataTable>>> tables = m_parent->DemandCalc();
        int counter = 0;
        for (const QPair<QPointer<DataTable>, QPointer<DataTable>>& table : tables) {
            if (table.first && table.second) {
                m_model->OverrideInDependentTable(table.first);
                if (!m_checked)
                    m_model->OverrideDependentTable(table.second);
                else
                    m_model->OverrideCheckedTable(table.second);

                optimise();
                counter++;
            } else
                continue;
        }
        if (!counter)
            break;
    }
    delete m_fit_thread;
}

void MonteCarloBatch::optimise()
{
    if (!m_model || m_interrupt) {
        qDebug() << "no model set";
        return;
    }
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
#ifdef _DEBUG
//         qDebug() <<  "started!";
#endif

    m_fit_thread->setModel(m_model, false);
    m_fit_thread->run();

    m_finished = m_fit_thread->Converged();

    m_model->ImportModel(m_fit_thread->ConvergedParameter());
    m_model->setFast(false);
    m_model->Calculate();

    m_model->setConverged(m_finished);
    m_models << m_model->ExportModel(false, false); //;

    qint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit IncrementProgress(t1 - t0);
#ifdef _DEBUG
//         qDebug() <<  "finished after " << t1-t0 << "msecs!";
#endif
}

MonteCarloStatistics::MonteCarloStatistics(QObject* parent)
    : AbstractSearchClass(parent)
{
}

MonteCarloStatistics::~MonteCarloStatistics()
{
}

bool MonteCarloStatistics::Evaluate()
{
    m_models.clear();
    QVector<QPointer<MonteCarloBatch>> threads = GenerateData();
    while (m_threadpool->activeThreadCount())
        QCoreApplication::processEvents();

    Collect(threads);
    if (m_models.size() == 0)
        return false;

    m_results = ToolSet::Model2Parameter(m_models);
    ToolSet::Parameter2Statistic(m_results, m_model.data());

    return true;
}

QJsonObject MonteCarloStatistics::Controller() const
{
    /*
    QJsonObject controller;
    controller["steps"] = m_steps;
    controller["variance"] = m_config.variance;
    controller["original"] = m_config.original;
    controller["bootstrap"] = m_config.bootstrap;
    controller["method"] = SupraFit::Statistic::MonteCarlo;
    controller["indep_variance"] = ToolSet::DoubleVec2String(m_config.indep_variance);*/
    return m_controller;
}

QVector<QPointer<MonteCarloBatch>> MonteCarloStatistics::GenerateData()
{
    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    rng.seed(seed);
    Phi = std::normal_distribution<double>(0, m_controller["Variance"].toDouble());

    int maxthreads = qApp->instance()->property("threads").toInt();
    int blocksize = maxthreads / maxthreads / 10;
    if (blocksize < 1)
        blocksize = 1;
    m_threadpool->setMaxThreadCount(maxthreads);
    m_model->Calculate();
    m_table = new DataTable(m_model->ModelTable());
    m_ptr_table << m_table;
    QVector<QPointer<MonteCarloBatch>> threads;
    m_generate = true;
    QVector<qreal> vector = m_model->ErrorTable()->toList();
    Uni = std::uniform_int_distribution<int>(0, vector.size() - 1);

    bool original = m_controller["original"].toBool();
    bool bootstrap = m_controller["bootstrap"].toBool();
    int MaxSteps = m_controller["MaxSteps"].toInt();

    QVector<qreal> indep_variance = ToolSet::String2DoubleVec(m_controller["IndependentRowVariance"].toString());

    QVector<std::normal_distribution<double>> indep_phi;
    for (int i = 0; i < indep_variance.size(); ++i) {
        std::normal_distribution<double> phi = std::normal_distribution<double>(0, indep_variance[i]);
        indep_phi << phi;
    }

#ifdef _DEBUG
    qDebug() << "Starting MC Simulation with" << MaxSteps << "steps";
#endif

    QVector<Pair> block;
    for (int step = 0; step < MaxSteps; ++step) {
        QPointer<DataTable> dep_table;
        if (original)
            dep_table = new DataTable(m_model->DependentModel());
        else
            dep_table = m_model->ModelTable();
        m_ptr_table << dep_table;
        if (bootstrap) {
            QPointer<DataTable> tmp = dep_table->PrepareBootStrap(Uni, rng, vector);
            m_ptr_table << tmp;
            dep_table = tmp;
        } else {
            QPointer<DataTable> tmp = dep_table->PrepareMC(Phi, rng);
            m_ptr_table << tmp;
            dep_table = tmp;
        }

        QPointer<DataTable> indep_table = new DataTable(m_model->IndependentModel());
        m_ptr_table << indep_table;
        for (int i = 0; i < indep_variance.size(); ++i) {
            QVector<int> cols(indep_variance.size(), 0);
            if (indep_variance[i] > 0) {
                cols[i] = 1;
                QPointer<DataTable> tmp = indep_table->PrepareMC(indep_phi[i], rng, cols);
                m_ptr_table << tmp;
                indep_table = tmp;
            }
        }

        block << Pair(indep_table, dep_table);
        if (block.size() == blocksize) {
            m_batch.enqueue(block);
            block.clear();
        }
    }
    for (int i = 0; i < maxthreads; ++i) {
        QPointer<MonteCarloBatch> thread = new MonteCarloBatch(this);
        thread->setChecked(false);
        connect(thread, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)));
        connect(this, &MonteCarloStatistics::InterruptAll, thread, &MonteCarloBatch::Interrupt);
        thread->setModel(m_model);
        threads << thread;
        m_threadpool->start(thread);
    }
    return threads;
}

void MonteCarloStatistics::Collect(const QVector<QPointer<MonteCarloBatch>>& threads)
{
    m_steps = 0;
    for (int i = 0; i < threads.size(); ++i) {
        if (threads[i]) {
            m_models << threads[i]->Models();
            m_steps++;
            delete threads[i];
        }
    }

    for (int i = 0; i < m_ptr_table.size(); ++i)
        if (m_ptr_table[i])
            delete m_ptr_table[i];
}

void MonteCarloStatistics::clear()
{
    m_models.clear();
}

void MonteCarloStatistics::Interrupt()
{
    m_generate = false;
    emit InterruptAll();
}

#include "montecarlostatistics.moc"
