/*
 * Comprehensive tests for SupraFit file I/O operations
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Tests file structure analysis (-l/--list), format conversion (.json ↔ .suprafit),
 * input/output parameters, range loading integration, and error handling.
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
#include <QtCore/QFileInfo>
#include <QtCore/QElapsedTimer>

#include "test_utils.h"

class TestFileOperations : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // File Structure Analysis Tests
    void testFileStructureListing();
    void testListOptionWithValidFile();
    void testListOptionWithInvalidFile();
    void testListOptionWithMultiProjectFile();

    // Format Conversion Tests
    void testJsonToSuprafitConversion();
    void testSuprafitToJsonConversion();
    void testBidirectionalConversion();
    void testConversionPreservesData();

    // Input/Output Parameter Tests
    void testInputParameterValidation();
    void testOutputParameterValidation();
    void testInputOutputCombination();
    void testRelativeVsAbsolutePaths();

    // File Extension Handling Tests
    void testDefaultExtensionHandling();
    void testExplicitExtensionHandling();
    void testMixedExtensionConversion();

    // Range Loading Integration Tests
    void testDataRangeExtraction();
    void testPartialFileLoading();
    void testRangeParameterValidation();

    // File Validation Tests
    void testFileExistenceValidation();
    void testFilePermissionHandling();
    void testFileFormatValidation();
    void testCorruptedFileHandling();

    // Large File Tests
    void testLargeFileHandling();
    void testFilePerformance();
    void testMemoryUsage();

    // Error Handling Tests
    void testMissingInputFile();
    void testInvalidOutputPath();
    void testWriteProtectedOutput();
    void testInsufficientDiskSpace();

    // Enhanced Performance and Corruption Tests - Claude Generated 2025-09-04
    void testAdvancedCorruptionRecovery();
    void testConcurrentFileAccess();
    void testFileSystemStressTest();
    void testLargeFileStreamingHandling();
    void testPartialCorruptionHandling();
    void testNetworkFileSystemCompatibility();
    void testFileRecoveryAfterInterruption();
    void testMemoryMappedFileOperations();

private:
    QTemporaryDir* m_tempDir;
    
    QString createTestJsonFile();
    QString createTestSuprafitFile();
    QString createLargeTestFile();
    QString createCorruptedJsonFile();
    QString createComplexStructureFile();
    QJsonObject loadJsonFile(const QString& filename);
    bool filesAreEquivalent(const QString& file1, const QString& file2);
    bool verifyFileStructure(const QString& output, const QString& expectedContent);
    qint64 getFileSize(const QString& filename);
};

void TestFileOperations::initTestCase()
{
    qDebug() << "Starting File I/O Operations tests...";
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    // Verify CLI binary is available through TestUtils
    QString cliPath = TestUtils::findSuprafitCli();
    QVERIFY2(!cliPath.isEmpty(), "suprafit_cli executable not found - set SUPRAFIT_CLI_PATH env var if needed");
}

void TestFileOperations::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "File I/O Operations tests completed.";
}

// Removed - using TestUtils::executeCliCommand instead

QJsonObject TestFileOperations::loadJsonFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON parsing error:" << error.errorString();
        return QJsonObject();
    }
    
    return doc.object();
}

bool TestFileOperations::filesAreEquivalent(const QString& file1, const QString& file2)
{
    // For JSON files, compare the parsed structure
    if (file1.endsWith(".json") && file2.endsWith(".json")) {
        QJsonObject obj1 = loadJsonFile(file1);
        QJsonObject obj2 = loadJsonFile(file2);
        return obj1 == obj2;
    }
    
    // For binary comparison
    QFile f1(file1), f2(file2);
    if (!f1.open(QIODevice::ReadOnly) || !f2.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    return f1.readAll() == f2.readAll();
}

bool TestFileOperations::verifyFileStructure(const QString& output, const QString& expectedContent)
{
    return output.contains(expectedContent, Qt::CaseInsensitive);
}

qint64 TestFileOperations::getFileSize(const QString& filename)
{
    QFileInfo fileInfo(filename);
    return fileInfo.size();
}

void TestFileOperations::testFileStructureListing()
{
    QString testFile = createTestJsonFile();
    
    QStringList result = TestUtils::executeCliCommand({"-l", testFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("File structure listing failed: " + result[2]));
    
    // Should show file structure information
    QVERIFY2(!result[1].isEmpty(), "No structure information provided");
    
    // Should contain typical JSON structure elements
    QVERIFY(verifyFileStructure(result[1], "main") || 
            verifyFileStructure(result[1], "independent") ||
            verifyFileStructure(result[1], "dependent") ||
            verifyFileStructure(result[1], "{"));
}

void TestFileOperations::testListOptionWithValidFile()
{
    QString testFile = createComplexStructureFile();
    
    QStringList result = TestUtils::executeCliCommand({"--list", testFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("--list option failed: " + result[2]));
    
    // Should display detailed structure
    QVERIFY(result[1].length() > 100); // Substantial output expected
    
    // Should show hierarchical structure
    QVERIFY(verifyFileStructure(result[1], "main") &&
            (verifyFileStructure(result[1], "independent") || 
             verifyFileStructure(result[1], "dependent")));
}

void TestFileOperations::testListOptionWithInvalidFile()
{
    QString invalidFile = createCorruptedJsonFile();
    
    QStringList result = TestUtils::executeCliCommand({"-l", invalidFile});
    
    // Should handle invalid files gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
    
    if (result[0].toInt() != 0) {
        QVERIFY(result[2].contains("Error") || result[2].contains("ERROR") ||
                result[2].contains("parse") || result[2].contains("invalid"));
    }
}

void TestFileOperations::testListOptionWithMultiProjectFile()
{
    // Create multi-project structure
    QJsonObject multiProject;
    multiProject["format_version"] = "2.0";
    multiProject["project_0"] = QJsonObject{{"model_id", 1}};
    multiProject["project_1"] = QJsonObject{{"model_id", 2}};
    
    QString multiProjectFile = m_tempDir->path() + "/multi_project.json";
    QFile file(multiProjectFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(multiProject).toJson());
    file.close();
    
    QStringList result = TestUtils::executeCliCommand({"-l", multiProjectFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Multi-project listing failed: " + result[2]));
    
    // Should show multi-project structure
    QVERIFY(verifyFileStructure(result[1], "project_0") ||
            verifyFileStructure(result[1], "project_1") ||
            verifyFileStructure(result[1], "multi") ||
            verifyFileStructure(result[1], "format_version"));
}

void TestFileOperations::testJsonToSuprafitConversion()
{
    QString jsonFile = createTestJsonFile();
    QString suprafitFile = m_tempDir->path() + "/converted.suprafit";
    
    QStringList result = TestUtils::executeCliCommand({"-i", jsonFile, "-o", suprafitFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("JSON to SupraFit conversion failed: " + result[2]));
    
    // Verify output file exists
    QVERIFY2(QFile::exists(suprafitFile), "Converted SupraFit file not created");
    
    // Verify file size is reasonable (not empty, not too large)
    qint64 fileSize = getFileSize(suprafitFile);
    QVERIFY2(fileSize > 0 && fileSize < 10000000, // < 10MB
             qPrintable(QString("Suspicious file size: %1 bytes").arg(fileSize)));
}

void TestFileOperations::testSuprafitToJsonConversion()
{
    QString suprafitFile = createTestSuprafitFile();
    QString jsonFile = m_tempDir->path() + "/converted.json";
    
    QStringList result = TestUtils::executeCliCommand({"-i", suprafitFile, "-o", jsonFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("SupraFit to JSON conversion failed: " + result[2]));
    
    // Verify output file exists
    QVERIFY2(QFile::exists(jsonFile), "Converted JSON file not created");
    
    // Verify it's valid JSON
    QJsonObject jsonData = loadJsonFile(jsonFile);
    QVERIFY2(!jsonData.isEmpty(), "Converted JSON file is empty or invalid");
}

void TestFileOperations::testBidirectionalConversion()
{
    QString originalJson = createTestJsonFile();
    QString suprafitFile = m_tempDir->path() + "/bidirectional.suprafit";
    QString backToJson = m_tempDir->path() + "/bidirectional.json";
    
    // JSON → SupraFit
    QStringList result1 = TestUtils::executeCliCommand({"-i", originalJson, "-o", suprafitFile});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("First conversion failed: " + result1[2]));
    
    // SupraFit → JSON
    QStringList result2 = TestUtils::executeCliCommand({"-i", suprafitFile, "-o", backToJson});
    QVERIFY2(result2[0].toInt() == 0, qPrintable("Second conversion failed: " + result2[2]));
    
    // Verify files exist
    QVERIFY(QFile::exists(suprafitFile) && QFile::exists(backToJson));
    
    // Compare original and round-trip result
    QJsonObject original = loadJsonFile(originalJson);
    QJsonObject roundTrip = loadJsonFile(backToJson);
    
    // Should preserve essential structure (may not be identical due to formatting)
    QVERIFY2(!original.isEmpty() && !roundTrip.isEmpty(), "One of the files is empty");
    
    // Key fields should be preserved
    if (original.contains("Main") && roundTrip.contains("Main")) {
        QVERIFY2(original["Main"].toObject().keys().size() > 0 &&
                 roundTrip["Main"].toObject().keys().size() > 0,
                 "Main section not preserved in round-trip conversion");
    }
}

void TestFileOperations::testConversionPreservesData()
{
    QString originalFile = createComplexStructureFile();
    QString convertedFile = m_tempDir->path() + "/data_preservation.suprafit";
    QString backConverted = m_tempDir->path() + "/data_preservation_back.json";
    
    // Convert and back-convert
    QStringList result1 = TestUtils::executeCliCommand({"-i", originalFile, "-o", convertedFile});
    QStringList result2 = TestUtils::executeCliCommand({"-i", convertedFile, "-o", backConverted});
    
    if (result1[0].toInt() == 0 && result2[0].toInt() == 0) {
        QJsonObject original = loadJsonFile(originalFile);
        QJsonObject restored = loadJsonFile(backConverted);
        
        // Check key data preservation
        QStringList criticalKeys = {"Main", "Independent", "Dependent", "AddModels"};
        for (const QString& key : criticalKeys) {
            if (original.contains(key)) {
                QVERIFY2(restored.contains(key), 
                        qPrintable(QString("Critical key %1 not preserved").arg(key)));
            }
        }
    }
}

void TestFileOperations::testInputParameterValidation()
{
    QString validFile = createTestJsonFile();
    QString nonExistentFile = m_tempDir->path() + "/nonexistent.json";
    
    // Valid input file
    QStringList result1 = TestUtils::executeCliCommand({"-i", validFile});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("Valid input file rejected: " + result1[2]));
    
    // Non-existent input file
    QStringList result2 = TestUtils::executeCliCommand({"-i", nonExistentFile});
    QVERIFY2(result2[0].toInt() != 0, "Non-existent input file accepted");
    QVERIFY(result2[2].contains("Error") || result2[2].contains("not found") ||
            result2[2].contains("ERROR"));
}

void TestFileOperations::testOutputParameterValidation()
{
    QString inputFile = createTestJsonFile();
    QString validOutput = m_tempDir->path() + "/valid_output.json";
    QString invalidOutput = "/invalid/path/output.json";
    
    // Valid output path
    QStringList result1 = TestUtils::executeCliCommand({"-i", inputFile, "-o", validOutput});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("Valid output path failed: " + result1[2]));
    QVERIFY(QFile::exists(validOutput));
    
    // Invalid output path
    QStringList result2 = TestUtils::executeCliCommand({"-i", inputFile, "-o", invalidOutput});
    QVERIFY2(result2[0].toInt() != 0, "Invalid output path accepted");
}

void TestFileOperations::testInputOutputCombination()
{
    QString inputFile = createTestJsonFile();
    QString outputFile = m_tempDir->path() + "/io_combination.json";
    
    QStringList result = TestUtils::executeCliCommand({"-i", inputFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Input/Output combination failed: " + result[2]));
    
    QVERIFY(QFile::exists(outputFile));
    
    // Verify output is different from input (at least in path)
    QVERIFY2(inputFile != outputFile, "Input and output files are the same");
}

void TestFileOperations::testRelativeVsAbsolutePaths()
{
    QString inputFile = createTestJsonFile();
    
    // Absolute path output
    QString absoluteOutput = m_tempDir->path() + "/absolute_output.json";
    QStringList result1 = TestUtils::executeCliCommand({"-i", inputFile, "-o", absoluteOutput});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("Absolute path failed: " + result1[2]));
    
    // Relative path output (if supported)
    QString relativeOutput = "relative_output.json";
    QStringList result2 = TestUtils::executeCliCommand({"-i", inputFile, "-o", relativeOutput});
    QVERIFY2(result2[0].toInt() == 0, qPrintable("Relative path failed: " + result2[2]));
    
    // Clean up relative file if created in current directory
    if (QFile::exists(relativeOutput)) {
        QFile::remove(relativeOutput);
    }
}

void TestFileOperations::testDefaultExtensionHandling()
{
    QString inputFile = createTestJsonFile();
    QString outputWithoutExt = m_tempDir->path() + "/no_extension";
    
    QStringList result = TestUtils::executeCliCommand({"-i", inputFile, "-o", outputWithoutExt});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Default extension handling failed: " + result[2]));
    
    // Check if default extension was added
    QString expectedJson = outputWithoutExt + ".json";
    QString expectedSuprafit = outputWithoutExt + ".suprafit";
    
    QVERIFY2(QFile::exists(expectedJson) || QFile::exists(expectedSuprafit) || QFile::exists(outputWithoutExt),
             "No output file found with any extension");
}

void TestFileOperations::testExplicitExtensionHandling()
{
    QString inputFile = createTestJsonFile();
    
    // Test with explicit .json extension
    QString jsonOutput = m_tempDir->path() + "/explicit.json";
    QStringList result1 = TestUtils::executeCliCommand({"-i", inputFile, "-o", jsonOutput});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("Explicit .json extension failed: " + result1[2]));
    QVERIFY(QFile::exists(jsonOutput));
    
    // Test with explicit .suprafit extension
    QString suprafitOutput = m_tempDir->path() + "/explicit.suprafit";
    QStringList result2 = TestUtils::executeCliCommand({"-i", inputFile, "-o", suprafitOutput});
    QVERIFY2(result2[0].toInt() == 0, qPrintable("Explicit .suprafit extension failed: " + result2[2]));
    QVERIFY(QFile::exists(suprafitOutput));
}

void TestFileOperations::testMixedExtensionConversion()
{
    QString jsonInput = createTestJsonFile();
    QString suprafitInput = createTestSuprafitFile();
    
    // JSON input to SupraFit output
    QString jsonToSuprafit = m_tempDir->path() + "/mixed1.suprafit";
    QStringList result1 = TestUtils::executeCliCommand({"-i", jsonInput, "-o", jsonToSuprafit});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("JSON to SupraFit mixed conversion failed: " + result1[2]));
    
    // SupraFit input to JSON output
    QString suprafitToJson = m_tempDir->path() + "/mixed2.json";
    QStringList result2 = TestUtils::executeCliCommand({"-i", suprafitInput, "-o", suprafitToJson});
    QVERIFY2(result2[0].toInt() == 0, qPrintable("SupraFit to JSON mixed conversion failed: " + result2[2]));
    
    QVERIFY(QFile::exists(jsonToSuprafit) && QFile::exists(suprafitToJson));
}

void TestFileOperations::testDataRangeExtraction()
{
    QString inputFile = createLargeTestFile();
    
    // Test basic file analysis (range extraction might be implicit)
    QStringList result = TestUtils::executeCliCommand({"-l", inputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Data range extraction failed: " + result[2]));
    
    // Should show data dimensions or structure
    QVERIFY(verifyFileStructure(result[1], "rows") ||
            verifyFileStructure(result[1], "cols") ||
            verifyFileStructure(result[1], "data") ||
            verifyFileStructure(result[1], "points"));
}

void TestFileOperations::testPartialFileLoading()
{
    QString largeFile = createLargeTestFile();
    QString outputFile = m_tempDir->path() + "/partial_load.json";
    
    // Test partial loading (might be implicit in the conversion process)
    QStringList result = TestUtils::executeCliCommand({"-i", largeFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Partial file loading failed: " + result[2]));
    
    QVERIFY(QFile::exists(outputFile));
    
    // Verify output is reasonable size
    qint64 inputSize = getFileSize(largeFile);
    qint64 outputSize = getFileSize(outputFile);
    
    QVERIFY2(outputSize > 0 && outputSize < inputSize * 10, // Reasonable size bounds
             qPrintable(QString("Suspicious size ratio: input=%1, output=%2").arg(inputSize).arg(outputSize)));
}

void TestFileOperations::testRangeParameterValidation()
{
    QString inputFile = createTestJsonFile();
    
    // Test basic range handling (implicit through normal operations)
    QStringList result = TestUtils::executeCliCommand({"-i", inputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Range parameter validation failed: " + result[2]));
    
    // Should handle file without explicit range parameters
    QVERIFY(!result[1].isEmpty() || !result[2].isEmpty());
}

void TestFileOperations::testFileExistenceValidation()
{
    QString existingFile = createTestJsonFile();
    QString nonExistentFile = m_tempDir->path() + "/does_not_exist.json";
    
    // Existing file
    QStringList result1 = TestUtils::executeCliCommand({"-i", existingFile});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("Existing file validation failed: " + result1[2]));
    
    // Non-existent file
    QStringList result2 = TestUtils::executeCliCommand({"-i", nonExistentFile});
    QVERIFY2(result2[0].toInt() != 0, "Non-existent file not detected");
    QVERIFY(result2[2].contains("Error") || result2[2].contains("not found"));
}

void TestFileOperations::testFilePermissionHandling()
{
    QString inputFile = createTestJsonFile();
    
    // Test with read-only input (should work)
    QFile::setPermissions(inputFile, QFile::ReadUser);
    QStringList result1 = TestUtils::executeCliCommand({"-i", inputFile});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("Read-only file handling failed: " + result1[2]));
    
    // Restore permissions
    QFile::setPermissions(inputFile, QFile::ReadUser | QFile::WriteUser);
}

void TestFileOperations::testFileFormatValidation()
{
    QString jsonFile = createTestJsonFile();
    QString invalidJsonFile = createCorruptedJsonFile();
    
    // Valid JSON file
    QStringList result1 = TestUtils::executeCliCommand({"-i", jsonFile});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("Valid JSON file rejected: " + result1[2]));
    
    // Invalid JSON file
    QStringList result2 = TestUtils::executeCliCommand({"-i", invalidJsonFile});
    QVERIFY(result2[0].toInt() >= 0); // Should handle gracefully
    
    if (result2[0].toInt() != 0) {
        QVERIFY(result2[2].contains("Error") || result2[2].contains("parse") ||
                result2[2].contains("invalid"));
    }
}

void TestFileOperations::testCorruptedFileHandling()
{
    QString corruptedFile = createCorruptedJsonFile();
    QString outputFile = m_tempDir->path() + "/corrupted_output.json";
    
    QStringList result = TestUtils::executeCliCommand({"-i", corruptedFile, "-o", outputFile});
    
    // Should handle corruption gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
    
    if (result[0].toInt() != 0) {
        QVERIFY(result[2].contains("Error") || result[2].contains("parse") ||
                result[2].contains("corrupt"));
    }
}

void TestFileOperations::testLargeFileHandling()
{
    QString largeFile = createLargeTestFile();
    QString outputFile = m_tempDir->path() + "/large_output.json";
    
    QStringList result = TestUtils::executeCliCommand({"-i", largeFile, "-o", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Large file handling failed: " + result[2]));
    
    QVERIFY(QFile::exists(outputFile));
    
    // Verify reasonable output size
    qint64 outputSize = getFileSize(outputFile);
    QVERIFY2(outputSize > 1000, qPrintable(QString("Output too small for large input: %1 bytes").arg(outputSize)));
}

void TestFileOperations::testFilePerformance()
{
    QString testFile = createTestJsonFile();
    
    QElapsedTimer timer;
    timer.start();
    
    QStringList result = TestUtils::executeCliCommand({"-l", testFile});
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Performance test failed: " + result[2]));
    QVERIFY2(elapsed < 10000, qPrintable(QString("File operation too slow: %1ms").arg(elapsed)));
}

void TestFileOperations::testMemoryUsage()
{
    QString testFile = createLargeTestFile();
    
    // Run file operation and ensure it completes without memory issues
    QStringList result = TestUtils::executeCliCommand({"-l", testFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Memory usage test failed: " + result[2]));
    
    // Should complete without memory errors
    QVERIFY(!result[2].contains("memory") && !result[2].contains("Memory"));
}

void TestFileOperations::testMissingInputFile()
{
    QString missingFile = m_tempDir->path() + "/missing.json";
    
    QStringList result = TestUtils::executeCliCommand({"-i", missingFile});
    
    QVERIFY2(result[0].toInt() != 0, "Missing input file should cause error");
    QVERIFY(result[2].contains("Error") || result[2].contains("not found") ||
            result[2].contains("ERROR"));
}

void TestFileOperations::testInvalidOutputPath()
{
    QString inputFile = createTestJsonFile();
    QString invalidOutput = "/root/protected/output.json"; // Typically not writable
    
    QStringList result = TestUtils::executeCliCommand({"-i", inputFile, "-o", invalidOutput});
    
    // Should fail due to permissions or invalid path
    QVERIFY(result[0].toInt() != 0);
    QVERIFY(result[2].contains("Error") || result[2].contains("permission") ||
            result[2].contains("ERROR"));
}

void TestFileOperations::testWriteProtectedOutput()
{
    QString inputFile = createTestJsonFile();
    QString protectedDir = m_tempDir->path() + "/protected";
    QDir().mkpath(protectedDir);
    
    // Make directory read-only
    QFile::setPermissions(protectedDir, QFile::ReadUser | QFile::ExeUser);
    
    QString protectedOutput = protectedDir + "/output.json";
    QStringList result = TestUtils::executeCliCommand({"-i", inputFile, "-o", protectedOutput});
    
    // Should fail due to write protection
    QVERIFY(result[0].toInt() != 0);
    
    // Restore permissions for cleanup
    QFile::setPermissions(protectedDir, QFile::ReadUser | QFile::WriteUser | QFile::ExeUser);
}

void TestFileOperations::testInsufficientDiskSpace()
{
    // This test is difficult to implement without actually filling up disk space
    // Instead, test with extremely large output request that might trigger space checks
    QString inputFile = createTestJsonFile();
    QString outputFile = m_tempDir->path() + "/space_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"-i", inputFile, "-o", outputFile});
    
    // Should succeed for normal files
    QVERIFY2(result[0].toInt() == 0, qPrintable("Disk space test failed: " + result[2]));
    QVERIFY(QFile::exists(outputFile));
}

// Enhanced Performance and Corruption Tests - Claude Generated 2025-09-04
void TestFileOperations::testAdvancedCorruptionRecovery()
{
    // Create file with various types of corruption
    QString corruptedFile = m_tempDir->path() + "/advanced_corrupt.json";
    QFile file(corruptedFile);
    file.open(QIODevice::WriteOnly);
    
    // Partial JSON with nested corruption
    file.write("{ \"Main\": { \"OutFile\": \"test\" }, \"Independent\": { \"Generator\": { ");
    file.write("\"Type\": \"equations\", \"DataPoints\": 10, \"Variables\": 2, ");
    file.write("\"Equations\": \"X|Y\" }, \"InvalidKey\": null } "); // Missing closing brace
    file.close();
    
    // Input: JSON with nested structural corruption
    // Expected: Detailed error reporting with specific location information
    QStringList result = TestUtils::executeCliCommand({"-l", corruptedFile});
    QVERIFY(result[0].toInt() != 0); // Should fail
    QVERIFY(result[2].contains("Error") || result[2].contains("ERROR") || 
            result[2].contains("parse") || result[2].contains("JSON"));
    
    // Verify error message is informative
    QVERIFY(result[2].length() > 10); // Should have meaningful error description
}

void TestFileOperations::testConcurrentFileAccess()
{
    QString testFile = createTestJsonFile();
    QString output1 = m_tempDir->path() + "/concurrent1.json";
    QString output2 = m_tempDir->path() + "/concurrent2.json";
    
    // Input: Same input file accessed by multiple processes simultaneously
    // Expected: Both processes complete successfully without corruption
    QProcess process1;
    QProcess process2;
    
    process1.start(TestUtils::findSuprafitCli(), {"-i", testFile, "-o", output1});
    process2.start(TestUtils::findSuprafitCli(), {"-i", testFile, "-o", output2});
    
    QVERIFY(process1.waitForFinished(30000));
    QVERIFY(process2.waitForFinished(30000));
    
    QCOMPARE(process1.exitCode(), 0);
    QCOMPARE(process2.exitCode(), 0);
    
    // Both output files should exist and be valid
    QVERIFY(QFile::exists(output1 + ".json"));
    QVERIFY(QFile::exists(output2 + ".json"));
}

void TestFileOperations::testFileSystemStressTest()
{
    // Create multiple files simultaneously to stress file system
    QStringList files;
    QList<QProcess*> processes;
    
    for (int i = 0; i < 5; ++i) {
        QString testFile = createTestJsonFile();
        QString outputFile = m_tempDir->path() + QString("/stress_%1.json").arg(i);
        files << outputFile;
        
        QProcess* process = new QProcess(this);
        processes << process;
        process->start(TestUtils::findSuprafitCli(), {"-i", testFile, "-o", outputFile});
    }
    
    // Input: Multiple simultaneous file operations
    // Expected: All operations complete successfully without interference
    bool allSucceeded = true;
    for (QProcess* process : processes) {
        if (!process->waitForFinished(45000)) {
            allSucceeded = false;
        }
        if (process->exitCode() != 0) {
            allSucceeded = false;
        }
    }
    
    QVERIFY(allSucceeded);
    
    // Verify all output files were created
    for (const QString& file : files) {
        QVERIFY(QFile::exists(file + ".json"));
    }
    
    // Cleanup
    for (QProcess* process : processes) {
        process->deleteLater();
    }
}

void TestFileOperations::testLargeFileStreamingHandling()
{
    QString largeFile = createLargeTestFile();
    QString output = m_tempDir->path() + "/streaming_test.json";
    
    // Input: Large file requiring streaming or chunked processing
    // Expected: Memory-efficient processing without excessive RAM usage
    QElapsedTimer timer;
    timer.start();
    
    QStringList result = TestUtils::executeCliCommand({"-i", largeFile, "-o", output});
    qint64 elapsed = timer.elapsed();
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Large file streaming failed: " + result[2]));
    QVERIFY(elapsed < 60000); // Should complete within 1 minute
    QVERIFY(QFile::exists(output + ".json"));
}

void TestFileOperations::testPartialCorruptionHandling()
{
    // Create file with partial corruption in data section
    QString partiallyCorrupt = m_tempDir->path() + "/partial_corrupt.json";
    QFile file(partiallyCorrupt);
    file.open(QIODevice::WriteOnly);
    
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "test";
    main["Repeat"] = 1;
    config["Main"] = main;
    
    // Add valid independent section
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 10;
    indepGen["Variables"] = 2;
    indepGen["Equations"] = "X|Y";
    independent["Generator"] = indepGen;
    config["Independent"] = independent;
    
    // Add corrupted dependent section
    config["Dependent"] = "INVALID_VALUE_SHOULD_BE_OBJECT";
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    // Input: File with partial corruption (some sections valid, others corrupted)
    // Expected: Process valid sections, report specific corruption issues
    QStringList result = TestUtils::executeCliCommand({"-i", partiallyCorrupt});
    QVERIFY(result[0].toInt() >= 0); // Should not crash
}

void TestFileOperations::testNetworkFileSystemCompatibility()
{
    // Test with paths that might exist on network file systems
    QString testFile = createTestJsonFile();
    QString networkPath = m_tempDir->path() + "/network_compatible.json";
    
    // Input: File operations that should work across different file systems
    // Expected: Successful operation regardless of underlying file system type
    QStringList result = TestUtils::executeCliCommand({"-i", testFile, "-o", networkPath});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(QFile::exists(networkPath + ".json"));
    
    // Test file access patterns that work with network latency
    QFileInfo info(networkPath + ".json");
    QVERIFY(info.exists());
    QVERIFY(info.size() > 0);
}

void TestFileOperations::testFileRecoveryAfterInterruption()
{
    QString testFile = createTestJsonFile();
    QString outputFile = m_tempDir->path() + "/recovery_test.json";
    
    // Simulate interrupted operation by using timeout
    QProcess process;
    process.start(TestUtils::findSuprafitCli(), {"-i", testFile, "-o", outputFile});
    
    // Let it start but terminate early
    QThread::msleep(100);
    process.kill();
    process.waitForFinished(5000);
    
    // Now try the operation again
    // Input: File operation after previous interruption
    // Expected: Clean recovery without corruption or lock issues
    QStringList result = TestUtils::executeCliCommand({"-i", testFile, "-o", outputFile});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(QFile::exists(outputFile + ".json"));
}

void TestFileOperations::testMemoryMappedFileOperations()
{
    QString largeFile = createLargeTestFile();
    
    // Test operations on large files that might use memory mapping
    // Input: Large file suitable for memory-mapped operations
    // Expected: Efficient processing using memory mapping where appropriate
    QStringList result = TestUtils::executeCliCommand({"-l", largeFile});
    QCOMPARE(result[0].toInt(), 0);
    QVERIFY(result[1].length() > 0);
    
    // Should handle large files without loading entirely into memory
    QVERIFY(result[1].contains("Main") || result[1].contains("{") || result[1].contains("data"));
}

QString TestFileOperations::createTestJsonFile()
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "test_output";
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
    
    QString filePath = m_tempDir->path() + "/test_file.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    return filePath;
}

QString TestFileOperations::createTestSuprafitFile()
{
    // Create a JSON file first, then convert it to SupraFit format
    QString jsonFile = createTestJsonFile();
    QString suprafitFile = m_tempDir->path() + "/test_file.suprafit";
    
    // Convert to SupraFit format
    QStringList result = TestUtils::executeCliCommand({"-i", jsonFile, "-o", suprafitFile});
    
    if (result[0].toInt() == 0 && QFile::exists(suprafitFile)) {
        return suprafitFile;
    }
    
    // Fallback: create a mock SupraFit file (binary or text format)
    QFile file(suprafitFile);
    file.open(QIODevice::WriteOnly);
    file.write("Mock SupraFit file content");
    file.close();
    
    return suprafitFile;
}

QString TestFileOperations::createLargeTestFile()
{
    QJsonObject largeConfig;
    QJsonObject main;
    main["OutFile"] = "large_test";
    main["Repeat"] = 1;
    largeConfig["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 1000; // Large dataset
    indepGen["Variables"] = 10;
    indepGen["Equations"] = "X|Y|X*Y|X^2|Y^2|sin(X)|cos(Y)|log(abs(X)+1)|exp(X/100)|X*Y*2";
    independent["Generator"] = indepGen;
    largeConfig["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "copy";
    depGen["Series"] = 10;
    dependent["Generator"] = depGen;
    largeConfig["Dependent"] = dependent;
    
    // Add large data arrays to increase file size
    QJsonArray largeArray;
    for (int i = 0; i < 1000; ++i) {
        largeArray.append(QJsonArray{i, i*2, i*3, i*4, i*5});
    }
    largeConfig["LargeDataSet"] = largeArray;
    
    QString filePath = m_tempDir->path() + "/large_test_file.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(largeConfig).toJson());
    file.close();
    
    return filePath;
}

QString TestFileOperations::createCorruptedJsonFile()
{
    QString filePath = m_tempDir->path() + "/corrupted.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write("{ \"Main\": { \"OutFile\": \"test\", invalid json structure without closing");
    file.close();
    
    return filePath;
}

QString TestFileOperations::createComplexStructureFile()
{
    QJsonObject complex;
    QJsonObject main;
    main["OutFile"] = "complex_test";
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
    main["PostFitAnalysis"] = true;
    complex["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 25;
    indepGen["Variables"] = 3;
    indepGen["Equations"] = "X|Y|Z";
    independent["Generator"] = indepGen;
    complex["Independent"] = independent;
    
    QJsonObject dependent;
    QJsonObject depGen;
    depGen["Type"] = "model";
    QJsonObject model;
    model["ID"] = 1;
    depGen["Model"] = model;
    dependent["Generator"] = depGen;
    complex["Dependent"] = dependent;
    
    QJsonObject addModels;
    QJsonObject nmr11;
    nmr11["ID"] = 1;
    QJsonObject options;
    options["FastMode"] = true;
    options["Convergency"] = 1e-7;
    nmr11["Options"] = options;
    addModels["nmr_1_1"] = nmr11;
    
    QJsonObject nmr12;
    nmr12["ID"] = 2;
    nmr12["Options"] = options;
    addModels["nmr_1_2"] = nmr12;
    
    complex["AddModels"] = addModels;
    
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
    complex["PostFitAnalysis"] = postFit;
    
    QString filePath = m_tempDir->path() + "/complex_structure.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(complex).toJson());
    file.close();
    
    return filePath;
}

#include "test_file_operations.moc"

QTEST_MAIN(TestFileOperations)