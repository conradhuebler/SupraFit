/*
 * AnalysisManager - Centralized Analysis Infrastructure for SupraFit
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 * 
 * This class centralizes analysis functionality previously duplicated between
 * CLI and GUI, providing a unified API for file analysis, model comparison,
 * and statistical analysis. Migrated from src/client/suprafit_cli.cpp
 * 
 * Claude Generated - 2025-01-09
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

#include "analysis_manager.h"

#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include "src/core/jsonhandler.h"
#include "src/core/filehandler.h"
#include "src/core/projectmanager.h"
#include "src/core/analyse.h"
#include "src/core/toolset.h"
#include "src/core/jsonutils.h"
#include "src/core/models/models.h"
#include "src/capabilities/jobmanager.h"


AnalysisManager::AnalysisManager(QObject* parent)
    : QObject(parent)
{
    // Claude Generated
}

QJsonObject AnalysisManager::analyzeFile(const QString& filePath)
{
    QJsonObject result;
    
    // Basic file information - migrated from CLI AnalyzeFile()
    QJsonObject fileInfo = analyzeFileInformation(filePath);
    result["fileInfo"] = fileInfo;
    
    // Load data using ProjectManager for consistency 
    auto& projectManager = SupraFit::ProjectManager::instance();
    // Load project using ProjectManager
    bool loadSuccess = projectManager.loadProject(filePath);
    if (!loadSuccess) {
        result["success"] = false;
        result["error"] = QString("Failed to load file: %1").arg(filePath);
        return result;
    }
    
    QWeakPointer<DataClass> weakData = projectManager.getCurrentProject();
    QSharedPointer<DataClass> sharedData = weakData.toStrongRef();
    QPointer<DataClass> data(sharedData.data());
    
    if (!data) {
        result["error"] = "Failed to load data from file";
        result["success"] = false;
        return result;
    }
    
    // Analyze data structure
    QJsonObject dataAnalysis = analyzeDataClass(data);
    result["dataAnalysis"] = dataAnalysis;
    
    // Extract configuration and model information
    // Use static method to load JSON file
    QJsonObject toplevel = JsonHandler::LoadFile(filePath);
    if (!toplevel.isEmpty()) {
        
        // Configuration analysis
        QJsonObject configAnalysis;
        configAnalysis["main"] = toplevel["main"];
        configAnalysis["independent"] = toplevel["independent"]; 
        configAnalysis["dependent"] = toplevel["dependent"];
        configAnalysis["models"] = toplevel["models"];
        configAnalysis["jobs"] = toplevel["jobs"];
        result["configuration"] = configAnalysis;
        
        // Model statistics analysis
        QVector<ModelStatistics> modelStats = extractModelStatistics(toplevel);
        result["modelStatistics"] = generateModelStatisticsJson(modelStats);
    }
    
    result["success"] = true;
    m_totalAnalysisCount++;
    m_successfulAnalysisCount++;
    
    emit analysisCompleted(true, result);
    return result;
}

QJsonObject AnalysisManager::analyzeDataClass(QPointer<DataClass> data)
{
    QJsonObject result;
    
    if (!data) {
        result["error"] = "Invalid data pointer";
        return result;
    }
    
    // Data structure analysis - migrated from CLI
    result["dataStructure"] = analyzeDataStructure(data);
    result["independentData"] = analyzeIndependentData(data);
    result["dependentData"] = analyzeDependentDataAndModels(data);
    
    // System parameters
    QJsonArray systemParams;
    QList<int> paramList = data->getSystemParameterList();
    for (int index : paramList) {
        SystemParameter param = data->getSystemParameter(index);
        QJsonObject paramObj;
        paramObj["index"] = index;
        paramObj["name"] = param.Name();
        paramObj["description"] = param.Description();
        paramObj["value"] = QJsonValue::fromVariant(param.value());
        systemParams.append(paramObj);
    }
    result["systemParameters"] = systemParams;
    
    // Metadata analysis
    QJsonObject exportData = data->ExportData();
    QJsonObject metadata;
    metadata["title"] = exportData["title"];
    metadata["uuid"] = exportData["uuid"];
    metadata["dataType"] = exportData["DataType"];
    metadata["content"] = exportData["content"];
    metadata["comment"] = exportData["comment"];
    metadata["instructions"] = exportData["instructions"];
    metadata["gitCommit"] = exportData["git_commit"];
    metadata["timestamp"] = exportData["timestamp"];
    metadata["suprafitVersion"] = exportData["SupraFit"];
    result["metadata"] = metadata;
    
    return result;
}

QVector<QJsonObject> AnalysisManager::fitModelsToData(QPointer<DataClass> data, 
                                                     const QJsonObject& modelsConfig,
                                                     const QJsonObject& globalAnalysisConfig)
{
    fmt::print("🔍 DEBUG AnalysisManager: fitModelsToData() called with {} models\n", modelsConfig.size());
    fmt::print("🔍 DEBUG AnalysisManager: data has {} independent rows, {} dependent rows\n", 
               data ? data->IndependentModel()->rowCount() : -1, 
               data ? data->DependentModel()->rowCount() : -1);
    
    QVector<QJsonObject> results;
    
    if (!data) {
        QJsonObject error;
        error["error"] = "Invalid data pointer";
        error["success"] = false;
        results.append(error);
        return results;
    }
    
    emit analysisProgress("Starting model fitting analysis", 0);
    
    // Extract models from configuration - Support both formats (Claude Generated - Critical Bug Fix)
    QJsonArray models;
    if (modelsConfig.contains("models") && modelsConfig["models"].isArray()) {
        // Format 1: {"models": [...]} array format
        models = modelsConfig["models"].toArray();
        fmt::print("🔍 DEBUG: Using models array format with {} models\n", models.size());
    } else {
        // Format 2: {"nmr_1_1": {"ID": 1}, ...} object format (CLI format)
        fmt::print("🔍 DEBUG: Converting CLI object format to models array\n");
        for (auto it = modelsConfig.begin(); it != modelsConfig.end(); ++it) {
            if (it.value().isObject()) {
                QJsonObject modelConfig = it.value().toObject();
                modelConfig["model_name"] = it.key(); // Add model name for identification
                models.append(modelConfig);
            }
        }
        fmt::print("🔍 DEBUG: Converted {} CLI models to array format\n", models.size());
    }
    
    if (models.isEmpty()) {
        QJsonObject error;
        error["error"] = "No models specified in configuration";
        error["success"] = false;
        fmt::print("❌ ERROR: No models found in configuration\n");
        results.append(error);
        return results;
    }
    
    fmt::print("🔍 DEBUG: Processing {} models for fitting\n", models.size());
    
    // Fit each model with progress reporting
    for (int i = 0; i < models.size(); ++i) {
        QJsonObject modelConfig = models[i].toObject();
        
        int progress = (i * 100) / models.size();
        emit analysisProgress(QString("Fitting model %1 of %2").arg(i+1).arg(models.size()), progress);
        
        QJsonObject modelResult = fitSingleModel(modelConfig, data, globalAnalysisConfig);
        results.append(modelResult);
    }
    
    emit analysisProgress("Model fitting completed", 100);
    return results;
}

QJsonObject AnalysisManager::performCompleteAnalysis(QSharedPointer<AbstractModel> model,
                                                    const QJsonObject& analysisConfig)
{
    /**
     * @brief Execute statistical analysis using JobManager integration
     * Migrated from original CLI implementation for consistency - Claude Generated
     */
    QJsonObject result;
    
    if (!model) {
        result["error"] = "Invalid model pointer";
        result["success"] = false;
        return result;
    }
    
    // Basic model information
    result["modelName"] = model->Name();
    result["converged"] = model->isConverged();
    result["sse"] = model->SSE();
    result["aic"] = model->GetAIC();
    result["globalParameters"] = model->GlobalParameterSize();
    result["localParameters"] = model->LocalParameterSize();
    
    // Execute statistical analysis using JobManager (migrated from CLI)
    JobManager jobManager;
    jobManager.setModel(model);
    
    QJsonObject methodResults;
    
    // Monte Carlo analysis
    if (analysisConfig.contains("monteCarlo") && analysisConfig.value("monteCarlo").toBool()) {
        QJsonObject job;
        job["Method"] = 1;  // Monte Carlo
        job["MaxSteps"] = analysisConfig.contains("mcIterations") ? analysisConfig.value("mcIterations").toInt() : 1000;
        job["VarianceSource"] = analysisConfig.contains("mcVarianceSource") ? analysisConfig.value("mcVarianceSource").toInt() : 2;
        
        jobManager.AddSingleJob(job);
        jobManager.RunJobs();
        
        // Extract results from model after JobManager execution - Claude Generated Fix
        QJsonObject modelExport = model->ExportModel(true, false);
        QJsonObject mcResults = SupraFit::JsonUtils::getStatisticalMethod(modelExport, SupraFit::Method::MonteCarlo);
        
        if (!mcResults.isEmpty()) {
            // Store results back in the model using official API - Follow JSON_DOKUMENTATION.md
            model->UpdateStatistic(mcResults);
            result["monteCarlo"] = mcResults;
            methodResults["MonteCarlo"] = mcResults;
        }
    }
    
    // Cross Validation
    if (analysisConfig.contains("crossValidation") && analysisConfig.value("crossValidation").toBool()) {
        QJsonObject job;
        job["Method"] = 4;  // Cross Validation
        job["CVType"] = analysisConfig.contains("cvType") ? analysisConfig.value("cvType").toInt() : 1;
        job["MaxSteps"] = analysisConfig.contains("cvX") ? analysisConfig.value("cvX").toInt() : 100;
        
        jobManager.AddSingleJob(job);
        jobManager.RunJobs();
        
        // Extract results from model after JobManager execution - Claude Generated Fix
        QJsonObject modelExport = model->ExportModel(true, false);
        QJsonObject cvResults = SupraFit::JsonUtils::getStatisticalMethod(modelExport, SupraFit::Method::CrossValidation);
        
        if (!cvResults.isEmpty()) {
            // Store results back in the model using official API - Follow JSON_DOKUMENTATION.md
            model->UpdateStatistic(cvResults);
            result["crossValidation"] = cvResults;
            methodResults["CrossValidation"] = cvResults;
        }
    }
    
    // Reduction Analysis
    if (analysisConfig.contains("reductionAnalysis") && analysisConfig.value("reductionAnalysis").toBool()) {
        QJsonObject job;
        job["Method"] = 5;  // Reduction Analysis
        
        jobManager.AddSingleJob(job);
        jobManager.RunJobs();
        
        // Extract results from model after JobManager execution
        QJsonObject modelExport = model->ExportModel(true, false);
        QJsonObject reductionResults = SupraFit::JsonUtils::getStatisticalMethod(modelExport, SupraFit::Method::Reduction);
        
        if (!reductionResults.isEmpty()) {
            result["reductionAnalysis"] = reductionResults;
            methodResults["Reduction"] = reductionResults;
        }
    }
    
    result["methodResults"] = methodResults;
    result["success"] = true;
    return result;
}

QJsonObject AnalysisManager::executePostFitAnalysis(QSharedPointer<AbstractModel> model, 
                                                   const QJsonObject& analysisConfig)
{
    // Wrapper for performCompleteAnalysis with JobManager integration potential
    return performCompleteAnalysis(model, analysisConfig);
}

QSharedPointer<AbstractModel> AnalysisManager::createModelFromConfig(int modelId, QPointer<DataClass> data)
{
    if (!data) {
        return nullptr;
    }
    
    // Use existing model creation infrastructure
    QSharedPointer<AbstractModel> model = CreateModel(modelId, data);
    return model;
}

QJsonObject AnalysisManager::generateModelComparisonSummary(const QVector<QJsonObject>& models)
{
    QJsonObject summary;
    
    if (models.isEmpty()) {
        summary["error"] = "No models provided";
        return summary;
    }
    
    // Extract comparison metrics
    QJsonArray modelsArray;
    double bestAIC = 1e10;
    double bestSSE = 1e10;
    QString bestAICModel, bestSSEModel;
    
    for (const auto& model : models) {
        if (model.contains("error")) {
            continue; // Skip failed models
        }
        
        QJsonObject modelSummary;
        modelSummary["name"] = model["modelName"];
        modelSummary["converged"] = model["converged"];
        modelSummary["sse"] = model["sse"];
        modelSummary["aic"] = model["aic"];
        
        // Track best models
        double aic = model["aic"].toDouble();
        double sse = model["sse"].toDouble();
        
        if (aic < bestAIC) {
            bestAIC = aic;
            bestAICModel = model["modelName"].toString();
        }
        
        if (sse < bestSSE) {
            bestSSE = sse;
            bestSSEModel = model["modelName"].toString();
        }
        
        modelsArray.append(modelSummary);
    }
    
    summary["models"] = modelsArray;
    summary["bestAICModel"] = bestAICModel;
    summary["bestAIC"] = bestAIC;
    summary["bestSSEModel"] = bestSSEModel;
    summary["bestSSE"] = bestSSE;
    summary["totalModels"] = modelsArray.size();
    
    return summary;
}

// ===== PRIVATE HELPER METHODS =====

QJsonObject AnalysisManager::analyzeFileInformation(const QString& filePath)
{
    QJsonObject info;
    QFileInfo fileInfo(filePath);
    
    info["inputFile"] = filePath;
    info["fileSize"] = qint64(fileInfo.size());
    info["fileExtension"] = fileInfo.suffix();
    info["absolutePath"] = fileInfo.absoluteFilePath();
    info["exists"] = fileInfo.exists();
    info["lastModified"] = fileInfo.lastModified().toString(Qt::ISODate);
    
    return info;
}

QJsonObject AnalysisManager::analyzeDataStructure(QPointer<DataClass> data)
{
    QJsonObject structure;
    
    if (!data) {
        return structure;
    }
    
    structure["dataType"] = static_cast<int>(data->Type());
    QJsonObject exportData = data->ExportData();
    structure["title"] = exportData["title"].toString();
    structure["uuid"] = exportData["uuid"].toString();
    
    return structure;
}

QJsonObject AnalysisManager::analyzeIndependentData(QPointer<DataClass> data)
{
    QJsonObject analysis;
    
    if (!data || !data->IndependentModel()) {
        analysis["error"] = "No independent data found";
        return analysis;
    }
    
    DataTable* indepModel = data->IndependentModel();
    int rows = indepModel->rowCount();
    int cols = indepModel->columnCount();
    
    analysis["dimensions"] = QJsonObject{{"rows", rows}, {"columns", cols}};
    
    QStringList headers = indepModel->header();
    QJsonArray headerArray;
    for (const QString& header : headers) {
        headerArray.append(header);
    }
    analysis["headers"] = headerArray;
    
    // Sample data (first 5 rows)
    if (rows > 0 && cols > 0) {
        QJsonArray sampleData;
        for (int i = 0; i < qMin(5, rows); ++i) {
            QJsonArray row;
            for (int j = 0; j < cols; ++j) {
                row.append(indepModel->data(i, j));
            }
            sampleData.append(row);
        }
        analysis["sampleData"] = sampleData;
    }
    
    return analysis;
}

QJsonObject AnalysisManager::analyzeDependentDataAndModels(QPointer<DataClass> data)
{
    QJsonObject analysis;
    
    if (!data || !data->DependentModel()) {
        analysis["error"] = "No dependent data found";
        return analysis;
    }
    
    DataTable* depModel = data->DependentModel();
    int rows = depModel->rowCount();
    int cols = depModel->columnCount();
    
    analysis["dimensions"] = QJsonObject{{"rows", rows}, {"columns", cols}};
    
    QStringList headers = depModel->header();
    QJsonArray headerArray;
    for (const QString& header : headers) {
        headerArray.append(header);
    }
    analysis["headers"] = headerArray;
    
    // Sample data (first 5 rows)
    if (rows > 0 && cols > 0) {
        QJsonArray sampleData;
        for (int i = 0; i < qMin(5, rows); ++i) {
            QJsonArray row;
            for (int j = 0; j < cols; ++j) {
                row.append(depModel->data(i, j));
            }
            sampleData.append(row);
        }
        analysis["sampleData"] = sampleData;
    }
    
    return analysis;
}

QJsonObject AnalysisManager::fitSingleModel(const QJsonObject& modelConfig,
                                           QPointer<DataClass> data,
                                           const QJsonObject& globalAnalysisConfig)
{
    QJsonObject result;
    
    try {
        // Support both "id" (lowercase) and "ID" (uppercase) keys - Claude Generated Critical Bug Fix
        int modelId = 0;
        if (modelConfig.contains("ID")) {
            modelId = modelConfig["ID"].toInt();
            fmt::print("🔍 DEBUG: Using model ID {} from 'ID' key\n", modelId);
        } else if (modelConfig.contains("id")) {
            modelId = modelConfig["id"].toInt();
            fmt::print("🔍 DEBUG: Using model ID {} from 'id' key\n", modelId);
        } else {
            result["error"] = "No model ID found in configuration (expected 'ID' or 'id')";
            result["success"] = false;
            fmt::print("❌ ERROR: No model ID found in configuration\n");
            return result;
        }
        
        fmt::print("🔧 DEBUG: Creating model with ID {} for analysis\n", modelId);
        QSharedPointer<AbstractModel> model = createModelFromConfig(modelId, data);
        
        if (!model) {
            result["error"] = QString("Failed to create model with ID %1").arg(modelId);
            result["success"] = false;
            return result;
        }
        
        // Configure model parameters if provided
        if (modelConfig.contains("globalParameters")) {
            QJsonArray globalParams = modelConfig["globalParameters"].toArray();
            for (int i = 0; i < globalParams.size() && i < model->GlobalParameterSize(); ++i) {
                model->setGlobalParameter(globalParams[i].toDouble(), i);
            }
        }
        
        // Apply model options (migrated from CLI)
        if (modelConfig.contains("Options")) {
            QJsonObject options = modelConfig["Options"].toObject();
            if (options.contains("FastMode") && options["FastMode"].toBool()) {
                model->setFast(true);
            }
        }
        
        // Perform model fitting (enhanced from CLI approach)
        model->InitialGuess();
        
        bool shouldFit = globalAnalysisConfig.contains("FitModels") ? globalAnalysisConfig.value("FitModels").toBool() : true;
        if (shouldFit) {
            // Use proper fitting approach (would need NonLinearFitThread integration)
            // For now, use Calculate() but mark for future enhancement
            model->setFast(false);
            model->Calculate();
            model->CalculateStatistics(true);
            // TODO: Integrate NonLinearFitThread for proper model fitting
        } else {
            model->setFast(false);
            model->Calculate();
        }
        
        // Evaluate model fit with comprehensive statistics
        int trueModelId = globalAnalysisConfig.contains("trueModelId") ? globalAnalysisConfig.value("trueModelId").toInt() : -1;
        QJsonObject evaluation = evaluateModelFit(model, trueModelId);
        
        // Execute post-fit analysis if enabled (Claude Generated - Fixed configuration detection)
        QJsonObject modelSpecificConfig = modelConfig["PostFitAnalysis"].toObject();
        
        // Check if post-fit analysis should run:
        // 1. Global config has methods array (indicating post-fit analysis is configured)
        // 2. Model-specific config is enabled
        bool runGlobalAnalysis = globalAnalysisConfig.contains("methods") && globalAnalysisConfig["methods"].isArray();
        bool runModelAnalysis = modelSpecificConfig.contains("enabled") && modelSpecificConfig.value("enabled").toBool();
        
        fmt::print("🔍 DEBUG AnalysisManager: runGlobalAnalysis = {} runModelAnalysis = {}\n", runGlobalAnalysis, runModelAnalysis);
        fmt::print("🔍 DEBUG AnalysisManager: globalAnalysisConfig keys: [");
        for (auto it = globalAnalysisConfig.begin(); it != globalAnalysisConfig.end(); ++it) {
            fmt::print("{}", it.key().toStdString());
            if (std::next(it) != globalAnalysisConfig.end()) fmt::print(", ");
        }
        fmt::print("]\n");
        fmt::print("🔍 DEBUG AnalysisManager: modelSpecificConfig keys: [");
        for (auto it = modelSpecificConfig.begin(); it != modelSpecificConfig.end(); ++it) {
            fmt::print("{}", it.key().toStdString());
            if (std::next(it) != modelSpecificConfig.end()) fmt::print(", ");
        }
        fmt::print("]\n");
        
        if (runGlobalAnalysis || runModelAnalysis) {
            QJsonObject analysisConfig = globalAnalysisConfig;
            if (runModelAnalysis) {
                // Model-specific config overrides global config
                analysisConfig = modelSpecificConfig;
            }
            
            fmt::print("      Running post-fit analysis for model {}...\n", model->Name().toStdString());
            QJsonObject postFitResults = runPostFitAnalysis(model, analysisConfig);
            evaluation["post_fit_analysis"] = postFitResults;
            
            if (postFitResults.contains("method_count") && postFitResults["method_count"].toInt() > 0) {
                fmt::print("        ✅ Post-fit analysis completed with {} methods\n", 
                          postFitResults["method_count"].toInt());
            } else {
                fmt::print("        ⚠️  Post-fit analysis ran but no methods completed\n");
            }
        } else {
            fmt::print("        Post-fit analysis disabled\n");
        }
        
        // Include model export (complete model state)
        evaluation["model_export"] = model->ExportModel(true, true);
        
        result = evaluation;
        result["success"] = true;
        
    } catch (const std::exception& e) {
        result["error"] = QString("Exception during model fitting: %1").arg(e.what());
        result["success"] = false;
    }
    
    return result;
}

QVector<ModelStatistics> AnalysisManager::extractModelStatistics(const QJsonObject& toplevel)
{
    QVector<ModelStatistics> modelStats;
    
    // Search for model objects (model_0, model_1, etc.)
    for (auto it = toplevel.begin(); it != toplevel.end(); ++it) {
        QString key = it.key();
        if (key.startsWith("model_") && it.value().isObject()) {
            QJsonObject modelObj = it.value().toObject();
            ModelStatistics stats = extractSingleModelStatistics(key, modelObj);
            modelStats.append(stats);
        }
    }
    
    return modelStats;
}

ModelStatistics AnalysisManager::extractSingleModelStatistics(const QString& key, const QJsonObject& modelObj)
{
    ModelStatistics stats;
    stats.key = key;
    stats.hasValidStats = false;
    
    // Extract model name
    stats.name = modelObj["name"].toString();
    if (stats.name.isEmpty()) {
        stats.name = modelObj["model"].toString();
    }
    if (stats.name.isEmpty()) {
        stats.name = key;
    }
    
    // Check convergence
    bool converged = modelObj["converged"].toBool(false);
    stats.status = converged ? "✅ Conv" : "❌ Failed";
    
    // Extract fit statistics
    stats.sse = modelObj["SSE"].toDouble(-1);
    stats.sae = modelObj["SAE"].toDouble(-1);
    stats.aic = modelObj["AIC"].toDouble(-999);
    stats.aicc = modelObj["AICc"].toDouble(-999);
    
    // Extract parameter counts from data structure
    QJsonObject data = modelObj["data"].toObject();
    if (!data.isEmpty()) {
        QJsonObject globalParamObj = data["globalParameter"].toObject();
        if (!globalParamObj.isEmpty()) {
            stats.globalParams = globalParamObj["rows"].toInt(-1);
        }
        
        QJsonObject localParamObj = data["localParameter"].toObject();
        if (!localParamObj.isEmpty()) {
            stats.localParams = localParamObj["rows"].toInt(-1);
        }
        
        // Analyze post-processing methods
        if (data.contains("methods")) {
            analyzePostProcessingMethods(stats, data["methods"].toObject(), {modelObj});
        }
    }
    
    // Check validity
    stats.hasValidStats = (stats.sse >= 0 || stats.sae >= 0 || stats.aic > -999 || 
                          stats.aicc > -999 || stats.globalParams > 0 || stats.localParams > 0);
    
    return stats;
}

void AnalysisManager::analyzePostProcessingMethods(ModelStatistics& stats, 
                                                  const QJsonObject& methods,
                                                  const QVector<QJsonObject>& modelVector)
{
    for (auto it = methods.begin(); it != methods.end(); ++it) {
        QJsonObject method = it.value().toObject();
        
        if (method.isEmpty())
            continue;
        
        // Look for controller
        QJsonObject controller;
        bool foundController = false;
        
        if (method.contains("controller")) {
            controller = method["controller"].toObject();
            foundController = true;
        } else {
            // Check nested controllers under numeric keys
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
        
        // Count and cache post-processing methods
        switch (methodType) {
            case 1: // MonteCarlo
                stats.mcBlocks++;
                if (!stats.postProcessingData.contains("MonteCarlo")) {
                    stats.postProcessingData["MonteCarlo"] = StatisticTool::CalculateMCMetrics(modelVector, 1);
                }
                break;
            case 2: // WeakenedGridSearch
                stats.wgsBlocks++;
                if (!stats.postProcessingData.contains("WeakenedGridSearch")) {
                    stats.postProcessingData["WeakenedGridSearch"] = StatisticTool::CalculateWGSMetrics(modelVector);
                }
                break;
            case 3: // ModelComparison
                stats.modelCompBlocks++;
                if (!stats.postProcessingData.contains("ModelComparison")) {
                    stats.postProcessingData["ModelComparison"] = StatisticTool::CalculateModelComparisonMetrics(modelVector);
                }
                break;
            case 4: // CrossValidation
                stats.cvBlocks++;
                if (!stats.postProcessingData.contains("CrossValidation")) {
                    stats.postProcessingData["CrossValidation"] = StatisticTool::CalculateCVMetrics(modelVector, 1, 3);
                }
                break;
            case 5: // Reduction
                stats.reductionBlocks++;
                if (!stats.postProcessingData.contains("Reduction")) {
                    stats.postProcessingData["Reduction"] = StatisticTool::CalculateReductionMetrics(modelVector, 0.1);
                }
                break;
            case 6: // FastConfidence
                stats.fastConfBlocks++;
                if (!stats.postProcessingData.contains("FastConfidence")) {
                    stats.postProcessingData["FastConfidence"] = StatisticTool::CalculateFastConfidenceMetrics(modelVector);
                }
                break;
            case 7: // GlobalSearch
                stats.globalBlocks++;
                if (!stats.postProcessingData.contains("GlobalSearch")) {
                    stats.postProcessingData["GlobalSearch"] = StatisticTool::CalculateGlobalSearchMetrics(modelVector);
                }
                break;
        }
    }
    
    stats.totalPPBlocks = stats.mcBlocks + stats.wgsBlocks + stats.modelCompBlocks + 
                         stats.cvBlocks + stats.reductionBlocks + stats.fastConfBlocks + stats.globalBlocks;
}

QJsonObject AnalysisManager::generateModelStatisticsJson(const QVector<ModelStatistics>& models)
{
    QJsonObject result;
    
    if (models.isEmpty()) {
        result["message"] = "No fitted models found";
        return result;
    }
    
    QJsonArray modelsArray;
    for (const auto& model : models) {
        QJsonObject modelObj;
        modelObj["key"] = model.key;
        modelObj["name"] = model.name;
        modelObj["status"] = model.status;
        modelObj["hasValidStats"] = model.hasValidStats;
        
        // Core statistics
        modelObj["sse"] = model.sse;
        modelObj["sae"] = model.sae;
        modelObj["aic"] = model.aic;
        modelObj["aicc"] = model.aicc;
        
        // Parameters
        modelObj["globalParams"] = model.globalParams;
        modelObj["localParams"] = model.localParams;
        
        // Post-processing counts
        QJsonObject ppCounts;
        ppCounts["monteCarlo"] = model.mcBlocks;
        ppCounts["weakenedGridSearch"] = model.wgsBlocks;
        ppCounts["modelComparison"] = model.modelCompBlocks;
        ppCounts["crossValidation"] = model.cvBlocks;
        ppCounts["reduction"] = model.reductionBlocks;
        ppCounts["fastConfidence"] = model.fastConfBlocks;
        ppCounts["globalSearch"] = model.globalBlocks;
        ppCounts["total"] = model.totalPPBlocks;
        modelObj["postProcessingCounts"] = ppCounts;
        
        // Post-processing data
        modelObj["postProcessingData"] = model.postProcessingData;
        
        modelsArray.append(modelObj);
    }
    
    result["models"] = modelsArray;
    result["totalModels"] = models.size();
    
    // Calculate summary statistics
    int validModels = 0;
    int convergedModels = 0;
    for (const auto& model : models) {
        if (model.hasValidStats) validModels++;
        if (model.status.contains("Conv")) convergedModels++;
    }
    
    result["validModels"] = validModels;
    result["convergedModels"] = convergedModels;
    
    return result;
}

QJsonObject AnalysisManager::evaluateModelFit(QSharedPointer<AbstractModel> model, int trueModelId)
{
    /**
     * @brief Extract comprehensive statistical evaluation of model fit - migrated from CLI
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
    evaluation["converged"] = model->isConverged();
    
    // Calculate relative performance if true model is known
    if (trueModelId >= 0) {
        evaluation["true_model_id"] = trueModelId;
        evaluation["is_correct_model"] = (static_cast<int>(model->Type()) == trueModelId);
    }
    
    // Extract ML features for training using core statistical analysis
    QJsonObject mlFeatures = StatisticTool::ExtractModelMLFeatures(model);
    evaluation["ml_features"] = mlFeatures;
    
    return evaluation;
}

QJsonObject AnalysisManager::runPostFitAnalysis(QSharedPointer<AbstractModel> model, 
                                               const QJsonObject& analysisConfig)
{
    /**
     * @brief Execute post-fit analysis using core statistical analysis capabilities
     * Integrates with JobManager and StatisticTool for comprehensive analysis
     * Claude Generated - Fixed methods array processing
     */
    QJsonObject results;
    
    if (!model) {
        results["error"] = "Invalid model provided";
        return results;
    }
    
    // Convert methods array to flag-based config for performCompleteAnalysis
    QJsonObject convertedConfig = analysisConfig;
    
    if (analysisConfig.contains("methods")) {
        QJsonArray methods = analysisConfig["methods"].toArray();
        
        for (const QJsonValue& methodValue : methods) {
            QJsonObject method = methodValue.toObject();
            int methodId = method["Method"].toInt();
            
            switch (methodId) {
                case 1: // Monte Carlo
                    convertedConfig["monteCarlo"] = true;
                    if (method.contains("MaxSteps")) {
                        convertedConfig["mcIterations"] = method["MaxSteps"];
                    }
                    if (method.contains("VarianceSource")) {
                        convertedConfig["mcVarianceSource"] = method["VarianceSource"];
                    }
                    break;
                    
                case 4: // Cross Validation  
                    convertedConfig["crossValidation"] = true;
                    if (method.contains("CVType")) {
                        convertedConfig["cvType"] = method["CVType"];
                    }
                    if (method.contains("MaxSteps")) {
                        convertedConfig["cvX"] = method["MaxSteps"];
                    }
                    break;
                    
                case 3: // Model Comparison
                    convertedConfig["modelComparison"] = true;
                    break;
                    
                case 5: // Reduction Analysis
                    convertedConfig["reductionAnalysis"] = true;
                    break;
                    
                default:
                    qDebug() << "🔍 DEBUG: Unknown post-fit analysis method:" << methodId;
                    break;
            }
        }
    }
    
    // Use the existing performCompleteAnalysis functionality
    // This integrates with StatisticTool's JSON-based analysis functions
    QJsonObject analysisResults = performCompleteAnalysis(model, convertedConfig);
    
    // Count enabled analysis methods
    int methodCount = 0;
    if (analysisResults.contains("monteCarlo")) methodCount++;
    if (analysisResults.contains("crossValidation")) methodCount++;
    if (analysisResults.contains("aicComparison")) methodCount++;
    if (analysisResults.contains("modelComparison")) methodCount++;
    if (analysisResults.contains("reductionAnalysis")) methodCount++;
    
    results["methods"] = analysisResults;
    results["method_count"] = methodCount;
    results["enabled"] = methodCount > 0;
    results["success"] = analysisResults["success"];
    
    return results;
}

// Claude Generated