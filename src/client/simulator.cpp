/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2021  Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#include <random>

#include <QCoreApplication>

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>

#include "src/capabilities/jobmanager.h"
#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/resampleanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"

#include "src/core/analyse.h"
#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/toolset.h"

#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"

#include "simulator.h"

Simulator::Simulator()
{
}

Simulator::Simulator(SupraFitCli* client)
    : SupraFitCli(client)
{
}

Simulator::~Simulator()
{
    if (m_data)
        delete m_data;
}
/*
QStringList Simulator::Generate()
{
    QStringList filelist;

    // m_extension = ".suprafit";
    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng;
    rng.seed(seed);

    int runs = m_main["MaxSteps"].toInt();
    double std = m_main["Variance"].toDouble();

    if (qFuzzyCompare(std, 0))
        runs = 1;

    if (m_main.contains("Threads")) {
        qApp->instance()->setProperty("threads", m_main.value("Threads").toInt(4));
        std::cout << "Setting # of threads to " << m_main.value("Threads").toInt(4) << std::endl;
    }

    QJsonObject table = m_data->DependentModel()->ExportTable(true);
    int file_int = 0;
    CheckStopFile();

    for (int i = 0; i < runs; ++i) {
        std::cout << "########################################################################################################" << std::endl;
        std::cout << "########################################################################################################" << std::endl
                  << std::endl;
        std::cout << "#####################  Starting run number " << i + 1 << " #######################################################" << std::endl
                  << std::endl;
        std::cout << "Generating new Data Table for Monte Carlo Simulation" << std::endl;
        std::normal_distribution<double> Phi = std::normal_distribution<double>(0, std);
        m_data->DependentModel()->ImportTable(table);
#ifdef _DEBUG
        // model_table->Debug();
        m_data->DependentModel()->Debug();
#endif
        QPointer<DataClass> data = new DataClass(m_data);
        if (!qFuzzyCompare(std, 0)) {
            QPointer<DataTable> model_table = m_data->DependentModel()->PrepareMC(Phi, rng);
            data->setDependentTable(model_table);
        }
        data->NewUUID();
#ifdef _DEBUG
        data->IndependentModel()->Debug();
        data->DependentModel()->Debug();
#endif
        QJsonObject exp_level, dataObject;
        dataObject = data->ExportData();
        exp_level["data"] = dataObject;


        if (!m_main.isEmpty()) {
            std::cout << "Loading Models into Dataset" << std::endl;
            QVector<QSharedPointer<AbstractModel>> models = AddModels(m_main, data);
            std::cout << "Loading " << models.size() << " Models into Dataset finished!" << std::endl;
            if (!m_jobs.isEmpty()) {
                std::cout << "Starting jobs ..." << std::endl;
                JobManager* manager = new JobManager;
                connect(manager, &JobManager::Message, this, [](const QString& str) {
                    std::cout << str.toStdString() << std::endl;
                });
                connect(manager, &JobManager::finished, this, [](int current, int all, int time) {
                    std::cout << "another job done: " << current << " of " << all << " after " << time << " msecs." << std::endl;
                });

                connect(this, &Simulator::Interrupt, manager, &JobManager::Interrupt);
                for (int model_index = 0; model_index < models.size(); ++model_index) {
                    std::cout << "... model  " << model_index << std::endl;
                    for (const QString& j : m_jobs.keys()) {
                        QJsonObject job = m_jobs[j].toObject();
                        manager->setModel(models[model_index]);
                        manager->AddJob(job);
                        manager->RunJobs();
                        std::cout << "... model  " << model_index << " job done!" << std::endl;
                    }
                    std::cout << "jobs for model  " << model_index << "  finished!" << std::endl;
                    if (m_interrupt) {
                        std::cout << "softly interrupted by stop file" << std::endl;
                        break;
                    }
                }
                delete manager;
                std::cout << "jobs all done!" << std::endl;
            }

            for (int i = 0; i < models.size(); ++i) {
                exp_level["model_" + QString::number(i)] = models[i]->ExportModel();
                models[i].clear();
            }
            models.clear();
        }

        QString outfile = QString(m_outfile + "_" + QString::number(file_int) + m_extension);
        while (QFileInfo::exists(outfile)) {
            ++file_int;
            outfile = QString(m_outfile + "_" + QString::number(file_int) + m_extension);
        }

        if (SaveFile(outfile, exp_level))
            filelist << outfile;

        if (m_interrupt) {
            break;
        }
    }
    return filelist;
}
*/

QJsonObject Simulator::PerformeJobs(const QJsonObject& data, const QJsonObject& models, const QJsonObject& job)
{
    if (m_main.contains("Threads")) {
        qApp->instance()->setProperty("threads", m_main.value("Threads").toInt(4));
        std::cout << "Setting # of threads to " << m_main.value("Threads").toInt(4) << std::endl;
    }

    QJsonObject project;
    if (data.contains("data"))
        project = data;
    else
        project["data"] = data;

    QPointer<DataClass> d;

    if (data.contains("data"))
        d = new DataClass(data["data"].toObject());
    else
        d = new DataClass(data);

    QVector<QJsonObject> models_json;
    if (!models.isEmpty()) {
        std::cout << "Loading Models into Dataset" << std::endl;
        QVector<QSharedPointer<AbstractModel>> m = AddModels(models, d);
        std::cout << "Loading " << m.size() << " Models into Dataset finished!" << std::endl;
        if (!m_jobs.isEmpty()) {
            std::cout << "Starting jobs ..." << std::endl;
            JobManager* manager = new JobManager;
            connect(manager, &JobManager::Message, this, [](const QString& str) {
                std::cout << str.toStdString() << std::endl;
            });
            connect(manager, &JobManager::finished, this, [](int current, int all, int time) {
                std::cout << "another job done: " << current << " of " << all << " after " << time << " msecs." << std::endl;
            });
            //connect(this, &Simulator::Interrupt, manager, &JobManager::Interrupt);
            for (int model_index = 0; model_index < models.size(); ++model_index) {
                std::cout << "... model  " << model_index << std::endl;
                for (const QString& j : job.keys()) {
                    QJsonObject single_job = job[j].toObject();
                    manager->setModel(m[model_index]);
                    manager->AddJob(single_job);
                    manager->RunJobs();
                    std::cout << "... model  " << model_index << " job done!" << std::endl;

                    if (m_interrupt) {
                        break;
                    }
                }
                std::cout << "jobs for model  " << model_index << "  finished!" << std::endl;
                if (m_interrupt) {
                    std::cout << "softly interrupted by stop file" << std::endl;
                    break;
                }
            }
            delete manager;
            std::cout << "jobs all done!" << std::endl;
        }

        for (int i = 0; i < m.size(); ++i) {
            project["model_" + QString::number(i)] = m[i]->ExportModel();
            models_json << m[i]->ExportModel();
            m[i].clear();
        }
        m.clear();
    }
    int file_int = 0;
    QString outfile = QString(m_outfile + "_" + QString::number(file_int) + "." + m_extension);
    while (QFileInfo::exists(outfile)) {
        ++file_int;
        outfile = QString(m_outfile + "_" + QString::number(file_int) + "." + m_extension);
    }
    Analyse(m_analyse, models_json);
    SaveFile(outfile, project);
    return project;
}
