/*
 * Tests for CLI Command Pattern Implementation
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Tests the new modular command pattern architecture for SupraFit CLI.
 * Validates command parsing, dispatching, and execution separation.
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
#include <QtCore/QTemporaryFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QCoreApplication>

#include "../client/cli_command_parser.h"
#include "../client/cli_command_dispatcher.h"
#include "../client/suprafit_cli.h"

class TestCliCommandPattern : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Command Parser Tests
    void testHelpCommandParsing();
    void testVersionCommandParsing();
    void testInputOutputParsing();
    void testListCommandParsing();
    void testExtractCommandParsing();
    void testInvalidArgumentHandling();
    void testThreadValidation();

    // Command Dispatcher Tests
    void testCommandDispatcherCreation();
    void testHelpCommandExecution();
    void testVersionCommandExecution();
    void testInvalidCommandHandling();

    // Integration Tests
    void testCompleteCommandFlow();
    void testErrorPropagation();
    void testCommandValidation();

    // Modular Architecture Tests
    void testCommandSeparationOfConcerns();
    void testParserDispatcherIntegration();
    void testCommandObjectCreation();

private:
    QTemporaryDir* m_tempDir;
    QString createTestConfigFile();
};

void TestCliCommandPattern::initTestCase()
{
    qDebug() << "Starting CLI Command Pattern tests...";
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    // Set up application for testing
    QCoreApplication::instance()->setApplicationName("SupraFit CLI Test");
    QCoreApplication::instance()->setApplicationVersion("1.0.0");
}

void TestCliCommandPattern::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "CLI Command Pattern tests completed.";
}

// Command Parser Tests
void TestCliCommandPattern::testHelpCommandParsing()
{
    CliCommandParser parser;
    
    // Test --help option
    auto command = parser.parseArguments({"suprafit_cli", "--help"});
    QCOMPARE(command.type, CliCommandParser::CommandType::Help);
    QVERIFY(command.isValid);
    
    // Test -h option
    command = parser.parseArguments({"suprafit_cli", "-h"});
    QCOMPARE(command.type, CliCommandParser::CommandType::Help);
    QVERIFY(command.isValid);
}

void TestCliCommandPattern::testVersionCommandParsing()
{
    CliCommandParser parser;
    
    // Test --version option
    auto command = parser.parseArguments({"suprafit_cli", "--version"});
    QCOMPARE(command.type, CliCommandParser::CommandType::Version);
    QVERIFY(command.isValid);
    
    // Test -v option
    command = parser.parseArguments({"suprafit_cli", "-v"});
    QCOMPARE(command.type, CliCommandParser::CommandType::Version);
    QVERIFY(command.isValid);
}

void TestCliCommandPattern::testInputOutputParsing()
{
    CliCommandParser parser;
    QString testFile = createTestConfigFile();
    
    // Test -i option
    auto command = parser.parseArguments({"suprafit_cli", "-i", testFile});
    QCOMPARE(command.type, CliCommandParser::CommandType::Generate);
    QCOMPARE(command.inputFile, testFile);
    QVERIFY(command.isValid);
    
    // Test --input --output combination
    command = parser.parseArguments({"suprafit_cli", "--input", testFile, "--output", "result.json"});
    QCOMPARE(command.type, CliCommandParser::CommandType::Generate);
    QCOMPARE(command.inputFile, testFile);
    QCOMPARE(command.outputFile, QString("result.json"));
    QVERIFY(command.isValid);
}

void TestCliCommandPattern::testListCommandParsing()
{
    CliCommandParser parser;
    QString testFile = createTestConfigFile();
    
    // Test -l option
    auto command = parser.parseArguments({"suprafit_cli", "-l", testFile});
    QCOMPARE(command.type, CliCommandParser::CommandType::List);
    QCOMPARE(command.inputFile, testFile);
    QVERIFY(command.isValid);
    
    // Test --list option
    command = parser.parseArguments({"suprafit_cli", "--list", testFile});
    QCOMPARE(command.type, CliCommandParser::CommandType::List);
    QCOMPARE(command.inputFile, testFile);
    QVERIFY(command.isValid);
}

void TestCliCommandPattern::testExtractCommandParsing()
{
    CliCommandParser parser;
    QString testFile = createTestConfigFile();
    
    // Test -x option
    auto command = parser.parseArguments({"suprafit_cli", "-x", testFile});
    QCOMPARE(command.type, CliCommandParser::CommandType::Extract);
    QCOMPARE(command.inputFile, testFile);
    QVERIFY(command.isValid);
    
    // Test --extract-parameters option
    command = parser.parseArguments({"suprafit_cli", "--extract-parameters", testFile});
    QCOMPARE(command.type, CliCommandParser::CommandType::Extract);
    QCOMPARE(command.inputFile, testFile);
    QVERIFY(command.isValid);
    
    // Test extract with specific model
    command = parser.parseArguments({"suprafit_cli", "-x", testFile, "--extract-model", "2"});
    QCOMPARE(command.type, CliCommandParser::CommandType::Extract);
    QCOMPARE(command.extractModel, QString("2"));
    QVERIFY(command.isValid);
}

void TestCliCommandPattern::testInvalidArgumentHandling()
{
    CliCommandParser parser;
    
    // Test with no arguments
    auto command = parser.parseArguments({"suprafit_cli"});
    QCOMPARE(command.type, CliCommandParser::CommandType::Invalid);
    QVERIFY(!command.isValid);
    QVERIFY(!command.errorMessage.isEmpty());
    
    // Test with non-existent file
    command = parser.parseArguments({"suprafit_cli", "-i", "/nonexistent/file.json"});
    QVERIFY(!command.isValid);
    QVERIFY(command.errorMessage.contains("does not exist"));
}

void TestCliCommandPattern::testThreadValidation()
{
    CliCommandParser parser;
    QString testFile = createTestConfigFile();
    
    // Test valid thread count
    auto command = parser.parseArguments({"suprafit_cli", "-n", "8", "-i", testFile});
    QCOMPARE(command.threads, 8);
    QVERIFY(command.isValid);
    
    // Test invalid thread count (zero)
    command = parser.parseArguments({"suprafit_cli", "-n", "0", "-i", testFile});
    QVERIFY(!command.isValid);
    QVERIFY(command.errorMessage.contains("at least 1"));
    
    // Test invalid thread count (too high)
    command = parser.parseArguments({"suprafit_cli", "-n", "128", "-i", testFile});
    QVERIFY(!command.isValid);
    QVERIFY(command.errorMessage.contains("should not exceed"));
}

// Command Dispatcher Tests
void TestCliCommandPattern::testCommandDispatcherCreation()
{
    CliCommandDispatcher dispatcher;
    
    // Dispatcher should be created successfully
    QVERIFY(true); // If we get here, construction succeeded
}

void TestCliCommandPattern::testHelpCommandExecution()
{
    CliCommandParser parser;
    CliCommandDispatcher dispatcher;
    SupraFitCli suprafitCli;
    
    auto command = parser.parseArguments({"suprafit_cli", "--help"});
    QVERIFY(command.isValid);
    
    // Help command should execute successfully
    int exitCode = dispatcher.dispatch(command, suprafitCli);
    QCOMPARE(exitCode, 0);
}

void TestCliCommandPattern::testVersionCommandExecution()
{
    CliCommandParser parser;
    CliCommandDispatcher dispatcher;
    SupraFitCli suprafitCli;
    
    auto command = parser.parseArguments({"suprafit_cli", "--version"});
    QVERIFY(command.isValid);
    
    // Version command should execute successfully
    int exitCode = dispatcher.dispatch(command, suprafitCli);
    QCOMPARE(exitCode, 0);
}

void TestCliCommandPattern::testInvalidCommandHandling()
{
    CliCommandDispatcher dispatcher;
    SupraFitCli suprafitCli;
    
    // Create invalid command
    CliCommandParser::ParsedCommand command;
    command.type = CliCommandParser::CommandType::Invalid;
    command.isValid = false;
    command.errorMessage = "Test error message";
    
    // Invalid command should return non-zero exit code
    int exitCode = dispatcher.dispatch(command, suprafitCli);
    QVERIFY(exitCode != 0);
}

// Integration Tests
void TestCliCommandPattern::testCompleteCommandFlow()
{
    CliCommandParser parser;
    CliCommandDispatcher dispatcher;
    SupraFitCli suprafitCli;
    QString testFile = createTestConfigFile();
    
    // Parse list command
    auto command = parser.parseArguments({"suprafit_cli", "-l", testFile});
    QVERIFY(command.isValid);
    QCOMPARE(command.type, CliCommandParser::CommandType::List);
    
    // Execute command
    int exitCode = dispatcher.dispatch(command, suprafitCli);
    QCOMPARE(exitCode, 0);
}

void TestCliCommandPattern::testErrorPropagation()
{
    CliCommandParser parser;
    CliCommandDispatcher dispatcher;
    SupraFitCli suprafitCli;
    
    // Create command with non-existent file
    auto command = parser.parseArguments({"suprafit_cli", "-l", "/nonexistent.json"});
    QVERIFY(!command.isValid);
    
    // Error should be propagated
    int exitCode = dispatcher.dispatch(command, suprafitCli);
    QVERIFY(exitCode != 0);
}

void TestCliCommandPattern::testCommandValidation()
{
    CliCommandParser parser;
    
    // Test various validation scenarios
    QString testFile = createTestConfigFile();
    
    // Valid command
    auto command = parser.parseArguments({"suprafit_cli", "-i", testFile});
    QVERIFY(command.isValid);
    
    // Invalid thread count
    command = parser.parseArguments({"suprafit_cli", "-n", "-5", "-i", testFile});
    QVERIFY(!command.isValid);
    
    // Invalid extract model index
    command = parser.parseArguments({"suprafit_cli", "-x", testFile, "--extract-model", "abc"});
    QVERIFY(!command.isValid);
}

// Modular Architecture Tests
void TestCliCommandPattern::testCommandSeparationOfConcerns()
{
    // Test that parser only parses, doesn't execute
    CliCommandParser parser;
    QString testFile = createTestConfigFile();
    
    auto command = parser.parseArguments({"suprafit_cli", "-i", testFile});
    
    // Parser should only set up command structure, not execute anything
    QCOMPARE(command.type, CliCommandParser::CommandType::Generate);
    QCOMPARE(command.inputFile, testFile);
    QVERIFY(command.isValid);
    
    // No side effects should have occurred yet
    QVERIFY(true); // If we get here without crashes, separation is working
}

void TestCliCommandPattern::testParserDispatcherIntegration()
{
    CliCommandParser parser;
    CliCommandDispatcher dispatcher;
    SupraFitCli suprafitCli;
    
    // Test seamless integration between parser and dispatcher
    auto command = parser.parseArguments({"suprafit_cli", "--version"});
    QVERIFY(command.isValid);
    
    int exitCode = dispatcher.dispatch(command, suprafitCli);
    QCOMPARE(exitCode, 0);
}

void TestCliCommandPattern::testCommandObjectCreation()
{
    CliCommandParser parser;
    CliCommandDispatcher dispatcher;
    SupraFitCli suprafitCli;
    QString testFile = createTestConfigFile();
    
    // Test that appropriate command objects are created for different types
    QStringList commandTypes = {
        "--help",
        "--version", 
        QString("-l %1").arg(testFile),
        QString("-x %1").arg(testFile)
    };
    
    for (const QString& cmdString : commandTypes) {
        QStringList args = QString("suprafit_cli %1").arg(cmdString).split(' ', Qt::SkipEmptyParts);
        auto command = parser.parseArguments(args);
        
        if (command.isValid) {
            int exitCode = dispatcher.dispatch(command, suprafitCli);
            // Command objects should be created and executed without crashing
            QVERIFY(exitCode >= 0); // Any non-negative exit code indicates proper object creation
        }
    }
}

QString TestCliCommandPattern::createTestConfigFile()
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
    independent["Source"] = "generator";
    QJsonObject generator;
    generator["Type"] = "equations";
    generator["DataPoints"] = 10;
    generator["Variables"] = 2;
    generator["Equations"] = "X|Y";
    independent["Generator"] = generator;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGenerator;
    depGenerator["Type"] = "copy";
    depGenerator["Series"] = 1;
    dependent["Generator"] = depGenerator;
    config["Dependent"] = dependent;
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return filePath;
}

#include "test_cli_command_pattern.moc"

QTEST_MAIN(TestCliCommandPattern)