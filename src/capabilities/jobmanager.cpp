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
#include "resampleanalyse.h"
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

    m_resample_handler = new ResampleAnalyse();
    connect(this, SIGNAL(Interrupt()), m_resample_handler, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_resample_handler, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_resample_handler, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);

    m_modelcomparison_handler = new ModelComparison(this);
    connect(this, SIGNAL(Interrupt()), m_modelcomparison_handler, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_modelcomparison_handler, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_modelcomparison_handler, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);

    m_globalsearch = new GlobalSearch(this);
    connect(this, SIGNAL(Interrupt()), m_globalsearch, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_globalsearch, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_globalsearch, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);
}

JobManager::~JobManager()
{
}

void JobManager::RunJobs()
{
    for (const QJsonObject& object : m_jobs) {
        SupraFit::Method method = static_cast<SupraFit::Method>(object["method"].toInt());
        QJsonObject result;
        // qDebug() << object;
        switch (method) {
        case SupraFit::Method::WeakenedGridSearch:
            result = RunGridSearch(object);
            break;

        case SupraFit::Method::ModelComparison:
        case SupraFit::Method::FastConfidence:

            result = RunModelComparison(object);
            break;


        case SupraFit::Method::Reduction:
        case SupraFit::Method::CrossValidation:
            result = RunResample(object);
            break;

        case SupraFit::Method::MonteCarlo:
            result = RunMonteCarlo(object);
            break;

        case SupraFit::Method::GlobalSearch:
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
    m_modelcomparison_handler->Run();

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
    m_montecarlo_handler->Run();
    emit finished();
    QJsonObject result = m_montecarlo_handler->Result();
    m_montecarlo_handler->clear();
    return result;
}

QJsonObject JobManager::RunGridSearch(const QJsonObject& job)
{

    m_gridsearch_handler->setModel(m_model);
    m_gridsearch_handler->setController(job);

    m_gridsearch_handler->Run();
    QJsonObject result = m_gridsearch_handler->Result();
    m_gridsearch_handler->clear();
    return result;
}

QJsonObject JobManager::RunResample(const QJsonObject& job)
{
    m_resample_handler->setModel(m_model);
    m_resample_handler->setController(job);
    m_resample_handler->Run();
    QJsonObject result = m_resample_handler->Result();
    m_resample_handler->clear();
    return result;
}

QJsonObject JobManager::RunGlobalSearch(const QJsonObject& job)
{
    m_globalsearch->setController(job);
    m_globalsearch->setModel(m_model);
    m_globalsearch->Run();

    QJsonObject result = m_globalsearch->Result();
    m_globalsearch->clear();
    return result;
}

#include "jobmanager.moc"
