/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2019  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/dataclass.h"
#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/models.h"
#include "src/core/toolset.h"

#include "simulator.h"

Simulator::Simulator()
{
}

Simulator::~Simulator()
{
    if (m_data)
        delete m_data;
}

QStringList Simulator::Generate()
{
    m_extension = ".suprafit";

    QStringList filelist;

    if (m_mainjson.isEmpty())
        return filelist;

    if (m_infile.isEmpty() || m_infile.isNull()) {
        if (m_mainjson.contains("InFile"))
            m_infile = m_mainjson["InFile"].toString();

        if (m_infile.isEmpty() || m_infile.isNull())
            return filelist;
    }

    m_independent_rows = m_mainjson["IndependentRows"].toInt(2);
    m_start_point = m_mainjson["StartPoint"].toInt(0);

    if (!LoadFile())
        return filelist;

    if (m_mainjson.contains("OutFile")) {
        m_outfile = m_mainjson["OutFile"].toString();
    }

    if (m_toplevel.isEmpty())
        return filelist;

    if (m_toplevel.keys().contains("data"))

        m_data = new DataClass(m_toplevel["data"].toObject());
    else
        m_data = new DataClass(m_toplevel);

    if (m_data->DataPoints() == 0) {
        QPointer<DataClass> data = new DataClass(m_data);

        QVector<QSharedPointer<AbstractModel>> models = AddModels(m_modelsjson, data);
        QJsonObject exp_level, dataObject;
        dataObject = data->ExportData();
        exp_level["data"] = dataObject;

        for (int i = 0; i < models.size(); ++i) {
            exp_level["model_" + QString::number(i)] = models[i]->ExportModel();
            models[i].clear();
        }
        SaveFile(m_outfile, exp_level);
        return filelist;
    }

    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng;
    rng.seed(seed);

    int runs = m_mainjson["MaxSteps"].toInt();
    double std = m_mainjson["Variance"].toDouble();

    if (qFuzzyCompare(std, 0))
        runs = 1;

    if (m_mainjson.contains("Threads")) {
        qApp->instance()->setProperty("threads", m_mainjson.value("Threads").toInt(4));
        std::cout << "Setting # of threads to " << m_mainjson.value("Threads").toInt(4) << std::endl;
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
        if (!m_modelsjson.isEmpty()) {
            std::cout << "Loading Models into Dataset" << std::endl;
            QVector<QSharedPointer<AbstractModel>> models = AddModels(m_modelsjson, data);
            std::cout << "Loading " << models.size() << " Models into Dataset finished!" << std::endl;
            if (!m_jobsjson.isEmpty()) {
                std::cout << "Starting jobs ..." << std::endl;
                JobManager* manager = new JobManager;
                connect(manager, &JobManager::finished, this, [](int current, int all, int time) {
                    std::cout << "another job done: " << current << " of " << all << " after " << time << " msecs." << std::endl;
                });
                connect(this, &Simulator::Interrupt, manager, &JobManager::Interrupt);
                for (int model_index = 0; model_index < models.size(); ++model_index) {
                    std::cout << "... model  " << model_index << std::endl;
                    for (const QString& j : m_jobsjson.keys()) {
                        QJsonObject job = m_jobsjson[j].toObject();
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

QJsonObject Simulator::PerfomeJobs(const QJsonObject& data, const QJsonObject& models, const QJsonObject& job)
{
    if (m_mainjson.contains("Threads")) {
        qApp->instance()->setProperty("threads", m_mainjson.value("Threads").toInt(4));
        std::cout << "Setting # of threads to " << m_mainjson.value("Threads").toInt(4) << std::endl;
    }

    QJsonObject project = data;

    QPointer<DataClass> d = new DataClass(data["data"].toObject());

    if (!models.isEmpty()) {
        std::cout << "Loading Models into Dataset" << std::endl;
        QVector<QSharedPointer<AbstractModel>> m = AddModels(models, d);
        std::cout << "Loading " << m.size() << " Models into Dataset finished!" << std::endl;
        if (!m_jobsjson.isEmpty()) {
            std::cout << "Starting jobs ..." << std::endl;
            JobManager* manager = new JobManager;
            connect(manager, &JobManager::finished, this, [](int current, int all, int time) {
                std::cout << "another job done: " << current << " of " << all << " after " << time << " msecs." << std::endl;
            });
            connect(this, &Simulator::Interrupt, manager, &JobManager::Interrupt);
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
            m[i].clear();
        }
        m.clear();
    }
    int file_int = 0;
    QString outfile = QString(m_outfile + "_" + QString::number(file_int) + m_extension);
    while (QFileInfo::exists(outfile)) {
        ++file_int;
        outfile = QString(m_outfile + "_" + QString::number(file_int) + m_extension);
    }

    SaveFile(outfile, project);
    return project;
}

QVector<QSharedPointer<AbstractModel>> Simulator::AddModels(const QJsonObject& modelsjson, QPointer<DataClass> data)
{
    QVector<QSharedPointer<AbstractModel>> models;
    for (const QString& str : modelsjson.keys()) {
        SupraFit::Model model;
        QJsonObject options;
        if (modelsjson[str].toObject().contains("model")) {
            model = static_cast<SupraFit::Model>(modelsjson[str].toObject()["model"].toInt());
            options = modelsjson[str].toObject()["options"].toObject();
        } else
            model = static_cast<SupraFit::Model>(modelsjson[str].toInt());

        QSharedPointer<AbstractModel> t = CreateModel(model, data);

        if (!t)
            continue;

        if (!options.isEmpty())
            t->setOptions(options);

        if (m_mainjson.contains("Guess") && m_mainjson["Guess"].toBool()) {
            t->InitialGuess();

            if (m_mainjson.contains("Fit") && m_mainjson["Fit"].toBool()) {
                QPointer<MonteCarloThread> thread = new MonteCarloThread();
                thread->setModel(t);
                thread->run();
                t->ImportModel(thread->Model());
                t->setFast(false);
                t->Calculate();
                delete thread;
            }
        }
        models << t;
    }
    return models;
}
QSharedPointer<AbstractModel> Simulator::AddModel(int model, QPointer<DataClass> data)
{
    QSharedPointer<AbstractModel> t = CreateModel(model, data);
    if (!t)
        return t;

    return t;
}
QVector<QJsonObject> Simulator::GenerateData()
{
    m_extension = ".suprafit";

    QVector<QJsonObject> project_list;

    if (m_mainjson.isEmpty())
        return project_list;

    if (m_infile.isEmpty() || m_infile.isNull()) {
        if (m_mainjson.contains("InFile"))
            m_infile = m_mainjson["InFile"].toString();

        if (m_infile.isEmpty() || m_infile.isNull())
            return project_list;
    }

    m_independent_rows = m_mainjson["IndependentRows"].toInt(2);
    m_start_point = m_mainjson["StartPoint"].toInt(0);
    m_series = m_datajson["Series"].toInt();

    if (!LoadFile())
        return project_list;

    if (m_mainjson.contains("OutFile")) {
        m_outfile = m_mainjson["OutFile"].toString();
    }

    if (m_toplevel.isEmpty())
        return project_list;

    if (m_toplevel.keys().contains("data"))
        m_data = new DataClass(m_toplevel["data"].toObject());
    else
        m_data = new DataClass(m_toplevel);

    int model = m_datajson["model"].toInt();
    double variance = m_datajson["Variance"].toDouble();
    int repeat = m_datajson["Repeat"].toInt();

    QPointer<DataClass> data = new DataClass(m_data.data());
    QJsonObject export_object = data->ExportData();
    QSharedPointer<AbstractModel> t = AddModel(model, data);
    QVector<qreal> random;

    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng(seed);
    int file_int = 0;
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
