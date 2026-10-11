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

#pragma once

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

// SupraFit core includes for statistical feature extraction - Claude Generated
#include "src/core/toolset.h"
#include "src/core/analyse.h"
#include "src/core/jsonhandler.h"

/**
 * @brief ML Feature Extractor for Neural Network Training Data
 * 
 * Extracts compact training samples from ML Pipeline RawData for neural network
 * model selection training. Converts comprehensive ML pipeline results into
 * standardized feature vectors suitable for machine learning algorithms.
 * 
 * Features extracted include:
 * - Ground-truth model parameters and input noise configuration
 * - Fitted model statistical metrics (AIC, SSE, R², etc.)
 * - Post-processing analysis results (MC, CV, Reduction analysis)
 * - Model selection labels based on ground-truth model ID matching
 * 
 * Claude Generated - 2025
 */
class MLFeatureExtractor : public QObject {
    Q_OBJECT

public:
    explicit MLFeatureExtractor(QObject* parent = nullptr);
    ~MLFeatureExtractor();

    /**
     * @brief Parse ML Pipeline RawData from JSON file
     * @param filename Path to JSON file containing ML pipeline results
     * @return QJsonObject with parsed ML RawData structure, empty if parsing failed
     */
    QJsonObject parseMLPipelineData(const QString& filename);

    /**
     * @brief Extract compact training sample from ML RawData
     * @param mlRawData Complete ML pipeline data structure
     * @return QJsonObject with compact training sample suitable for neural network training
     */
    QJsonObject extractCompactTrainingSample(const QJsonObject& mlRawData);

    /**
     * @brief Batch process multiple ML pipeline files for training data extraction
     * @param filenames Vector of file paths to process
     * @return QVector<QJsonObject> with extracted training samples from all files
     */
    QVector<QJsonObject> extractBatchTrainingData(const QVector<QString>& filenames);

    /**
     * @brief Export training samples in neural network optimized format
     * @param trainingSamples Vector of training samples from extractBatchTrainingData
     * @return QJsonObject with standardized neural network training dataset
     */
    QJsonObject exportNeuralNetFormat(const QVector<QJsonObject>& trainingSamples);

    /**
     * @brief Set feature extraction options
     * @param includeAdvancedStats Include advanced statistical analysis (MC, CV, etc.)
     * @param includeFitParameters Include fitted model parameter values
     * @param includeInputNoise Include input noise/variance configuration
     * @param includeRawData Include raw statistical data (MC samples, CV results, etc.)
     */
    void setExtractionOptions(bool includeAdvancedStats = true,
                            bool includeFitParameters = false,
                            bool includeInputNoise = true,
                            bool includeRawData = false);

private:
    /**
     * @brief Extract ground-truth information from generation config
     * @param generationConfig ML pipeline generation_config section
     * @return QJsonObject with ground-truth model parameters and input configuration
     */
    QJsonObject extractGroundTruth(const QJsonObject& generationConfig);

    /**
     * @brief Extract ground-truth with content string fallback for measurement noise
     * @param generationConfig ML pipeline generation_config section
     * @param contentString data.content string for measurement noise parsing fallback
     * @return QJsonObject with complete ground-truth including measurement noise info
     */
    QJsonObject extractGroundTruthWithContent(const QJsonObject& generationConfig, const QString& contentString);

    /**
     * @brief Extract candidate model features from fitted models
     * @param fittedModels Array of fitted models with statistical analysis
     * @return QJsonArray with candidate model features for neural network input
     */
    QJsonArray extractCandidateModels(const QJsonArray& fittedModels);

    /**
     * @brief Determine correct model label based on ground-truth model ID
     * @param groundTruth Ground-truth model information
     * @param candidates Array of candidate models
     * @return QJsonObject with correct model identification for supervised learning
     */
    QJsonObject determineCorrectLabel(const QJsonObject& groundTruth, const QJsonArray& candidates);

    /**
     * @brief Extract input noise/variance parameters from generation config
     * @param generationConfig ML pipeline generation configuration
     * @return QJsonObject with noise parameters used for synthetic data generation
     */
    QJsonObject extractInputNoise(const QJsonObject& generationConfig);

    /**
     * @brief Extract statistical features from post-fit analysis
     * @param postFitAnalysis Post-processing analysis results
     * @return QJsonObject with statistical confidence metrics for model selection
     */
    QJsonObject extractStatisticalFeatures(const QJsonObject& postFitAnalysis);

    // Statistical method feature extraction functions - Claude Generated
    // Primitive extraction methods removed - Claude Generated
    // Now using robust JsonUtils + analyse.cpp functions instead
    
    /**
     * @brief Parse noise configuration from base_data content string
     * @param content The content string containing JSON configuration
     * @return QJsonObject with parsed noise parameters (type, std, random_seed)
     */
    QJsonObject parseNoiseFromContent(const QString& content);


    /**
     * @brief Parse matrix string limits from input_json (GlobalRandomLimits/LocalRandomLimits)
     * @param inputJson Input configuration JSON with matrix string limits
     * @return QJsonObject with structured min/max arrays
     */
    QJsonObject parseMatrixStringLimits(const QJsonObject& inputJson);

    /**
     * @brief Parse individual matrix string to min/max structure
     * @param matrixStr Matrix string like "[2 5]" or "[6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5]"
     * @param type "global" for single pair, "local" for multiple pairs
     * @return QJsonObject with min/max arrays
     */
    QJsonObject parseMatrixString(const QString& matrixStr, const QString& type);

    // Configuration options
    bool m_includeAdvancedStats;
    bool m_includeFitParameters;
    bool m_includeInputNoise;
    bool m_includeRawData;
    
    // Metadata
    QString m_version;
};