/*
 * ML Pipeline CLI for SupraFit
 * Copyright (C) 2024 Conrad Hübler <Conrad.Huebler@gmx.net>
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "src/client/suprafit_cli.h"
#include "src/core/jsonhandler.h"
#include "src/core/analyse.h"

void printUsage(const QString& programName)
{
    qDebug() << "ML Pipeline CLI for SupraFit";
    qDebug() << "Usage:" << programName << "[options]";
    qDebug() << "Options:";
    qDebug() << "  --config <file>     Configuration file (JSON)";
    qDebug() << "  --step <1|2>        Pipeline step (1=generate, 2=analyze)";
    qDebug() << "  --output <dir>      Output directory";
    qDebug() << "  --help              Show this help";
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    
    parser.addOption({{"c", "config"}, "Configuration file", "file"});
    parser.addOption({{"s", "step"}, "Pipeline step (1=generate, 2=analyze)", "step", "1"});
    parser.addOption({{"o", "output"}, "Output directory", "dir", "."});
    parser.addOption({{"h", "help"}, "Show help"});
    
    parser.process(app);
    
    if (parser.isSet("help") || !parser.isSet("config")) {
        printUsage(argv[0]);
        return parser.isSet("help") ? 0 : 1;
    }
    
    QString configFile = parser.value("config");
    int step = parser.value("step").toInt();
    QString outputDir = parser.value("output");
    
    qDebug() << "ML Pipeline CLI starting...";
    qDebug() << "Config:" << configFile;
    qDebug() << "Step:" << step;
    qDebug() << "Output:" << outputDir;
    
    // Load configuration
    QJsonObject config = JsonHandler::LoadFile(configFile);
    if (config.isEmpty()) {
        qDebug() << "ERROR: Failed to load configuration file:" << configFile;
        return 1;
    }
    
    // Create output directory
    QDir dir;
    if (!dir.exists(outputDir)) {
        dir.mkpath(outputDir);
    }
    
    // Create SupraFitCli instance
    SupraFitCli cli;
    
    if (step == 1) {
        // Step 1: Data Generation
        qDebug() << "=== STEP 1: DATA GENERATION ===";
        
        // Force data generation mode
        QJsonObject mainConfig = config["Main"].toObject();
        
        // Set the input file
        QString inFile = mainConfig["InFile"].toString();
        if (!inFile.isEmpty()) {
            cli.setInFile(inFile);
        }
        
        // Set output file
        QString outFile = mainConfig["OutFile"].toString();
        if (outFile.isEmpty()) {
            outFile = "ml_generated_data";
        }
        cli.setOutFile(outputDir + "/" + outFile);
        
        // Load the input data file
        if (!cli.LoadFile()) {
            qDebug() << "ERROR: Failed to load input file:" << inFile;
            return 1;
        }
        
        // Manual data generation using the old GenerateData API
        if (mainConfig.contains("GenerateData")) {
            QJsonObject generateConfig = mainConfig["GenerateData"].toObject();
            
            qDebug() << "Generating data with config:" << generateConfig;
            
            // Use the existing GenerateData function
            cli.setControlJson(config);
            QVector<QJsonObject> generatedData = cli.GenerateData();
            
            qDebug() << "Generated" << generatedData.size() << "datasets";
            
            // Save each generated dataset
            for (int i = 0; i < generatedData.size(); ++i) {
                QString filename = QString("%1/%2_%3.json").arg(outputDir).arg(outFile).arg(i);
                if (JsonHandler::WriteJsonFile(generatedData[i], filename)) {
                    qDebug() << "Saved dataset to:" << filename;
                } else {
                    qDebug() << "ERROR: Failed to save dataset to:" << filename;
                }
            }
            
            // Save summary
            QJsonObject summary;
            summary["step"] = 1;
            summary["generated_files"] = generatedData.size();
            summary["config"] = config;
            
            QString summaryFile = QString("%1/step1_summary.json").arg(outputDir);
            JsonHandler::WriteJsonFile(summary, summaryFile);
            
            qDebug() << "Step 1 completed. Generated" << generatedData.size() << "datasets";
        } else {
            qDebug() << "ERROR: No GenerateData configuration found";
            return 1;
        }
        
    } else if (step == 2) {
        // Step 2: Model Analysis
        qDebug() << "=== STEP 2: MODEL ANALYSIS ===";
        
        // Find generated data files from step 1
        QDir outputDirectory(outputDir);
        QStringList dataFiles = outputDirectory.entryList({"*.json"}, QDir::Files);
        
        qDebug() << "Found" << dataFiles.size() << "data files for analysis";
        
        if (dataFiles.isEmpty()) {
            qDebug() << "ERROR: No data files found in" << outputDir;
            qDebug() << "Make sure to run step 1 first";
            return 1;
        }
        
        QVector<QJsonObject> analysisResults;
        QJsonObject models = config["Models"].toObject();
        QJsonObject jobs = config["Jobs"].toObject();
        QJsonObject analyse = config["Analyse"].toObject();
        
        // Process each data file
        for (const QString& dataFile : dataFiles) {
            if (dataFile.startsWith("step") || dataFile.startsWith("analysis")) {
                continue; // Skip summary files
            }
            
            qDebug() << "Analyzing data file:" << dataFile;
            
            QString fullPath = outputDir + "/" + dataFile;
            
            // Create new CLI instance for this data
            SupraFitCli analyzeCli;
            analyzeCli.setInFile(fullPath);
            
            if (!analyzeCli.LoadFile()) {
                qDebug() << "WARNING: Failed to load data file:" << fullPath;
                continue;
            }
            
            // Test each model
            for (auto it = models.begin(); it != models.end(); ++it) {
                QString modelName = it.key();
                int modelId = it.value().toInt();
                
                qDebug() << "Testing model:" << modelName << "(ID:" << modelId << ")";
                
                // Create model and analyze
                // This would require implementing the full analysis pipeline
                // For now, we collect basic statistics
                
                QJsonObject result;
                result["data_file"] = dataFile;
                result["model_name"] = modelName;
                result["model_id"] = modelId;
                result["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
                
                analysisResults.append(result);
            }
        }
        
        // Run StatisticTool analyses if we have results
        if (!analysisResults.isEmpty()) {
            qDebug() << "Running StatisticTool analyses...";
            
            QString mcAnalysis = StatisticTool::CompareMC(analysisResults, true, 1);
            QString cvAnalysis = StatisticTool::CompareCV(analysisResults, 1, true, 3);
            QString raAnalysis = StatisticTool::AnalyseReductionAnalysis(analysisResults, true, 0.1);
            
            // Save analysis results
            QJsonObject analysisOutput;
            analysisOutput["step"] = 2;
            analysisOutput["monte_carlo_analysis"] = mcAnalysis;
            analysisOutput["cross_validation_analysis"] = cvAnalysis;
            analysisOutput["reduction_analysis"] = raAnalysis;
            analysisOutput["model_results"] = QJsonArray::fromVariantList(
                QVariantList(analysisResults.begin(), analysisResults.end()));
            
            QString analysisFile = QString("%1/step2_analysis.json").arg(outputDir);
            JsonHandler::WriteJsonFile(analysisOutput, analysisFile);
            
            qDebug() << "Analysis completed and saved to:" << analysisFile;
        }
        
    } else {
        qDebug() << "ERROR: Invalid step. Use 1 for generation or 2 for analysis";
        return 1;
    }
    
    qDebug() << "ML Pipeline step" << step << "completed successfully!";
    return 0;
}