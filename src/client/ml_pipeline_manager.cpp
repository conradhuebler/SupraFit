/*
 * ML Pipeline Manager for SupraFit CLI
 * Copyright (C) 2024 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "ml_pipeline_manager.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QMutexLocker>
#include <QtCore/QRandomGenerator>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include <QtCore/QThreadPool>

#include "src/core/analyse.h"
#include "src/core/jsonhandler.h"
#include "src/core/models/models.h"

MLPipelineManager::MLPipelineManager(QObject* parent)
    : QObject(parent)
    , m_isRunning(false)
    , m_currentBatch(0)
    , m_totalBatches(0)
    , m_numWorkerThreads(QThread::idealThreadCount())
{
}

MLPipelineManager::~MLPipelineManager()
{
    cleanupWorkers();
}

void MLPipelineManager::setBatchConfig(const QJsonObject& config)
{
    m_batchConfig = config;
    
    if (config.contains("BatchConfig")) {
        QJsonObject batchConfig = config["BatchConfig"].toObject();
        m_totalBatches = batchConfig["TotalBatches"].toInt(10);
        m_numWorkerThreads = batchConfig["WorkerThreads"].toInt(QThread::idealThreadCount());
    }
}

void MLPipelineManager::setPipelineSteps(const QStringList& steps)
{
    m_pipelineSteps = steps;
}

void MLPipelineManager::runBatchPipeline()
{
    if (m_isRunning) {
        qWarning() << "Pipeline is already running";
        return;
    }
    
    m_isRunning = true;
    m_currentBatch = 0;
    m_mlDataset.clear();
    
    qDebug() << "Starting batch pipeline with" << m_totalBatches << "batches";
    
    // Initialize worker threads
    initializeWorkers();
    
    // Process batches
    for (int i = 0; i < m_totalBatches; ++i) {
        QJsonObject batchConfig = generateParameterVariation(m_batchConfig, i);
        
        BatchWorker* worker = new BatchWorker(i, batchConfig, this);
        connect(worker, &BatchWorker::batchProcessed, this, &MLPipelineManager::onBatchFinished);
        connect(worker, &BatchWorker::batchError, this, &MLPipelineManager::onPipelineError);
        connect(worker, &BatchWorker::finished, worker, &QObject::deleteLater);
        
        worker->start();
        
        emit progressUpdate(i + 1, m_totalBatches);
    }
}

void MLPipelineManager::runSinglePipeline(const QString& configFile)
{
    qDebug() << "Running single pipeline with config:" << configFile;
    
    QJsonObject config = JsonHandler::LoadFile(configFile);
    if (config.isEmpty()) {
        emit errorOccurred("Failed to load configuration file: " + configFile);
        return;
    }
    qDebug() << "Loading file content" << config;
    // Check if this is a GenerateData configuration
    if (config.contains("Main") && config["Main"].toObject().contains("GenerateData")) {
        qDebug() << "=== STEP 1: DATA GENERATION ===";
        
        SupraFitCli cli;
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
        cli.setOutFile(outFile);
        
        // Load the input data file
        if (!cli.LoadFile()) {
            qDebug() << "ERROR: Failed to load input file:" << inFile;
            emit errorOccurred("Failed to load input file: " + inFile);
            return;
        }
        
        // Prepare config in the format expected by SupraFitCli
        QJsonObject modifiedConfig = config;
        QJsonObject modifiedMain = mainConfig;
        
        // Move GenerateData parameters to main level and set Tasks
        if (mainConfig.contains("GenerateData")) {
            QJsonObject generateData = mainConfig["GenerateData"].toObject();
            qDebug() << "GenerateData parameters found, modifying main configuration";
            qDebug() << generateData;

            // Copy all GenerateData parameters to main level
            for (auto it = generateData.begin(); it != generateData.end(); ++it) {
                modifiedMain[it.key()] = it.value();
            }
            
            // Special handling for models - GenerateData() needs "models" in Main
            if (generateData.contains("Model")) {
                QJsonObject models;
                models["simulation_model"] = generateData["Model"].toInt();
                modifiedMain["models"] = models;
            }
            
            // Set Tasks to trigger generation - now using GenerateData method
            modifiedMain["Tasks"] = "GenerateData";
            
            // IMPORTANT: Set simulation JSON explicitly - this is synonymous with GenerateData
            // This ensures m_simulation is populated in SupraFitCli::setControlJson
            modifiedConfig["simulation"] = generateData;
            modifiedConfig["Main"] = modifiedMain;
        }
        
        // Set control JSON for data generation
        cli.setControlJson(modifiedConfig);
        qDebug() << modifiedConfig;
        
        // Generate data using the simulation approach (GenerateData method)
        QVector<QJsonObject> generatedData;
        
        qDebug() << "Generating simulation data using GenerateData method...";
        generatedData = cli.GenerateData();
        qDebug() << "Generated" << generatedData.size() << "simulation datasets";
        
        // Process each generated dataset for table extraction
        for (int i = 0; i < generatedData.size(); ++i) {
            const QJsonObject& simulationData = generatedData[i];
            
            // Extract and save data tables for debug and as .dat files
            if (simulationData.contains("data")) {
                QJsonObject dataContent = simulationData["data"].toObject();
                
                // Extract independent table
                if (dataContent.contains("independent")) {
                    QJsonObject independentTable = dataContent["independent"].toObject();
                    qDebug() << "Independent table size:" << independentTable["rows"].toInt() << "x" << independentTable["cols"].toInt();
                    
                    // Save as .dat file
                    QString independentFile = QString("%1_independent_%2.dat").arg(outFile).arg(i);
                    saveTableAsDat(independentTable, independentFile);
                }
                
                // Extract dependent table
                if (dataContent.contains("dependent")) {
                    QJsonObject dependentTable = dataContent["dependent"].toObject();
                    qDebug() << "Dependent table size:" << dependentTable["rows"].toInt() << "x" << dependentTable["cols"].toInt();
                    
                    // Save as .dat file
                    QString dependentFile = QString("%1_dependent_%2.dat").arg(outFile).arg(i);
                    saveTableAsDat(dependentTable, dependentFile);
                    
                    // Combine independent and dependent for full experiment file
                    QString experimentFile = QString("%1_experiment_%2.dat").arg(outFile).arg(i);
                    saveExperimentAsDat(dataContent.value("independent").toObject(), dependentTable, experimentFile);
                }
            }
        }
        
        qDebug() << "Generated" << generatedData.size() << "datasets";
        
        // Save each generated dataset as individual files (JSON or suprafit based on extension)
        for (int i = 0; i < generatedData.size(); ++i) {
            QString filename;
            if (outFile.endsWith(".json")) {
                filename = QString("%1_%2.json").arg(outFile.left(outFile.lastIndexOf(".json"))).arg(i);
            } else {
                filename = QString("%1_%2.suprafit").arg(outFile).arg(i);
            }
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
        
        QString summaryFile = QString("%1_summary.json").arg(outFile);
        JsonHandler::WriteJsonFile(summary, summaryFile);
        
        qDebug() << "Step 1 completed. Generated" << generatedData.size() << "datasets";
        
    } else {
        qDebug() << "ERROR: No GenerateData configuration found";
        emit errorOccurred("No GenerateData configuration found");
    }
}

QJsonObject MLPipelineManager::extractMLFeatures(const QVector<QJsonObject>& models)
{
    QJsonObject features;
    QJsonArray modelFeatures;
    
    for (const auto& model : models) {
        QJsonObject modelData = model["data"].toObject();
        QJsonObject basicStats;
        
        // Extract basic statistics
        basicStats["SSE"] = modelData["SSE"].toDouble();
        basicStats["AIC"] = modelData["AIC"].toDouble();
        basicStats["AICc"] = modelData["AICc"].toDouble();
        basicStats["SEy"] = modelData["SEy"].toDouble();
        basicStats["ChiSquared"] = modelData["ChiSquared"].toDouble();
        basicStats["RSquared"] = modelData["RSquared"].toDouble();
        
        QJsonObject modelFeature;
        modelFeature["model_name"] = model["name"].toString();
        modelFeature["model_id"] = model["id"].toInt();
        modelFeature["basic_statistics"] = basicStats;
        
        modelFeatures.append(modelFeature);
    }
    
    features["models"] = modelFeatures;
    return features;
}

QJsonObject MLPipelineManager::extractStatisticToolFeatures(const QVector<QJsonObject>& models)
{
    QJsonObject features;
    
    // Run StatisticTool analyses
    QString mcAnalysis = runMonteCarloAnalysis(models);
    QString cvAnalysis = runCrossValidationAnalysis(models);
    QString raAnalysis = runReductionAnalysis(models);
    
    // Parse results
    features["monte_carlo"] = parseStatisticToolOutput(mcAnalysis, "MonteCarlo");
    features["cross_validation"] = parseStatisticToolOutput(cvAnalysis, "CrossValidation");
    features["reduction_analysis"] = parseStatisticToolOutput(raAnalysis, "ReductionAnalysis");
    
    return features;
}

QString MLPipelineManager::runMonteCarloAnalysis(const QVector<QJsonObject>& models, bool local, int index)
{
    return StatisticTool::CompareMC(models, local, index);
}

QString MLPipelineManager::runCrossValidationAnalysis(const QVector<QJsonObject>& models, int cvtype, bool local, int cv_x)
{
    return StatisticTool::CompareCV(models, cvtype, local, cv_x);
}

QString MLPipelineManager::runReductionAnalysis(const QVector<QJsonObject>& models, bool local, double cutoff)
{
    return StatisticTool::AnalyseReductionAnalysis(models, local, cutoff);
}

QJsonObject MLPipelineManager::formatMLTrainingData(const QVector<QJsonObject>& experimentalData, 
                                                   const QVector<QJsonObject>& analysisResults,
                                                   int trueModelId)
{
    QJsonObject trainingData;
    QJsonArray samples;
    
    for (int i = 0; i < analysisResults.size(); ++i) {
        QJsonObject sample;
        
        // Features
        QJsonObject features = extractMLFeatures(QVector<QJsonObject>() << analysisResults[i]);
        sample["features"] = features;
        
        // Labels
        int testedModelId = analysisResults[i]["id"].toInt();
        sample["labels"] = generateMLLabels(trueModelId, testedModelId, features);
        
        // Metadata
        sample["experiment_id"] = experimentalData[i]["id"].toString();
        sample["true_model_id"] = trueModelId;
        sample["tested_model_id"] = testedModelId;
        
        samples.append(sample);
    }
    
    trainingData["samples"] = samples;
    trainingData["metadata"] = QJsonObject{
        {"total_samples", samples.size()},
        {"true_model_id", trueModelId},
        {"generation_timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)}
    };
    
    return trainingData;
}

void MLPipelineManager::processBatch(int batchId, const QJsonObject& batchConfig)
{
    qDebug() << "Processing batch" << batchId;
    
    // Create batch-specific configuration
    QJsonObject config = batchConfig;
    config["BatchId"] = batchId;
    
    // Run pipeline steps
    for (const QString& step : m_pipelineSteps) {
        runSinglePipeline(step);
    }
    
    emit batchCompleted(batchId);
}

void MLPipelineManager::saveMLDataset(const QString& outputDir, const QVector<QJsonObject>& mlData)
{
    QDir dir(outputDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Save individual batch results
    for (int i = 0; i < mlData.size(); ++i) {
        QString filename = QString("%1/batch_%2.json").arg(outputDir).arg(i);
        JsonHandler::WriteJsonFile(mlData[i], filename);
    }
    
    // Save combined dataset
    QJsonObject dataset;
    QJsonArray batches;
    for (const auto& batch : mlData) {
        batches.append(batch);
    }
    dataset["batches"] = batches;
    dataset["metadata"] = QJsonObject{
        {"total_batches", mlData.size()},
        {"generation_timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)}
    };
    
    QString combinedFilename = QString("%1/ml_dataset_combined.json").arg(outputDir);
    JsonHandler::WriteJsonFile(dataset, combinedFilename);
    
    qDebug() << "ML dataset saved to" << outputDir;
}

void MLPipelineManager::saveStructuredResults(const QString& filename, const QJsonObject& results)
{
    if (JsonHandler::WriteJsonFile(results, filename)) {
        qDebug() << "Structured results saved to" << filename;
    } else {
        qWarning() << "Failed to save structured results to" << filename;
    }
}

void MLPipelineManager::onBatchFinished()
{
    QMutexLocker locker(&m_dataMutex);
    m_currentBatch++;
    
    if (m_currentBatch >= m_totalBatches) {
        m_isRunning = false;
        emit pipelineCompleted();
        
        // Save final dataset
        if (!m_mlDataset.isEmpty()) {
            QString outputDir = m_batchConfig["BatchConfig"].toObject()["OutputDir"].toString();
            saveMLDataset(outputDir, m_mlDataset);
        }
    }
}

void MLPipelineManager::onPipelineError(int batchId, const QString& error)
{
    qWarning() << "Pipeline error in batch" << batchId << ":" << error;
    emit errorOccurred(error);
}

void MLPipelineManager::initializeWorkers()
{
    // Setup thread pool
    QThreadPool::globalInstance()->setMaxThreadCount(m_numWorkerThreads);
}

void MLPipelineManager::cleanupWorkers()
{
    // Clean up any remaining threads
    for (QThread* thread : m_workerThreads) {
        if (thread && thread->isRunning()) {
            thread->quit();
            thread->wait();
        }
        delete thread;
    }
    m_workerThreads.clear();
}

QJsonObject MLPipelineManager::parseStatisticToolOutput(const QString& output, const QString& method)
{
    QJsonObject parsed;
    
    // Parse HTML output from StatisticTool methods
    // This is a simplified parser - would need more sophisticated HTML parsing
    
    QStringList lines = output.split('\n');
    QJsonArray rankings;
    
    for (const QString& line : lines) {
        if (line.contains("H(x)") || line.contains("σ")) {
            // Extract ranking information
            QJsonObject ranking;
            // ... parse ranking data
            rankings.append(ranking);
        }
    }
    
    parsed["method"] = method;
    parsed["rankings"] = rankings;
    parsed["raw_output"] = output;
    
    return parsed;
}

QJsonObject MLPipelineManager::generateParameterVariation(const QJsonObject& baseConfig, int variationIndex)
{
    QJsonObject varied = baseConfig;
    
    // Add variation to parameters based on index
    if (baseConfig.contains("ParameterVariation")) {
        QJsonObject paramVar = baseConfig["ParameterVariation"].toObject();
        
        // Vary noise levels
        if (paramVar.contains("NoiseVariance")) {
            QJsonArray noiseArray = paramVar["NoiseVariance"].toArray();
            int noiseIndex = variationIndex % noiseArray.size();
            
            // Update main config with varied noise
            QJsonObject main = varied["Main"].toObject();
            QJsonObject generateData = main["GenerateData"].toObject();
            generateData["Variance"] = noiseArray[noiseIndex].toDouble();
            main["GenerateData"] = generateData;
            varied["Main"] = main;
        }
    }
    
    return varied;
}

QJsonObject MLPipelineManager::generateMLLabels(int trueModelId, int testedModelId, const QJsonObject& features)
{
    QJsonObject labels;
    
    labels["true_model_id"] = trueModelId;
    labels["tested_model_id"] = testedModelId;
    labels["is_correct_model"] = (trueModelId == testedModelId);
    
    // Generate quality score based on features
    double qualityScore = 0.0;
    if (features.contains("basic_statistics")) {
        QJsonObject stats = features["basic_statistics"].toObject();
        double aic = stats["AIC"].toDouble();
        double sse = stats["SSE"].toDouble();
        
        // Simple quality score (lower is better)
        qualityScore = 1.0 / (1.0 + aic + sse);
    }
    
    labels["model_quality_score"] = qualityScore;
    labels["confidence_level"] = (trueModelId == testedModelId) ? 1.0 : qualityScore;
    
    return labels;
}

// BatchWorker implementation
BatchWorker::BatchWorker(int batchId, const QJsonObject& config, QObject* parent)
    : QThread(parent)
    , m_batchId(batchId)
    , m_config(config)
    , m_cli(new SupraFitCli())
{
}

void BatchWorker::run()
{
    try {
        qDebug() << "Starting batch worker" << m_batchId;
        
        m_cli->setControlJson(m_config);
        
        // Step 1: Generate data
        QVector<QJsonObject> generatedData = m_cli->GenerateData();
        
        // Step 2: Analyze data
        // ... analysis logic
        
        QJsonObject results;
        results["batch_id"] = m_batchId;
        results["generated_data"] = QJsonArray::fromVariantList(QVariantList());
        results["analysis_results"] = QJsonArray::fromVariantList(QVariantList());
        
        emit batchProcessed(m_batchId, results);
        
    } catch (const std::exception& e) {
        emit batchError(m_batchId, QString("Batch processing failed: %1").arg(e.what()));
    }
}

void MLPipelineManager::saveTableAsDat(const QJsonObject& table, const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for writing:" << filename;
        return;
    }
    
    QTextStream out(&file);
    
    // Get table dimensions
    int rows = table["rows"].toInt();
    
    // Write header if available
    if (table.contains("header")) {
        QString header = table["header"].toString();
        if (!header.isEmpty()) {
            out << "# " << header.replace("|", "\t") << "\n";
        }
    }
    
    // Write data
    QJsonObject data = table["data"].toObject();
    for (int i = 0; i < rows; ++i) {
        QString rowKey = QString::number(i);
        if (data.contains(rowKey)) {
            QString rowData = data[rowKey].toString();
            // Convert comma-separated to tab-separated
            rowData = rowData.replace(",", "\t");
            // Remove trailing separator if present
            if (rowData.endsWith("\t")) {
                rowData = rowData.left(rowData.length() - 1);
            }
            out << rowData << "\n";
        }
    }
    
    file.close();
    qDebug() << "Saved table as .dat file:" << filename;
}

void MLPipelineManager::saveExperimentAsDat(const QJsonObject& independentTable, const QJsonObject& dependentTable, const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for writing:" << filename;
        return;
    }
    
    QTextStream out(&file);
    
    // Get dimensions
    int indepRows = independentTable["rows"].toInt();
    int depRows = dependentTable["rows"].toInt();
    int indepCols = independentTable["cols"].toInt();
    int depCols = dependentTable["cols"].toInt();
    
    int rows = qMin(indepRows, depRows); // Use minimum of both
    
    // Write header
    QString indepHeader = independentTable["header"].toString();
    QString depHeader = dependentTable["header"].toString();
    
    QStringList headers;
    if (!indepHeader.isEmpty()) {
        headers << indepHeader.split("|");
    }
    if (!depHeader.isEmpty()) {
        headers << depHeader.split("|");
    }
    
    if (!headers.isEmpty()) {
        out << "# " << headers.join("\t") << "\n";
    }
    
    // Write combined data (independent + dependent columns)
    QJsonObject indepData = independentTable["data"].toObject();
    QJsonObject depData = dependentTable["data"].toObject();
    
    for (int i = 0; i < rows; ++i) {
        QString rowKey = QString::number(i);
        QStringList rowValues;
        
        // Add independent columns
        if (indepData.contains(rowKey)) {
            QString indepRow = indepData[rowKey].toString();
            QStringList indepValues = indepRow.split(",");
            rowValues << indepValues;
        }
        
        // Add dependent columns
        if (depData.contains(rowKey)) {
            QString depRow = depData[rowKey].toString();
            QStringList depValues = depRow.split(",");
            rowValues << depValues;
        }
        
        // Remove empty trailing values
        while (!rowValues.isEmpty() && rowValues.last().isEmpty()) {
            rowValues.removeLast();
        }
        
        out << rowValues.join("\t") << "\n";
    }
    
    file.close();
    qDebug() << "Saved experiment as .dat file:" << filename << 
               QString("(%1 rows, %2+%3 cols)").arg(rows).arg(indepCols).arg(depCols);
}

#include "ml_pipeline_manager.moc"