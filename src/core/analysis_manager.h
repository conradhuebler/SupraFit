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

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVector>

#include "src/core/models/dataclass.h"
#include "src/core/models/AbstractModel.h"

// Model statistics structure for analysis results - Claude Generated
struct ModelStatistics {
    QString key;           // Model identifier (e.g., "model_0")
    QString name;          // Model name
    QString status;        // Convergence status
    bool hasValidStats = false;
    
    // Core fit statistics
    double sse = -1;       // Sum of Squared Errors
    double sae = -1;       // Sum of Absolute Errors  
    double aic = -999;     // Akaike Information Criterion
    double aicc = -999;    // Corrected AIC
    
    // Parameter counts
    int globalParams = -1; // Global parameters
    int localParams = -1;  // Local parameters
    
    // Post-processing analysis counts
    int mcBlocks = 0;           // Monte Carlo blocks
    int wgsBlocks = 0;          // Weakened Grid Search blocks
    int modelCompBlocks = 0;    // Model Comparison blocks
    int cvBlocks = 0;           // Cross Validation blocks
    int reductionBlocks = 0;    // Reduction blocks
    int fastConfBlocks = 0;     // Fast Confidence blocks
    int globalBlocks = 0;       // Global Search blocks
    int totalPPBlocks = 0;      // Total post-processing blocks
    
    // Post-processing data cache
    QJsonObject postProcessingData;
};

class AnalysisManager : public QObject
{
    Q_OBJECT

public:
    explicit AnalysisManager(QObject* parent = nullptr);
    ~AnalysisManager() = default;

    // ===== FILE ANALYSIS API =====
    // Migrated from CLI AnalyzeFile() function
    
    /**
     * @brief Comprehensive file analysis - migrated from CLI AnalyzeFile()
     * @param filePath Path to .suprafit or .json file for analysis
     * @return QJsonObject with detailed file analysis results
     */
    QJsonObject analyzeFile(const QString& filePath);
    
    /**
     * @brief Analyze loaded DataClass object
     * @param data DataClass to analyze
     * @return QJsonObject with data structure analysis
     */
    QJsonObject analyzeDataClass(QPointer<DataClass> data);

    // ===== MODEL COMPARISON API =====
    // Migrated from CLI FitModelsToData() function
    
    /**
     * @brief Fit multiple models to data for comparison and ML feature extraction
     * Migrated from CLI FitModelsToData() - Tests various analytical models against
     * simulated or experimental data, extracting statistical metrics for model
     * selection and ML training.
     * 
     * @param data DataClass with experimental data
     * @param modelsConfig Models to test (JSON configuration)
     * @param globalAnalysisConfig Global analysis settings
     * @return QVector<QJsonObject> with fitted model results and statistics
     */
    QVector<QJsonObject> fitModelsToData(QPointer<DataClass> data, 
                                        const QJsonObject& modelsConfig,
                                        const QJsonObject& globalAnalysisConfig = QJsonObject());

    // ===== STATISTICAL ANALYSIS API =====
    // Unified interface for all statistical analysis methods
    
    /**
     * @brief Perform complete statistical analysis on fitted model
     * @param model Fitted model for analysis
     * @param analysisConfig Configuration for analysis types (MC, CV, etc.)
     * @return QJsonObject with comprehensive statistical results
     */
    QJsonObject performCompleteAnalysis(QSharedPointer<AbstractModel> model,
                                       const QJsonObject& analysisConfig);

    /**
     * @brief Execute post-fit analysis with JobManager integration
     * @param model Model to analyze
     * @param analysisConfig Analysis configuration
     * @return QJsonObject with analysis results from all enabled methods
     */
    QJsonObject executePostFitAnalysis(QSharedPointer<AbstractModel> model, 
                                      const QJsonObject& analysisConfig);

    // ===== UTILITY METHODS =====
    
    /**
     * @brief Create model instance from configuration
     * @param modelId Model ID to create
     * @param data DataClass for model association
     * @return QSharedPointer<AbstractModel> or nullptr if failed
     */
    QSharedPointer<AbstractModel> createModelFromConfig(int modelId, QPointer<DataClass> data);

    /**
     * @brief Generate analysis summary for multiple models
     * @param models Vector of analyzed models
     * @return QJsonObject with comparison summary
     */
    QJsonObject generateModelComparisonSummary(const QVector<QJsonObject>& models);
    
    /**
     * @brief Evaluate model fit with comprehensive statistics
     * @param model Fitted model to evaluate
     * @param trueModelId Optional true model ID for comparison (-1 if unknown)
     * @return QJsonObject with comprehensive evaluation metrics
     */
    QJsonObject evaluateModelFit(QSharedPointer<AbstractModel> model, int trueModelId = -1);
    
    /**
     * @brief Execute post-fit analysis using JobManager integration
     * @param model Model to analyze
     * @param analysisConfig Analysis configuration
     * @return QJsonObject with post-fit analysis results
     */
    QJsonObject runPostFitAnalysis(QSharedPointer<AbstractModel> model, 
                                  const QJsonObject& analysisConfig);

signals:
    /**
     * @brief Progress update during analysis operations
     * @param message Progress message
     * @param percentage Progress percentage (0-100)
     */
    void analysisProgress(const QString& message, int percentage);
    
    /**
     * @brief Analysis operation completed
     * @param success Whether operation succeeded
     * @param results Analysis results
     */
    void analysisCompleted(bool success, const QJsonObject& results);
    
    /**
     * @brief Error occurred during analysis
     * @param errorMessage Detailed error description
     */
    void analysisError(const QString& errorMessage);

private:
    // ===== INTERNAL HELPER METHODS =====
    
    /**
     * @brief Analyze file information and metadata
     * @param filePath File to analyze
     * @return QJsonObject with file information
     */
    QJsonObject analyzeFileInformation(const QString& filePath);
    
    /**
     * @brief Analyze data structure and content
     * @param data DataClass to analyze
     * @return QJsonObject with data structure analysis
     */
    QJsonObject analyzeDataStructure(QPointer<DataClass> data);
    
    /**
     * @brief Analyze independent data table
     * @param data DataClass with independent data
     * @return QJsonObject with independent data statistics
     */
    QJsonObject analyzeIndependentData(QPointer<DataClass> data);
    
    /**
     * @brief Analyze dependent data table and models
     * @param data DataClass with dependent data and models
     * @return QJsonObject with dependent data and model analysis
     */
    QJsonObject analyzeDependentDataAndModels(QPointer<DataClass> data);
    
    /**
     * @brief Fit single model to data with error handling
     * @param modelConfig Model configuration
     * @param data Target data
     * @param globalAnalysisConfig Global settings
     * @return QJsonObject with model results or error information
     */
    QJsonObject fitSingleModel(const QJsonObject& modelConfig,
                              QPointer<DataClass> data,
                              const QJsonObject& globalAnalysisConfig);
    
    /**
     * @brief Extract model statistics from JSON toplevel object
     * @param toplevel JSON object containing model data
     * @return Vector of model statistics
     */
    QVector<ModelStatistics> extractModelStatistics(const QJsonObject& toplevel);
    
    /**
     * @brief Extract statistics for a single model
     * @param key Model key (e.g., "model_0")
     * @param modelObj Model JSON object
     * @return Single model statistics
     */
    ModelStatistics extractSingleModelStatistics(const QString& key, const QJsonObject& modelObj);
    
    /**
     * @brief Analyze post-processing methods for a model
     * @param stats Model statistics to update
     * @param methods Methods JSON object
     * @param modelVector Vector of model JSON objects
     */
    void analyzePostProcessingMethods(ModelStatistics& stats, 
                                    const QJsonObject& methods,
                                    const QVector<QJsonObject>& modelVector);
    
    /**
     * @brief Generate JSON object from model statistics vector
     * @param models Vector of model statistics
     * @return JSON object with model statistics
     */
    QJsonObject generateModelStatisticsJson(const QVector<ModelStatistics>& models);

private:
    // Performance and configuration
    bool m_verbose = true;          // Enable detailed logging
    int m_maxConcurrentModels = 4;  // Parallel model fitting limit
    
    // Statistics for performance monitoring
    int m_totalAnalysisCount = 0;
    int m_successfulAnalysisCount = 0;
    
    // Claude Generated
};