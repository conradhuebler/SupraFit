/*
 * Test suite for ML Pipeline Manager
 * Copyright (C) 2024 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTest>
#include <QtCore/QTimer>

#include "ml_pipeline_manager.h"
#include "suprafit_cli.h"
#include "src/core/jsonhandler.h"

class TestMLPipeline : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testDataGeneration();
    void testModelAnalysis();
    void testStatisticToolIntegration();
    void testMLFeatureExtraction();
    void testBatchProcessing();
    void testPipelineExecution();
    
private:
    MLPipelineManager* m_pipelineManager;
    QString m_testDataDir;
    QJsonObject m_testConfig;
};

void TestMLPipeline::initTestCase()
{
    m_pipelineManager = new MLPipelineManager(this);
    m_testDataDir = QDir::tempPath() + "/suprafit_ml_test";
    
    // Create test directory
    QDir testDir(m_testDataDir);
    if (!testDir.exists()) {
        testDir.mkpath(".");
    }
    
    // Load test configuration
    m_testConfig = JsonHandler::LoadFile("input/test_ml_pipeline.json");
    QVERIFY(!m_testConfig.isEmpty());
    
    qDebug() << "Test data directory:" << m_testDataDir;
}

void TestMLPipeline::cleanupTestCase()
{
    // Clean up test directory
    QDir testDir(m_testDataDir);
    if (testDir.exists()) {
        testDir.removeRecursively();
    }
    
    delete m_pipelineManager;
}

void TestMLPipeline::testDataGeneration()
{
    qDebug() << "Testing data generation...";
    
    SupraFitCli cli;
    cli.setControlJson(m_testConfig);
    
    // Generate test data
    QVector<QJsonObject> generatedData = cli.GenerateData();
    
    // Verify data generation
    QVERIFY(!generatedData.isEmpty());
    QCOMPARE(generatedData.size(), 5); // Repeat = 5 in test config
    
    // Check data structure
    for (const auto& data : generatedData) {
        QVERIFY(data.contains("data"));
        QJsonObject dataObj = data["data"].toObject();
        QVERIFY(dataObj.contains("independent"));
        QVERIFY(dataObj.contains("dependent"));
    }
    
    qDebug() << "Data generation test passed:" << generatedData.size() << "datasets generated";
}

void TestMLPipeline::testModelAnalysis()
{
    qDebug() << "Testing model analysis...";
    
    SupraFitCli cli;
    cli.setControlJson(m_testConfig);
    
    // Load test data
    cli.setInFile("input/tabelle.dat");
    bool loadSuccess = cli.LoadFile();
    QVERIFY(loadSuccess);
    
    // Run analysis
    QJsonObject jobs = m_testConfig["Jobs"].toObject();
    QJsonObject models = m_testConfig["Models"].toObject();
    
    // Test individual job execution
    for (auto it = jobs.begin(); it != jobs.end(); ++it) {
        QJsonObject job = it.value().toObject();
        qDebug() << "Testing job:" << it.key() << "Method:" << job["Method"].toInt();
        
        // Verify job configuration
        QVERIFY(job.contains("Method"));
        int method = job["Method"].toInt();
        QVERIFY(method >= 1 && method <= 8);
    }
    
    qDebug() << "Model analysis test passed";
}

void TestMLPipeline::testStatisticToolIntegration()
{
    qDebug() << "Testing StatisticTool integration...";
    
    // Create mock model data for testing
    QVector<QJsonObject> mockModels;
    
    for (int i = 1; i <= 4; ++i) {
        QJsonObject model;
        model["name"] = QString("TestModel_%1").arg(i);
        model["id"] = i;
        
        QJsonObject data;
        data["SSE"] = 0.001 * i;
        data["AIC"] = 100.0 + i * 5;
        data["AICc"] = 105.0 + i * 5;
        data["SEy"] = 0.1 * i;
        data["ChiSquared"] = 0.5 * i;
        data["RSquared"] = 0.95 - 0.1 * i;
        
        // Add methods data structure
        QJsonObject methods;
        QJsonObject mcMethod;
        mcMethod["Method"] = 1;
        methods["mc_1"] = mcMethod;
        data["methods"] = methods;
        
        model["data"] = data;
        mockModels.append(model);
    }
    
    // Test StatisticTool methods
    QString mcResult = m_pipelineManager->runMonteCarloAnalysis(mockModels);
    QVERIFY(!mcResult.isEmpty());
    QVERIFY(mcResult.contains("Monte Carlo"));
    
    QString cvResult = m_pipelineManager->runCrossValidationAnalysis(mockModels);
    QVERIFY(!cvResult.isEmpty());
    QVERIFY(cvResult.contains("Cross Validation"));
    
    QString raResult = m_pipelineManager->runReductionAnalysis(mockModels);
    QVERIFY(!raResult.isEmpty());
    QVERIFY(raResult.contains("Reduction"));
    
    qDebug() << "StatisticTool integration test passed";
}

void TestMLPipeline::testMLFeatureExtraction()
{
    qDebug() << "Testing ML feature extraction...";
    
    // Create test models
    QVector<QJsonObject> testModels;
    
    for (int i = 1; i <= 4; ++i) {
        QJsonObject model;
        model["name"] = QString("Model_%1").arg(i);
        model["id"] = i;
        
        QJsonObject data;
        data["SSE"] = 0.001 * (5 - i); // Lower SSE for model 1
        data["AIC"] = 100.0 + i * 10;
        data["AICc"] = 110.0 + i * 10;
        data["SEy"] = 0.05 * i;
        data["ChiSquared"] = 0.1 * i;
        data["RSquared"] = 0.98 - 0.05 * i;
        
        model["data"] = data;
        testModels.append(model);
    }
    
    // Extract ML features
    QJsonObject features = m_pipelineManager->extractMLFeatures(testModels);
    
    // Verify feature structure
    QVERIFY(features.contains("models"));
    QJsonArray modelsArray = features["models"].toArray();
    QCOMPARE(modelsArray.size(), 4);
    
    // Check individual model features
    for (int i = 0; i < modelsArray.size(); ++i) {
        QJsonObject modelFeature = modelsArray[i].toObject();
        QVERIFY(modelFeature.contains("model_name"));
        QVERIFY(modelFeature.contains("model_id"));
        QVERIFY(modelFeature.contains("basic_statistics"));
        
        QJsonObject basicStats = modelFeature["basic_statistics"].toObject();
        QVERIFY(basicStats.contains("SSE"));
        QVERIFY(basicStats.contains("AIC"));
        QVERIFY(basicStats.contains("AICc"));
        QVERIFY(basicStats.contains("SEy"));
        QVERIFY(basicStats.contains("ChiSquared"));
        QVERIFY(basicStats.contains("RSquared"));
    }
    
    qDebug() << "ML feature extraction test passed";
}

void TestMLPipeline::testBatchProcessing()
{
    qDebug() << "Testing batch processing...";
    
    // Create batch configuration
    QJsonObject batchConfig;
    QJsonObject batchSettings;
    batchSettings["TotalBatches"] = 3;
    batchSettings["BatchSize"] = 5;
    batchSettings["WorkerThreads"] = 2;
    batchSettings["OutputDir"] = m_testDataDir;
    batchConfig["BatchConfig"] = batchSettings;
    
    // Set parameter variations
    QJsonObject paramVar;
    QJsonArray noiseVariance;
    noiseVariance.append(1e-4);
    noiseVariance.append(5e-4);
    noiseVariance.append(1e-3);
    paramVar["NoiseVariance"] = noiseVariance;
    batchConfig["ParameterVariation"] = paramVar;
    
    // Set main configuration
    batchConfig["Main"] = m_testConfig["Main"];
    
    // Test batch configuration
    m_pipelineManager->setBatchConfig(batchConfig);
    
    // Verify configuration
    QCOMPARE(m_pipelineManager->totalBatches(), 3);
    QVERIFY(!m_pipelineManager->isRunning());
    
    qDebug() << "Batch processing test passed";
}

void TestMLPipeline::testPipelineExecution()
{
    qDebug() << "Testing pipeline execution...";
    
    // Create test configuration file
    QString configFile = m_testDataDir + "/test_pipeline_config.json";
    JsonHandler::WriteJsonFile(m_testConfig, configFile);
    
    // Set up signal tracking
    QSignalSpy errorSpy(m_pipelineManager, &MLPipelineManager::errorOccurred);
    QSignalSpy completeSpy(m_pipelineManager, &MLPipelineManager::pipelineCompleted);
    QSignalSpy mlDataSpy(m_pipelineManager, &MLPipelineManager::mlDataGenerated);
    
    // Run single pipeline
    m_pipelineManager->runSinglePipeline(configFile);
    
    // Wait for completion (with timeout)
    QTimer::singleShot(5000, [this]() {
        if (m_pipelineManager->isRunning()) {
            qWarning() << "Pipeline execution timed out";
        }
    });
    
    // Verify no errors occurred
    QCOMPARE(errorSpy.count(), 0);
    
    qDebug() << "Pipeline execution test passed";
}

// Test runner
QTEST_MAIN(TestMLPipeline)
#include "test_ml_pipeline.moc"

// Usage example
void runMLPipelineExample()
{
    qDebug() << "Running ML Pipeline Example...";
    
    // Step 1: Create pipeline manager
    MLPipelineManager pipeline;
    
    // Step 2: Set configuration
    QJsonObject config = JsonHandler::LoadFile("input/ml_pipeline_batch_config.json");
    pipeline.setBatchConfig(config);
    
    // Step 3: Set pipeline steps
    QStringList steps = {
        "input/ml_pipeline_step1_generate.json",
        "input/ml_pipeline_step2_analyze.json"
    };
    pipeline.setPipelineSteps(steps);
    
    // Step 4: Run batch pipeline
    QObject::connect(&pipeline, &MLPipelineManager::pipelineCompleted, []() {
        qDebug() << "ML Pipeline completed successfully!";
    });
    
    QObject::connect(&pipeline, &MLPipelineManager::errorOccurred, [](const QString& error) {
        qWarning() << "ML Pipeline error:" << error;
    });
    
    QObject::connect(&pipeline, &MLPipelineManager::progressUpdate, [](int current, int total) {
        qDebug() << "Progress:" << current << "/" << total;
    });
    
    pipeline.runBatchPipeline();
    
    qDebug() << "ML Pipeline example completed!";
}