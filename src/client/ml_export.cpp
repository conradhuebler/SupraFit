/*
 * SupraFit CLI - Machine-learning training-data export
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Extracted from suprafit_cli.cpp (CLI decomposition, TECHNICAL_DEBT.md D3).
 * Claude Generated.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ml_export.h"

#include "src/capabilities/mlfeatureextractor.h"
#include "src/core/projectmanager.h"

#include <QtCore/QDir>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include <fmt/core.h>

// ML Training Data Export Implementation - Claude Generated
bool MlExport::exportMLTrainingData(const QVector<QString>& inputFiles, const QString& outputFile, bool excludeRawData)
{
    if (inputFiles.isEmpty()) {
        fmt::print("❌ ERROR: No input files provided for ML training data export\n");
        return false;
    }

    MLFeatureExtractor* extractor = new MLFeatureExtractor(nullptr);

    // Configure extraction options - Claude Generated
    extractor->setExtractionOptions(
        true,               // includeAdvancedStats
        false,              // includeFitParameters (keep compact for NN)
        true,               // includeInputNoise
        !excludeRawData     // includeRawData (invert excludeRawData flag)
    );
    
    fmt::print("🔧 Extracting ML training data from {} files...\n", inputFiles.size());
    
    // Extract training samples from all input files
    QVector<QJsonObject> trainingSamples = extractor->extractBatchTrainingData(inputFiles);
    
    if (trainingSamples.isEmpty()) {
        fmt::print("❌ ERROR: No training samples could be extracted from input files\n");
        delete extractor;
        return false;
    }
    
    // Export in neural network format
    QJsonObject neuralNetData = extractor->exportNeuralNetFormat(trainingSamples);
    
    // Save to file via ProjectManager (mirrors SupraFitCli::SaveFile) - Claude Generated
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();
    QString tempProjectId = pm.createProjectFromJson(neuralNetData, "Temporary CLI Project");
    bool success = !tempProjectId.isEmpty() && pm.saveProject(outputFile, tempProjectId);
    if (!tempProjectId.isEmpty())
        pm.removeProject(tempProjectId);
    
    if (success) {
        fmt::print("✅ ML training data exported: {} samples → {}\n", trainingSamples.size(), outputFile.toStdString());
    } else {
        fmt::print("❌ ERROR: Failed to save ML training data to {}\n", outputFile.toStdString());
    }
    
    delete extractor;
    return success;
}

bool MlExport::exportMLTrainingDataSingle(const QString& inputFile, const QString& outputFile, bool excludeRawData)
{
    return exportMLTrainingData(QVector<QString>() << inputFile, outputFile, excludeRawData);
}

bool MlExport::exportMLTrainingDataBatch(const QString& inputDirectory, const QString& outputFile, bool excludeRawData)
{
    QDir dir(inputDirectory);
    if (!dir.exists()) {
        fmt::print("❌ ERROR: Input directory does not exist: {}\n", inputDirectory.toStdString());
        return false;
    }

    // Find all JSON files in directory
    QStringList nameFilters;
    nameFilters << "*.json";
    QStringList jsonFiles = dir.entryList(nameFilters, QDir::Files);

    if (jsonFiles.isEmpty()) {
        fmt::print("❌ ERROR: No JSON files found in directory: {}\n", inputDirectory.toStdString());
        return false;
    }

    // Convert to full paths
    QVector<QString> inputFiles;
    for (const QString& fileName : jsonFiles) {
        inputFiles.append(dir.absoluteFilePath(fileName));
    }

    fmt::print("🔍 Found {} JSON files in directory {}\n", inputFiles.size(), inputDirectory.toStdString());

    return exportMLTrainingData(inputFiles, outputFile, excludeRawData);
}

