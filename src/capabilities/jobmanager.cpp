/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2019 - 2021 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "landscapesearch.h"
#include "modelcomparison.h"
#include "montecarlostatistics.h"
#include "resampleanalyse.h"
#include "weakenedgridsearch.h"

#include "src/core/toolset.h"
#include "src/core/models/meta_model.h"
#include "src/core/models/models.h"

#include "src/global.h"

#include <QtCore/QDateTime>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QObject>
#include <QtCore/QSet>

#include "src/core/phasetiming.h"

#include "jobmanager.h"

namespace {
// the managers that run jobs now (Claude Generated): asked by ProjectManager::replaceProjectData
QMutex s_runningMutex;
QSet<const JobManager*> s_running;
}

bool JobManager::hasRunningJobs(const QString& projectId)
{
    QMutexLocker locker(&s_runningMutex);
    for (const JobManager* manager : std::as_const(s_running)) {
        const QSharedPointer<AbstractModel> model = manager->m_model;
        if (!model)
            continue;
        if (model->UUID() == projectId)
            return true;
        if (const MetaModel* meta = qobject_cast<const MetaModel*>(model.data()))
            for (const QSharedPointer<AbstractModel>& m : meta->Models())
                if (m && m->UUID() == projectId)
                    return true;
    }
    return false;
}

JobManager::JobManager(QObject* parent)
    : QObject(parent)
{
    m_montecarlo_handler = new MonteCarloStatistics(this);
    connect(this, SIGNAL(Interrupt()), m_montecarlo_handler, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_montecarlo_handler, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_montecarlo_handler, &AbstractSearchClass::Message, this, &JobManager::Message);
    connect(m_montecarlo_handler, &AbstractSearchClass::Stage, this, &JobManager::Stage);
    connect(m_montecarlo_handler, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);

    m_gridsearch_handler = new WeakenedGridSearch(this);
    connect(this, SIGNAL(Interrupt()), m_gridsearch_handler, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_gridsearch_handler, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_gridsearch_handler, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);
    connect(m_gridsearch_handler, &AbstractSearchClass::Message, this, &JobManager::Message);
    connect(m_gridsearch_handler, &AbstractSearchClass::Stage, this, &JobManager::Stage);

    m_resample_handler = new ResampleAnalyse();
    connect(this, SIGNAL(Interrupt()), m_resample_handler, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_resample_handler, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_resample_handler, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);
    connect(m_resample_handler, &AbstractSearchClass::Message, this, &JobManager::Message);
    connect(m_resample_handler, &AbstractSearchClass::Stage, this, &JobManager::Stage);

    m_modelcomparison_handler = new ModelComparison(this);
    connect(this, SIGNAL(Interrupt()), m_modelcomparison_handler, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_modelcomparison_handler, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_modelcomparison_handler, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);
    connect(m_modelcomparison_handler, &AbstractSearchClass::Message, this, &JobManager::Message);
    connect(m_modelcomparison_handler, &AbstractSearchClass::Stage, this, &JobManager::Stage);

    m_globalsearch = new GlobalSearch(this);
    connect(this, SIGNAL(Interrupt()), m_globalsearch, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_globalsearch, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_globalsearch, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);
    connect(m_globalsearch, &AbstractSearchClass::Message, this, &JobManager::Message);
    connect(m_globalsearch, &AbstractSearchClass::Stage, this, &JobManager::Stage);

    m_landscape = new LandscapeSearch(this);
    connect(this, SIGNAL(Interrupt()), m_landscape, SLOT(Interrupt()), Qt::DirectConnection);
    connect(m_landscape, SIGNAL(IncrementProgress(int)), this, SIGNAL(incremented(int)), Qt::DirectConnection);
    connect(m_landscape, SIGNAL(setMaximumSteps(int)), this, SIGNAL(prepare(int)), Qt::DirectConnection);
    connect(m_landscape, &AbstractSearchClass::Message, this, &JobManager::Message);
    connect(m_landscape, &AbstractSearchClass::Stage, this, &JobManager::Stage);

    connect(this, &JobManager::Interrupt, this, [this]() {
        m_interrupt = true;
    });
}

JobManager::~JobManager()
{
    QMutexLocker locker(&s_runningMutex);
    s_running.remove(this);
}

void JobManager::RunJobs()
{
    m_working = true;
    {
        QMutexLocker locker(&s_runningMutex);
        s_running.insert(this);
    }
    m_interrupt = false;
    int start = 0;
    int threads = qApp->instance()->property("threads").toInt();
    if (m_model->PreventThreads())
        qApp->instance()->setProperty("threads", 1);
    for (const QJsonObject& object : m_jobs) {
        SupraFit::Method method = static_cast<SupraFit::Method>(object["Method"].toInt());
        QJsonObject result;

        qint64 t0 = 0, t1 = 0;
        emit started();
        t0 = QDateTime::currentMSecsSinceEpoch();

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

        case SupraFit::Method::LandscapeSampling:
            result = RunLandscapeSearch(object);
            break;
        }
        if (result.isEmpty()) { // nothing to store, e.g. a model the method cannot handle (the reason went out as a Message)
            start++;
            emit finished(start, m_jobs.size(), 0);
            if (m_interrupt)
                break;
            continue;
        }
        start++;
        t1 = QDateTime::currentMSecsSinceEpoch();
        const QString final_message = QString("%1 took %2 msecs (%3 msecs in parallel processes) for %4 in %5").arg(Method2Name(method)).arg(t1 - t0).arg(m_last_multicore).arg(m_model->Name()).arg(m_model->ProjectTitle());
        emit m_model->Info()->Message(final_message);
        emit Message(final_message);

        // NOTE: finished() hides the progress dialog, but the two calls below still run — so from
        // here on the user gets no feedback at all. Timed separately for exactly that reason. CG.
        emit finished(start, m_jobs.size(), t1 - t0);
        int index = m_model->UpdateStatistic(result);
        PhaseTiming::Mark(QStringLiteral("store statistic in model (after progress hidden)"));
        emit ShowResult(method, index);
        if (m_interrupt)
            break;
    }
    m_jobs.clear();
    m_working = false;
    {
        QMutexLocker locker(&s_runningMutex);
        s_running.remove(this);
    }
    qApp->instance()->setProperty("threads", threads);
    emit AllFinished();
}

void JobManager::AddSingleJob(const QJsonObject& job)
{
    if (!job.contains("Repeat"))
        m_jobs << job;
    else {
        int repeat = job["Repeat"].toInt();
        for (int i = 0; i < repeat; ++i)
            m_jobs << job;
    }
}

/*! \brief A job that names no parameters (the CLI, agents) tests all of them: the lists of the global and the local
 * parameters (one entry per local parameter and series) are filled with 1 where they are missing. Claude Generated. */
void JobManager::AllParametersIfUnset(QJsonObject& block) const
{
    if (!m_model)
        return;
    auto all = [](int count) { return ToolSet::IntVec2String(QVector<int>(count, 1)); };
    if (block["GlobalParameterList"].toString().isEmpty())
        block["GlobalParameterList"] = all(m_model->GlobalParameterSize());
    if (block["LocalParameterList"].toString().isEmpty())
        block["LocalParameterList"] = all(m_model->LocalParameterSize() * m_model->SeriesCount());
}

QJsonObject JobManager::RunModelComparison(const QJsonObject& job)
{
    QJsonObject block = QJsonObject(ModelComparisonConfigBlock);

    for (const QString& key : job.keys())
        block[key] = job[key];
    AllParametersIfUnset(block);

    m_modelcomparison_handler->setModel(m_model);
    m_modelcomparison_handler->setController(block);
    m_modelcomparison_handler->Run();

    QJsonObject result = m_modelcomparison_handler->Result();
    m_last_multicore = m_modelcomparison_handler->MultiCoreTime();

    m_modelcomparison_handler->clear();

    return result;
}

QJsonObject JobManager::RunMonteCarlo(const QJsonObject& job)
{
    QJsonObject block = QJsonObject(MonteCarloConfigBlock);

    for (const QString& key : job.keys())
        block[key] = job[key];

    m_montecarlo_handler->setController(block);

    m_montecarlo_handler->setModel(m_model);
    m_montecarlo_handler->Run();

    QJsonObject result = m_montecarlo_handler->Result();
    m_last_multicore = m_montecarlo_handler->MultiCoreTime();
    m_montecarlo_handler->clear();
    return result;
}

QJsonObject JobManager::RunGridSearch(const QJsonObject& job)
{
    QJsonObject block = QJsonObject(GridSearchConfigBlock);

    for (const QString& key : job.keys())
        block[key] = job[key];
    AllParametersIfUnset(block);

    m_gridsearch_handler->setModel(m_model);
    m_gridsearch_handler->setController(block);
    m_gridsearch_handler->Run();

    QJsonObject result = m_gridsearch_handler->Result();
    m_last_multicore = m_gridsearch_handler->MultiCoreTime();

    m_gridsearch_handler->clear();
    return result;
}

QJsonObject JobManager::RunResample(const QJsonObject& job)
{
    QJsonObject block = QJsonObject(ResampleConfigBlock);

    for (const QString& key : job.keys())
        block[key] = job[key];

    m_resample_handler->setModel(m_model);
    m_resample_handler->setController(block);
    m_resample_handler->Run();

    QJsonObject result = m_resample_handler->Result();
    m_last_multicore = m_resample_handler->MultiCoreTime();

    m_resample_handler->clear();
    return result;
}

QJsonObject JobManager::RunGlobalSearch(const QJsonObject& job)
{
    m_globalsearch->setController(job);
    m_globalsearch->setModel(m_model);
    m_globalsearch->Run();

    QJsonObject result = m_globalsearch->Result();
    m_last_multicore = m_globalsearch->MultiCoreTime();

    m_globalsearch->clear();
    return result;
}

QJsonObject JobManager::RunLandscapeSearch(const QJsonObject& job)
{
    m_landscape->setController(job);
    m_landscape->setModel(m_model);
    const bool ok = m_landscape->Run();

    QJsonObject result = ok ? m_landscape->Result() : QJsonObject();
    m_last_multicore = m_landscape->MultiCoreTime();

    m_landscape->clear();
    return result;
}

#include "jobmanager.moc"
