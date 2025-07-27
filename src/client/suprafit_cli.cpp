/*
 * SupraFit Command Line Tools for batch processing and data generation
 * Copyright (C) 2018 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 * 
 * This CLI tool provides batch processing capabilities for SupraFit,
 * including modular data generation with Independent/Dependent structure,
 * file loading with range selection, and ML pipeline management.
 * Enhanced with modular structure by Claude Code AI Assistant.
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
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QRandomGenerator>
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
#include "src/version.h"

#include <fmt/color.h>
#include <fmt/core.h>

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
    fmt::print("🔍 DEBUG setControlJson: Input JSON keys: {}\n", keys.join(", ").toStdString());
    
    for (const QString& key : keys) {
        if (key.compare("main", Qt::CaseInsensitive) == 0) {
            m_main = control[key].toObject();
            fmt::print("🔍 DEBUG setControlJson: Found Main section with keys: {}\n", m_main.keys().join(", ").toStdString());
        }

        if (key.compare("models", Qt::CaseInsensitive) == 0)
            m_models = control[key].toObject();

        if (key.compare("jobs", Qt::CaseInsensitive) == 0)
            m_jobs = control[key].toObject();

        if (key.compare("analyse", Qt::CaseInsensitive) == 0)
            m_analyse = control[key].toObject();
            
        if (key.compare("simulation", Qt::CaseInsensitive) == 0)
            m_simulation = control[key].toObject();
            
        // New modular structure support - Claude Generated
        if (key.compare("independent", Qt::CaseInsensitive) == 0) {
            m_independent = control[key].toObject();
            m_use_modular_structure = true;
            m_generate_dependent = true;  // Enable dependent data generation
            m_simulate_job = true;        // Enable simulation mode
            fmt::print("🔍 DEBUG setControlJson: Found Independent section with source: {}\n", 
                      m_independent["Source"].toString().toStdString());
        }
        
        if (key.compare("dependent", Qt::CaseInsensitive) == 0) {
            m_dependent = control[key].toObject();
            m_use_modular_structure = true;
            m_generate_dependent = true;  // Enable dependent data generation
            m_simulate_job = true;        // Enable simulation mode
            fmt::print("🔍 DEBUG setControlJson: Found Dependent section with source: {}\n", 
                      m_dependent["Source"].toString().toStdString());
        }
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
            
            // Handle GenerateData configuration
            if (str.compare("GenerateData", Qt::CaseInsensitive) == 0) {
                fmt::print("🔍 DEBUG: Found GenerateData configuration!\n");
                m_generate_dependent = true;
                m_generate_noisy_dependent = true;
                m_simulate_job = true;
                m_simulation = m_main[str].toObject();
                fmt::print("🔍 DEBUG: Set m_generate_dependent = true\n");
            }
            
            // Handle DataOnly task (simple load and save)
            if (str.compare("Tasks", Qt::CaseInsensitive) == 0) {
                QString task = m_main[str].toString();
                if (task.compare("DataOnly", Qt::CaseInsensitive) == 0) {
                    m_data_only = true;
                    qDebug() << "DataOnly task detected";
                }
                else if (task.compare("GenerateInputData", Qt::CaseInsensitive) == 0) {
                    m_generate_input_data = true;
                    qDebug() << "GenerateInputData task detected";
                }
                else if (task.compare("GenerateData", Qt::CaseInsensitive) == 0) {
                    m_generate_dependent = true;
                    m_generate_noisy_dependent = true;
                    m_simulate_job = true;
                    qDebug() << "GenerateData task detected";
                }
            }
        }
    }
    ParseMain();
}

bool SupraFitCli::LoadFile()
{
    fmt::print("🔍 DEBUG LoadFile: Starting to load file: {}\n", m_infile.toStdString());
    
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
    
    fmt::print("🔍 DEBUG LoadFile: Handler type: {}\n", static_cast<int>(handler->Type()));
    
    if (handler->Type() == FileHandler::SupraFit) {
        fmt::print("🔍 DEBUG LoadFile: Processing SupraFit file\n");
        if (!JsonHandler::ReadJsonFile(m_toplevel, m_infile))
            return false;
    } else if (handler->Type() == FileHandler::dH) {
        fmt::print("🔍 DEBUG LoadFile: Processing dH file\n");
        m_toplevel = handler->getJsonData();

    } else if (handler->Type() == FileHandler::ITC) {
        fmt::print("🔍 DEBUG LoadFile: Processing ITC file\n");
        m_toplevel = handler->getJsonData();
    } else {
        fmt::print("🔍 DEBUG LoadFile: Processing other file type\n");
        m_toplevel = handler->getJsonData();
    }
    m_data_vector << m_toplevel;

    fmt::print("🔍 DEBUG LoadFile: m_toplevel keys: {}\n", m_toplevel.keys().join(", ").toStdString());

    if (m_toplevel.keys().contains("data")) {
        fmt::print("🔍 DEBUG LoadFile: Creating DataClass from m_toplevel[\"data\"]\n");
        QJsonObject dataObj = m_toplevel["data"].toObject();
        qDebug() << dataObj;
        fmt::print("🔍 DEBUG LoadFile: data object keys: {}\n", dataObj.keys().join(", ").toStdString());
        
        if (dataObj.contains("dependent")) {
            QJsonObject depObj = dataObj["dependent"].toObject();
            fmt::print("🔍 DEBUG LoadFile: dependent object keys: {}\n", depObj.keys().join(", ").toStdString());
            fmt::print("🔍 DEBUG LoadFile: dependent rows: {}, cols: {}\n", depObj["rows"].toInt(), depObj["cols"].toInt());
            
            if (depObj.contains("data")) {
                QJsonObject depDataObj = depObj["data"].toObject();
                fmt::print("🔍 DEBUG LoadFile: dependent data sample row 0: '{}'\n", depDataObj["0"].toString().toStdString());
            }
        }

        if (dataObj.contains("independent")) {
            QJsonObject depObj = dataObj["independent"].toObject();
            fmt::print("🔍 DEBUG LoadFile: independent object keys: {}\n", depObj.keys().join(", ").toStdString());
            fmt::print("🔍 DEBUG LoadFile: independent rows: {}, cols: {}\n", depObj["rows"].toInt(), depObj["cols"].toInt());
            
            if (depObj.contains("data")) {
                QJsonObject depDataObj = depObj["data"].toObject();
                fmt::print("🔍 DEBUG LoadFile: independent data sample row 0: '{}'\n", depDataObj["0"].toString().toStdString());
            }
        }
        
        m_data = new DataClass(dataObj);
    } else {
        fmt::print("🔍 DEBUG LoadFile: Creating DataClass from m_toplevel\n");
        m_data = new DataClass(m_toplevel);
    }

    fmt::print("🔍 DEBUG LoadFile: DataClass created successfully\n");
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
    if (m_data_vector.isEmpty()) {
        qDebug() << "No data to process in Work()";
        return;
    }
    
    if (m_models.isEmpty()) {
        qDebug() << "No models configured for Work()";
        return;
    }
    
    if (m_jobs.isEmpty()) {
        qDebug() << "No jobs configured for Work()";
        return;
    }
    
    qDebug() << "Processing" << m_data_vector.size() << "datasets with" << m_models.keys().size() << "models";
    
    for (int i = 0; i < m_data_vector.size(); ++i) {
        const auto& project = m_data_vector[i];
        
        qDebug() << "Processing dataset" << (i + 1) << "/" << m_data_vector.size();
        
        QJsonObject result = PerformeJobs(project, m_models, m_jobs);
        
        if (!result.isEmpty()) {
            // Save results for this dataset
            QString outputFile = QString("%1_%2.json").arg(m_outfile).arg(i);
            if (SaveFile(outputFile, result)) {
                qDebug() << "Saved results to:" << outputFile;
            } else {
                qWarning() << "Failed to save results to:" << outputFile;
            }
        }
    }
    
    qDebug() << "Work() completed for all datasets";
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

QVector<QJsonObject> SupraFitCli::GenerateDataOnly()
{
    QVector<QJsonObject> project_list;
    
    if (m_main.isEmpty())
        return project_list;
    
    // Load input file
    m_independent_rows = m_main["IndependentRows"].toInt(2);
    m_start_point = m_main["StartPoint"].toInt(0);
    
    if (!LoadFile())
        return project_list;
    
    if (m_main.contains("OutFile")) {
        m_outfile = m_main["OutFile"].toString();
    }
    
    qDebug() << "GenerateDataOnly: Loading data from" << m_infile;
    qDebug() << "Independent Rows:" << m_independent_rows;
    qDebug() << "Start Point:" << m_start_point;
    
    if (!m_data) {
        qDebug() << "ERROR: No data loaded!";
        return project_list;
    }
    
    // Simply export the loaded data as-is and wrap in proper SupraFit project structure
    QJsonObject dataObject = m_data->ExportData();
    dataObject["DataType"] = 1;
    dataObject["content"] = "Data loaded from " + m_infile;
    
    QJsonObject export_object;
    export_object["data"] = dataObject;  // Wrap data in "data" field for proper SupraFit project structure
    
    project_list << export_object;
    
    qDebug() << "GenerateDataOnly: Exported data with" 
             << m_data->IndependentModel()->rowCount() << "rows,"
             << m_data->IndependentModel()->columnCount() << "columns";
    
    return project_list;
}

QVector<QJsonObject> SupraFitCli::GenerateInputData()
{
    QVector<QJsonObject> project_list;
    
    if (m_main.isEmpty())
        return project_list;
    
    // Check if we have DataGenerator configuration
    if (!m_main.contains("DataGenerator")) {
        qDebug() << "ERROR: No DataGenerator configuration found!";
        return project_list;
    }
    
    QJsonObject generatorConfig = m_main["DataGenerator"].toObject();
    qDebug() << "GenerateInputData: Using DataGenerator configuration:" << generatorConfig;
    
    // Set up DataGenerator
    DataGenerator* generator = new DataGenerator(this);
    generator->setJson(generatorConfig);
    
    if (!generator->Evaluate()) {
        qDebug() << "ERROR: DataGenerator evaluation failed!";
        delete generator;
        return project_list;
    }
    
    // Create DataClass with generated independent data
    DataClass* dataClass = new DataClass();
    dataClass->setIndependentTable(generator->Table());
    dataClass->setIndependentRawTable(new DataTable(generator->Table()));
    dataClass->setType(DataClassPrivate::DataType::Table);
    
    // Generate dependent data
    DataTable* dependentTable = nullptr;
    QString dependentEquations = "";
    
    if (generatorConfig.contains("dependent_equations")) {
        // Generate dependent data using equations with Y variables
        dependentEquations = generatorConfig["dependent_equations"].toString();
        qDebug() << "GenerateInputData: Generating dependent data with equations:" << dependentEquations;
        
        // Create a new DataGenerator for dependent data
        QJsonObject dependentConfig = generatorConfig;
        dependentConfig["equations"] = dependentEquations;
        
        DataGenerator* dependentGenerator = new DataGenerator(this);
        dependentGenerator->setJson(dependentConfig);
        
        if (dependentGenerator->Evaluate()) {
            dependentTable = new DataTable(dependentGenerator->Table());
            qDebug() << "GenerateInputData: Generated dependent table with" 
                     << dependentTable->rowCount() << "rows," 
                     << dependentTable->columnCount() << "columns";
        } else {
            qDebug() << "ERROR: Dependent data generation failed!";
        }
        
        delete dependentGenerator;
    } else {
        // Generate random dependent data (default behavior)
        int rows = generator->Table()->rowCount();
        int dependentCols = generatorConfig.contains("dependent_columns") ? 
                           generatorConfig["dependent_columns"].toInt() : 1;
        
        qDebug() << "GenerateInputData: Generating random dependent data with" 
                 << rows << "rows," << dependentCols << "columns";
        
        dependentTable = new DataTable(rows, dependentCols, this);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < dependentCols; ++j) {
                double randomValue = QRandomGenerator::global()->bounded(1.0);
                QModelIndex index = dependentTable->index(i, j);
                dependentTable->setData(index, randomValue);
            }
        }
        
        // Set headers for random dependent data
        QStringList headers;
        for (int j = 0; j < dependentCols; ++j) {
            headers << QString("Y%1").arg(j + 1);
        }
        dependentTable->setHeader(headers);
        dependentEquations = "Random values";
    }
    if (dependentTable) {
        dataClass->setDependentTable(dependentTable);
        dataClass->setDependentRawTable(new DataTable(dependentTable));
    }
    
    // Set output file name
    if (m_main.contains("OutFile")) {
        m_outfile = m_main["OutFile"].toString();
    }
    
    // Export the generated data and wrap in proper SupraFit project structure
    QJsonObject dataObject = dataClass->ExportData();
    dataObject["DataType"] = 1;
    dataObject["content"] = QString("Generated data: %1 rows, %2 independent columns (%3), %4 dependent columns (%5)")
                                .arg(generator->Table()->rowCount())
                                .arg(generator->Table()->columnCount())
                                .arg(generatorConfig["equations"].toString())
                                .arg(dependentTable ? dependentTable->columnCount() : 0)
                                .arg(dependentEquations);
    
    QJsonObject export_object;
    export_object["data"] = dataObject;  // Wrap data in "data" field for proper SupraFit project structure
    
    project_list << export_object;
    
    qDebug() << "GenerateInputData: Generated data with" 
             << generator->Table()->rowCount() << "rows,"
             << generator->Table()->columnCount() << "independent columns"
             << "using equations:" << generatorConfig["equations"].toString()
             << "and" << (dependentTable ? dependentTable->columnCount() : 0) << "dependent columns";
    
    delete generator;
    delete dataClass;
    
    return project_list;
}

void SupraFitCli::AnalyzeFile()
{
    fmt::print("\n" + std::string(80, '=') + "\n");
    fmt::print("📊 SUPRAFIT FILE ANALYSIS\n");
    fmt::print(std::string(80, '=') + "\n\n");

    // Basic file information
    fmt::print("📁 FILE INFORMATION:\n");
    fmt::print("   Input file: {}\n", m_infile.toStdString());
    
    QFileInfo fileInfo(m_infile);
    fmt::print("   File size: {} bytes\n", fileInfo.size());
    fmt::print("   File extension: {}\n", fileInfo.suffix().toStdString());
    fmt::print("   Absolute path: {}\n", fileInfo.absoluteFilePath().toStdString());
    fmt::print("   File exists: {}\n", fileInfo.exists() ? "Yes" : "No");
    fmt::print("   Last modified: {}\n", fileInfo.lastModified().toString(Qt::ISODate).toStdString());

    if (!m_data) {
        fmt::print("   ❌ No data loaded!\n\n");
        return;
    }
    
    // Data structure analysis
    fmt::print("\n📋 DATA STRUCTURE:\n");
    fmt::print("   Data type: {}\n", static_cast<int>(m_data->Type()));
    QJsonObject exportData = m_data->ExportData();
    fmt::print("   Title: '{}'\n", exportData["title"].toString().toStdString());
    fmt::print("   UUID: {}\n", exportData["uuid"].toString().toStdString());
    
    // Independent data analysis
    fmt::print("\n🔢 INDEPENDENT DATA:\n");
    if (m_data->IndependentModel()) {
        int indepRows = m_data->IndependentModel()->rowCount();
        int indepCols = m_data->IndependentModel()->columnCount();
        fmt::print("   Dimensions: {} rows × {} columns\n", indepRows, indepCols);
        
        QStringList headers = m_data->IndependentModel()->header();
        if (!headers.isEmpty()) {
            fmt::print("   Headers: {}\n", headers.join(", ").toStdString());
        }
        
        // Show first few data points
        if (indepRows > 0 && indepCols > 0) {
            fmt::print("   Sample data (first 5 rows):\n");
            for (int i = 0; i < std::min(5, indepRows); ++i) {
                fmt::print("      Row {}: ", i);
                for (int j = 0; j < indepCols; ++j) {
                    double value = m_data->IndependentModel()->data(i, j);
                    fmt::print("{:.3f} ", value);
                }
                fmt::print("\n");
            }
        }
    } else {
        fmt::print("   ❌ No independent data found!\n");
    }
    
    // Dependent data analysis
    fmt::print("\n📈 DEPENDENT DATA:\n");
    if (m_data->DependentModel()) {
        int depRows = m_data->DependentModel()->rowCount();
        int depCols = m_data->DependentModel()->columnCount();
        fmt::print("   Dimensions: {} rows × {} columns\n", depRows, depCols);
        
        QStringList headers = m_data->DependentModel()->header();
        if (!headers.isEmpty()) {
            fmt::print("   Headers: {}\n", headers.join(", ").toStdString());
        }
        
        // Show first few data points
        if (depRows > 0 && depCols > 0) {
            fmt::print("   Sample data (first 5 rows):\n");
            for (int i = 0; i < std::min(5, depRows); ++i) {
                fmt::print("      Row {}: ", i);
                for (int j = 0; j < depCols; ++j) {
                    double value = m_data->DependentModel()->data(i, j);
                    fmt::print("{:.3f} ", value);
                }
                fmt::print("\n");
            }
        }
    } else {
        fmt::print("   ❌ No dependent data found!\n");
    }

    // Enhanced Configuration analysis - Claude Generated
    fmt::print("\n⚙️  CONFIGURATION:\n");
    if (!m_main.isEmpty()) {
        fmt::print("   Main configuration found:\n");
        for (auto it = m_main.begin(); it != m_main.end(); ++it) {
            QString key = it.key();
            QJsonValue value = it.value();
            if (value.isObject()) {
                fmt::print("      {}: [Object with {} keys]\n", key.toStdString(), value.toObject().size());

                // Special handling for GenerateData configuration
                if (key == "GenerateData") {
                    QJsonObject genData = value.toObject();
                    analyzeGenerateDataConfig(genData);
                }
            } else if (value.isArray()) {
                fmt::print("      {}: [Array with {} elements]\n", key.toStdString(), value.toArray().size());
            } else {
                fmt::print("      {}: '{}'\n", key.toStdString(), value.toVariant().toString().toStdString());
            }
        }
    } else if (!m_toplevel.isEmpty()) {
        // Check if configuration is stored directly in toplevel - Claude Generated
        fmt::print("   Analyzing top-level configuration:\n");
        for (auto it = m_toplevel.begin(); it != m_toplevel.end(); ++it) {
            QString key = it.key();
            QJsonValue value = it.value();
            if (key == "Main" && value.isObject()) {
                fmt::print("      Main section found with {} keys:\n", value.toObject().size());
                QJsonObject mainObj = value.toObject();
                for (auto mainIt = mainObj.begin(); mainIt != mainObj.end(); ++mainIt) {
                    QString mainKey = mainIt.key();
                    QJsonValue mainValue = mainIt.value();
                    if (mainValue.isObject()) {
                        fmt::print("         {}: [Object with {} keys]\n", mainKey.toStdString(), mainValue.toObject().size());

                        // Special handling for GenerateData configuration in Main section
                        if (mainKey == "GenerateData") {
                            QJsonObject genData = mainValue.toObject();
                            analyzeGenerateDataConfig(genData);
                        }
                    } else if (mainValue.isArray()) {
                        fmt::print("         {}: [Array with {} elements]\n", mainKey.toStdString(), mainValue.toArray().size());
                    } else {
                        fmt::print("         {}: '{}'\n", mainKey.toStdString(), mainValue.toVariant().toString().toStdString());
                    }
                }
            } else if (value.isObject()) {
                fmt::print("      {}: [Object with {} keys]\n", key.toStdString(), value.toObject().size());
            } else if (value.isArray()) {
                fmt::print("      {}: [Array with {} elements]\n", key.toStdString(), value.toArray().size());
            } else {
                fmt::print("      {}: '{}'\n", key.toStdString(), value.toVariant().toString().toStdString());
            }
        }
    }

    // Enhanced modular structure analysis - Claude Generated
    if (!m_independent.isEmpty() || !m_dependent.isEmpty()) {
        fmt::print("   Modular structure configuration found:\n");
        if (!m_independent.isEmpty()) {
            fmt::print("      Independent: {} configuration keys\n", m_independent.size());
            if (m_independent.contains("Source")) {
                fmt::print("         Source: {}\n", m_independent["Source"].toString().toStdString());
            }
            if (m_independent.contains("Generator")) {
                QJsonObject gen = m_independent["Generator"].toObject();
                fmt::print("         Generator: {} (DataPoints: {})\n",
                    gen["Type"].toString().toStdString(),
                    gen["DataPoints"].toInt());
            }
        }
        if (!m_dependent.isEmpty()) {
            fmt::print("      Dependent: {} configuration keys\n", m_dependent.size());
            if (m_dependent.contains("Source")) {
                fmt::print("         Source: {}\n", m_dependent["Source"].toString().toStdString());
            }
            if (m_dependent.contains("Generator")) {
                QJsonObject gen = m_dependent["Generator"].toObject();
                fmt::print("         Generator: {} (Series: {})\n",
                    gen["Type"].toString().toStdString(),
                    gen["Series"].toInt());
            }
        }
    }

    if (m_main.isEmpty() && m_independent.isEmpty() && m_dependent.isEmpty()) {
        fmt::print("   No configuration sections found\n");
    }

    // Model analysis
    fmt::print("\n🔬 MODELS:\n");
    if (!m_models.isEmpty()) {
        fmt::print("   Models configuration found:\n");
        for (auto it = m_models.begin(); it != m_models.end(); ++it) {
            fmt::print("      {}: {}\n", it.key().toStdString(), it.value().toVariant().toString().toStdString());
        }
    } else {
        fmt::print("   No models configuration found\n");
    }
    
    // Jobs analysis
    fmt::print("\n🏗️  JOBS:\n");
    if (!m_jobs.isEmpty()) {
        fmt::print("   Jobs configuration found:\n");
        for (auto it = m_jobs.begin(); it != m_jobs.end(); ++it) {
            fmt::print("      {}: {}\n", it.key().toStdString(), it.value().toVariant().toString().toStdString());
        }
    } else {
        fmt::print("   No jobs configuration found\n");
    }
    
    // System parameters
    fmt::print("\n🔧 SYSTEM PARAMETERS:\n");
    QList<int> sysParamList = m_data->getSystemParameterList();
    if (!sysParamList.isEmpty()) {
        fmt::print("   System parameters: {} found\n", sysParamList.size());
        for (int index : sysParamList) {
            SystemParameter param = m_data->getSystemParameter(index);
            fmt::print("      Param {}: '{}' ({})\n", index, param.Name().toStdString(), param.Description().toStdString());
        }
    } else {
        fmt::print("   No system parameters found\n");
    }

    // Enhanced data type analysis - Claude Generated
    fmt::print("\n📊 DATA TYPE ANALYSIS:\n");
    exportData = m_data->ExportData();
    if (exportData.contains("DataType")) {
        int dataType = exportData["DataType"].toInt();
        fmt::print("   DataType code: {}\n", dataType);

        QString dataTypeDescription;
        switch (dataType) {
        case 1:
            dataTypeDescription = "Standard experimental data";
            break;
        case 10:
            dataTypeDescription = "Simulation data (generated)";
            break;
        default:
            dataTypeDescription = "Unknown data type";
            break;
        }
        fmt::print("   Description: {}\n", dataTypeDescription.toStdString());

        if (dataType == 10) {
            fmt::print("   ⚠️  This is a simulation file - generated data\n");
            if (exportData.contains("simulate_dependent")) {
                fmt::print("   Simulate dependent: {}\n", exportData["simulate_dependent"].toInt());
            }
        }
    }

    // Comments and instructions
    fmt::print("\n💬 METADATA:\n");
    if (exportData.contains("content")) {
        fmt::print("   Content: '{}'\n", exportData["content"].toString().toStdString());
    }
    if (exportData.contains("comment")) {
        fmt::print("   Comment: '{}'\n", exportData["comment"].toString().toStdString());
    }
    if (exportData.contains("instructions")) {
        fmt::print("   Instructions: '{}'\n", exportData["instructions"].toString().toStdString());
    }
    if (exportData.contains("git_commit")) {
        fmt::print("   Git commit: {}\n", exportData["git_commit"].toString().toStdString());
    }
    if (exportData.contains("timestamp")) {
        fmt::print("   Timestamp: {}\n", exportData["timestamp"].toVariant().toString().toStdString());
    }
    if (exportData.contains("SupraFit")) {
        fmt::print("   SupraFit version: {}\n", exportData["SupraFit"].toInt());
    }

    // Output file information
    fmt::print("\n📤 OUTPUT SETTINGS:\n");
    if (!m_outfile.isEmpty()) {
        fmt::print("   Output file: {}\n", m_outfile.toStdString());
    } else if (m_main.contains("OutFile")) {
        fmt::print("   Output file: {}\n", m_main["OutFile"].toString().toStdString());
    } else {
        fmt::print("   No output file specified\n");
    }

    // File processing recommendations - Claude Generated
    fmt::print("\n💡 RECOMMENDATIONS:\n");
    if (exportData.contains("DataType") && exportData["DataType"].toInt() == 10) {
        fmt::print("   • This is a simulation file with generated data\n");
        fmt::print("   • Use for testing and validation purposes\n");
        fmt::print("   • Consider using -i flag for further processing\n");
    }
    if (!m_models.isEmpty()) {
        fmt::print("   • Models configuration detected - ready for fitting\n");
    }
    if (!m_independent.isEmpty() && !m_dependent.isEmpty()) {
        fmt::print("   • Modular structure detected - can generate new data\n");
    }

    // Update metadata section fix - Claude Generated
    QJsonObject exportDataCheck = m_data->ExportData();

    fmt::print("\n" + std::string(80, '=') + "\n");
    fmt::print("✅ FILE ANALYSIS COMPLETE\n");
    fmt::print(std::string(80, '=') + "\n\n");
}

// Enhanced DataGenerator configuration analysis - Claude Generated
void SupraFitCli::analyzeGenerateDataConfig(const QJsonObject& generateDataConfig)
{
    fmt::print("         📊 GENERATE DATA CONFIGURATION:\n");

    if (generateDataConfig.contains("UseDataGenerator") && generateDataConfig["UseDataGenerator"].toBool()) {
        fmt::print("            🔧 DataGenerator Mode: ENABLED\n");

        // Analyze independent variables
        if (generateDataConfig.contains("IndependentVariables")) {
            int indepVars = generateDataConfig["IndependentVariables"].toInt();
            fmt::print("            📈 Independent Variables: {}\n", indepVars);
        }

        // Analyze data points
        if (generateDataConfig.contains("DataPoints")) {
            int dataPoints = generateDataConfig["DataPoints"].toInt();
            fmt::print("            📊 Data Points: {}\n", dataPoints);
        }

        // Analyze equations
        if (generateDataConfig.contains("Equations")) {
            QString equations = generateDataConfig["Equations"].toString();
            QStringList equationList = equations.split("|");
            fmt::print("            🧮 Equations ({} total):\n", equationList.size());
            for (int i = 0; i < equationList.size(); ++i) {
                fmt::print("               X{}: {}\n", i + 1, equationList[i].toStdString());
            }
        }

        // Analyze dependent equations
        if (generateDataConfig.contains("DependentEquations")) {
            QString depEquations = generateDataConfig["DependentEquations"].toString();
            QStringList depEquationList = depEquations.split("|");
            fmt::print("            🎯 Dependent Equations ({} total):\n", depEquationList.size());
            for (int i = 0; i < depEquationList.size(); ++i) {
                fmt::print("               Y{}: {}\n", i + 1, depEquationList[i].toStdString());
            }
        }

        // Analyze series and model
        if (generateDataConfig.contains("Series")) {
            int series = generateDataConfig["Series"].toInt();
            fmt::print("            📈 Series Count: {}\n", series);
        }

        if (generateDataConfig.contains("Model")) {
            int model = generateDataConfig["Model"].toInt();
            fmt::print("            🔬 Model ID: {}\n", model);
        }

        // Analyze repetition and variance
        if (generateDataConfig.contains("Repeat")) {
            int repeat = generateDataConfig["Repeat"].toInt();
            fmt::print("            🔄 Repeat Count: {}\n", repeat);
        }

        if (generateDataConfig.contains("Variance")) {
            double variance = generateDataConfig["Variance"].toDouble();
            fmt::print("            📊 Variance: {:.2e}\n", variance);
        }

        // Analyze random parameter limits
        if (generateDataConfig.contains("RandomParameterLimits")) {
            QJsonObject randomLimits = generateDataConfig["RandomParameterLimits"].toObject();
            fmt::print("            🎲 Random Parameter Limits ({} parameters):\n", randomLimits.size());
            for (auto it = randomLimits.begin(); it != randomLimits.end(); ++it) {
                QString param = it.key();
                QJsonObject limits = it.value().toObject();
                if (limits.contains("min") && limits.contains("max")) {
                    fmt::print("               {}: [{:.3f}, {:.3f}]\n",
                        param.toStdString(), limits["min"].toDouble(), limits["max"].toDouble());
                } else {
                    fmt::print("               {}: {}\n", param.toStdString(), it.value().toVariant().toString().toStdString());
                }
            }
        }

        // Analyze global and local random limits (legacy format)
        if (generateDataConfig.contains("GlobalRandomLimits")) {
            QString globalLimits = generateDataConfig["GlobalRandomLimits"].toString();
            fmt::print("            🌍 Global Random Limits: {}\n", globalLimits.toStdString());
        }

        if (generateDataConfig.contains("LocalRandomLimits")) {
            QString localLimits = generateDataConfig["LocalRandomLimits"].toString();
            fmt::print("            📍 Local Random Limits: {}\n", localLimits.toStdString());
        }

        // Validate configuration consistency
        fmt::print("            ✅ CONFIGURATION VALIDATION:\n");
        validateGenerateDataConfig(generateDataConfig);

    } else {
        fmt::print("            🔧 DataGenerator Mode: DISABLED (using traditional model-based generation)\n");

        if (generateDataConfig.contains("Model")) {
            int model = generateDataConfig["Model"].toInt();
            fmt::print("            🔬 Model ID: {}\n", model);
        }
    }
}

// Configuration validation for read-only analysis - Claude Generated
void SupraFitCli::validateGenerateDataConfig(const QJsonObject& config)
{
    bool hasErrors = false;

    // Check equations vs independent variables consistency
    if (config.contains("Equations") && config.contains("IndependentVariables")) {
        QString equations = config["Equations"].toString();
        int indepVars = config["IndependentVariables"].toInt();
        QStringList equationList = equations.split("|");

        if (equationList.size() != indepVars) {
            fmt::print("               ⚠️  WARNING: Equation count ({}) ≠ Independent Variables ({})\n",
                equationList.size(), indepVars);
            hasErrors = true;
        } else {
            fmt::print("               ✅ Equations match Independent Variables ({})\n", indepVars);
        }
    }

    // Check dependent equations vs series consistency
    if (config.contains("DependentEquations") && config.contains("Series")) {
        QString depEquations = config["DependentEquations"].toString();
        int series = config["Series"].toInt();
        QStringList depEquationList = depEquations.split("|");

        if (depEquationList.size() != series) {
            fmt::print("               ⚠️  WARNING: Dependent Equation count ({}) ≠ Series ({})\n",
                depEquationList.size(), series);
            hasErrors = true;
        } else {
            fmt::print("               ✅ Dependent Equations match Series ({})\n", series);
        }
    }

    // Check data points validity
    if (config.contains("DataPoints")) {
        int dataPoints = config["DataPoints"].toInt();
        if (dataPoints <= 0) {
            fmt::print("               ❌ ERROR: DataPoints must be > 0 (current: {})\n", dataPoints);
            hasErrors = true;
        } else {
            fmt::print("               ✅ DataPoints valid ({})\n", dataPoints);
        }
    }

    // Check repeat count
    if (config.contains("Repeat")) {
        int repeat = config["Repeat"].toInt();
        if (repeat <= 0) {
            fmt::print("               ❌ ERROR: Repeat must be > 0 (current: {})\n", repeat);
            hasErrors = true;
        } else {
            fmt::print("               ✅ Repeat count valid ({})\n", repeat);
        }
    }

    if (!hasErrors) {
        fmt::print("               🎉 Configuration appears valid!\n");
    }
}

QVector<QJsonObject> SupraFitCli::GenerateData()
{
    QVector<QJsonObject> project_list;

    if (m_main.isEmpty() && !m_use_modular_structure)
        return project_list;
    
    // Check if we should use the new modular structure - Claude Generated
    if (m_use_modular_structure) {
        fmt::print("Using new modular Independent/Dependent structure\n");
        return GenerateDataWithModularStructure();
    }
    
    // Use DataGenerator for all data generation - Claude Generated Integration
    fmt::print("Using DataGenerator for unified data generation\n");
    return GenerateDataWithDataGenerator();
}

// Enhanced methods using DataGenerator - Claude Generated
QVector<QJsonObject> SupraFitCli::GenerateDataWithDataGenerator()
{
    QVector<QJsonObject> project_list;
    
    if (m_main.isEmpty() || m_simulation.isEmpty()) {
        fmt::print("Error: Missing configuration data for DataGenerator\n");
        fmt::print("🔍 DEBUG: m_main.isEmpty() = {}, m_simulation.isEmpty() = {}\n", 
                  m_main.isEmpty(), m_simulation.isEmpty());
        fmt::print("🔍 DEBUG: m_main keys: {}\n", m_main.keys().join(", ").toStdString());
        fmt::print("🔍 DEBUG: m_simulation keys: {}\n", m_simulation.keys().join(", ").toStdString());
        return project_list;
    }
    
    // Check if equation-based or model-based generation
    bool useEquations = m_simulation.contains("UseDataGenerator") && m_simulation["UseDataGenerator"].toBool();
    bool useEquationGeneration = useEquations && m_simulation.contains("Equations");
    
    if (useEquationGeneration) {
        // Validate DataGenerator configuration for equation-based generation
        if (!validateDataGeneratorConfig(m_simulation)) {
            fmt::print("Error: Invalid DataGenerator configuration\n");
            return project_list;
        }
    }
    
    // Setup DataGenerator
    DataGenerator* generator = new DataGenerator(this);
    
    if (m_simulation.contains("RandomSeed")) {
        generator->setRandomSeed(m_simulation["RandomSeed"].toVariant().toULongLong());
    }
    
    // Set output file name from configuration - Claude Generated
    if (m_main.contains("OutFile")) {
        m_outfile = m_main["OutFile"].toString();
    }
    
    int repeat = m_simulation["Repeat"].toInt(1);
    
    for (int i = 0; i < repeat; ++i) {
        bool success = false;
        QPointer<DataClass> dataClass = nullptr;
        QJsonObject config;  // Declare outside conditional blocks
        QJsonObject randomLimits;  // Declare outside conditional blocks
        
        if (useEquationGeneration) {
            // Equation-based generation using DataGenerator
            config["independent"] = m_simulation["IndependentVariables"].toInt(1);
            config["datapoints"] = m_simulation["DataPoints"].toInt(20);
            config["equations"] = m_simulation["Equations"].toString();
            
            // Handle dependent equations with consistency checking - Claude Generated
            QString dependentEquations;
            int seriesCount = m_simulation["Series"].toInt(1);
            int actualDependentCount = 0;
            
            if (m_simulation.contains("DependentEquations")) {
                dependentEquations = m_simulation["DependentEquations"].toString();
                actualDependentCount = dependentEquations.split("|").size();
                
                // Consistency check between series and dependent equations
                if (seriesCount != actualDependentCount) {
                    fmt::print("⚠️  WARNING: Series count ({}) differs from dependent equations count ({}). Using dependent equations count.\n", 
                              seriesCount, actualDependentCount);
                    seriesCount = actualDependentCount;
                }
                
                config["dependent_equations"] = dependentEquations;
            }
            
            generator->setJson(config);
            
            // Setup random parameter limits
            if (m_simulation.contains("RandomParameterLimits")) {
                randomLimits = m_simulation["RandomParameterLimits"].toObject();
            }
            
            // Generate independent data
            fmt::print("🔍 DEBUG: Generating independent data with equations: {}\n", config["equations"].toString().toStdString());
            fmt::print("🔍 DEBUG: RandomLimits isEmpty: {}\n", randomLimits.isEmpty());
            if (!randomLimits.isEmpty()) {
                fmt::print("🔍 DEBUG: RandomLimits keys: {}\n", randomLimits.keys().join(", ").toStdString());
                success = generator->EvaluateWithRandomParameters(randomLimits);
            } else {
                success = generator->Evaluate();
            }
            
            if (!success) {
                fmt::print("Error: Failed to generate independent data with DataGenerator (iteration {})\n", i+1);
                continue;
            }
            
            fmt::print("✅ Successfully generated independent data: {} rows x {} cols\n", 
                      generator->Table()->rowCount(), generator->Table()->columnCount());
            
            DataTable* independentTable = generator->Table();
            if (!independentTable) {
                fmt::print("Error: DataGenerator produced null independent table (iteration {})\n", i+1);
                continue;
            }
            
            // Create DataClass and set the generated independent data
            dataClass = new DataClass();
            dataClass->setIndependentTable(new DataTable(independentTable));
            dataClass->setIndependentRawTable(new DataTable(independentTable));
            dataClass->setDataType(DataClassPrivate::Table);

            // Generate dependent data if equations are provided - Claude Generated
            DataTable* dependentTable = nullptr;
            if (!dependentEquations.isEmpty()) {
                // Create separate configuration for dependent data generation
                QJsonObject depConfig;
                depConfig["equations"] = dependentEquations;
                depConfig["datapoints"] = independentTable->rowCount();
                depConfig["independent"] = dependentEquations.split("|").size();
                generator->setJson(depConfig);
                
                bool depSuccess = false;
                if (!randomLimits.isEmpty()) {
                    depSuccess = generator->EvaluateWithRandomParameters(randomLimits);
                } else {
                    depSuccess = generator->Evaluate();
                }
                
                if (depSuccess) {
                    // Use the dependent table directly - no need to extract columns
                    dependentTable = new DataTable(generator->Table());
                } else {
                    fmt::print("Error: Failed to generate dependent data (iteration {})\n", i+1);
                    fmt::print("🔍 DEBUG: Dependent equations: {}\n", depConfig["equations"].toString().toStdString());
                    fmt::print("🔍 DEBUG: DataPoints: {}, Columns: {}\n", 
                              depConfig["datapoints"].toInt(), depConfig["independent"].toInt());
                    continue;
                }
            } else {
                // Create empty dependent table with correct dimensions
                int dependentRows = independentTable->rowCount();
                dependentTable = new DataTable(dependentRows, seriesCount, this);
            }
            
            dataClass->setDependentTable(dependentTable);
            dataClass->setDependentRawTable(new DataTable(dependentTable));
            
            fmt::print("Generated independent data with DataGenerator (iteration {})\n", i+1);
            fmt::print("🔍 DEBUG: Set up dependent data structure with {} rows, {} columns\n", 
                      dependentTable->rowCount(), dependentTable->columnCount());
            
        } else {
            // Traditional model-based generation using existing data
            if (!LoadFile()) {
                fmt::print("Error: Failed to load input file for model-based generation\n");
                continue;
            }
            
            if (!m_data) {
                fmt::print("Error: No input data available for model-based generation\n");
                continue;
            }
            
            // Set config for model-based generation
            config["generation_type"] = "model_based";
            if (m_simulation.contains("Model")) {
                config["model"] = m_simulation["Model"].toInt();
            }
            if (m_simulation.contains("RandomParameterLimits")) {
                randomLimits = m_simulation["RandomParameterLimits"].toObject();
            }
            
            // Create a copy of input data for this iteration
            dataClass = new DataClass(m_data.data());
            
            // Copy independent data
            if (m_data->IndependentModel()->rowCount() > 0) {
                DataTable* indepTable = new DataTable(m_data->IndependentModel());
                dataClass->setIndependentTable(indepTable);
                dataClass->setIndependentRawTable(new DataTable(m_data->IndependentModel()));
                fmt::print("Using input file independent data ({} rows, {} cols)\n", 
                          indepTable->rowCount(), indepTable->columnCount());
            }
            
            success = true;
        }
        
        // Generate dependent data using model if specified
        if (success && m_simulation.contains("Model")) {
            int model = m_simulation["Model"].toInt();
            
            // Use DataGenerator for model-based dependent data generation
            bool modelSuccess = generator->EvaluateWithModel(model, dataClass, m_simulation);
            
            if (modelSuccess) {
                DataTable* modelTable = generator->Table();
                if (modelTable) {
                    fmt::print("🔍 DEBUG: Model table has {} rows, {} columns\n", modelTable->rowCount(), modelTable->columnCount());
                    dataClass->setDependentTable(new DataTable(modelTable));
                    dataClass->setDependentRawTable(new DataTable(modelTable));
                    
                    // Verify the dependent data was set correctly
                    fmt::print("🔍 DEBUG: DataClass dependent table has {} rows, {} columns\n", 
                              dataClass->DependentModel()->rowCount(), dataClass->DependentModel()->columnCount());
                    
                    fmt::print("Generated dependent data using model {} (iteration {})\n", model, i+1);
                } else {
                    fmt::print("🔍 DEBUG: Model table is null!\n");
                }
            } else {
                fmt::print("Warning: Failed to generate model-based dependent data (iteration {})\n", i+1);
            }
        }
        
        QJsonObject data_object = dataClass->ExportData();
        data_object["DataType"] = DataClassPrivate::Table;
        
        // Enhanced content with model parameters and configuration - Claude Generated
        QString baseContent = QString("Generated with DataGenerator (iteration %1)").arg(i+1);
        int configModelId = m_simulation.contains("Model") ? m_simulation["Model"].toInt() : 0;
        if (configModelId > 0) {
            // Get enhanced content from DataGenerator
            QString enhancedContent = dataClass->getContent();
            if (!enhancedContent.isEmpty() && enhancedContent != "Generated with DataGenerator") {
                data_object["content"] = enhancedContent;
            } else {
                data_object["content"] = baseContent;
            }
        } else {
            data_object["content"] = baseContent;
        }
        
        data_object["generator_config"] = config;
        data_object["random_limits"] = randomLimits;
        
        // Wrap in correct SupraFit project structure
        QJsonObject project_object;
        project_object["data"] = data_object;
        
        project_list << project_object;
        fmt::print("Generated dataset {}/{} with DataGenerator\n", i+1, repeat);
    }
    
    SaveFiles("", project_list); // Use m_outfile directly - Claude Generated
    return project_list;
}

bool SupraFitCli::validateDataGeneratorConfig(const QJsonObject& config) const
{
    // Check required fields
    if (!config.contains("Equations") || config["Equations"].toString().isEmpty()) {
        fmt::print("Error: Missing or empty 'Equations' field\n");
        return false;
    }
    
    if (!config.contains("IndependentVariables")) {
        fmt::print("Error: Missing 'IndependentVariables' field\n");
        return false;
    }
    
    int independentVars = config["IndependentVariables"].toInt();
    if (independentVars <= 0) {
        fmt::print("Error: IndependentVariables must be > 0\n");
        return false;
    }
    
    QString equations = config["Equations"].toString();
    QStringList equationList = equations.split("|");
    if (equationList.size() != independentVars) {
        fmt::print("Error: Number of equations ({}) doesn't match IndependentVariables ({})\n", 
                  equationList.size(), independentVars);
        return false;
    }
    
    int dataPoints = config["DataPoints"].toInt(20);
    if (dataPoints <= 0) {
        fmt::print("Error: DataPoints must be > 0\n");
        return false;
    }
    
    return true;
}

QJsonObject SupraFitCli::PerformeJobs(const QJsonObject& data, const QJsonObject& models, const QJsonObject& jobs)
{
    QJsonObject result;
    
    qDebug() << "PerformeJobs called with data keys:" << data.keys();
    qDebug() << "Models:" << models.keys();
    qDebug() << "Jobs:" << jobs.keys();
    
    // Create DataClass from input data
    QPointer<DataClass> dataClass;
    if (data.contains("data")) {
        dataClass = new DataClass(data["data"].toObject());
    } else {
        dataClass = new DataClass(data);
    }
    
    if (!dataClass) {
        qWarning() << "Failed to create DataClass from input data";
        return result;
    }
    
    // Test each model against the data
    QJsonObject modelResults;
    for (const QString& modelKey : models.keys()) {
        qDebug() << "Testing model:" << modelKey;
        
        int modelId = models[modelKey].toInt();
        QSharedPointer<AbstractModel> model = CreateModel(modelId, dataClass);
        
        if (!model) {
            qWarning() << "Failed to create model" << modelId;
            continue;
        }
        
        // Perform initial guess and fit
        model->InitialGuess();
        model->Calculate();
        
        // Extract model statistics
        QJsonObject modelStats;
        modelStats["SSE"] = model->SSE();
        modelStats["AIC"] = model->AIC();
        modelStats["AICc"] = model->AICc();
        modelStats["SEy"] = model->SEy();
        modelStats["ChiSquared"] = model->ChiSquared();
        modelStats["RSquared"] = model->RSquared();
        modelStats["ModelId"] = modelId;
        modelStats["ModelName"] = modelKey;
        
        // Run jobs (statistical analysis) for this model
        JobManager* jobManager = new JobManager(this);
        jobManager->setModel(model);
        
        QJsonObject jobResults;
        for (const QString& jobKey : jobs.keys()) {
            qDebug() << "Running job:" << jobKey << "for model:" << modelKey;
            
            QJsonObject job = jobs[jobKey].toObject();
            jobManager->AddSingleJob(job);
            jobManager->RunJobs();
            
            // Extract job-specific results
            QJsonObject jobResult;
            jobResult["Method"] = job["Method"].toInt();
            jobResult["ModelId"] = modelId;
            jobResult["JobKey"] = jobKey;
            
            jobResults[jobKey] = jobResult;
        }
        
        // Combine model stats and job results
        QJsonObject combinedResult;
        combinedResult["statistics"] = modelStats;
        combinedResult["jobs"] = jobResults;
        
        modelResults[modelKey] = combinedResult;
        
        delete jobManager;
    }
    
    result["models"] = modelResults;
    result["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    qDebug() << "PerformeJobs completed for" << models.keys().size() << "models";
    return result;
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

// New modular JSON structure implementation - Claude Generated
QVector<QJsonObject> SupraFitCli::GenerateDataWithModularStructure()
{
    QVector<QJsonObject> project_list;
    
    fmt::print("🔧 Generating data with modular Independent/Dependent structure\n");
    
    // Validate configuration
    if (m_independent.isEmpty() || m_dependent.isEmpty()) {
        fmt::print("❌ ERROR: Both Independent and Dependent sections are required\n");
        return project_list;
    }
    
    // Get repeat count from Main section
    int repeat = 1;
    if (!m_main.isEmpty()) {
        repeat = m_main["Repeat"].toInt(1);
        if (!m_main.contains("OutFile")) {
            fmt::print("❌ ERROR: OutFile is required in Main section\n");
            return project_list;
        }
        setOutFile(m_main["OutFile"].toString());
    }
    
    fmt::print("📊 Generating {} dataset(s) with modular structure\n", repeat);
    
    // Generate datasets
    for (int iteration = 0; iteration < repeat; ++iteration) {
        fmt::print("🔄 Generating dataset {}/{}\n", iteration + 1, repeat);
        
        // Step 1: Generate Independent data table as JSON
        QJsonObject independentTableJson = generateIndependentDataTable(m_independent);
        if (independentTableJson.isEmpty()) {
            fmt::print("❌ ERROR: Failed to generate independent data for iteration {}\n", iteration + 1);
            continue;
        }
        
        // Step 2: Generate Dependent data table as JSON
        QJsonObject dependentTableJson = generateDependentDataTable(m_dependent, independentTableJson);
        if (dependentTableJson.isEmpty()) {
            fmt::print("❌ ERROR: Failed to generate dependent data for iteration {}\n", iteration + 1);
            continue;
        }
        
        // Step 3: Create DataClass and import JSON tables
        QPointer<DataClass> fullData = new DataClass();
        
        // Import independent data from JSON
        DataTable* independentTable = new DataTable(independentTableJson);
        fullData->setIndependentTable(independentTable);
        fullData->setIndependentRawTable(new DataTable(independentTableJson));
        
        // Import dependent data from JSON  
        DataTable* dependentTable = new DataTable(dependentTableJson);
        fullData->setDependentTable(dependentTable);
        fullData->setDependentRawTable(new DataTable(dependentTableJson));

        fullData->setDataType(DataClassPrivate::Table);

        // Step 3: Apply noise if specified
        fullData = applyNoise(fullData, m_independent["Noise"].toObject(), true);  // Independent noise
        fullData = applyNoise(fullData, m_dependent["Noise"].toObject(), false);   // Dependent noise

        // Step 4: Create project JSON with correct structure - Claude Generated
        QJsonObject innerData = fullData->ExportData();
        innerData["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        innerData["git_commit"] = git_commit_hash;

        // Enhanced content with model details and generation info - Claude Generated
        QString content = m_modelContent;
        if (content.isEmpty()) {
            content = QString("Generated with modular structure (iteration %1)").arg(iteration + 1);
        } else {
            content += QString("\n\nConfiguration Parameters:\n  DataPoints: %1\n  Series: %2\n  Iteration: %3")
                           .arg(fullData->DataPoints())
                           .arg(fullData->SeriesCount())
                           .arg(iteration + 1);
            // Add input configuration - Claude Generated
            QJsonObject inputConfig;
            inputConfig["Main"] = m_main;
            inputConfig["Independent"] = m_independent;
            inputConfig["Dependent"] = m_dependent;
            content += "\n\nInput Configuration:\n" + QJsonDocument(inputConfig).toJson(QJsonDocument::Compact);
        }
        innerData["content"] = content;

        // Wrap everything in "data" object for correct structure
        QJsonObject project;
        project["data"] = innerData;

        project_list << project;
        
        // Save individual file
        QString filename = QString("%1__%2.json").arg(m_outfile).arg(iteration);
        if (SaveFile(filename, project)) {
            fmt::print("✅ {} successfully written to disk\n", filename.toStdString());
        } else {
            fmt::print("❌ ERROR: Could not save file {}\n", filename.toStdString());
        }
        
        delete fullData;
    }
    
    fmt::print("🎉 Modular structure generation completed: {} datasets\n", project_list.size());
    return project_list;
}

QJsonObject SupraFitCli::generateIndependentDataTable(const QJsonObject& independentConfig)
{
    fmt::print("🔧 Generating independent data table...\n");
    
    QString source = independentConfig["Source"].toString();
    
    if (source == "generator") {
        QJsonObject generatorConfig = independentConfig["Generator"].toObject();
        QString type = generatorConfig["Type"].toString();
        
        if (type == "equations") {
            // Generate using DataGenerator equations with careful memory management
            fmt::print("🔧 Setting up DataGenerator for independent data...\n");
            
            // Setup generator configuration
            QJsonObject genData;
            genData["independent"] = generatorConfig["Variables"].toInt(1);
            genData["datapoints"] = generatorConfig["DataPoints"].toInt(20);
            genData["equations"] = generatorConfig["Equations"].toString("X");
            
            fmt::print("🔍 DEBUG: Independent config - DataPoints: {}, Variables: {}, Equations: {}\n", 
                      genData["datapoints"].toInt(), genData["independent"].toInt(), genData["equations"].toString().toStdString());
            
            // Create generator with explicit parent to manage memory
            DataGenerator* generator = new DataGenerator(this);
            
            // Set configuration
            generator->setJson(genData);
            
            // Apply random parameters if specified
            QJsonObject randomParams = generatorConfig["RandomParameters"].toObject();
            bool success = false;
            
            fmt::print("🔍 DEBUG: Random parameters empty: {}\n", randomParams.isEmpty());
            
            if (!randomParams.isEmpty()) {
                success = generator->EvaluateWithRandomParameters(randomParams);
            } else {
                success = generator->Evaluate();
            }
            
            if (!success) {
                fmt::print("❌ ERROR: DataGenerator failed for independent data\n");
                delete generator;
                return QJsonObject();
            }
            
            // Check if generator table is valid
            if (!generator->Table()) {
                fmt::print("❌ ERROR: Generator table is null\n");
                delete generator;
                return QJsonObject();
            }
            
            // Export table as JSON immediately to avoid pointer issues
            QJsonObject tableJson = generator->Table()->ExportTable(false);
            
            fmt::print("✅ Generated independent data: {} rows x {} cols (via clean pointer management)\n", 
                      generator->Table()->rowCount(), generator->Table()->columnCount());
            
            // Clean up generator immediately after use
            delete generator;
            generator = nullptr;
            
            fmt::print("🔍 DEBUG: DataGenerator cleaned up successfully\n");
            
            return tableJson;
            
        } else if (type == "model") {
            fmt::print("❌ ERROR: Model-based independent generation not yet implemented\n");
            return QJsonObject();
        }
        
    } else if (source == "file") {
        QJsonObject fileConfig = independentConfig["File"].toObject();
        return loadDataTableFromFile(fileConfig);
    }
    
    fmt::print("❌ ERROR: Unknown independent data source: {}\n", source.toStdString());
    return QJsonObject();
}

QJsonObject SupraFitCli::generateDependentDataTable(const QJsonObject& dependentConfig, const QJsonObject& independentTableJson)
{
    fmt::print("🔧 Generating dependent data table...\n");
    
    if (independentTableJson.isEmpty()) {
        fmt::print("❌ ERROR: Independent table JSON is empty\n");
        return QJsonObject();
    }
    
    QString source = dependentConfig["Source"].toString();
    
    if (source == "generator") {
        QJsonObject generatorConfig = dependentConfig["Generator"].toObject();
        QString type = generatorConfig["Type"].toString();
        
        if (type == "equations") {
            // Generate dependent data using equations with careful memory management
            fmt::print("🔧 Setting up DataGenerator for dependent data...\n");
            
            // Get independent data dimensions from JSON
            int dataPoints = independentTableJson["rows"].toInt();
            
            // Setup dependent generator configuration
            QJsonObject genData;
            genData["independent"] = generatorConfig["Series"].toInt(3);  // Number of dependent columns
            genData["datapoints"] = dataPoints;
            genData["equations"] = generatorConfig["Equations"].toString();
            
            fmt::print("🔍 DEBUG: Dependent config - DataPoints: {}, Series: {}, Equations: {}\n", 
                      dataPoints, genData["independent"].toInt(), genData["equations"].toString().toStdString());
            
            // Create generator with explicit parent to manage memory
            DataGenerator* generator = new DataGenerator(this);
            
            // Set configuration
            generator->setJson(genData);
            
            // Apply random parameters if specified
            QJsonObject randomParams = generatorConfig["RandomParameters"].toObject();
            bool success = false;
            
            fmt::print("🔍 DEBUG: Random parameters empty: {}\n", randomParams.isEmpty());
            
            if (!randomParams.isEmpty()) {
                success = generator->EvaluateWithRandomParameters(randomParams);
            } else {
                success = generator->Evaluate();
            }
            
            if (!success) {
                fmt::print("❌ ERROR: DataGenerator failed for dependent data\n");
                delete generator;
                return QJsonObject();
            }
            
            // Check if generator table is valid
            if (!generator->Table()) {
                fmt::print("❌ ERROR: Generator produced null dependent table\n");
                delete generator;
                return QJsonObject();
            }
            
            // Export dependent DataTable as JSON immediately to avoid pointer issues
            QJsonObject dependentTableJson = generator->Table()->ExportTable(false);
            
            fmt::print("✅ Generated dependent data table: {} rows x {} cols (via clean pointer management)\n", 
                      generator->Table()->rowCount(), generator->Table()->columnCount());
            
            // Clean up generator immediately after use
            delete generator;
            generator = nullptr;
            
            fmt::print("🔍 DEBUG: DataGenerator cleaned up successfully\n");
            
            return dependentTableJson;
            
        } else if (type == "model") {
            // Generate dependent data using DataGenerator's EvaluateWithModel - Claude Generated
            fmt::print("🔧 Setting up model-based dependent data generation with EvaluateWithModel...\n");

            QJsonObject modelConfig = generatorConfig["Model"].toObject();
            int modelId = modelConfig["ID"].toInt(1);

            fmt::print("🔍 DEBUG: Model-based generation - Model ID: {}\n", modelId);

            // Create DataClass with independent data and empty dependent structure - Claude Generated
            QPointer<DataClass> data = new DataClass();
            DataTable* independentTable = new DataTable(independentTableJson);
            data->setIndependentTable(independentTable);
            data->setIndependentRawTable(new DataTable(independentTableJson));
            data->setDataType(DataClassPrivate::Table);

            int dataPoints = independentTable->rowCount();
            int series = generatorConfig["Series"].toInt(2);

            // Create empty dependent data table with correct dimensions
            DataTable* dependentTable = new DataTable(dataPoints, series, data);
            data->setDependentTable(dependentTable);
            data->setDependentRawTable(new DataTable(dependentTable));

            // Set simulation parameters
            data->setSimulateDependent(series);
            data->setDataBegin(0);
            data->setDataEnd(dataPoints);

            fmt::print("🔍 DEBUG: Created DataClass with {} data points, {} series, simulation settings applied\n",
                dataPoints, series);

            // Use DataGenerator's EvaluateWithModel with random parameters - Claude Generated
            DataGenerator* generator = new DataGenerator();
            bool success = generator->EvaluateWithModel(modelId, data, generatorConfig);

            if (success) {
                DataTable* modelTable = generator->Table();
                if (modelTable && modelTable->rowCount() > 0 && modelTable->columnCount() > 0) {
                    QJsonObject dependentTableJson = modelTable->ExportTable(false);

                    // Extract enhanced content from DataClass and store globally for later use - Claude Generated
                    m_modelContent = data->getContent();
                    fmt::print("🔍 DEBUG: Extracted model content ({} chars) for later use\n", m_modelContent.length());

                    fmt::print("✅ Generated model-based dependent data: {} rows x {} cols\n",
                        modelTable->rowCount(), modelTable->columnCount());

                    delete generator;
                    delete data;
                    return dependentTableJson;
                } else {
                    fmt::print("❌ ERROR: DataGenerator ModelTable is empty or null\n");
                }
            } else {
                fmt::print("❌ ERROR: EvaluateWithModel failed\n");
            }

            delete generator;
            delete data;
            return QJsonObject();
        }
        
    } else if (source == "file") {
        // Load dependent data from file
        QJsonObject fileConfig = dependentConfig["File"].toObject();
        return loadDataTableFromFile(fileConfig);
    }
    
    fmt::print("❌ ERROR: Unknown dependent data source: {}\n", source.toStdString());
    return QJsonObject();
}

QJsonObject SupraFitCli::loadDataTableFromFile(const QJsonObject& fileConfig)
{
    fmt::print("📁 Loading data table from file...\n");
    
    QString path = fileConfig["Path"].toString();
    int startRow = fileConfig["StartRow"].toInt(0);
    int startCol = fileConfig["StartCol"].toInt(0);
    int rows = fileConfig["Rows"].toInt(-1);
    int cols = fileConfig["Cols"].toInt(-1);
    
    if (path.isEmpty()) {
        fmt::print("❌ ERROR: No file path specified\n");
        return QJsonObject();
    }
    
    fmt::print("📂 Loading file: {}\n", path.toStdString());
    fmt::print("🔍 Range: StartRow={}, StartCol={}, Rows={}, Cols={}\n", 
              startRow, startCol, rows, cols);
    
    // Create FileHandler and load the file - Claude Generated
    FileHandler* handler = new FileHandler(path, this);
    handler->LoadFile();
    
    if (!handler->FileSupported()) {
        fmt::print("❌ ERROR: File format not supported: {}\n", path.toStdString());
        delete handler;
        return QJsonObject();
    }
    
    if (!handler->getData()) {
        fmt::print("❌ ERROR: No data could be loaded from file: {}\n", path.toStdString());
        delete handler;
        return QJsonObject();
    }
    
    fmt::print("✅ File loaded successfully: {} rows x {} cols\n", 
              handler->getData()->rowCount(), handler->getData()->columnCount());
    
    // Calculate end positions based on rows/cols parameters
    int endRow = (rows > 0) ? startRow + rows - 1 : -1;
    int endCol = (cols > 0) ? startCol + cols - 1 : -1;
    
    // Extract the requested range using FileHandler's new functionality
    QPointer<DataTable> rangeTable = handler->getDataRange(startRow, endRow, startCol, endCol);
    
    if (!rangeTable) {
        fmt::print("❌ ERROR: Failed to extract requested data range\n");
        delete handler;
        return QJsonObject();
    }

    fmt::print("✅ Extracted data range: {} rows x {} cols\n", 
              rangeTable->rowCount(), rangeTable->columnCount());
    
    // Export the range as JSON
    QJsonObject tableJson = rangeTable->ExportTable(false);
    
    // Clean up
    delete handler;
    
    fmt::print("✅ Data table converted to JSON successfully\n");
    
    return tableJson;
}

QPointer<DataClass> SupraFitCli::loadDataFromFile(const QJsonObject& fileConfig)
{
    fmt::print("📁 Loading data from file...\n");
    
    QString path = fileConfig["Path"].toString();
    int startRow = fileConfig["StartRow"].toInt(0);
    int startCol = fileConfig["StartCol"].toInt(0);
    int rows = fileConfig["Rows"].toInt(-1);
    int cols = fileConfig["Cols"].toInt(-1);
    
    fmt::print("❌ ERROR: File loading not yet implemented\n");
    fmt::print("   Path: {}, StartRow: {}, StartCol: {}, Rows: {}, Cols: {}\n", 
              path.toStdString(), startRow, startCol, rows, cols);
    
    return nullptr;
}

QPointer<DataClass> SupraFitCli::applyNoise(QPointer<DataClass> data, const QJsonObject& noiseConfig, bool isIndependent)
{
    if (noiseConfig.isEmpty() || noiseConfig["Type"].toString() == "none") {
        return data;  // No noise to apply
    }
    
    QString type = noiseConfig["Type"].toString();
    fmt::print("🔊 Applying {} noise to {} data...\n", type.toStdString(), 
              isIndependent ? "independent" : "dependent");
    
    if (type == "exportMC" || type == "montecarlo" || type == "gaussian") {
        // All noise types use DataTable's PrepareMC functionality - Claude Generated
        
        // Get noise parameters with reasonable defaults
        double stddev = noiseConfig["StdDev"].toDouble(0.05);  // Default 5% noise
        int seed = noiseConfig["Seed"].toInt(42);
        QVector<int> columns;  // Empty means all columns
        
        if (noiseConfig.contains("Columns")) {
            QJsonArray colArray = noiseConfig["Columns"].toArray();
            for (const auto& col : colArray) {
                columns.append(col.toInt());
            }
        }
        
        fmt::print("🔍 DEBUG: Noise - Type: {}, StdDev: {}, Seed: {}, Columns: {}\n", 
                  type.toStdString(), stddev, seed, columns.isEmpty() ? "all" : QString::number(columns.size()).toStdString());
        
        // Setup random number generator and distribution
        std::mt19937 rng(seed);
        QVector<double> stddevVector;
        
        // Apply noise to appropriate table
        DataTable* targetTable = isIndependent ? data->IndependentModel() : data->DependentModel();
        if (!targetTable) {
            fmt::print("❌ ERROR: Target table is null\n");
            return data;
        }
        
        // Create stddev vector for each column
        for (int col = 0; col < targetTable->columnCount(); ++col) {
            stddevVector.append(stddev);
        }
        
        // Generate noisy table using PrepareMC
        QPointer<DataTable> noisyTable = targetTable->PrepareMC(stddevVector, rng, columns);
        
        if (!noisyTable) {
            fmt::print("❌ ERROR: Failed to generate noisy table\n");
            return data;
        }
        
        // Replace the table in DataClass
        if (isIndependent) {
            data->setIndependentTable(noisyTable);
            data->setIndependentRawTable(new DataTable(noisyTable));
        } else {
            data->setDependentTable(noisyTable);
            data->setDependentRawTable(new DataTable(noisyTable));
        }
        
        fmt::print("✅ Applied {} noise: {} rows x {} cols\n", 
                  type.toStdString(), noisyTable->rowCount(), noisyTable->columnCount());
        
    } else {
        fmt::print("❌ ERROR: Unknown noise type: {}\n", type.toStdString());
        fmt::print("   Supported types: none, exportMC, montecarlo, gaussian\n");
    }
    
    return data;
}

/*
SupraFitCli * SupraFitCli::Generate() const
{
    SupraFitCli *generated = new SupraFitCli;

    return generated;
}*/
