/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2020  Conrad Hübler <Conrad.Huebler@gmx.net>
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

        /*
        std::cout << "********************************************************************************************************" << std::endl;
        std::cout << "************************    Model analysis starts right now       **************************************" << std::endl;
        std::cout << "********************************************************************************************************" << std::endl
                  << std::endl;
        std::cout << "                         " << model->Name().toStdString() << std::endl;
        */
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


QVector<QJsonObject> Simulator::GenerateData()
{
    m_extension = ".suprafit";

    QVector<QJsonObject> project_list;

    if (m_main.isEmpty())
        return project_list;

    if (m_infile.isEmpty() || m_infile.isNull()) {
        if (m_main.contains("InFile"))
            m_infile = m_main["InFile"].toString();

        if (m_infile.isEmpty() || m_infile.isNull())
            return project_list;
    }

    m_independent_rows = m_main["IndependentRows"].toInt(2);
    m_start_point = m_main["StartPoint"].toInt(0);
    m_series = m_data_json["Series"].toInt();

    if (!LoadFile())
        return project_list;

    if (m_main.contains("OutFile")) {
        m_outfile = m_main["OutFile"].toString();
    }

    if (m_toplevel.isEmpty())
        return project_list;

    if (m_toplevel.keys().contains("data"))
        m_data = new DataClass(m_toplevel["data"].toObject());
    else
        m_data = new DataClass(m_toplevel);

    int model = m_data_json["model"].toInt();
    double variance = m_data_json["Variance"].toDouble();
    int repeat = m_data_json["Repeat"].toInt();

    QPointer<DataClass> data = new DataClass(m_data.data());
    QJsonObject export_object = data->ExportData();
    QSharedPointer<AbstractModel> t = AddModel(model, data);
    QVector<qreal> random;

    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng(seed);
    int file_int = 0;

    QString global_limits, local_limits;
    if (m_data_json.contains("GlobalRandomLimits")) {
        global_limits = m_data_json["GlobalRandomLimits"].toString();
        QStringList limits = global_limits.split(";");
        if (limits.size()) {
            QVector<QPair<qreal, qreal>> global_block;

            if (limits.size() == t->GlobalParameterSize()) {
                for (int i = 0; i < limits.size(); ++i) {
                    global_block << ToolSet::QString2QPair(limits[i]);
                }
            } else {
                const QPair<qreal, qreal> pair = ToolSet::QString2QPair(limits.first());
                global_block = QVector<QPair<qreal, qreal>>(t->GlobalParameterSize(), pair);
            }

            t->setGlobalRandom(global_block);
        }
    }

    if (m_data_json.contains("LocalRandomLimits")) {
        local_limits = m_data_json["LocalRandomLimits"].toString();

        QStringList local_limits_block = local_limits.split(":");
        if (local_limits_block.size()) {
            if (local_limits_block.size() == m_series) {
                for (int j = 0; j < m_series; ++j) {
                    QStringList limits = local_limits_block[j].split(";");
                    if (limits.size()) {
                        QVector<QPair<qreal, qreal>> local_block;

                        if (limits.size() == t->GlobalParameterSize()) {
                            for (int i = 0; i < limits.size(); ++i) {
                                local_block << ToolSet::QString2QPair(limits[i]);
                            }
                        } else {
                            const QPair<qreal, qreal> pair = ToolSet::QString2QPair(limits.first());
                            local_block = QVector<QPair<qreal, qreal>>(t->LocalParameterSize(), pair);
                        }

                        t->setLocalRandom(local_block, j);
                    }
                }
            } else {
                QStringList local_limits_block = local_limits.split(":");
                QStringList limits = local_limits_block.first().split(";");
                const QPair<qreal, qreal> pair = ToolSet::QString2QPair(limits.first());

                for (int j = 0; j < m_series; ++j) {
                    t->setLocalRandom(QVector<QPair<qreal, qreal>>(t->LocalParameterSize(), pair), j);
                }
            }
        }
    }
    for (int i = 0; i < repeat; ++i) {
        t->InitialGuess();

        export_object["dependent"] = t->ModelTable()->PrepareMC(QVector<double>() << variance, rng)->ExportTable(true);
        export_object["DataType"] = 1;
        export_object["content"] = t->Model2Text();
        QJsonObject blob;
        blob["data"] = export_object;
        project_list << blob;
        /*
        QString outfile = QString(m_outfile + "_" + QString::number(file_int) + m_extension);
        while (QFileInfo::exists(outfile)) {
            ++file_int;
            outfile = QString(m_outfile + "_" + QString::number(file_int) + m_extension);
        }

        SaveFile(outfile, blob);*/
    }

    return project_list;
}

void Simulator::CheckStopFile()
{
    if (QFileInfo::exists("stop")) {
        m_interrupt = true;
        emit Interrupt();
        QFile file("stop");
        file.remove();
    } else
        QTimer::singleShot(100, this, &Simulator::CheckStopFile);
}
