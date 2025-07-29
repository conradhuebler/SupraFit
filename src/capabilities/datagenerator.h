/*
 * DataGenerator - Mathematical equation-based data generation for SupraFit
 * Copyright (C) 2022 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 * 
 * This class provides JavaScript-based equation evaluation for generating
 * independent variable data from mathematical expressions. It supports
 * multiple variables and can be used for random parameter generation
 * in machine learning pipelines.
 * 
 * Enhanced by Claude Code AI Assistant for improved functionality.
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

#include <QtCore/QJsonObject>
#include <QtCore/QObject>

#include <QJSEngine>
#include <random> // Claude Generated

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

class DataGenerator : public QObject {
    Q_OBJECT
public:
    explicit DataGenerator(QObject* parent = nullptr);

    // Core functionality
    void setJson(const QJsonObject& data) { m_data = data; }
    bool Evaluate();
    DataTable* Table() const { return m_table; }

    // Enhanced functionality - Claude Generated
    bool EvaluateWithRandomParameters(const QJsonObject& randomLimits = QJsonObject());
    void setRandomSeed(quint64 seed) { m_randomSeed = seed; initializeRNG(); } // Claude Generated
    
    // Model-based generation - Claude Generated Integration
    bool EvaluateWithModel(int modelId, QPointer<DataClass> dataClass, const QJsonObject& config = QJsonObject());
    bool GenerateModelBasedData(int modelId, QPointer<DataClass> inputData, const QJsonObject& modelConfig);

    // Performance optimization - Claude Generated
    void enablePerformanceMode(bool enabled = true) { m_performanceMode = enabled; }

    // Batch processing for multiple datasets - Claude Generated
    bool EvaluateWithModelBatch(int modelId, QPointer<DataClass> dataClass,
        const QVector<QJsonObject>& configs,
        QVector<DataTable*>& results);

    // Post-fit analysis integration with JobManager - Claude Generated
    bool EvaluateWithModelAndAnalyze(int modelId, QPointer<DataClass> dataClass, const QJsonObject& config);

    // Enhanced method for JSON structure v2.0 - Claude Generated
    bool EvaluateWithModelAndAnalyzeV2(int modelId, QPointer<DataClass> dataClass,
        const QJsonObject& config,
        const QJsonObject& globalAnalysisConfig,
        const QJsonObject& modelSpecificConfig = QJsonObject());

    QJsonObject executePostFitAnalysis(QSharedPointer<AbstractModel> model, const QJsonObject& analysisConfig);

    // Performance monitoring - Claude Generated
    void resetPerformanceCounters();
    int getModelCreationCount() const { return m_modelCreationCount; }

    // Test utility methods - Claude Generated (moved to public for testing)
    static QJsonObject generateRandomParameters(const QJsonObject& limits, quint64 seed = 0);
    static double generateRandomValue(double min, double max, quint64 seed = 0);

private:
    // Post-fit analysis now handled by JobManager in CLI - Claude Generated
    static bool applyModelRandomization(QSharedPointer<class AbstractModel> model, const QJsonObject& config, int series = 1);
    void applyGlobalRandomLimits(QSharedPointer<class AbstractModel> model, const QString& globalLimits); // Claude Generated
    void applyLocalRandomLimits(QSharedPointer<class AbstractModel> model, const QString& localLimits); // Claude Generated
    
    // Content enhancement - Claude Generated
    QString createEnhancedContent(QPointer<DataClass> dataClass, const QString& originalContent, const QJsonObject& config, QSharedPointer<class AbstractModel> model = nullptr);

private:
    double m_generator_version = 1.0;
    QJsonObject m_data;
    QPointer<DataTable> m_table;
    quint64 m_randomSeed = 0;
    mutable std::mt19937 m_rng; // Global random number generator - Claude Generated
    
    // Helper methods - Claude Generated  
    void setupRandomEngine(QJSEngine& engine, const QJsonObject& randomParams);
    void createControlledRandomFunctions(QJSEngine& engine, const QJsonObject& randomParams); // Claude Generated
    void initializeRNG(); // Claude Generated
    
    // Member variable for storing random parameters - Claude Generated
    QJsonObject m_randomParams;

    // Performance optimization - Claude Generated
    bool m_performanceMode = false;
    QString m_cachedContent; // Reuse content template for JSON formatting
    QJsonObject m_lastConfig; // Track configuration changes for content caching

    // Performance monitoring - Claude Generated
    mutable int m_modelCreationCount = 0;

public slots:
    // Qt slots for JavaScript integration - Claude Generated
    double generateControlledRandom(const QString& parameterName);
    double generateControlledRandomValue(double min, double max, const QString& distribution = "uniform");
    
signals:
};
