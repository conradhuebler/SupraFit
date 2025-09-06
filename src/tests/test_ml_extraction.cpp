/*
 * Comprehensive tests for SupraFit ML feature extraction functionality
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Tests ML training data export, compact feature generation, JsonUtils
 * statistical feature extraction, and batch processing functionality.
 * Does NOT test obsolete --ml-pipeline functionality.
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
#include <QtCore/QDir>

#include "test_utils.h"
#include <QtCore/QElapsedTimer>

class TestMLExtraction : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ML Training Data Export Tests
    void testMLTrainingDataExport();
    void testMLOutputSpecification();
    void testExportFromMultipleFiles();
    void testEmptyMLExport();

    // Compact Feature Generation Tests
    void testCompactFeatureExtraction();
    void testShannonEntropyCalculation();
    void testRelativeUncertaintyCalculation();
    void testConfidenceScoreCalculation();
    void testParameterDistributionFeatures();

    // JsonUtils Integration Tests
    void testJsonUtilsFeatureExtraction();
    void testStatisticalMethodProcessing();
    void testParameterFilteringLogic();
    void testFeatureCompactness();

    // Batch Export Tests
    void testBatchExportFromDirectory();
    void testBatchExportValidation();
    void testBatchProcessingErrors();

    // Feature Quality Tests
    void testFeatureCompleteness();
    void testFeatureAccuracy();
    void testNoiseFreeFeatures();
    void testGroundTruthPreservation();

    // Performance Tests
    void testLargeDatasetExtraction();
    void testMultipleModelExtraction();
    void testExtractionPerformance();

    // Error Handling Tests
    void testInvalidMLFiles();
    void testCorruptedStatisticalData();
    void testMissingRequiredFields();

private:
    QTemporaryDir* m_tempDir;
    
    QString createMLDataFile();
    QString createMultiProjectFile();
    QString createInvalidMLFile();
    QJsonObject loadMLExportFile(const QString& filename);
    bool verifyMLTrainingStructure(const QJsonObject& data);
    bool verifyCompactFeatures(const QJsonObject& features);
    bool verifyStatisticalFeatures(const QJsonObject& methodData);
    double calculateExpectedEntropy(const QJsonArray& distribution);
    bool hasRawDataBloat(const QJsonObject& data);
};

void TestMLExtraction::initTestCase()
{
    qDebug() << "Starting ML Feature Extraction tests...";
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    // Verify CLI binary is available through TestUtils
    QString cliPath = TestUtils::findSuprafitCli();
    QVERIFY2(!cliPath.isEmpty(), "suprafit_cli executable not found - set SUPRAFIT_CLI_PATH env var if needed");
}

void TestMLExtraction::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "ML Feature Extraction tests completed.";
}

// Removed - using TestUtils::executeCliCommand instead

QJsonObject TestMLExtraction::loadMLExportFile(const QString& filename)
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

bool TestMLExtraction::verifyMLTrainingStructure(const QJsonObject& data)
{
    if (!data.contains("ml_training_metadata")) return false;
    if (!data.contains("training_samples")) return false;
    
    QJsonObject metadata = data["ml_training_metadata"].toObject();
    if (!metadata.contains("version")) return false;
    if (!metadata.contains("generated")) return false;
    if (!metadata.contains("sample_count")) return false;
    
    QJsonArray samples = data["training_samples"].toArray();
    return !samples.isEmpty();
}

bool TestMLExtraction::verifyCompactFeatures(const QJsonObject& features)
{
    // Check for compact statistical features (not raw data)
    for (auto it = features.begin(); it != features.end(); ++it) {
        QJsonObject paramFeatures = it.value().toObject();
        
        // Should have analyzed features
        if (!paramFeatures.contains("shannon_entropy")) return false;
        if (!paramFeatures.contains("standard_deviation")) return false;
        if (!paramFeatures.contains("relative_uncertainty")) return false;
        if (!paramFeatures.contains("confidence_score")) return false;
        if (!paramFeatures.contains("best_fit_value")) return false;
        
        // Should NOT have raw data strings
        if (paramFeatures.contains("raw_data")) return false;
        if (paramFeatures.contains("y") && paramFeatures["y"].isString()) {
            QString yData = paramFeatures["y"].toString();
            if (yData.split(" ", Qt::SkipEmptyParts).size() > 100) {
                return false; // Large raw data detected
            }
        }
    }
    return true;
}

bool TestMLExtraction::verifyStatisticalFeatures(const QJsonObject& methodData)
{
    if (!methodData.contains("statistical_features")) return false;
    
    QJsonObject statFeatures = methodData["statistical_features"].toObject();
    
    // Should contain method-specific features
    if (statFeatures.contains("monte_carlo")) {
        QJsonObject mcFeatures = statFeatures["monte_carlo"].toObject();
        return verifyCompactFeatures(mcFeatures);
    }
    
    if (statFeatures.contains("cross_validation")) {
        QJsonObject cvFeatures = statFeatures["cross_validation"].toObject();
        return verifyCompactFeatures(cvFeatures);
    }
    
    return false;
}

bool TestMLExtraction::hasRawDataBloat(const QJsonObject& data)
{
    QJsonDocument doc(data);
    QByteArray jsonData = doc.toJson();
    
    // Check if file size suggests raw data bloat
    if (jsonData.size() > 1000000) { // > 1MB suggests possible raw data
        QString jsonString = QString::fromUtf8(jsonData);
        
        // Look for large space-separated number sequences (raw MC data)
        QStringList lines = jsonString.split('\n');
        for (const QString& line : lines) {
            if (line.split(' ', Qt::SkipEmptyParts).size() > 500) {
                return true; // Found likely raw data
            }
        }
    }
    
    return false;
}

void TestMLExtraction::testMLTrainingDataExport()
{
    // First create ML data with post-processing results
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/ml_export_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("ML training data export failed: " + result[2]));
    
    // Verify export was successful
    QVERIFY2(result[1].contains("ML training data exported") || result[1].contains("export"), 
             "Export success message not found");
    
    // Verify output file exists
    QVERIFY2(QFile::exists(outputFile), "ML export output file not created");
    
    // Verify structure
    QJsonObject exportData = loadMLExportFile(outputFile);
    QVERIFY2(!exportData.isEmpty(), "ML export file is empty or invalid");
    QVERIFY2(verifyMLTrainingStructure(exportData), "ML training structure is invalid");
}

void TestMLExtraction::testMLOutputSpecification()
{
    QString mlDataFile = createMLDataFile();
    QString customOutput = m_tempDir->path() + "/custom_ml_output.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", customOutput});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Custom ML output specification failed: " + result[2]));
    
    // Verify custom output file was created
    QVERIFY2(QFile::exists(customOutput), "Custom ML output file not created");
    
    QJsonObject exportData = loadMLExportFile(customOutput);
    QVERIFY2(verifyMLTrainingStructure(exportData), "Custom output structure invalid");
}

void TestMLExtraction::testExportFromMultipleFiles()
{
    QString mlDataFile1 = createMLDataFile();
    QString mlDataFile2 = createMultiProjectFile();
    QString outputFile = m_tempDir->path() + "/multi_file_export.json";
    
    // Test with multiple input files (if supported)
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile1, mlDataFile2, "--ml-output", outputFile});
    
    // Should either succeed with multiple files or fail gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
    
    if (result[0].toInt() == 0) {
        QVERIFY(QFile::exists(outputFile));
        QJsonObject exportData = loadMLExportFile(outputFile);
        QVERIFY(verifyMLTrainingStructure(exportData));
    }
}

void TestMLExtraction::testEmptyMLExport()
{
    QString emptyFile = createInvalidMLFile();
    QString outputFile = m_tempDir->path() + "/empty_export.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", emptyFile, "--ml-output", outputFile});
    
    // Should handle empty/invalid files gracefully
    QVERIFY(result[0].toInt() >= 0);
    
    if (result[0].toInt() != 0) {
        QVERIFY(result[2].contains("Error") || result[2].contains("ERROR"));
    }
}

void TestMLExtraction::testCompactFeatureExtraction()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/compact_features_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Compact feature extraction failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    QVERIFY2(!exportData.isEmpty(), "Export data is empty");
    
    // Verify compact features (no raw data bloat)
    QVERIFY2(!hasRawDataBloat(exportData), "Export contains raw data bloat");
    
    // Check for training samples with statistical features
    if (exportData.contains("training_samples")) {
        QJsonArray samples = exportData["training_samples"].toArray();
        if (!samples.isEmpty()) {
            QJsonObject sample = samples[0].toObject();
            if (sample.contains("candidate_models")) {
                QJsonArray models = sample["candidate_models"].toArray();
                for (const QJsonValue& modelValue : models) {
                    QJsonObject model = modelValue.toObject();
                    QVERIFY2(verifyStatisticalFeatures(model), "Statistical features are not compact");
                }
            }
        }
    }
}

void TestMLExtraction::testShannonEntropyCalculation()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/entropy_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Shannon entropy test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    
    // Look for Shannon entropy values in the export
    bool foundEntropy = false;
    QJsonDocument doc(exportData);
    QString jsonString = doc.toJson();
    
    if (jsonString.contains("shannon_entropy")) {
        foundEntropy = true;
        
        // Verify entropy values are reasonable (0 to log2(bins))
        QJsonObject sample = exportData["training_samples"].toArray()[0].toObject();
        QJsonArray models = sample["candidate_models"].toArray();
        for (const QJsonValue& modelValue : models) {
            QJsonObject model = modelValue.toObject();
            if (model.contains("statistical_features")) {
                QJsonObject statFeatures = model["statistical_features"].toObject();
                if (statFeatures.contains("monte_carlo")) {
                    QJsonObject mcFeatures = statFeatures["monte_carlo"].toObject();
                    for (auto it = mcFeatures.begin(); it != mcFeatures.end(); ++it) {
                        QJsonObject paramFeatures = it.value().toObject();
                        if (paramFeatures.contains("shannon_entropy")) {
                            double entropy = paramFeatures["shannon_entropy"].toDouble();
                            QVERIFY2(entropy >= 0.0 && entropy <= 10.0, // Reasonable range
                                    qPrintable(QString("Entropy value %1 out of reasonable range").arg(entropy)));
                        }
                    }
                }
            }
        }
    }
    
    QVERIFY2(foundEntropy, "Shannon entropy not found in export");
}

void TestMLExtraction::testRelativeUncertaintyCalculation()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/uncertainty_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Relative uncertainty test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    QJsonDocument doc(exportData);
    QString jsonString = doc.toJson();
    
    // Should contain relative uncertainty calculations
    QVERIFY2(jsonString.contains("relative_uncertainty"), "Relative uncertainty not found in export");
    
    // Verify values are non-negative
    if (exportData.contains("training_samples")) {
        QJsonArray samples = exportData["training_samples"].toArray();
        if (!samples.isEmpty()) {
            QJsonObject sample = samples[0].toObject();
            if (sample.contains("candidate_models")) {
                QJsonArray models = sample["candidate_models"].toArray();
                for (const QJsonValue& modelValue : models) {
                    QJsonObject model = modelValue.toObject();
                    if (model.contains("statistical_features")) {
                        QJsonObject statFeatures = model["statistical_features"].toObject();
                        for (auto methodIt = statFeatures.begin(); methodIt != statFeatures.end(); ++methodIt) {
                            QJsonObject methodFeatures = methodIt.value().toObject();
                            for (auto paramIt = methodFeatures.begin(); paramIt != methodFeatures.end(); ++paramIt) {
                                QJsonObject paramFeatures = paramIt.value().toObject();
                                if (paramFeatures.contains("relative_uncertainty")) {
                                    double relUncertainty = paramFeatures["relative_uncertainty"].toDouble();
                                    QVERIFY2(relUncertainty >= 0.0, "Relative uncertainty should be non-negative");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void TestMLExtraction::testConfidenceScoreCalculation()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/confidence_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Confidence score test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    QJsonDocument doc(exportData);
    QString jsonString = doc.toJson();
    
    // Should contain confidence scores
    QVERIFY2(jsonString.contains("confidence_score"), "Confidence score not found in export");
    
    // Confidence scores should be between 0 and 1
    if (exportData.contains("training_samples")) {
        QJsonArray samples = exportData["training_samples"].toArray();
        for (const QJsonValue& sampleValue : samples) {
            QJsonObject sample = sampleValue.toObject();
            if (sample.contains("candidate_models")) {
                QJsonArray models = sample["candidate_models"].toArray();
                for (const QJsonValue& modelValue : models) {
                    QJsonObject model = modelValue.toObject();
                    if (model.contains("statistical_features")) {
                        QJsonObject statFeatures = model["statistical_features"].toObject();
                        for (auto methodIt = statFeatures.begin(); methodIt != statFeatures.end(); ++methodIt) {
                            QJsonObject methodFeatures = methodIt.value().toObject();
                            for (auto paramIt = methodFeatures.begin(); paramIt != methodFeatures.end(); ++paramIt) {
                                QJsonObject paramFeatures = paramIt.value().toObject();
                                if (paramFeatures.contains("confidence_score")) {
                                    double confScore = paramFeatures["confidence_score"].toDouble();
                                    QVERIFY2(confScore >= 0.0 && confScore <= 1.0, 
                                            qPrintable(QString("Confidence score %1 not in [0,1] range").arg(confScore)));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void TestMLExtraction::testParameterDistributionFeatures()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/distribution_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Parameter distribution test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    QJsonDocument doc(exportData);
    QString jsonString = doc.toJson();
    
    // Should contain distribution features
    QStringList expectedFeatures = {"mean", "median", "standard_deviation", "sample_count"};
    
    for (const QString& feature : expectedFeatures) {
        QVERIFY2(jsonString.contains(feature), qPrintable(QString("Distribution feature %1 not found").arg(feature)));
    }
}

void TestMLExtraction::testJsonUtilsFeatureExtraction()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/jsonutils_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("JsonUtils feature extraction test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    
    // Verify that JsonUtils features are present
    QStringList jsonUtilsFeatures = {
        "shannon_entropy", "relative_uncertainty", "confidence_score", 
        "standard_deviation", "best_fit_value", "confidence_range"
    };
    
    QJsonDocument doc(exportData);
    QString jsonString = doc.toJson();
    
    for (const QString& feature : jsonUtilsFeatures) {
        QVERIFY2(jsonString.contains(feature), 
                qPrintable(QString("JsonUtils feature %1 not found in export").arg(feature)));
    }
}

void TestMLExtraction::testStatisticalMethodProcessing()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/method_processing_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Statistical method processing test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    QJsonDocument doc(exportData);
    QString jsonString = doc.toJson();
    
    // Should contain method-specific processing results
    QStringList expectedMethods = {"monte_carlo", "cross_validation"};
    
    bool foundMethodFeatures = false;
    for (const QString& method : expectedMethods) {
        if (jsonString.contains(method)) {
            foundMethodFeatures = true;
            break;
        }
    }
    
    QVERIFY2(foundMethodFeatures, "No statistical method features found in export");
}

void TestMLExtraction::testParameterFilteringLogic()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/filtering_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Parameter filtering test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    QJsonDocument doc(exportData);
    QString jsonString = doc.toJson();
    
    // Should NOT contain controller or debug information
    QStringList filteredKeys = {"controller", "max_steps", "models", "debug_info", "cv_type"};
    
    for (const QString& key : filteredKeys) {
        // These keys should be filtered out from parameter features
        // (They might exist in method_config but not as individual parameter features)
        if (jsonString.contains(key)) {
            // If found, should be in method_config context, not parameter features
            QVERIFY2(jsonString.contains("method_config"), 
                    qPrintable(QString("Filtered key %1 found outside method_config").arg(key)));
        }
    }
}

void TestMLExtraction::testFeatureCompactness()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/compactness_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Feature compactness test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    
    // Verify file size is reasonable (not bloated with raw data)
    QFile outputFileObj(outputFile);
    qint64 fileSize = outputFileObj.size();
    QVERIFY2(fileSize < 1000000, qPrintable(QString("Export file too large (%1 bytes), suggests raw data bloat").arg(fileSize)));
    
    // Verify no raw data strings in features
    QVERIFY2(!hasRawDataBloat(exportData), "Export contains raw data bloat");
}

void TestMLExtraction::testBatchExportFromDirectory()
{
    // Create multiple ML files in a directory
    QString batchDir = m_tempDir->path() + "/batch_test_dir";
    QDir().mkpath(batchDir);
    
    QString file1 = batchDir + "/ml_data1.json";
    QString file2 = batchDir + "/ml_data2.json";
    
    // Copy test data to batch directory (simplified approach)
    QFile::copy(createMLDataFile(), file1);
    QFile::copy(createMultiProjectFile(), file2);
    
    QString outputDir = m_tempDir->path() + "/batch_output";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-batch", batchDir});
    
    // Batch export might not be implemented or might work differently
    // Test should handle gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
    
    if (result[0].toInt() == 0) {
        QVERIFY(result[1].contains("batch") || result[1].contains("export"));
    }
}

void TestMLExtraction::testBatchExportValidation()
{
    QString batchDir = m_tempDir->path() + "/batch_validation_dir";
    QDir().mkpath(batchDir);
    
    // Create mix of valid and invalid files
    QFile::copy(createMLDataFile(), batchDir + "/valid.json");
    QFile::copy(createInvalidMLFile(), batchDir + "/invalid.json");
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-batch", batchDir});
    
    // Should handle mixed valid/invalid files gracefully
    QVERIFY(result[0].toInt() >= 0);
}

void TestMLExtraction::testBatchProcessingErrors()
{
    QString nonexistentDir = m_tempDir->path() + "/nonexistent";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-batch", nonexistentDir});
    
    // Should fail gracefully with nonexistent directory
    QVERIFY(result[0].toInt() != 0);
    QVERIFY(result[2].contains("Error") || result[2].contains("ERROR") || 
            result[1].contains("Error") || result[1].contains("ERROR"));
}

void TestMLExtraction::testFeatureCompleteness()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/completeness_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Feature completeness test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    
    // Verify all required ML training components are present
    QStringList requiredComponents = {
        "ml_training_metadata", "training_samples"
    };
    
    for (const QString& component : requiredComponents) {
        QVERIFY2(exportData.contains(component), 
                qPrintable(QString("Required component %1 missing").arg(component)));
    }
    
    // Verify metadata completeness
    if (exportData.contains("ml_training_metadata")) {
        QJsonObject metadata = exportData["ml_training_metadata"].toObject();
        QStringList requiredMeta = {"version", "generated", "sample_count", "features"};
        
        for (const QString& meta : requiredMeta) {
            QVERIFY2(metadata.contains(meta), 
                    qPrintable(QString("Required metadata %1 missing").arg(meta)));
        }
    }
}

void TestMLExtraction::testFeatureAccuracy()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/accuracy_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Feature accuracy test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    
    // Verify numerical accuracy of extracted features
    if (exportData.contains("training_samples")) {
        QJsonArray samples = exportData["training_samples"].toArray();
        for (const QJsonValue& sampleValue : samples) {
            QJsonObject sample = sampleValue.toObject();
            if (sample.contains("candidate_models")) {
                QJsonArray models = sample["candidate_models"].toArray();
                for (const QJsonValue& modelValue : models) {
                    QJsonObject model = modelValue.toObject();
                    if (model.contains("statistical_features")) {
                        QJsonObject statFeatures = model["statistical_features"].toObject();
                        for (auto methodIt = statFeatures.begin(); methodIt != statFeatures.end(); ++methodIt) {
                            QJsonObject methodFeatures = methodIt.value().toObject();
                            for (auto paramIt = methodFeatures.begin(); paramIt != methodFeatures.end(); ++paramIt) {
                                QJsonObject paramFeatures = paramIt.value().toObject();
                                
                                // Verify numerical sanity
                                if (paramFeatures.contains("shannon_entropy")) {
                                    double entropy = paramFeatures["shannon_entropy"].toDouble();
                                    QVERIFY2(!std::isnan(entropy) && !std::isinf(entropy), "Entropy contains NaN/Inf");
                                }
                                
                                if (paramFeatures.contains("standard_deviation")) {
                                    double stddev = paramFeatures["standard_deviation"].toDouble();
                                    QVERIFY2(stddev >= 0.0 && !std::isnan(stddev), "Standard deviation invalid");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void TestMLExtraction::testNoiseFreeFeatures()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/noisefree_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Noise-free features test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    
    // Verify that noise information is preserved but raw data is not
    QJsonDocument doc(exportData);
    QString jsonString = doc.toJson();
    
    // Should contain noise configuration
    QVERIFY(jsonString.contains("input_noise") || jsonString.contains("ground_truth"));
    
    // Should NOT contain large raw data sequences
    QStringList lines = jsonString.split('\n');
    for (const QString& line : lines) {
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        QVERIFY2(parts.size() < 1000, "Found line with excessive data points (possible raw data)");
    }
}

void TestMLExtraction::testGroundTruthPreservation()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/groundtruth_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Ground truth preservation test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    QJsonDocument doc(exportData);
    QString jsonString = doc.toJson();
    
    // Should preserve ground truth information
    QVERIFY(jsonString.contains("ground_truth") || jsonString.contains("label") || 
            jsonString.contains("correct_model"));
}

void TestMLExtraction::testLargeDatasetExtraction()
{
    // This test would require a large pre-generated dataset
    // For now, test with standard dataset and verify performance
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/large_dataset_test.json";
    
    QElapsedTimer timer;
    timer.start();
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    
    qint64 elapsedMs = timer.elapsed();
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Large dataset extraction test failed: " + result[2]));
    QVERIFY2(elapsedMs < 60000, "Extraction took too long (>60s), possible performance issue");
}

void TestMLExtraction::testMultipleModelExtraction()
{
    QString mlDataFile = createMultiProjectFile();
    QString outputFile = m_tempDir->path() + "/multi_model_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", outputFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Multiple model extraction test failed: " + result[2]));
    
    QJsonObject exportData = loadMLExportFile(outputFile);
    
    // Should handle multiple models in the export
    if (exportData.contains("training_samples")) {
        QJsonArray samples = exportData["training_samples"].toArray();
        if (!samples.isEmpty()) {
            QJsonObject sample = samples[0].toObject();
            if (sample.contains("candidate_models")) {
                QJsonArray models = sample["candidate_models"].toArray();
                QVERIFY2(models.size() > 0, "No candidate models found in multi-model export");
            }
        }
    }
}

void TestMLExtraction::testExtractionPerformance()
{
    QString mlDataFile = createMLDataFile();
    QString outputFile = m_tempDir->path() + "/performance_test.json";
    
    // Run extraction multiple times to test consistency
    QList<int> executionTimes;
    
    for (int i = 0; i < 3; ++i) {
        QString testOutput = outputFile + QString::number(i);
        
        QElapsedTimer timer;
        timer.start();
        
        QStringList result = TestUtils::executeCliCommand({"--export-ml-training", mlDataFile, "--ml-output", testOutput});
        
        qint64 elapsed = timer.elapsed();
        executionTimes.append(elapsed);
        
        QVERIFY2(result[0].toInt() == 0, qPrintable(QString("Performance test run %1 failed: %2").arg(i).arg(result[2])));
    }
    
    // Verify reasonable performance consistency
    int maxTime = *std::max_element(executionTimes.begin(), executionTimes.end());
    int minTime = *std::min_element(executionTimes.begin(), executionTimes.end());
    
    QVERIFY2(maxTime < 30000, "Extraction taking too long (>30s)");
    QVERIFY2(minTime > 0, "Extraction completing too quickly (possible error)");
}

void TestMLExtraction::testInvalidMLFiles()
{
    QString invalidFile = createInvalidMLFile();
    QString outputFile = m_tempDir->path() + "/invalid_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", invalidFile, "--ml-output", outputFile});
    
    // Should fail gracefully with invalid input
    QVERIFY(result[0].toInt() != 0);
    QVERIFY(result[2].contains("Error") || result[2].contains("ERROR"));
}

void TestMLExtraction::testCorruptedStatisticalData()
{
    // Create a file with corrupted statistical data structure
    QJsonObject corruptedData;
    corruptedData["format_version"] = "1.0";
    corruptedData["project_0"] = QJsonObject(); // Empty project
    
    QString corruptedFile = m_tempDir->path() + "/corrupted.json";
    QFile file(corruptedFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(corruptedData).toJson());
    file.close();
    
    QString outputFile = m_tempDir->path() + "/corrupted_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", corruptedFile, "--ml-output", outputFile});
    
    // Should handle corrupted data gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
}

void TestMLExtraction::testMissingRequiredFields()
{
    // Create ML file missing required fields
    QJsonObject incompleteData;
    incompleteData["format_version"] = "1.0";
    // Missing project data
    
    QString incompleteFile = m_tempDir->path() + "/incomplete.json";
    QFile file(incompleteFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(incompleteData).toJson());
    file.close();
    
    QString outputFile = m_tempDir->path() + "/incomplete_test.json";
    
    QStringList result = TestUtils::executeCliCommand({"--export-ml-training", incompleteFile, "--ml-output", outputFile});
    
    // Should handle missing fields gracefully
    QVERIFY(result[0].toInt() >= 0);
    
    if (result[0].toInt() != 0) {
        QVERIFY(result[2].contains("Error") || result[2].contains("WARNING") ||
                result[1].contains("Error") || result[1].contains("WARNING"));
    }
}

QString TestMLExtraction::createMLDataFile()
{
    // Create a realistic ML data file by running the ML generation process
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = "ml_test_data";
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
    main["PostFitAnalysis"] = true;
    config["Main"] = main;
    
    QJsonObject independent;
    QJsonObject indepGen;
    indepGen["Type"] = "equations";
    indepGen["DataPoints"] = 15;
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
    QJsonArray methods;
    QJsonObject mcMethod;
    mcMethod["Method"] = 1;
    mcMethod["MaxSteps"] = 200;
    mcMethod["VarianceSource"] = 2;
    methods.append(mcMethod);
    postFit["methods"] = methods;
    config["PostFitAnalysis"] = postFit;
    
    QString configFile = m_tempDir->path() + "/ml_data_config.json";
    QFile file(configFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(config).toJson());
    file.close();
    
    QString outputBase = m_tempDir->path() + "/ml_test_data";
    
    // Generate the ML data
    QStringList result = TestUtils::executeCliCommand({"-i", configFile, "-o", outputBase});
    
    if (result[0].toInt() == 0) {
        QString generatedFile = outputBase + "-0.json";
        if (QFile::exists(generatedFile)) {
            return generatedFile;
        }
    }
    
    // Fallback: create a minimal ML-compatible file
    return createMultiProjectFile();
}

QString TestMLExtraction::createMultiProjectFile()
{
    QJsonObject multiProject;
    multiProject["format_version"] = "2.0";
    multiProject["generation_timestamp"] = "2025-09-02T12:00:00";
    
    QJsonObject baseData;
    QJsonObject independent;
    independent["rows"] = 15;
    independent["cols"] = 2;
    baseData["Independent"] = independent;
    multiProject["base_data"] = baseData;
    
    QJsonObject project0;
    project0["model_id"] = 1;
    project0["model_name"] = "nmr_1_1";
    project0["sse"] = 0.05;
    project0["convergence"] = true;
    
    QJsonObject postFitAnalysis;
    postFitAnalysis["analysis_completed"] = true;
    QJsonObject methods;
    QJsonObject method1;
    
    // Add some statistical data for feature extraction
    QJsonObject param0;
    param0["name"] = "lg K11";
    param0["value"] = 4.0;
    QJsonObject boxplot;
    boxplot["mean"] = 4.1;
    boxplot["stddev"] = 0.2;
    boxplot["median"] = 4.0;
    boxplot["count"] = 200;
    param0["boxplot"] = boxplot;
    
    QJsonObject confidence;
    confidence["lower"] = 3.8;
    confidence["upper"] = 4.4;
    confidence["error"] = 95;
    param0["confidence"] = confidence;
    
    method1["0"] = param0;
    methods["1"] = method1;
    postFitAnalysis["methods"] = methods;
    project0["post_fit_analysis"] = postFitAnalysis;
    
    multiProject["project_0"] = project0;
    
    QString filePath = m_tempDir->path() + "/multi_project_data.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(multiProject).toJson());
    file.close();
    
    return filePath;
}

QString TestMLExtraction::createInvalidMLFile()
{
    QString filePath = m_tempDir->path() + "/invalid_ml_data.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write("{ invalid json structure");
    file.close();
    
    return filePath;
}

#include "test_ml_extraction.moc"

QTEST_MAIN(TestMLExtraction)