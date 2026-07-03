/*
 * SupraFit CLI - Model statistics reporting (console tables)
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Extracted verbatim from suprafit_cli.cpp (CLI decomposition, TECHNICAL_DEBT.md D3).
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

#include "analysis_reporter.h"

#include "src/core/analyse.h"
#include "src/core/projectmanager.h"
#include "src/core/toolset.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include <cmath>
#include <fmt/core.h>
#include <limits>

AnalysisReporter::ModelStatistics AnalysisReporter::extractModelStatistics(const QString& key, const QJsonObject& modelObj)
{
    ModelStatistics stats;
    stats.key = key;
    stats.hasValidStats = false;
    
    // Extract model name - try different possible field names
    stats.name = modelObj["name"].toString();
    if (stats.name.isEmpty()) {
        stats.name = modelObj["model"].toString();  // Alternative field name
    }
    if (stats.name.isEmpty()) {
        stats.name = key;  // Use key as fallback
    }
    
    // Check convergence status
    bool converged = modelObj["converged"].toBool(false);
    stats.status = converged ? "✅ Conv" : "❌ Failed";
    
    // Extract fit statistics - SupraFit specific structure
    stats.sse = modelObj["SSE"].toDouble(-1);
    stats.sae = modelObj["SAE"].toDouble(-1);  // Sum of Absolute Errors
    stats.aic = modelObj["AIC"].toDouble(-999);  // Akaike Information Criterion
    stats.aicc = modelObj["AICc"].toDouble(-999); // Corrected AIC
    
    // Try to get parameter counts from data structure
    QJsonObject data = modelObj["data"].toObject();
    stats.globalParams = -1;
    stats.localParams = -1;
    
    if (!data.isEmpty()) {
        QJsonObject globalParamObj = data["globalParameter"].toObject();
        if (!globalParamObj.isEmpty()) {
            stats.globalParams = globalParamObj["rows"].toInt(-1);
        }
        
        QJsonObject localParamObj = data["localParameter"].toObject();
        if (!localParamObj.isEmpty()) {
            stats.localParams = localParamObj["rows"].toInt(-1);
        }
    }
    
    // Analyze post-processing methods - Claude Generated
    if (!data.isEmpty() && data.contains("methods")) {
        QJsonObject methods = data["methods"].toObject();
        QVector<QJsonObject> modelVector = {modelObj};
        
        for (auto it = methods.begin(); it != methods.end(); ++it) {
            QJsonObject method = it.value().toObject();
            
            // Skip empty method blocks
            if (method.isEmpty())
                continue;
            
            // Look for controller - it might be directly in method or nested under numeric keys
            QJsonObject controller;
            bool foundController = false;
            
            if (method.contains("controller")) {
                controller = method["controller"].toObject();
                foundController = true;
            } else {
                // Check for nested controller under numeric keys (like "0", "1", etc.)
                for (auto subIt = method.begin(); subIt != method.end(); ++subIt) {
                    if (subIt.value().isObject()) {
                        QJsonObject subMethod = subIt.value().toObject();
                        if (subMethod.contains("controller")) {
                            controller = subMethod["controller"].toObject();
                            foundController = true;
                            break;
                        }
                    }
                }
            }
            
            if (!foundController || !controller.contains("Method"))
                continue;
                
            int methodType = controller["Method"].toInt();
            
            switch (methodType) {
                case 1: // MonteCarlo
                    stats.mcBlocks++;
                    if (!stats.postProcessingData.contains("MonteCarlo") || stats.postProcessingData["MonteCarlo"].isNull()) {
                        stats.postProcessingData["MonteCarlo"] = StatisticTool::CalculateMCMetrics(modelVector, 1);
                    }
                    break;
                case 2: // WeakenedGridSearch
                    stats.wgsBlocks++;
                    if (!stats.postProcessingData.contains("WeakenedGridSearch") || stats.postProcessingData["WeakenedGridSearch"].isNull()) {
                        stats.postProcessingData["WeakenedGridSearch"] = StatisticTool::CalculateWGSMetrics(modelVector);
                    }
                    break;
                case 3: // ModelComparison
                    stats.modelCompBlocks++;
                    if (!stats.postProcessingData.contains("ModelComparison") || stats.postProcessingData["ModelComparison"].isNull()) {
                        stats.postProcessingData["ModelComparison"] = StatisticTool::CalculateModelComparisonMetrics(modelVector);
                    }
                    break;
                case 4: // CrossValidation
                    stats.cvBlocks++;
                    if (!stats.postProcessingData.contains("CrossValidation") || stats.postProcessingData["CrossValidation"].isNull()) {
                        stats.postProcessingData["CrossValidation"] = StatisticTool::CalculateCVMetrics(modelVector, 1, 3);
                    }
                    break;
                case 5: // Reduction
                    stats.reductionBlocks++;
                    if (!stats.postProcessingData.contains("Reduction") || stats.postProcessingData["Reduction"].isNull()) {
                        stats.postProcessingData["Reduction"] = StatisticTool::CalculateReductionMetrics(modelVector, 0.1);
                    }
                    break;
                case 6: // FastConfidence
                    stats.fastConfBlocks++;
                    if (!stats.postProcessingData.contains("FastConfidence") || stats.postProcessingData["FastConfidence"].isNull()) {
                        stats.postProcessingData["FastConfidence"] = StatisticTool::CalculateFastConfidenceMetrics(modelVector);
                    }
                    break;
                case 7: // GlobalSearch
                    stats.globalBlocks++;
                    if (!stats.postProcessingData.contains("GlobalSearch") || stats.postProcessingData["GlobalSearch"].isNull()) {
                        stats.postProcessingData["GlobalSearch"] = StatisticTool::CalculateGlobalSearchMetrics(modelVector);
                    }
                    break;
            }
        }
        
        stats.totalPPBlocks = stats.mcBlocks + stats.wgsBlocks + stats.modelCompBlocks + 
                             stats.cvBlocks + stats.reductionBlocks + stats.fastConfBlocks + stats.globalBlocks;
    }
    
    // Check if we have any valid statistics
    stats.hasValidStats = (stats.sse >= 0 || stats.sae >= 0 || stats.aic > -999 || 
                          stats.aicc > -999 || stats.globalParams > 0 || stats.localParams > 0);
    
    return stats;
}

void AnalysisReporter::displayModelStatisticsTable(const QVector<ModelStatistics>& models)
{
    if (models.isEmpty()) {
        fmt::print("   No fitted models found in this file\n");
        return;
    }
    
    // Calculate column widths based on content
    int keyWidth = 8;  // "Model" header minimum
    int nameWidth = 10; // "Name" header minimum
    int statusWidth = 8; // "Status" header minimum
    int sseWidth = 10;  // "SSE" header + scientific notation
    int saeWidth = 8;   // "SAE" header minimum
    int aicWidth = 8;   // "AIC" header minimum
    int aiccWidth = 8;  // "AICc" header minimum
    int globalWidth = 8; // "Global" header minimum
    int localWidth = 7;  // "Local" header minimum
    int ppWidth = 12;   // "Post-Proc" header minimum
    
    // Calculate actual widths needed
    for (const auto& model : models) {
        keyWidth = qMax(keyWidth, model.key.length());
        nameWidth = qMax(nameWidth, model.name.length());
        statusWidth = qMax(statusWidth, model.status.length());
        if (model.sse >= 0) sseWidth = qMax(sseWidth, 10); // Scientific notation: 1.23e-05
        if (model.sae >= 0) saeWidth = qMax(saeWidth, 8);
        if (model.aic > -999) aicWidth = qMax(aicWidth, 8);
        if (model.aicc > -999) aiccWidth = qMax(aiccWidth, 8);
        if (model.globalParams > 0) globalWidth = qMax(globalWidth, QString::number(model.globalParams).length());
        if (model.localParams > 0) localWidth = qMax(localWidth, QString::number(model.localParams).length());
        
        // Calculate post-processing summary width
        QString ppSummary;
        if (model.totalPPBlocks > 0) {
            QStringList ppMethods;
            if (model.mcBlocks > 0) ppMethods << QString("%1 MC").arg(model.mcBlocks);
            if (model.cvBlocks > 0) ppMethods << QString("%1 CV").arg(model.cvBlocks);
            if (model.wgsBlocks > 0) ppMethods << QString("%1 WGS").arg(model.wgsBlocks);
            if (model.modelCompBlocks > 0) ppMethods << QString("%1 MCo").arg(model.modelCompBlocks);
            if (model.reductionBlocks > 0) ppMethods << QString("%1 Red").arg(model.reductionBlocks);
            if (model.fastConfBlocks > 0) ppMethods << QString("%1 FCo").arg(model.fastConfBlocks);
            if (model.globalBlocks > 0) ppMethods << QString("%1 Glo").arg(model.globalBlocks);
            ppSummary = ppMethods.join(", ");
        } else {
            ppSummary = "-";
        }
        ppWidth = qMax(ppWidth, ppSummary.length());
    }
    
    // Print table header
    std::string separator = "+";
    separator += std::string(keyWidth + 2, '-') + "+";
    separator += std::string(nameWidth + 2, '-') + "+";
    separator += std::string(statusWidth + 2, '-') + "+";
    separator += std::string(sseWidth + 2, '-') + "+";
    separator += std::string(saeWidth + 2, '-') + "+";
    separator += std::string(aicWidth + 2, '-') + "+";
    separator += std::string(aiccWidth + 2, '-') + "+";
    separator += std::string(globalWidth + 2, '-') + "+";
    separator += std::string(localWidth + 2, '-') + "+";
    separator += std::string(ppWidth + 2, '-') + "+";
    
    fmt::print("{}\n", separator);
    fmt::print("| {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} |\n",
               "Model", keyWidth,
               "Name", nameWidth,
               "Status", statusWidth,
               "SSE", sseWidth,
               "SAE", saeWidth,
               "AIC", aicWidth,
               "AICc", aiccWidth,
               "Global", globalWidth,
               "Local", localWidth,
               "Post-Proc", ppWidth);
    fmt::print("{}\n", separator);
    
    // Print data rows
    for (const auto& model : models) {
        std::string sseStr = model.sse >= 0 ? fmt::format("{:.2e}", model.sse) : "N/A";
        std::string saeStr = model.sae >= 0 ? fmt::format("{:.4f}", model.sae) : "N/A";
        std::string aicStr = model.aic > -999 ? fmt::format("{:.2f}", model.aic) : "N/A";
        std::string aiccStr = model.aicc > -999 ? fmt::format("{:.2f}", model.aicc) : "N/A";
        std::string globalStr = model.globalParams > 0 ? std::to_string(model.globalParams) : "N/A";
        std::string localStr = model.localParams > 0 ? std::to_string(model.localParams) : "N/A";
        
        // Format post-processing summary
        QString ppSummary;
        if (model.totalPPBlocks > 0) {
            QStringList ppMethods;
            if (model.mcBlocks > 0) ppMethods << QString("%1 MC").arg(model.mcBlocks);
            if (model.cvBlocks > 0) ppMethods << QString("%1 CV").arg(model.cvBlocks);
            if (model.wgsBlocks > 0) ppMethods << QString("%1 WGS").arg(model.wgsBlocks);
            if (model.modelCompBlocks > 0) ppMethods << QString("%1 MCo").arg(model.modelCompBlocks);
            if (model.reductionBlocks > 0) ppMethods << QString("%1 Red").arg(model.reductionBlocks);
            if (model.fastConfBlocks > 0) ppMethods << QString("%1 FCo").arg(model.fastConfBlocks);
            if (model.globalBlocks > 0) ppMethods << QString("%1 Glo").arg(model.globalBlocks);
            ppSummary = ppMethods.join(", ");
        } else {
            ppSummary = "-";
        }
        
        fmt::print("| {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} | {:^{}} |\n",
                   model.key.toStdString(), keyWidth,
                   model.name.toStdString(), nameWidth,
                   model.status.toStdString(), statusWidth,
                   sseStr, sseWidth,
                   saeStr, saeWidth,
                   aicStr, aicWidth,
                   aiccStr, aiccWidth,
                   globalStr, globalWidth,
                   localStr, localWidth,
                   ppSummary.toStdString(), ppWidth);
    }
    fmt::print("{}\n", separator);
}

// Helper function to display post-processing results using Print::TextFromConfidence - Claude Generated
void AnalysisReporter::displayPostProcessingMethod(const QString& methodName, const QString& emoji, 
                                              int blockCount, const QJsonObject& methodData, 
                                              int methodType) {
    fmt::print("  {} {} ({} block{}):\n", emoji.toStdString(), methodName.toStdString(), 
              blockCount, blockCount == 1 ? "" : "s");
    
    // Create controller JSON for Print::TextFromConfidence
    QJsonObject controller;
    controller["Method"] = methodType;
    controller["EntropyBins"] = 30;
    
    // Method-specific controller settings
    if (methodData.contains("variance")) {
        controller["Variance"] = methodData["variance"].toDouble();
        controller["VarianceSource"] = 1; // Standard variance source
    }
    if (methodData.contains("cv_type")) {
        controller["CVType"] = methodData["cv_type"].toInt();
    }
    if (methodData.contains("cv_x")) {
        controller["X"] = methodData["cv_x"].toInt();
    }
    
    // Process parameters for all methods that have model/parameter structure
    if (methodData.contains("models") && methodData["models"].isArray()) {
        QJsonArray models_array = methodData["models"].toArray();
        for (const auto& modelValue : models_array) {
            QJsonObject modelData = modelValue.toObject();
            if (modelData.contains("parameters") && modelData["parameters"].isArray()) {
                QJsonArray params = modelData["parameters"].toArray();
                for (const auto& paramValue : params) {
                    QJsonObject param = paramValue.toObject();
                    
                    // Convert our JSON structure to the format expected by Print::TextFromConfidence
                    QJsonObject result;
                    result["name"] = param["name"];
                    result["type"] = param["type"];
                    result["value"] = param.contains("mean") ? param["mean"] : param["value"];
                    
                    // Create confidence object
                    QJsonObject confidence;
                    confidence["upper"] = param.contains("confidence_upper") ? 
                                        param["confidence_upper"] : param["upper"];
                    confidence["lower"] = param.contains("confidence_lower") ? 
                                        param["confidence_lower"] : param["lower"];
                    confidence["error"] = param.contains("confidence_error") ? 
                                        param["confidence_error"].toDouble() : 95.0;
                    result["confidence"] = confidence;
                    
                    // Create data object with raw data
                    QJsonObject data;
                    if (param.contains("raw_data")) {
                        data["raw"] = param["raw_data"];
                    } else {
                        data["raw"] = "";
                    }
                    result["data"] = data;
                    
                    // Add index for local parameters
                    if (param.contains("index")) {
                        result["index"] = param["index"];
                    }
                    
                    // Use Print::TextFromConfidence and convert HTML to raw text
                    QString htmlOutput = Print::TextFromConfidence(result, controller);
                    QString consoleOutput = Print::Html2Raw(htmlOutput);
                    fmt::print("{}\n", consoleOutput.toStdString());
                }
            }
        }
    }
    
    // Method-specific summary information
    if (methodData.contains("parameter_averages") && methodData["parameter_averages"].isArray()) {
        QJsonArray avgParams = methodData["parameter_averages"].toArray();
        fmt::print("    📊 Parameter Summary:\n");
        for (const auto& avgValue : avgParams) {
            QJsonObject avg = avgValue.toObject();
            fmt::print("       {}: Avg σ={:.4f}, Avg H={:.2f} ({} model{})\n",
                     avg["parameter"].toString().toStdString(),
                     avg["avg_stddev"].toDouble(),
                     avg["avg_entropy"].toDouble(),
                     avg["model_count"].toInt(),
                     avg["model_count"].toInt() == 1 ? "" : "s");
        }
        fmt::print("\n");
    }
}

// Post-processing details display function - Claude Generated  
void AnalysisReporter::displayPostProcessingDetails(const QVector<ModelStatistics>& models)
{
    if (models.isEmpty()) {
        fmt::print("   No models found for post-processing details\n");
        return;
    }
    
    fmt::print("\n📊 POST-PROCESSING DETAILS:\n");
    
    for (const auto& model : models) {
        if (model.totalPPBlocks == 0) {
            fmt::print("\n{} ({}):\n", model.key.toStdString(), model.name.toStdString());
            fmt::print("  No post-processing methods found\n");
            continue;
        }
        
        fmt::print("\n{} ({}):\n", model.key.toStdString(), model.name.toStdString());
        
        // Display Monte Carlo results
        if (model.mcBlocks > 0 && !model.postProcessingData["MonteCarlo"].isUndefined()) {
            displayPostProcessingMethod("Monte Carlo Analysis", "🎲", model.mcBlocks, 
                                      model.postProcessingData["MonteCarlo"].toObject(), 1);
        }
        
        // Display Cross-Validation results
        if (model.cvBlocks > 0 && !model.postProcessingData["CrossValidation"].isUndefined()) {
            displayPostProcessingMethod("Cross-Validation", "🔄", model.cvBlocks, 
                                      model.postProcessingData["CrossValidation"].toObject(), 4);
        }
        
        // Display Parameter Reduction results
        if (model.reductionBlocks > 0 && !model.postProcessingData["Reduction"].isUndefined()) {
            displayPostProcessingMethod("Parameter Reduction", "🔬", model.reductionBlocks, 
                                      model.postProcessingData["Reduction"].toObject(), 5);
        }
        
        // Display Weakened Grid Search results  
        if (model.wgsBlocks > 0 && !model.postProcessingData["WeakenedGridSearch"].isUndefined()) {
            displayPostProcessingMethod("Weakened Grid Search", "🔍", model.wgsBlocks, 
                                      model.postProcessingData["WeakenedGridSearch"].toObject(), 2);
        }
        
        // Display Model Comparison results
        if (model.modelCompBlocks > 0 && !model.postProcessingData["ModelComparison"].isUndefined()) {
            displayPostProcessingMethod("Model Comparison", "⚖️", model.modelCompBlocks, 
                                      model.postProcessingData["ModelComparison"].toObject(), 3);
        }
        
        // Display Fast Confidence results
        if (model.fastConfBlocks > 0 && !model.postProcessingData["FastConfidence"].isUndefined()) {
            displayPostProcessingMethod("Fast Confidence", "⚡", model.fastConfBlocks, 
                                      model.postProcessingData["FastConfidence"].toObject(), 6);
        }
        
        // Display Global Search results
        if (model.globalBlocks > 0 && !model.postProcessingData["GlobalSearch"].isUndefined()) {
            displayPostProcessingMethod("Global Search", "🌐", model.globalBlocks, 
                                      model.postProcessingData["GlobalSearch"].toObject(), 7);
        }
    }
}


// Enhanced DataGenerator configuration analysis - Claude Generated
void AnalysisReporter::analyzeGenerateDataConfig(const QJsonObject& generateDataConfig)
{
    fmt::print("         📊 GENERATE DATA CONFIGURATION:\n");

    if (generateDataConfig.contains("UseDataGenerator") && generateDataConfig["UseDataGenerator"].toBool()) {
        fmt::print("            🔧 DataGenerator Mode: ENABLED\n");

        // Analyze independent variables
        if (generateDataConfig.contains("IndependentVariables")) {
            int indepVars = generateDataConfig["IndependentVariables"].toInt();
            fmt::print("            📈 Independent Variables: {}\n", indepVars);
        }

        // Analyze data points
        if (generateDataConfig.contains("DataPoints")) {
            int dataPoints = generateDataConfig["DataPoints"].toInt();
            fmt::print("            📊 Data Points: {}\n", dataPoints);
        }

        // Analyze equations
        if (generateDataConfig.contains("Equations")) {
            QString equations = generateDataConfig["Equations"].toString();
            QStringList equationList = equations.split("|");
            fmt::print("            🧮 Equations ({} total):\n", equationList.size());
            for (int i = 0; i < equationList.size(); ++i) {
                fmt::print("               X{}: {}\n", i + 1, equationList[i].toStdString());
            }
        }

        // Analyze dependent equations
        if (generateDataConfig.contains("DependentEquations")) {
            QString depEquations = generateDataConfig["DependentEquations"].toString();
            QStringList depEquationList = depEquations.split("|");
            fmt::print("            🎯 Dependent Equations ({} total):\n", depEquationList.size());
            for (int i = 0; i < depEquationList.size(); ++i) {
                fmt::print("               Y{}: {}\n", i + 1, depEquationList[i].toStdString());
            }
        }

        // Analyze series and model
        if (generateDataConfig.contains("Series")) {
            int series = generateDataConfig["Series"].toInt();
            fmt::print("            📈 Series Count: {}\n", series);
        }

        if (generateDataConfig.contains("Model")) {
            int model = generateDataConfig["Model"].toInt();
            fmt::print("            🔬 Model ID: {}\n", model);
        }

        // Analyze repetition and variance
        if (generateDataConfig.contains("Repeat")) {
            int repeat = generateDataConfig["Repeat"].toInt();
            fmt::print("            🔄 Repeat Count: {}\n", repeat);
        }

        if (generateDataConfig.contains("Variance")) {
            double variance = generateDataConfig["Variance"].toDouble();
            fmt::print("            📊 Variance: {:.2e}\n", variance);
        }

        // Analyze random parameter limits
        if (generateDataConfig.contains("RandomParameterLimits")) {
            QJsonObject randomLimits = generateDataConfig["RandomParameterLimits"].toObject();
            fmt::print("            🎲 Random Parameter Limits ({} parameters):\n", randomLimits.size());
            for (auto it = randomLimits.begin(); it != randomLimits.end(); ++it) {
                QString param = it.key();
                QJsonObject limits = it.value().toObject();
                if (limits.contains("min") && limits.contains("max")) {
                    fmt::print("               {}: [{:.3f}, {:.3f}]\n",
                        param.toStdString(), limits["min"].toDouble(), limits["max"].toDouble());
                } else {
                    fmt::print("               {}: {}\n", param.toStdString(), it.value().toVariant().toString().toStdString());
                }
            }
        }

        // Analyze global and local random limits (legacy format)
        if (generateDataConfig.contains("GlobalRandomLimits")) {
            QString globalLimits = generateDataConfig["GlobalRandomLimits"].toString();
            fmt::print("            🌍 Global Random Limits: {}\n", globalLimits.toStdString());
        }

        if (generateDataConfig.contains("LocalRandomLimits")) {
            QString localLimits = generateDataConfig["LocalRandomLimits"].toString();
            fmt::print("            📍 Local Random Limits: {}\n", localLimits.toStdString());
        }

        // Validate configuration consistency
        fmt::print("            ✅ CONFIGURATION VALIDATION:\n");
        validateGenerateDataConfig(generateDataConfig);

    } else {
        fmt::print("            🔧 DataGenerator Mode: DISABLED (using traditional model-based generation)\n");

        if (generateDataConfig.contains("Model")) {
            int model = generateDataConfig["Model"].toInt();
            fmt::print("            🔬 Model ID: {}\n", model);
        }
    }
}

// Configuration validation for read-only analysis - Claude Generated
void AnalysisReporter::validateGenerateDataConfig(const QJsonObject& config)
{
    bool hasErrors = false;

    // Check equations vs independent variables consistency
    if (config.contains("Equations") && config.contains("IndependentVariables")) {
        QString equations = config["Equations"].toString();
        int indepVars = config["IndependentVariables"].toInt();
        QStringList equationList = equations.split("|");

        if (equationList.size() != indepVars) {
            fmt::print("               ⚠️  WARNING: Equation count ({}) ≠ Independent Variables ({})\n",
                equationList.size(), indepVars);
            hasErrors = true;
        } else {
            fmt::print("               ✅ Equations match Independent Variables ({})\n", indepVars);
        }
    }

    // Check dependent equations vs series consistency
    if (config.contains("DependentEquations") && config.contains("Series")) {
        QString depEquations = config["DependentEquations"].toString();
        int series = config["Series"].toInt();
        QStringList depEquationList = depEquations.split("|");

        if (depEquationList.size() != series) {
            fmt::print("               ⚠️  WARNING: Dependent Equation count ({}) ≠ Series ({})\n",
                depEquationList.size(), series);
            hasErrors = true;
        } else {
            fmt::print("               ✅ Dependent Equations match Series ({})\n", series);
        }
    }

    // Check data points validity
    if (config.contains("DataPoints")) {
        int dataPoints = config["DataPoints"].toInt();
        if (dataPoints <= 0) {
            fmt::print("               ❌ ERROR: DataPoints must be > 0 (current: {})\n", dataPoints);
            hasErrors = true;
        } else {
            fmt::print("               ✅ DataPoints valid ({})\n", dataPoints);
        }
    }

    // Check repeat count
    if (config.contains("Repeat")) {
        int repeat = config["Repeat"].toInt();
        if (repeat <= 0) {
            fmt::print("               ❌ ERROR: Repeat must be > 0 (current: {})\n", repeat);
            hasErrors = true;
        } else {
            fmt::print("               ✅ Repeat count valid ({})\n", repeat);
        }
    }

    if (!hasErrors) {
        fmt::print("               🎉 Configuration appears valid!\n");
    }
}


// Claude Generated: Extract and display fitted model parameters
bool AnalysisReporter::ExtractModelParameters(const QString& modelIndexStr)
{
    // Migrated: read the current project from ProjectManager - Claude Generated
    QJsonObject projectJson = SupraFit::ProjectManager::instance().getProjectAsJson();
    if (projectJson.isEmpty()) {
        fmt::print("❌ ERROR: No data loaded. File structure is empty.\n");
        return false;
    }

    // Check if this is a models file
    QStringList keys = projectJson.keys();
    QStringList modelKeys;
    for (const QString& key : keys) {
        if (key.startsWith("model_")) {
            modelKeys << key;
        }
    }

    if (modelKeys.isEmpty()) {
        fmt::print("❌ ERROR: No models found in file. Expected model_0, model_1, etc.\n");
        fmt::print("Available keys: {}\n", keys.join(", ").toStdString());
        return false;
    }

    fmt::print("📊 Found {} models in file\n", modelKeys.size());

    // Filter models if specific index requested
    QStringList targetsToProcess;
    if (!modelIndexStr.isEmpty()) {
        QString targetKey = QString("model_%1").arg(modelIndexStr);
        if (modelKeys.contains(targetKey)) {
            targetsToProcess << targetKey;
        } else {
            fmt::print("❌ ERROR: Model {} not found. Available models: {}\n", 
                      targetKey.toStdString(), modelKeys.join(", ").toStdString());
            return false;
        }
    } else {
        targetsToProcess = modelKeys;
    }

    // Extract and display parameters for each model
    fmt::print("\n=== Model Parameter Extraction ===\n\n");
    
    for (const QString& modelKey : targetsToProcess) {
        QJsonObject model = projectJson[modelKey].toObject();
        
        fmt::print("🔬 Model: {}\n", modelKey.toStdString());
        
        // Basic model information
        if (model.contains("name")) {
            fmt::print("   Name: {}\n", model["name"].toString().toStdString());
        }
        
        if (model.contains("converged")) {
            bool converged = model["converged"].toBool();
            fmt::print("   Status: {}\n", converged ? "✅ Converged" : "❌ Not Converged");
        }
        
        // Fit quality metrics
        if (model.contains("SSE")) {
            double sse = model["SSE"].toDouble();
            fmt::print("   SSE: {:.6e}\n", sse);
        }
        
        if (model.contains("AIC")) {
            double aic = model["AIC"].toDouble();
            if (aic != std::numeric_limits<double>::infinity() && aic > -999) {
                fmt::print("   AIC: {:.6e}\n", aic);
            }
        }
        
        if (model.contains("AICc")) {
            double aicc = model["AICc"].toDouble();
            if (aicc != std::numeric_limits<double>::infinity() && aicc > -999) {
                fmt::print("   AICc: {:.6e}\n", aicc);
            }
        }
        
        // Extract fitted parameters
        if (model.contains("parameter")) {
            QJsonObject params = model["parameter"].toObject();
            fmt::print("   📈 Fitted Parameters:\n");
            
            for (auto it = params.begin(); it != params.end(); ++it) {
                QString paramName = it.key();
                QJsonValue paramValue = it.value();
                
                if (paramValue.isDouble()) {
                    double value = paramValue.toDouble();
                    if (value != std::numeric_limits<double>::infinity() && 
                        value != -std::numeric_limits<double>::infinity() &&
                        !std::isnan(value)) {
                        fmt::print("      {}: {:.6e}\n", paramName.toStdString(), value);
                    } else {
                        fmt::print("      {}: [Invalid/Infinite]\n", paramName.toStdString());
                    }
                } else if (paramValue.isArray()) {
                    QJsonArray paramArray = paramValue.toArray();
                    fmt::print("      {}:", paramName.toStdString());
                    for (int i = 0; i < paramArray.size(); ++i) {
                        double value = paramArray[i].toDouble();
                        if (value != std::numeric_limits<double>::infinity() && 
                            value != -std::numeric_limits<double>::infinity() &&
                            !std::isnan(value)) {
                            fmt::print(" {:.6e}", value);
                        } else {
                            fmt::print(" [Invalid]");
                        }
                    }
                    fmt::print("\n");
                } else {
                    fmt::print("      {}: {}\n", paramName.toStdString(), 
                              paramValue.toString().toStdString());
                }
            }
        }
        
        // Extract confidence intervals if available
        if (model.contains("confidence_intervals")) {
            QJsonObject ci = model["confidence_intervals"].toObject();
            fmt::print("   📊 Confidence Intervals:\n");
            
            for (auto it = ci.begin(); it != ci.end(); ++it) {
                QString paramName = it.key();
                QJsonObject interval = it.value().toObject();
                
                if (interval.contains("lower") && interval.contains("upper")) {
                    double lower = interval["lower"].toDouble();
                    double upper = interval["upper"].toDouble();
                    fmt::print("      {}: [{:.6e}, {:.6e}]\n", 
                              paramName.toStdString(), lower, upper);
                }
            }
        }
        
        // Extract standard errors if available
        if (model.contains("standard_errors")) {
            QJsonObject se = model["standard_errors"].toObject();
            fmt::print("   📏 Standard Errors:\n");
            
            for (auto it = se.begin(); it != se.end(); ++it) {
                QString paramName = it.key();
                double error = it.value().toDouble();
                if (!std::isnan(error) && error != std::numeric_limits<double>::infinity()) {
                    fmt::print("      {}: {:.6e}\n", paramName.toStdString(), error);
                }
            }
        }
        
        fmt::print("\n");
    }

    return true;
}
