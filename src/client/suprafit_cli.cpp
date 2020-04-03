/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <iostream>

#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>
#include <QtCore/QThreadPool>

#include "src/capabilities/jobmanager.h"
#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/resampleanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"

#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"

#include "src/core/analyse.h"
#include "src/core/filehandler.h"
#include "src/core/jsonhandler.h"
#include "src/core/toolset.h"

#include "suprafit_cli.h"

SupraFitCli::SupraFitCli()
{
}

SupraFitCli::SupraFitCli(SupraFitCli* core)
{
    m_infile = core->m_infile;
    m_outfile = core->m_outfile;
    m_extension = core->m_extension;
    m_main = core->m_main;
    m_models = core->m_models;
    m_jobs = core->m_jobs;
    m_analyse = core->m_analyse;
    m_prepare = core->m_prepare;
    m_simulation = core->m_simulation;
    m_data = core->m_data;
    m_data_json = core->m_data_json;
    m_toplevel = core->m_toplevel;
    m_data = new DataClass(core->m_data);

    ParseMain();
}

SupraFitCli::~SupraFitCli()
{
}

void SupraFitCli::setControlJson(const QJsonObject& control)
{
    QStringList keys = control.keys();
    for (const QString& key : keys) {
        if (key.compare("main", Qt::CaseInsensitive) == 0)
            m_main = control[key].toObject();

        if (key.compare("models", Qt::CaseInsensitive) == 0)
            m_models = control[key].toObject();

        if (key.compare("jobs", Qt::CaseInsensitive) == 0)
            m_jobs = control[key].toObject();

        if (key.compare("analyse", Qt::CaseInsensitive) == 0)
            m_analyse = control[key].toObject();
    }
    if (!m_main.isEmpty()) {
        for (const auto str : m_main.keys()) {
            if (str.compare("GenerateData", Qt::CaseInsensitive) == 0) {
                m_simulate_job = true;
                m_simulation = m_main[str].toObject();
            }
        }
    }
    ParseMain();
}

bool SupraFitCli::LoadFile()
{
    FileHandler* handler = new FileHandler(m_infile, this);
    handler->setIndependentRows(m_independent_rows);
    handler->setStartPoint(m_start_point);
    handler->setSeriesCount(m_series); // this only important for the subclassed simulator
    if (m_prepare.size()) {
        if (m_prepare.contains("Integration")) {
            QJsonObject integ = m_prepare["Integration"].toObject();
            integ["SupraFit"] = qint_version; // We have to add SupraFit
            handler->setThermogramParameter(integ);
        }
    }
    handler->LoadFile();
    if (handler->Type() == FileHandler::SupraFit) {
        if (!JsonHandler::ReadJsonFile(m_toplevel, m_infile))
            return false;
    } else if (handler->Type() == FileHandler::dH) {
        m_toplevel = handler->getJsonData();

    } else if (handler->Type() == FileHandler::ITC) {
        m_toplevel = handler->getJsonData();
    } else {
        m_toplevel = handler->getJsonData();
    }
    m_data_vector << m_toplevel;
    return true;
}

void SupraFitCli::ParseMain()
{

    if (m_main.isEmpty())
        return;

    if (m_infile.isEmpty() || m_infile.isNull()) {
        if (m_main.contains("InFile"))
            m_infile = m_main["InFile"].toString();

        if (m_infile.isEmpty() || m_infile.isNull())
            return;
    }

    m_independent_rows = m_main["IndependentRows"].toInt(2);
    m_independent_rows = m_main["InputSize"].toInt(2);
    m_start_point = m_main["StartPoint"].toInt(0);
    qApp->instance()->setProperty("threads", m_main["Threads"].toInt(QThreadPool::globalInstance()->maxThreadCount()));
    m_guess = m_main["Guess"].toBool(false);
    m_fit = m_main["Fit"].toBool(false);
    m_extension = m_main["Extension"].toString("suprafit");
    if (m_main.contains("OutFile")) {
        m_outfile = m_main["OutFile"].toString();
    }

    /*
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
        return;
    }
    return;
    */
}

bool SupraFitCli::SaveFile(const QString& file, const QJsonObject& data)
{
    if (JsonHandler::WriteJsonFile(data, file)) {
        std::cout << file.toStdString() << " successfully written to disk" << std::endl;
        return true;
    }
    return false;
}

bool SupraFitCli::SaveFile()
{
    if (JsonHandler::WriteJsonFile(m_toplevel, m_outfile + m_extension)) {
        std::cout << m_outfile.toStdString() << " successfully written to disk" << std::endl;
        return true;
    }
    return false;
}

void SupraFitCli::PrintFileContent(int index)
{
    int i = 1;
    DataClass* data = new DataClass(m_toplevel["data"].toObject());

    if (data->DataPoints() == 0)
        return;

    std::cout << data->Data2Text().toStdString() << std::endl;

    for (const QString& key : m_toplevel.keys()) {
        if (key.contains("model")) {
            QSharedPointer<AbstractModel> model = JsonHandler::Json2Model(m_toplevel[key].toObject(), data);
            if (index == 0 || i == index) {
                std::cout << Print::Html2Raw(model->ModelInfo()).toStdString() << std::endl;
                std::cout << Print::Html2Raw(model->AnalyseStatistic()).toStdString() << std::endl;
            }
        }
        ++i;
    }
}

void SupraFitCli::PrintFileStructure()
{
    for (const QString& key : m_toplevel.keys()) {
        std::cout << key.toStdString() << std::endl;
    }
}

void SupraFitCli::Work()
{
    for (const auto project : m_data_vector) {
        auto result = PerfomeJobs(project, m_models, m_jobs);
    }
}

QJsonObject SupraFitCli::PerfomeJobs(const QJsonObject& data, const QJsonObject& models, const QJsonObject& job)
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

QVector<QSharedPointer<AbstractModel>> SupraFitCli::AddModels(const QJsonObject& modelsjson, QPointer<DataClass> data)
{
    QVector<QSharedPointer<AbstractModel>> models;
    for (const QString& str : modelsjson.keys()) {
        SupraFit::Model model;
        QJsonObject options;
        if (modelsjson[str].toObject().contains("model")) {
            model = static_cast<SupraFit::Model>(modelsjson[str].toObject()["model"].toInt());
            options = modelsjson[str].toObject()["options"].toObject();
        } else if (modelsjson[str].toObject().contains("ScriptModel")) {
            model = SupraFit::ScriptModel;
        } else
            model = static_cast<SupraFit::Model>(modelsjson[str].toInt());

        QSharedPointer<AbstractModel> t = CreateModel(model, data);
        if (model == SupraFit::ScriptModel)
            t->DefineModel(modelsjson[str].toObject());

        if (!t)
            continue;

        if (!options.isEmpty())
            t->setOptions(options);

        if (m_main.contains("Guess") && m_main["Guess"].toBool()) {
            t->InitialGuess();

            if (m_main.contains("Fit") && m_main["Fit"].toBool()) {
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

QSharedPointer<AbstractModel> SupraFitCli::AddModel(int model, QPointer<DataClass> data)
{
    QSharedPointer<AbstractModel> t = CreateModel(model, data);
    if (!t)
        return t;
    return t;
}
/*
bool SupraFitCli::Prepare()
{

    if (m_infile.isEmpty() || m_infile.isNull()) {
        if (m_mainjson.contains("InFile"))
            m_infile = m_mainjson["InFile"].toString();
    }

    if (m_mainjson.contains("OutFile")) {
        m_outfile = m_mainjson["OutFile"].toString();
    }

    LoadFile();
    m_extension = ".json";
    SaveFile();
    return true;
}
*/
void SupraFitCli::Analyse(const QJsonObject& analyse, const QVector<QJsonObject>& models)
{
    QVector<QJsonObject> models_json;
    if (models.isEmpty()) {
        for (const QString& str : m_toplevel.keys()) {
            if (str.contains("model"))
                models_json << m_toplevel[str].toObject();
        }
    } else
        models_json = models;

    if (!analyse.isEmpty()) {
        for (const QString& key : analyse.keys()) {
            QJsonObject object = analyse[key].toObject();
            QString text;
            if (object["method"].toInt() == 1) {
                bool local = object["Local"].toBool(false);
                int index = object["Index"].toInt(1);
                text = StatisticTool::CompareMC(models_json, local, index);
            } else if (object["method"].toInt() == 4) {
                bool local = object["Local"].toBool(false);
                int CXO = object["CXO"].toInt(1);
                int X = object["X"].toInt(1);

                text = StatisticTool::CompareCV(models_json, CXO, local, X);
            } else if (object["method"].toInt() == 5) {
                bool local = object["Local"].toBool(false);
                double cutoff = object["CutOff"].toDouble(0);
                text = StatisticTool::AnalyseReductionAnalysis(models_json, local, cutoff);
            }
            std::cout << Print::Html2Raw(text).toStdString() << std::endl;
        }
    }
}

void SupraFitCli::OpenFile()
{
    LoadFile();
}
