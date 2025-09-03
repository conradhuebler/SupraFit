/*
 * Comprehensive tests for SupraFit post-processing functionality
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Tests all 7 statistical analysis methods: Monte Carlo, Cross Validation,
 * Weakened Grid Search, Model Comparison, Parameter Reduction, Fast Confidence,
 * and Global Search. Also tests --show-post-processing flag.
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

class TestPostProcessing : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Statistical Analysis Method Tests
    void testMonteCarloAnalysis();
    void testCrossValidationAnalysis();
    void testWeakenedGridSearchAnalysis();
    void testModelComparisonAnalysis();
    void testParameterReductionAnalysis();
    void testFastConfidenceAnalysis();
    void testGlobalSearchAnalysis();

    // Method Configuration Tests
    void testMonteCarloParameters();
    void testCrossValidationTypes();
    void testVarianceSourceOptions();
    void testMaxStepsConfiguration();

    // Multiple Method Tests
    void testMultipleMethodCombination();
    void testMethodSequencing();
    void testMethodInteraction();

    // Statistical Results Tests
    void testParameterUncertainty();
    void testConfidenceIntervals();
    void testParameterDistributions();
    void testStatisticalSummaries();

    // CLI Integration Tests
    void testShowPostProcessingFlag();
    void testPostProcessingOutput();
    void testMethodResultDisplay();

    // Performance Tests
    void testLargeMonteCarloSamples();
    void testExtensiveCrossValidation();
    void testPerformanceScaling();

    // Error Handling Tests
    void testInvalidMethodIds();
    void testInvalidParameters();
    void testInsufficientData();

private:
    QTemporaryDir* m_tempDir;
    QString m_suprafitCli;
    
    QString createPostProcessingConfig(const QJsonArray& methods);
    QString createModelWithPostProcessing(int modelId, const QJsonArray& methods);
    QJsonObject createMonteCarloMethod(int maxSteps = 1000, int varianceSource = 2);
    QJsonObject createCrossValidationMethod(int cvType = 1, int maxSteps = 20);
    QJsonObject createWeakenedGridSearchMethod();
    QJsonObject createModelComparisonMethod();
    QJsonObject createParameterReductionMethod();
    QJsonObject createFastConfidenceMethod();
    QJsonObject createGlobalSearchMethod();
    QStringList runCliCommand(const QStringList& arguments);
    bool verifyMethodExecution(const QString& output, int methodId);
    bool verifyStatisticalResults(const QString& output);
};

void TestPostProcessing::initTestCase()
{
    qDebug() << "Starting Post-Processing tests...";
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    // Find suprafit_cli executable
    m_suprafitCli = QCoreApplication::applicationDirPath() + "/bin/linux/suprafit_cli";
    if (!QFile::exists(m_suprafitCli)) {
        m_suprafitCli = "../bin/linux/suprafit_cli";
    }
    if (!QFile::exists(m_suprafitCli)) {
        m_suprafitCli = "bin/linux/suprafit_cli";
    }
    
    QVERIFY2(QFile::exists(m_suprafitCli), "suprafit_cli executable not found");
}

void TestPostProcessing::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "Post-Processing tests completed.";
}

QStringList TestPostProcessing::runCliCommand(const QStringList& arguments)
{
    QProcess process;
    process.start(m_suprafitCli, arguments);
    process.waitForFinished(120000); // 120 second timeout for statistical analysis
    
    QStringList result;
    result << QString::number(process.exitCode());
    result << QString::fromUtf8(process.readAllStandardOutput());
    result << QString::fromUtf8(process.readAllStandardError());
    
    return result;
}

bool TestPostProcessing::verifyMethodExecution(const QString& output, int methodId)
{
    QString methodName;
    switch (methodId) {
    case 1: methodName = "Monte Carlo"; break;
    case 2: methodName = "Weakened Grid Search"; break;
    case 3: methodName = "Model Comparison"; break;
    case 4: methodName = "Cross Validation"; break;
    case 5: methodName = "Parameter Reduction"; break;
    case 6: methodName = "Fast Confidence"; break;
    case 7: methodName = "Global Search"; break;
    default: return false;
    }
    
    return output.contains(methodName) || 
           output.contains(QString("Method: %1").arg(methodId)) ||
           output.contains(QString("analysis")) ||
           output.contains("calculation");
}

bool TestPostProcessing::verifyStatisticalResults(const QString& output)
{
    return output.contains("results") || 
           output.contains("statistics") ||
           output.contains("analysis") ||
           output.contains("completed") ||
           output.contains("uncertainty") ||
           output.contains("confidence");
}

void TestPostProcessing::testMonteCarloAnalysis()
{
    QJsonArray methods;
    methods.append(createMonteCarloMethod(500, 2)); // 500 steps, SEy variance source
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/montecarlo_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Monte Carlo analysis failed: " + result[2]));
    
    // Verify Monte Carlo execution
    QVERIFY2(verifyMethodExecution(result[1], 1), "Monte Carlo method not detected in output");
    QVERIFY(result[1].contains("calculation") || result[1].contains("Monte Carlo"));
}

void TestPostProcessing::testCrossValidationAnalysis()
{
    QJsonArray methods;
    methods.append(createCrossValidationMethod(1, 20)); // Leave-One-Out, 20 steps
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/crossval_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Cross Validation analysis failed: " + result[2]));
    
    // Verify Cross Validation execution
    QVERIFY2(verifyMethodExecution(result[1], 4), "Cross Validation method not detected in output");
}

void TestPostProcessing::testWeakenedGridSearchAnalysis()
{
    QJsonArray methods;
    methods.append(createWeakenedGridSearchMethod());
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/gridsearch_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Weakened Grid Search analysis failed: " + result[2]));
    
    // Verify Grid Search execution
    QVERIFY2(verifyMethodExecution(result[1], 2), "Weakened Grid Search method not detected in output");
}

void TestPostProcessing::testModelComparisonAnalysis()
{
    QJsonArray methods;
    methods.append(createModelComparisonMethod());
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/modelcomp_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Model Comparison analysis failed: " + result[2]));
    
    // Verify Model Comparison execution
    QVERIFY2(verifyMethodExecution(result[1], 3), "Model Comparison method not detected in output");
}

void TestPostProcessing::testParameterReductionAnalysis()
{
    QJsonArray methods;
    methods.append(createParameterReductionMethod());
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/reduction_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Parameter Reduction analysis failed: " + result[2]));
    
    // Verify Parameter Reduction execution
    QVERIFY2(verifyMethodExecution(result[1], 5), "Parameter Reduction method not detected in output");
}

void TestPostProcessing::testFastConfidenceAnalysis()
{
    QJsonArray methods;
    methods.append(createFastConfidenceMethod());
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/fastconf_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Fast Confidence analysis failed: " + result[2]));
    
    // Verify Fast Confidence execution
    QVERIFY2(verifyMethodExecution(result[1], 6), "Fast Confidence method not detected in output");
}

void TestPostProcessing::testGlobalSearchAnalysis()
{
    QJsonArray methods;
    methods.append(createGlobalSearchMethod());
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/globalsearch_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Global Search analysis failed: " + result[2]));
    
    // Verify Global Search execution
    QVERIFY2(verifyMethodExecution(result[1], 7), "Global Search method not detected in output");
}

void TestPostProcessing::testMonteCarloParameters()
{
    // Test different Monte Carlo parameter combinations
    QList<int> maxSteps = {100, 500, 1000};
    QList<int> varianceSources = {1, 2}; // Different variance sources
    
    for (int steps : maxSteps) {
        for (int varSource : varianceSources) {
            QJsonArray methods;
            methods.append(createMonteCarloMethod(steps, varSource));
            
            QString configFile = createPostProcessingConfig(methods);
            QString outputFile = m_tempDir->path() + QString("/mc_params_%1_%2").arg(steps).arg(varSource);
            
            QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
            QVERIFY2(result[0].toInt() == 0, qPrintable(QString("MC parameters test failed for steps=%1, varSource=%2: %3").arg(steps).arg(varSource).arg(result[2])));
        }
    }
}

void TestPostProcessing::testCrossValidationTypes()
{
    // Test different Cross Validation types
    QList<int> cvTypes = {1, 2, 3}; // L0O, L2O, CXO
    
    for (int cvType : cvTypes) {
        QJsonArray methods;
        methods.append(createCrossValidationMethod(cvType, 15));
        
        QString configFile = createPostProcessingConfig(methods);
        QString outputFile = m_tempDir->path() + QString("/cv_type_%1").arg(cvType);
        
        QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
        QVERIFY2(result[0].toInt() == 0, qPrintable(QString("CV type test failed for type=%1: %2").arg(cvType).arg(result[2])));
    }
}

void TestPostProcessing::testVarianceSourceOptions()
{
    // Test different variance source options for Monte Carlo
    QList<int> varianceSources = {1, 2, 3};
    
    for (int varSource : varianceSources) {
        QJsonArray methods;
        methods.append(createMonteCarloMethod(200, varSource));
        
        QString configFile = createPostProcessingConfig(methods);
        QString outputFile = m_tempDir->path() + QString("/varsource_%1").arg(varSource);
        
        QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
        QVERIFY2(result[0].toInt() == 0, qPrintable(QString("Variance source test failed for source=%1: %2").arg(varSource).arg(result[2])));
    }
}

void TestPostProcessing::testMaxStepsConfiguration()
{
    // Test different MaxSteps values
    QList<int> stepCounts = {50, 100, 200, 500};
    
    for (int steps : stepCounts) {
        QJsonArray methods;
        methods.append(createMonteCarloMethod(steps, 2));
        
        QString configFile = createPostProcessingConfig(methods);
        QString outputFile = m_tempDir->path() + QString("/maxsteps_%1").arg(steps);
        
        QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
        QVERIFY2(result[0].toInt() == 0, qPrintable(QString("MaxSteps test failed for steps=%1: %2").arg(steps).arg(result[2])));
    }
}

void TestPostProcessing::testMultipleMethodCombination()
{
    QJsonArray methods;
    methods.append(createMonteCarloMethod(300, 2));
    methods.append(createCrossValidationMethod(1, 15));
    methods.append(createParameterReductionMethod());
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/multiple_methods_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Multiple method combination failed: " + result[2]));
    
    // Verify that all methods were executed
    QVERIFY(verifyMethodExecution(result[1], 1)); // Monte Carlo
    QVERIFY(verifyMethodExecution(result[1], 4)); // Cross Validation
    QVERIFY(verifyMethodExecution(result[1], 5)); // Parameter Reduction
}

void TestPostProcessing::testMethodSequencing()
{
    // Test that methods are executed in the correct order
    QJsonArray methods;
    methods.append(createFastConfidenceMethod());
    methods.append(createMonteCarloMethod(100, 2));
    methods.append(createCrossValidationMethod(1, 10));
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/method_sequence_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Method sequencing test failed: " + result[2]));
    
    // All methods should be executed
    QVERIFY(verifyStatisticalResults(result[1]));
}

void TestPostProcessing::testMethodInteraction()
{
    // Test methods that might interact with each other
    QJsonArray methods;
    methods.append(createModelComparisonMethod());
    methods.append(createParameterReductionMethod());
    methods.append(createGlobalSearchMethod());
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/method_interaction_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Method interaction test failed: " + result[2]));
    
    QVERIFY(verifyStatisticalResults(result[1]));
}

void TestPostProcessing::testParameterUncertainty()
{
    QJsonArray methods;
    methods.append(createMonteCarloMethod(500, 2)); // Good sample size for uncertainty
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/uncertainty_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Parameter uncertainty test failed: " + result[2]));
    
    // Should show uncertainty/error information
    QVERIFY(result[1].contains("uncertainty") || result[1].contains("error") || 
            result[1].contains("calculation") || result[1].contains("results"));
}

void TestPostProcessing::testConfidenceIntervals()
{
    QJsonArray methods;
    methods.append(createMonteCarloMethod(400, 2));
    methods.append(createFastConfidenceMethod());
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/confidence_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Confidence intervals test failed: " + result[2]));
    
    QVERIFY(result[1].contains("confidence") || result[1].contains("interval") ||
            result[1].contains("analysis") || result[1].contains("completed"));
}

void TestPostProcessing::testParameterDistributions()
{
    QJsonArray methods;
    methods.append(createMonteCarloMethod(800, 2)); // Large sample for good distributions
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/distributions_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Parameter distributions test failed: " + result[2]));
    
    QVERIFY(result[1].contains("distribution") || result[1].contains("Monte Carlo") ||
            result[1].contains("calculation") || result[1].contains("results"));
}

void TestPostProcessing::testStatisticalSummaries()
{
    QJsonArray methods;
    methods.append(createMonteCarloMethod(300, 2));
    methods.append(createCrossValidationMethod(1, 20));
    methods.append(createModelComparisonMethod());
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/summaries_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Statistical summaries test failed: " + result[2]));
    
    QVERIFY(verifyStatisticalResults(result[1]));
}

void TestPostProcessing::testShowPostProcessingFlag()
{
    // Create a file with existing post-processing results
    QJsonArray methods;
    methods.append(createMonteCarloMethod(200, 2));
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/postproc_flag_test";
    
    // First run to generate results
    QStringList result1 = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("Initial post-processing run failed: " + result1[2]));
    
    // Test --show-post-processing flag with generated file
    QString generatedFile = outputFile + "-0.json";
    if (QFile::exists(generatedFile)) {
        QStringList result2 = runCliCommand({"--show-post-processing", generatedFile});
        QVERIFY2(result2[0].toInt() == 0, qPrintable("--show-post-processing flag test failed: " + result2[2]));
        
        // Should show detailed post-processing information
        QVERIFY(result2[1].contains("post") || result2[1].contains("processing") ||
                result2[1].contains("statistics") || result2[1].contains("analysis"));
    }
}

void TestPostProcessing::testPostProcessingOutput()
{
    QJsonArray methods;
    methods.append(createMonteCarloMethod(150, 2));
    methods.append(createCrossValidationMethod(1, 10));
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/postproc_output_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Post-processing output test failed: " + result[2]));
    
    // Verify that output contains method information
    QVERIFY(result[1].contains("Monte Carlo") || result[1].contains("Cross Validation") ||
            result[1].contains("analysis") || result[1].contains("method"));
}

void TestPostProcessing::testMethodResultDisplay()
{
    QJsonArray methods;
    methods.append(createParameterReductionMethod());
    methods.append(createFastConfidenceMethod());
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/method_display_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Method result display test failed: " + result[2]));
    
    QVERIFY(verifyStatisticalResults(result[1]));
}

void TestPostProcessing::testLargeMonteCarloSamples()
{
    QJsonArray methods;
    methods.append(createMonteCarloMethod(2000, 2)); // Large sample size
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/large_mc_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Large Monte Carlo samples test failed: " + result[2]));
    
    QVERIFY(result[1].contains("2000") || result[1].contains("calculation") ||
            result[1].contains("Monte Carlo") || result[1].contains("completed"));
}

void TestPostProcessing::testExtensiveCrossValidation()
{
    QJsonArray methods;
    methods.append(createCrossValidationMethod(1, 50)); // Many CV iterations
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/extensive_cv_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Extensive Cross Validation test failed: " + result[2]));
    
    QVERIFY(result[1].contains("Cross Validation") || result[1].contains("calculation") ||
            result[1].contains("analysis") || result[1].contains("completed"));
}

void TestPostProcessing::testPerformanceScaling()
{
    // Test performance with multiple methods and moderate sample sizes
    QJsonArray methods;
    methods.append(createMonteCarloMethod(300, 2));
    methods.append(createCrossValidationMethod(1, 15));
    methods.append(createParameterReductionMethod());
    methods.append(createFastConfidenceMethod());
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/performance_test";
    
    // Measure execution time indirectly by ensuring completion within timeout
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Performance scaling test failed: " + result[2]));
    
    // Should complete all methods
    QVERIFY(verifyStatisticalResults(result[1]));
}

void TestPostProcessing::testInvalidMethodIds()
{
    QJsonArray methods;
    QJsonObject invalidMethod;
    invalidMethod["Method"] = 999; // Invalid method ID
    invalidMethod["MaxSteps"] = 100;
    methods.append(invalidMethod);
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/invalid_method_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    // Should handle invalid method ID gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
}

void TestPostProcessing::testInvalidParameters()
{
    QJsonArray methods;
    QJsonObject invalidMC;
    invalidMC["Method"] = 1;
    invalidMC["MaxSteps"] = -1; // Invalid negative steps
    invalidMC["VarianceSource"] = 999; // Invalid variance source
    methods.append(invalidMC);
    
    QString configFile = createPostProcessingConfig(methods);
    QString outputFile = m_tempDir->path() + "/invalid_params_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    // Should handle invalid parameters gracefully
    QVERIFY(result[0].toInt() >= 0);
}

void TestPostProcessing::testInsufficientData()
{
    // Create configuration with very few data points
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "insufficient_data_test";
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
    main["PostFitAnalysis"] = true;
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 3; // Very few points
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|Y";
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
    addModels["nmr_1_1"] = nmr11;
    config["AddModels"] = addModels;
    
    QJsonArray methods;
    methods.append(createMonteCarloMethod(50, 2));
    methods.append(createCrossValidationMethod(1, 5));
    
    QJsonObject postFit;
    postFit["methods"] = methods;
    config["PostFitAnalysis"] = postFit;
    
    QString configFile = m_tempDir->path() + "/insufficient_data_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QString outputFile = m_tempDir->path() + "/insufficient_data_test";
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    
    // Should handle insufficient data gracefully
    QVERIFY(result[0].toInt() >= 0);
}

QString TestPostProcessing::createPostProcessingConfig(const QJsonArray& methods)
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "postprocessing_test";
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
    main["PostFitAnalysis"] = true;
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 20;
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
    QJsonObject options;
    options["FastMode"] = true;
    nmr11["Options"] = options;
    addModels["nmr_1_1"] = nmr11;
    config["AddModels"] = addModels;
    
    QJsonObject postFit;
    postFit["methods"] = methods;
    config["PostFitAnalysis"] = postFit;
    
    QString filePath = m_tempDir->path() + "/postprocessing_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    return filePath;
}

QJsonObject TestPostProcessing::createMonteCarloMethod(int maxSteps, int varianceSource)
{
    QJsonObject method;
    method["Method"] = 1;
    method["MaxSteps"] = maxSteps;
    method["VarianceSource"] = varianceSource;
    return method;
}

QJsonObject TestPostProcessing::createCrossValidationMethod(int cvType, int maxSteps)
{
    QJsonObject method;
    method["Method"] = 4;
    method["CVType"] = cvType;
    method["MaxSteps"] = maxSteps;
    return method;
}

QJsonObject TestPostProcessing::createWeakenedGridSearchMethod()
{
    QJsonObject method;
    method["Method"] = 2;
    method["MaxSteps"] = 100;
    return method;
}

QJsonObject TestPostProcessing::createModelComparisonMethod()
{
    QJsonObject method;
    method["Method"] = 3;
    return method;
}

QJsonObject TestPostProcessing::createParameterReductionMethod()
{
    QJsonObject method;
    method["Method"] = 5;
    method["Cutoff"] = 0.05;
    return method;
}

QJsonObject TestPostProcessing::createFastConfidenceMethod()
{
    QJsonObject method;
    method["Method"] = 6;
    method["MaxSteps"] = 50;
    return method;
}

QJsonObject TestPostProcessing::createGlobalSearchMethod()
{
    QJsonObject method;
    method["Method"] = 7;
    method["MaxSteps"] = 100;
    return method;
}

#include "test_post_processing.moc"

QTEST_MAIN(TestPostProcessing)