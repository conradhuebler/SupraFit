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
 *
 */

#include "src/capabilities/jobmanager.h"

#include "src/core/models/models.h"

#include "src/client/analysis_reporter.h"
#include "src/client/ml_export.h"
#include "src/client/ml_pipeline_manager.h"

#include "src/core/equil.h"
#include "src/core/jsonhandler.h"

#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QDir>

#include "src/global.h"
#include "src/global_config.h"
#include "src/global_infos.h"
#include "src/version.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>

#include <QtCore/QRandomGenerator>
#include <QCoreApplication>

#include <fmt/color.h>
#include <fmt/core.h>

#include <iostream>

#ifndef _WIN32
#if __GNUC__
// Thanks to
// https://stackoverflow.com/questions/77005/how-to-automatically-generate-a-stacktrace-when-my-program-crashes
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void bt_handler(int sig)
{
    void* array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "SupraFit Client crashed. Although this is probably unintended, it happened anyway.\n Some kind of backtrace will be printed out!\n\n");
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    fprintf(stderr, "Good-By\n");
    exit(1);
}
#endif
#endif

// File configuration type detection - Claude Generated
enum FileConfigType {
    SimpleProject, // Simple SupraFit project (data + optional models)
    TaskConfig, // Task configuration (Main, GenerateData, Jobs, etc.)
    MLPipelineConfig, // ML Pipeline configuration
    InvalidConfig // Invalid or unreadable configuration
};

// Detect configuration file type based on JSON structure - Claude Generated
FileConfigType detectConfigType(const QJsonObject& config)
{
    if (config.isEmpty()) {
        return InvalidConfig;
    }

    // Multi-project file detection (project_0, project_1, etc.)
    QStringList keys = config.keys();
    bool hasProjectKeys = false;
    for (const QString& key : keys) {
        if (key.startsWith("project_")) {
            hasProjectKeys = true;
            break;
        }
    }
    if (hasProjectKeys) {
        return SimpleProject; // Multi-project files are treated as simple projects
    }

    // ML Pipeline configuration detection
    if (config.contains("Pipeline") || config.contains("BatchConfig") || config.contains("MLDataStructure") || 
        (config.contains("ProcessMLPipeline") && config["ProcessMLPipeline"].toBool())) {
        return MLPipelineConfig;
    }

    // Task configuration detection (has Main section with tasks)
    if (config.contains("Main") || config.contains("main")) {
        QJsonObject mainSection = config.contains("Main") ? config["Main"].toObject() : config["main"].toObject();
        if (mainSection.contains("Tasks") || mainSection.contains("GenerateData") || mainSection.contains("OutFile") || mainSection.contains("Repeat")) {
            return TaskConfig;
        }
    }

    // Check for modular structure (Independent/Dependent)
    if (config.contains("Independent") && config.contains("Dependent")) {
        return TaskConfig;
    }

    // Simple project detection (has data section)
    if (config.contains("data") || config.contains("SupraFit") || config.contains("DataType") || config.contains("independent")) {
        return SimpleProject;
    }

    // Default to task config if it has recognizable sections
    if (config.contains("Models") || config.contains("Jobs") || config.contains("Analyse")) {
        return TaskConfig;
    }

    return InvalidConfig;
}

// Analyze multi-project file - Claude Generated
void analyzeMultiProjectFile(const QJsonObject& config)
{
    std::cout << "=== Multi-Project File Analysis ===" << std::endl;

    QStringList projectKeys;
    for (auto it = config.begin(); it != config.end(); ++it) {
        if (it.key().startsWith("project_")) {
            projectKeys << it.key();
        }
    }

    std::cout << "Total projects found: " << projectKeys.size() << std::endl
              << std::endl;

    for (const QString& projectKey : projectKeys) {
        QJsonObject project = config[projectKey].toObject();
        std::cout << "📁 " << projectKey.toStdString() << ":" << std::endl;

        // Analyze data section
        if (project.contains("data")) {
            QJsonObject data = project["data"].toObject();
            std::cout << "   📊 Data:" << std::endl;

            if (data.contains("title")) {
                std::cout << "      Title: " << data["title"].toString().toStdString() << std::endl;
            }

            if (data.contains("independent")) {
                QJsonObject indep = data["independent"].toObject();
                std::cout << "      Independent: " << indep["rows"].toInt() << " rows × "
                          << indep["cols"].toInt() << " cols" << std::endl;
                if (indep.contains("header")) {
                    std::cout << "         Headers: " << indep["header"].toString().toStdString() << std::endl;
                }
            }

            if (data.contains("dependent")) {
                QJsonObject dep = data["dependent"].toObject();
                std::cout << "      Dependent: " << dep["rows"].toInt() << " rows × "
                          << dep["cols"].toInt() << " cols" << std::endl;
                if (dep.contains("header")) {
                    std::cout << "         Headers: " << dep["header"].toString().toStdString() << std::endl;
                }
            }

            if (data.contains("DataType")) {
                std::cout << "      Data Type: " << data["DataType"].toInt() << std::endl;
            }
        }

        // Analyze models
        QStringList modelKeys;
        for (auto it = project.begin(); it != project.end(); ++it) {
            if (it.key().startsWith("model_")) {
                modelKeys << it.key();
            }
        }

        if (!modelKeys.isEmpty()) {
            std::cout << "   🔬 Models (" << modelKeys.size() << "):" << std::endl;
            for (const QString& modelKey : modelKeys) {
                QJsonObject model = project[modelKey].toObject();
                if (model.contains("name")) {
                    std::cout << "      " << modelKey.toStdString() << ": "
                              << model["name"].toString().toStdString();
                    if (model.contains("converged")) {
                        std::cout << (model["converged"].toBool() ? " [Converged]" : " [Not Converged]");
                    }
                    if (model.contains("SSE")) {
                        std::cout << " (SSE: " << model["SSE"].toDouble() << ")";
                    }
                    std::cout << std::endl;
                }
            }
        }

        std::cout << std::endl;
    }
}

// Output path validation function - Claude Generated
bool validateOutputPath(const QString& outputPath)
{
    if (outputPath.isEmpty()) {
        return true; // No output path specified is valid
    }
    
    QFileInfo fileInfo(outputPath);
    QDir parentDir = fileInfo.dir();
    
    // Check if parent directory exists
    if (!parentDir.exists()) {
        std::cerr << "ERROR: Output directory does not exist: " << parentDir.absolutePath().toStdString() << std::endl;
        return false;
    }
    
    // Check if parent directory is writable
    QFileInfo dirInfo(parentDir.absolutePath());
    if (!dirInfo.isWritable()) {
        std::cerr << "ERROR: Cannot write to output directory (insufficient permissions): " 
                  << parentDir.absolutePath().toStdString() << std::endl;
        return false;
    }
    
    // If file already exists, check if it's writable
    if (fileInfo.exists() && !fileInfo.isWritable()) {
        std::cerr << "ERROR: Cannot overwrite existing file (insufficient permissions): " 
                  << outputPath.toStdString() << std::endl;
        return false;
    }
    
    return true;
}

// Join multiple files into a single multi-project file - Claude Generated
bool performFileJoin(const QStringList& inputFiles, const QString& outputFile)
{
    std::cout << "Joining " << inputFiles.size() << " files into: " << outputFile.toStdString() << std::endl;

    QJsonObject joinedData;
    int projectCounter = 0;

    for (const QString& inputFile : inputFiles) {
        std::cout << "Processing: " << inputFile.toStdString() << std::endl;

        // Load input file
        QJsonObject fileData = JsonHandler::LoadFile(inputFile);
        if (fileData.isEmpty()) {
            std::cout << "   ERROR: Could not load file: " << inputFile.toStdString() << std::endl;
            continue;
        }

        // Check if this is already a multi-project file
        QStringList projectKeys;
        for (auto it = fileData.begin(); it != fileData.end(); ++it) {
            if (it.key().startsWith("project_")) {
                projectKeys << it.key();
            }
        }

        if (!projectKeys.isEmpty()) {
            // This is a multi-project file - copy all projects
            std::cout << "   Multi-project file with " << projectKeys.size() << " projects" << std::endl;
            for (const QString& projectKey : projectKeys) {
                QString newProjectKey = QString("project_%1").arg(projectCounter++);
                joinedData[newProjectKey] = fileData[projectKey];
                std::cout << "     " << projectKey.toStdString() << " -> " << newProjectKey.toStdString() << std::endl;
            }
        } else {
            // This is a single project file - add it as project_N
            QString newProjectKey = QString("project_%1").arg(projectCounter++);

            // Check if it has a 'data' section (SupraFit project format)
            if (fileData.contains("data")) {
                joinedData[newProjectKey] = fileData;
                std::cout << "   Single project (SupraFit format) -> " << newProjectKey.toStdString() << std::endl;
            } else {
                // Assume the entire file is the data section
                QJsonObject projectWrapper;
                projectWrapper["data"] = fileData;
                joinedData[newProjectKey] = projectWrapper;
                std::cout << "   Raw data file -> " << newProjectKey.toStdString() << std::endl;
            }
        }
    }

    if (joinedData.isEmpty()) {
        std::cout << "ERROR: No valid projects found to join." << std::endl;
        return false;
    }

    // Write the joined multi-project file
    if (JsonHandler::WriteJsonFile(joinedData, outputFile)) {
        std::cout << "Successfully joined " << projectCounter << " projects into: "
                  << outputFile.toStdString() << std::endl;
        return true;
    } else {
        std::cout << "ERROR: Failed to write joined file: " << outputFile.toStdString() << std::endl;
        return false;
    }
}

// Perform generic file conversion - Claude Generated enhanced with UUID/Title filtering
bool performGenericConversion(const QString& inputFile, const QString& outputFile, const QString& projectIndex = QString(),
    const QString& uuidFilter = QString(), const QString& titleFilter = QString(), bool splitOutput = false)
{
    std::cout << "Converting: " << inputFile.toStdString() << " -> " << outputFile.toStdString() << std::endl;

    // Load input file
    QJsonObject inputData = JsonHandler::LoadFile(inputFile);
    if (inputData.isEmpty()) {
        std::cout << "ERROR: Could not load input file: " << inputFile.toStdString() << std::endl;
        return false;
    }

    // Verify it's a simple project file
    FileConfigType type = detectConfigType(inputData);
    if (type != SimpleProject) {
        std::cout << "ERROR: File is not a simple project file. Use -i for task execution instead." << std::endl;
        return false;
    }

    // Check if this is a multi-project file
    QStringList keys = inputData.keys();
    bool isMultiProject = false;
    for (const QString& key : keys) {
        if (key.startsWith("project_")) {
            isMultiProject = true;
            break;
        }
    }

    if (isMultiProject) {
        // Handle multi-project conversion - Claude Generated enhanced with UUID/Title filtering
        std::cout << "Multi-project file detected." << std::endl;

        // Collect matching projects based on filters - Claude Generated
        QStringList matchingProjects;

        if (!projectIndex.isEmpty()) {
            // Legacy -p option: specific project by index
            bool ok;
            int index = projectIndex.toInt(&ok);
            if (!ok) {
                std::cout << "ERROR: Invalid project index '" << projectIndex.toStdString()
                          << "'. Must be a number." << std::endl;
                return false;
            }

            QString targetProjectKey = QString("project_%1").arg(index);
            if (inputData.contains(targetProjectKey)) {
                matchingProjects << targetProjectKey;
            } else {
                std::cout << "ERROR: Project " << targetProjectKey.toStdString()
                          << " not found in file." << std::endl;
                std::cout << "Available projects: ";
                QStringList availableProjects;
                for (auto it = inputData.begin(); it != inputData.end(); ++it) {
                    if (it.key().startsWith("project_")) {
                        availableProjects << it.key();
                    }
                }
                std::cout << availableProjects.join(", ").toStdString() << std::endl;
                return false;
            }
        } else if (!uuidFilter.isEmpty() || !titleFilter.isEmpty()) {
            // New UUID/Title filtering - Claude Generated
            std::cout << "Filtering projects by ";
            if (!uuidFilter.isEmpty())
                std::cout << "UUID: '" << uuidFilter.toStdString() << "'";
            if (!uuidFilter.isEmpty() && !titleFilter.isEmpty())
                std::cout << " and ";
            if (!titleFilter.isEmpty())
                std::cout << "Title: '" << titleFilter.toStdString() << "'";
            std::cout << std::endl;

            for (auto it = inputData.begin(); it != inputData.end(); ++it) {
                if (it.key().startsWith("project_")) {
                    QJsonObject project = it.value().toObject();
                    if (project.contains("data")) {
                        QJsonObject data = project["data"].toObject();

                        bool matchesUuid = uuidFilter.isEmpty();
                        bool matchesTitle = titleFilter.isEmpty();

                        if (!uuidFilter.isEmpty() && data.contains("uuid")) {
                            QString projectUuid = data["uuid"].toString();
                            matchesUuid = projectUuid.contains(uuidFilter, Qt::CaseInsensitive);
                        }

                        if (!titleFilter.isEmpty() && data.contains("title")) {
                            QString projectTitle = data["title"].toString();
                            matchesTitle = projectTitle.contains(titleFilter, Qt::CaseInsensitive);
                        }

                        if (matchesUuid && matchesTitle) {
                            matchingProjects << it.key();
                            std::cout << "   Found match: " << it.key().toStdString();
                            if (data.contains("title")) {
                                std::cout << " ('" << data["title"].toString().toStdString() << "')";
                            }
                            std::cout << std::endl;
                        }
                    }
                }
            }

            if (matchingProjects.isEmpty()) {
                std::cout << "ERROR: No projects match the specified filters." << std::endl;
                return false;
            }
        } else {
            // No specific filters: extract all projects (original behavior)
            for (auto it = inputData.begin(); it != inputData.end(); ++it) {
                if (it.key().startsWith("project_")) {
                    matchingProjects << it.key();
                }
            }
        }

        // Process matching projects - Claude Generated
        if (matchingProjects.size() == 1 || (!splitOutput && matchingProjects.size() > 1)) {
            // Single project OR multiple projects merged into one file
            if (matchingProjects.size() == 1) {
                // Extract single project
                QString projectKey = matchingProjects.first();
                QJsonObject singleProject = inputData[projectKey].toObject();
                if (JsonHandler::WriteJsonFile(singleProject, outputFile)) {
                    std::cout << "Extracted " << projectKey.toStdString()
                              << " -> " << outputFile.toStdString() << std::endl;
                    return true;
                } else {
                    std::cout << "ERROR: Failed to write project "
                              << projectKey.toStdString() << std::endl;
                    return false;
                }
            } else {
                // Merge multiple projects into one multi-project file
                QJsonObject mergedFile;
                for (const QString& projectKey : matchingProjects) {
                    mergedFile[projectKey] = inputData[projectKey];
                }
                if (JsonHandler::WriteJsonFile(mergedFile, outputFile)) {
                    std::cout << "Merged " << matchingProjects.size() << " projects -> "
                              << outputFile.toStdString() << std::endl;
                    std::cout << "   Projects: " << matchingProjects.join(", ").toStdString() << std::endl;
                    return true;
                } else {
                    std::cout << "ERROR: Failed to write merged projects file." << std::endl;
                    return false;
                }
            }
        } else {
            // Split multiple projects into separate files - Claude Generated
            std::cout << "Splitting " << matchingProjects.size() << " projects into separate files..." << std::endl;

            QFileInfo outputInfo(outputFile);
            QString baseName = outputInfo.completeBaseName();
            QString extension = outputInfo.suffix();
            QString outputDir = outputInfo.path();

            int projectCount = 0;
            for (const QString& projectKey : matchingProjects) {
                QString projectOutputFile = QString("%1/%2_%3.%4")
                                                .arg(outputDir)
                                                .arg(baseName)
                                                .arg(projectKey)
                                                .arg(extension.isEmpty() ? "suprafit" : extension);

                QJsonObject singleProject = inputData[projectKey].toObject();
                if (JsonHandler::WriteJsonFile(singleProject, projectOutputFile)) {
                    std::cout << "   Extracted " << projectKey.toStdString()
                              << " -> " << projectOutputFile.toStdString() << std::endl;
                    projectCount++;
                } else {
                    std::cout << "   ERROR: Failed to write " << projectKey.toStdString() << std::endl;
                }
            }

            std::cout << "Multi-project splitting completed: " << projectCount << " projects extracted." << std::endl;
            return projectCount > 0;
        }
    } else {
        // Perform standard conversion
        if (JsonHandler::WriteJsonFile(inputData, outputFile)) {
            std::cout << "Conversion completed successfully." << std::endl;
            return true;
        } else {
            std::cout << "ERROR: Could not write output file: " << outputFile.toStdString() << std::endl;
            return false;
        }
    }
}

// Execute task configuration - Claude Generated
bool executeTaskConfiguration(const QString& inputFile, const QString& outputOverride = "", bool autoExportML = false, const QString& mlOutputFile = "")
{
    std::cout << "Executing task configuration: " << inputFile.toStdString() << std::endl;

    QJsonObject config = JsonHandler::LoadFile(inputFile);
    if (config.isEmpty()) {
        std::cout << "ERROR: Could not load configuration file: " << inputFile.toStdString() << std::endl;
        return false;
    }

    FileConfigType type = detectConfigType(config);

    // Handle ML Pipeline
    if (type == MLPipelineConfig) {
        // Check if this is new ProcessMLPipeline or legacy ML Pipeline
        if (config.contains("ProcessMLPipeline") && config["ProcessMLPipeline"].toBool()) {
            std::cout << "New ProcessMLPipeline detected. Using enhanced SupraFitCli workflow..." << std::endl;
            // Execute using the new SupraFitCli ProcessMLPipeline implementation
            SupraFitCli* core = new SupraFitCli;
            core->setControlJson(config);

            if (!outputOverride.isEmpty()) {
                QFileInfo outputInfo(outputOverride);
                QString baseName = outputInfo.completeBaseName();
                QString outputDir = outputInfo.absolutePath();
                
                // Change to output directory if specified
                QString originalDir = QDir::currentPath();
                if (outputDir != "." && outputDir != originalDir) {
                    QDir::setCurrent(outputDir);
                }
                
                core->setOutFile(baseName);
            }

            if (!core->LoadFile()) {
                std::cout << "ERROR: Could not process input data for ProcessMLPipeline." << std::endl;
                delete core;
                return false;
            }

            // Execute ML Pipeline
            QVector<QJsonObject> results = core->ProcessMLPipeline();
            
            std::cout << "ProcessMLPipeline completed successfully!" << std::endl;
            
            // Auto-export ML training data if requested - Claude Generated
            if (autoExportML) {
                std::cout << "Auto-exporting ML training data..." << std::endl;
                
                // Determine output file name
                QString outputFileName = mlOutputFile;
                if (outputFileName.isEmpty()) {
                    QFileInfo inputInfo(inputFile);
                    outputFileName = inputInfo.completeBaseName() + "_ml_training.json";
                }
                
                // Export with JSON file containing ML RawData (with iteration number)
                QVector<QString> processedFiles;
                processedFiles.append(core->OutFile() + "-0.json");
                
                bool exportSuccess = MlExport::exportMLTrainingData(processedFiles, outputFileName);
                if (exportSuccess) {
                    std::cout << "ML training data exported to: " << outputFileName.toStdString() << std::endl;
                } else {
                    std::cout << "WARNING: ML training data export failed." << std::endl;
                }
            }
            
            delete core;
            return true;
        } else {
            std::cout << "Legacy ML Pipeline configuration detected. Switching to ML Pipeline mode..." << std::endl;
            MLPipelineManager pipelineManager;

            if (config.contains("BatchConfig")) {
                pipelineManager.setBatchConfig(config);
                pipelineManager.runBatchPipeline();
            } else {
                pipelineManager.runSinglePipeline(inputFile);
            }

            std::cout << "ML Pipeline completed successfully!" << std::endl;
            return true;
        }
    }

    // Handle Task Configuration
    if (type == TaskConfig) {
        SupraFitCli* core = new SupraFitCli;
        core->setInFile(inputFile); // Claude Generated - Fix missing setInFile call
        core->setControlJson(config);

        // Override output file if specified
        if (!outputOverride.isEmpty()) {
            // Extract base name without extension
            QFileInfo outputInfo(outputOverride);
            QString baseName = outputInfo.completeBaseName();
            QString outputDir = outputInfo.absolutePath();
            
            // Change to output directory if specified
            QString originalDir = QDir::currentPath();
            if (outputDir != "." && outputDir != originalDir) {
                QDir::setCurrent(outputDir);
            }
            
            core->setOutFile(baseName);
        }

        if (!core->LoadFile()) {
            std::cout << "ERROR: Could not process input data." << std::endl;
            delete core;
            return false;
        }

        // Execute appropriate tasks
        QVector<QJsonObject> results;

        // Check if ML Pipeline processing is requested first - Claude Generated
        QJsonObject config = core->getControlJson();
        if (config.contains("ProcessMLPipeline") && config["ProcessMLPipeline"].toBool()) {
            std::cout << "Executing ML Pipeline task..." << std::endl;
            results = core->ProcessMLPipeline();
        } else if (core->CheckDataOnly()) {
            std::cout << "Executing DataOnly task..." << std::endl;
            results = core->GenerateDataOnly();
        } else if (core->CheckGenerateInputData()) {
            std::cout << "Executing GenerateInputData task..." << std::endl;
            results = core->GenerateInputData();
        } else if (core->CheckGenerateDependent()) {
            std::cout << "Executing GenerateData task..." << std::endl;
            results = core->GenerateData();

            // Process with models if available
            if (!results.isEmpty()) {
                core->setDataVector(results);
                core->Work();
            }
        } else {
            std::cout << "No specific task detected. Running standard processing..." << std::endl;
            if (!core->SaveFile()) {
                std::cout << "ERROR: Could not save output file." << std::endl;
                delete core;
                return false;
            }
        }

        std::cout << "Task execution completed successfully." << std::endl;
        delete core;
        return true;
    }

    std::cout << "ERROR: Unrecognized configuration format." << std::endl;
    return false;
}

// Show comprehensive help - Claude Generated
void showComprehensiveHelp()
{
    std::cout << "\n=== SupraFit CLI - Command Line Interface ===\n"
              << std::endl;

    std::cout << "USAGE:\n";
    std::cout << "  suprafit_cli [file]                           # Analyze configuration file\n";
    std::cout << "  suprafit_cli -i <config> [-o <output>]        # Execute task configuration\n";
    std::cout << "  suprafit_cli -i <project> -o <converted>      # Convert project files\n";
    std::cout << "  suprafit_cli -i <multi> -o <out> [-p <X>]     # Extract project X from multi-project file\n";
    std::cout << "  suprafit_cli -i <multi> -o <out> [-u <uuid>] # Extract projects matching UUID\n";
    std::cout << "  suprafit_cli -i <multi> -o <out> [-t <title>] # Extract projects matching title\n";
    std::cout << "  suprafit_cli -i <file1> -i <file2> -o <out> -j # Join multiple files into multi-project\n";
    std::cout << "  suprafit_cli -l <file>                        # List file structure\n";
    std::cout << "  suprafit_cli --ml-pipeline -i <config>        # Run ML pipeline\n"
              << std::endl;

    std::cout << "MODES:\n";
    std::cout << "  1. File Analysis (read-only):\n";
    std::cout << "     suprafit_cli config.json\n";
    std::cout << "     → Analyzes and displays file content without execution\n\n";

    std::cout << "  2. Generic File Conversion:\n";
    std::cout << "     suprafit_cli -i project.json -o project.suprafit\n";
    std::cout << "     suprafit_cli -i data.suprafit -o data.json\n";
    std::cout << "     → Converts between JSON and SupraFit project formats\n\n";

    std::cout << "  3. Task Execution:\n";
    std::cout << "     suprafit_cli -i task_config.json\n";
    std::cout << "     suprafit_cli -i generate_data.json -o results\n";
    std::cout << "     → Executes data generation, modeling, or analysis tasks\n\n";

    std::cout << "  4. ML Pipeline:\n";
    std::cout << "     suprafit_cli --ml-pipeline -i ml_config.json\n";
    std::cout << "     suprafit_cli --batch-config batch.json\n";
    std::cout << "     → Runs machine learning pipeline workflows\n\n";

    std::cout << "  5. Multi-Project Files:\n";
    std::cout << "     suprafit_cli projects.json\n";
    std::cout << "     suprafit_cli -i projects.json -o extracted_projects      # Extract all projects\n";
    std::cout << "     suprafit_cli -i projects.json -o project.suprafit -p 0   # Extract only project_0\n";
    std::cout << "     suprafit_cli -i projects.json -o filtered.suprafit -t \"NMR\" # Extract projects with 'NMR' in title\n";
    std::cout << "     suprafit_cli -i projects.json -o split_projects -t \"Test\" -s # Split matching projects into separate files\n";
    std::cout << "     → Analyzes/converts files with project_0, project_1, etc.\n\n";

    std::cout << "  6. File Joining:\n";
    std::cout << "     suprafit_cli -i file1.suprafit -i file2.suprafit -o combined.json -j\n";
    std::cout << "     suprafit_cli -i nmr_*.suprafit -o nmr_collection.json -j\n";
    std::cout << "     → Combines multiple individual or multi-project files into one\n\n";

    std::cout << "OPTIONS:\n";
    std::cout << "  -i, --input <file>     Input configuration or project file\n";
    std::cout << "  -o, --output <file>    Output file name (overrides config settings)\n";
    std::cout << "  -p, --project <index>  Extract specific project from multi-project file\n";
    std::cout << "  -u, --uuid <uuid>      Extract projects matching UUID (supports partial match)\n";
    std::cout << "  -t, --title <title>    Extract projects matching title (supports partial match)\n";
    std::cout << "  -s, --split            Split multiple selected projects into separate files\n";
    std::cout << "  -j, --join             Join multiple input files into single multi-project file\n";
    std::cout << "  -l, --list             List file structure for debugging\n";
    std::cout << "  -n, --nproc <N>        Number of parallel threads (default: 4)\n";
    std::cout << "  --ml-pipeline          Enable ML pipeline mode\n";
    std::cout << "  --batch-config <file>  Run ML pipeline batch processing\n";
    std::cout << "  -x, --extract-parameters [N]  Extract fitted parameters from models file (optional: specify model index)\n";
    std::cout << "  -h, --help             Show this help message\n";
    std::cout << "  -v, --version          Show version information\n"
              << std::endl;

    std::cout << "EXAMPLES:\n";
    std::cout << "  # Analyze a configuration file\n";
    std::cout << "  suprafit_cli my_config.json\n\n";

    std::cout << "  # Convert project formats\n";
    std::cout << "  suprafit_cli -i project.json -o project.suprafit\n";
    std::cout << "  suprafit_cli -i data.suprafit -o converted.json\n\n";

    std::cout << "  # Generate synthetic data\n";
    std::cout << "  suprafit_cli -i generate_config.json -o synthetic_data\n\n";

    std::cout << "  # Run model fitting\n";
    std::cout << "  suprafit_cli -i fitting_config.json\n\n";

    std::cout << "  # Debug file structure\n";
    std::cout << "  suprafit_cli -l complex_project.suprafit\n\n";

    std::cout << "  # Extract fitted parameters from all models\n";
    std::cout << "  suprafit_cli -x project-models-0.suprafit\n\n";

    std::cout << "  # Extract parameters from specific model\n";
    std::cout << "  suprafit_cli -x 2 project-models-0.suprafit\n\n";

    std::cout << "FILE TYPES:\n";
    std::cout << "  • Simple Projects: SupraFit project files with data and models\n";
    std::cout << "  • Multi-Projects: Files containing project_0, project_1, etc.\n";
    std::cout << "  • Task Configs: Configuration files with Main/GenerateData sections\n";
    std::cout << "  • ML Configs: Machine learning pipeline configurations\n"
              << std::endl;

    std::cout << "For detailed documentation, visit: https://github.com/conradhuebler/SupraFit\n"
              << std::endl;
}

int main(int argc, char** argv)
{
#ifndef _WIN32
#if __GNUC__
    signal(SIGSEGV, bt_handler);
    signal(SIGABRT, bt_handler);
#endif
#endif

    SupraFit::timer t;
    qInstallMessageHandler(myMessageOutput);

    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    Version(&app, &parser);

    // Simplified CLI options - Claude Generated
    QCommandLineOption input(QStringList() << "i" << "input",
        "Input configuration or project file", "file");
    parser.addOption(input);

    QCommandLineOption output(QStringList() << "o" << "output",
        "Output file name (overrides config settings)", "file");
    parser.addOption(output);

    QCommandLineOption list(QStringList() << "l" << "list",
        "List file structure for debugging");
    parser.addOption(list);

    QCommandLineOption threads(QStringList() << "n" << "nproc",
        "Number of parallel threads", "threads", "4");
    parser.addOption(threads);

    QCommandLineOption project(QStringList() << "p" << "project",
        "Extract specific project from multi-project file (e.g., -p 0 for project_0)", "index");
    parser.addOption(project);

    QCommandLineOption uuid(QStringList() << "u" << "uuid",
        "Extract projects matching UUID (can select multiple projects)", "uuid");
    parser.addOption(uuid);

    QCommandLineOption title(QStringList() << "t" << "title",
        "Extract projects matching title (can select multiple projects)", "title");
    parser.addOption(title);

    QCommandLineOption split(QStringList() << "s" << "split",
        "Split multiple selected projects into separate files");
    parser.addOption(split);

    QCommandLineOption join(QStringList() << "j" << "join",
        "Join multiple input files into a single multi-project file (use with multiple -i options)");
    parser.addOption(join);

    QCommandLineOption mlPipeline(QStringList() << "ml-pipeline",
        "Enable ML pipeline mode");
    parser.addOption(mlPipeline);

    QCommandLineOption batchConfig(QStringList() << "batch-config",
        "Run ML pipeline batch processing", "file");
    parser.addOption(batchConfig);
    
    QCommandLineOption showPostProcessing(QStringList() << "show-post-processing",
        "Show detailed post-processing statistics for all methods");
    parser.addOption(showPostProcessing);

    // ML Training Data Export Options - Claude Generated
    QCommandLineOption exportMLTraining(QStringList() << "export-ml-training",
        "Export compact ML training data from ML pipeline files", "files");
    parser.addOption(exportMLTraining);
    
    QCommandLineOption mlOutput(QStringList() << "ml-output",
        "Output filename for ML training data export", "output.json");
    parser.addOption(mlOutput);
    
    QCommandLineOption autoExportML(QStringList() << "auto-export-ml",
        "Automatically export ML training data after ML pipeline execution");
    parser.addOption(autoExportML);
    
    QCommandLineOption exportMLBatch(QStringList() << "export-ml-batch",
        "Batch export ML training data from directory", "directory");
    parser.addOption(exportMLBatch);

    QCommandLineOption excludeRawData(QStringList() << "exclude-raw-data",
        "Exclude raw statistical data from ML training export (smaller file size)");
    parser.addOption(excludeRawData);

    // Claude Generated: Add parameter extraction option
    QCommandLineOption extractParameters(QStringList() << "x" << "extract-parameters",
        "Extract fitted model parameters from results file");
    parser.addOption(extractParameters);
    
    QCommandLineOption extractModel("extract-model",
        "Specific model index to extract (use with --extract-parameters)", "model_index");
    parser.addOption(extractModel);

    parser.process(app);

    // Show version info
    std::cout << aboutSF().toStdString() << std::endl;

    // Command line echo
    for (int index = 0; index < argc; ++index)
        std::cout << argv[index] << " ";
    std::cout << std::endl
              << std::endl;

    // Parse command line arguments - Claude Generated
    const QString inputFile = parser.value("input");
    const QStringList inputFiles = parser.values("input"); // For multiple -i options
    const QString outputFile = parser.value("output");
    const QString projectIndex = parser.value("project");
    const QString uuidFilter = parser.value("uuid");
    const QString titleFilter = parser.value("title");
    const bool splitOutput = parser.isSet("split");
    const bool joinMode = parser.isSet("join");
    const bool showList = parser.isSet("list");
    const bool mlPipelineMode = parser.isSet("ml-pipeline");
    const QString batchConfigFile = parser.value("batch-config");
    
    // ML Training Data Export Parameters - Claude Generated
    const bool exportMLTrainingMode = parser.isSet("export-ml-training");
    const QString mlOutputFile = parser.value("ml-output");
    const bool autoExportMLMode = parser.isSet("auto-export-ml");
    const QString exportMLBatchDirectory = parser.value("export-ml-batch");
    const bool excludeRawDataMode = parser.isSet("exclude-raw-data");
    
    // Claude Generated - Thread count parsing and validation
    const QString threadCountStr = parser.value("nproc");
    int threadCount = 4;  // Default value
    
    if (!threadCountStr.isEmpty()) {
        bool ok;
        threadCount = threadCountStr.toInt(&ok);
        
        // Validate thread count
        if (!ok || threadCount <= 0 || threadCount > 256) {
            std::cout << "ERROR: Invalid thread count '" << threadCountStr.toStdString() 
                      << "'. Must be a positive integer between 1 and 256." << std::endl;
            return 1;
        }
    }
    
    // Claude Generated: Parameter extraction parameters
    const bool extractParametersMode = parser.isSet("extract-parameters");
    const QString modelIndex = parser.value("extract-model");

    // Set thread count
    qApp->instance()->setProperty("threads", parser.value("nproc").toInt());
    qApp->instance()->setProperty("series_confidence", true);
    qApp->instance()->setProperty("InitialiseRandom", true);
    qApp->instance()->setProperty("StoreRawData", true);

    // New simplified CLI logic - Claude Generated

    // 1. Handle special cases first

    // ML Pipeline batch mode
    if (!batchConfigFile.isEmpty()) {
        std::cout << "Running ML Pipeline batch mode with: " << batchConfigFile.toStdString() << std::endl;

        QJsonObject batchConfig = JsonHandler::LoadFile(batchConfigFile);
        if (batchConfig.isEmpty()) {
            std::cout << "ERROR: Could not load batch configuration file." << std::endl;
            return 1;
        }

        MLPipelineManager pipelineManager;
        pipelineManager.setBatchConfig(batchConfig);
        pipelineManager.runBatchPipeline();

        std::cout << "ML Pipeline batch processing completed!" << std::endl;
        return 0;
    }

    // Show help if no arguments
    if (inputFile.isEmpty() && parser.positionalArguments().isEmpty()) {
        showComprehensiveHelp();
        return 0;
    }

    // 2. Determine input file (from -i or positional argument)
    QString actualInputFile = inputFile;
    if (actualInputFile.isEmpty() && !parser.positionalArguments().isEmpty()) {
        actualInputFile = parser.positionalArguments().first();
    }

    if (actualInputFile.isEmpty()) {
        std::cout << "ERROR: No input file specified." << std::endl;
        showComprehensiveHelp();
        return 1;
    }

    // 3. Handle list mode - Claude Generated with improved error handling
    if (showList) {
        std::cout << "Listing file structure: " << actualInputFile.toStdString() << std::endl;
        
        // Check if file exists before attempting to load
        if (!QFile::exists(actualInputFile)) {
            std::cout << "ERROR: Could not load file for listing." << std::endl;
            return 1;
        }

        SupraFitCli* cli = new SupraFitCli;
        cli->setInFile(actualInputFile);
        if (cli->LoadFile()) {
            cli->PrintFileStructure();
        } else {
            std::cout << "ERROR: Could not load file for listing." << std::endl;
            delete cli;
            return 1;
        }
        delete cli;
        return 0;
    }

    // 4. Handle Parameter Extraction mode - Claude Generated
    if (extractParametersMode) {
        std::cout << "Extracting model parameters from: " << actualInputFile.toStdString() << std::endl;
        
        SupraFitCli* cli = new SupraFitCli;
        cli->setInFile(actualInputFile);
        
        if (!cli->LoadFile()) {
            std::cout << "ERROR: Could not load file for parameter extraction." << std::endl;
            delete cli;
            return 1;
        }
        
        // Extract and display model parameters (loaded into ProjectManager above)
        bool success = AnalysisReporter::ExtractModelParameters(modelIndex);
        
        delete cli;
        return success ? 0 : 1;
    }

    // 5. Handle ML Training Data Export mode - Claude Generated
    if (exportMLTrainingMode) {
        std::cout << "Exporting ML training data from: " << actualInputFile.toStdString() << std::endl;
        
        SupraFitCli* cli = new SupraFitCli;
        bool success = false;
        
        if (!exportMLBatchDirectory.isEmpty()) {
            // Batch export mode
            std::cout << "Batch export mode - processing directory: " << exportMLBatchDirectory.toStdString() << std::endl;
            success = MlExport::exportMLTrainingDataBatch(exportMLBatchDirectory,
                                                   mlOutputFile.isEmpty() ? actualInputFile + "_ml_training.json" : mlOutputFile,
                                                   excludeRawDataMode);
        } else {
            // Single file export mode
            QVector<QString> inputFiles;
            inputFiles.append(actualInputFile);
            success = MlExport::exportMLTrainingData(inputFiles,
                                              mlOutputFile.isEmpty() ? actualInputFile + "_ml_training.json" : mlOutputFile,
                                              excludeRawDataMode);
        }
        
        if (success) {
            std::cout << "ML training data export completed successfully!" << std::endl;
        } else {
            std::cout << "ERROR: ML training data export failed." << std::endl;
            delete cli;
            return 1;
        }
        
        delete cli;
        return 0;
    }

    // 5. Handle ML Pipeline mode
    if (mlPipelineMode) {
        // Claude Generated - Validate output path before ML pipeline execution
        if (!validateOutputPath(outputFile)) {
            return 1;
        }
        return executeTaskConfiguration(actualInputFile, outputFile, autoExportMLMode, mlOutputFile) ? 0 : 1;
    }

    // 6. Main processing logic

    // Load and analyze input file
    QJsonObject config = JsonHandler::LoadFile(actualInputFile);
    if (config.isEmpty()) {
        std::cout << "ERROR: Could not load input file: " << actualInputFile.toStdString() << std::endl;
        return 1;
    }

    FileConfigType configType = detectConfigType(config);

    // Only filename provided (no -i flag) = Analysis mode
    if (inputFile.isEmpty() && !parser.positionalArguments().isEmpty()) {
        std::cout << "=== File Analysis Mode ===" << std::endl;
        std::cout << "File: " << actualInputFile.toStdString() << std::endl;

        // Show file type
        QString typeStr;
        switch (configType) {
        case SimpleProject:
            typeStr = "Simple SupraFit Project";
            break;
        case TaskConfig:
            typeStr = "Task Configuration";
            break;
        case MLPipelineConfig:
            typeStr = "ML Pipeline Configuration";
            break;
        default:
            typeStr = "Unknown/Invalid";
            break;
        }
        std::cout << "Type: " << typeStr.toStdString() << std::endl
                  << std::endl;

        // Check if this is a multi-project file
        QStringList keys = config.keys();
        bool isMultiProject = false;
        for (const QString& key : keys) {
            if (key.startsWith("project_")) {
                isMultiProject = true;
                break;
            }
        }

        if (isMultiProject) {
            // Handle multi-project files specially
            analyzeMultiProjectFile(config);
        } else {
            // Perform standard detailed analysis
            SupraFitCli* cli = new SupraFitCli;
            cli->setInFile(actualInputFile);
            cli->setShowPostProcessingDetails(parser.isSet(showPostProcessing));
            if (cli->LoadFile()) {
                cli->AnalyzeFile();
            } else {
                std::cout << "ERROR: Could not analyze file." << std::endl;
                delete cli;
                return 1;
            }
            delete cli;
        }
        return 0;
    }

    // -i flag provided = Execution mode
    if (!inputFile.isEmpty()) {
        // Special case: Join mode with multiple input files - Claude Generated
        if (joinMode && inputFiles.size() > 1 && !outputFile.isEmpty()) {
            return performFileJoin(inputFiles, outputFile) ? 0 : 1;
        }

        // Check if this is conversion or execution
        if (!outputFile.isEmpty() && configType == SimpleProject) {
            // Simple project conversion
            // Claude Generated - Validate output path before conversion
            if (!validateOutputPath(outputFile)) {
                return 1;
            }
            return performGenericConversion(actualInputFile, outputFile, projectIndex, uuidFilter, titleFilter, splitOutput) ? 0 : 1;
        } else {
            // Task execution
            // Claude Generated - Validate output path before task execution
            if (!validateOutputPath(outputFile)) {
                return 1;
            }
            return executeTaskConfiguration(actualInputFile, outputFile, autoExportMLMode, mlOutputFile) ? 0 : 1;
        }
    }

    std::cout << "ERROR: Invalid command line arguments." << std::endl;
    showComprehensiveHelp();
    return 1;  // Claude Generated - Fix: Invalid arguments should return failure code
}
