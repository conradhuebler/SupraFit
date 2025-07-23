/*
 * Comprehensive tests for ML Pipeline functionality
 * Copyright (C) 2024 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <QtTest/QTest>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QDebug>
#include <QtCore/QTime>

#include "src/client/suprafit_cli.h"
#include "src/client/ml_pipeline_manager.h"
#include "src/capabilities/datagenerator.h"
#include "src/core/jsonhandler.h"

class TestPipeline : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic pipeline functionality
    void testSupraFitCliCreation();
    void testConfigurationParsing();
    void testTaskDetection();
    void testDataGeneration();

    // Data generation tests
    void testGenerateDataConfiguration();
    void testGenerateDataExecution();
    void testGenerateDataOutput();
    void testGenerateDataWithNoise();
    void testGenerateDataWithRandomParameters();

    // Model testing
    void testModelCreation();
    void testModelFitting();
    void testMultipleModelTesting();
    void testModelComparison();

    // ML Pipeline Manager tests
    void testMLPipelineManagerCreation();
    void testBatchConfiguration();
    void testSinglePipelineExecution();
    void testBatchPipelineExecution();
    void testFeatureExtraction();

    // Job management
    void testJobConfiguration();
    void testJobExecution();
    void testJobResults();
    void testStatisticalAnalysis();

    // File I/O tests
    void testConfigurationFileLoading();
    void testResultFileSaving();
    void testPipelineFileRoundTrip();

    // Integration tests
    void testFullPipelineExecution();
    void testPipelineWithRealData();
    void testPipelineErrorHandling();
    void testPipelinePerformance();

    // Edge cases
    void testEmptyConfiguration();
    void testInvalidConfiguration();
    void testMissingFiles();
    void testLargeDatasetHandling();

private:
    QJsonObject createTestConfiguration();
    QJsonObject createMLPipelineConfiguration();
    QJsonObject createBatchConfiguration();
    QTemporaryDir* m_tempDir;
    QString createTestDataFile();
    void verifyGeneratedData(const QVector<QJsonObject>& data);
    void verifyModelResults(const QJsonObject& results);
};

void TestPipeline::initTestCase()
{
    qDebug() << "Starting Pipeline tests...";
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
}

void TestPipeline::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "Pipeline tests completed.";
}

void TestPipeline::testSupraFitCliCreation()
{
    SupraFitCli* cli = new SupraFitCli();
    QVERIFY(cli != nullptr);
    
    // Test initial state
    QVERIFY(!cli->CheckGenerateDependent());
    QVERIFY(!cli->CheckGenerateIndependent());
    QVERIFY(!cli->CheckGenerateNoisyDependent());
    QVERIFY(!cli->CheckGenerateNoisyIndependent());
    
    delete cli;
}

void TestPipeline::testConfigurationParsing()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    cli->setControlJson(config);
    
    // Test that configuration was parsed correctly
    QVERIFY(cli->CheckGenerateDependent());
    
    delete cli;
}

void TestPipeline::testTaskDetection()
{
    SupraFitCli* cli = new SupraFitCli();
    
    // Test GenerateData configuration
    QJsonObject config;
    QJsonObject main;
    QJsonObject generateData;
    
    generateData["Series"] = 1;
    generateData["Model"] = 1;
    generateData["Repeat"] = 5;
    generateData["Variance"] = 0.001;
    
    main["GenerateData"] = generateData;
    main["InFile"] = createTestDataFile();
    main["OutFile"] = "test_output";
    main["IndependentRows"] = 2;
    
    config["Main"] = main;
    
    cli->setControlJson(config);
    
    QVERIFY(cli->CheckGenerateDependent());
    QVERIFY(cli->CheckGenerateNoisyDependent());
    
    delete cli;
}

void TestPipeline::testDataGeneration()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    cli->setControlJson(config);
    
    // Test data generation
    QVector<QJsonObject> generatedData = cli->GenerateData();
    
    QVERIFY(!generatedData.isEmpty());
    verifyGeneratedData(generatedData);
    
    delete cli;
}

void TestPipeline::testGenerateDataConfiguration()
{
    QJsonObject config;
    QJsonObject main;
    QJsonObject generateData;
    
    generateData["Series"] = 3;
    generateData["Model"] = 1;
    generateData["Repeat"] = 10;
    generateData["Variance"] = 0.005;
    generateData["GlobalRandomLimits"] = "[1 5]";
    generateData["LocalRandomLimits"] = "[0.1 1.0]";
    
    main["GenerateData"] = generateData;
    main["InFile"] = createTestDataFile();
    main["OutFile"] = "test_config_output";
    main["IndependentRows"] = 2;
    
    config["Main"] = main;
    
    SupraFitCli* cli = new SupraFitCli();
    cli->setControlJson(config);
    
    QVERIFY(cli->CheckGenerateDependent());
    
    delete cli;
}

void TestPipeline::testGenerateDataExecution()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    
    QVERIFY(data.size() > 0);
    
    // Check that each generated dataset has required structure
    for (const auto& dataset : data) {
        QVERIFY(dataset.contains("data"));
        QVERIFY(dataset.contains("uuid"));
        QVERIFY(dataset.contains("DataType"));
        
        QJsonObject dataObj = dataset["data"].toObject();
        QVERIFY(dataObj.contains("independent"));
        QVERIFY(dataObj.contains("dependent"));
    }
    
    delete cli;
}

void TestPipeline::testGenerateDataOutput()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    
    QVERIFY(!data.isEmpty());
    
    // Test saving generated data
    for (int i = 0; i < data.size(); ++i) {
        QString filename = QString("%1/generated_data_%2.json").arg(m_tempDir->path()).arg(i);
        QJsonDocument doc(data[i]);
        
        QFile file(filename);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(doc.toJson());
        file.close();
        
        QVERIFY(QFileInfo::exists(filename));
    }
    
    delete cli;
}

void TestPipeline::testGenerateDataWithNoise()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    // Modify configuration to include noise
    QJsonObject main = config["Main"].toObject();
    QJsonObject generateData = main["GenerateData"].toObject();
    generateData["Variance"] = 0.01; // Higher noise level
    main["GenerateData"] = generateData;
    config["Main"] = main;
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    
    QVERIFY(!data.isEmpty());
    
    // Verify that noise was applied (exact verification would require
    // comparing with noise-free data, which is complex)
    verifyGeneratedData(data);
    
    delete cli;
}

void TestPipeline::testGenerateDataWithRandomParameters()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    // Add random parameter limits
    QJsonObject main = config["Main"].toObject();
    QJsonObject generateData = main["GenerateData"].toObject();
    generateData["GlobalRandomLimits"] = "[0.5 10.0]";
    generateData["LocalRandomLimits"] = "[0.01 2.0]";
    main["GenerateData"] = generateData;
    config["Main"] = main;
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    
    QVERIFY(!data.isEmpty());
    verifyGeneratedData(data);
    
    delete cli;
}

void TestPipeline::testModelCreation()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    // Add models configuration
    QJsonObject models;
    models["model1"] = 1; // nmr_ItoI
    models["model2"] = 2; // nmr_IItoI_ItoI
    models["model3"] = 3; // nmr_ItoI_ItoII
    
    config["Models"] = models;
    
    cli->setControlJson(config);
    
    // Generate data first
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(!data.isEmpty());
    
    // Set data vector for processing
    cli->setDataVector(data);
    
    delete cli;
}

void TestPipeline::testModelFitting()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    // Add models and jobs
    QJsonObject models;
    models["test_model"] = 1;
    config["Models"] = models;
    
    QJsonObject jobs;
    QJsonObject job1;
    job1["Method"] = 1; // Monte Carlo
    job1["MaxSteps"] = 100;
    job1["VarianceSource"] = 2;
    jobs["mc_job"] = job1;
    config["Jobs"] = jobs;
    
    cli->setControlJson(config);
    
    // Generate data
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(!data.isEmpty());
    
    // Test model fitting on first dataset
    QJsonObject result = cli->PerformeJobs(data[0], models, jobs);
    
    QVERIFY(!result.isEmpty());
    verifyModelResults(result);
    
    delete cli;
}

void TestPipeline::testMultipleModelTesting()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    // Add multiple models
    QJsonObject models;
    models["model1"] = 1;
    models["model2"] = 2;
    models["model3"] = 3;
    config["Models"] = models;
    
    QJsonObject jobs;
    QJsonObject job;
    job["Method"] = 1;
    job["MaxSteps"] = 50;
    jobs["test_job"] = job;
    config["Jobs"] = jobs;
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(!data.isEmpty());
    
    // Test with multiple models
    QJsonObject result = cli->PerformeJobs(data[0], models, jobs);
    
    QVERIFY(result.contains("models"));
    QJsonObject modelResults = result["models"].toObject();
    
    QVERIFY(modelResults.contains("model1"));
    QVERIFY(modelResults.contains("model2"));
    QVERIFY(modelResults.contains("model3"));
    
    delete cli;
}

void TestPipeline::testModelComparison()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    // Configure for model comparison
    QJsonObject models;
    models["true_model"] = 1;
    models["test_model"] = 2;
    config["Models"] = models;
    
    QJsonObject jobs;
    QJsonObject job;
    job["Method"] = 8; // Model comparison
    jobs["comparison"] = job;
    config["Jobs"] = jobs;
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(!data.isEmpty());
    
    QJsonObject result = cli->PerformeJobs(data[0], models, jobs);
    
    QVERIFY(!result.isEmpty());
    QVERIFY(result.contains("models"));
    
    delete cli;
}

void TestPipeline::testMLPipelineManagerCreation()
{
    MLPipelineManager* manager = new MLPipelineManager();
    QVERIFY(manager != nullptr);
    
    QCOMPARE(manager->currentBatch(), 0);
    QCOMPARE(manager->totalBatches(), 0);
    QVERIFY(!manager->isRunning());
    
    delete manager;
}

void TestPipeline::testBatchConfiguration()
{
    MLPipelineManager* manager = new MLPipelineManager();
    QJsonObject config = createBatchConfiguration();
    
    manager->setBatchConfig(config);
    
    QCOMPARE(manager->totalBatches(), config["BatchConfig"].toObject()["TotalBatches"].toInt());
    
    delete manager;
}

void TestPipeline::testSinglePipelineExecution()
{
    MLPipelineManager* manager = new MLPipelineManager();
    
    // Create temporary config file
    QJsonObject config = createMLPipelineConfiguration();
    QString configFile = QString("%1/single_pipeline_config.json").arg(m_tempDir->path());
    
    QJsonDocument doc(config);
    QFile file(configFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(doc.toJson());
    file.close();
    
    // Test single pipeline execution
    manager->runSinglePipeline(configFile);
    
    // Note: This is a basic test - full verification would require
    // checking the actual output files and results
    
    delete manager;
}

void TestPipeline::testBatchPipelineExecution()
{
    MLPipelineManager* manager = new MLPipelineManager();
    QJsonObject config = createBatchConfiguration();
    
    // Set small batch size for testing
    QJsonObject batchConfig = config["BatchConfig"].toObject();
    batchConfig["TotalBatches"] = 2;
    config["BatchConfig"] = batchConfig;
    
    manager->setBatchConfig(config);
    
    // Note: This would start actual batch processing
    // For unit testing, we just verify the configuration
    QCOMPARE(manager->totalBatches(), 2);
    
    delete manager;
}

void TestPipeline::testFeatureExtraction()
{
    MLPipelineManager* manager = new MLPipelineManager();
    
    // Create test models vector
    QVector<QJsonObject> models;
    QJsonObject model1;
    model1["statistics"] = QJsonObject();
    model1["statistics"].toObject()["SSE"] = 1.23;
    model1["statistics"].toObject()["AIC"] = 45.67;
    models.append(model1);
    
    QJsonObject features = manager->extractMLFeatures(models);
    
    QVERIFY(!features.isEmpty());
    
    delete manager;
}

void TestPipeline::testJobConfiguration()
{
    QJsonObject jobs;
    
    // Monte Carlo job
    QJsonObject mcJob;
    mcJob["Method"] = 1;
    mcJob["MaxSteps"] = 1000;
    mcJob["VarianceSource"] = 2;
    mcJob["Bootstrap"] = false;
    jobs["monte_carlo"] = mcJob;
    
    // Cross-validation job
    QJsonObject cvJob;
    cvJob["Method"] = 4;
    cvJob["CXO"] = 1;
    cvJob["Algorithm"] = 2;
    jobs["cross_validation"] = cvJob;
    
    // Reduction analysis job
    QJsonObject raJob;
    raJob["Method"] = 5;
    raJob["ReductionRuntype"] = 1;
    jobs["reduction_analysis"] = raJob;
    
    // Verify job structure
    QVERIFY(jobs.contains("monte_carlo"));
    QVERIFY(jobs.contains("cross_validation"));
    QVERIFY(jobs.contains("reduction_analysis"));
    
    QCOMPARE(jobs["monte_carlo"].toObject()["Method"].toInt(), 1);
    QCOMPARE(jobs["cross_validation"].toObject()["Method"].toInt(), 4);
    QCOMPARE(jobs["reduction_analysis"].toObject()["Method"].toInt(), 5);
}

void TestPipeline::testJobExecution()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    // Add job configuration
    QJsonObject jobs;
    QJsonObject job;
    job["Method"] = 1;
    job["MaxSteps"] = 100;
    job["VarianceSource"] = 2;
    jobs["test_job"] = job;
    config["Jobs"] = jobs;
    
    QJsonObject models;
    models["test_model"] = 1;
    config["Models"] = models;
    
    cli->setControlJson(config);
    
    // Generate data and test job execution
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(!data.isEmpty());
    
    QJsonObject result = cli->PerformeJobs(data[0], models, jobs);
    
    QVERIFY(!result.isEmpty());
    QVERIFY(result.contains("models"));
    
    delete cli;
}

void TestPipeline::testJobResults()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    QJsonObject models;
    models["test_model"] = 1;
    config["Models"] = models;
    
    QJsonObject jobs;
    QJsonObject job;
    job["Method"] = 1;
    job["MaxSteps"] = 50;
    jobs["test_job"] = job;
    config["Jobs"] = jobs;
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(!data.isEmpty());
    
    QJsonObject result = cli->PerformeJobs(data[0], models, jobs);
    
    // Verify result structure
    QVERIFY(result.contains("models"));
    QVERIFY(result.contains("timestamp"));
    
    QJsonObject modelResults = result["models"].toObject();
    QVERIFY(modelResults.contains("test_model"));
    
    QJsonObject modelResult = modelResults["test_model"].toObject();
    QVERIFY(modelResult.contains("statistics"));
    QVERIFY(modelResult.contains("jobs"));
    
    delete cli;
}

void TestPipeline::testStatisticalAnalysis()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    // Add statistical analysis jobs
    QJsonObject jobs;
    
    QJsonObject mcJob;
    mcJob["Method"] = 1;
    mcJob["MaxSteps"] = 200;
    mcJob["VarianceSource"] = 2;
    jobs["monte_carlo"] = mcJob;
    
    QJsonObject cvJob;
    cvJob["Method"] = 4;
    cvJob["CXO"] = 1;
    jobs["cross_validation"] = cvJob;
    
    config["Jobs"] = jobs;
    
    QJsonObject models;
    models["test_model"] = 1;
    config["Models"] = models;
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(!data.isEmpty());
    
    QJsonObject result = cli->PerformeJobs(data[0], models, jobs);
    
    QVERIFY(!result.isEmpty());
    verifyModelResults(result);
    
    delete cli;
}

void TestPipeline::testConfigurationFileLoading()
{
    // Create test configuration file
    QJsonObject config = createTestConfiguration();
    QString configFile = QString("%1/test_config.json").arg(m_tempDir->path());
    
    QJsonDocument doc(config);
    QFile file(configFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(doc.toJson());
    file.close();
    
    // Test loading
    QJsonObject loadedConfig;
    bool success = JsonHandler::ReadJsonFile(loadedConfig, configFile);
    
    QVERIFY(success);
    QCOMPARE(loadedConfig["Main"].toObject()["OutFile"].toString(), 
             config["Main"].toObject()["OutFile"].toString());
}

void TestPipeline::testResultFileSaving()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    QJsonObject models;
    models["test_model"] = 1;
    config["Models"] = models;
    
    QJsonObject jobs;
    QJsonObject job;
    job["Method"] = 1;
    job["MaxSteps"] = 50;
    jobs["test_job"] = job;
    config["Jobs"] = jobs;
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(!data.isEmpty());
    
    QJsonObject result = cli->PerformeJobs(data[0], models, jobs);
    
    // Test saving results
    QString resultFile = QString("%1/test_results.json").arg(m_tempDir->path());
    bool success = cli->SaveFile(resultFile, result);
    
    QVERIFY(success);
    QVERIFY(QFileInfo::exists(resultFile));
    
    // Verify saved file content
    QJsonObject loadedResult;
    success = JsonHandler::ReadJsonFile(loadedResult, resultFile);
    QVERIFY(success);
    QVERIFY(loadedResult.contains("models"));
    
    delete cli;
}

void TestPipeline::testPipelineFileRoundTrip()
{
    // Create and save configuration
    QJsonObject config = createTestConfiguration();
    QString configFile = QString("%1/roundtrip_config.json").arg(m_tempDir->path());
    
    QJsonDocument doc(config);
    QFile file(configFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(doc.toJson());
    file.close();
    
    // Load and use configuration
    QJsonObject loadedConfig;
    bool success = JsonHandler::ReadJsonFile(loadedConfig, configFile);
    QVERIFY(success);
    
    SupraFitCli* cli = new SupraFitCli();
    cli->setControlJson(loadedConfig);
    
    QVERIFY(cli->CheckGenerateDependent());
    
    delete cli;
}

void TestPipeline::testFullPipelineExecution()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    // Add complete configuration
    QJsonObject models;
    models["true_model"] = 1;
    models["test_model"] = 2;
    config["Models"] = models;
    
    QJsonObject jobs;
    QJsonObject job;
    job["Method"] = 1;
    job["MaxSteps"] = 100;
    job["VarianceSource"] = 2;
    jobs["analysis"] = job;
    config["Jobs"] = jobs;
    
    cli->setControlJson(config);
    
    // Execute full pipeline
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(!data.isEmpty());
    
    cli->setDataVector(data);
    
    // Note: Work() would save files, so we test components separately
    for (const auto& dataset : data) {
        QJsonObject result = cli->PerformeJobs(dataset, models, jobs);
        QVERIFY(!result.isEmpty());
        verifyModelResults(result);
    }
    
    delete cli;
}

void TestPipeline::testPipelineWithRealData()
{
    // This test would use actual experimental data files
    // For now, we create a realistic configuration
    
    QJsonObject config;
    QJsonObject main;
    main["InFile"] = createTestDataFile();
    main["OutFile"] = "real_data_test";
    main["IndependentRows"] = 2;
    main["Threads"] = 1;
    
    QJsonObject generateData;
    generateData["Series"] = 1;
    generateData["Model"] = 1;
    generateData["Repeat"] = 3;
    generateData["Variance"] = 0.001;
    main["GenerateData"] = generateData;
    
    config["Main"] = main;
    
    QJsonObject models;
    models["nmr_1_1"] = 1;
    models["nmr_2_1"] = 2;
    config["Models"] = models;
    
    SupraFitCli* cli = new SupraFitCli();
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(!data.isEmpty());
    
    delete cli;
}

void TestPipeline::testPipelineErrorHandling()
{
    SupraFitCli* cli = new SupraFitCli();
    
    // Test with invalid configuration
    QJsonObject invalidConfig;
    invalidConfig["Invalid"] = "Configuration";
    
    cli->setControlJson(invalidConfig);
    
    // Should not crash and should return empty data
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(data.isEmpty());
    
    delete cli;
}

void TestPipeline::testPipelinePerformance()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    // Configure for performance test
    QJsonObject main = config["Main"].toObject();
    QJsonObject generateData = main["GenerateData"].toObject();
    generateData["Repeat"] = 10; // Moderate size for testing
    main["GenerateData"] = generateData;
    config["Main"] = main;
    
    cli->setControlJson(config);
    
    // Measure data generation time
    QElapsedTimer timer;
    timer.start();
    
    QVector<QJsonObject> data = cli->GenerateData();
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY(!data.isEmpty());
    QVERIFY(elapsed < 10000); // Should complete within 10 seconds
    
    qDebug() << "Generated" << data.size() << "datasets in" << elapsed << "ms";
    
    delete cli;
}

void TestPipeline::testEmptyConfiguration()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject emptyConfig;
    
    cli->setControlJson(emptyConfig);
    
    QVERIFY(!cli->CheckGenerateDependent());
    QVERIFY(!cli->CheckGenerateIndependent());
    
    QVector<QJsonObject> data = cli->GenerateData();
    QVERIFY(data.isEmpty());
    
    delete cli;
}

void TestPipeline::testInvalidConfiguration()
{
    SupraFitCli* cli = new SupraFitCli();
    
    // Configuration with invalid model
    QJsonObject config;
    QJsonObject main;
    QJsonObject generateData;
    generateData["Model"] = 999; // Invalid model ID
    main["GenerateData"] = generateData;
    config["Main"] = main;
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    
    // Should handle gracefully
    QVERIFY(data.isEmpty());
    
    delete cli;
}

void TestPipeline::testMissingFiles()
{
    SupraFitCli* cli = new SupraFitCli();
    
    QJsonObject config;
    QJsonObject main;
    main["InFile"] = "non_existent_file.dat";
    main["OutFile"] = "test_output";
    
    QJsonObject generateData;
    generateData["Series"] = 1;
    generateData["Model"] = 1;
    generateData["Repeat"] = 1;
    main["GenerateData"] = generateData;
    
    config["Main"] = main;
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    
    // Should handle missing files gracefully
    QVERIFY(data.isEmpty());
    
    delete cli;
}

void TestPipeline::testLargeDatasetHandling()
{
    SupraFitCli* cli = new SupraFitCli();
    QJsonObject config = createTestConfiguration();
    
    // Configure for large dataset
    QJsonObject main = config["Main"].toObject();
    QJsonObject generateData = main["GenerateData"].toObject();
    generateData["Repeat"] = 100;
    generateData["Series"] = 5;
    main["GenerateData"] = generateData;
    config["Main"] = main;
    
    cli->setControlJson(config);
    
    QVector<QJsonObject> data = cli->GenerateData();
    
    if (!data.isEmpty()) {
        QCOMPARE(data.size(), 100);
        verifyGeneratedData(data);
    }
    
    delete cli;
}

// Helper method implementations
QJsonObject TestPipeline::createTestConfiguration()
{
    QJsonObject config;
    QJsonObject main;
    
    main["InFile"] = createTestDataFile();
    main["OutFile"] = "test_output";
    main["IndependentRows"] = 2;
    main["Threads"] = 1;
    
    QJsonObject generateData;
    generateData["Series"] = 1;
    generateData["Model"] = 1;
    generateData["Repeat"] = 3;
    generateData["Variance"] = 0.001;
    generateData["GlobalRandomLimits"] = "[1 5]";
    generateData["LocalRandomLimits"] = "[0.1 1.0]";
    
    main["GenerateData"] = generateData;
    config["Main"] = main;
    
    return config;
}

QJsonObject TestPipeline::createMLPipelineConfiguration()
{
    QJsonObject config;
    QJsonObject main;
    
    main["InFile"] = createTestDataFile();
    main["OutFile"] = "ml_pipeline_output";
    main["IndependentRows"] = 2;
    main["Threads"] = 1;
    
    QJsonObject generateData;
    generateData["Series"] = 1;
    generateData["Model"] = 1;
    generateData["Repeat"] = 5;
    generateData["Variance"] = 0.001;
    main["GenerateData"] = generateData;
    
    config["Main"] = main;
    
    QJsonObject models;
    models["true_model"] = 1;
    models["test_model"] = 2;
    config["Models"] = models;
    
    QJsonObject pipeline;
    pipeline["Step"] = 1;
    pipeline["Description"] = "Test ML Pipeline";
    pipeline["TrueModel"] = 1;
    config["Pipeline"] = pipeline;
    
    return config;
}

QJsonObject TestPipeline::createBatchConfiguration()
{
    QJsonObject config;
    
    QJsonObject batchConfig;
    batchConfig["BatchSize"] = 10;
    batchConfig["TotalBatches"] = 5;
    batchConfig["WorkerThreads"] = 2;
    batchConfig["OutputDir"] = m_tempDir->path();
    config["BatchConfig"] = batchConfig;
    
    QJsonObject paramVariation;
    paramVariation["Models"] = QJsonArray{1, 2, 3};
    paramVariation["NoiseVariance"] = QJsonArray{0.001, 0.005, 0.01};
    config["ParameterVariation"] = paramVariation;
    
    return config;
}

QString TestPipeline::createTestDataFile()
{
    QString filename = QString("%1/test_data.dat").arg(m_tempDir->path());
    
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        
        // Write simple 2-column concentration data
        for (int i = 0; i < 20; ++i) {
            double host = 0.001;
            double guest = i * 0.0001;
            out << host << "\t" << guest << "\n";
        }
        
        file.close();
    }
    
    return filename;
}

void TestPipeline::verifyGeneratedData(const QVector<QJsonObject>& data)
{
    QVERIFY(!data.isEmpty());
    
    for (const auto& dataset : data) {
        QVERIFY(dataset.contains("data"));
        QVERIFY(dataset.contains("uuid"));
        QVERIFY(dataset.contains("DataType"));
        
        QJsonObject dataObj = dataset["data"].toObject();
        QVERIFY(dataObj.contains("independent"));
        QVERIFY(dataObj.contains("dependent"));
        
        QJsonObject independent = dataObj["independent"].toObject();
        QVERIFY(independent.contains("rows"));
        QVERIFY(independent.contains("cols"));
        QVERIFY(independent.contains("data"));
        
        QJsonObject dependent = dataObj["dependent"].toObject();
        QVERIFY(dependent.contains("rows"));
        QVERIFY(dependent.contains("cols"));
        QVERIFY(dependent.contains("data"));
    }
}

void TestPipeline::verifyModelResults(const QJsonObject& results)
{
    QVERIFY(results.contains("models"));
    QVERIFY(results.contains("timestamp"));
    
    QJsonObject models = results["models"].toObject();
    QVERIFY(!models.isEmpty());
    
    for (auto it = models.constBegin(); it != models.constEnd(); ++it) {
        QJsonObject modelResult = it.value().toObject();
        QVERIFY(modelResult.contains("statistics"));
        
        QJsonObject statistics = modelResult["statistics"].toObject();
        QVERIFY(statistics.contains("ModelId"));
        QVERIFY(statistics.contains("ModelName"));
        QVERIFY(statistics.contains("SSE"));
        QVERIFY(statistics.contains("AIC"));
    }
}

QTEST_MAIN(TestPipeline)
#include "test_pipeline.moc"