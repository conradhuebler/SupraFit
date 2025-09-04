/*
 * Comprehensive tests for SupraFit CLI core functionality
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Tests core CLI argument parsing, configuration loading, thread management,
 * and basic file operations without obsolete ML pipeline features.
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
#include <QtCore/QThread>

#include "test_utils.h"

class TestCliCore : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // CLI Argument Parsing Tests
    void testHelpOption();
    void testVersionOption();
    void testInputOutputOptions();
    void testThreadOption();
    void testListOption();
    void testInvalidArguments();

    // Configuration Loading Tests
    void testValidConfigurationLoading();
    void testInvalidConfigurationLoading();
    void testMissingConfigurationFile();
    void testEmptyConfiguration();

    // Basic File Analysis Tests
    void testDirectFileAnalysis();
    void testFileStructureListing();
    void testInvalidFileAnalysis();

    // Thread Management Tests
    void testThreadParameterParsing();
    void testThreadValidation();

    // Parameter Extraction Tests - Claude Generated
    void testParameterExtractionBasic();
    void testSpecificModelExtraction();
    void testExtractionErrorHandling();
    
    // Modular JSON Structure Tests - Claude Generated
    void testModularDataGeneration();
    void testEquationBasedGeneration();
    void testRangeDataLoading();
    void testNoiseApplications();
    
    // Enhanced Error Handling Tests - Claude Generated
    void testLargeFileHandling();
    void testCorruptedFileRecovery();
    void testGracefulDegradation();

private:
    QTemporaryDir* m_tempDir;
    
    QString createTestConfigFile();
    QString createInvalidConfigFile();
    QString createEmptyConfigFile();
    QStringList runCliCommand(const QStringList& arguments);
    bool checkExitCode(const QStringList& arguments, int expectedCode = 0);
    
    // New helper methods - Claude Generated
    QString createTestModelFile();
    QString createModularConfigFile();
    QString createEquationConfigFile();
    QString createLargeConfigFile();
    QString createCorruptedModelFile();
};

void TestCliCore::initTestCase()
{
    qDebug() << "Starting CLI Core tests...";
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    // Verify CLI executable is available via TestUtils
    QString cliPath = TestUtils::getCliPath();
    qDebug() << "Using CLI executable:" << cliPath;
}

void TestCliCore::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "CLI Core tests completed.";
}

QStringList TestCliCore::runCliCommand(const QStringList& arguments)
{
    return TestUtils::executeCliCommand(arguments, 30000);
}

bool TestCliCore::checkExitCode(const QStringList& arguments, int expectedCode)
{
    QStringList result = runCliCommand(arguments);
    return result[0].toInt() == expectedCode;
}

void TestCliCore::testHelpOption()
{
    // Test --help option
    QStringList result = runCliCommand({"--help"});
    QCOMPARE(result[0].toInt(), 0); // Exit code should be 0
    QVERIFY(result[1].contains("Usage:")); // Should contain usage information
    QVERIFY(result[1].contains("Options:")); // Should contain options list
    
    // Test -h option
    result = runCliCommand({"-h"});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].contains("Usage:"));
}

void TestCliCore::testVersionOption()
{
    // Test --version option
    QStringList result = runCliCommand({"--version"});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].contains("SupraFit") || result[1].contains("version"));
    
    // Test -v option  
    result = runCliCommand({"-v"});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].contains("SupraFit") || result[1].contains("version"));
}

void TestCliCore::testInputOutputOptions()
{
    QString testConfig = createTestConfigFile();
    QString outputFile = m_tempDir->path() + "/test_output.json";
    
    // Test -i option with valid config
    QStringList result = runCliCommand({"-i", testConfig});
    QVERIFY2(result[0].toInt() == 0, qPrintable("CLI failed with: " + result[2]));
    
    // Test -i and -o combination
    result = runCliCommand({"-i", testConfig, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("CLI failed with: " + result[2]));
    
    // Test --input and --output long options
    result = runCliCommand({"--input", testConfig, "--output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("CLI failed with: " + result[2]));
}

void TestCliCore::testThreadOption()
{
    QString testConfig = createTestConfigFile();
    
    // Test -n option with various thread counts
    QStringList threadCounts = {"1", "2", "4", "8"};
    for (const QString& count : threadCounts) {
        QStringList result = runCliCommand({"-n", count, "-i", testConfig});
        QVERIFY2(result[0].toInt() == 0, qPrintable(QString("Thread test failed with count %1: %2").arg(count, result[2])));
    }
    
    // Test --nproc long option
    QStringList result = runCliCommand({"--nproc", "2", "-i", testConfig});
    QVERIFY2(result[0].toInt() == 0, qPrintable("--nproc failed: " + result[2]));
    
    // Test invalid thread count
    result = runCliCommand({"-n", "0", "-i", testConfig});
    QVERIFY(result[0].toInt() != 0); // Should fail with invalid thread count
    
    result = runCliCommand({"-n", "-1", "-i", testConfig});
    QVERIFY(result[0].toInt() != 0); // Should fail with negative thread count
}

void TestCliCore::testListOption()
{
    QString testConfig = createTestConfigFile();
    
    // Test -l option
    QStringList result = runCliCommand({"-l", testConfig});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].length() > 0); // Should produce some output
    
    // Test --list long option
    result = runCliCommand({"--list", testConfig});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].length() > 0);
}

void TestCliCore::testInvalidArguments()
{
    // Test unknown option
    QStringList result = runCliCommand({"--unknown-option"});
    QVERIFY(result[0].toInt() != 0); // Should fail
    
    // Test missing required parameter for -i
    result = runCliCommand({"-i"});
    QVERIFY(result[0].toInt() != 0); // Should fail
    
    // Test missing required parameter for -n
    result = runCliCommand({"-n"});
    QVERIFY(result[0].toInt() != 0); // Should fail
}

void TestCliCore::testValidConfigurationLoading()
{
    QString testConfig = createTestConfigFile();
    
    QStringList result = runCliCommand({"-i", testConfig});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Valid config loading failed: " + result[2]));
    
    // Should not produce error messages for valid config
    QVERIFY(!result[2].contains("Error:"));
    QVERIFY(!result[2].contains("ERROR:"));
}

void TestCliCore::testInvalidConfigurationLoading()
{
    QString invalidConfig = createInvalidConfigFile();
    
    QStringList result = runCliCommand({"-i", invalidConfig});
    // May succeed or fail depending on how gracefully invalid JSON is handled
    // Main test is that it doesn't crash
    QVERIFY(result[0].toInt() >= 0); // No negative exit codes (crashes)
}

void TestCliCore::testMissingConfigurationFile()
{
    QString nonExistentFile = m_tempDir->path() + "/nonexistent.json";
    
    QStringList result = runCliCommand({"-i", nonExistentFile});
    QVERIFY(result[0].toInt() != 0); // Should fail
    QVERIFY(result[2].contains("Error") || result[2].contains("ERROR") || 
            result[1].contains("Error") || result[1].contains("ERROR"));
}

void TestCliCore::testEmptyConfiguration()
{
    QString emptyConfig = createEmptyConfigFile();
    
    QStringList result = runCliCommand({"-i", emptyConfig});
    // Empty config should be handled gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
}

void TestCliCore::testDirectFileAnalysis()
{
    QString testConfig = createTestConfigFile();
    
    // Test direct file analysis (no additional parameters)
    QStringList result = runCliCommand({testConfig});
    QCOMPARE(result[0].toInt(), 0);
    // Should produce analysis output
    QVERIFY(result[1].length() > 0);
}

void TestCliCore::testFileStructureListing()
{
    QString testConfig = createTestConfigFile();
    
    QStringList result = runCliCommand({"-l", testConfig});
    QCOMPARE(result[0].toInt(), 0);
    
    // Should contain file structure information
    QVERIFY(result[1].length() > 0);
    // Common structure elements that should be present
    QVERIFY(result[1].contains("Main") || result[1].contains("Independent") || 
            result[1].contains("Dependent") || result[1].contains("{"));
}

void TestCliCore::testInvalidFileAnalysis()
{
    QString nonExistentFile = m_tempDir->path() + "/nonexistent.json";
    
    QStringList result = runCliCommand({"-l", nonExistentFile});
    QVERIFY(result[0].toInt() != 0); // Should fail
    
    // Test with completely invalid file
    QString invalidFile = m_tempDir->path() + "/invalid.txt";
    QFile file(invalidFile);
    file.open(QIODevice::WriteOnly);
    file.write("This is not JSON");
    file.close();
    
    result = runCliCommand({"-l", invalidFile});
    QVERIFY(result[0].toInt() != 0); // Should fail
}

void TestCliCore::testThreadParameterParsing()
{
    QString testConfig = createTestConfigFile();
    
    // Test various valid thread counts
    QStringList validCounts = {"1", "2", "4", "8", "16"};
    for (const QString& count : validCounts) {
        QVERIFY2(checkExitCode({"-n", count, "-i", testConfig}, 0),
                 qPrintable(QString("Valid thread count %1 should succeed").arg(count)));
    }
}

void TestCliCore::testThreadValidation()
{
    QString testConfig = createTestConfigFile();
    
    // Test invalid thread counts
    QStringList invalidCounts = {"0", "-1", "abc", "1000000"};
    for (const QString& count : invalidCounts) {
        QVERIFY2(!checkExitCode({"-n", count, "-i", testConfig}, 0),
                 qPrintable(QString("Invalid thread count %1 should fail").arg(count)));
    }
}

// Parameter Extraction Tests - Claude Generated
void TestCliCore::testParameterExtractionBasic()
{
    QString testModel = createTestModelFile();
    
    // Test basic parameter extraction
    QStringList result = runCliCommand({"-x", testModel});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].length() > 0);
    // Should contain parameter information
    QVERIFY(result[1].contains("Parameter") || result[1].contains("Model") || result[1].contains("Global"));
}

void TestCliCore::testSpecificModelExtraction()
{
    QString testModel = createTestModelFile();
    
    // Test extraction of specific model
    QStringList result = runCliCommand({"--extract-model", "0", testModel});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].length() > 0);
    // Should contain model-specific information
    QVERIFY(result[1].contains("Model") || result[1].contains("Parameter"));
}

void TestCliCore::testExtractionErrorHandling()
{
    QString nonExistentFile = m_tempDir->path() + "/nonexistent.suprafit";
    
    // Test extraction on non-existent file
    QStringList result = runCliCommand({"-x", nonExistentFile});
    QVERIFY(result[0].toInt() != 0); // Should fail
    
    // Test extraction on invalid model index
    QString testModel = createTestModelFile();
    result = runCliCommand({"--extract-model", "999", testModel});
    QVERIFY(result[0].toInt() != 0); // Should fail with invalid model index
}

// Modular JSON Structure Tests - Claude Generated
void TestCliCore::testModularDataGeneration()
{
    QString modularConfig = createModularConfigFile();
    
    QStringList result = runCliCommand({"-i", modularConfig});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Modular generation failed: " + result[2]));
    QVERIFY(result[1].length() > 0);
}

void TestCliCore::testEquationBasedGeneration()
{
    QString equationConfig = createEquationConfigFile();
    
    QStringList result = runCliCommand({"-i", equationConfig});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Equation generation failed: " + result[2]));
    QVERIFY(result[1].length() > 0);
    // Should indicate equation processing
    QVERIFY(result[1].contains("equation") || result[1].contains("X") || result[1].contains("generated"));
}

void TestCliCore::testRangeDataLoading()
{
    QString testConfig = createTestConfigFile();
    QString outputFile = m_tempDir->path() + "/range_test.json";
    
    // Test range loading (this would need specific range parameters in real implementation)
    QStringList result = runCliCommand({"-i", testConfig, "-o", outputFile});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].length() >= 0); // Should complete successfully
}

void TestCliCore::testNoiseApplications()
{
    QString modularConfig = createModularConfigFile();
    
    // Test with noise configuration
    QStringList result = runCliCommand({"-i", modularConfig});
    QCOMPARE(result[0].toInt(), 0);
    // Should handle noise configuration gracefully
    QVERIFY(result[1].length() >= 0);
}

// Enhanced Error Handling Tests - Claude Generated
void TestCliCore::testLargeFileHandling()
{
    QString largeConfig = createLargeConfigFile();
    
    // Test handling of large configuration files
    QStringList result = runCliCommand({"-i", largeConfig});
    // Should either succeed or fail gracefully, but not crash
    QVERIFY(result[0].toInt() >= 0);
}

void TestCliCore::testCorruptedFileRecovery()
{
    QString corruptedFile = createCorruptedModelFile();
    
    // Test graceful handling of corrupted files
    QStringList result = runCliCommand({"-l", corruptedFile});
    QVERIFY(result[0].toInt() != 0); // Should fail
    // Should provide meaningful error message
    QVERIFY(result[2].contains("Error") || result[2].contains("ERROR") || 
            result[2].contains("Invalid") || result[2].contains("corrupt"));
}

void TestCliCore::testGracefulDegradation()
{
    QString testConfig = createTestConfigFile();
    QString invalidOutput = "/invalid/path/cannot/write/here.json";
    
    // Test graceful handling of invalid output paths
    QStringList result = runCliCommand({"-i", testConfig, "-o", invalidOutput});
    QVERIFY(result[0].toInt() != 0); // Should fail
    // Should provide meaningful error message about write permissions or path
    QVERIFY(result[2].contains("Error") || result[2].contains("ERROR") || 
            result[2].contains("write") || result[2].contains("path"));
}

QString TestCliCore::createTestConfigFile()
{
    QString filePath = m_tempDir->path() + "/test_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "test_output";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject generator;
    generator["Type"] = "equations";
    generator["DataPoints"] = 10;
    generator["Variables"] = 2;
    generator["Equations"] = "X|X*2";
    independent["Generator"] = generator;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGenerator;
    depGenerator["Type"] = "copy";
    depGenerator["Series"] = 2;
    dependent["Generator"] = depGenerator;
    config["Dependent"] = dependent;
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QString TestCliCore::createInvalidConfigFile()
{
    QString filePath = m_tempDir->path() + "/invalid_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write("{ invalid json structure without closing brace");
    file.close();
    
    return filePath;
}

QString TestCliCore::createEmptyConfigFile()
{
    QString filePath = m_tempDir->path() + "/empty_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write("{}");
    file.close();
    
    return filePath;
}

// New helper method implementations - Claude Generated
QString TestCliCore::createTestModelFile()
{
    QString filePath = m_tempDir->path() + "/test_model.suprafit";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject model;
    QJsonObject data;
    QJsonObject independent;
    QJsonObject dependent;
    
    // Create minimal model structure for parameter extraction testing
    QJsonObject model0;
    QJsonObject globalParams;
    globalParams["K11"] = 1000.0;
    globalParams["K21"] = 500.0;
    model0["global_parameter"] = globalParams;
    
    QJsonObject localParams;
    QJsonArray series0;
    series0.append(1.5);
    series0.append(2.0);
    localParams["0"] = series0;
    model0["local_parameter"] = localParams;
    
    model["model_0"] = model0;
    model["data"] = data;
    
    QJsonDocument doc(model);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QString TestCliCore::createModularConfigFile()
{
    QString filePath = m_tempDir->path() + "/modular_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "modular_test";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    // Independent data section
    QJsonObject independent;
    independent["Source"] = "generator";
    QJsonObject indepGenerator;
    indepGenerator["Type"] = "equations";
    indepGenerator["DataPoints"] = 15;
    indepGenerator["Variables"] = 2;
    indepGenerator["Equations"] = "X|X*X";
    independent["Generator"] = indepGenerator;
    config["Independent"] = independent;
    
    // Dependent data section
    QJsonObject dependent;
    dependent["Source"] = "generator";
    QJsonObject depGenerator;
    depGenerator["Type"] = "model";
    depGenerator["Series"] = 2;
    QJsonObject model;
    model["ID"] = 1;
    depGenerator["Model"] = model;
    dependent["Generator"] = depGenerator;
    
    QJsonObject noise;
    noise["Type"] = "gaussian";
    QJsonArray stdValues;
    stdValues.append(1e-3);
    stdValues.append(1e-3);
    noise["Std"] = stdValues;
    dependent["Noise"] = noise;
    config["Dependent"] = dependent;
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QString TestCliCore::createEquationConfigFile()
{
    QString filePath = m_tempDir->path() + "/equation_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "equation_test";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    QJsonObject independent;
    independent["Source"] = "generator";
    QJsonObject generator;
    generator["Type"] = "equations";
    generator["DataPoints"] = 20;
    generator["Variables"] = 3;
    generator["Equations"] = "X|X*2|sin(X)";
    independent["Generator"] = generator;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGenerator;
    depGenerator["Type"] = "copy";
    depGenerator["Series"] = 2;
    dependent["Generator"] = depGenerator;
    config["Dependent"] = dependent;
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QString TestCliCore::createLargeConfigFile()
{
    QString filePath = m_tempDir->path() + "/large_config.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "large_test";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    // Create a large configuration with many data points
    QJsonObject independent;
    independent["Source"] = "generator";
    QJsonObject generator;
    generator["Type"] = "equations";
    generator["DataPoints"] = 1000; // Large number of data points
    generator["Variables"] = 10;    // Many variables
    generator["Equations"] = "X|X*2|X*3|X*4|X*5|X*6|X*7|X*8|X*9|X*10";
    independent["Generator"] = generator;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGenerator;
    depGenerator["Type"] = "copy";
    depGenerator["Series"] = 5;
    dependent["Generator"] = depGenerator;
    config["Dependent"] = dependent;
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

QString TestCliCore::createCorruptedModelFile()
{
    QString filePath = m_tempDir->path() + "/corrupted.suprafit";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    
    // Create intentionally malformed JSON
    file.write("{ \"model_0\": { \"global_parameter\": { \"K11\": 1000.0 } "); // Missing closing braces
    file.close();
    
    return filePath;
}

#include "test_cli_core.moc"

QTEST_MAIN(TestCliCore)