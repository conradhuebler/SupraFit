/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "mlfeatureextractor.h"

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QUuid>

// STL includes for statistical calculations - Claude Generated
#include <algorithm>
#include <numeric>
#include <cmath>

// Qt includes for RegEx parsing - Claude Generated
#include <QtCore/QRegularExpression>

MLFeatureExtractor::MLFeatureExtractor(QObject* parent)
    : QObject(parent)
    , m_includeAdvancedStats(true)
    , m_includeFitParameters(false)
    , m_includeInputNoise(true)
    , m_version("neural_net_v1.0")
{
}

MLFeatureExtractor::~MLFeatureExtractor()
{
}

void MLFeatureExtractor::setExtractionOptions(bool includeAdvancedStats, bool includeFitParameters, bool includeInputNoise)
{
    m_includeAdvancedStats = includeAdvancedStats;
    m_includeFitParameters = includeFitParameters;
    m_includeInputNoise = includeInputNoise;
}

QJsonObject MLFeatureExtractor::parseMLPipelineData(const QString& filename)
{
    QJsonObject root;
    
    // Handle .suprafit files using JsonHandler - Claude Generated
    if (filename.endsWith(".suprafit", Qt::CaseInsensitive)) {
        root = JsonHandler::LoadFile(filename);
        if (root.isEmpty()) {
            qWarning() << "MLFeatureExtractor: Cannot load .suprafit file" << filename;
            return QJsonObject();
        }
    } else {
        // Handle JSON files directly
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "MLFeatureExtractor: Cannot open file" << filename;
            return QJsonObject();
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull()) {
            qWarning() << "MLFeatureExtractor: Invalid JSON in file" << filename;
            return QJsonObject();
        }

        root = doc.object();
    }
    QJsonObject mlData;
    
    // Enhanced parsing for all SupraFit structure types - Claude Generated
    // Based on SUPRAFIT_JSON_FORMAT.md documentation
    
    qDebug() << "MLFeatureExtractor: Parsing file structure, root keys:" << root.keys();
    qDebug() << "MLFeatureExtractor: Has model_0?" << root.contains("model_0");
    qDebug() << "MLFeatureExtractor: Has data.raw?" << (root.contains("data") && root["data"].toObject().contains("raw"));
    
    // PRIORITY: TYPE C first (ML-Pipeline hybrid), then TYPE A (Standard SupraFit)
    // TYPE C: ML-Pipeline structure (hybrid - ml_pipeline + root models)
    if (root.contains("data") && root["data"].toObject().contains("raw")) {
        qDebug() << "MLFeatureExtractor: Using TYPE C parsing (ML-Pipeline hybrid)";
    
        QJsonObject rawData = root["data"].toObject()["raw"].toObject();
        if (rawData.contains("ml_pipeline")) {
            mlData = rawData["ml_pipeline"].toObject();
            
            // CRITICAL: Merge root-level models with ML pipeline data
            // The post-processing data is in root.model_X, not in ml_pipeline block!
            if (root.contains("model_0")) {
                qDebug() << "MLFeatureExtractor: Merging root models with ML pipeline data";
                QJsonArray fittedModels = mlData["fitted_models"].toArray();
                qDebug() << "MLFeatureExtractor: Original fitted_models size:" << fittedModels.size();
                
                // Enhance each fitted model with post-processing data from root
                for (int i = 0; i < fittedModels.size(); ++i) {
                    QString modelKey = QString("model_%1").arg(i);
                    if (root.contains(modelKey)) {
                        QJsonObject fittedModel = fittedModels[i].toObject();
                        QJsonObject rootModel = root[modelKey].toObject();
                        qDebug() << "MLFeatureExtractor: Root model" << modelKey << "keys:" << rootModel.keys();
                        
                        // Merge post_fit_analysis from root model (data.methods structure)
                        if (rootModel.contains("data") && rootModel["data"].toObject().contains("methods")) {
                            qDebug() << "MLFeatureExtractor: Found methods in" << modelKey << ", merging...";
                            QJsonObject rootMethods = rootModel["data"].toObject()["methods"].toObject();
                            qDebug() << "MLFeatureExtractor: Root methods keys:" << rootMethods.keys();
                            
                            // Check if root methods have actual data (not just empty config)
                            for (auto methodIt = rootMethods.begin(); methodIt != rootMethods.end(); ++methodIt) {
                                QString methodId = methodIt.key();
                                QJsonObject methodData = methodIt.value().toObject();
                                qDebug() << "MLFeatureExtractor: Method" << methodId << "keys:" << methodData.keys();
                                
                                // Check for parameter data (0, 1, 2, etc.)
                                for (auto paramIt = methodData.begin(); paramIt != methodData.end(); ++paramIt) {
                                    if (paramIt.key().toInt() >= 0 || paramIt.key() == "0") {
                                        qDebug() << "MLFeatureExtractor: Found parameter data" << paramIt.key() << "in method" << methodId;
                                        QJsonObject paramData = paramIt.value().toObject();
                                        qDebug() << "MLFeatureExtractor: Parameter data keys:" << paramData.keys();
                                        break; // Found real data
                                    }
                                }
                            }
                            
                            QJsonObject postFitAnalysis;
                            postFitAnalysis["analysis_completed"] = true;
                            postFitAnalysis["methods"] = rootMethods;
                            fittedModel["post_fit_analysis"] = postFitAnalysis;
                            qDebug() << "MLFeatureExtractor: Added post_fit_analysis to model" << i;
                        } else {
                            qDebug() << "MLFeatureExtractor: No methods found in" << modelKey;
                        }
                        
                        fittedModels[i] = fittedModel;
                    } else {
                        qDebug() << "MLFeatureExtractor: Root model" << modelKey << "not found";
                    }
                }
                
                mlData["fitted_models"] = fittedModels;
                qDebug() << "MLFeatureExtractor: Merge complete, updated fitted_models";
            } else {
                qDebug() << "MLFeatureExtractor: No model_0 found in root for merging";
            }
            
            // Preserve original content string for measurement noise parsing - Claude Generated
            if (root.contains("data") && root["data"].toObject().contains("content")) {
                QString originalContent = root["data"].toObject()["content"].toString();
                mlData["original_content"] = originalContent;
                qDebug() << "MLFeatureExtractor: Preserved original content string (length:" << originalContent.length() << ")";
            }
        }
    }
    
    // TYPE A: Standard SupraFit Project (.suprafit, models) - ONLY if no ML pipeline
    else if (root.contains("model_0")) {
        qDebug() << "MLFeatureExtractor: Using TYPE A parsing (Standard SupraFit)";
    
        QJsonObject syntheticML;
        
        // Extract ground truth from data.content if available
        if (root.contains("data") && root["data"].toObject().contains("content")) {
            QJsonObject generationConfig;
            
            // Parse noise configuration from content string
            QString content = root["data"].toObject()["content"].toString();
            QJsonObject inputNoise = parseNoiseFromContent(content);
            if (!inputNoise.isEmpty()) {
                generationConfig["input_json"] = inputNoise;
            }
            
            // Try to extract ground truth from content metadata
            generationConfig["source"] = "content_metadata";
            syntheticML["generation_config"] = generationConfig;
        }
        
        // Convert models to fitted_models array
        QJsonArray fittedModels;
        for (auto it = root.begin(); it != root.end(); ++it) {
            if (it.key().startsWith("model_")) {
                QJsonObject model = it.value().toObject();
                model["source"] = "standard_suprafit";
                fittedModels.append(model);
            }
        }
        syntheticML["fitted_models"] = fittedModels;
        mlData = syntheticML;
    }
    
    // TYPE B: Direct Analysis (vonHand_mc.json style)
    else if (root.contains("methods")) {
        QJsonObject syntheticML;
        
        // Create fitted model from direct methods structure
        QJsonArray fittedModels;
        QJsonObject model;
        
        // Extract basic model info from data if available
        if (root.contains("data")) {
            model["source"] = "direct_analysis";
            // Add post_fit_analysis with direct methods
            QJsonObject postFitAnalysis;
            postFitAnalysis["analysis_completed"] = true;
            postFitAnalysis["methods"] = root["methods"];
            model["post_fit_analysis"] = postFitAnalysis;
        }
        
        fittedModels.append(model);
        syntheticML["fitted_models"] = fittedModels;
        mlData = syntheticML;
    }
    
    // Direct ml_pipeline structure
    else if (root.contains("ml_pipeline")) {
        mlData = root["ml_pipeline"].toObject();
    }
    
    // Raw data directly
    else if (root.contains("raw") && root["raw"].toObject().contains("ml_pipeline")) {
        mlData = root["raw"].toObject()["ml_pipeline"].toObject();
    }

    if (mlData.isEmpty()) {
        qWarning() << "MLFeatureExtractor: No ML pipeline data found in" << filename;
        qDebug() << "MLFeatureExtractor: Available root keys:" << root.keys();
        
        // Debug .suprafit structure - Claude Generated
        if (filename.endsWith(".suprafit", Qt::CaseInsensitive)) {
            if (root.contains("model_0")) {
                QJsonObject model0 = root["model_0"].toObject();
                qDebug() << "MLFeatureExtractor: model_0 keys:" << model0.keys();
                if (model0.contains("post_fit_analysis")) {
                    QJsonObject postFit = model0["post_fit_analysis"].toObject();
                    qDebug() << "MLFeatureExtractor: post_fit_analysis keys:" << postFit.keys();
                    if (postFit.contains("methods")) {
                        QJsonObject methods = postFit["methods"].toObject();
                        qDebug() << "MLFeatureExtractor: methods keys:" << methods.keys();
                    }
                }
            }
        }
    }

    return mlData;
}

QJsonObject MLFeatureExtractor::extractCompactTrainingSample(const QJsonObject& mlRawData)
{
    if (mlRawData.isEmpty()) {
        qWarning() << "MLFeatureExtractor: Empty ML raw data provided";
        return QJsonObject();
    }
    
    // Debug: Show what we have in mlRawData - Claude Generated
    qDebug() << "MLFeatureExtractor: Processing mlRawData with keys:" << mlRawData.keys();
    if (mlRawData.contains("fitted_models")) {
        QJsonArray fittedModels = mlRawData["fitted_models"].toArray();
        qDebug() << "MLFeatureExtractor: Found" << fittedModels.size() << "fitted models";
        for (int i = 0; i < fittedModels.size(); ++i) {
            QJsonObject model = fittedModels[i].toObject();
            qDebug() << "MLFeatureExtractor: Model" << i << "keys:" << model.keys();
            if (model.contains("post_fit_analysis")) {
                QJsonObject pfa = model["post_fit_analysis"].toObject();
                qDebug() << "MLFeatureExtractor: Model" << i << "post_fit_analysis keys:" << pfa.keys();
                if (pfa.contains("methods")) {
                    QJsonObject methods = pfa["methods"].toObject();
                    qDebug() << "MLFeatureExtractor: Model" << i << "methods keys:" << methods.keys();
                }
            }
        }
    }

    QJsonObject trainingSample;
    
    // Generate unique sample ID
    trainingSample["sample_id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Extract ground-truth information
    if (mlRawData.contains("generation_config")) {
        QJsonObject generationConfig = mlRawData["generation_config"].toObject();
        
        // Try to get data.content string for noise parsing fallback
        // For TYPE C files (ML-Pipeline hybrid), content is in original file structure
        QString contentString;
        if (mlRawData.contains("original_content")) {
            contentString = mlRawData["original_content"].toString();
        }
        
        trainingSample["ground_truth"] = extractGroundTruthWithContent(generationConfig, contentString);
    }
    
    // Extract candidate models with statistical features
    if (mlRawData.contains("fitted_models")) {
        QJsonArray fittedModels = mlRawData["fitted_models"].toArray();
        trainingSample["candidate_models"] = extractCandidateModels(fittedModels);
        
        // Determine correct label based on ground-truth
        if (trainingSample.contains("ground_truth")) {
            trainingSample["label"] = determineCorrectLabel(
                trainingSample["ground_truth"].toObject(),
                trainingSample["candidate_models"].toArray()
            );
        }
    }
    
    return trainingSample;
}

QVector<QJsonObject> MLFeatureExtractor::extractBatchTrainingData(const QVector<QString>& filenames)
{
    QVector<QJsonObject> trainingSamples;
    
    for (const QString& filename : filenames) {
        QJsonObject mlData = parseMLPipelineData(filename);
        if (!mlData.isEmpty()) {
            QJsonObject sample = extractCompactTrainingSample(mlData);
            if (!sample.isEmpty()) {
                trainingSamples.append(sample);
            }
        }
    }
    
    qDebug() << "MLFeatureExtractor: Extracted" << trainingSamples.size() << "training samples from" << filenames.size() << "files";
    return trainingSamples;
}

QJsonObject MLFeatureExtractor::exportNeuralNetFormat(const QVector<QJsonObject>& trainingSamples)
{
    QJsonObject result;
    
    // Metadata
    QJsonObject metadata;
    metadata["version"] = m_version;
    metadata["generated"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    metadata["sample_count"] = trainingSamples.size();
    
    // Feature configuration
    QJsonArray features;
    features.append("fit_quality_metrics");
    if (m_includeAdvancedStats) {
        features.append("statistical_analysis");
    }
    if (m_includeFitParameters) {
        features.append("fitted_parameters");
    }
    if (m_includeInputNoise) {
        features.append("input_noise");
    }
    metadata["features"] = features;
    
    result["ml_training_metadata"] = metadata;
    
    // Training samples
    QJsonArray samples;
    for (const QJsonObject& sample : trainingSamples) {
        samples.append(sample);
    }
    result["training_samples"] = samples;
    
    return result;
}

QJsonArray MLFeatureExtractor::extractCandidateModels(const QJsonArray& fittedModels)
{
    QJsonArray candidates;
    
    for (const QJsonValue& modelValue : fittedModels) {
        QJsonObject model = modelValue.toObject();
        QJsonObject candidate;
        
        // Model identification
        if (model.contains("model_id")) {
            candidate["model_id"] = model["model_id"];
        }
        if (model.contains("model_name")) {
            candidate["model_name"] = model["model_name"];
        }
        
        // Fit quality metrics (always included)
        if (model.contains("fit_quality")) {
            candidate["fit_quality"] = model["fit_quality"];
        }
        
        // SSE and convergence (essential metrics)
        if (model.contains("sse")) {
            candidate["sse"] = model["sse"];
        }
        if (model.contains("convergence")) {
            candidate["convergence"] = model["convergence"];
        }
        
        // Advanced statistical features from post_fit_analysis - Claude Generated
        if (m_includeAdvancedStats && model.contains("post_fit_analysis")) {
            QJsonObject postFitAnalysis = model["post_fit_analysis"].toObject();
            candidate["statistical_features"] = extractStatisticalFeatures(postFitAnalysis);
        }
        
        // Fitted parameters (optional, can be large)
        if (m_includeFitParameters && model.contains("fitted_parameters")) {
            candidate["fitted_parameters"] = model["fitted_parameters"];
        }
        
        // Timestamp for tracking
        if (model.contains("timestamp")) {
            candidate["timestamp"] = model["timestamp"];
        }
        
        candidates.append(candidate);
    }
    
    return candidates;
}

QJsonObject MLFeatureExtractor::determineCorrectLabel(const QJsonObject& groundTruth, const QJsonArray& candidates)
{
    QJsonObject label;
    
    if (!groundTruth.contains("model_id")) {
        qWarning() << "MLFeatureExtractor: No ground-truth model ID found for labeling";
        return label;
    }
    
    int groundTruthId = groundTruth["model_id"].toInt();
    QString groundTruthName = groundTruth["model_name"].toString();
    
    // Find matching candidate model
    for (const QJsonValue& candidateValue : candidates) {
        QJsonObject candidate = candidateValue.toObject();
        if (candidate.contains("model_id") && candidate["model_id"].toInt() == groundTruthId) {
            label["correct_model_id"] = groundTruthId;
            if (candidate.contains("model_name")) {
                label["correct_model_name"] = candidate["model_name"].toString();
            } else {
                label["correct_model_name"] = groundTruthName;
            }
            break;
        }
    }
    
    if (label.isEmpty()) {
        qWarning() << "MLFeatureExtractor: No matching candidate found for ground-truth model ID" << groundTruthId;
    }
    
    return label;
}

QJsonObject MLFeatureExtractor::extractGroundTruth(const QJsonObject& generationConfig)
{
    QJsonObject groundTruth;
    
    // Enhanced ground-truth extraction from generation_config - Claude Generated
    if (generationConfig.contains("ground_truth_model")) {
        QJsonObject groundTruthModel = generationConfig["ground_truth_model"].toObject();
        
        // Extract all available ground-truth parameters
        if (groundTruthModel.contains("id")) {
            groundTruth["model_id"] = groundTruthModel["id"];
        }
        if (groundTruthModel.contains("name")) {
            groundTruth["model_name"] = groundTruthModel["name"];
        }
        if (groundTruthModel.contains("datapoints")) {
            groundTruth["datapoints"] = groundTruthModel["datapoints"];
        }
        if (groundTruthModel.contains("series_count")) {
            groundTruth["series_count"] = groundTruthModel["series_count"];
        }
        if (groundTruthModel.contains("global_parameters")) {
            groundTruth["global_params"] = groundTruthModel["global_parameters"];
        }
        if (groundTruthModel.contains("local_parameters")) {
            groundTruth["local_params"] = groundTruthModel["local_parameters"];
        }
        
        qDebug() << "MLFeatureExtractor: Extracted enhanced ground-truth from generation_config";
    }
    
    // Fallback: try legacy structure or basic extraction
    if (groundTruth.isEmpty()) {
        // Try to find any model identification
        if (generationConfig.contains("model_id")) {
            groundTruth["model_id"] = generationConfig["model_id"];
        }
        if (generationConfig.contains("model_name")) {
            groundTruth["model_name"] = generationConfig["model_name"];
        }
        
        qDebug() << "MLFeatureExtractor: Used fallback ground-truth extraction";
    }
    
    // Extract input noise information
    if (m_includeInputNoise) {
        QJsonObject inputNoise = extractInputNoise(generationConfig);
        if (!inputNoise.isEmpty()) {
            groundTruth["input_noise"] = inputNoise;
        }
    }
    
    return groundTruth;
}

QJsonObject MLFeatureExtractor::extractGroundTruthWithContent(const QJsonObject& generationConfig, const QString& contentString)
{
    // qDebug() << "MLFeatureExtractor: extractGroundTruthWithContent called, contentString length:" << contentString.length();
    
    // Start with standard ground-truth extraction - Claude Generated
    QJsonObject groundTruth = extractGroundTruth(generationConfig);
    
    // Enhanced input noise extraction with content string fallback
    if (m_includeInputNoise) {
        QJsonObject inputNoise = extractInputNoise(generationConfig);
        
        // qDebug() << "MLFeatureExtractor: Current input_noise keys:" << inputNoise.keys();
        // qDebug() << "MLFeatureExtractor: Content string empty?" << contentString.isEmpty();
        // qDebug() << "MLFeatureExtractor: Input noise contains std?" << inputNoise.contains("std");
        
        // Parse measurement noise from content string if not already found
        if (!contentString.isEmpty() && (inputNoise.isEmpty() || !inputNoise.contains("std"))) {
            // qDebug() << "MLFeatureExtractor: Triggering content string parsing for measurement noise";
            QJsonObject contentNoise = parseNoiseFromContent(contentString);
            if (!contentNoise.isEmpty()) {
                // qDebug() << "MLFeatureExtractor: Found measurement noise in content string";
                
                // Merge with existing input_noise, prioritizing content string for measurement noise
                for (auto it = contentNoise.begin(); it != contentNoise.end(); ++it) {
                    inputNoise[it.key()] = it.value();
                }
                inputNoise["source"] = inputNoise.contains("source") ? 
                    inputNoise["source"].toString() + "_and_content_string" : 
                    "content_string";
            }
        }
        
        if (!inputNoise.isEmpty()) {
            groundTruth["input_noise"] = inputNoise;
        }
    }
    
    return groundTruth;
}

QJsonObject MLFeatureExtractor::extractInputNoise(const QJsonObject& generationConfig)
{
    QJsonObject inputNoise;
    
    // Check for applied noise information
    if (generationConfig.contains("noise_applied")) {
        inputNoise = generationConfig["noise_applied"].toObject();
    }
    
    // Extract noise configuration from input_json - Claude Generated
    if (generationConfig.contains("input_json")) {
        QJsonObject inputJson = generationConfig["input_json"].toObject();
        
        // Look for various noise configuration keys (case-sensitive)
        QStringList noiseKeys = {"Noise", "noise", "dependent_noise", "DependentNoise"};
        
        for (const QString& key : noiseKeys) {
            if (inputJson.contains(key)) {
                QJsonObject noiseConfig = inputJson[key].toObject();
                if (!noiseConfig.isEmpty()) {
                    // Store complete noise configuration with proper structure
                    inputNoise["type"] = noiseConfig.contains("Type") ? noiseConfig["Type"].toString() : "unknown";
                    inputNoise["std"] = noiseConfig.contains("Std") ? noiseConfig["Std"].toArray() : QJsonArray();
                    inputNoise["random_seed"] = noiseConfig.contains("RandomSeed") ? noiseConfig["RandomSeed"].toInt() : -1;
                    
                    // Store original configuration for reference
                    inputNoise["original_config"] = noiseConfig;
                    break; // Use first found configuration
                }
            }
        }
        
        // Extract GlobalRandomLimits and LocalRandomLimits (Matrix-String format) - Claude Generated
        // These are PARAMETER LIMITS, not measurement noise - add as separate section
        if (inputJson.contains("GlobalRandomLimits") || inputJson.contains("LocalRandomLimits")) {
            QJsonObject randomLimits = parseMatrixStringLimits(inputJson);
            if (!randomLimits.isEmpty()) {
                // Don't overwrite existing noise config - merge instead
                for (auto it = randomLimits.begin(); it != randomLimits.end(); ++it) {
                    inputNoise[it.key()] = it.value();
                }
                inputNoise["source"] = inputNoise.contains("source") ? 
                    inputNoise["source"].toString() + "_and_matrix_string_limits" : 
                    "matrix_string_limits";
                qDebug() << "MLFeatureExtractor: Merged matrix string limits with existing input_noise";
            }
        }
        
        // Also check for nested Dependent.Noise structure
        if (inputNoise.isEmpty() && inputJson.contains("Dependent") && inputJson["Dependent"].isObject()) {
            QJsonObject dependent = inputJson["Dependent"].toObject();
            if (dependent.contains("Noise") && dependent["Noise"].isObject()) {
                QJsonObject depNoise = dependent["Noise"].toObject();
                if (!depNoise.isEmpty()) {
                    inputNoise["type"] = depNoise.contains("Type") ? depNoise["Type"].toString() : "unknown";
                    inputNoise["std"] = depNoise.contains("Std") ? depNoise["Std"].toArray() : QJsonArray();
                    inputNoise["random_seed"] = depNoise.contains("RandomSeed") ? depNoise["RandomSeed"].toInt() : -1;
                    inputNoise["original_config"] = depNoise;
                }
            }
        }
    }
    
    // Fallback: Try to parse noise from data.content string if no noise config found
    if (!inputNoise.contains("type") && !inputNoise.contains("std")) {
        // This means we only have parameter limits but no measurement noise info
        // Try to get the content string from the ML pipeline data structure
        qDebug() << "MLFeatureExtractor: No measurement noise found, trying content string fallback";
        
        // NOTE: This would require access to the root data.content, which we don't have here
        // The caller should pass this information or we need to restructure the call
        inputNoise["measurement_noise_status"] = "not_found_in_generation_config";
    }
    
    return inputNoise;
}

QJsonObject MLFeatureExtractor::extractStatisticalFeatures(const QJsonObject& postFitAnalysis)
{
    QJsonObject features;
    
    if (!postFitAnalysis.contains("methods")) {
        return features;
    }
    
    QJsonObject methods = postFitAnalysis["methods"].toObject();
    
    // Extract results from each statistical method using SupraFit core functions - Claude Generated
    for (auto it = methods.begin(); it != methods.end(); ++it) {
        QString methodId = it.key();
        QJsonObject methodData = it.value().toObject();
        
        // Method ID based extraction (works with merged data structure) - Claude Generated
        qDebug() << "MLFeatureExtractor: Processing method" << methodId << "keys:" << methodData.keys();
        QJsonObject methodFeatures;
            
        // Extract features based on method ID (from SupraFit method enums)
        if (methodId == "1") {
            // Method ID 1 = Monte Carlo Simulation
            methodFeatures = extractMonteCarloFeatures(methodData);
            if (!methodFeatures.isEmpty()) {
                features["monte_carlo"] = methodFeatures;
            }
        }
        else if (methodId == "2") {
            // Method ID 2 = Cross Validation
            methodFeatures = extractCrossValidationFeatures(methodData);
            if (!methodFeatures.isEmpty()) {
                features["cross_validation"] = methodFeatures;
            }
        }
        else if (methodId == "3") {
            // Method ID 3 = Model Comparison
            methodFeatures = extractModelComparisonFeatures(methodData);
            if (!methodFeatures.isEmpty()) {
                features["model_comparison"] = methodFeatures;
            }
        }
        else if (methodId == "4") {
            // Method ID 4 = Weakened Grid Search
            methodFeatures = extractGridSearchFeatures(methodData);
            if (!methodFeatures.isEmpty()) {
                features["grid_search"] = methodFeatures;
            }
        }
        else if (methodId == "5") {
            // Method ID 5 = Fast Confidence
            methodFeatures = extractFastConfidenceFeatures(methodData);
            if (!methodFeatures.isEmpty()) {
                features["fast_confidence"] = methodFeatures;
            }
        }
        else if (methodId == "6") {
            // Method ID 6 = Reduction Analysis
            methodFeatures = extractReductionFeatures(methodData);
            if (!methodFeatures.isEmpty()) {
                features["reduction_analysis"] = methodFeatures;
            }
        }
        else if (methodId == "7") {
            // Method ID 7 = Global Search
            methodFeatures = extractGlobalSearchFeatures(methodData);
            if (!methodFeatures.isEmpty()) {
                features["global_search"] = methodFeatures;
            }
        }
        else {
            // Unknown method: extract basic features
            methodFeatures = extractBasicMethodFeatures(methodData);
            if (!methodFeatures.isEmpty()) {
                features[QString("method_%1").arg(methodId)] = methodFeatures;
            }
        }
    }
    
    return features;
}

QJsonObject MLFeatureExtractor::extractMonteCarloFeatures(const QJsonObject& methodData)
{
    QJsonObject features;
    
    // Extract Monte Carlo results using correct SupraFit structure - Claude Generated
    // Based on SUPRAFIT_JSON_FORMAT.md: methods.1.PARAM_INDEX or methods.1.results.PARAM_INDEX
    
    QJsonObject resultsData;
    
    // Check for direct parameter structure (Type B: vonHand_mc.json)
    if (methodData.contains("0")) {
        resultsData = methodData;
    }
    // Check for results sub-structure (Type A: .suprafit)
    else if (methodData.contains("results")) {
        resultsData = methodData["results"].toObject();
    }
    
    if (resultsData.isEmpty()) {
        qDebug() << "MLFeatureExtractor: No Monte Carlo results found in method data";
        return features;
    }
    
    // Extract parameters (0, 1, 2, ...)
    for (auto paramIt = resultsData.begin(); paramIt != resultsData.end(); ++paramIt) {
        QString paramIndex = paramIt.key();
        QJsonObject paramData = paramIt.value().toObject();
        
        if (paramData.isEmpty()) continue;
        
        QString paramName = paramData.contains("name") ? paramData["name"].toString() : QString("param_%1").arg(paramIndex);
        QString paramType = paramData.contains("type") ? paramData["type"].toString() : "Unknown";
        
        QJsonObject paramFeatures;
        
        // Extract boxplot statistics (direct from SupraFit structure)
        if (paramData.contains("boxplot")) {
            QJsonObject boxplot = paramData["boxplot"].toObject();
            paramFeatures["boxplot_count"] = boxplot["count"];
            paramFeatures["boxplot_mean"] = boxplot["mean"];
            paramFeatures["boxplot_median"] = boxplot["median"];
            paramFeatures["boxplot_stddev"] = boxplot["stddev"];
            paramFeatures["boxplot_lower_quartile"] = boxplot["lower_quantile"];
            paramFeatures["boxplot_upper_quartile"] = boxplot["upper_quantile"];
            paramFeatures["boxplot_lower_whisker"] = boxplot["lower_whisker"];
            paramFeatures["boxplot_upper_whisker"] = boxplot["upper_whisker"];
        }
        
        // Extract confidence intervals (direct from SupraFit structure)
        if (paramData.contains("confidence")) {
            QJsonObject confidence = paramData["confidence"].toObject();
            paramFeatures["confidence_level"] = confidence["error"];
            paramFeatures["confidence_lower"] = confidence["lower"];
            paramFeatures["confidence_upper"] = confidence["upper"];
            
            // Calculate confidence interval width
            if (confidence.contains("lower") && confidence.contains("upper")) {
                double width = confidence["upper"].toDouble() - confidence["lower"].toDouble();
                paramFeatures["confidence_width"] = width;
            }
        }
        
        // Extract raw Monte Carlo data using ToolSet functions
        if (paramData.contains("data") && paramData["data"].toObject().contains("raw")) {
            QString rawDataString = paramData["data"].toObject()["raw"].toString();
            QVector<double> rawValues = ToolSet::String2DoubleVec(rawDataString);
            
            if (!rawValues.isEmpty()) {
                paramFeatures["raw_sample_count"] = rawValues.size();
                
                // Calculate additional statistics from raw data
                double sum = std::accumulate(rawValues.begin(), rawValues.end(), 0.0);
                double mean = sum / rawValues.size();
                double sq_sum = std::inner_product(rawValues.begin(), rawValues.end(), rawValues.begin(), 0.0);
                double variance = (sq_sum / rawValues.size()) - (mean * mean);
                
                paramFeatures["raw_mean"] = mean;
                paramFeatures["raw_variance"] = variance;
                paramFeatures["raw_std"] = std::sqrt(variance);
                
                // Percentiles from sorted data
                std::vector<double> sorted(rawValues.begin(), rawValues.end());
                std::sort(sorted.begin(), sorted.end());
                int n = sorted.size();
                
                if (n >= 4) {
                    paramFeatures["raw_p5"] = sorted[int(n * 0.05)];
                    paramFeatures["raw_p95"] = sorted[int(n * 0.95)];
                    paramFeatures["raw_median"] = sorted[n/2];
                    paramFeatures["raw_iqr"] = sorted[int(n * 0.75)] - sorted[int(n * 0.25)];
                }
            }
        }
        
        // Parameter metadata
        paramFeatures["param_name"] = paramName;
        paramFeatures["param_type"] = paramType;
        paramFeatures["param_index"] = paramIndex;
        if (paramData.contains("value")) {
            paramFeatures["optimal_value"] = paramData["value"];
        }
        
        // Store parameter features
        features[QString("param_%1_%2").arg(paramIndex, paramName.toLower().replace(" ", "_"))] = paramFeatures;
    }
    
    return features;
}

QJsonObject MLFeatureExtractor::extractCrossValidationFeatures(const QJsonObject& methodData)
{
    QJsonObject features;
    
    // Extract CV scores and fold statistics - Claude Generated
    if (methodData.contains("cv_results")) {
        QJsonObject cvResults = methodData["cv_results"].toObject();
        
        // Extract fold-wise statistics
        if (cvResults.contains("fold_scores")) {
            QString scoresString = cvResults["fold_scores"].toString();
            QVector<double> scores = ToolSet::String2DoubleVec(scoresString);
            
            if (!scores.isEmpty()) {
                // Calculate mean and std deviation
                double sum = std::accumulate(scores.begin(), scores.end(), 0.0);
                double mean = sum / scores.size();
                double sq_sum = std::inner_product(scores.begin(), scores.end(), scores.begin(), 0.0);
                double stdev = std::sqrt(sq_sum / scores.size() - mean * mean);
                
                features["cv_mean_score"] = mean;
                features["cv_std_score"] = stdev;
                features["cv_fold_count"] = scores.size();
                features["cv_score_range"] = *std::max_element(scores.begin(), scores.end()) - 
                                           *std::min_element(scores.begin(), scores.end());
            }
        }
        
        // Extract prediction variance
        if (cvResults.contains("prediction_variance")) {
            features["prediction_variance"] = cvResults["prediction_variance"];
        }
    }
    
    // Extract residual analysis
    if (methodData.contains("residual_analysis")) {
        features["residual_analysis"] = methodData["residual_analysis"];
    }
    
    return features;
}

QJsonObject MLFeatureExtractor::extractModelComparisonFeatures(const QJsonObject& methodData)
{
    QJsonObject features;
    
    // Extract AIC, BIC, and likelihood ratios - Claude Generated
    if (methodData.contains("information_criteria")) {
        features["information_criteria"] = methodData["information_criteria"];
    }
    
    if (methodData.contains("model_probabilities")) {
        features["model_probabilities"] = methodData["model_probabilities"];
    }
    
    // Extract likelihood ratio tests
    if (methodData.contains("likelihood_tests")) {
        features["likelihood_tests"] = methodData["likelihood_tests"];
    }
    
    return features;
}

QJsonObject MLFeatureExtractor::extractReductionFeatures(const QJsonObject& methodData)
{
    QJsonObject features;
    
    // Extract parameter convergence and stability metrics - Claude Generated
    if (methodData.contains("convergence_analysis")) {
        QJsonObject convergence = methodData["convergence_analysis"].toObject();
        
        // Extract convergence statistics
        for (auto it = convergence.begin(); it != convergence.end(); ++it) {
            QString paramName = it.key();
            QJsonObject paramConv = it.value().toObject();
            
            if (paramConv.contains("stability_metric")) {
                features[paramName + "_stability"] = paramConv["stability_metric"];
            }
            if (paramConv.contains("convergence_rate")) {
                features[paramName + "_convergence"] = paramConv["convergence_rate"];
            }
        }
    }
    
    // Extract reduction ratios and thresholds
    if (methodData.contains("reduction_ratios")) {
        features["reduction_ratios"] = methodData["reduction_ratios"];
    }
    
    return features;
}

QJsonObject MLFeatureExtractor::extractGridSearchFeatures(const QJsonObject& methodData)
{
    QJsonObject features;
    
    // Extract search space exploration metrics - Claude Generated
    if (methodData.contains("search_metrics")) {
        features["search_metrics"] = methodData["search_metrics"];
    }
    
    if (methodData.contains("parameter_exploration")) {
        features["parameter_exploration"] = methodData["parameter_exploration"];
    }
    
    // Extract optimization landscape information
    if (methodData.contains("optimization_surface")) {
        features["optimization_surface"] = methodData["optimization_surface"];
    }
    
    return features;
}

QJsonObject MLFeatureExtractor::extractFastConfidenceFeatures(const QJsonObject& methodData)
{
    QJsonObject features;
    
    // Extract bootstrap confidence intervals - Claude Generated
    if (methodData.contains("bootstrap_results")) {
        QJsonObject bootstrap = methodData["bootstrap_results"].toObject();
        
        // Extract bootstrap statistics using ToolSet functions
        for (auto it = bootstrap.begin(); it != bootstrap.end(); ++it) {
            QString paramName = it.key();
            QJsonObject paramBootstrap = it.value().toObject();
            
            if (paramBootstrap.contains("bootstrap_samples")) {
                QString samplesString = paramBootstrap["bootstrap_samples"].toString();
                QVector<double> samples = ToolSet::String2DoubleVec(samplesString);
                
                if (!samples.isEmpty()) {
                    // Calculate bootstrap statistics
                    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
                    double mean = sum / samples.size();
                    double sq_sum = std::inner_product(samples.begin(), samples.end(), samples.begin(), 0.0);
                    double stdev = std::sqrt(sq_sum / samples.size() - mean * mean);
                    
                    QJsonObject paramStats;
                    paramStats["bootstrap_mean"] = mean;
                    paramStats["bootstrap_std"] = stdev;
                    paramStats["sample_count"] = samples.size();
                    
                    features[paramName + "_bootstrap"] = paramStats;
                }
            }
        }
    }
    
    return features;
}

QJsonObject MLFeatureExtractor::extractGlobalSearchFeatures(const QJsonObject& methodData)
{
    QJsonObject features;
    
    // Extract global optimization metrics - Claude Generated
    if (methodData.contains("optimization_metrics")) {
        features["optimization_metrics"] = methodData["optimization_metrics"];
    }
    
    if (methodData.contains("search_convergence")) {
        features["search_convergence"] = methodData["search_convergence"];
    }
    
    // Extract multi-start optimization results
    if (methodData.contains("multistart_results")) {
        features["multistart_results"] = methodData["multistart_results"];
    }
    
    return features;
}

QJsonObject MLFeatureExtractor::extractBasicMethodFeatures(const QJsonObject& methodData)
{
    QJsonObject features;
    
    // Extract common statistical metrics available in any method - Claude Generated
    if (methodData.contains("method_name")) {
        features["method_name"] = methodData["method_name"];
    }
    
    if (methodData.contains("execution_time")) {
        features["execution_time"] = methodData["execution_time"];
    }
    
    if (methodData.contains("iterations")) {
        features["iterations"] = methodData["iterations"];
    }
    
    // Extract any numerical results using ToolSet functions
    if (methodData.contains("raw_results")) {
        QString rawResults = methodData["raw_results"].toString();
        QVector<double> values = ToolSet::String2DoubleVec(rawResults);
        
        if (!values.isEmpty()) {
            // Calculate basic statistics
            double sum = std::accumulate(values.begin(), values.end(), 0.0);
            double mean = sum / values.size();
            double sq_sum = std::inner_product(values.begin(), values.end(), values.begin(), 0.0);
            double stdev = std::sqrt(sq_sum / values.size() - mean * mean);
            
            features["results_count"] = values.size();
            features["results_mean"] = mean;
            features["results_std"] = stdev;
        }
    }
    
    return features;
}

QJsonObject MLFeatureExtractor::parseMatrixStringLimits(const QJsonObject& inputJson)
{
    QJsonObject result;
    
    // Parse GlobalRandomLimits: "[2 5]" → {"min": [2.0], "max": [5.0]}
    if (inputJson.contains("GlobalRandomLimits")) {
        QString globalLimits = inputJson["GlobalRandomLimits"].toString();
        QJsonObject globalParsed = parseMatrixString(globalLimits, "global");
        if (!globalParsed.isEmpty()) {
            result["global_random_limits"] = globalParsed;
        }
    }
    
    // Parse LocalRandomLimits: "[6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5]" → min/max arrays
    if (inputJson.contains("LocalRandomLimits")) {
        QString localLimits = inputJson["LocalRandomLimits"].toString();
        QJsonObject localParsed = parseMatrixString(localLimits, "local");
        if (!localParsed.isEmpty()) {
            result["local_random_limits"] = localParsed;
        }
    }
    
    // Add metadata
    if (!result.isEmpty()) {
        result["format"] = "matrix_string";
        if (inputJson.contains("Model")) {
            result["model"] = inputJson["Model"];
        }
        if (inputJson.contains("Series")) {
            result["series"] = inputJson["Series"];
        }
        if (inputJson.contains("Type")) {
            result["type"] = inputJson["Type"];
        }
    }
    
    return result;
}

QJsonObject MLFeatureExtractor::parseMatrixString(const QString& matrixStr, const QString& type)
{
    QJsonObject result;
    
    if (matrixStr.isEmpty()) {
        return result;
    }
    
    // Remove brackets and clean string
    QString cleaned = matrixStr;
    cleaned = cleaned.remove('[').remove(']').trimmed();
    
    if (type == "global") {
        // Global format: "2 5" → min=2, max=5
        QStringList parts = cleaned.split(' ', Qt::SkipEmptyParts);
        if (parts.size() == 2) {
            bool ok1, ok2;
            double min = parts[0].toDouble(&ok1);
            double max = parts[1].toDouble(&ok2);
            
            if (ok1 && ok2) {
                result["min"] = QJsonArray{min};
                result["max"] = QJsonArray{max};
                qDebug() << "MLFeatureExtractor: Parsed global limits:" << min << "to" << max;
            }
        }
    }
    else if (type == "local") {
        // Local format: "6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5" → 4 parameter pairs
        QStringList rows = cleaned.split(';', Qt::SkipEmptyParts);
        QJsonArray minArray, maxArray;
        
        for (const QString& row : rows) {
            QStringList pair = row.trimmed().split(' ', Qt::SkipEmptyParts);
            if (pair.size() == 2) {
                bool ok1, ok2;
                double min = pair[0].toDouble(&ok1);
                double max = pair[1].toDouble(&ok2);
                
                if (ok1 && ok2) {
                    minArray.append(min);
                    maxArray.append(max);
                }
            }
        }
        
        if (!minArray.isEmpty() && !maxArray.isEmpty()) {
            result["min"] = minArray;
            result["max"] = maxArray;
            result["parameter_count"] = minArray.size();
            qDebug() << "MLFeatureExtractor: Parsed local limits for" << minArray.size() << "parameters";
        }
    }
    
    return result;
}

QJsonObject MLFeatureExtractor::parseNoiseFromContent(const QString& content)
{
    QJsonObject result;
    
    // Parse JSON from content string - Claude Generated
    // Format: "...\n\nInput Configuration:\n{JSON_HERE}"
    
    // qDebug() << "MLFeatureExtractor: Parsing noise from content string (length:" << content.length() << ")";
    
    // Find all JSON objects in content, not just the last one
    QRegularExpression jsonRegex(R"(\{[^{}]*(?:\{[^{}]*\}[^{}]*)*\})", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator iterator = jsonRegex.globalMatch(content);
    
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        QString jsonString = match.captured(0);
        
        // qDebug() << "MLFeatureExtractor: Found JSON candidate (length:" << jsonString.length() << ")";
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &parseError);
        
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject parsed = doc.object();
            
            // qDebug() << "MLFeatureExtractor: JSON object keys:" << parsed.keys();
            
            // Extract Dependent.Noise structure - try multiple formats
            if (parsed.contains("Dependent") && parsed["Dependent"].isObject()) {
                QJsonObject dependent = parsed["Dependent"].toObject();
                qDebug() << "MLFeatureExtractor: Found Dependent object, keys:" << dependent.keys();
                
                if (dependent.contains("Noise") && dependent["Noise"].isObject()) {
                    result = dependent["Noise"].toObject();
                    qDebug() << "MLFeatureExtractor: Found Noise object, keys:" << result.keys();
                    break; // Found what we need
                }
            }
            // Format 1: JSON object might BE the Dependent block with Generator, Noise, Source
            else if (parsed.contains("Noise") && parsed.contains("Generator") && parsed.contains("Source")) {
                qDebug() << "MLFeatureExtractor: Found Dependent-like object, checking Noise...";
                qDebug() << "MLFeatureExtractor: Noise type:" << parsed["Noise"].type() << "is object?" << parsed["Noise"].isObject();
                if (parsed["Noise"].isObject()) {
                    result = parsed["Noise"].toObject();
                    qDebug() << "MLFeatureExtractor: Found Noise object in Dependent-like structure, keys:" << result.keys();
                    break; // Found what we need
                } else if (parsed["Noise"].isNull()) {
                    qDebug() << "MLFeatureExtractor: Noise is null, skipping...";
                } else {
                    qDebug() << "MLFeatureExtractor: Noise is not an object, value:" << parsed["Noise"];
                }
            }
            // Format 2: JSON object might BE the Noise object itself (RandomSeed, Std, Type)
            else if (parsed.contains("RandomSeed") && parsed.contains("Std") && parsed.contains("Type")) {
                // qDebug() << "MLFeatureExtractor: Found direct Noise object (RandomSeed, Std, Type)";
                result = parsed;
                // qDebug() << "MLFeatureExtractor: Direct Noise object keys:" << result.keys();
                break; // Found what we need
            }
        } else {
            qDebug() << "MLFeatureExtractor: JSON parse error:" << parseError.errorString();
        }
    }
    
    return result;
}

#include "mlfeatureextractor.moc"