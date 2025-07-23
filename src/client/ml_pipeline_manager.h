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

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QMutex>

#include "suprafit_cli.h"

class MLPipelineManager : public QObject {
    Q_OBJECT

public:
    MLPipelineManager(QObject* parent = nullptr);
    ~MLPipelineManager();

    // Configuration methods
    void setBatchConfig(const QJsonObject& config);
    void setPipelineSteps(const QStringList& steps);
    
    // Execution methods
    void runBatchPipeline();
    void runSinglePipeline(const QString& configFile);
    
    // ML data extraction methods
    QJsonObject extractMLFeatures(const QVector<QJsonObject>& models);
    QJsonObject extractStatisticToolFeatures(const QVector<QJsonObject>& models);
    
    // StatisticTool integration
    QString runMonteCarloAnalysis(const QVector<QJsonObject>& models, bool local = true, int index = 1);
    QString runCrossValidationAnalysis(const QVector<QJsonObject>& models, int cvtype = 1, bool local = true, int cv_x = 3);
    QString runReductionAnalysis(const QVector<QJsonObject>& models, bool local = true, double cutoff = 0.1);
    
    // ML data formatting
    QJsonObject formatMLTrainingData(const QVector<QJsonObject>& experimentalData, 
                                     const QVector<QJsonObject>& analysisResults,
                                     int trueModelId);
    
    // Batch processing
    void processBatch(int batchId, const QJsonObject& batchConfig);
    
    // Output methods
    void saveMLDataset(const QString& outputDir, const QVector<QJsonObject>& mlData);
    void saveStructuredResults(const QString& filename, const QJsonObject& results);
    
    // Progress tracking
    inline int currentBatch() const { return m_currentBatch; }
    inline int totalBatches() const { return m_totalBatches; }
    inline bool isRunning() const { return m_isRunning; }
    
    // Feature extraction helpers
    QJsonObject extractBasicStatistics(const QSharedPointer<AbstractModel>& model);
    QJsonObject extractMonteCarloFeatures(const QJsonObject& mcResults);
    QJsonObject extractCrossValidationFeatures(const QJsonObject& cvResults);
    QJsonObject extractReductionAnalysisFeatures(const QJsonObject& raResults);
    
signals:
    void batchCompleted(int batchId);
    void pipelineCompleted();
    void progressUpdate(int current, int total);
    void errorOccurred(const QString& error);
    void mlDataGenerated(const QJsonObject& data);

public slots:
    void onBatchFinished();
    void onPipelineError(int batchId, const QString& error);

private:
    // Configuration
    QJsonObject m_batchConfig;
    QStringList m_pipelineSteps;
    
    // Execution state
    bool m_isRunning;
    int m_currentBatch;
    int m_totalBatches;
    int m_numWorkerThreads;
    
    // Data storage
    QVector<QJsonObject> m_mlDataset;
    QVector<QJsonObject> m_batchResults;
    
    // Threading
    QMutex m_dataMutex;
    QVector<QThread*> m_workerThreads;
    
    // Helper methods
    void initializeWorkers();
    void cleanupWorkers();
    QJsonObject parseStatisticToolOutput(const QString& output, const QString& method);
    QVector<double> extractRankingScores(const QString& analysisOutput);
    QJsonObject generateMLLabels(int trueModelId, int testedModelId, const QJsonObject& features);
    
    // Parameter variation
    QJsonObject generateParameterVariation(const QJsonObject& baseConfig, int variationIndex);
    QVector<double> generateRandomParameters(const QJsonObject& bounds);
    
    // Statistics extraction
    double extractShannonEntropy(const QJsonObject& results, const QString& parameter);
    double extractStandardDeviation(const QJsonObject& results, const QString& parameter);
    int extractModelRank(const QJsonObject& results, const QString& modelName);
    
    // Data table export helpers
    void saveTableAsDat(const QJsonObject& table, const QString& filename);
    void saveExperimentAsDat(const QJsonObject& independentTable, const QJsonObject& dependentTable, const QString& filename);
};

// Worker thread for batch processing
class BatchWorker : public QThread {
    Q_OBJECT
    
public:
    BatchWorker(int batchId, const QJsonObject& config, QObject* parent = nullptr);
    
protected:
    void run() override;
    
signals:
    void batchProcessed(int batchId, const QJsonObject& results);
    void batchError(int batchId, const QString& error);
    
private:
    int m_batchId;
    QJsonObject m_config;
    SupraFitCli* m_cli;
};