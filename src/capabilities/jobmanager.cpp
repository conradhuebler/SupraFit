/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "modelcomparison.h"
#include "montecarlostatistics.h"
#include "reductionanalyse.h"
#include "weakenedgridsearch.h"

#include "src/core/models.h"

#include "src/global.h"

#include <QtCore/QObject>

#include "jobmanager.h"

JobManager::JobManager(QObject* parent)
    : QObject(parent)
{
    m_montecarlo_handler = new MonteCarloStatistics(this);
    connect(this, SIGNAL(Interrupt()), m_montecarlo_handler, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_montecarlo_handler, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);

    m_gridsearch_handler = new WeakenedGridSearch(this);
    connect(this, SIGNAL(Interrupt()), m_gridsearch_handler, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_gridsearch_handler, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_gridsearch_handler, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);

    m_reduction_handler = new ReductionAnalyse();
    connect(this, SIGNAL(Interrupt()), m_reduction_handler, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_reduction_handler, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_reduction_handler, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);

    m_modelcomparison_handler = new ModelComparison(this);
    connect(this, SIGNAL(Interrupt()), m_modelcomparison_handler, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_modelcomparison_handler, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_modelcomparison_handler, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);
}

JobManager::~JobManager()
{
}

void JobManager::RunJobs()
{
    for (const QJsonObject& object : m_jobs) {
        qDebug() << object;
        SupraFit::Statistic method = static_cast<SupraFit::Statistic>(object["method"].toInt());
        QJsonObject result;

        switch (method) {
        case SupraFit::Statistic::WeakenedGridSearch:
            result = RunGridSearch(object);
            break;

        case SupraFit::Statistic::ModelComparison:
            result = RunModelComparison(object);
            break;

        case SupraFit::Statistic::FastConfidence:
            result = RunFastConfidence(object);
            break;

        case SupraFit::Statistic::Reduction:
            result = RunReduction(object);
            break;

        case SupraFit::Statistic::MonteCarlo:
            result = RunMonteCarlo(object);
            break;

        case SupraFit::Statistic::CrossValidation:
            result = RunCrossValidation(object);
            break;

        case SupraFit::Statistic::GlobalSearch:
            result = RunGlobalSearch(object);
            break;
        }
        int index = m_model->UpdateStatistic(result);
        emit ShowResult(method, index);
    }
    m_jobs.clear();
}

QJsonObject JobManager::RunModelComparison(const QJsonObject& job)
{
    m_modelcomparison_handler->setModel(m_model);
    m_modelcomparison_handler->setController(job);
    m_modelcomparison_handler->Confidence();

    QJsonObject result = m_modelcomparison_handler->Result();
    m_modelcomparison_handler->clear();

    return result;
}

QJsonObject JobManager::RunMonteCarlo(const QJsonObject& job)
{
    int MaxSteps = job["MaxSteps"].toInt();
    emit prepare(MaxSteps);
    m_montecarlo_handler->setController(job);

    m_montecarlo_handler->setModel(m_model);
    m_montecarlo_handler->Evaluate();
    emit finished();
    QJsonObject result = m_montecarlo_handler->Result();
    m_montecarlo_handler->clear();
    return result;
}

QJsonObject JobManager::RunCrossValidation(const QJsonObject& job)
{
    m_reduction_handler->setModel(m_model);
    m_reduction_handler->setController(job);
    m_reduction_handler->CrossValidation();

    QJsonObject result = m_reduction_handler->Result();
    m_reduction_handler->clear();
    return result;
}

QJsonObject JobManager::RunGridSearch(const QJsonObject& job)
{

    m_gridsearch_handler->setModel(m_model);
    m_gridsearch_handler->setController(job);

    m_gridsearch_handler->ConfidenceAssesment();
    QJsonObject result = m_gridsearch_handler->Result();
    m_gridsearch_handler->clear();
    return result;
}

QJsonObject JobManager::RunFastConfidence(const QJsonObject& job)
{
    m_modelcomparison_handler->setModel(m_model);
    m_modelcomparison_handler->setController(job);
    m_modelcomparison_handler->FastConfidence();

    QJsonObject result = m_modelcomparison_handler->Result();
    m_modelcomparison_handler->clear();

    return result;
}

QJsonObject JobManager::RunReduction(const QJsonObject& job)
{
    m_reduction_handler->setModel(m_model);
    m_reduction_handler->PlainReduction();
    QJsonObject result = m_reduction_handler->Result();
    m_reduction_handler->clear();
    return result;
}

QJsonObject JobManager::RunGlobalSearch(const QJsonObject& job)
{
}

#include "jobmanager.moc"
