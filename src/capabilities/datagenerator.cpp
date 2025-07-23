/*
 * DataGenerator - Mathematical equation-based data generation for SupraFit
 * Copyright (C) 2022 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 * 
 * This class provides JavaScript-based equation evaluation for generating
 * independent variable data from mathematical expressions. Enhanced with
 * random parameter generation capabilities for ML pipeline training.
 * 
 * Enhanced by Claude Code AI Assistant for improved functionality.
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

#include "src/core/models/dataclass.h"

#include <QtCore/QJsonObject>
#include <QtCore/QDateTime>
#include <QtCore/QRandomGenerator>

#include <QJSEngine>
#include <random>

// For model integration - Claude Generated
#include "src/core/models/models.h"
#include "src/core/toolset.h"

#include "datagenerator.h"

DataGenerator::DataGenerator(QObject* parent)
    : QObject{ parent }
{
    initializeRNG();
}

bool DataGenerator::Evaluate()
{
    int independent = m_data["independent"].toInt();
    int datapoints = m_data["datapoints"].toInt();
    QStringList equations = m_data["equations"].toString().split("|");
    if (equations.size() != independent)
        return false;

    DataTable* table = new DataTable(datapoints, independent, this);
    QJSEngine myEngine;
    for (int indep = 0; indep < independent; ++indep) {
        QString equation = equations[indep];
        for (int datapoint = 0; datapoint < datapoints; ++datapoint) {
            QString tmp = equation;
            myEngine.globalObject().setProperty("X", datapoint + 1);
            QJSValue value = myEngine.evaluate(tmp);
            if (value.isNumber()) {
                table->data(datapoint, indep) = value.toNumber();
            }
        }
    }
    table->setHeader(equations);
    if (m_table)
        delete m_table;
    m_table = table;
    return true;
}

// Enhanced functionality - Claude Generated
bool DataGenerator::EvaluateWithRandomParameters(const QJsonObject& randomLimits)
{
    int independent = m_data["independent"].toInt();
    int datapoints = m_data["datapoints"].toInt();
    QStringList equations = m_data["equations"].toString().split("|");
    if (equations.size() != independent)
        return false;

    DataTable* table = new DataTable(datapoints, independent, this);
    QJSEngine myEngine;
    
    // Setup random parameters in JavaScript engine
    setupRandomEngine(myEngine, randomLimits);
    
    for (int indep = 0; indep < independent; ++indep) {
        QString equation = equations[indep];
        for (int datapoint = 0; datapoint < datapoints; ++datapoint) {
            QString tmp = equation;
            myEngine.globalObject().setProperty("X", datapoint + 1);
            QJSValue value = myEngine.evaluate(tmp);
            if (value.isNumber()) {
                table->data(datapoint, indep) = value.toNumber();
            }
        }
    }
    table->setHeader(equations);
    if (m_table)
        delete m_table;
    m_table = table;
    return true;
}

void DataGenerator::setupRandomEngine(QJSEngine& engine, const QJsonObject& randomParams)
{
    std::uniform_real_distribution<double> uniformDist(0.0, 1.0);
    
    // Add static random parameters to JavaScript engine (one value per dataset)
    for (auto it = randomParams.constBegin(); it != randomParams.constEnd(); ++it) {
        const QString& paramName = it.key();
        const QJsonObject& limits = it.value().toObject();
        
        double min = limits["min"].toDouble(0.0);
        double max = limits["max"].toDouble(1.0);
        QString distribution = limits["distribution"].toString("uniform");
        
        double randomValue;
        if (distribution == "uniform") {
            randomValue = min + (max - min) * uniformDist(m_rng);
        } else if (distribution == "normal") {
            std::normal_distribution<double> normalDist((min + max) / 2.0, (max - min) / 6.0);
            randomValue = std::max(min, std::min(max, normalDist(m_rng)));
        } else {
            randomValue = min + (max - min) * uniformDist(m_rng);
        }
        
        engine.globalObject().setProperty(paramName, randomValue);
    }
    
    // Create controlled random number generator accessible from JavaScript - Claude Generated
    createControlledRandomFunctions(engine, randomParams);
}

void DataGenerator::createControlledRandomFunctions(QJSEngine& engine, const QJsonObject& randomParams)
{
    // Store parameters for use in random functions
    m_randomParams = randomParams;
    
    // Add parameter-specific random functions for per-datapoint randomization - Claude Generated
    for (auto it = randomParams.constBegin(); it != randomParams.constEnd(); ++it) {
        const QString& paramName = it.key();
        const QJsonObject& limits = it.value().toObject();
        
        double min = limits["min"].toDouble(0.0);
        double max = limits["max"].toDouble(1.0);
        QString distribution = limits["distribution"].toString("uniform");
        
        // Create a C++ random function and expose it to JavaScript
        QString functionName = paramName + "_random";
        
        // Register the function in JavaScript that calls our C++ generator
        QString jsCode = QString(
            "function %1() {"
            "  return generator.generateControlledRandom('%2');"
            "}"
        ).arg(functionName, paramName);
        
        // Make this generator accessible to JavaScript
        engine.globalObject().setProperty("generator", engine.newQObject(this));
        engine.evaluate(jsCode);
    }
    
    // Add general controlled random function
    QString generalRandomCode = 
        "function random(min, max, dist) {"
        "  return generator.generateControlledRandomValue(min, max, dist || 'uniform');"
        "}";
    engine.evaluate(generalRandomCode);
}

QJsonObject DataGenerator::generateRandomParameters(const QJsonObject& limits, quint64 seed)
{
    QJsonObject result;
    quint64 actualSeed = seed;
    if (actualSeed == 0) {
        actualSeed = QDateTime::currentMSecsSinceEpoch();
    }
    
    std::mt19937 rng(actualSeed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    for (auto it = limits.constBegin(); it != limits.constEnd(); ++it) {
        const QString& paramName = it.key();
        const QJsonObject& limitObj = it.value().toObject();
        
        double min = limitObj["min"].toDouble(0.0);
        double max = limitObj["max"].toDouble(1.0);
        double randomValue = min + (max - min) * dist(rng);
        
        QJsonObject paramObj;
        paramObj["value"] = randomValue;
        paramObj["min"] = min;
        paramObj["max"] = max;
        result[paramName] = paramObj;
    }
    
    return result;
}

double DataGenerator::generateRandomValue(double min, double max, quint64 seed)
{
    quint64 actualSeed = seed;
    if (actualSeed == 0) {
        actualSeed = QDateTime::currentMSecsSinceEpoch();
    }
    
    std::mt19937 rng(actualSeed);
    std::uniform_real_distribution<double> dist(min, max);
    
    return dist(rng);
}

// Initialize RNG with default or custom seed - Claude Generated
void DataGenerator::initializeRNG()
{
    if (m_randomSeed == 0) {
        m_randomSeed = QDateTime::currentMSecsSinceEpoch();
    }
    m_rng.seed(m_randomSeed);
    qDebug() << "🔍 DEBUG DataGenerator: Initialized RNG with seed" << m_randomSeed;
}

// Apply GlobalRandomLimits as stability constants - Claude Generated
void DataGenerator::applyGlobalRandomLimits(QSharedPointer<AbstractModel> model, const QString& globalLimits)
{
    // Parse GlobalRandomLimits format: "[3 4]" -> stability constants between 1e3 and 1e4
    QString cleaned = globalLimits;
    cleaned.remove('[').remove(']');
    QStringList minMax = cleaned.trimmed().split(' ', Qt::SkipEmptyParts);
    
    if (minMax.size() == 2) {
        double min = minMax[0].toDouble();
        double max = minMax[1].toDouble();
        
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        
        // Apply to all global parameters (stability constants)
        for (int i = 0; i < model->GlobalParameterSize(); ++i) {
            double randomValue = min + (max - min) * dist(m_rng);
            model->setGlobalParameter(randomValue, i);
            qDebug() << "🔍 DEBUG GlobalRandomLimits: Global Parameter" << i << "=" << randomValue << "(range:" << min << "-" << max << ")";
        }
        
        qDebug() << "🔍 DEBUG GlobalRandomLimits: Applied" << model->GlobalParameterSize() << "global parameters";
    }
}

// Apply LocalRandomLimits as model parameters - Claude Generated
void DataGenerator::applyLocalRandomLimits(QSharedPointer<AbstractModel> model, const QString& localLimits)
{
    // Parse LocalRandomLimits format: "[6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5]"
    QString cleaned = localLimits;
    cleaned.remove('[').remove(']');
    QStringList ranges = cleaned.split(';');
    
    QVector<qreal> parameters;
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    for (const QString& range : ranges) {
        QStringList minMax = range.trimmed().split(' ', Qt::SkipEmptyParts);
        if (minMax.size() == 2) {
            double min = minMax[0].toDouble();
            double max = minMax[1].toDouble();
            double randomValue = min + (max - min) * dist(m_rng);
            parameters.append(randomValue);
            qDebug() << "🔍 DEBUG LocalRandomLimits: Parameter" << parameters.size()-1 << "=" << randomValue << "(range:" << min << "-" << max << ")";
        }
    }
    // Apply parameters to model
    int index = 0;
    for (int i = 0; i < parameters.size() && i < model->LocalParameterSize(); ++i) {
        for (int j = 0; j < model->SeriesCount(); ++j) {
            if (model->LocalTable()->columnCount() <= i) {
                qDebug() << "🔍 DEBUG LocalRandomLimits: Skipping parameter" << i << "for series" << j << "due to insufficient columns";
                continue;
            }
            // Set local parameter for each series
            qDebug() << "🔍 DEBUG LocalRandomLimits: Setting Local Parameter" << i << "for Series" << j << "to" << parameters[index];
            model->setLocalParameter(parameters[index++], j, i);
        }
    }
    
    qDebug() << "🔍 DEBUG LocalRandomLimits: Applied" << parameters.size() << "parameters to model";
}

// Model-based generation methods - Claude Generated Integration
bool DataGenerator::EvaluateWithModel(int modelId, QPointer<DataClass> dataClass, const QJsonObject& config)
{
    if (!dataClass) {
        qDebug() << "🔍 DEBUG EvaluateWithModel: dataClass is null";
        return false;
    }
    
    qDebug() << "🔍 DEBUG EvaluateWithModel: Creating model" << modelId << "with dataClass having"
             << dataClass->IndependentModel()->rowCount() << "independent rows,"
             << dataClass->DependentModel()->rowCount() << "dependent rows,"
             << dataClass->DependentModel()->columnCount() << "dependent columns";
    
    // Create the model
    QSharedPointer<AbstractModel> model = CreateModel(modelId, dataClass);
    if (!model) {
        qDebug() << "🔍 DEBUG EvaluateWithModel: Failed to create model" << modelId;
        return false;
    }
    
    qDebug() << "🔍 DEBUG EvaluateWithModel: Successfully created model" << modelId
             << "DataPoints:" << model->DataPoints() << "SeriesCount:" << model->SeriesCount();
    
    // Set model to simulation mode to avoid error calculation
    
    // Apply randomization if specified
    if (!config.isEmpty()) {
        int series = config["Series"].toInt(1);
        applyModelRandomization(model, config, series);
        
        // Apply GlobalRandomLimits (stability constants)
        if (config.contains("GlobalRandomLimits")) {
            QString globalLimits = config["GlobalRandomLimits"].toString();
            qDebug() << "🔍 DEBUG EvaluateWithModel: Applying GlobalRandomLimits:" << globalLimits;
            applyGlobalRandomLimits(model, globalLimits);
        }
        
        // Apply LocalRandomLimits as model parameters (chemical shifts)
        if (config.contains("LocalRandomLimits")) {
            QString localLimits = config["LocalRandomLimits"].toString();
            qDebug() << "🔍 DEBUG EvaluateWithModel: Applying LocalRandomLimits:" << localLimits;
            applyLocalRandomLimits(model, localLimits);
        }
    }
    
    // Calculate the model with the set parameters
    model->Calculate();
    qDebug() << model->ExportModel();
    // Get the model table and apply noise if specified
    DataTable* modelTable = model->ModelTable();
    if (!modelTable) {
        qDebug() << "🔍 DEBUG EvaluateWithModel: ModelTable() returned null";
        return false;
    }
    
    qDebug() << "🔍 DEBUG EvaluateWithModel: ModelTable has" << modelTable->rowCount() 
             << "rows," << modelTable->columnCount() << "columns";
    
    // Apply noise if variance is specified using PrepareMC
    if (config.contains("Variance")) {
        double variance = config["Variance"].toDouble();
        if (variance > 0) {
            // Create variance vector for all columns
            QVector<double> stddevVector(modelTable->columnCount(), variance);
            modelTable = modelTable->PrepareMC(stddevVector, m_rng);
            
            qDebug() << "🔍 DEBUG EvaluateWithModel: Applied noise with variance" << variance << "to" << modelTable->columnCount() << "columns";
        }
    }
    
    // Store the result
    if (m_table) {
        delete m_table;
    }
    m_table = new DataTable(modelTable);
    
    // Enhance content with model parameters and configuration - Claude Generated
    QString originalContent = dataClass->getContent();
    QString enhancedContent = createEnhancedContent(dataClass, originalContent, config, model);
    dataClass->setContent(enhancedContent);
    
    return true;
}

bool DataGenerator::GenerateModelBasedData(int modelId, QPointer<DataClass> inputData, const QJsonObject& modelConfig)
{
    if (!inputData) {
        return false;
    }
    
    // Create a copy of the input data
    QPointer<DataClass> workingData = new DataClass(inputData.data());
    
    // Use the working data for model generation
    return EvaluateWithModel(modelId, workingData, modelConfig);
}

bool DataGenerator::applyModelRandomization(QSharedPointer<AbstractModel> model, const QJsonObject& config, int series)
{
    if (!model) {
        return false;
    }
    
    // Apply GlobalRandomLimits
    if (config.contains("GlobalRandomLimits")) {
        QString global_limits = config["GlobalRandomLimits"].toString();
        QStringList limits = global_limits.split(";");
        if (limits.size()) {
            QVector<QPair<qreal, qreal>> global_block;
            
            if (limits.size() == model->GlobalParameterSize()) {
                for (int j = 0; j < limits.size(); ++j) {
                    global_block << ToolSet::QString2QPair(limits[j]);
                }
            } else {
                const QPair<qreal, qreal> pair = ToolSet::QString2QPair(limits.first());
                global_block = QVector<QPair<qreal, qreal>>(model->GlobalParameterSize(), pair);
            }
            
            model->setGlobalRandom(global_block);
        }
    }
    
    // Apply LocalRandomLimits
    if (config.contains("LocalRandomLimits")) {
        QString local_limits = config["LocalRandomLimits"].toString();
        QStringList local_limits_block = local_limits.split(":");
        
        if (local_limits_block.size()) {
            if (local_limits_block.size() == series) {
                for (int j = 0; j < series; ++j) {
                    QStringList limits = local_limits_block[j].split(";");
                    if (limits.size()) {
                        QVector<QPair<qreal, qreal>> local_block;
                        
                        if (limits.size() == model->LocalParameterSize()) {
                            for (int k = 0; k < limits.size(); ++k) {
                                local_block << ToolSet::QString2QPair(limits[k]);
                            }
                        } else {
                            const QPair<qreal, qreal> pair = ToolSet::QString2QPair(limits.first());
                            local_block = QVector<QPair<qreal, qreal>>(model->LocalParameterSize(), pair);
                        }
                        
                        model->setLocalRandom(local_block, j);
                    }
                }
            } else {
                QStringList limits = local_limits_block.first().split(";");
                const QPair<qreal, qreal> pair = ToolSet::QString2QPair(limits.first());
                
                for (int j = 0; j < series; ++j) {
                    model->setLocalRandom(QVector<QPair<qreal, qreal>>(model->LocalParameterSize(), pair), j);
                }
            }
        }
    }
    
    return true;
}

// Create enhanced content with model parameters and configuration - Claude Generated
QString DataGenerator::createEnhancedContent(QPointer<DataClass> dataClass, const QString& originalContent, const QJsonObject& config, QSharedPointer<AbstractModel> model)
{
    QString content = originalContent;
    if (content.isEmpty()) {
        content = "Generated with DataGenerator version " + QString::number(m_generator_version);
    }
    
    // Add model information
    if (model) {
        content += QString("\nModel: %1 (%2)")
                   .arg(model->Name())
                   .arg(model->GlobalParameterSize() + model->LocalParameterSize() + " parameters");
        
        // Add global parameters (stability constants)
        if (model->GlobalParameterSize() > 0) {
            content += "\nGlobal Parameters (Stability Constants):";
            for (int i = 0; i < model->GlobalParameterSize(); ++i) {
                double value = model->GlobalParameter(i);
                content += QString("\n  K%1 = %2 (log K = %3)")
                           .arg(i + 1)
                           .arg(std::pow(10, value), 0, 'e', 2)
                           .arg(value, 0, 'f', 2);
            }
        }
        
        // Add local parameters (chemical shifts)  
        if (model->LocalParameterSize() > 0) {
            content += "\nLocal Parameters (Chemical Shifts):";
            for (int i = 0; i < model->LocalParameterSize(); ++i) {
                for (int j = 0; j < model->SeriesCount(); ++j) {
                    double value = model->LocalParameter(i, j);
                    content += QString("\n  δ%1_series%2 = %3 ppm")
                               .arg(i + 1)
                               .arg(j + 1)
                               .arg(value, 0, 'f', 3);
                }
            }
        }
    }
    
    // Add configuration information
    if (!config.isEmpty()) {
        content += "\n\nConfiguration Parameters:";
        if (config.contains("DataPoints")) {
            content += QString("\n  DataPoints: %1").arg(config["DataPoints"].toInt());
        }
        if (config.contains("Series")) {
            content += QString("\n  Series: %1").arg(config["Series"].toInt());
        }
        if (config.contains("Variance")) {
            content += QString("\n  Noise Variance: %1").arg(config["Variance"].toDouble(), 0, 'e', 2);
        }
        if (config.contains("GlobalRandomLimits")) {
            content += QString("\n  Global Limits: %1").arg(config["GlobalRandomLimits"].toString());
        }
        if (config.contains("LocalRandomLimits")) {
            content += QString("\n  Local Limits: %1").arg(config["LocalRandomLimits"].toString());
        }
        
        // TODO: Add input JSON block from generator configuration
        // This should include the original JSON configuration that was used to generate this data
        // Format: "\nInput Configuration:\n" + QJsonDocument(config).toJson(QJsonDocument::Indented)
    }
    
    return content;
}

// Qt slots for JavaScript integration - Claude Generated
double DataGenerator::generateControlledRandom(const QString& parameterName)
{
    if (!m_randomParams.contains(parameterName)) {
        qDebug() << "🔍 DEBUG generateControlledRandom: Parameter" << parameterName << "not found in random parameters";
        return 0.0;
    }
    
    const QJsonObject& limits = m_randomParams[parameterName].toObject();
    double min = limits["min"].toDouble(0.0);
    double max = limits["max"].toDouble(1.0);
    QString distribution = limits["distribution"].toString("uniform");
    
    return generateControlledRandomValue(min, max, distribution);
}

double DataGenerator::generateControlledRandomValue(double min, double max, const QString& distribution)
{
    std::uniform_real_distribution<double> uniformDist(0.0, 1.0);
    
    if (distribution == "uniform") {
        return min + (max - min) * uniformDist(m_rng);
    } else if (distribution == "normal") {
        // For normal distribution, use mean = (min+max)/2, std = (max-min)/6 (3-sigma rule)
        std::normal_distribution<double> normalDist((min + max) / 2.0, (max - min) / 6.0);
        double value = normalDist(m_rng);
        // Clamp to bounds
        return std::max(min, std::min(max, value));
    } else {
        // Default to uniform if unknown distribution
        qDebug() << "🔍 DEBUG generateControlledRandomValue: Unknown distribution" << distribution << ", using uniform";
        return min + (max - min) * uniformDist(m_rng);
    }
}
