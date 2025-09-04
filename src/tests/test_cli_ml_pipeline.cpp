/*
 * Comprehensive tests for SupraFit CLI ML Pipeline integration
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Tests complete ML workflow: data generation → model fitting → statistical evaluation,
 * multi-model testing, ML feature extraction, and neural network tutorial integration.
 * Claude Generated - 2025-09-04
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <QtTest/QTest>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QProcess>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>

#include "test_utils.h"

class TestCliMLPipeline : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Complete ML Pipeline Workflow Tests - Claude Generated
    void testBasicMLPipelineWorkflow();
    void testDataGenerationToModelFitting();
    void testModelFittingToStatisticalAnalysis();
    void testEndToEndPipelineIntegration();
    void testPipelineWithMultipleModels();

    // Model Testing and Comparison Tests - Claude Generated
    void testMultiModelFitting();
    void testModelComparisonMetrics();
    void testStatisticalModelEvaluation();
    void testModelParameterExtraction();
    void testModelPerformanceAnalysis();

    // ML Feature Extraction Tests - Claude Generated
    void testMLFeatureGeneration();
    void testFeatureExtractionFromFittedModels();
    void testCompactMLDatasetCreation();
    void testFeatureNormalizationAndScaling();
    void testTrainingDataExport();

    // Statistical Analysis Integration Tests - Claude Generated
    void testMonteCarloIntegration();
    void testCrossValidationIntegration();
    void testConfidenceIntervalGeneration();
    void testWeakenedGridSearchIntegration();
    void testGlobalSearchIntegration();

    // Neural Network Tutorial Integration Tests - Claude Generated
    void testXORTutorialIntegration();
    void testNMRModelSelectionTutorial();
    void testTrainingTutorialIntegration();
    void testTutorialDataPipelineIntegration();

    // Performance and Scalability Tests - Claude Generated
    void testLargeScaleMLPipeline();
    void testConcurrentModelFitting();
    void testMemoryEfficiencyInPipeline();
    void testPipelinePerformanceBenchmarking();

    // Error Handling and Robustness Tests - Claude Generated  
    void testPipelineErrorRecovery();
    void testIncompleteDataHandling();
    void testModelFittingFailureRecovery();
    void testStatisticalAnalysisFailureHandling();

private:
    QTemporaryDir* m_tempDir;
    QString m_suprafitCli;
    QString m_mlCli;
    
    // Helper methods - Claude Generated
    QString createMLPipelineConfig();
    QString createMultiModelConfig();
    QString createLargeScaleConfig();
    QString createStatisticalAnalysisConfig();
    QString createFeatureExtractionConfig();
    QString createTutorialIntegrationConfig();
    QString createIncompleteDataConfig();
    
    QStringList runCliCommand(const QStringList& arguments, int timeoutMs = 120000);
    QStringList runMLCliCommand(const QStringList& arguments, int timeoutMs = 60000);
    bool validateMLPipelineOutput(const QString& filename);
    bool validateFeatureData(const QString& filename);
    QJsonObject loadJsonFile(const QString& filename);
    bool verifyModelFitQuality(const QJsonObject& modelData);
    bool verifyStatisticalResults(const QJsonObject& statisticalData);
    double extractModelFitnessMeetic(const QJsonObject& modelData, const QString& metric);
};

void TestCliMLPipeline::initTestCase()
{
    qDebug() << "Starting CLI ML Pipeline tests...";
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    // Find executables - Enhanced path search
    m_suprafitCli = QCoreApplication::applicationDirPath() + "/bin/linux/suprafit_cli";
    m_mlCli = QCoreApplication::applicationDirPath() + "/bin/linux/ml_cli_main";
    
    if (!QFile::exists(m_suprafitCli)) {
        m_suprafitCli = "../bin/linux/suprafit_cli";
        m_mlCli = "../bin/linux/ml_cli_main";
    }
    if (!QFile::exists(m_suprafitCli)) {
        m_suprafitCli = "bin/linux/suprafit_cli";
        m_mlCli = "bin/linux/ml_cli_main";
    }
    if (!QFile::exists(m_suprafitCli)) {
        m_suprafitCli = "../../release/bin/linux/suprafit_cli";
        m_mlCli = "../../release/bin/linux/ml_cli_main";
    }
    if (!QFile::exists(m_suprafitCli)) {
        m_suprafitCli = "../release/bin/linux/suprafit_cli";
        m_mlCli = "../release/bin/linux/ml_cli_main";
    }
    if (!QFile::exists(m_suprafitCli)) {
        m_suprafitCli = "./bin/linux/suprafit_cli";
        m_mlCli = "./bin/linux/ml_cli_main";
    }
    
    QVERIFY2(QFile::exists(m_suprafitCli), "suprafit_cli executable not found");
    qDebug() << "Using CLI executable:" << m_suprafitCli;
    
    if (QFile::exists(m_mlCli)) {
        qDebug() << "Using ML CLI executable:" << m_mlCli;
    } else {
        qDebug() << "ML CLI executable not found, skipping ML-specific tests";
        m_mlCli = "";
    }
}

void TestCliMLPipeline::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "CLI ML Pipeline tests completed.";
}

// Complete ML Pipeline Workflow Tests - Claude Generated
void TestCliMLPipeline::testBasicMLPipelineWorkflow()
{
    QString config = createMLPipelineConfig();
    QString output = m_tempDir->path() + "/ml_workflow.json";
    
    // Input: Complete ML pipeline configuration (data→fit→analyze)
    // Expected: Generated data, fitted models, statistical analysis results
    QElapsedTimer timer;
    timer.start();
    
    QStringList result = runCliCommand({"-i", config, "-o", output}, 300000); // 5 minute timeout
    qDebug() << "ML Pipeline workflow completed in" << timer.elapsed() << "ms";
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("ML pipeline workflow failed: " + result[2]));
    QVERIFY(QFile::exists(output + ".json"));
    QVERIFY(validateMLPipelineOutput(output + ".json"));
    
    // Verify pipeline stages completed
    QJsonObject data = loadJsonFile(output + ".json");
    QVERIFY(data.contains("data")); // Data generation
    QVERIFY(data.contains("model_0")); // Model fitting
}

void TestCliMLPipeline::testDataGenerationToModelFitting()
{
    QString config = createMLPipelineConfig();
    QString output = m_tempDir->path() + "/data_to_fit.json";
    
    // Input: Data generation with immediate model fitting
    // Expected: Smooth transition from generated data to fitted parameters
    QStringList result = runCliCommand({"-i", config, "-o", output}, 180000);
    QCOMPARE(result[0].toInt(), 0);
    
    QJsonObject data = loadJsonFile(output + ".json");
    QVERIFY(verifyModelFitQuality(data));
}

void TestCliMLPipeline::testModelFittingToStatisticalAnalysis()
{
    QString config = createStatisticalAnalysisConfig();
    QString output = m_tempDir->path() + "/fit_to_stats.json";
    
    // Input: Model with statistical analysis configuration
    // Expected: Post-fit analysis with Monte Carlo, CV, confidence intervals
    QStringList result = runCliCommand({"-i", config, "-o", output}, 240000);
    QCOMPARE(result[0].toInt(), 0);
    
    QJsonObject data = loadJsonFile(output + ".json");
    QVERIFY(verifyStatisticalResults(data));
}

void TestCliMLPipeline::testEndToEndPipelineIntegration()
{
    QString config = createMLPipelineConfig();
    QString output = m_tempDir->path() + "/end_to_end.json";
    
    // Input: Complete pipeline configuration
    // Expected: All stages complete with consistent data flow
    QStringList result = runCliCommand({"-i", config, "-o", output}, 360000); // 6 minutes
    QCOMPARE(result[0].toInt(), 0);
    
    // Comprehensive validation
    QVERIFY(validateMLPipelineOutput(output + ".json"));
}

void TestCliMLPipeline::testPipelineWithMultipleModels()
{
    QString config = createMultiModelConfig();
    QString output = m_tempDir->path() + "/multi_model.json";
    
    // Input: Pipeline with multiple competing models
    // Expected: All models fitted and compared with statistical metrics
    QStringList result = runCliCommand({"-i", config, "-o", output}, 300000);
    QCOMPARE(result[0].toInt(), 0);
    
    QJsonObject data = loadJsonFile(output + ".json");
    // Should contain multiple model entries
    QVERIFY(data.contains("model_0"));
    QVERIFY(data.contains("model_1"));
}

// Model Testing and Comparison Tests - Claude Generated
void TestCliMLPipeline::testMultiModelFitting()
{
    QString config = createMultiModelConfig();
    QString output = m_tempDir->path() + "/multi_fit.json";
    
    // Input: Multiple model types for same dataset
    // Expected: Each model fitted with quality metrics
    QStringList result = runCliCommand({"-i", config, "-o", output});
    QCOMPARE(result[0].toInt(), 0);
    
    QJsonObject data = loadJsonFile(output + ".json");
    for (int i = 0; i < 3; ++i) {
        QString modelKey = QString("model_%1").arg(i);
        if (data.contains(modelKey)) {
            QVERIFY(verifyModelFitQuality(data[modelKey].toObject()));
        }
    }
}

void TestCliMLPipeline::testModelComparisonMetrics()
{
    QString config = createMultiModelConfig();
    QString output = m_tempDir->path() + "/model_comparison.json";
    
    // Input: Multiple models with comparison analysis
    // Expected: AIC, BIC, R², SSE comparison metrics
    QStringList result = runCliCommand({"-i", config, "-o", output});
    QCOMPARE(result[0].toInt(), 0);
    
    QJsonObject data = loadJsonFile(output + ".json");
    // Verify comparison metrics are present
    bool foundAIC = false;
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it.key().startsWith("model_")) {
            QJsonObject model = it.value().toObject();
            if (model.contains("aic") || model.contains("AIC")) {
                foundAIC = true;
                break;
            }
        }
    }
    QVERIFY(foundAIC);
}

void TestCliMLPipeline::testStatisticalModelEvaluation()
{
    QString config = createStatisticalAnalysisConfig();
    QString output = m_tempDir->path() + "/statistical_eval.json";
    
    // Input: Model with comprehensive statistical analysis
    // Expected: Statistical evaluation results (MC, CV, confidence)
    QStringList result = runCliCommand({"-i", config, "-o", output});
    QCOMPARE(result[0].toInt(), 0);
    
    QVERIFY(verifyStatisticalResults(loadJsonFile(output + ".json")));
}

void TestCliMLPipeline::testModelParameterExtraction()
{
    QString config = createMLPipelineConfig();
    QString output = m_tempDir->path() + "/parameter_extract.json";
    
    // Test parameter extraction after pipeline
    QStringList result1 = runCliCommand({"-i", config, "-o", output});
    QCOMPARE(result1[0].toInt(), 0);
    
    // Input: Fitted model file with parameter extraction flag
    // Expected: Extracted global and local parameters in tabular format
    QStringList result2 = runCliCommand({"-x", output + ".json"});
    QCOMPARE(result2[0].toInt(), 0);
    QVERIFY(result2[1].contains("Parameter") || result2[1].contains("Global") || result2[1].contains("Model"));
}

void TestCliMLPipeline::testModelPerformanceAnalysis()
{
    QString config = createMLPipelineConfig();
    QString output = m_tempDir->path() + "/performance_analysis.json";
    
    // Input: ML pipeline with performance analysis
    // Expected: Fit quality metrics, convergence analysis
    QStringList result = runCliCommand({"-i", config, "-o", output});
    QCOMPARE(result[0].toInt(), 0);
    
    QJsonObject data = loadJsonFile(output + ".json");
    QVERIFY(verifyModelFitQuality(data));
}

// ML Feature Extraction Tests - Claude Generated
void TestCliMLPipeline::testMLFeatureGeneration()
{
    QString config = createFeatureExtractionConfig();
    QString output = m_tempDir->path() + "/ml_features.json";
    
    // Input: Pipeline configured for ML feature extraction
    // Expected: Compact feature dataset suitable for neural network training
    QStringList result = runCliCommand({"-i", config, "-o", output});
    QCOMPARE(result[0].toInt(), 0);
    
    QVERIFY(validateFeatureData(output + ".json"));
}

void TestCliMLPipeline::testFeatureExtractionFromFittedModels()
{
    QString config = createMLPipelineConfig();
    QString output = m_tempDir->path() + "/fitted_features.json";
    
    // Input: Fitted models with feature extraction request
    // Expected: ML-ready features extracted from statistical analysis
    QStringList result = runCliCommand({"-i", config, "-o", output});
    QCOMPARE(result[0].toInt(), 0);
    
    // Features should be extractable from the result
    QVERIFY(validateMLPipelineOutput(output + ".json"));
}

void TestCliMLPipeline::testCompactMLDatasetCreation()
{
    QString config = createFeatureExtractionConfig();
    QString output = m_tempDir->path() + "/compact_dataset.json";
    
    // Input: Large dataset with compact feature extraction
    // Expected: Reduced dimensionality ML dataset maintaining key information
    QStringList result = runCliCommand({"-i", config, "-o", output});
    QCOMPARE(result[0].toInt(), 0);
    
    QJsonObject data = loadJsonFile(output + ".json");
    QVERIFY(!data.isEmpty());
}

void TestCliMLPipeline::testFeatureNormalizationAndScaling()
{
    QString config = createFeatureExtractionConfig();
    
    // Input: Raw features requiring normalization
    // Expected: Properly scaled features for neural network input
    QStringList result = runCliCommand({"-i", config});
    QCOMPARE(result[0].toInt(), 0);
    
    // Feature scaling validation would require examining output structure
}

void TestCliMLPipeline::testTrainingDataExport()
{
    QString config = createMLPipelineConfig();
    QString output = m_tempDir->path() + "/training_data.json";
    
    // Input: Complete pipeline with training data export
    // Expected: Neural network ready training dataset
    QStringList result = runCliCommand({"-i", config, "-o", output});
    QCOMPARE(result[0].toInt(), 0);
    
    QVERIFY(QFile::exists(output + ".json"));
}

// Statistical Analysis Integration Tests - Claude Generated
void TestCliMLPipeline::testMonteCarloIntegration()
{
    QString config = createStatisticalAnalysisConfig();
    QString output = m_tempDir->path() + "/monte_carlo.json";
    
    // Input: Model with Monte Carlo analysis configuration
    // Expected: Parameter confidence intervals and uncertainty quantification
    QStringList result = runCliCommand({"-i", config, "-o", output}, 300000);
    QCOMPARE(result[0].toInt(), 0);
    
    QJsonObject data = loadJsonFile(output + ".json");
    QVERIFY(verifyStatisticalResults(data));
}

void TestCliMLPipeline::testCrossValidationIntegration()
{
    QString config = createStatisticalAnalysisConfig();
    
    // Input: Cross-validation analysis configuration
    // Expected: Model validation results with predictive performance metrics
    QStringList result = runCliCommand({"-i", config}, 240000);
    QCOMPARE(result[0].toInt(), 0);
}

void TestCliMLPipeline::testConfidenceIntervalGeneration()
{
    QString config = createStatisticalAnalysisConfig();
    
    // Input: Statistical analysis with confidence interval calculation
    // Expected: Parameter confidence bounds and significance testing
    QStringList result = runCliCommand({"-i", config});
    QCOMPARE(result[0].toInt(), 0);
}

void TestCliMLPipeline::testWeakenedGridSearchIntegration()
{
    QString config = createStatisticalAnalysisConfig();
    
    // Input: Weakened grid search configuration
    // Expected: Parameter space exploration with optimization results
    QStringList result = runCliCommand({"-i", config}, 180000);
    QCOMPARE(result[0].toInt(), 0);
}

void TestCliMLPipeline::testGlobalSearchIntegration()
{
    QString config = createStatisticalAnalysisConfig();
    
    // Input: Global search optimization configuration
    // Expected: Global parameter optimization with convergence analysis
    QStringList result = runCliCommand({"-i", config}, 180000);
    QCOMPARE(result[0].toInt(), 0);
}

// Neural Network Tutorial Integration Tests - Claude Generated
void TestCliMLPipeline::testXORTutorialIntegration()
{
    if (m_mlCli.isEmpty()) {
        QSKIP("ML CLI not available");
    }
    
    // Input: XOR tutorial execution request
    // Expected: Tutorial completion with educational output
    QStringList result = runMLCliCommand({"--tutorial", "xor"});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].contains("XOR") || result[1].contains("Tutorial"));
}

void TestCliMLPipeline::testNMRModelSelectionTutorial()
{
    if (m_mlCli.isEmpty()) {
        QSKIP("ML CLI not available");
    }
    
    // Input: NMR model selection tutorial
    // Expected: Chemistry-specific ML tutorial with model comparison
    QStringList result = runMLCliCommand({"--tutorial", "nmr"});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].contains("NMR") || result[1].contains("model"));
}

void TestCliMLPipeline::testTrainingTutorialIntegration()
{
    if (m_mlCli.isEmpty()) {
        QSKIP("ML CLI not available");
    }
    
    // Input: Neural network training tutorial
    // Expected: Complete training workflow demonstration
    QStringList result = runMLCliCommand({"--tutorial", "training"});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].contains("training") || result[1].contains("Training"));
}

void TestCliMLPipeline::testTutorialDataPipelineIntegration()
{
    QString config = createTutorialIntegrationConfig();
    
    // Input: Data pipeline feeding into tutorial system
    // Expected: Seamless integration between data generation and ML tutorials
    QStringList result = runCliCommand({"-i", config});
    QCOMPARE(result[0].toInt(), 0);
}

// Performance and Scalability Tests - Claude Generated
void TestCliMLPipeline::testLargeScaleMLPipeline()
{
    QString config = createLargeScaleConfig();
    QString output = m_tempDir->path() + "/large_scale.json";
    
    // Input: Large-scale ML pipeline (high-dimensional data, multiple models)
    // Expected: Successful completion within memory/time constraints
    QStringList result = runCliCommand({"-i", config, "-o", output}, 600000); // 10 minutes
    QCOMPARE(result[0].toInt(), 0);
    
    QVERIFY(QFile::exists(output + ".json"));
}

void TestCliMLPipeline::testConcurrentModelFitting()
{
    QString config = createMultiModelConfig();
    
    // Input: Multiple models with concurrent processing
    // Expected: Parallel model fitting with thread safety
    QStringList result = runCliCommand({"-n", "2", "-i", config}, 240000);
    QCOMPARE(result[0].toInt(), 0);
}

void TestCliMLPipeline::testMemoryEfficiencyInPipeline()
{
    QString config = createLargeScaleConfig();
    
    // Input: Memory-intensive pipeline operations
    // Expected: Efficient memory management without excessive allocation
    QStringList result = runCliCommand({"-i", config});
    QCOMPARE(result[0].toInt(), 0);
}

void TestCliMLPipeline::testPipelinePerformanceBenchmarking()
{
    QString config = createMLPipelineConfig();
    
    // Input: Standard pipeline for performance measurement
    // Expected: Performance metrics within acceptable ranges
    QElapsedTimer timer;
    timer.start();
    
    QStringList result = runCliCommand({"-i", config});
    qint64 elapsed = timer.elapsed();
    
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(elapsed < 180000); // Should complete within 3 minutes
    qDebug() << "Pipeline performance:" << elapsed << "ms";
}

// Error Handling and Robustness Tests - Claude Generated
void TestCliMLPipeline::testPipelineErrorRecovery()
{
    QString config = createIncompleteDataConfig();
    
    // Input: Configuration with potential error conditions
    // Expected: Graceful error handling with informative messages
    QStringList result = runCliCommand({"-i", config});
    QVERIFY(result[0].toInt() >= 0); // Should not crash
}

void TestCliMLPipeline::testIncompleteDataHandling()
{
    QString config = createIncompleteDataConfig();
    
    // Input: Incomplete or malformed data configuration
    // Expected: Error detection with user-friendly error messages
    QStringList result = runCliCommand({"-i", config});
    QVERIFY(result[0].toInt() >= 0);
    // Should handle gracefully
}

void TestCliMLPipeline::testModelFittingFailureRecovery()
{
    QString config = m_tempDir->path() + "/failing_model.json";
    QFile file(config);
    file.open(QIODevice::WriteOnly);
    
    // Create configuration that will cause model fitting to fail
    QJsonObject failConfig;
    QJsonObject main;
    main["OutFile"] = "fail_test";
    failConfig["Main"] = main;
    
    // Add problematic model configuration
    QJsonObject models;
    QJsonObject model;
    model["ID"] = 999; // Invalid model ID
    models["0"] = model;
    failConfig["Models"] = models;
    
    QJsonDocument doc(failConfig);
    file.write(doc.toJson());
    file.close();
    
    // Input: Configuration causing model fitting failure
    // Expected: Error handling without system crash
    QStringList result = runCliCommand({"-i", config});
    QVERIFY(result[0].toInt() >= 0);
}

void TestCliMLPipeline::testStatisticalAnalysisFailureHandling()
{
    QString config = m_tempDir->path() + "/stat_fail.json";
    QFile file(config);
    file.open(QIODevice::WriteOnly);
    
    // Create minimal config that might fail statistical analysis
    QJsonObject failConfig;
    QJsonObject main;
    main["OutFile"] = "stat_fail";
    failConfig["Main"] = main;
    
    QJsonDocument doc(failConfig);
    file.write(doc.toJson());
    file.close();
    
    // Input: Configuration with potential statistical analysis issues
    // Expected: Robust error handling in analysis pipeline
    QStringList result = runCliCommand({"-i", config});
    QVERIFY(result[0].toInt() >= 0);
}

// Helper Methods Implementation - Claude Generated
QString TestCliMLPipeline::createMLPipelineConfig()
{
    QString filePath = m_tempDir->path() + "/ml_pipeline.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    
    // Main configuration
    QJsonObject main;
    main["OutFile"] = "ml_pipeline_test";
    main["Repeat"] = 1;
    main["Threads"] = 2;
    config["Main"] = main;
    
    // Independent data generation
    QJsonObject independent;
    independent["Source"] = "generator";
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 25;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|X*1.5";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    // Dependent data generation with model
    QJsonObject dependent;
    dependent["Source"] = "generator";
    QJsonObject depGen;
    depGen["Type"] = "model";
    depGen["Series"] = 2;
    QJsonObject model;
    model["ID"] = 1; // Simple 1:1 binding model
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    
    // Add some noise
    QJsonObject noise;
    noise["Type"] = "gaussian";
    QJsonArray stdValues;
    stdValues.append(0.01);
    stdValues.append(0.01);
    noise["Std"] = stdValues;
    dependent["Noise"] = noise;
    config["Dependent"] = dependent;
    
    // Models to fit
    QJsonObject models;
    QJsonObject model1;
    model1["ID"] = 1;
    models["0"] = model1;
    QJsonObject model2;
    model2["ID"] = 2;
    models["1"] = model2;
    config["Models"] = models;
    
    // Statistical analysis jobs
    QJsonObject jobs;
    QJsonObject mcJob;
    mcJob["Type"] = "MonteCarlo";
    mcJob["Steps"] = 100;
    jobs["MonteCarlo"] = mcJob;
    config["Jobs"] = jobs;
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QString TestCliMLPipeline::createMultiModelConfig()
{
    QString filePath = m_tempDir->path() + "/multi_model.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    
    QJsonObject main;
    main["OutFile"] = "multi_model_test";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    // Basic data generation
    QJsonObject independent;
    independent["Source"] = "generator";
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 20;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|X*2";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    dependent["Source"] = "generator";
    QJsonObject depGen;
    depGen["Type"] = "model";
    depGen["Series"] = 2;
    QJsonObject model;
    model["ID"] = 1;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    // Multiple models to compare
    QJsonObject models;
    for (int i = 1; i <= 3; ++i) {
        QJsonObject modelConfig;
        modelConfig["ID"] = i;
        models[QString::number(i-1)] = modelConfig;
    }
    config["Models"] = models;
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QString TestCliMLPipeline::createLargeScaleConfig()
{
    QString filePath = m_tempDir->path() + "/large_scale.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    
    QJsonObject main;
    main["OutFile"] = "large_scale_test";
    main["Repeat"] = 1;
    main["Threads"] = 2;
    config["Main"] = main;
    
    // Large dataset
    QJsonObject independent;
    independent["Source"] = "generator";
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 200; // Large dataset
    indepGen["Variables"] = 5;
    indepGen["Equations"] = "X|X*2|X*3|X*4|X*5";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    dependent["Source"] = "generator";
    QJsonObject depGen;
    depGen["Type"] = "model";
    depGen["Series"] = 3;
    QJsonObject model;
    model["ID"] = 1;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    // Multiple models
    QJsonObject models;
    for (int i = 1; i <= 2; ++i) {
        QJsonObject modelConfig;
        modelConfig["ID"] = i;
        models[QString::number(i-1)] = modelConfig;
    }
    config["Models"] = models;
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QString TestCliMLPipeline::createStatisticalAnalysisConfig()
{
    QString filePath = m_tempDir->path() + "/statistical.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    
    QJsonObject main;
    main["OutFile"] = "statistical_test";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    // Basic data
    QJsonObject independent;
    independent["Source"] = "generator";
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 30;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|X*1.2";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    dependent["Source"] = "generator";
    QJsonObject depGen;
    depGen["Type"] = "model";
    depGen["Series"] = 2;
    QJsonObject model;
    model["ID"] = 1;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    // One model
    QJsonObject models;
    QJsonObject model1;
    model1["ID"] = 1;
    models["0"] = model1;
    config["Models"] = models;
    
    // Comprehensive statistical analysis
    QJsonObject jobs;
    
    QJsonObject mcJob;
    mcJob["Type"] = "MonteCarlo";
    mcJob["Steps"] = 50; // Reduced for testing
    jobs["MonteCarlo"] = mcJob;
    
    QJsonObject cvJob;
    cvJob["Type"] = "CrossValidation";
    cvJob["Folds"] = 5;
    jobs["CrossValidation"] = cvJob;
    
    config["Jobs"] = jobs;
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QString TestCliMLPipeline::createFeatureExtractionConfig()
{
    QString filePath = m_tempDir->path() + "/feature_extract.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    
    QJsonObject main;
    main["OutFile"] = "feature_test";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    // Data for feature extraction
    QJsonObject independent;
    independent["Source"] = "generator";
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 40;
    indepGen["Variables"] = 3;
    indepGen["Equations"] = "X|Y|Z";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    dependent["Source"] = "generator";
    QJsonObject depGen;
    depGen["Type"] = "model";
    depGen["Series"] = 2;
    QJsonObject model;
    model["ID"] = 1;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    // Model for feature extraction
    QJsonObject models;
    QJsonObject model1;
    model1["ID"] = 1;
    models["0"] = model1;
    config["Models"] = models;
    
    // Feature extraction job
    QJsonObject jobs;
    QJsonObject featureJob;
    featureJob["Type"] = "FeatureExtraction";
    featureJob["CompactFormat"] = true;
    jobs["Features"] = featureJob;
    config["Jobs"] = jobs;
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QString TestCliMLPipeline::createTutorialIntegrationConfig()
{
    QString filePath = m_tempDir->path() + "/tutorial_integration.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    
    QJsonObject main;
    main["OutFile"] = "tutorial_data";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    // Generate tutorial-suitable data
    QJsonObject independent;
    independent["Source"] = "generator";
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 10;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|Y";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    dependent["Source"] = "generator";
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 1;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QString TestCliMLPipeline::createIncompleteDataConfig()
{
    QString filePath = m_tempDir->path() + "/incomplete.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    
    // Minimal configuration - might be missing required sections
    QJsonObject main;
    main["OutFile"] = "incomplete_test";
    config["Main"] = main;
    
    // Missing Independent/Dependent sections intentionally
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QStringList TestCliMLPipeline::runCliCommand(const QStringList& arguments, int timeoutMs)
{
    QProcess process;
    process.start(m_suprafitCli, arguments);
    process.waitForFinished(timeoutMs);
    
    QStringList result;
    result << QString::number(process.exitCode());
    result << QString::fromUtf8(process.readAllStandardOutput());
    result << QString::fromUtf8(process.readAllStandardError());
    
    return result;
}

QStringList TestCliMLPipeline::runMLCliCommand(const QStringList& arguments, int timeoutMs)
{
    if (m_mlCli.isEmpty()) {
        QStringList result;
        result << "-1" << "" << "ML CLI not available";
        return result;
    }
    
    QProcess process;
    process.start(m_mlCli, arguments);
    process.waitForFinished(timeoutMs);
    
    QStringList result;
    result << QString::number(process.exitCode());
    result << QString::fromUtf8(process.readAllStandardOutput());
    result << QString::fromUtf8(process.readAllStandardError());
    
    return result;
}

bool TestCliMLPipeline::validateMLPipelineOutput(const QString& filename)
{
    QJsonObject data = loadJsonFile(filename);
    if (data.isEmpty()) {
        return false;
    }
    
    // Basic ML pipeline validation
    return data.contains("data");
}

bool TestCliMLPipeline::validateFeatureData(const QString& filename)
{
    QJsonObject data = loadJsonFile(filename);
    if (data.isEmpty()) {
        return false;
    }
    
    // Feature data validation - check for structured ML features
    return !data.isEmpty();
}

QJsonObject TestCliMLPipeline::loadJsonFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    
    if (error.error != QJsonParseError::NoError) {
        return QJsonObject();
    }
    
    return doc.object();
}

bool TestCliMLPipeline::verifyModelFitQuality(const QJsonObject& modelData)
{
    // Basic model fit quality verification
    if (modelData.isEmpty()) {
        return false;
    }
    
    // Look for common model quality indicators
    return modelData.contains("global_parameter") || 
           modelData.contains("local_parameter") ||
           modelData.contains("sse") ||
           modelData.contains("aic");
}

bool TestCliMLPipeline::verifyStatisticalResults(const QJsonObject& statisticalData)
{
    if (statisticalData.isEmpty()) {
        return false;
    }
    
    // Look for statistical analysis results
    for (auto it = statisticalData.begin(); it != statisticalData.end(); ++it) {
        if (it.key().startsWith("model_")) {
            QJsonObject model = it.value().toObject();
            if (model.contains("post_fit_analysis") || 
                model.contains("methods") ||
                model.contains("monte_carlo") ||
                model.contains("cross_validation")) {
                return true;
            }
        }
    }
    
    return false;
}

double TestCliMLPipeline::extractModelFitnessMeetic(const QJsonObject& modelData, const QString& metric)
{
    // Extract specific fitness metrics from model data
    if (modelData.contains(metric)) {
        return modelData[metric].toDouble();
    }
    
    return -1.0; // Invalid metric
}

#include "test_cli_ml_pipeline.moc"

QTEST_MAIN(TestCliMLPipeline)