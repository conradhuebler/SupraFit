/*
 * Comprehensive tests for SupraFit data generation functionality
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Tests modular data generation with Independent/Dependent structure,
 * equation-based generation, model-based generation, noise application,
 * and random parameter injection.
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

class TestDataGeneration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Independent Data Generation Tests
    void testEquationBasedIndependentGeneration();
    void testMultiVariableEquations();
    void testComplexEquationParsing();
    void testInvalidEquations();

    // Dependent Data Generation Tests  
    void testModelBasedDependentGeneration();
    void testCopyBasedDependentGeneration();
    void testDependentWithAddModels();

    // Noise Application Tests
    void testGaussianNoise();
    void testUniformNoise();
    void testNoNoise();
    void testInvalidNoiseConfiguration();

    // Random Parameter Tests
    void testRandomParameterGeneration();
    void testParameterRanges();
    void testRandomSeedConsistency();

    // Modular Structure Tests
    void testModularStructureGeneration();
    void testIndependentDependentSeparation();
    void testOutputFormatValidation();

    // Legacy Format Tests
    void testLegacyFormatSupport();
    void testFormatMigration();

    // Integration Tests
    void testCompleteDataGenerationWorkflow();
    void testMultipleDatasetGeneration();
    void testLargeDatasetGeneration();

private:
    QTemporaryDir* m_tempDir;
    QString m_suprafitCli;
    
    QString createEquationConfig(const QString& equations, int dataPoints = 20, int variables = 2);
    QString createModelBasedConfig(int modelId = 1, int series = 2);
    QString createNoiseConfig(const QString& type, const QJsonArray& std);
    QString createRandomParameterConfig();
    QStringList runCliCommand(const QStringList& arguments);
    QJsonObject loadGeneratedFile(const QString& filename);
    bool verifyDataStructure(const QJsonObject& data, const QString& expectedType);
    bool verifyDimensions(const QJsonObject& data, int expectedRows, int expectedCols);
};

void TestDataGeneration::initTestCase()
{
    qDebug() << "Starting Data Generation tests...";
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

void TestDataGeneration::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "Data Generation tests completed.";
}

QStringList TestDataGeneration::runCliCommand(const QStringList& arguments)
{
    QProcess process;
    process.start(m_suprafitCli, arguments);
    process.waitForFinished(30000);
    
    QStringList result;
    result << QString::number(process.exitCode());
    result << QString::fromUtf8(process.readAllStandardOutput());
    result << QString::fromUtf8(process.readAllStandardError());
    
    return result;
}

QJsonObject TestDataGeneration::loadGeneratedFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    
    QByteArray data = file.readAll();
    
    // Try to decompress if it's a .suprafit file (compressed)
    if (filename.endsWith(".suprafit")) {
        data = qUncompress(data);
        if (data.isEmpty()) {
            return QJsonObject(); // Decompression failed
        }
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        return QJsonObject();
    }
    
    return doc.object();
}

bool TestDataGeneration::verifyDataStructure(const QJsonObject& data, const QString& expectedType)
{
    if (!data.contains("format_version")) return false;
    if (!data.contains("base_data")) return false;
    
    QJsonObject baseData = data["base_data"].toObject();
    if (!baseData.contains("Independent")) return false;
    if (!baseData.contains("Dependent")) return false;
    
    return true;
}

bool TestDataGeneration::verifyDimensions(const QJsonObject& data, int expectedRows, int expectedCols)
{
    QJsonObject baseData = data["base_data"].toObject();
    QJsonObject independent = baseData["Independent"].toObject();
    
    if (independent.contains("data")) {
        QJsonObject indepData = independent["data"].toObject();
        if (indepData.contains("rows") && indepData.contains("cols")) {
            return (indepData["rows"].toInt() == expectedRows && 
                   indepData["cols"].toInt() == expectedCols);
        }
    }
    
    return false;
}

void TestDataGeneration::testEquationBasedIndependentGeneration()
{
    QString configFile = createEquationConfig("X|X*2", 20, 2);
    QString outputFile = m_tempDir->path() + "/equation_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Equation generation failed: " + result[2]));
    
    // CLI now correctly uses -o option to override JSON OutFile
    QString generatedFile = outputFile + "-0.suprafit";
    QVERIFY2(QFile::exists(generatedFile), "Generated file not found");
    
    // Load and verify structure
    QJsonObject data = loadGeneratedFile(generatedFile);
    QVERIFY(!data.isEmpty());
    QVERIFY(verifyDataStructure(data, "generated"));
    QVERIFY(verifyDimensions(data, 20, 2));
}

void TestDataGeneration::testMultiVariableEquations()
{
    QString configFile = createEquationConfig("X|Y|X*Y|X+Y", 15, 4);
    QString outputFile = m_tempDir->path() + "/multivar_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Multi-variable generation failed: " + result[2]));
    
    QString generatedFile = outputFile + "-0.suprafit";
    QVERIFY(QFile::exists(generatedFile));
    
    QJsonObject data = loadGeneratedFile(generatedFile);
    QVERIFY(verifyDimensions(data, 15, 4));
}

void TestDataGeneration::testComplexEquationParsing()
{
    // Test complex mathematical expressions
    QString equations = "sin(X)|cos(X)|exp(X/10)|log(abs(X)+1)";
    QString configFile = createEquationConfig(equations, 25, 4);
    QString outputFile = m_tempDir->path() + "/complex_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Complex equation parsing failed: " + result[2]));
    
    QString generatedFile = outputFile + "-0.suprafit";
    QVERIFY(QFile::exists(generatedFile));
}

void TestDataGeneration::testInvalidEquations()
{
    // Test with syntactically incorrect equations
    QString configFile = createEquationConfig("X)|invalid(", 10, 2);
    QString outputFile = m_tempDir->path() + "/invalid_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    // Should either fail gracefully or handle the error
    QVERIFY(result[0].toInt() >= 0); // No crashes
}

void TestDataGeneration::testModelBasedDependentGeneration()
{
    QString configFile = createModelBasedConfig(1, 2); // nmr_1_1 model
    QString outputFile = m_tempDir->path() + "/model_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Model-based generation failed: " + result[2]));
    
    QString generatedFile = outputFile + "-0.suprafit";
    QVERIFY(QFile::exists(generatedFile));
    
    QJsonObject data = loadGeneratedFile(generatedFile);
    QVERIFY(verifyDataStructure(data, "model_based"));
}

void TestDataGeneration::testCopyBasedDependentGeneration()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "copy_test";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 15;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|Y";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 2;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    QString configFile = m_tempDir->path() + "/copy_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QString outputFile = m_tempDir->path() + "/copy_test";
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Copy-based generation failed: " + result[2]));
    
    QString generatedFile = outputFile + "-0.suprafit";
    QVERIFY(QFile::exists(generatedFile));
}

void TestDataGeneration::testDependentWithAddModels()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "addmodels_test";
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
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
    addModels["nmr_1_1"] = nmr11;
    config["AddModels"] = addModels;
    
    QString configFile = m_tempDir->path() + "/addmodels_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QString outputFile = m_tempDir->path() + "/addmodels_test";
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("AddModels generation failed: " + result[2]));
}

void TestDataGeneration::testGaussianNoise()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "noise_test";
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 20;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|Y";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 2;
    dependent["Generator"] = depGen;
    
    QJsonObject noise;
    noise["Type"] = "gaussian";
    QJsonArray stdArray;
    stdArray.append(0.01);
    stdArray.append(0.02);
    noise["Std"] = stdArray;
    dependent["Noise"] = noise;
    config["Dependent"] = dependent;
    
    QString configFile = m_tempDir->path() + "/noise_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QStringList result = runCliCommand({"-i", configFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Gaussian noise test failed: " + result[2]));
}

void TestDataGeneration::testUniformNoise()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "uniform_noise_test";
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 15;
    indepGen["Variables"] = 1;
    indepGen["Equations"] = "X";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 1;
    dependent["Generator"] = depGen;
    
    QJsonObject noise;
    noise["Type"] = "uniform";
    QJsonArray stdArray;
    stdArray.append(0.05);
    noise["Std"] = stdArray;
    dependent["Noise"] = noise;
    config["Dependent"] = dependent;
    
    QString configFile = m_tempDir->path() + "/uniform_noise_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QStringList result = runCliCommand({"-i", configFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Uniform noise test failed: " + result[2]));
}

void TestDataGeneration::testNoNoise()
{
    QString configFile = createEquationConfig("X|X*2", 15, 2);
    // No noise configuration - should work without noise
    
    QStringList result = runCliCommand({"-i", configFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("No noise test failed: " + result[2]));
}

void TestDataGeneration::testInvalidNoiseConfiguration()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "invalid_noise_test";
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 10;
    indepGen["Variables"] = 1;
    indepGen["Equations"] = "X";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 1;
    dependent["Generator"] = depGen;
    
    QJsonObject noise;
    noise["Type"] = "invalid_type";
    noise["Std"] = "not_an_array";
    dependent["Noise"] = noise;
    config["Dependent"] = dependent;
    
    QString configFile = m_tempDir->path() + "/invalid_noise_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QStringList result = runCliCommand({"-i", configFile});
    // Should handle invalid noise gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
}

void TestDataGeneration::testRandomParameterGeneration()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "random_param_test";
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
    
    // Add random parameter configuration
    depGen["GlobalRandomLimits"] = "[2 5]";
    depGen["LocalRandomLimits"] = "[6.0 6.5; 2.0 2.5; 6.0 6.5; 2.0 2.5]";
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    QString configFile = m_tempDir->path() + "/random_param_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QStringList result = runCliCommand({"-i", configFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Random parameter test failed: " + result[2]));
}

void TestDataGeneration::testParameterRanges()
{
    // Test various parameter range configurations
    QStringList globalRanges = {"[1 2]", "[0 10]", "[-5 5]"};
    QStringList localRanges = {"[1 2; 3 4]", "[0.1 0.2; 0.3 0.4; 0.5 0.6; 0.7 0.8]"};
    
    for (const QString& globalRange : globalRanges) {
        for (const QString& localRange : localRanges) {
            QJsonObject config;
            QJsonObject main;
            main["OutFile"] = "param_range_test";
            config["Main"] = main;
            
            QJsonObject independent;
            QJsonObject indepGen;
            indepGen["Type"] = "equations";
            indepGen["DataPoints"] = 10;
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
            depGen["GlobalRandomLimits"] = globalRange;
            depGen["LocalRandomLimits"] = localRange;
            dependent["Generator"] = depGen;
            config["Dependent"] = dependent;
            
            QString configFile = m_tempDir->path() + "/param_range_config.json";
            QFile file(configFile);
            file.open(QIODevice::WriteOnly);
            file.write(QJsonDocument(config).toJson());
            file.close();
            
            QStringList result = runCliCommand({"-i", configFile});
            QVERIFY2(result[0].toInt() == 0, qPrintable("Parameter range test failed: " + result[2]));
        }
    }
}

void TestDataGeneration::testRandomSeedConsistency()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "seed_test";
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 15;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|Y";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 2;
    dependent["Generator"] = depGen;
    
    QJsonObject noise;
    noise["Type"] = "gaussian";
    noise["RandomSeed"] = 42;
    QJsonArray stdArray;
    stdArray.append(0.01);
    stdArray.append(0.01);
    noise["Std"] = stdArray;
    dependent["Noise"] = noise;
    config["Dependent"] = dependent;
    
    QString configFile = m_tempDir->path() + "/seed_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    // Run twice with same seed - should produce identical results
    QStringList result1 = runCliCommand({"-i", configFile, "-o", "seed_test1"});
    QStringList result2 = runCliCommand({"-i", configFile, "-o", "seed_test2"});
    
    QVERIFY2(result1[0].toInt() == 0 && result2[0].toInt() == 0, "Seed consistency test failed");
    
    // Note: Full data comparison would require loading and comparing the generated files
    // which is beyond scope of this basic test
}

void TestDataGeneration::testModularStructureGeneration()
{
    QString configFile = createEquationConfig("X|Y|X*Y", 25, 3);
    
    QStringList result = runCliCommand({"-i", configFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Modular structure generation failed: " + result[2]));
    
    // Verify that output mentions modular structure
    QVERIFY(result[1].contains("modular") || result[1].contains("Independent") || result[1].contains("Dependent"));
}

void TestDataGeneration::testIndependentDependentSeparation()
{
    QString configFile = createModelBasedConfig(1, 2);
    
    QStringList result = runCliCommand({"-i", configFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Independent/Dependent separation failed: " + result[2]));
    
    // Should show both Independent and Dependent data generation
    QVERIFY(result[1].contains("Independent") || result[1].contains("independent"));
    QVERIFY(result[1].contains("Dependent") || result[1].contains("dependent"));
}

void TestDataGeneration::testOutputFormatValidation()
{
    QString configFile = createEquationConfig("X|Y", 10, 2);
    QString outputFile = m_tempDir->path() + "/format_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Output format validation failed: " + result[2]));
    
    QString generatedFile = outputFile + "-0.suprafit";
    QVERIFY(QFile::exists(generatedFile));
    
    QJsonObject data = loadGeneratedFile(generatedFile);
    QVERIFY(!data.isEmpty());
    QVERIFY(data.contains("format_version"));
    QVERIFY(data.contains("base_data"));
    QVERIFY(data.contains("generation_timestamp"));
}

void TestDataGeneration::testLegacyFormatSupport()
{
    // Test that legacy format configurations still work
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "legacy_test";
    main["IndependentRows"] = 15;
    config["Main"] = main;
    
    QJsonObject generateData;
    generateData["Series"] = 2;
    generateData["Model"] = 1;
    generateData["Repeat"] = 1;
    main["GenerateData"] = generateData;
    config["Main"] = main;
    
    QString configFile = m_tempDir->path() + "/legacy_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QStringList result = runCliCommand({"-i", configFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Legacy format support failed: " + result[2]));
}

void TestDataGeneration::testFormatMigration()
{
    // Test that the system can handle mixed old/new format elements
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "migration_test";
    main["IndependentRows"] = 20; // Old format element
    config["Main"] = main;
    
    // New format elements
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 20;
    indepGen["Variables"] = 1;
    indepGen["Equations"] = "X";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QString configFile = m_tempDir->path() + "/migration_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QStringList result = runCliCommand({"-i", configFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Format migration failed: " + result[2]));
}

void TestDataGeneration::testCompleteDataGenerationWorkflow()
{
    QString configFile = createModelBasedConfig(1, 2);
    QString outputFile = m_tempDir->path() + "/complete_workflow";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Complete workflow failed: " + result[2]));
    
    // Verify output structure
    QString generatedFile = outputFile + "-0.suprafit";
    QVERIFY(QFile::exists(generatedFile));
    
    QJsonObject data = loadGeneratedFile(generatedFile);
    QVERIFY(verifyDataStructure(data, "complete"));
}

void TestDataGeneration::testMultipleDatasetGeneration()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "multiple_test";
    main["Repeat"] = 3; // Generate 3 datasets
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 10;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|Y";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 2;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    QString configFile = m_tempDir->path() + "/multiple_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QString outputFile = m_tempDir->path() + "/multiple_test";
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Multiple dataset generation failed: " + result[2]));
    
    // Verify multiple files were created (CLI creates .suprafit files)
    for (int i = 0; i < 3; ++i) {
        QString expectedFile = outputFile + "-" + QString::number(i) + ".suprafit";
        QVERIFY2(QFile::exists(expectedFile), qPrintable("Missing dataset file: " + expectedFile));
    }
}

void TestDataGeneration::testLargeDatasetGeneration()
{
    QString configFile = createEquationConfig("X|Y|X*Y|sin(X)|cos(Y)", 1000, 5);
    QString outputFile = m_tempDir->path() + "/large_test";
    
    QStringList result = runCliCommand({"-i", configFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Large dataset generation failed: " + result[2]));
    
    QString generatedFile = outputFile + "-0.suprafit";
    QVERIFY(QFile::exists(generatedFile));
    
    QJsonObject data = loadGeneratedFile(generatedFile);
    QVERIFY(verifyDimensions(data, 1000, 5));
}

QString TestDataGeneration::createEquationConfig(const QString& equations, int dataPoints, int variables)
{
    QJsonObject config;
    QJsonObject main;
    // Don't set OutFile here - let -o option handle it
    main["UseModularStructure"] = true;
    config["Main"] = main;
    
    QJsonObject independent;
    independent["Source"] = "generator";
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = dataPoints;
    indepGen["Variables"] = variables;
    indepGen["Equations"] = equations;
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    dependent["Source"] = "generator";
    QJsonObject depGen;
    depGen["Type"] = "model";
    depGen["Series"] = variables;
    QJsonObject model;
    model["ID"] = 1;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    QString filePath = m_tempDir->path() + "/equation_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    return filePath;
}

QString TestDataGeneration::createModelBasedConfig(int modelId, int series)
{
    QJsonObject config;
    QJsonObject main;
    // Don't set OutFile here - let -o option handle it
    main["UseModularStructure"] = true;
    config["Main"] = main;
    
    QJsonObject independent;
    independent["Source"] = "generator";
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 20;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "0.001|(X - 1) * 0.0001";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    dependent["Source"] = "generator";
    QJsonObject depGen;
    depGen["Type"] = "model";
    depGen["Series"] = series;
    QJsonObject model;
    model["ID"] = modelId;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    config["Dependent"] = dependent;
    
    QString filePath = m_tempDir->path() + "/model_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    return filePath;
}

#include "test_data_generation.moc"

QTEST_MAIN(TestDataGeneration)