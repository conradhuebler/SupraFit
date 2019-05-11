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

#include "src/global.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

class AbstractModel;
class GlobalSearch;
class MonteCarloStatistics;
class ModelComparison;
class WeakenedGridSearch;
class ReductionAnalyse;

class JobManager : public QObject {
    Q_OBJECT

public:
    JobManager(QObject* parent = 0);
    ~JobManager();

    inline void setModel(const QSharedPointer<AbstractModel>& model) { m_model = model; }
    inline void AddJob(const QJsonObject& job) { m_jobs << job; }

    void RunJobs();

public slots:

private:
    QSharedPointer<AbstractModel> m_model;
    QList<QJsonObject> m_jobs;

    QJsonObject RunMonteCarlo(const QJsonObject& job);
    QJsonObject RunGridSearch(const QJsonObject& job);
    QJsonObject RunReduction(const QJsonObject& job);
    QJsonObject RunCrossValidation(const QJsonObject& job);
    QJsonObject RunModelComparison(const QJsonObject& job);
    QJsonObject RunFastConfidence(const QJsonObject& job);
    QJsonObject RunGlobalSearch(const QJsonObject& job);

    QPointer<MonteCarloStatistics> m_montecarlo_handler;
    QPointer<WeakenedGridSearch> m_gridsearch_handler;
    QPointer<ModelComparison> m_modelcomparison_handler;
    QPointer<ReductionAnalyse> m_reduction_handler;
    QPointer<GlobalSearch> m_globalsearch;

signals:
    void started();
    void finished();
    void incremented(int t);
    void prepare(int count);
    void ShowResult(SupraFit::Statistic type, int index);
    void Interrupt();
};
