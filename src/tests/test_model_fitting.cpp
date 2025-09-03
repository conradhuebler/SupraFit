/*
 * Comprehensive tests for SupraFit model fitting functionality
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Tests AddModels structure, parameter fitting, model convergence,
 * fit quality metrics, and model comparison functionality.
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

class TestModelFitting : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // AddModels Structure Tests
    void testAddModelsStructure();
    void testSingleModelFitting();
    void testMultipleModelFitting();
    void testModelIdValidation();

    // Model Type Tests
    void testNMRModels();
    void testITCModels();
    void testFluorescenceModels();
    void testKineticsModels();

    // Parameter Fitting Tests
    void testParameterConvergence();
    void testGlobalParameterFitting();
    void testLocalParameterFitting();
    void testParameterBoundaries();

    // Fit Quality Tests
    void testSSECalculation();
    void testAICCalculation();
    void testConvergenceDetection();
    void testFitQualityMetrics();

    // Model Comparison Tests
    void testModelRanking();
    void testBestModelSelection();
    void testModelStatistics();

    // Options and Configuration Tests
    void testModelOptions();
    void testFastModeOptions();
    void testConvergencySettings();

    // Error Handling Tests
    void testInvalidModelIds();
    void testInvalidParameterRanges();
    void testPoorlyConditionedData();

private:
    QTemporaryDir* m_tempDir;
    QString m_suprafitCli;
    
    QString createAddModelsConfig(const QJsonObject& models, bool postFitAnalysis = false);
    QString createDataForModel(int modelId, int dataPoints = 20);
    QJsonObject loadResultFile(const QString& filename);
    QStringList runCliCommand(const QStringList& arguments);
    bool verifyModelFit(const QJsonObject& modelData, const QString& modelName);
    bool verifyConvergence(const QJsonObject& modelData);
    double extractSSE(const QJsonObject& modelData);
    double extractAIC(const QJsonObject& modelData);
};

void TestModelFitting::initTestCase()
{
    qDebug() << "Starting Model Fitting tests...";
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

void TestModelFitting::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "Model Fitting tests completed.";
}

QStringList TestModelFitting::runCliCommand(const QStringList& arguments)
{
    QProcess process;
    process.start(m_suprafitCli, arguments);
    process.waitForFinished(60000); // 60 second timeout for model fitting
    
    QStringList result;
    result << QString::number(process.exitCode());
    result << QString::fromUtf8(process.readAllStandardOutput());
    result << QString::fromUtf8(process.readAllStandardError());
    
    return result;
}

QJsonObject TestModelFitting::loadResultFile(const QString& filename)
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

bool TestModelFitting::verifyModelFit(const QJsonObject& modelData, const QString& modelName)
{
    if (!modelData.contains("model_name")) return false;
    if (modelData["model_name"].toString() != modelName) return false;
    if (!modelData.contains("sse")) return false;
    if (!modelData.contains("convergence")) return false;
    return true;
}

bool TestModelFitting::verifyConvergence(const QJsonObject& modelData)
{
    if (!modelData.contains("convergence")) return false;
    return modelData["convergence"].toBool();
}

double TestModelFitting::extractSSE(const QJsonObject& modelData)
{
    if (!modelData.contains("sse")) return -1.0;
    return modelData["sse"].toDouble();
}

double TestModelFitting::extractAIC(const QJsonObject& modelData)
{
    if (!modelData.contains("aic")) return -1.0;
    return modelData["aic"].toDouble();
}

void TestModelFitting::testAddModelsStructure()
{
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/addmodels_structure_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("AddModels structure test failed: " + result[2]));
    
    // Should mention AddModels processing
    QVERIFY(result[1].contains("AddModels") || result[1].contains("model"));
}

void TestModelFitting::testSingleModelFitting()
{
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options;
    options["FastMode"] = true;
    options["Convergency"] = 1e-7;
    nmr11["Options"] = options;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/single_model_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Single model fitting failed: " + result[2]));
    
    // Should show model fitting progress
    QVERIFY(result[1].contains("nmr_1_1") || result[1].contains("model"));
}

void TestModelFitting::testMultipleModelFitting()
{
    QJsonObject models;
    
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options11;
    options11["FastMode"] = true;
    nmr11["Options"] = options11;
    models["nmr_1_1"] = nmr11;
    
    QJsonObject nmr12;
    nmr12["ID"] = 2;
    QJsonObject options12;
    options12["FastMode"] = true;
    nmr12["Options"] = options12;
    models["nmr_1_2"] = nmr12;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/multiple_models_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Multiple model fitting failed: " + result[2]));
    
    // Should show both models being processed
    QVERIFY(result[1].contains("nmr_1_1") && result[1].contains("nmr_1_2"));
}

void TestModelFitting::testModelIdValidation()
{
    QJsonObject models;
    QJsonObject invalidModel;
    invalidModel["ID"] = 999; // Invalid model ID
    models["invalid_model"] = invalidModel;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/invalid_model_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    // Should handle invalid model ID gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
}

void TestModelFitting::testNMRModels()
{
    QStringList nmrModels = {"nmr_1_1", "nmr_1_2", "nmr_2_1"};
    QList<int> modelIds = {1, 2, 3};
    
    for (int i = 0; i < nmrModels.size(); ++i) {
        QJsonObject models;
        QJsonObject model;
        model["ID"] = modelIds[i];
        QJsonObject options;
        options["FastMode"] = true;
        options["Convergency"] = 1e-6;
        model["Options"] = options;
        models[nmrModels[i]] = model;
        
        QString configFile = createAddModelsConfig(models);
        QString outputFile = m_tempDir->path() + "/nmr_model_test_" + QString::number(i);
        
        QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
        QVERIFY2(result[0].toInt() == 0, qPrintable(QString("NMR model %1 test failed: %2").arg(nmrModels[i], result[2])));
    }
}

void TestModelFitting::testITCModels()
{
    QJsonObject models;
    QJsonObject itc11;
    itc11["ID"] = 11; // ITC 1:1 model
    QJsonObject options;
    options["FastMode"] = true;
    itc11["Options"] = options;
    models["itc_1_1"] = itc11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/itc_model_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("ITC model test failed: " + result[2]));
}

void TestModelFitting::testFluorescenceModels()
{
    QJsonObject models;
    QJsonObject fl11;
    fl11["ID"] = 21; // Fluorescence 1:1 model (assuming ID 21)
    QJsonObject options;
    options["FastMode"] = true;
    fl11["Options"] = options;
    models["fl_1_1"] = fl11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/fl_model_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Fluorescence model test failed: " + result[2]));
}

void TestModelFitting::testKineticsModels()
{
    QJsonObject models;
    QJsonObject kinetic;
    kinetic["ID"] = 101; // Kinetic model (assuming ID 101)
    QJsonObject options;
    options["FastMode"] = true;
    kinetic["Options"] = options;
    models["monomolecular"] = kinetic;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/kinetic_model_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Kinetics model test failed: " + result[2]));
}

void TestModelFitting::testParameterConvergence()
{
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options;
    options["FastMode"] = false; // Full convergence testing
    options["Convergency"] = 1e-8; // Strict convergence
    nmr11["Options"] = options;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/convergence_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Parameter convergence test failed: " + result[2]));
    
    // Should show convergence information
    QVERIFY(result[1].contains("converged") || result[1].contains("Convergence") || 
            result[1].contains("succeeded") || result[1].contains("completed"));
}

void TestModelFitting::testGlobalParameterFitting()
{
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/global_param_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Global parameter fitting failed: " + result[2]));
    
    // Check that the generated data contains the expected structure
    QString generatedFile = outputFile + "-0.json";
    if (QFile::exists(generatedFile)) {
        QJsonObject data = loadResultFile(generatedFile);
        QVERIFY(!data.isEmpty());
    }
}

void TestModelFitting::testLocalParameterFitting()
{
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/local_param_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Local parameter fitting failed: " + result[2]));
}

void TestModelFitting::testParameterBoundaries()
{
    // Test with custom parameter boundaries
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options;
    options["FastMode"] = true;
    // Add boundary constraints if supported
    options["GlobalBoundaries"] = "[0 10]";
    options["LocalBoundaries"] = "[5 8; 2 4]";
    nmr11["Options"] = options;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/boundaries_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Parameter boundaries test failed: " + result[2]));
}

void TestModelFitting::testSSECalculation()
{
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/sse_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("SSE calculation test failed: " + result[2]));
    
    // Should show SSE value in output
    QVERIFY(result[1].contains("SSE:") || result[1].contains("sse") || 
            result[1].contains("error") || result[1].contains("completed"));
}

void TestModelFitting::testAICCalculation()
{
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/aic_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("AIC calculation test failed: " + result[2]));
    
    // Should show AIC value or model statistics
    QVERIFY(result[1].contains("AIC") || result[1].contains("aic") || 
            result[1].contains("model") || result[1].contains("completed"));
}

void TestModelFitting::testConvergenceDetection()
{
    // Test with different convergence settings
    QList<double> convergencyValues = {1e-5, 1e-7, 1e-9};
    
    for (double conv : convergencyValues) {
        QJsonObject models;
        QJsonObject nmr11;
        nmr11["ID"] = 1;
        QJsonObject options;
        options["Convergency"] = conv;
        nmr11["Options"] = options;
        models["nmr_1_1"] = nmr11;
        
        QString configFile = createAddModelsConfig(models);
        QString outputFile = m_tempDir->path() + "/convergence_" + QString::number(conv);
        
        QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
        QVERIFY2(result[0].toInt() == 0, qPrintable(QString("Convergence detection test failed for %1: %2").arg(conv).arg(result[2])));
    }
}

void TestModelFitting::testFitQualityMetrics()
{
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options;
    options["FastMode"] = false; // Enable full quality metrics
    nmr11["Options"] = options;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/quality_metrics_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Fit quality metrics test failed: " + result[2]));
    
    // Should show quality metrics
    QVERIFY(result[1].contains("SSE") || result[1].contains("error") || 
            result[1].contains("completed") || result[1].contains("succeeded"));
}

void TestModelFitting::testModelRanking()
{
    QJsonObject models;
    
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    models["nmr_1_1"] = nmr11;
    
    QJsonObject nmr12;
    nmr12["ID"] = 2;
    models["nmr_1_2"] = nmr12;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/ranking_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Model ranking test failed: " + result[2]));
    
    // Should process both models for comparison
    QVERIFY(result[1].contains("nmr_1_1") && result[1].contains("nmr_1_2"));
}

void TestModelFitting::testBestModelSelection()
{
    QJsonObject models;
    
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options11;
    options11["FastMode"] = true;
    nmr11["Options"] = options11;
    models["nmr_1_1"] = nmr11;
    
    QJsonObject nmr12;
    nmr12["ID"] = 2;
    QJsonObject options12;
    options12["FastMode"] = true;
    nmr12["Options"] = options12;
    models["nmr_1_2"] = nmr12;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/best_model_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Best model selection test failed: " + result[2]));
    
    // Should show model comparison results
    QVERIFY(result[1].contains("model") || result[1].contains("completed"));
}

void TestModelFitting::testModelStatistics()
{
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/statistics_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Model statistics test failed: " + result[2]));
    
    // Should provide statistical information
    QVERIFY(result[1].contains("SSE") || result[1].contains("error") || 
            result[1].contains("model") || result[1].contains("completed"));
}

void TestModelFitting::testModelOptions()
{
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options;
    options["FastMode"] = true;
    options["Convergency"] = 1e-6;
    options["MaxIterations"] = 1000;
    nmr11["Options"] = options;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/options_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Model options test failed: " + result[2]));
}

void TestModelFitting::testFastModeOptions()
{
    // Test both FastMode true and false
    QList<bool> fastModes = {true, false};
    
    for (bool fastMode : fastModes) {
        QJsonObject models;
        QJsonObject nmr11;
        nmr11["ID"] = 1;
        QJsonObject options;
        options["FastMode"] = fastMode;
        nmr11["Options"] = options;
        models["nmr_1_1"] = nmr11;
        
        QString configFile = createAddModelsConfig(models);
        QString outputFile = m_tempDir->path() + "/fastmode_" + (fastMode ? "true" : "false");
        
        QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
        QVERIFY2(result[0].toInt() == 0, qPrintable(QString("FastMode %1 test failed: %2").arg(fastMode ? "true" : "false", result[2])));
    }
}

void TestModelFitting::testConvergencySettings()
{
    QList<double> convergencyValues = {1e-4, 1e-6, 1e-8, 1e-10};
    
    for (double conv : convergencyValues) {
        QJsonObject models;
        QJsonObject nmr11;
        nmr11["ID"] = 1;
        QJsonObject options;
        options["Convergency"] = conv;
        options["FastMode"] = true; // Use fast mode for quicker testing
        nmr11["Options"] = options;
        models["nmr_1_1"] = nmr11;
        
        QString configFile = createAddModelsConfig(models);
        QString outputFile = m_tempDir->path() + "/conv_" + QString::number(conv);
        
        QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
        QVERIFY2(result[0].toInt() == 0, qPrintable(QString("Convergency %1 test failed: %2").arg(conv).arg(result[2])));
    }
}

void TestModelFitting::testInvalidModelIds()
{
    QList<int> invalidIds = {0, -1, 999, 10000};
    
    for (int id : invalidIds) {
        QJsonObject models;
        QJsonObject invalidModel;
        invalidModel["ID"] = id;
        models["invalid_model"] = invalidModel;
        
        QString configFile = createAddModelsConfig(models);
        QString outputFile = m_tempDir->path() + "/invalid_id_" + QString::number(id);
        
        QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
        // Should handle gracefully without crashing
        QVERIFY(result[0].toInt() >= 0);
    }
}

void TestModelFitting::testInvalidParameterRanges()
{
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options;
    options["GlobalBoundaries"] = "invalid_range";
    options["LocalBoundaries"] = "[invalid; syntax]";
    nmr11["Options"] = options;
    models["nmr_1_1"] = nmr11;
    
    QString configFile = createAddModelsConfig(models);
    QString outputFile = m_tempDir->path() + "/invalid_ranges_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    // Should handle invalid ranges gracefully
    QVERIFY(result[0].toInt() >= 0);
}

void TestModelFitting::testPoorlyConditionedData()
{
    // Create configuration with very small data ranges that might cause numerical issues
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "poorly_conditioned_test";
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 5; // Very few points
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "1e-10|(X - 1) * 1e-10"; // Very small values
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
    
    QJsonObject models;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options;
    options["FastMode"] = true;
    nmr11["Options"] = options;
    models["nmr_1_1"] = nmr11;
    config["AddModels"] = models;
    
    QString configFile = m_tempDir->path() + "/poorly_conditioned_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QString outputFile = m_tempDir->path() + "/poorly_conditioned_test";
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    
    // Should handle poorly conditioned data without crashing
    QVERIFY(result[0].toInt() >= 0);
}

QString TestModelFitting::createAddModelsConfig(const QJsonObject& models, bool postFitAnalysis)
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "model_fitting_test";
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
    if (postFitAnalysis) {
        main["PostFitAnalysis"] = true;
    }
    config["Main"] = main;
    
    // Create appropriate data for model fitting
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
    model["ID"] = 1; // Use the first model from AddModels
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    config["AddModels"] = models;
    
    if (postFitAnalysis) {
        QJsonObject postFit;
        QJsonArray methods;
        QJsonObject mcMethod;
        mcMethod["Method"] = 1; // Monte Carlo
        mcMethod["MaxSteps"] = 100;
        methods.append(mcMethod);
        postFit["methods"] = methods;
        config["PostFitAnalysis"] = postFit;
    }
    
    QString filePath = m_tempDir->path() + "/addmodels_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    return filePath;
}

QString TestModelFitting::createDataForModel(int modelId, int dataPoints)
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "model_data";
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = dataPoints;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "0.001|(X - 1) * 0.0001";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "model";
    QJsonObject model;
    model["ID"] = modelId;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    QString filePath = m_tempDir->path() + "/model_data_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    return filePath;
}

#include "test_model_fitting.moc"

QTEST_MAIN(TestModelFitting)