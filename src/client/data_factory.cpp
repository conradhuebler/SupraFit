/*
 * SupraFit CLI — DataFactory: data-table generation / loading helpers
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 *
 */

/* Claude Generated — verbatim move of the stateless data-table builders out of
 * SupraFitCli (D3): validateDataGeneratorConfig / generateIndependentDataTable /
 * loadDataTableFromFile / applyNoise. Behaviour-identical; only the owning class changed. */

#include "src/global.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QRandomGenerator>

#include "src/capabilities/datagenerator.h"
#include "src/core/filehandler.h"
#include "src/core/jsonhandler.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/toolset.h"

#include <fmt/core.h>

#include "data_factory.h"

bool DataFactory::validateDataGeneratorConfig(const QJsonObject& config)
{
    // Check required fields
    if (!config.contains("Equations") || config["Equations"].toString().isEmpty()) {
        fmt::print("Error: Missing or empty 'Equations' field\n");
        return false;
    }
    
    if (!config.contains("IndependentVariables")) {
        fmt::print("Error: Missing 'IndependentVariables' field\n");
        return false;
    }
    
    int independentVars = config["IndependentVariables"].toInt();
    if (independentVars <= 0) {
        fmt::print("Error: IndependentVariables must be > 0\n");
        return false;
    }
    
    QString equations = config["Equations"].toString();
    QStringList equationList = equations.split("|");
    if (equationList.size() != independentVars) {
        fmt::print("Error: Number of equations ({}) doesn't match IndependentVariables ({})\n", 
                  equationList.size(), independentVars);
        return false;
    }
    
    int dataPoints = config["DataPoints"].toInt(20);
    if (dataPoints <= 0) {
        fmt::print("Error: DataPoints must be > 0\n");
        return false;
    }
    
    return true;
}


QJsonObject DataFactory::generateIndependentDataTable(const QJsonObject& independentConfig)
{
    fmt::print("🔧 Generating independent data table...\n");
    
    QString source = independentConfig["Source"].toString();
    
    if (source == "generator") {
        QJsonObject generatorConfig = independentConfig["Generator"].toObject();
        QString type = generatorConfig["Type"].toString();
        
        if (type == "equations") {
            // Generate using DataGenerator equations with careful memory management
            fmt::print("🔧 Setting up DataGenerator for independent data...\n");
            
            // Setup generator configuration
            QJsonObject genData;
            genData["independent"] = generatorConfig["Variables"].toInt(1);
            genData["datapoints"] = generatorConfig["DataPoints"].toInt(20);
            genData["equations"] = generatorConfig["Equations"].toString("X");
            
            SFDebugPrint("🔍 DEBUG: Independent config - DataPoints: {}, Variables: {}, Equations: {}\n", 
                      genData["datapoints"].toInt(), genData["independent"].toInt(), genData["equations"].toString().toStdString());
            
            // Create generator with explicit parent to manage memory
            DataGenerator* generator = new DataGenerator(nullptr);
            
            // Set configuration
            generator->setJson(genData);
            
            // Apply random parameters if specified
            QJsonObject randomParams = generatorConfig["RandomParameters"].toObject();
            bool success = false;
            
            SFDebugPrint("🔍 DEBUG: Random parameters empty: {}\n", randomParams.isEmpty());
            
            if (!randomParams.isEmpty()) {
                success = generator->EvaluateWithRandomParameters(randomParams);
            } else {
                success = generator->Evaluate();
            }
            
            if (!success) {
                fmt::print("❌ ERROR: DataGenerator failed for independent data\n");
                delete generator;
                return QJsonObject();
            }
            
            // Check if generator table is valid
            if (!generator->Table()) {
                fmt::print("❌ ERROR: Generator table is null\n");
                delete generator;
                return QJsonObject();
            }
            
            // Export table as JSON immediately to avoid pointer issues
            QJsonObject tableJson = generator->Table()->ExportTable(false);
            
            fmt::print("✅ Generated independent data: {} rows x {} cols (via clean pointer management)\n", 
                      generator->Table()->rowCount(), generator->Table()->columnCount());
            
            // Clean up generator immediately after use
            delete generator;
            generator = nullptr;
            
            SFDebugPrint("🔍 DEBUG: DataGenerator cleaned up successfully\n");
            
            return tableJson;
            
        } else if (type == "model") {
            fmt::print("❌ ERROR: Model-based independent generation not yet implemented\n");
            return QJsonObject();
        }
        
    } else if (source == "file") {
        QJsonObject fileConfig = independentConfig["File"].toObject();
        return loadDataTableFromFile(fileConfig);
    }
    
    fmt::print("❌ ERROR: Unknown independent data source: {}\n", source.toStdString());
    return QJsonObject();
}


QJsonObject DataFactory::loadDataTableFromFile(const QJsonObject& fileConfig)
{
    fmt::print("📁 Loading data table from file...\n");
    
    QString path = fileConfig["Path"].toString();
    int startRow = fileConfig["StartRow"].toInt(0);
    int startCol = fileConfig["StartCol"].toInt(0);
    int rows = fileConfig["Rows"].toInt(-1);
    int cols = fileConfig["Cols"].toInt(-1);
    
    if (path.isEmpty()) {
        fmt::print("❌ ERROR: No file path specified\n");
        return QJsonObject();
    }
    
    fmt::print("📂 Loading file: {}\n", path.toStdString());
    fmt::print("🔍 Range: StartRow={}, StartCol={}, Rows={}, Cols={}\n", 
              startRow, startCol, rows, cols);
    
    // Create FileHandler and load the file - Claude Generated
    FileHandler* handler = new FileHandler(path, nullptr);
    handler->LoadFile();
    
    if (!handler->FileSupported()) {
        fmt::print("❌ ERROR: File format not supported: {}\n", path.toStdString());
        delete handler;
        return QJsonObject();
    }
    
    if (!handler->getData()) {
        fmt::print("❌ ERROR: No data could be loaded from file: {}\n", path.toStdString());
        delete handler;
        return QJsonObject();
    }
    
    fmt::print("✅ File loaded successfully: {} rows x {} cols\n", 
              handler->getData()->rowCount(), handler->getData()->columnCount());
    
    // Calculate end positions based on rows/cols parameters
    int endRow = (rows > 0) ? startRow + rows - 1 : -1;
    int endCol = (cols > 0) ? startCol + cols - 1 : -1;
    
    // Extract the requested range using FileHandler's new functionality
    QPointer<DataTable> rangeTable = handler->getDataRange(startRow, endRow, startCol, endCol);
    
    if (!rangeTable) {
        fmt::print("❌ ERROR: Failed to extract requested data range\n");
        delete handler;
        return QJsonObject();
    }

    fmt::print("✅ Extracted data range: {} rows x {} cols\n", 
              rangeTable->rowCount(), rangeTable->columnCount());
    
    // Export the range as JSON
    QJsonObject tableJson = rangeTable->ExportTable(false);
    
    // Clean up
    delete handler;
    
    fmt::print("✅ Data table converted to JSON successfully\n");
    
    return tableJson;
}


QPointer<DataClass> DataFactory::applyNoise(QPointer<DataClass> data, const QJsonObject& noiseConfig, bool isIndependent)
{
    if (noiseConfig.isEmpty() || noiseConfig["Type"].toString() == "none") {
        return data;  // No noise to apply
    }
    
    QString type = noiseConfig["Type"].toString();
    fmt::print("🔊 Applying {} noise to {} data...\n", type.toStdString(), 
              isIndependent ? "independent" : "dependent");
    
    if (type == "exportMC" || type == "montecarlo" || type == "gaussian") {
        // All noise types use DataTable's PrepareMC functionality - Claude Generated
        
        // Get noise parameters with reasonable defaults
        double stddev = noiseConfig["StdDev"].toDouble(0.05);  // Default 5% noise
        int seed = noiseConfig["Seed"].toInt(42);
        QVector<int> columns;  // Empty means all columns
        
        if (noiseConfig.contains("Columns")) {
            QJsonArray colArray = noiseConfig["Columns"].toArray();
            for (const auto& col : colArray) {
                columns.append(col.toInt());
            }
        }
        
        SFDebugPrint("🔍 DEBUG: Noise - Type: {}, StdDev: {}, Seed: {}, Columns: {}\n", 
                  type.toStdString(), stddev, seed, columns.isEmpty() ? "all" : QString::number(columns.size()).toStdString());
        
        // Setup random number generator and distribution
        std::mt19937 rng(seed);
        QVector<double> stddevVector;
        
        // Apply noise to appropriate table
        DataTable* targetTable = isIndependent ? data->IndependentModel() : data->DependentModel();
        if (!targetTable) {
            fmt::print("❌ ERROR: Target table is null\n");
            return data;
        }
        
        // Create stddev vector for each column
        for (int col = 0; col < targetTable->columnCount(); ++col) {
            stddevVector.append(stddev);
        }
        
        // Generate noisy table using PrepareMC
        QPointer<DataTable> noisyTable = targetTable->PrepareMC(stddevVector, rng, columns);
        
        if (!noisyTable) {
            fmt::print("❌ ERROR: Failed to generate noisy table\n");
            return data;
        }
        
        // Replace the table in DataClass
        if (isIndependent) {
            data->setIndependentTable(noisyTable);
            data->setIndependentRawTable(new DataTable(noisyTable));
        } else {
            data->setDependentTable(noisyTable);
            data->setDependentRawTable(new DataTable(noisyTable));
        }
        
        fmt::print("✅ Applied {} noise: {} rows x {} cols\n", 
                  type.toStdString(), noisyTable->rowCount(), noisyTable->columnCount());
        
    } else {
        fmt::print("❌ ERROR: Unknown noise type: {}\n", type.toStdString());
        fmt::print("   Supported types: none, exportMC, montecarlo, gaussian\n");
    }
    
    return data;
}

// Execute post-fit statistical analysis using JobManager - Claude Generated

