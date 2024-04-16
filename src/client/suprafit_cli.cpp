/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2023 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include <QtCore/QTimer>

#include "src/capabilities/datagenerator.h"
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
    //m_data = core->m_data;
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
        for (const auto& str : m_main.keys()) {
            if (str.compare("Tasks", Qt::CaseInsensitive) == 0) {
                QString string = m_main[str].toString();
                QStringList tasks = string.split("|");
                tasks << string.split(",");
                tasks << string.split(";");

                if (tasks.contains("GenerateIndependent"))
                    m_generate_independent = true;
                if (tasks.contains("GenerateNoisyIndependent"))
                    m_generate_noisy_independent = true;
                if (tasks.contains("GenerateDependent"))
                    m_generate_dependent = true;
                if (tasks.contains("GenerateNoisyDependent"))
                    m_generate_noisy_dependent = true;

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

    if (m_toplevel.keys().contains("data"))
        m_data = new DataClass(m_toplevel["data"].toObject());
    else
        m_data = new DataClass(m_toplevel);

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
    m_extension = m_main["extension"].toString("suprafit");
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

bool SupraFitCli::SaveFiles(const QString& file, const QVector<QJsonObject>& projects)
{
    for (int i = 0; i < projects.size(); ++i) {
        SaveFile(m_outfile + "_" + file + "_" + QString::number(i) + "." + m_extension, projects[i]);
    }
    return true;
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
    for (const auto& project : m_data_vector) {
        // auto result = PerformeJobs(project, m_models, m_jobs);
    }
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
            if (object["Method"].toInt() == 1) {
                bool local = object["Local"].toBool(false);
                int index = object["Index"].toInt(1);
                text = StatisticTool::CompareMC(models_json, local, index);
            } else if (object["Method"].toInt() == 4) {
                bool local = object["Local"].toBool(false);
                int CXO = object["CXO"].toInt(1);
                int X = object["X"].toInt(1);

                text = StatisticTool::CompareCV(models_json, CXO, local, X);
            } else if (object["Method"].toInt() == 5) {
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

QVector<QJsonObject> SupraFitCli::GenerateIndependent()
{
    QVector<QJsonObject> project_list;

    m_independent_rows = m_main["IndependentRows"].toInt(2);
    m_start_point = m_main["StartPoint"].toInt(0);
    m_series = m_simulation["Series"].toInt();
    int dep_columns = m_main["dependent"].toInt(1);
    qDebug() << m_main << dep_columns;
    if (!LoadFile())
        return project_list;

    if (m_main.contains("OutFile")) {
        m_outfile = m_main["OutFile"].toString();
    }

    QJsonObject independent;
    DataClass* final_data = new DataClass;
    DataGenerator* generator = new DataGenerator(this);
    generator->setJson(m_main);
    if (!generator->Evaluate())
        return project_list;
    independent = generator->Table()->ExportTable(true);

    m_data = new DataClass();
    m_data->setIndependentTable(generator->Table());
    m_data->setType(DataClassPrivate::DataType::Simulation);
    m_data->setSimulateDependent(dep_columns);
    m_data->setDataBegin(0);
    m_data->setDataEnd(m_data->IndependentModel()->rowCount());

    final_data->setIndependentTable(generator->Table());
    final_data->setDataBegin(0);
    final_data->setDataEnd(m_data->IndependentModel()->rowCount());
    final_data->setType(DataClassPrivate::DataType::Simulation);
    final_data->setSimulateDependent(dep_columns);
    DataTable* model = new DataTable(m_data->IndependentModel()->rowCount(), dep_columns, this);
    final_data->setDependentTable(model);

    QJsonObject d;
    d["data"] = final_data->ExportData();
    project_list << d;
    SaveFiles("gen_indep", project_list);
    return project_list;
}
QVector<QJsonObject> SupraFitCli::GenerateNoisyIndependent(const QJsonObject& json_data)
{
    QVector<QJsonObject> project_list;
    DataClass* final_data = new DataClass;
    final_data->ImportData(json_data["data"].toObject());
    final_data->setDataType(DataClassPrivate::Table);
    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng(seed);
    DataTable* table = final_data->IndependentModel()->PrepareMC(QVector<double>() << 0.0001, rng);
    table->Debug();
    final_data->setIndependentRawTable(table);
    QJsonObject d;
    d["data"] = final_data->ExportData();
    project_list << d;
    SaveFiles("gen_noisy_indep", project_list);
    return project_list;
}

QVector<QJsonObject> SupraFitCli::GenerateDependent(const QJsonObject& json_data)
{
    QVector<QJsonObject> project_list;
    DataClass* final_data = new DataClass;
    final_data->ImportData(json_data["data"].toObject());
    final_data->setDataType(DataClassPrivate::Table);
    DataClass* tmp = new DataClass;
    tmp->ImportData(json_data["data"].toObject());

    QJsonObject models = m_main["models"].toObject();
    for (const QString& key : models.keys()) {
        QSharedPointer<AbstractModel> t;
        if (models[key].isString()) {
            QString file = models[key].toString();
            t = JsonHandler::Json2Model(JsonHandler::LoadFile(file), tmp);
        } else if (models[key].isObject()) {
            QJsonObject object = models[key].toObject();
            t = JsonHandler::Json2Model(object, tmp);
        } else {
            t = CreateModel(models[key].toInt(), tmp);
        }
        final_data->setDependentTable(t->ModelTable());
        QJsonObject d;
        d["data"] = final_data->ExportData();
        project_list << d;
    }
    SaveFiles("gen_dep", project_list);

    return project_list;
}

QVector<QJsonObject> SupraFitCli::GenerateNoisyDependent(const QJsonObject& json_data)
{
    QVector<QJsonObject> project_list;
    SaveFiles("gen_noisy_dep", project_list);

    return project_list;
}

QVector<QJsonObject> SupraFitCli::GenerateData()
{
    QVector<QJsonObject> project_list;

    if (m_main.isEmpty())
        return project_list;
    /*
    if (m_infile.isEmpty() || m_infile.isNull()) {
        if (m_main.contains("InFile"))
            m_infile = m_main["InFile"].toString();

        if (m_infile.isEmpty() || m_infile.isNull())
            return project_list;
    }
    */
    m_independent_rows = m_main["IndependentRows"].toInt(2);
    m_start_point = m_main["StartPoint"].toInt(0);
    m_series = m_simulation["Series"].toInt();
    int dep_columns = m_main["dependent"].toInt(1);
    if (!LoadFile())
        return project_list;

    if (m_main.contains("OutFile")) {
        m_outfile = m_main["OutFile"].toString();
    }

    QJsonObject independent;
    DataClass* final_data = new DataClass;
    if (m_toplevel.isEmpty()) {
        DataGenerator* generator = new DataGenerator(this);
        generator->setJson(m_main);
        if (!generator->Evaluate())
            return project_list;
        independent = generator->Table()->ExportTable(true);
        m_data = new DataClass();
        m_data->setIndependentTable(generator->Table());
        m_data->setType(DataClassPrivate::DataType::Simulation);
        m_data->setSimulateDependent(dep_columns);
        m_data->setDataBegin(0);
        m_data->setDataEnd(m_data->IndependentModel()->rowCount());

        final_data->setIndependentTable(generator->Table());
        final_data->setDataBegin(0);
        final_data->setDataEnd(m_data->IndependentModel()->rowCount());
    } else {
        if (m_toplevel.keys().contains("data"))
            m_data = new DataClass(m_toplevel["data"].toObject());
        else
            m_data = new DataClass(m_toplevel);
    }
    if (!m_data)
        return project_list;

    QJsonObject models = m_main["models"].toObject();
    for (const QString& key : models.keys()) {
        QSharedPointer<AbstractModel> t;
        if (models[key].isString()) {
            QString file = models[key].toString();
            t = JsonHandler::Json2Model(JsonHandler::LoadFile(file), m_data);
        } else if (models[key].isObject()) {
            QJsonObject object = models[key].toObject();
            t = JsonHandler::Json2Model(object, m_data);
        } else {
            t = CreateModel(models[key].toInt(), m_data);
        }
        final_data->setDependentTable(t->ModelTable());
        project_list << final_data->ExportData();
    }

    /*
    int model = m_simulation["Model"].toInt();
    double variance = m_simulation["Variance"].toDouble();
    int repeat = m_simulation["Repeat"].toInt();

    QPointer<DataClass> data = new DataClass(m_data.data());
    QJsonObject export_object = data->ExportData();
    QSharedPointer<AbstractModel> t = AddModel(model, data);
    QVector<qreal> random;

    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng(seed);
    int file_int = 0;

    QString global_limits, local_limits;
    if (m_simulation.contains("GlobalRandomLimits")) {
        global_limits = m_simulation["GlobalRandomLimits"].toString();
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

    if (m_simulation.contains("LocalRandomLimits")) {
        local_limits = m_simulation["LocalRandomLimits"].toString();

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
    //}

    return project_list;
}

void SupraFitCli::CheckStopFile()
{
    if (QFileInfo::exists("stop")) {
        m_interrupt = true;
        emit Interrupt();
        QFile file("stop");
        file.remove();
    } else
        QTimer::singleShot(100, this, &SupraFitCli::CheckStopFile);
}

/*
SupraFitCli * SupraFitCli::Generate() const
{
    SupraFitCli *generated = new SupraFitCli;

    return generated;
}*/
