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
#include <QtCore/QDir>
#include <QtCore/QFile>
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
#include "src/capabilities/mlfeatureextractor.h"
#include "src/core/analyse.h"

#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"

#include "src/core/analyse.h"
#include "src/core/filehandler.h"
#include "src/core/jsonhandler.h"
#include "src/core/jsonutils.h"
#include "src/core/minimizer.h"
#include "src/core/toolset.h"
#include "src/version.h"

#include <fmt/color.h>
#include <fmt/core.h>

#include "suprafit_cli.h"

SupraFitCli::SupraFitCli()
{
    // Initialize JobManager for statistical analysis - Claude Generated
    m_jobmanager = new JobManager(this);
    
    // Initialize AnalysisManager for centralized analysis - Claude Generated
    m_analysisManager = new AnalysisManager(this);
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

    // Initialize JobManager for statistical analysis - Claude Generated
    m_jobmanager = new JobManager(this);
    
    // Initialize AnalysisManager for centralized analysis - Claude Generated
    m_analysisManager = new AnalysisManager(this);

    ParseMain();
}

SupraFitCli::~SupraFitCli()
{
}

void SupraFitCli::setControlJson(const QJsonObject& control)
{
    // Store original configuration for ML Pipeline - Claude Generated
    m_original_config = control;
    
    QStringList keys = control.keys();
#ifdef DEBUG_ON
    fmt::print("🔍 DEBUG setControlJson: Input JSON keys: {}\n", keys.join(", ").toStdString());
#endif
    
    for (const QString& key : keys) {
        if (key.compare("main", Qt::CaseInsensitive) == 0) {
            m_main = control[key].toObject();
#ifdef DEBUG_ON
            fmt::print("🔍 DEBUG setControlJson: Found Main section with keys: {}\n", m_main.keys().join(", ").toStdString());
#endif
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
#ifdef DEBUG_ON
            fmt::print("🔍 DEBUG setControlJson: Found Independent section with source: {}\n", 
                      m_independent["Source"].toString().toStdString());
#endif
        }
        
        if (key.compare("dependent", Qt::CaseInsensitive) == 0) {
            m_dependent = control[key].toObject();
            m_use_modular_structure = true;
            m_generate_dependent = true;  // Enable dependent data generation
            m_simulate_job = true;        // Enable simulation mode
#ifdef DEBUG_ON
            fmt::print("🔍 DEBUG setControlJson: Found Dependent section with source: {}\n", 
                      m_dependent["Source"].toString().toStdString());
#endif
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
#ifdef DEBUG_ON
                fmt::print("🔍 DEBUG: Found GenerateData configuration!\n");
#endif
                m_generate_dependent = true;
                m_generate_noisy_dependent = true;
                m_simulate_job = true;
                m_simulation = m_main[str].toObject();
#ifdef DEBUG_ON
                fmt::print("🔍 DEBUG: Set m_generate_dependent = true\n");
#endif
            }
            
            // Handle DataOnly task (simple load and save)
            if (str.compare("Tasks", Qt::CaseInsensitive) == 0) {
                QString task = m_main[str].toString();
                if (task.compare("DataOnly", Qt::CaseInsensitive) == 0) {
                    m_data_only = true;
#ifdef DEBUG_ON
                    qDebug() << "DataOnly task detected";
#endif
                }
                else if (task.compare("GenerateInputData", Qt::CaseInsensitive) == 0) {
                    m_generate_input_data = true;
#ifdef DEBUG_ON
                    qDebug() << "GenerateInputData task detected";
#endif
                }
                else if (task.compare("GenerateData", Qt::CaseInsensitive) == 0) {
                    m_generate_dependent = true;
                    m_generate_noisy_dependent = true;
                    m_simulate_job = true;
#ifdef DEBUG_ON
                    qDebug() << "GenerateData task detected";
#endif
                }
            }
        }
    }
    ParseMain();
}

bool SupraFitCli::LoadFile()
{
    std::cout << "🔧 DEBUG LoadFile: SupraFitCli::LoadFile() CALLED!" << std::endl;
    std::cout << "🔍 DEBUG LoadFile: Starting to load file: " << m_infile.toStdString() << std::endl;

    // Check if filename is valid
    if (m_infile.isEmpty() || m_infile.isNull()) {
#ifdef DEBUG_ON
        fmt::print("❌ DEBUG LoadFile: Input file is empty or null!\n");
#endif
        return false;
    }

    // First, try to load as configuration file using FileHandler (for data generation workflows)
    FileHandler* handler = new FileHandler(m_infile, this);
    handler->LoadFile();

    QJsonObject fileData = handler->getJsonData();

    // Check if this is a configuration file (Main, Independent, Dependent structure)
    if (fileData.contains("Main") || (fileData.contains("Independent") && fileData.contains("Dependent"))) {

        std::cout << "🔍 DEBUG LoadFile: Detected configuration file format" << std::endl;

        // This is a configuration file - use FileHandler logic
        m_toplevel = fileData;
        m_main = m_toplevel["Main"].toObject();
        m_models = m_toplevel["models"].toObject();
        m_analyse = m_toplevel["controller"].toObject();

        // Set additional configuration sections for AddModels workflow
        if (fileData.contains("AddModels")) {
            m_models = fileData["AddModels"].toObject();
#ifdef DEBUG_ON
            fmt::print("🔍 DEBUG LoadFile: Found AddModels section with {} models\n", m_models.size());
#endif
        }

        if (fileData.contains("PostFitAnalysis")) {
            m_analyse = fileData["PostFitAnalysis"].toObject();
#ifdef DEBUG_ON
            fmt::print("🔍 DEBUG LoadFile: Found PostFitAnalysis section\n");
#endif
        }

        std::cout << "✅ DEBUG LoadFile: Configuration file loaded successfully, returning true" << std::endl;

        delete handler;
        return true;

    } else if (fileData.contains("datatype") || fileData.contains("data")) {

        std::cout << "🔍 DEBUG LoadFile: Detected project file format, using ProjectManager" << std::endl;

        // This is a project file - use ProjectManager
        delete handler;

        SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();
        bool success = projectManager.loadProject(m_infile);

        if (success) {
            // Update legacy data structures for backward compatibility
            QWeakPointer<DataClass> currentProject = projectManager.getCurrentProject();
            if (!currentProject.isNull()) {
                QSharedPointer<DataClass> project = currentProject.toStrongRef();
                if (project) {
                    m_data = project.data(); // Set current data for legacy compatibility
                    m_toplevel = projectManager.getProjectAsJson(); // Set toplevel for legacy compatibility

                    // Update data vector for legacy compatibility
                    m_data_vector.clear();
                    m_data_vector = projectManager.getAllProjectsAsJson();

#ifdef DEBUG_ON
                    fmt::print("✅ DEBUG LoadFile: Successfully loaded project using ProjectManager\n");
                    fmt::print("🔍 DEBUG LoadFile: Project UUID: {}\n", project->UUID().toStdString());
#endif
                }
            }
        }

        return success;

    } else {

        std::cout << "❌ DEBUG LoadFile: Unknown file format" << std::endl;
        std::cout << "🔍 DEBUG LoadFile: File keys: " << fileData.keys().join(", ").toStdString() << std::endl;

        delete handler;
        return false;
    }
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
    // Only use OutFile from JSON if not already set via -o option
    if (m_main.contains("OutFile") && m_outfile.isEmpty()) {
        // Only use OutFile from JSON if not already set via -o option
        if (m_outfile.isEmpty()) {
            m_outfile = m_main["OutFile"].toString();
        }
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
    // Use ProjectManager exclusively for JSON structure compliance - Claude Generated
    SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();
    QString tempProjectId = projectManager.createProjectFromJson(data, "Temporary CLI Project");

    if (!tempProjectId.isEmpty()) {
        bool success = projectManager.saveProject(file, tempProjectId);
        // Clean up temporary project
        projectManager.removeProject(tempProjectId);

        if (success) {
            std::cout << file.toStdString() << " successfully written to disk using ProjectManager" << std::endl;
        } else {
            std::cout << "Error: Failed to save " << file.toStdString() << " using ProjectManager" << std::endl;
        }
        return success;
    }

    std::cout << "Error: Failed to create temporary project for " << file.toStdString() << std::endl;
    return false;
}

bool SupraFitCli::SaveFiles(const QString& file, const QVector<QJsonObject>& projects)
{
    // Use ProjectManager to save all projects as a batch
    SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();

    // Option 1: Save as individual files (preserve existing behavior)
    bool allSuccess = true;
    for (int i = 0; i < projects.size(); ++i) {
        QString individualFile = m_outfile + "_" + file + "_" + QString::number(i) + "." + m_extension;
        bool success = SaveFile(individualFile, projects[i]);
        allSuccess = allSuccess && success;
    }

    // Option 2: Also offer batch save capability
    QString batchFile = m_outfile + "_" + file + "_batch." + m_extension;
    bool batchSuccess = projectManager.saveAllProjects(batchFile);

    return allSuccess;
}

bool SupraFitCli::SaveFile()
{
    // Use ProjectManager to save current project
    SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();

    // Use ProjectManager exclusively for JSON structure compliance - Claude Generated
    QWeakPointer<DataClass> currentProject = projectManager.getCurrentProject();
    if (!currentProject.isNull()) {
        QSharedPointer<DataClass> project = currentProject.toStrongRef();
        if (project) {
            bool success = projectManager.saveProject(m_outfile + m_extension, project->UUID());
            if (success) {
                std::cout << m_outfile.toStdString() << " successfully written to disk using ProjectManager" << std::endl;
                return true;
            } else {
                std::cout << "Error: Failed to save " << m_outfile.toStdString() << " using ProjectManager" << std::endl;
                return false;
            }
        }
    }

    // No fallback - ProjectManager must handle all saves for structure compliance
    std::cout << "Error: No current project available for saving" << std::endl;
    return false;
}

void SupraFitCli::PrintFileContent(int index)
{
    // Use ProjectManager to get current project data
    SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();
    QWeakPointer<DataClass> currentProject = projectManager.getCurrentProject();

    if (currentProject.isNull()) {
        std::cout << "No current project loaded in ProjectManager" << std::endl;
        return;
    }

    QSharedPointer<DataClass> project = currentProject.toStrongRef();
    if (!project) {
        std::cout << "Current project reference is null" << std::endl;
        return;
    }

    if (project->DataPoints() == 0) {
        std::cout << "Project contains no data points" << std::endl;
        return;
    }

    std::cout << project->Data2Text().toStdString() << std::endl;

    // Get project JSON for model access
    QJsonObject projectJson = projectManager.getProjectAsJson();
    int i = 1;

    for (const QString& key : projectJson.keys()) {
        if (key.contains("model")) {
            QSharedPointer<AbstractModel> model = JsonHandler::Json2Model(projectJson[key].toObject(), project.data());
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
    // Use ProjectManager to get all projects for processing
    SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();
    QVector<QJsonObject> projects = projectManager.getAllProjectsAsJson();

    if (projects.isEmpty()) {
#ifdef DEBUG_ON
        qDebug() << "No data to process in Work() - ProjectManager has no projects";
#endif
        return;
    }

    if (m_models.isEmpty()) {
#ifdef DEBUG_ON
        qDebug() << "No models configured for Work()";
#endif
        return;
    }
    
    if (m_jobs.isEmpty()) {
#ifdef DEBUG_ON
        qDebug() << "No jobs configured for Work()";
#endif
        return;
    }

    qDebug() << "Processing" << projects.size() << "datasets with" << m_models.keys().size() << "models using ProjectManager";

    for (int i = 0; i < projects.size(); ++i) {
        const auto& project = projects[i];

        qDebug() << "Processing dataset" << (i + 1) << "/" << projects.size();

        QJsonObject result = PerformeJobs(project, m_models, m_jobs);
        
        if (!result.isEmpty()) {
            // Save results for this dataset using ProjectManager
            QString outputFile = QString("%1_%2.json").arg(m_outfile).arg(i);
            if (SaveFile(outputFile, result)) {
                qDebug() << "Saved results to:" << outputFile;
            } else {
                qWarning() << "Failed to save results to:" << outputFile;
            }
        }
    }

    qDebug() << "Work() completed for all datasets using ProjectManager";
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
        // Only use OutFile from JSON if not already set via -o option
        if (m_outfile.isEmpty()) {
            m_outfile = m_mainjson["OutFile"].toString();
        }
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
    
    // Only use OutFile from JSON if not already set via -o option
    if (m_main.contains("OutFile") && m_outfile.isEmpty()) {
        // Only use OutFile from JSON if not already set via -o option
        if (m_outfile.isEmpty()) {
            m_outfile = m_main["OutFile"].toString();
        }
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
    
    // Claude Generated - Add missing fields expected by tests
    export_object["uuid"] = dataObject["uuid"];
    export_object["DataType"] = dataObject["DataType"];
    
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
    // Only use OutFile from JSON if not already set via -o option
    if (m_main.contains("OutFile") && m_outfile.isEmpty()) {
        // Only use OutFile from JSON if not already set via -o option
        if (m_outfile.isEmpty()) {
            m_outfile = m_main["OutFile"].toString();
        }
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
    
    // Claude Generated - Add missing fields expected by tests
    export_object["uuid"] = dataObject["uuid"];
    export_object["DataType"] = dataObject["DataType"];
    
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
    fmt::print("📊 SUPRAFIT FILE ANALYSIS (using AnalysisManager)\n");
    fmt::print(std::string(80, '=') + "\n\n");
    
    // Use centralized AnalysisManager for file analysis - Claude Generated
    QJsonObject analysisResults = m_analysisManager->analyzeFile(m_infile);
    
    if (!analysisResults["success"].toBool()) {
        fmt::print("❌ Analysis failed: {}\n", analysisResults["error"].toString().toStdString());
        return;
    }
    
    // Display results in CLI format
    displayAnalysisResults(analysisResults);
    
    fmt::print("\n" + std::string(80, '=') + "\n");
    fmt::print("✅ FILE ANALYSIS COMPLETE (centralized approach)\n");
    fmt::print(std::string(80, '=') + "\n\n");
}

void SupraFitCli::displayAnalysisResults(const QJsonObject& results)
{
    // Display file information
    QJsonObject fileInfo = results["fileInfo"].toObject();
    fmt::print("📁 FILE INFORMATION:\n");
    fmt::print("   Input file: {}\n", fileInfo["inputFile"].toString().toStdString());
    fmt::print("   File size: {} bytes\n", fileInfo["fileSize"].toInt());
    fmt::print("   File extension: {}\n", fileInfo["fileExtension"].toString().toStdString());
    fmt::print("   File exists: {}\n", fileInfo["exists"].toBool() ? "Yes" : "No");
    fmt::print("   Last modified: {}\n", fileInfo["lastModified"].toString().toStdString());
    
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

    // Enhanced Model Analysis - Claude Generated  
    fmt::print("\n🔬 MODEL STATISTICS TABLE:\n");
    
    QVector<ModelStatistics> modelStatistics;
    
    // Search for model objects in toplevel JSON (model_0, model_1, etc.)
    for (auto it = m_toplevel.begin(); it != m_toplevel.end(); ++it) {
        QString key = it.key();
        if (key.startsWith("model_") && it.value().isObject()) {
            QJsonObject modelObj = it.value().toObject();
            ModelStatistics stats = extractModelStatistics(key, modelObj);
            modelStatistics.append(stats);
        }
    }
    
    // Display the table
    displayModelStatisticsTable(modelStatistics);
    
    // Show post-processing details if requested - Claude Generated
    if (m_show_post_processing_details) {
        displayPostProcessingDetails(modelStatistics);
    }
    
    // Show additional parameter details for models with statistics
    if (!modelStatistics.isEmpty()) {
        fmt::print("\n📋 PARAMETER DETAILS:\n");
        for (const auto& stats : modelStatistics) {
            if (stats.hasValidStats) {
                // Find the original model object to get parameter details
                QJsonObject modelObj = m_toplevel[stats.key].toObject();
                QJsonObject data = modelObj["data"].toObject();
                
                if (!data.isEmpty()) {
                    QJsonObject globalParamObj = data["globalParameter"].toObject();
                    if (!globalParamObj.isEmpty() && globalParamObj.contains("data")) {
                        fmt::print("   {} ({}) - Global parameters:\n", 
                                 stats.key.toStdString(), stats.name.toStdString());
                        QJsonObject paramData = globalParamObj["data"].toObject();
                        QString header = globalParamObj["header"].toString();
                        for (auto paramIt = paramData.begin(); paramIt != paramData.end(); ++paramIt) {
                            fmt::print("     {}: {}\n", 
                                     header.isEmpty() ? QString("P%1").arg(paramIt.key()).toStdString() : header.toStdString(),
                                     paramIt.value().toString().toStdString());
                        }
                    }
                }
            }
        }
        
        // Show debug info for models without statistics
        for (const auto& stats : modelStatistics) {
            if (!stats.hasValidStats) {
                QJsonObject modelObj = m_toplevel[stats.key].toObject();
                fmt::print("   {} - Available fields: ", stats.key.toStdString());
                for (auto statIt = modelObj.begin(); statIt != modelObj.end(); ++statIt) {
                    fmt::print("{} ", statIt.key().toStdString());
                }
                fmt::print("\n");
            }
        }
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

// Model statistics table formatting helper functions - Claude Generated
SupraFitCli::ModelStatistics SupraFitCli::extractModelStatistics(const QString& key, const QJsonObject& modelObj)
{
    ModelStatistics stats;
    stats.key = key;
    stats.hasValidStats = false;
    
    // Extract model name - try different possible field names
    stats.name = modelObj["name"].toString();
    if (stats.name.isEmpty()) {
        stats.name = modelObj["model"].toString();  // Alternative field name
    }
    if (stats.name.isEmpty()) {
        stats.name = key;  // Use key as fallback
    }
    
    // Check convergence status
    bool converged = modelObj["converged"].toBool(false);
    stats.status = converged ? "✅ Conv" : "❌ Failed";
    
    // Extract fit statistics - SupraFit specific structure
    stats.sse = modelObj["SSE"].toDouble(-1);
    stats.sae = modelObj["SAE"].toDouble(-1);  // Sum of Absolute Errors
    stats.aic = modelObj["AIC"].toDouble(-999);  // Akaike Information Criterion
    stats.aicc = modelObj["AICc"].toDouble(-999); // Corrected AIC
    
    // Try to get parameter counts from data structure
    QJsonObject data = modelObj["data"].toObject();
    stats.globalParams = -1;
    stats.localParams = -1;
    
    if (!data.isEmpty()) {
        QJsonObject globalParamObj = data["globalParameter"].toObject();
        if (!globalParamObj.isEmpty()) {
            stats.globalParams = globalParamObj["rows"].toInt(-1);
        }
        
        QJsonObject localParamObj = data["localParameter"].toObject();
        if (!localParamObj.isEmpty()) {
            stats.localParams = localParamObj["rows"].toInt(-1);
        }
    }
    
    // Analyze post-processing methods - Claude Generated
    if (!data.isEmpty() && data.contains("methods")) {
        QJsonObject methods = data["methods"].toObject();
        QVector<QJsonObject> modelVector = {modelObj};
        
        for (auto it = methods.begin(); it != methods.end(); ++it) {
            QJsonObject method = it.value().toObject();
            
            // Skip empty method blocks
            if (method.isEmpty())
                continue;
            
            // Look for controller - it might be directly in method or nested under numeric keys
            QJsonObject controller;
            bool foundController = false;
            
            if (method.contains("controller")) {
                controller = method["controller"].toObject();
                foundController = true;
            } else {
                // Check for nested controller under numeric keys (like "0", "1", etc.)
                for (auto subIt = method.begin(); subIt != method.end(); ++subIt) {
                    if (subIt.value().isObject()) {
                        QJsonObject subMethod = subIt.value().toObject();
                        if (subMethod.contains("controller")) {
                            controller = subMethod["controller"].toObject();
                            foundController = true;
                            break;
                        }
                    }
                }
            }
            
            if (!foundController || !controller.contains("Method"))
                continue;
                
            int methodType = controller["Method"].toInt();
            
            switch (methodType) {
                case 1: // MonteCarlo
                    stats.mcBlocks++;
                    if (!stats.postProcessingData.contains("MonteCarlo") || stats.postProcessingData["MonteCarlo"].isNull()) {
                        stats.postProcessingData["MonteCarlo"] = StatisticTool::CalculateMCMetrics(modelVector, 1);
                    }
                    break;
                case 2: // WeakenedGridSearch
                    stats.wgsBlocks++;
                    if (!stats.postProcessingData.contains("WeakenedGridSearch") || stats.postProcessingData["WeakenedGridSearch"].isNull()) {
                        stats.postProcessingData["WeakenedGridSearch"] = StatisticTool::CalculateWGSMetrics(modelVector);
                    }
                    break;
                case 3: // ModelComparison
                    stats.modelCompBlocks++;
                    if (!stats.postProcessingData.contains("ModelComparison") || stats.postProcessingData["ModelComparison"].isNull()) {
                        stats.postProcessingData["ModelComparison"] = StatisticTool::CalculateModelComparisonMetrics(modelVector);
                    }
                    break;
                case 4: // CrossValidation
                    stats.cvBlocks++;
                    if (!stats.postProcessingData.contains("CrossValidation") || stats.postProcessingData["CrossValidation"].isNull()) {
                        stats.postProcessingData["CrossValidation"] = StatisticTool::CalculateCVMetrics(modelVector, 1, 3);
                    }
                    break;
                case 5: // Reduction
                    stats.reductionBlocks++;
                    if (!stats.postProcessingData.contains("Reduction") || stats.postProcessingData["Reduction"].isNull()) {
                        stats.postProcessingData["Reduction"] = StatisticTool::CalculateReductionMetrics(modelVector, 0.1);
                    }
                    break;
                case 6: // FastConfidence
                    stats.fastConfBlocks++;
                    if (!stats.postProcessingData.contains("FastConfidence") || stats.postProcessingData["FastConfidence"].isNull()) {
                        stats.postProcessingData["FastConfidence"] = StatisticTool::CalculateFastConfidenceMetrics(modelVector);
                    }
                    break;
                case 7: // GlobalSearch
                    stats.globalBlocks++;
                    if (!stats.postProcessingData.contains("GlobalSearch") || stats.postProcessingData["GlobalSearch"].isNull()) {
                        stats.postProcessingData["GlobalSearch"] = StatisticTool::CalculateGlobalSearchMetrics(modelVector);
                    }
                    break;
            }
        }
        
        stats.totalPPBlocks = stats.mcBlocks + stats.wgsBlocks + stats.modelCompBlocks + 
                             stats.cvBlocks + stats.reductionBlocks + stats.fastConfBlocks + stats.globalBlocks;
    }
    
    // Check if we have any valid statistics
    stats.hasValidStats = (stats.sse >= 0 || stats.sae >= 0 || stats.aic > -999 || 
                          stats.aicc > -999 || stats.globalParams > 0 || stats.localParams > 0);
    
    return stats;
}

void SupraFitCli::displayModelStatisticsTable(const QVector<ModelStatistics>& models)
{
    if (models.isEmpty()) {
        fmt::print("   No fitted models found in this file\n");
        return;
    }
    
    // Calculate column widths based on content
    int keyWidth = 8;  // "Model" header minimum
    int nameWidth = 10; // "Name" header minimum
    int statusWidth = 8; // "Status" header minimum
    int sseWidth = 10;  // "SSE" header + scientific notation
    int saeWidth = 8;   // "SAE" header minimum
    int aicWidth = 8;   // "AIC" header minimum
    int aiccWidth = 8;  // "AICc" header minimum
    int globalWidth = 8; // "Global" header minimum
    int localWidth = 7;  // "Local" header minimum
    int ppWidth = 12;   // "Post-Proc" header minimum
    
    // Calculate actual widths needed
    for (const auto& model : models) {
        keyWidth = qMax(keyWidth, model.key.length());
        nameWidth = qMax(nameWidth, model.name.length());
        statusWidth = qMax(statusWidth, model.status.length());
        if (model.sse >= 0) sseWidth = qMax(sseWidth, 10); // Scientific notation: 1.23e-05
        if (model.sae >= 0) saeWidth = qMax(saeWidth, 8);
        if (model.aic > -999) aicWidth = qMax(aicWidth, 8);
        if (model.aicc > -999) aiccWidth = qMax(aiccWidth, 8);
        if (model.globalParams > 0) globalWidth = qMax(globalWidth, QString::number(model.globalParams).length());
        if (model.localParams > 0) localWidth = qMax(localWidth, QString::number(model.localParams).length());
        
        // Calculate post-processing summary width
        QString ppSummary;
        if (model.totalPPBlocks > 0) {
            QStringList ppMethods;
            if (model.mcBlocks > 0) ppMethods << QString("%1 MC").arg(model.mcBlocks);
            if (model.cvBlocks > 0) ppMethods << QString("%1 CV").arg(model.cvBlocks);
            if (model.wgsBlocks > 0) ppMethods << QString("%1 WGS").arg(model.wgsBlocks);
            if (model.modelCompBlocks > 0) ppMethods << QString("%1 MCo").arg(model.modelCompBlocks);
            if (model.reductionBlocks > 0) ppMethods << QString("%1 Red").arg(model.reductionBlocks);
            if (model.fastConfBlocks > 0) ppMethods << QString("%1 FCo").arg(model.fastConfBlocks);
            if (model.globalBlocks > 0) ppMethods << QString("%1 Glo").arg(model.globalBlocks);
            ppSummary = ppMethods.join(", ");
        } else {
            ppSummary = "-";
        }
        ppWidth = qMax(ppWidth, ppSummary.length());
    }
    
    // Print table header
    std::string separator = "+";
    separator += std::string(keyWidth + 2, '-') + "+";
    separator += std::string(nameWidth + 2, '-') + "+";
    separator += std::string(statusWidth + 2, '-') + "+";
    separator += std::string(sseWidth + 2, '-') + "+";
    separator += std::string(saeWidth + 2, '-') + "+";
    separator += std::string(aicWidth + 2, '-') + "+";
    separator += std::string(aiccWidth + 2, '-') + "+";
    separator += std::string(globalWidth + 2, '-') + "+";
    separator += std::string(localWidth + 2, '-') + "+";
    separator += std::string(ppWidth + 2, '-') + "+";
    
    fmt::print("{}\n", separator);
    fmt::print("| {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} |\n",
               "Model", keyWidth,
               "Name", nameWidth,
               "Status", statusWidth,
               "SSE", sseWidth,
               "SAE", saeWidth,
               "AIC", aicWidth,
               "AICc", aiccWidth,
               "Global", globalWidth,
               "Local", localWidth,
               "Post-Proc", ppWidth);
    fmt::print("{}\n", separator);
    
    // Print data rows
    for (const auto& model : models) {
        std::string sseStr = model.sse >= 0 ? fmt::format("{:.2e}", model.sse) : "N/A";
        std::string saeStr = model.sae >= 0 ? fmt::format("{:.4f}", model.sae) : "N/A";
        std::string aicStr = model.aic > -999 ? fmt::format("{:.2f}", model.aic) : "N/A";
        std::string aiccStr = model.aicc > -999 ? fmt::format("{:.2f}", model.aicc) : "N/A";
        std::string globalStr = model.globalParams > 0 ? std::to_string(model.globalParams) : "N/A";
        std::string localStr = model.localParams > 0 ? std::to_string(model.localParams) : "N/A";
        
        // Format post-processing summary
        QString ppSummary;
        if (model.totalPPBlocks > 0) {
            QStringList ppMethods;
            if (model.mcBlocks > 0) ppMethods << QString("%1 MC").arg(model.mcBlocks);
            if (model.cvBlocks > 0) ppMethods << QString("%1 CV").arg(model.cvBlocks);
            if (model.wgsBlocks > 0) ppMethods << QString("%1 WGS").arg(model.wgsBlocks);
            if (model.modelCompBlocks > 0) ppMethods << QString("%1 MCo").arg(model.modelCompBlocks);
            if (model.reductionBlocks > 0) ppMethods << QString("%1 Red").arg(model.reductionBlocks);
            if (model.fastConfBlocks > 0) ppMethods << QString("%1 FCo").arg(model.fastConfBlocks);
            if (model.globalBlocks > 0) ppMethods << QString("%1 Glo").arg(model.globalBlocks);
            ppSummary = ppMethods.join(", ");
        } else {
            ppSummary = "-";
        }
        
        fmt::print("| {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} |\n",
                   model.key.toStdString(), keyWidth,
                   model.name.toStdString(), nameWidth,
                   model.status.toStdString(), statusWidth,
                   sseStr, sseWidth,
                   saeStr, saeWidth,
                   aicStr, aicWidth,
                   aiccStr, aiccWidth,
                   globalStr, globalWidth,
                   localStr, localWidth,
                   ppSummary.toStdString(), ppWidth);
    }
    fmt::print("{}\n", separator);
}

// Helper function to display post-processing results using Print::TextFromConfidence - Claude Generated
void SupraFitCli::displayPostProcessingMethod(const QString& methodName, const QString& emoji, 
                                              int blockCount, const QJsonObject& methodData, 
                                              int methodType) {
    fmt::print("  {} {} ({} block{}):\n", emoji.toStdString(), methodName.toStdString(), 
              blockCount, blockCount == 1 ? "" : "s");
    
    // Create controller JSON for Print::TextFromConfidence
    QJsonObject controller;
    controller["Method"] = methodType;
    controller["EntropyBins"] = 30;
    
    // Method-specific controller settings
    if (methodData.contains("variance")) {
        controller["Variance"] = methodData["variance"].toDouble();
        controller["VarianceSource"] = 1; // Standard variance source
    }
    if (methodData.contains("cv_type")) {
        controller["CVType"] = methodData["cv_type"].toInt();
    }
    if (methodData.contains("cv_x")) {
        controller["X"] = methodData["cv_x"].toInt();
    }
    
    // Process parameters for all methods that have model/parameter structure
    if (methodData.contains("models") && methodData["models"].isArray()) {
        QJsonArray models_array = methodData["models"].toArray();
        for (const auto& modelValue : models_array) {
            QJsonObject modelData = modelValue.toObject();
            if (modelData.contains("parameters") && modelData["parameters"].isArray()) {
                QJsonArray params = modelData["parameters"].toArray();
                for (const auto& paramValue : params) {
                    QJsonObject param = paramValue.toObject();
                    
                    // Convert our JSON structure to the format expected by Print::TextFromConfidence
                    QJsonObject result;
                    result["name"] = param["name"];
                    result["type"] = param["type"];
                    result["value"] = param.contains("mean") ? param["mean"] : param["value"];
                    
                    // Create confidence object
                    QJsonObject confidence;
                    confidence["upper"] = param.contains("confidence_upper") ? 
                                        param["confidence_upper"] : param["upper"];
                    confidence["lower"] = param.contains("confidence_lower") ? 
                                        param["confidence_lower"] : param["lower"];
                    confidence["error"] = param.contains("confidence_error") ? 
                                        param["confidence_error"].toDouble() : 95.0;
                    result["confidence"] = confidence;
                    
                    // Create data object with raw data
                    QJsonObject data;
                    if (param.contains("raw_data")) {
                        data["raw"] = param["raw_data"];
                    } else {
                        data["raw"] = "";
                    }
                    result["data"] = data;
                    
                    // Add index for local parameters
                    if (param.contains("index")) {
                        result["index"] = param["index"];
                    }
                    
                    // Use Print::TextFromConfidence and convert HTML to raw text
                    QString htmlOutput = Print::TextFromConfidence(result, controller);
                    QString consoleOutput = Print::Html2Raw(htmlOutput);
                    fmt::print("{}\n", consoleOutput.toStdString());
                }
            }
        }
    }
    
    // Method-specific summary information
    if (methodData.contains("parameter_averages") && methodData["parameter_averages"].isArray()) {
        QJsonArray avgParams = methodData["parameter_averages"].toArray();
        fmt::print("    📊 Parameter Summary:\n");
        for (const auto& avgValue : avgParams) {
            QJsonObject avg = avgValue.toObject();
            fmt::print("       {}: Avg σ={:.4f}, Avg H={:.2f} ({} model{})\n",
                     avg["parameter"].toString().toStdString(),
                     avg["avg_stddev"].toDouble(),
                     avg["avg_entropy"].toDouble(),
                     avg["model_count"].toInt(),
                     avg["model_count"].toInt() == 1 ? "" : "s");
        }
        fmt::print("\n");
    }
}

// Post-processing details display function - Claude Generated  
void SupraFitCli::displayPostProcessingDetails(const QVector<ModelStatistics>& models)
{
    if (models.isEmpty()) {
        fmt::print("   No models found for post-processing details\n");
        return;
    }
    
    fmt::print("\n📊 POST-PROCESSING DETAILS:\n");
    
    for (const auto& model : models) {
        if (model.totalPPBlocks == 0) {
            fmt::print("\n{} ({}):\n", model.key.toStdString(), model.name.toStdString());
            fmt::print("  No post-processing methods found\n");
            continue;
        }
        
        fmt::print("\n{} ({}):\n", model.key.toStdString(), model.name.toStdString());
        
        // Display Monte Carlo results
        if (model.mcBlocks > 0 && !model.postProcessingData["MonteCarlo"].isUndefined()) {
            displayPostProcessingMethod("Monte Carlo Analysis", "🎲", model.mcBlocks, 
                                      model.postProcessingData["MonteCarlo"].toObject(), 1);
        }
        
        // Display Cross-Validation results
        if (model.cvBlocks > 0 && !model.postProcessingData["CrossValidation"].isUndefined()) {
            displayPostProcessingMethod("Cross-Validation", "🔄", model.cvBlocks, 
                                      model.postProcessingData["CrossValidation"].toObject(), 4);
        }
        
        // Display Parameter Reduction results
        if (model.reductionBlocks > 0 && !model.postProcessingData["Reduction"].isUndefined()) {
            displayPostProcessingMethod("Parameter Reduction", "🔬", model.reductionBlocks, 
                                      model.postProcessingData["Reduction"].toObject(), 5);
        }
        
        // Display Weakened Grid Search results  
        if (model.wgsBlocks > 0 && !model.postProcessingData["WeakenedGridSearch"].isUndefined()) {
            displayPostProcessingMethod("Weakened Grid Search", "🔍", model.wgsBlocks, 
                                      model.postProcessingData["WeakenedGridSearch"].toObject(), 2);
        }
        
        // Display Model Comparison results
        if (model.modelCompBlocks > 0 && !model.postProcessingData["ModelComparison"].isUndefined()) {
            displayPostProcessingMethod("Model Comparison", "⚖️", model.modelCompBlocks, 
                                      model.postProcessingData["ModelComparison"].toObject(), 3);
        }
        
        // Display Fast Confidence results
        if (model.fastConfBlocks > 0 && !model.postProcessingData["FastConfidence"].isUndefined()) {
            displayPostProcessingMethod("Fast Confidence", "⚡", model.fastConfBlocks, 
                                      model.postProcessingData["FastConfidence"].toObject(), 6);
        }
        
        // Display Global Search results
        if (model.globalBlocks > 0 && !model.postProcessingData["GlobalSearch"].isUndefined()) {
            displayPostProcessingMethod("Global Search", "🌐", model.globalBlocks, 
                                      model.postProcessingData["GlobalSearch"].toObject(), 7);
        }
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
#ifdef DEBUG_ON
        fmt::print("🔍 DEBUG: m_main.isEmpty() = {}, m_simulation.isEmpty() = {}\n", 
                  m_main.isEmpty(), m_simulation.isEmpty());
#endif
#ifdef DEBUG_ON
        fmt::print("🔍 DEBUG: m_main keys: {}\n", m_main.keys().join(", ").toStdString());
        fmt::print("🔍 DEBUG: m_simulation keys: {}\n", m_simulation.keys().join(", ").toStdString());
#endif
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
    // Only use OutFile from JSON if not already set via -o option
    if (m_main.contains("OutFile") && m_outfile.isEmpty()) {
        // Only use OutFile from JSON if not already set via -o option
        if (m_outfile.isEmpty()) {
            m_outfile = m_main["OutFile"].toString();
        }
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
        
        // Wrap in correct SupraFit project structure - no fields outside "data" object
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
        modelStats["AIC"] = model->GetAIC();
        modelStats["AICc"] = model->GetAICc();
        modelStats["SEy"] = model->SEy();
        modelStats["ChiSquared"] = model->GetChiSquare();
        modelStats["RSquared"] = model->GetRSquared();
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
        
        // Only use OutFile from JSON if not already set via -o option
        if (m_outfile.isEmpty()) {
            if (!m_main.contains("OutFile")) {
                fmt::print("❌ ERROR: OutFile is required in Main section when -o option is not used\n");
                return project_list;
            }
            setOutFile(m_main["OutFile"].toString());
        }
        qDebug() << m_outfile << m_extension;
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

        // Transfer ML RawData from model generation if available - Claude Generated
        if (!m_mlRawData.isEmpty()) {
            fullData->setRawData(m_mlRawData);
            fmt::print("🔍 DEBUG: Transferred ML RawData with {} keys to final DataClass\n", m_mlRawData.keys().size());
        }

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

        // Claude Generated - Add missing fields expected by tests
        project["uuid"] = innerData["uuid"];
        project["DataType"] = innerData["DataType"];

        project_list << project;
        
        // Save individual file
        QString filename = QString("%1-%2%3").arg(m_outfile).arg(iteration).arg(m_extension);
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

                    // Extract ML RawData and store globally for later transfer - Claude Generated
                    m_mlRawData = data->RawData();
                    fmt::print("🔍 DEBUG: Extracted ML RawData with {} keys for later transfer\n", m_mlRawData.keys().size());

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

// ML Pipeline integration - Claude Generated
QVector<QJsonObject> SupraFitCli::ProcessMLPipeline()
{
    /**
     * @brief Complete ML pipeline: data generation → model fitting → evaluation
     * Combines DataGenerator simulation with multi-model fitting and statistical evaluation
     * for ML training data generation and model comparison studies.
     */
    fmt::print("🚀 Starting ML Pipeline: DataGenerator → Model Fitting → Statistical Evaluation\n");
    
    QVector<QJsonObject> results;
    
    // Step 1: Generate data using existing modular structure
    QVector<QJsonObject> simulatedData;
    if (m_use_modular_structure) {
        simulatedData = GenerateDataWithModularStructure();
    } else {
        simulatedData = GenerateDataWithDataGenerator();
    }
    
    if (simulatedData.isEmpty()) {
        fmt::print("❌ No data generated, ML pipeline aborted\n");
        return results;
    }
    
    fmt::print("✅ Generated {} datasets for ML pipeline\n", simulatedData.size());
    
    // Step 2: Process each dataset with multiple models
    for (int i = 0; i < simulatedData.size(); ++i) {
        const QJsonObject& dataset = simulatedData[i];
        
        fmt::print("🔧 Processing dataset {} with multiple models...\n", i + 1);
        
        // Create DataClass from generated data
        QPointer<DataClass> data = new DataClass();
        if (dataset.contains("data")) {
            data->ImportData(dataset["data"].toObject());
        } else {
            data->ImportData(dataset);
        }
        
        // Define models to test (from configuration or default set)
        QJsonObject modelsConfig;
        QJsonObject globalAnalysisConfig;

        // Debug: Check what configuration sections are available
        fmt::print("🔍 DEBUG ProcessMLPipeline: Available config sections: {}\n", m_original_config.keys().join(", ").toStdString());
        fmt::print("🔍 DEBUG ProcessMLPipeline: Contains AddModels? {}\n", m_original_config.contains("AddModels"));
        fmt::print("🔍 DEBUG ProcessMLPipeline: Contains MLModels? {}\n", m_original_config.contains("MLModels"));

        // Support new AddModels structure (v2.0) or legacy MLModels (v1.0)
        if (m_original_config.contains("AddModels")) {
            modelsConfig = m_original_config["AddModels"].toObject();
            globalAnalysisConfig = m_original_config.contains("PostFitAnalysis") ? m_original_config.value("PostFitAnalysis").toObject() : QJsonObject();
            fmt::print("🔍 Using AddModels structure (v2.0): {} models with post-fit analysis\n", modelsConfig.size());
        } else if (m_original_config.contains("MLModels")) {
            // Convert legacy MLModels to AddModels format for compatibility
            QJsonObject legacyModels = m_original_config["MLModels"].toObject();
            for (auto it = legacyModels.begin(); it != legacyModels.end(); ++it) {
                QJsonObject modelEntry;
                if (it.value().isDouble()) {
                    modelEntry["ID"] = it.value().toInt();
                } else {
                    modelEntry = it.value().toObject();
                }
                modelsConfig[it.key()] = modelEntry;
            }
            fmt::print("🔍 Using configured MLModels (v1.0 compatibility): {} models\n", modelsConfig.size());
        } else {
            // Default model set for testing
            modelsConfig = {
                { "nmr_1_1", QJsonObject{ { "ID", 1 } } }, // NMR 1:1
                { "nmr_1_2", QJsonObject{ { "ID", 2 } } }, // NMR 1:2
                { "nmr_2_1", QJsonObject{ { "ID", 3 } } } // NMR 2:1
            };
            fmt::print("🔍 Using default models: {} models\n", modelsConfig.size());
        }

        // Step 3: Fit models and evaluate with post-fit analysis (using AnalysisManager)
        fmt::print("🚨 CRITICAL DEBUG: About to call AnalysisManager->fitModelsToData()\n");
        fmt::print("🔍 DEBUG: data pointer = {}\n", data.data() != nullptr ? "VALID" : "NULL");
        if (data) {
            fmt::print("🔍 DEBUG: data has {} independent rows, {} dependent rows\n", 
                       data->IndependentModel()->rowCount(), data->DependentModel()->rowCount());
        }
        fmt::print("🔍 DEBUG: modelsConfig size = {}\n", modelsConfig.size());
        fmt::print("🔍 DEBUG: globalAnalysisConfig size = {}\n", globalAnalysisConfig.size());
        fmt::print("🔍 DEBUG: globalAnalysisConfig keys: {}\n", globalAnalysisConfig.keys().join(", ").toStdString());
        
        fmt::print("🚨 CALLING AnalysisManager->fitModelsToData() NOW...\n");
        QVector<QJsonObject> fittedModels = m_analysisManager->fitModelsToData(data, modelsConfig, globalAnalysisConfig);
        fmt::print("🚨 RETURNED from AnalysisManager->fitModelsToData() with {} results\n", fittedModels.size());

        // Step 3.5: Create Multi-Project architecture according to JSON_DOKUMENTATION.md - Claude Generated
        // Convert fitted models to individual projects using official project_X format
        QJsonObject multiProjectData;
        multiProjectData["format_version"] = "2.0";
        multiProjectData["generation_timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        // Remove base_data - not part of official JSON_DOKUMENTATION.md structure
        
        // Extract ground truth model information from content for ML labeling - Claude Generated
        QString content = data->getContent();
        if (content.contains("Model: ¹H 1:1-Model")) {
            QJsonObject groundTruth;
            groundTruth["model_id"] = 1;
            groundTruth["model_name"] = "¹H 1:1-Model";
            groundTruth["source"] = "data_generation_content";
            multiProjectData["ground_truth"] = groundTruth;
        } else if (content.contains("Model: ¹H 1:2-Model")) {
            QJsonObject groundTruth;
            groundTruth["model_id"] = 2; 
            groundTruth["model_name"] = "¹H 1:2-Model";
            groundTruth["source"] = "data_generation_content";
            multiProjectData["ground_truth"] = groundTruth;
        }
        
        // Create individual project entries following JSON_DOKUMENTATION.md structure - Claude Generated
        for (int modelIndex = 0; modelIndex < fittedModels.size(); ++modelIndex) {
            const QJsonObject& modelResult = fittedModels[modelIndex];
            QString projectKey = QString("project_%1").arg(modelIndex);
            
            // Create the proper project structure with "data" wrapper (per JSON_DOKUMENTATION.md)
            QJsonObject projectData;
            QJsonObject dataWrapper;
            
            // Basic project metadata (required by JSON_DOKUMENTATION.md)
            dataWrapper["DataType"] = 1;
            dataWrapper["SupraFit"] = 2004;
            dataWrapper["title"] = QString("Model Analysis %1").arg(modelIndex + 1);
            dataWrapper["uuid"] = QUuid::createUuid().toString();
            
            // Include original data structure
            dataWrapper["independent"] = data->IndependentModel()->ExportTable(true);
            dataWrapper["dependent"] = data->DependentModel()->ExportTable(true);
            
            // Export the fitted model with complete statistics (including Monte Carlo)
            if (modelResult.contains("model_export") && modelResult["model_export"].isObject()) {
                QJsonObject fullModelExport = modelResult["model_export"].toObject();
                
                // Extract and include the model in the standard format
                if (fullModelExport.contains("data")) {
                    QJsonObject modelData = fullModelExport["data"].toObject();
                    QString modelKey = QString("model_0");
                    
                    QJsonObject modelEntry;
                    modelEntry["model"] = modelResult.contains("model_id") ? modelResult["model_id"].toInt() : 1;
                    modelEntry["name"] = fullModelExport.contains("name") ? fullModelExport["name"].toString() : "Unknown Model";
                    modelEntry["data"] = modelData;  // This contains the "methods" with statistical results
                    
                    dataWrapper[modelKey] = modelEntry;
                }
            }
            
            projectData["data"] = dataWrapper;
            multiProjectData[projectKey] = projectData;
        }
        
        fmt::print("🔍 DEBUG: Created Multi-Project structure with {} projects\n", fittedModels.size());
        
        // Save the Multi-Project dataset - Claude Generated
        QString datasetFilename = m_outfile + "-" + QString::number(i);
        if (m_outfile.endsWith(".json")) {
            datasetFilename += ".json";
        } else {
            datasetFilename += ".json";  // Always save dataset as JSON
        }
        
        SaveFile(datasetFilename, multiProjectData);
        fmt::print("🔍 DEBUG: Saved Multi-Project dataset '{}' with {} projects\n", 
                  datasetFilename.toStdString(), fittedModels.size());

        // Step 4: Create SupraFit project with fitted models (like GUI SaveWorkspace)
        QJsonObject projectFile;
        projectFile["data"] = data->ExportData();  // Use updated data with fitted models in ML RawData
        
        // Add fitted models in SupraFit format (model_0, model_1, model_2...)
        int modelIndex = 0;
        for (const QJsonObject& modelResult : fittedModels) {
            if (modelResult.contains("model_export")) {
                projectFile["model_" + QString::number(modelIndex)] = modelResult["model_export"];
                modelIndex++;
            }
        }

        // Add post-fit analysis results to project file if available
        QJsonObject analysisResults;
        for (const QJsonObject& modelResult : fittedModels) {
            if (modelResult.contains("post_fit_analysis")) {
                QString modelName = modelResult["model_name"].toString();
                analysisResults[modelName + "_analysis"] = modelResult["post_fit_analysis"];
            }
        }
        if (!analysisResults.isEmpty()) {
            // projectFile["post_fit_analysis"] = analysisResults;
        }

        // Also create ML training entry 
        for (const QJsonObject& modelResult : fittedModels) {
            QJsonObject mlEntry;
            mlEntry["dataset_id"] = i;
            mlEntry["original_data"] = dataset;
            mlEntry["fitted_model"] = modelResult;
            mlEntry["ml_features"] = modelResult["ml_features"];
            mlEntry["pipeline_timestamp"] = QDateTime::currentMSecsSinceEpoch();
            
            results.append(mlEntry);
        }
        
        // Save the project file with fitted models
        QString modelsFilename = m_outfile + "-models-" + QString::number(i);
        if (m_outfile.endsWith(".json")) {
            modelsFilename += ".json";
        } else {
            modelsFilename += ".suprafit";
        }
        SaveFile(modelsFilename, projectFile);

        delete data;
    }
    
    fmt::print("🎉 ML Pipeline completed: {} total model evaluations\n", results.size());
    return results;
}

QVector<QJsonObject> SupraFitCli::FitModelsToData(QPointer<DataClass> data, const QJsonObject& modelsConfig, const QJsonObject& globalAnalysisConfig)
{
    /**
     * @brief Fit multiple models to data for comparison and ML feature extraction
     * Tests various analytical models against simulated or experimental data,
     * extracting statistical metrics for model selection and ML training.
     */
    QVector<QJsonObject> results;
    
    if (!data) {
        fmt::print("❌ Invalid data provided to FitModelsToData\n");
        return results;
    }
    
    fmt::print("🔍 Fitting {} models to dataset with {} data points\n", 
        modelsConfig.size(), data->IndependentModel()->rowCount());
    
    // Extract true model ID if available from content
    int trueModelId = -1;
    QString content = data->getContent();
    if (content.contains("Model: ")) {
        // TODO: Extract model ID from content string (if DataGenerator created)
        // This helps evaluate how well each model performs vs. true model
    }

    for (auto it = modelsConfig.begin(); it != modelsConfig.end(); ++it) {
        const QString& modelName = it.key();
        QJsonValue modelValue = it.value();
        
        fmt::print("  📊 Testing model: {}\n", modelName.toStdString());
        
        try {
            // Create model based on AddModels structure (v2.0) or legacy format
            QSharedPointer<AbstractModel> model;
            QJsonObject modelSpecificConfig;

            if (modelValue.isDouble()) {
                // Legacy MLModels format: "model_name": model_id
                int modelId = modelValue.toInt();
                model = AddModel(modelId, data);
            } else if (modelValue.isObject()) {
                QJsonObject modelObj = modelValue.toObject();

                // New AddModels format: "model_name": {"ID": model_id, "Options": {...}, "PostFitAnalysis": {...}}
                if (modelObj.contains("ID")) {
                    int modelId = modelObj["ID"].toInt();
                    model = AddModel(modelId, data);

                    // Apply model-specific options
                    if (modelObj.contains("Options")) {
                        QJsonObject options = modelObj["Options"].toObject();
                        // Apply FastMode, Convergency, MaxIterations, etc.
                        if (options.contains("FastMode") && options["FastMode"].toBool()) {
                            model->setFast(true);
                        }
                        if (options.contains("Convergency")) {
                            // model->setConvergence(options["Convergency"].toDouble()); // Method not available in AbstractModel
                        }
                        if (options.contains("MaxIterations")) {
                            // model->setMaxIterations(options["MaxIterations"].toInt()); // Method not available in AbstractModel
                        }
                    }

                    // Store model-specific PostFitAnalysis configuration
                    if (modelObj.contains("PostFitAnalysis")) {
                        modelSpecificConfig = modelObj["PostFitAnalysis"].toObject();
                    }
                } else if (modelObj.contains("model")) {
                    // Legacy format: {"model": model_id, "options": {...}}
                    int modelId = modelObj["model"].toInt();
                    model = AddModel(modelId, data);
                    if (modelObj.contains("options")) {
                        model->setOptions(modelObj["options"].toObject());
                    }
                }
            }
            
            if (!model) {
                fmt::print("    ❌ Failed to create model {}\n", modelName.toStdString());
                continue;
            }
            
            // Perform initial guess and fitting
            model->InitialGuess();
            
            // Proper model fitting using NonLinearFitThread (like MonteCarloStatistics) - Claude Generated
            if (m_main.contains("FitModels") && m_main["FitModels"].toBool()) {
                fmt::print("      Fitting model {}...\n", modelName.toStdString());
                
                // Create and configure NonLinearFitThread for optimization
                NonLinearFitThread* fit_thread = new NonLinearFitThread(false);
                fit_thread->setModel(model, false);
                fit_thread->run();
                
                // Import fitted parameters and calculate statistics
                bool converged = fit_thread->Converged();
                model->ImportModel(fit_thread->ConvergedParameter());
                model->setFast(true);
                model->CalculateStatistics(true);
                model->Calculate();
                model->setConverged(converged);
                
                fmt::print("      Model {} fitting {} (converged: {})\n", 
                    modelName.toStdString(), 
                    converged ? "succeeded" : "failed", 
                    converged);
                
                delete fit_thread;
            } else {
                // Just calculate with default parameters if fitting disabled
                model->setFast(false);
                model->Calculate();
            }

            // Step 4: Execute post-fit analysis if enabled (v2.0 feature)
            QJsonObject postFitResults;
            if ((m_main.contains("PostFitAnalysis") && m_main["PostFitAnalysis"].toBool()) || (!modelSpecificConfig.isEmpty() && modelSpecificConfig["enabled"].toBool())) {

                fmt::print("      Running post-fit analysis for model {}...\n", modelName.toStdString());

                // Use JobManager for post-fit analysis (corrected architecture)
                // Determine which analysis configuration to use (model-specific overrides global)
                QJsonObject analysisConfig = globalAnalysisConfig;
                if (!modelSpecificConfig.isEmpty() && modelSpecificConfig["enabled"].toBool()) {
                    analysisConfig = modelSpecificConfig;
                    fmt::print("        Using model-specific analysis configuration\n");
                } else if (!globalAnalysisConfig.isEmpty() && globalAnalysisConfig["enabled"].toBool()) {
                    fmt::print("        Using global analysis configuration\n");
                }

                if (!analysisConfig.isEmpty() && analysisConfig["enabled"].toBool()) {
                    postFitResults = runPostFitAnalysis(model, analysisConfig);
                    fmt::print("        Post-fit analysis completed with {} methods\n",
                        postFitResults["methods"].toObject().size());
                } else {
                    fmt::print("        Post-fit analysis disabled\n");
                }
            }

            // Evaluate model performance
            QJsonObject evaluation = EvaluateModelFit(model, trueModelId);
            evaluation["model_name"] = modelName; // TODO marked as potentially obsolete as we have the name in model
            evaluation["model_id"] = model->Type();// TODO marked as potentially obsolete as we have the id in model
            // TODO for Claude -> Analyse json structure of the model and write it to Claude.md in core
            
            // Export complete model for saving (like GUI SaveWorkspace)
            evaluation["model_export"] = model->ExportModel(true, true);

            // Include post-fit analysis results if available
            if (!postFitResults.isEmpty()) {
                evaluation["post_fit_analysis"] = postFitResults;
            }

            results.append(evaluation);
            
            fmt::print("    ✅ Model {} completed (SSE: {:.2e})\n", 
                modelName.toStdString(), evaluation["SSE"].toDouble());
                
        } catch (const std::exception& e) {
            fmt::print("    ❌ Model {} failed: {}\n", modelName.toStdString(), e.what());
        }
    }
    return results;
}

QJsonObject SupraFitCli::EvaluateModelFit(QSharedPointer<AbstractModel> model, int trueModelId)
{
    /**
     * @brief Extract comprehensive statistical evaluation of model fit
     * Calculates fit quality metrics, parameter statistics, and ML features
     * for model comparison and machine learning training data generation.
     */
    QJsonObject evaluation;
    
    if (!model) {
        evaluation["error"] = "Invalid model provided";
        return evaluation;
    }
    
    // Basic fit statistics
    evaluation["SSE"] = model->SSE();
    evaluation["AIC"] = model->GetAIC();
    evaluation["AICc"] = model->GetAICc();
    evaluation["SEy"] = model->SEy();
    evaluation["RSquared"] = model->GetRSquared();
    evaluation["ChiSquared"] = model->GetChiSquare();
    
    // Parameter information
    evaluation["parameter_count"] = model->GlobalParameterSize() + model->LocalParameterSize();
    evaluation["global_parameters"] = model->GlobalParameterSize();
    evaluation["local_parameters"] = model->LocalParameterSize();
    
    // Data information
    evaluation["data_points"] = model->DataPoints();
    evaluation["series_count"] = model->SeriesCount();
    
    // Model identification
    evaluation["model_type"] = static_cast<int>(model->Type());
    evaluation["model_name"] = model->Name();
    
    // Calculate relative performance if true model is known
    if (trueModelId >= 0) {
        evaluation["true_model_id"] = trueModelId;
        evaluation["is_correct_model"] = (static_cast<int>(model->Type()) == trueModelId);
    }
    
    // Extract ML features for training
    evaluation["ml_features"] = ExtractMLFeatures(model, false);
    
    return evaluation;
}

QJsonObject SupraFitCli::ExtractMLFeatures(QSharedPointer<AbstractModel> model, bool includeStatistics)
{
    /**
     * @brief Extract machine learning features from fitted model
     * Creates feature vector suitable for ML model selection training,
     * including statistical metrics, parameter characteristics, and fit quality indicators.
     * 
     * TODO: Move statistical analysis to src/core/analyse.cpp - separate calculation from formatting
     * TODO: Use core statistical functions instead of local calculations
     */
    QJsonObject features;
    
    if (!model) {
        return features;
    }
    
    // Primary fit quality features
    double sse = model->SSE();
    double aic = model->GetAIC();
    double aic_c = model->GetAICc();
    double r_squared = model->GetRSquared();
    double chi_squared = model->GetChiSquare();
    
    features["sse"] = sse;
    features["aic"] = aic;
    features["aicc"] = aic_c;
    features["r_squared"] = r_squared;
    features["chi_squared"] = chi_squared;
    features["log_sse"] = std::log10(std::max(sse, 1e-15));
    
    // Normalized features (relative to data scale)
    int dataPoints = model->DataPoints();
    features["sse_per_point"] = sse / std::max(dataPoints, 1);
    features["rmse"] = std::sqrt(sse / std::max(dataPoints - model->GlobalParameterSize(), 1));
    
    // Parameter complexity features
    features["parameter_count"] = model->GlobalParameterSize() + model->LocalParameterSize();
    features["global_params"] = model->GlobalParameterSize();
    features["local_params"] = model->LocalParameterSize();
    features["data_to_param_ratio"] = double(dataPoints) / std::max(features["parameter_count"].toInt(), 1);
    
    // Model characteristics
    features["model_type"] = static_cast<int>(model->Type());
    features["series_count"] = model->SeriesCount();
    features["data_points"] = dataPoints;
    
    // Information criteria ratios (for relative comparison)
    features["aic_aicc_ratio"] = (aic_c > 0) ? aic / aic_c : 1.0;
    
    // Advanced statistical features (optional for performance)
    if (includeStatistics) {
        // TODO: Move to core statistical analysis functions
        // TODO: Implement parameter uncertainty calculation in core
        // TODO: Implement prediction variance calculation in core
        features["parameter_uncertainty"] = 0.0;  // Placeholder - needs core implementation
        features["prediction_variance"] = 0.0;    // Placeholder - needs core implementation
    }
    
    return features;
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

// Execute post-fit statistical analysis using JobManager - Claude Generated
QJsonObject SupraFitCli::runPostFitAnalysis(QSharedPointer<AbstractModel> model, const QJsonObject& analysisConfig)
{
    QJsonObject results;

    if (!analysisConfig.contains("enabled") || !analysisConfig["enabled"].toBool()) {
        return results;
    }

    if (!analysisConfig.contains("methods") || !analysisConfig["methods"].isArray()) {
        fmt::print("❌ ERROR: PostFitAnalysis configuration missing 'methods' array\n");
        return results;
    }

    // Set the model for JobManager (like in modelwidget.cpp)
    m_jobmanager->setModel(model);

    QJsonArray methodsArray = analysisConfig["methods"].toArray();
    QJsonObject methodResults;

    for (const QJsonValue& methodValue : methodsArray) {
        QJsonObject methodConfig = methodValue.toObject();
        if (!methodConfig.contains("Method")) {
            continue;
        }

        int methodType = methodConfig["Method"].toInt();
        QString methodName = SupraFit::Method2Name(static_cast<SupraFit::Method>(methodType));

        fmt::print("        Running {} analysis (Method: {})\n", methodName.toStdString(), methodType);

        // Create job configuration (like in modelwidget.cpp)
        QJsonObject job = methodConfig;
        job["Method"] = methodType;

        // Add default parameters if not specified
        if (!job.contains("MaxSteps")) {
            job["MaxSteps"] = 1000;
        }

        // For Monte Carlo (Method 1), ensure VarianceSource is set
        if (methodType == static_cast<int>(SupraFit::Method::MonteCarlo)) {
            if (!job.contains("VarianceSource")) {
                job["VarianceSource"] = 2; // SEy (model error) - recommended default
            }
            if (!job.contains("EntropyBins")) {
                job["EntropyBins"] = 50;
            }
        }

        try {
            // Execute job using JobManager (like in modelwidget.cpp)
            m_jobmanager->AddSingleJob(job);
            m_jobmanager->RunJobs();

            // Get results from model after job completion
            // The JobManager stores results in the model itself - Claude Generated
            QJsonObject methodResult;
            methodResult["method"] = methodType;
            methodResult["method_name"] = methodName;
            methodResult["configuration"] = job;
            methodResult["completed"] = true;
            
            // Extract actual analysis results from model after JobManager execution - Claude Generated
            // JobManager stores the real statistical results in the model - use JsonUtils to extract them
            // CRITICAL: Must use ExportModel(true, false) to include statistics in export!
            QJsonObject modelExport = model->ExportModel(true, false);
            QJsonObject actualResults = SupraFit::JsonUtils::getStatisticalMethod(modelExport, static_cast<SupraFit::Method>(methodType));
            
            if (!actualResults.isEmpty()) {
                methodResult["results"] = actualResults;
                fmt::print("✅ Extracted {} results from model: {} entries\n", 
                          methodName.toStdString(), actualResults.keys().size());
            } else {
                fmt::print("⚠️  Warning: No results found for {} in model export\n", methodName.toStdString());
                
                // Fallback: Create empty structure with debug info
                QJsonObject fallbackResults;
                fallbackResults["max_steps"] = -1;
                fallbackResults["models"] = QJsonArray();
                fallbackResults["parameter_averages"] = QJsonArray(); 
                fallbackResults["variance"] = 0;
                fallbackResults["debug_info"] = "JobManager execution completed but no results found in model";
                methodResult["results"] = fallbackResults;
            }

            methodResults[QString::number(methodType)] = methodResult;

        } catch (const std::exception& e) {
            fmt::print("❌ ERROR: Failed to execute {} analysis: {}\n",
                methodName.toStdString(), e.what());

            QJsonObject errorResult;
            errorResult["method"] = methodType;
            errorResult["method_name"] = methodName;
            errorResult["error"] = QString::fromStdString(e.what());
            errorResult["completed"] = false;

            methodResults[QString::number(methodType)] = errorResult;
        }
    }

    results["methods"] = methodResults;
    results["analysis_completed"] = true;

    return results;
}

// ML Training Data Export Implementation - Claude Generated
bool SupraFitCli::exportMLTrainingData(const QVector<QString>& inputFiles, const QString& outputFile)
{
    if (inputFiles.isEmpty()) {
        fmt::print("❌ ERROR: No input files provided for ML training data export\n");
        return false;
    }

    MLFeatureExtractor* extractor = new MLFeatureExtractor(this);
    
    // Configure extraction options
    extractor->setExtractionOptions(
        true,   // includeAdvancedStats
        false,  // includeFitParameters (keep compact for NN)
        true    // includeInputNoise
    );
    
    fmt::print("🔧 Extracting ML training data from {} files...\n", inputFiles.size());
    
    // Extract training samples from all input files
    QVector<QJsonObject> trainingSamples = extractor->extractBatchTrainingData(inputFiles);
    
    if (trainingSamples.isEmpty()) {
        fmt::print("❌ ERROR: No training samples could be extracted from input files\n");
        delete extractor;
        return false;
    }
    
    // Export in neural network format
    QJsonObject neuralNetData = extractor->exportNeuralNetFormat(trainingSamples);
    
    // Save to file
    bool success = SaveFile(outputFile, neuralNetData);
    
    if (success) {
        fmt::print("✅ ML training data exported: {} samples → {}\n", trainingSamples.size(), outputFile.toStdString());
    } else {
        fmt::print("❌ ERROR: Failed to save ML training data to {}\n", outputFile.toStdString());
    }
    
    delete extractor;
    return success;
}

bool SupraFitCli::exportMLTrainingDataSingle(const QString& inputFile, const QString& outputFile)
{
    return exportMLTrainingData(QVector<QString>() << inputFile, outputFile);
}

bool SupraFitCli::exportMLTrainingDataBatch(const QString& inputDirectory, const QString& outputFile)
{
    QDir dir(inputDirectory);
    if (!dir.exists()) {
        fmt::print("❌ ERROR: Input directory does not exist: {}\n", inputDirectory.toStdString());
        return false;
    }
    
    // Find all JSON files in directory
    QStringList nameFilters;
    nameFilters << "*.json";
    QStringList jsonFiles = dir.entryList(nameFilters, QDir::Files);
    
    if (jsonFiles.isEmpty()) {
        fmt::print("❌ ERROR: No JSON files found in directory: {}\n", inputDirectory.toStdString());
        return false;
    }
    
    // Convert to full paths
    QVector<QString> inputFiles;
    for (const QString& fileName : jsonFiles) {
        inputFiles.append(dir.absoluteFilePath(fileName));
    }
    
    fmt::print("🔍 Found {} JSON files in directory {}\n", inputFiles.size(), inputDirectory.toStdString());
    
    return exportMLTrainingData(inputFiles, outputFile);
}

// Claude Generated: Extract and display fitted model parameters
bool SupraFitCli::ExtractModelParameters(const QString& modelIndexStr)
{
    if (m_toplevel.isEmpty()) {
        fmt::print("❌ ERROR: No data loaded. File structure is empty.\n");
        return false;
    }

    // Check if this is a models file
    QStringList keys = m_toplevel.keys();
    QStringList modelKeys;
    for (const QString& key : keys) {
        if (key.startsWith("model_")) {
            modelKeys << key;
        }
    }

    if (modelKeys.isEmpty()) {
        fmt::print("❌ ERROR: No models found in file. Expected model_0, model_1, etc.\n");
        fmt::print("Available keys: {}\n", keys.join(", ").toStdString());
        return false;
    }

    fmt::print("📊 Found {} models in file\n", modelKeys.size());

    // Filter models if specific index requested
    QStringList targetsToProcess;
    if (!modelIndexStr.isEmpty()) {
        QString targetKey = QString("model_%1").arg(modelIndexStr);
        if (modelKeys.contains(targetKey)) {
            targetsToProcess << targetKey;
        } else {
            fmt::print("❌ ERROR: Model {} not found. Available models: {}\n", 
                      targetKey.toStdString(), modelKeys.join(", ").toStdString());
            return false;
        }
    } else {
        targetsToProcess = modelKeys;
    }

    // Extract and display parameters for each model
    fmt::print("\n=== Model Parameter Extraction ===\n\n");
    
    for (const QString& modelKey : targetsToProcess) {
        QJsonObject model = m_toplevel[modelKey].toObject();
        
        fmt::print("🔬 Model: {}\n", modelKey.toStdString());
        
        // Basic model information
        if (model.contains("name")) {
            fmt::print("   Name: {}\n", model["name"].toString().toStdString());
        }
        
        if (model.contains("converged")) {
            bool converged = model["converged"].toBool();
            fmt::print("   Status: {}\n", converged ? "✅ Converged" : "❌ Not Converged");
        }
        
        // Fit quality metrics
        if (model.contains("SSE")) {
            double sse = model["SSE"].toDouble();
            fmt::print("   SSE: {:.6e}\n", sse);
        }
        
        if (model.contains("AIC")) {
            double aic = model["AIC"].toDouble();
            if (aic != std::numeric_limits<double>::infinity() && aic > -999) {
                fmt::print("   AIC: {:.6e}\n", aic);
            }
        }
        
        if (model.contains("AICc")) {
            double aicc = model["AICc"].toDouble();
            if (aicc != std::numeric_limits<double>::infinity() && aicc > -999) {
                fmt::print("   AICc: {:.6e}\n", aicc);
            }
        }
        
        // Extract fitted parameters
        if (model.contains("parameter")) {
            QJsonObject params = model["parameter"].toObject();
            fmt::print("   📈 Fitted Parameters:\n");
            
            for (auto it = params.begin(); it != params.end(); ++it) {
                QString paramName = it.key();
                QJsonValue paramValue = it.value();
                
                if (paramValue.isDouble()) {
                    double value = paramValue.toDouble();
                    if (value != std::numeric_limits<double>::infinity() && 
                        value != -std::numeric_limits<double>::infinity() &&
                        !std::isnan(value)) {
                        fmt::print("      {}: {:.6e}\n", paramName.toStdString(), value);
                    } else {
                        fmt::print("      {}: [Invalid/Infinite]\n", paramName.toStdString());
                    }
                } else if (paramValue.isArray()) {
                    QJsonArray paramArray = paramValue.toArray();
                    fmt::print("      {}:", paramName.toStdString());
                    for (int i = 0; i < paramArray.size(); ++i) {
                        double value = paramArray[i].toDouble();
                        if (value != std::numeric_limits<double>::infinity() && 
                            value != -std::numeric_limits<double>::infinity() &&
                            !std::isnan(value)) {
                            fmt::print(" {:.6e}", value);
                        } else {
                            fmt::print(" [Invalid]");
                        }
                    }
                    fmt::print("\n");
                } else {
                    fmt::print("      {}: {}\n", paramName.toStdString(), 
                              paramValue.toString().toStdString());
                }
            }
        }
        
        // Extract confidence intervals if available
        if (model.contains("confidence_intervals")) {
            QJsonObject ci = model["confidence_intervals"].toObject();
            fmt::print("   📊 Confidence Intervals:\n");
            
            for (auto it = ci.begin(); it != ci.end(); ++it) {
                QString paramName = it.key();
                QJsonObject interval = it.value().toObject();
                
                if (interval.contains("lower") && interval.contains("upper")) {
                    double lower = interval["lower"].toDouble();
                    double upper = interval["upper"].toDouble();
                    fmt::print("      {}: [{:.6e}, {:.6e}]\n", 
                              paramName.toStdString(), lower, upper);
                }
            }
        }
        
        // Extract standard errors if available
        if (model.contains("standard_errors")) {
            QJsonObject se = model["standard_errors"].toObject();
            fmt::print("   📏 Standard Errors:\n");
            
            for (auto it = se.begin(); it != se.end(); ++it) {
                QString paramName = it.key();
                double error = it.value().toDouble();
                if (!std::isnan(error) && error != std::numeric_limits<double>::infinity()) {
                    fmt::print("      {}: {:.6e}\n", paramName.toStdString(), error);
                }
            }
        }
        
        fmt::print("\n");
    }
    
    return true;
}


/*
SupraFitCli * SupraFitCli::Generate() const
{
    SupraFitCli *generated = new SupraFitCli;

    return generated;
}*/
