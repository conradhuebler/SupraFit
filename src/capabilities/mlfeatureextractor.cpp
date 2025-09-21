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

#include "src/core/jsonutils.h"
#include "src/core/analyse.h"

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
    , m_includeRawData(false)
    , m_version("neural_net_v1.0")
{
}

MLFeatureExtractor::~MLFeatureExtractor()
{
}

void MLFeatureExtractor::setExtractionOptions(bool includeAdvancedStats, bool includeFitParameters, bool includeInputNoise, bool includeRawData)
{
    m_includeAdvancedStats = includeAdvancedStats;
    m_includeFitParameters = includeFitParameters;
    m_includeInputNoise = includeInputNoise;
    m_includeRawData = includeRawData;
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
    
    // Simplified Multi-Project format parsing - Claude Generated
    // Expects project_X format created by new ML Pipeline Manager
    QJsonObject mlData;
    QJsonArray fittedModels;
    
    qDebug() << "MLFeatureExtractor: Parsing Multi-Project format, root keys:" << root.keys();
    
    // Extract projects in Multi-Project format (project_X, model_X, etc.) - Claude Generated
    QRegularExpression multiProjectPattern("^(project|model)_(\\d+)$");
    for (auto it = root.begin(); it != root.end(); ++it) {
        QString key = it.key();
        QRegularExpressionMatch match = multiProjectPattern.match(key);
        if (match.hasMatch()) {
            QJsonObject projectData = it.value().toObject();
            
            // Convert project data to fitted model format
            QJsonObject fittedModel;
            fittedModel["model_name"] = projectData["model_name"];
            fittedModel["model_id"] = projectData["model_id"];
            fittedModel["convergence"] = projectData["convergence"];
            fittedModel["sse"] = projectData["sse"];
            fittedModel["fitted_parameters"] = projectData["fitted_parameters"];
            fittedModel["timestamp"] = projectData["timestamp"];
            
            // Include post-fit analysis data
            if (projectData.contains("post_fit_analysis")) {
                fittedModel["post_fit_analysis"] = projectData["post_fit_analysis"];
            }
            
            // Include ML features if available
            if (projectData.contains("ml_features")) {
                fittedModel["ml_features"] = projectData["ml_features"];
            }
            
            fittedModels.append(fittedModel);
            QString projectType = match.captured(1); // "project" or "model"
            QString projectIndex = match.captured(2); // "0", "1", etc.
            qDebug() << "MLFeatureExtractor: Added" << projectType << "entry" << key << "as fitted model";
        }
    }
    
    if (!fittedModels.isEmpty()) {
        mlData["fitted_models"] = fittedModels;
        mlData["format_version"] = root["format_version"];
        mlData["generation_timestamp"] = root["generation_timestamp"];
        
        // Extract ground truth and input noise from Multi-Project format - Claude Generated
        QJsonObject generationConfig;
        
        // First, check for ground_truth at root level (Multi-Project format)
        if (root.contains("ground_truth")) {
            QJsonObject groundTruth = root["ground_truth"].toObject();
            generationConfig["ground_truth_model"] = groundTruth;
            qDebug() << "MLFeatureExtractor: Extracted ground_truth from root level";
        }
        
        // Also extract from base_data if available
        if (root.contains("base_data")) {
            QJsonObject baseData = root["base_data"].toObject();
            
            // Create generation config from base data content
            if (baseData.contains("content")) {
                QString content = baseData["content"].toString();
                
                // Parse input noise configuration from content JSON - Claude Generated
                if (content.contains("Input Configuration") && content.contains("Noise")) {
                    QJsonObject inputNoise = parseNoiseFromContent(content);
                    if (!inputNoise.isEmpty()) {
                        inputNoise["source"] = "base_data_content";
                        generationConfig["noise_applied"] = inputNoise;
                        qDebug() << "MLFeatureExtractor: Extracted noise from content:" << inputNoise.keys();
                    }
                }
                
                // If no ground truth yet, try parsing from content
                if (!generationConfig.contains("ground_truth_model") && 
                    content.contains("Global Parameters") && content.contains("Local Parameters")) {
                    QJsonObject groundTruth;
                    groundTruth["source"] = "base_data_content";
                    generationConfig["ground_truth_model"] = groundTruth;
                }
            }
        }
        
        if (!generationConfig.isEmpty()) {
            mlData["generation_config"] = generationConfig;
        }
        
        qDebug() << "MLFeatureExtractor: Created ML data structure with" << fittedModels.size() << "models";
    } else {
        qWarning() << "MLFeatureExtractor: No Multi-Project entries (project_X, model_X) found in" << filename;
        qDebug() << "MLFeatureExtractor: Available root keys:" << root.keys();
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
        
        // Extract all available ground-truth parameters - handle both formats
        if (groundTruthModel.contains("model_id")) {
            groundTruth["model_id"] = groundTruthModel["model_id"];
        } else if (groundTruthModel.contains("id")) {
            groundTruth["model_id"] = groundTruthModel["id"];
        }
        
        if (groundTruthModel.contains("model_name")) {
            groundTruth["model_name"] = groundTruthModel["model_name"];
        } else if (groundTruthModel.contains("name")) {
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
    // Simplified ground-truth extraction for Multi-Project format - Claude Generated
    QJsonObject groundTruth = extractGroundTruth(generationConfig);
    
    // Include input noise from generation config only
    if (m_includeInputNoise) {
        QJsonObject inputNoise = extractInputNoise(generationConfig);
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
        
        // Note: Matrix string limits (GlobalRandomLimits/LocalRandomLimits) are 
        // parameter generation limits, not measurement noise - skipped for ML features
        
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
    
    // Use SupraFit::JsonUtils for robust post-fit analysis access - Claude Generated
    if (!SupraFit::JsonUtils::hasPostFitAnalysis(postFitAnalysis)) {
        qDebug() << "MLFeatureExtractor: No valid post-fit analysis structure found";
        return features;
    }
    
    // Get available statistical methods for this model - Claude Generated
    QStringList availableMethods = SupraFit::JsonUtils::getAvailableStatisticalMethods(postFitAnalysis);
    qDebug() << "MLFeatureExtractor: Available statistical methods:" << availableMethods;
    
    // Extract features for each available method using SupraFit core analyse.cpp functions
    for (const QString& methodId : availableMethods) {
        int methodInt = methodId.toInt();
        SupraFit::Method method = static_cast<SupraFit::Method>(methodInt);
        
        // Get raw statistical method data using JsonUtils
        QJsonObject methodData = SupraFit::JsonUtils::getStatisticalMethod(postFitAnalysis, method);
        qDebug() << "MLFeatureExtractor: Processing method" << methodId << "keys:" << methodData.keys();
        
        if (methodData.isEmpty()) {
            qDebug() << "MLFeatureExtractor: No data found for method" << methodId;
            continue;
        }
        
        // Process features based on method type using analyse.cpp functions
        QJsonObject methodFeatures;
        
        switch (method) {
            case SupraFit::Method::MonteCarlo: {
                // Monte Carlo: Extract parameter features (with optional raw data) - Claude Generated
                if (m_includeRawData) {
                    // Use full StatisticTool analysis with raw data included
                    QVector<QJsonObject> modelVector = {methodData};
                    methodFeatures = StatisticTool::CalculateMCMetrics(modelVector, 1);
                } else {
                    // Use compact features without raw data
                    methodFeatures = SupraFit::JsonUtils::extractCompactMLFeatures(methodData);
                }
                if (!methodFeatures.isEmpty()) {
                    features["monte_carlo"] = methodFeatures;
                }
                break;
            }
            
            case SupraFit::Method::CrossValidation: {
                // Cross Validation: Extract real CV results directly from methodData - Claude Generated
                // methodData already contains the real CV results, no need for StatisticTool processing
                methodFeatures = methodData;  // Use the real Cross Validation data directly
                if (!methodFeatures.isEmpty()) {
                    features["cross_validation"] = methodFeatures;
                }
                break;
            }
            
            case SupraFit::Method::WeakenedGridSearch: {
                // Weakened Grid Search: Use CalculateWGSMetrics from analyse.cpp
                QVector<QJsonObject> modelVector = {methodData};
                methodFeatures = StatisticTool::CalculateWGSMetrics(modelVector);
                if (!methodFeatures.isEmpty()) {
                    features["weakened_grid_search"] = methodFeatures;
                }
                break;
            }
            
            case SupraFit::Method::ModelComparison: {
                // Model Comparison: Use CalculateModelComparisonMetrics from analyse.cpp
                QVector<QJsonObject> modelVector = {methodData};
                methodFeatures = StatisticTool::CalculateModelComparisonMetrics(modelVector);
                if (!methodFeatures.isEmpty()) {
                    features["model_comparison"] = methodFeatures;
                }
                break;
            }
            
            case SupraFit::Method::Reduction: {
                // Reduction Analysis: Use CalculateReductionMetrics from analyse.cpp
                QVector<QJsonObject> modelVector = {methodData};
                double cutoff = 0.1; // Default cutoff
                if (methodData.contains("configuration")) {
                    QJsonObject config = methodData["configuration"].toObject();
                    cutoff = config.contains("cutoff") ? config["cutoff"].toDouble() : 0.1;
                }
                methodFeatures = StatisticTool::CalculateReductionMetrics(modelVector, cutoff);
                if (!methodFeatures.isEmpty()) {
                    features["reduction_analysis"] = methodFeatures;
                }
                break;
            }
            
            case SupraFit::Method::FastConfidence: {
                // Fast Confidence: Use CalculateFastConfidenceMetrics from analyse.cpp
                QVector<QJsonObject> modelVector = {methodData};
                methodFeatures = StatisticTool::CalculateFastConfidenceMetrics(modelVector);
                if (!methodFeatures.isEmpty()) {
                    features["fast_confidence"] = methodFeatures;
                }
                break;
            }
            
            case SupraFit::Method::GlobalSearch: {
                // Global Search: Use CalculateGlobalSearchMetrics from analyse.cpp
                QVector<QJsonObject> modelVector = {methodData};
                methodFeatures = StatisticTool::CalculateGlobalSearchMetrics(modelVector);
                if (!methodFeatures.isEmpty()) {
                    features["global_search"] = methodFeatures;
                }
                break;
            }
            
            default: {
                // Unknown method: store raw data with method ID
                qDebug() << "MLFeatureExtractor: Unknown statistical method" << methodId << "- storing raw data";
                if (!methodData.isEmpty()) {
                    features[QString("method_%1").arg(methodId)] = methodData;
                }
                break;
            }
        }
    }
    
    qDebug() << "MLFeatureExtractor: Extracted statistical features:" << features.keys();
    return features;
}

QJsonObject MLFeatureExtractor::parseNoiseFromContent(const QString& content)
{
    QJsonObject noiseConfig;
    
    // Find JSON configuration in content string - Claude Generated
    // Pattern: "Noise":{"Type":"gaussian","Std":[0.001,0.001],"RandomSeed":12345}
    
    QRegularExpression noisePattern("\"Noise\"\\s*:\\s*\\{([^}]+)\\}");
    QRegularExpressionMatch match = noisePattern.match(content);
    
    if (match.hasMatch()) {
        QString noiseJsonStr = "{" + match.captured(1) + "}";
        qDebug() << "MLFeatureExtractor: Found noise JSON in content:" << noiseJsonStr;
        
        // Parse the extracted JSON string
        QJsonParseError parseError;
        QJsonDocument noiseDoc = QJsonDocument::fromJson(noiseJsonStr.toUtf8(), &parseError);
        
        if (parseError.error == QJsonParseError::NoError && noiseDoc.isObject()) {
            QJsonObject rawNoise = noiseDoc.object();
            
            // Extract and normalize noise parameters
            if (rawNoise.contains("Type")) {
                noiseConfig["type"] = rawNoise["Type"].toString().toLower();
            }
            if (rawNoise.contains("Std")) {
                noiseConfig["std"] = rawNoise["Std"];
            }
            if (rawNoise.contains("RandomSeed")) {
                noiseConfig["random_seed"] = rawNoise["RandomSeed"].toInt();
            }
            
            // Store original configuration for reference
            noiseConfig["original_config"] = rawNoise;
            
            qDebug() << "MLFeatureExtractor: Successfully parsed noise config from content";
        } else {
            qWarning() << "MLFeatureExtractor: Failed to parse noise JSON:" << parseError.errorString();
        }
    } else {
        qDebug() << "MLFeatureExtractor: No noise configuration pattern found in content";
    }
    
    return noiseConfig;
}

// All primitive extraction methods removed - Claude Generated
// Now using robust JsonUtils + analyse.cpp functions in extractStatisticalFeatures()

#include "mlfeatureextractor.moc"
