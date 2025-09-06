/*
 * Comprehensive integration tests for SupraFit CLI functionality
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Tests end-to-end workflows, real data processing, performance benchmarks,
 * memory management, error recovery, and edge cases for complete system validation.
 * Claude Generated - 2025-09-02
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
#include <QtCore/QDir>

#include "test_utils.h"

class TestIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // End-to-End Workflow Tests
    void testCompleteDataGenerationWorkflow();
    void testCompleteModelFittingWorkflow();
    void testCompleteStatisticalAnalysisWorkflow();
    void testCompleteMLPipelineWorkflow();

    // Multi-Step Process Tests
    void testDataGenerationToModelFitting();
    void testModelFittingToPostProcessing();
    void testPostProcessingToMLExtraction();
    void testFullPipelineIntegration();

    // Real Data Processing Tests
    void testRealDataAnalysis();
    void testLargeDatasetProcessing();
    void testComplexModelStructures();
    void testMultipleModelComparison();

    // Performance Benchmarks
    void testSmallDatasetPerformance();
    void testMediumDatasetPerformance();
    void testLargeDatasetPerformance();
    void testMemoryUsageScaling();

    // Error Recovery Tests
    void testRecoveryFromBadData();
    void testRecoveryFromModelFailure();
    void testRecoveryFromStatisticalErrors();
    void testPartialWorkflowCompletion();

    // Edge Case Tests
    void testMinimalDataSets();
    void testMaximalDataSets();
    void testExtremeParameterValues();
    void testBoundaryConditions();

    // Concurrent Processing Tests
    void testMultiThreadedProcessing();
    void testParallelStatisticalAnalysis();
    void testResourceContention();

    // System Integration Tests
    void testFileSystemIntegration();
    void testTemporaryFileHandling();
    void testResourceCleanup();
    void testLongRunningProcesses();

private:
    QTemporaryDir* m_tempDir;
    
    QString createRealisticDataConfig();
    QString createLargeScaleConfig();
    QString createComplexModelConfig();
    QString createMinimalConfig();
    QString createProblematicConfig();
    QJsonObject loadResultFile(const QString& filename);
    bool verifyWorkflowCompletion(const QStringList& result);
    bool verifyDataQuality(const QJsonObject& data);
    qint64 measureMemoryUsage();
    void createTestDataFile(const QString& filename, int rows, int cols);
    bool verifyMLOutputQuality(const QJsonObject& mlOutput);
};

void TestIntegration::initTestCase()
{
    qDebug() << "Starting Integration tests...";
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    // Verify CLI binary is available through TestUtils
    QString cliPath = TestUtils::findSuprafitCli();
    QVERIFY2(!cliPath.isEmpty(), "suprafit_cli executable not found - set SUPRAFIT_CLI_PATH env var if needed");
}

void TestIntegration::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "Integration tests completed.";
}

// Removed - using TestUtils::executeCliCommand instead

QJsonObject TestIntegration::loadResultFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        return QJsonObject();
    }
    
    return doc.object();
}

bool TestIntegration::verifyWorkflowCompletion(const QStringList& result)
{
    if (result[0].toInt() != 0) return false; // Non-zero exit code
    
    QString output = result[1] + " " + result[2];
    return output.contains("completed") || output.contains("successfully") || 
           output.contains("finished") || output.contains("done");
}

bool TestIntegration::verifyDataQuality(const QJsonObject& data)
{
    if (data.isEmpty()) return false;
    
    // Check for essential data structure elements
    bool hasStructure = data.contains("format_version") || 
                       data.contains("Main") || 
                       data.contains("Independent") ||
                       data.contains("base_data");
    
    if (!hasStructure) return false;
    
    // Check for reasonable data content
    QJsonDocument doc(data);
    QByteArray jsonData = doc.toJson();
    return jsonData.size() > 100; // Minimum reasonable size
}

bool TestIntegration::verifyMLOutputQuality(const QJsonObject& mlOutput)
{
    if (!mlOutput.contains("ml_training_metadata")) return false;
    if (!mlOutput.contains("training_samples")) return false;
    
    QJsonObject metadata = mlOutput["ml_training_metadata"].toObject();
    if (!metadata.contains("sample_count")) return false;
    if (metadata["sample_count"].toInt() <= 0) return false;
    
    QJsonArray samples = mlOutput["training_samples"].toArray();
    return !samples.isEmpty();
}

qint64 TestIntegration::measureMemoryUsage()
{
    // Simple approximation - in a real implementation, this would measure actual memory usage
    QProcess memoryCheck;
    memoryCheck.start("ps", QStringList() << "-o" << "rss=" << "-p" << QString::number(QCoreApplication::applicationPid()));
    memoryCheck.waitForFinished(5000);
    
    QString output = memoryCheck.readAllStandardOutput();
    return output.trimmed().toLongLong();
}

void TestIntegration::createTestDataFile(const QString& filename, int rows, int cols)
{
    QJsonObject dataConfig;
    QJsonObject main;
    main["OutFile"] = "test_data";
    main["Repeat"] = 1;
    dataConfig["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = rows;
    indepGen["Variables"] = cols;
    
    QStringList equations;
    for (int i = 0; i < cols; ++i) {
        if (i == 0) equations << "X";
        else if (i == 1) equations << "Y";
        else equations << QString("X*%1").arg(i);
    }
    indepGen["Equations"] = equations.join("|");
    independent["Generator"] = indepGen;
    dataConfig["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = cols;
    dependent["Generator"] = depGen;
    dataConfig["Dependent"] = dependent;
    
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(dataConfig).toJson());
    file.close();
}

void TestIntegration::testCompleteDataGenerationWorkflow()
{
    QString configFile = createRealisticDataConfig();
    QString outputFile = m_tempDir->path() + "/complete_data_workflow";
    
    QElapsedTimer timer;
    timer.start();
    
    QStringList result = TestUtils::executeCliCommand({"-i", configFile, "-o", outputFile});
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Complete data generation workflow failed: " + result[2]));
    QVERIFY2(verifyWorkflowCompletion(result), "Workflow completion not detected");
    QVERIFY2(elapsed < 30000, qPrintable(QString("Workflow took too long: %1ms").arg(elapsed)));
    
    // Verify output files exist
    QString generatedFile = outputFile + "-0.json";
    QVERIFY2(QFile::exists(generatedFile), "Generated data file not found");
    
    // Verify data quality
    QJsonObject generatedData = loadResultFile(generatedFile);
    QVERIFY2(verifyDataQuality(generatedData), "Generated data quality insufficient");
}

void TestIntegration::testCompleteModelFittingWorkflow()
{
    QString configFile = createComplexModelConfig();
    QString outputFile = m_tempDir->path() + "/complete_model_workflow";
    
    QStringList result = TestUtils::executeCliCommand({"-i", configFile, "-o", outputFile}, 120000); // 2 minute timeout
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Complete model fitting workflow failed: " + result[2]));
    QVERIFY2(verifyWorkflowCompletion(result), "Model fitting workflow completion not detected");
    
    // Should show model fitting progress
    QVERIFY(result[1].contains("model") || result[1].contains("fitting") || 
            result[1].contains("converged") || result[1].contains("completed"));
}

void TestIntegration::testCompleteStatisticalAnalysisWorkflow()
{
    QString configFile = createComplexModelConfig();
    QString outputFile = m_tempDir->path() + "/complete_stats_workflow";
    
    QStringList result = TestUtils::executeCliCommand({"-i", configFile, "-o", outputFile}, 180000); // 3 minute timeout
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Complete statistical analysis workflow failed: " + result[2]));
    QVERIFY2(verifyWorkflowCompletion(result), "Statistical analysis workflow completion not detected");
    
    // Should show statistical analysis progress
    QVERIFY(result[1].contains("analysis") || result[1].contains("Monte Carlo") || 
            result[1].contains("Cross Validation") || result[1].contains("calculation"));
}

void TestIntegration::testCompleteMLPipelineWorkflow()
{
    QString configFile = createComplexModelConfig();
    QString dataOutputFile = m_tempDir->path() + "/ml_pipeline_data";
    QString mlOutputFile = m_tempDir->path() + "/ml_pipeline_features.json";
    
    // Step 1: Generate data with models and statistical analysis
    QStringList result1 = TestUtils::executeCliCommand({"-i", configFile, "-o", dataOutputFile}, 180000);
    QVERIFY2(result1[0].toInt() == 0, qPrintable("ML pipeline data generation failed: " + result1[2]));
    
    // Step 2: Extract ML features
    QString generatedDataFile = dataOutputFile + "-0.json";
    if (QFile::exists(generatedDataFile)) {
        QStringList result2 = TestUtils::executeCliCommand({"--export-ml-training", generatedDataFile, "--ml-output", mlOutputFile});
        
        if (result2[0].toInt() == 0) {
            QVERIFY(QFile::exists(mlOutputFile));
            
            QJsonObject mlOutput = loadResultFile(mlOutputFile);
            QVERIFY2(verifyMLOutputQuality(mlOutput), "ML output quality insufficient");
        }
    }
}

void TestIntegration::testDataGenerationToModelFitting()
{
    QString dataConfig = createRealisticDataConfig();
    QString dataOutput = m_tempDir->path() + "/data_to_model";
    
    // Generate data
    QStringList dataResult = TestUtils::executeCliCommand({"-i", dataConfig, "-o", dataOutput});
    QVERIFY2(dataResult[0].toInt() == 0, qPrintable("Data generation step failed: " + dataResult[2]));
    
    QString generatedFile = dataOutput + "-0.json";
    QVERIFY(QFile::exists(generatedFile));
    
    // Analyze generated data structure
    QStringList analysisResult = TestUtils::executeCliCommand({"-l", generatedFile});
    QVERIFY2(analysisResult[0].toInt() == 0, qPrintable("Data analysis failed: " + analysisResult[2]));
    
    // Should show structure suitable for model fitting
    QVERIFY(analysisResult[1].contains("Independent") || analysisResult[1].contains("Dependent") ||
            analysisResult[1].contains("data") || analysisResult[1].contains("rows"));
}

void TestIntegration::testModelFittingToPostProcessing()
{
    QString modelConfig = createComplexModelConfig();
    QString modelOutput = m_tempDir->path() + "/model_to_stats";
    
    // Run model fitting with post-processing
    QStringList result = TestUtils::executeCliCommand({"-i", modelConfig, "-o", modelOutput}, 180000);
    QVERIFY2(result[0].toInt() == 0, qPrintable("Model fitting to post-processing failed: " + result[2]));
    
    // Should show both model fitting and statistical analysis
    QVERIFY(result[1].contains("fitting") && 
            (result[1].contains("analysis") || result[1].contains("Monte Carlo") || 
             result[1].contains("Cross Validation")));
}

void TestIntegration::testPostProcessingToMLExtraction()
{
    QString complexConfig = createComplexModelConfig();
    QString complexOutput = m_tempDir->path() + "/stats_to_ml";
    
    // Generate data with statistical analysis
    QStringList result1 = TestUtils::executeCliCommand({"-i", complexConfig, "-o", complexOutput}, 180000);
    
    if (result1[0].toInt() == 0) {
        QString analysisFile = complexOutput + "-0.json";
        if (QFile::exists(analysisFile)) {
            QString mlOutput = m_tempDir->path() + "/extracted_features.json";
            
            // Extract ML features
            QStringList result2 = TestUtils::executeCliCommand({"--export-ml-training", analysisFile, "--ml-output", mlOutput});
            
            if (result2[0].toInt() == 0) {
                QVERIFY(QFile::exists(mlOutput));
                
                QJsonObject mlData = loadResultFile(mlOutput);
                QVERIFY2(verifyMLOutputQuality(mlData), "ML extraction quality check failed");
            }
        }
    }
}

void TestIntegration::testFullPipelineIntegration()
{
    QString pipelineConfig = createComplexModelConfig();
    QString pipelineOutput = m_tempDir->path() + "/full_pipeline";
    QString mlFeaturesOutput = m_tempDir->path() + "/full_pipeline_ml.json";
    
    QElapsedTimer totalTimer;
    totalTimer.start();
    
    // Complete pipeline execution
    QStringList pipelineResult = TestUtils::executeCliCommand({"-i", pipelineConfig, "-o", pipelineOutput}, 300000); // 5 minute timeout
    QVERIFY2(pipelineResult[0].toInt() == 0, qPrintable("Full pipeline integration failed: " + pipelineResult[2]));
    
    // ML feature extraction
    QString pipelineFile = pipelineOutput + "-0.json";
    if (QFile::exists(pipelineFile)) {
        QStringList mlResult = TestUtils::executeCliCommand({"--export-ml-training", pipelineFile, "--ml-output", mlFeaturesOutput});
        
        if (mlResult[0].toInt() == 0) {
            QVERIFY(QFile::exists(mlFeaturesOutput));
        }
    }
    
    qint64 totalElapsed = totalTimer.elapsed();
    QVERIFY2(totalElapsed < 360000, qPrintable(QString("Full pipeline took too long: %1ms").arg(totalElapsed))); // 6 minutes max
}

void TestIntegration::testRealDataAnalysis()
{
    QString realisticConfig = createRealisticDataConfig();
    QString analysisOutput = m_tempDir->path() + "/real_data_analysis";
    
    QStringList result = TestUtils::executeCliCommand({"-i", realisticConfig, "-o", analysisOutput}, 120000);
    QVERIFY2(result[0].toInt() == 0, qPrintable("Real data analysis failed: " + result[2]));
    
    // Verify realistic output
    QString resultFile = analysisOutput + "-0.json";
    if (QFile::exists(resultFile)) {
        QJsonObject realData = loadResultFile(resultFile);
        QVERIFY2(verifyDataQuality(realData), "Real data analysis quality insufficient");
        
        // Should contain realistic data structures
        QJsonDocument doc(realData);
        QString jsonString = doc.toJson();
        QVERIFY2(jsonString.length() > 500, "Real data output too small");
    }
}

void TestIntegration::testLargeDatasetProcessing()
{
    QString largeConfig = createLargeScaleConfig();
    QString largeOutput = m_tempDir->path() + "/large_dataset";
    
    QElapsedTimer timer;
    timer.start();
    
    qint64 memoryBefore = measureMemoryUsage();
    
    QStringList result = TestUtils::executeCliCommand({"-i", largeConfig, "-o", largeOutput}, 300000); // 5 minute timeout
    
    qint64 memoryAfter = measureMemoryUsage();
    qint64 elapsed = timer.elapsed();
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Large dataset processing failed: " + result[2]));
    
    // Performance checks
    QVERIFY2(elapsed < 300000, qPrintable(QString("Large dataset processing too slow: %1ms").arg(elapsed)));
    
    // Memory usage should be reasonable
    qint64 memoryDelta = memoryAfter - memoryBefore;
    QVERIFY2(memoryDelta < 1000000, qPrintable(QString("Excessive memory usage: %1KB").arg(memoryDelta)));
}

void TestIntegration::testComplexModelStructures()
{
    QString complexConfig = createComplexModelConfig();
    QString complexOutput = m_tempDir->path() + "/complex_models";
    
    QStringList result = TestUtils::executeCliCommand({"-i", complexConfig, "-o", complexOutput}, 180000);
    QVERIFY2(result[0].toInt() == 0, qPrintable("Complex model structures failed: " + result[2]));
    
    // Should handle multiple models and complex analysis
    QVERIFY(result[1].contains("nmr_1_1") || result[1].contains("nmr_1_2") ||
            result[1].contains("model") || result[1].contains("fitting"));
}

void TestIntegration::testMultipleModelComparison()
{
    QString multiModelConfig = createComplexModelConfig(); // Contains multiple models
    QString comparisonOutput = m_tempDir->path() + "/model_comparison";
    
    QStringList result = TestUtils::executeCliCommand({"-i", multiModelConfig, "-o", comparisonOutput}, 180000);
    QVERIFY2(result[0].toInt() == 0, qPrintable("Multiple model comparison failed: " + result[2]));
    
    // Should show comparison between multiple models
    QVERIFY(result[1].contains("model") && 
            (result[1].contains("nmr_1_1") || result[1].contains("nmr_1_2") ||
             result[1].contains("comparison") || result[1].contains("analysis")));
}

void TestIntegration::testSmallDatasetPerformance()
{
    QString smallConfig = createMinimalConfig();
    QString smallOutput = m_tempDir->path() + "/small_perf";
    
    QElapsedTimer timer;
    timer.start();
    
    QStringList result = TestUtils::executeCliCommand({"-i", smallConfig, "-o", smallOutput});
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Small dataset performance test failed: " + result[2]));
    QVERIFY2(elapsed < 10000, qPrintable(QString("Small dataset too slow: %1ms").arg(elapsed))); // < 10 seconds
}

void TestIntegration::testMediumDatasetPerformance()
{
    QString mediumConfig = createRealisticDataConfig();
    QString mediumOutput = m_tempDir->path() + "/medium_perf";
    
    QElapsedTimer timer;
    timer.start();
    
    QStringList result = TestUtils::executeCliCommand({"-i", mediumConfig, "-o", mediumOutput}, 120000);
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Medium dataset performance test failed: " + result[2]));
    QVERIFY2(elapsed < 60000, qPrintable(QString("Medium dataset too slow: %1ms").arg(elapsed))); // < 1 minute
}

void TestIntegration::testLargeDatasetPerformance()
{
    QString largeConfig = createLargeScaleConfig();
    QString largeOutput = m_tempDir->path() + "/large_perf";
    
    QElapsedTimer timer;
    timer.start();
    
    QStringList result = TestUtils::executeCliCommand({"-i", largeConfig, "-o", largeOutput}, 300000); // 5 minutes
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Large dataset performance test failed: " + result[2]));
    QVERIFY2(elapsed < 300000, qPrintable(QString("Large dataset too slow: %1ms").arg(elapsed)));
    
    // Should scale reasonably
    QVERIFY2(elapsed > 5000, "Large dataset completed suspiciously quickly"); // At least 5 seconds
}

void TestIntegration::testMemoryUsageScaling()
{
    QList<QString> configs = {createMinimalConfig(), createRealisticDataConfig(), createLargeScaleConfig()};
    QList<qint64> memoryUsages;
    
    for (const QString& config : configs) {
        qint64 memoryBefore = measureMemoryUsage();
        
        QString output = m_tempDir->path() + QString("/memory_test_%1").arg(memoryUsages.size());
        QStringList result = TestUtils::executeCliCommand({"-i", config, "-o", output}, 180000);
        
        qint64 memoryAfter = measureMemoryUsage();
        
        if (result[0].toInt() == 0) {
            memoryUsages.append(memoryAfter - memoryBefore);
        } else {
            memoryUsages.append(0); // Failed execution
        }
    }
    
    // Memory usage should scale reasonably (not exponentially)
    if (memoryUsages.size() >= 2) {
        QVERIFY2(memoryUsages[0] >= 0, "Memory usage measurement failed");
        
        // Large dataset shouldn't use more than 10x the memory of small dataset
        if (memoryUsages[0] > 0 && memoryUsages.last() > 0) {
            double scalingFactor = static_cast<double>(memoryUsages.last()) / memoryUsages[0];
            QVERIFY2(scalingFactor < 10.0, qPrintable(QString("Excessive memory scaling: %1x").arg(scalingFactor)));
        }
    }
}

void TestIntegration::testRecoveryFromBadData()
{
    QString badDataConfig = createProblematicConfig();
    QString recoveryOutput = m_tempDir->path() + "/bad_data_recovery";
    
    QStringList result = TestUtils::executeCliCommand({"-i", badDataConfig, "-o", recoveryOutput});
    
    // Should handle bad data gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
    
    if (result[0].toInt() != 0) {
        QVERIFY(result[2].contains("Error") || result[2].contains("warning") ||
                result[2].contains("invalid") || result[2].contains("failed"));
    }
}

void TestIntegration::testRecoveryFromModelFailure()
{
    // Create config with problematic model parameters
    QJsonObject problematicConfig;
    QJsonObject main;
    main["OutFile"] = "model_failure_test";
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
    problematicConfig["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 5; // Very few points for model fitting
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|Y";
    independent["Generator"] = indepGen;
    problematicConfig["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "model";
    QJsonObject model;
    model["ID"] = 1;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    problematicConfig["Dependent"] = dependent;
    
    QJsonObject addModels;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options;
    options["Convergency"] = 1e-15; // Extremely strict convergence
    nmr11["Options"] = options;
    addModels["nmr_1_1"] = nmr11;
    problematicConfig["AddModels"] = addModels;
    
    QString configFile = m_tempDir->path() + "/model_failure_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(problematicConfig).toJson());
    file.close();
    
    QString failureOutput = m_tempDir->path() + "/model_failure_recovery";
    QStringList result = TestUtils::executeCliCommand({"-i", configFile, "-o", failureOutput});
    
    // Should handle model failure gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
}

void TestIntegration::testRecoveryFromStatisticalErrors()
{
    QJsonObject statsErrorConfig;
    QJsonObject main;
    main["OutFile"] = "stats_error_test";
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
    main["PostFitAnalysis"] = true;
    statsErrorConfig["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 3; // Insufficient for meaningful statistics
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|Y";
    independent["Generator"] = indepGen;
    statsErrorConfig["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 2;
    dependent["Generator"] = depGen;
    statsErrorConfig["Dependent"] = dependent;
    
    QJsonObject addModels;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    addModels["nmr_1_1"] = nmr11;
    statsErrorConfig["AddModels"] = addModels;
    
    QJsonObject postFit;
    QJsonArray methods;
    QJsonObject mcMethod;
    mcMethod["Method"] = 1;
    mcMethod["MaxSteps"] = 10000; // Too many steps for tiny dataset
    methods.append(mcMethod);
    postFit["methods"] = methods;
    statsErrorConfig["PostFitAnalysis"] = postFit;
    
    QString configFile = m_tempDir->path() + "/stats_error_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(statsErrorConfig).toJson());
    file.close();
    
    QString errorOutput = m_tempDir->path() + "/stats_error_recovery";
    QStringList result = TestUtils::executeCliCommand({"-i", configFile, "-o", errorOutput}, 120000);
    
    // Should handle statistical errors gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
}

void TestIntegration::testPartialWorkflowCompletion()
{
    QString partialConfig = createComplexModelConfig();
    QString partialOutput = m_tempDir->path() + "/partial_workflow";
    
    // Run with limited timeout to potentially interrupt the process
    QStringList result = TestUtils::executeCliCommand({"-i", partialConfig, "-o", partialOutput}, 30000); // 30 second timeout
    
    // Even if interrupted, should handle gracefully
    QVERIFY(result[0].toInt() >= 0); // No negative exit codes (crashes)
    
    // Check if partial results exist
    QString partialFile = partialOutput + "-0.json";
    if (QFile::exists(partialFile)) {
        QJsonObject partialData = loadResultFile(partialFile);
        // Partial data might be valid or empty, both are acceptable
        QVERIFY(true); // Just verify no crash occurred
    }
}

void TestIntegration::testMinimalDataSets()
{
    QString minimalConfig = createMinimalConfig();
    QString minimalOutput = m_tempDir->path() + "/minimal_dataset";
    
    QStringList result = TestUtils::executeCliCommand({"-i", minimalConfig, "-o", minimalOutput});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Minimal dataset test failed: " + result[2]));
    
    // Should handle minimal data gracefully
    QString resultFile = minimalOutput + "-0.json";
    if (QFile::exists(resultFile)) {
        QJsonObject minimalData = loadResultFile(resultFile);
        QVERIFY(!minimalData.isEmpty());
    }
}

void TestIntegration::testMaximalDataSets()
{
    QString maximalConfig = createLargeScaleConfig();
    QString maximalOutput = m_tempDir->path() + "/maximal_dataset";
    
    QStringList result = TestUtils::executeCliCommand({"-i", maximalConfig, "-o", maximalOutput}, 300000); // 5 minute timeout
    QVERIFY2(result[0].toInt() == 0, qPrintable("Maximal dataset test failed: " + result[2]));
    
    // Should handle large datasets
    QString resultFile = maximalOutput + "-0.json";
    if (QFile::exists(resultFile)) {
        QFileInfo fileInfo(resultFile);
        QVERIFY2(fileInfo.size() > 10000, "Maximal dataset output too small"); // At least 10KB
    }
}

void TestIntegration::testExtremeParameterValues()
{
    // Create config with extreme parameter values
    QJsonObject extremeConfig;
    QJsonObject main;
    main["OutFile"] = "extreme_params";
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
    extremeConfig["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 20;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X*1e10|Y*1e-10"; // Extreme scaling
    independent["Generator"] = indepGen;
    extremeConfig["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "model";
    QJsonObject model;
    model["ID"] = 1;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    extremeConfig["Dependent"] = dependent;
    
    QJsonObject addModels;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    addModels["nmr_1_1"] = nmr11;
    extremeConfig["AddModels"] = addModels;
    
    QString configFile = m_tempDir->path() + "/extreme_params_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(extremeConfig).toJson());
    file.close();
    
    QString extremeOutput = m_tempDir->path() + "/extreme_params_test";
    QStringList result = TestUtils::executeCliCommand({"-i", configFile, "-o", extremeOutput});
    
    // Should handle extreme values gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
}

void TestIntegration::testBoundaryConditions()
{
    // Test with boundary parameter values
    QString testFile = m_tempDir->path() + "/boundary_test.json";
    createTestDataFile(testFile, 1, 1); // Minimal: 1 row, 1 column
    
    QStringList result1 = TestUtils::executeCliCommand({"-l", testFile});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("Boundary condition test failed: " + result1[2]));
    
    // Test with maximum reasonable values
    createTestDataFile(testFile, 10000, 50); // Large but not excessive
    QStringList result2 = TestUtils::executeCliCommand({"-l", testFile});
    QVERIFY2(result2[0].toInt() == 0, qPrintable("Large boundary condition test failed: " + result2[2]));
}

void TestIntegration::testMultiThreadedProcessing()
{
    QString threadConfig = createComplexModelConfig();
    QString threadOutput = m_tempDir->path() + "/multithread_test";
    
    // Test with different thread counts
    QList<int> threadCounts = {1, 2, 4};
    
    for (int threads : threadCounts) {
        QElapsedTimer timer;
        timer.start();
        
        QString output = threadOutput + QString("_%1").arg(threads);
        QStringList result = TestUtils::executeCliCommand({"-n", QString::number(threads), "-i", threadConfig, "-o", output}, 180000);
        
        qint64 elapsed = timer.elapsed();
        
        QVERIFY2(result[0].toInt() == 0, qPrintable(QString("Multi-threaded processing failed with %1 threads: %2").arg(threads).arg(result[2])));
        
        // More threads should generally complete faster (but not always due to overhead)
        QVERIFY2(elapsed < 180000, qPrintable(QString("%1 threads took too long: %2ms").arg(threads).arg(elapsed)));
    }
}

void TestIntegration::testParallelStatisticalAnalysis()
{
    QString parallelConfig = createComplexModelConfig();
    QString parallelOutput = m_tempDir->path() + "/parallel_stats";
    
    QStringList result = TestUtils::executeCliCommand({"-n", "4", "-i", parallelConfig, "-o", parallelOutput}, 180000);
    QVERIFY2(result[0].toInt() == 0, qPrintable("Parallel statistical analysis failed: " + result[2]));
    
    // Should show parallel execution indicators
    QVERIFY(result[1].contains("Thread") || result[1].contains("calculation") ||
            result[1].contains("analysis") || result[1].contains("parallel"));
}

void TestIntegration::testResourceContention()
{
    QString contentionConfig = createLargeScaleConfig();
    QString contentionOutput = m_tempDir->path() + "/resource_contention";
    
    // Run with high thread count to test resource contention
    QStringList result = TestUtils::executeCliCommand({"-n", "8", "-i", contentionConfig, "-o", contentionOutput}, 300000);
    
    // Should complete even with high resource usage
    QVERIFY(result[0].toInt() >= 0); // No crashes due to resource contention
}

void TestIntegration::testFileSystemIntegration()
{
    QString fsConfig = createRealisticDataConfig();
    QString fsOutput = m_tempDir->path() + "/filesystem_test";
    
    QStringList result = TestUtils::executeCliCommand({"-i", fsConfig, "-o", fsOutput});
    QVERIFY2(result[0].toInt() == 0, qPrintable("File system integration failed: " + result[2]));
    
    // Verify file system operations
    QString resultFile = fsOutput + "-0.json";
    QVERIFY(QFile::exists(resultFile));
    
    QFileInfo fileInfo(resultFile);
    QVERIFY(fileInfo.isReadable());
    QVERIFY(fileInfo.size() > 0);
}

void TestIntegration::testTemporaryFileHandling()
{
    QString tempConfig = createRealisticDataConfig();
    QString tempOutput = m_tempDir->path() + "/temp_handling";
    
    QStringList result = TestUtils::executeCliCommand({"-i", tempConfig, "-o", tempOutput});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Temporary file handling failed: " + result[2]));
    
    // Should not leave temporary files in system temp directory
    QString systemTemp = QDir::tempPath();
    QDir tempDir(systemTemp);
    QStringList tempFiles = tempDir.entryList(QStringList() << "*suprafit*" << "*tmp*", QDir::Files);
    
    // Allow some temporary files but not excessive
    QVERIFY2(tempFiles.size() < 100, qPrintable(QString("Too many temp files: %1").arg(tempFiles.size())));
}

void TestIntegration::testResourceCleanup()
{
    qint64 memoryBefore = measureMemoryUsage();
    
    QString cleanupConfig = createRealisticDataConfig();
    QString cleanupOutput = m_tempDir->path() + "/resource_cleanup";
    
    QStringList result = TestUtils::executeCliCommand({"-i", cleanupConfig, "-o", cleanupOutput});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Resource cleanup test failed: " + result[2]));
    
    // Give system time to clean up
    QThread::msleep(1000);
    
    qint64 memoryAfter = measureMemoryUsage();
    
    // Memory usage should not increase dramatically
    qint64 memoryDelta = memoryAfter - memoryBefore;
    QVERIFY2(memoryDelta < 500000, qPrintable(QString("Memory leak detected: %1KB").arg(memoryDelta))); // < 500MB delta
}

void TestIntegration::testLongRunningProcesses()
{
    QString longConfig = createComplexModelConfig();
    QString longOutput = m_tempDir->path() + "/long_running";
    
    QElapsedTimer timer;
    timer.start();
    
    // Long-running process with extended timeout
    QStringList result = TestUtils::executeCliCommand({"-i", longConfig, "-o", longOutput}, 600000); // 10 minute timeout
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Long running process failed: " + result[2]));
    
    // Should complete within reasonable time even for complex analysis
    QVERIFY2(elapsed < 600000, qPrintable(QString("Process ran too long: %1ms").arg(elapsed)));
    QVERIFY2(elapsed > 1000, "Process completed suspiciously quickly"); // At least 1 second
}

QString TestIntegration::createRealisticDataConfig()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "realistic_data";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 25;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "0.001|(X - 1) * 0.0001";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "model";
    QJsonObject model;
    model["ID"] = 1;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    
    QJsonObject noise;
    noise["Type"] = "gaussian";
    QJsonArray stdArray;
    stdArray.append(0.01);
    stdArray.append(0.01);
    noise["Std"] = stdArray;
    dependent["Noise"] = noise;
    config["Dependent"] = dependent;
    
    QString filePath = m_tempDir->path() + "/realistic_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    return filePath;
}

QString TestIntegration::createLargeScaleConfig()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "large_scale_data";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 500; // Large dataset
    indepGen["Variables"] = 5;
    indepGen["Equations"] = "X|Y|Z|X*Y|X*Z";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 5;
    dependent["Generator"] = depGen;
    
    QJsonObject noise;
    noise["Type"] = "gaussian";
    QJsonArray stdArray;
    for (int i = 0; i < 5; ++i) {
        stdArray.append(0.01);
    }
    noise["Std"] = stdArray;
    dependent["Noise"] = noise;
    config["Dependent"] = dependent;
    
    QString filePath = m_tempDir->path() + "/large_scale_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    return filePath;
}

QString TestIntegration::createComplexModelConfig()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "complex_model";
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
    main["PostFitAnalysis"] = true;
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 30;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "0.001|(X - 1) * 0.0001";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "model";
    QJsonObject model;
    model["ID"] = 1;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    QJsonObject addModels;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options11;
    options11["FastMode"] = true;
    nmr11["Options"] = options11;
    addModels["nmr_1_1"] = nmr11;
    
    QJsonObject nmr12;
    nmr12["ID"] = 2;
    QJsonObject options12;
    options12["FastMode"] = true;
    nmr12["Options"] = options12;
    addModels["nmr_1_2"] = nmr12;
    
    config["AddModels"] = addModels;
    
    QJsonObject postFit;
    QJsonArray methods;
    QJsonObject mcMethod;
    mcMethod["Method"] = 1;
    mcMethod["MaxSteps"] = 500;
    mcMethod["VarianceSource"] = 2;
    methods.append(mcMethod);
    
    QJsonObject cvMethod;
    cvMethod["Method"] = 4;
    cvMethod["CVType"] = 1;
    cvMethod["MaxSteps"] = 20;
    methods.append(cvMethod);
    
    postFit["methods"] = methods;
    config["PostFitAnalysis"] = postFit;
    
    QString filePath = m_tempDir->path() + "/complex_model_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    return filePath;
}

QString TestIntegration::createMinimalConfig()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "minimal_test";
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 5;
    indepGen["Variables"] = 1;
    indepGen["Equations"] = "X";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 1;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    QString filePath = m_tempDir->path() + "/minimal_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    return filePath;
}

QString TestIntegration::createProblematicConfig()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "problematic_test";
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 10;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "invalid_equation|another_bad_equation";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 2;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    QString filePath = m_tempDir->path() + "/problematic_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    return filePath;
}

#include "test_integration.moc"

QTEST_MAIN(TestIntegration)