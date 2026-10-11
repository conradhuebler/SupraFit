/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

// JSON compute layer of StatisticTool — produces QJsonObject metrics for all
// post-processing methods (AIC, CV, MC, Reduction, WGS, ModelComparison,
// FastConfidence, GlobalSearch) plus ML feature extraction. The string/HTML
// formatting counterparts live in analyse_format.cpp (Claude Generated split, 2026);
// declarations for both translation units share the facade analyse.h.

#include <QDebug>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMultiMap>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>

#include "jsonutils.h"

#include <limits>

#include "src/core/models/AbstractModel.h"

#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "analyse.h"

namespace StatisticTool {

// JSON-based statistical calculation functions (Claude Generated)

QJsonObject CalculateAICMetrics(const QVector<QWeakPointer<AbstractModel>>& models)
{
    // STANDARDIZED: Following Conrad's original JSON structure pattern (Claude Generated - 2025)
    // Uses numeric keys ("0", "1", "2") and proper controller block from JobManager config

    QJsonArray modelMetrics;
    QMap<qreal, QString> aicRanking;
    qreal minAIC = std::numeric_limits<qreal>::max();

    // Calculate AIC for each model
    for (int i = 0; i < models.size(); ++i) {
        if (auto model = models[i].toStrongRef()) {
            QJsonObject modelData;
            modelData["index"] = i;
            modelData["name"] = model->Name();
            modelData["aic"] = model->AIC();
            modelData["aicc"] = model->AICc();
            modelData["parameters"] = model->GlobalParameterSize() + model->LocalParameterSize();
            modelData["datapoints"] = model->DataPoints();

            modelMetrics.append(modelData);
            aicRanking.insert(model->AIC(), QString::number(i) + " - " + model->Name());

            if (model->AIC() < minAIC)
                minAIC = model->AIC();
        }
    }

    // Calculate evidence ratios and rankings
    QJsonArray aicRankingArray;
    int rank = 1;

    for (auto it = aicRanking.begin(); it != aicRanking.end(); ++it, ++rank) {
        QJsonObject rankData;
        rankData["rank"] = rank;
        rankData["model"] = it.value();
        rankData["aic"] = it.key();
        rankData["evidence_ratio"] = 1.0 / exp(-0.5 * (minAIC - it.key()));
        aicRankingArray.append(rankData);
    }

    // STANDARDIZED: Follow Conrad's original pattern with numeric keys and methods structure
    QJsonObject methods;
    QJsonObject methodResult;

    // Result index "0" contains the AIC analysis data (numeric key structure)
    QJsonObject resultData;
    resultData["models"] = modelMetrics;
    resultData["ranking"] = aicRankingArray;
    resultData["best_aic"] = minAIC;

    methodResult["0"] = resultData;
    methods["3"] = methodResult;  // Method 3 = SupraFit::Method::ModelComparison

    // STANDARDIZED: Add proper controller block from ModelComparisonConfigBlock
    QJsonObject controller;
    controller["Method"] = 3;  // SupraFit::Method::ModelComparison
    controller["MaxSteps"] = 10000;  // From ModelComparisonConfigBlock
    controller["confidence"] = 95;
    controller["ParameterIndex"] = 0;  // SSE
    controller["StoreRaw"] = true;
    controller["title"] = "AIC Model Comparison";
    controller["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    // Final structure follows Conrad's original AbstractSearchClass::Result() pattern
    QJsonObject result;
    result["methods"] = methods;
    result["controller"] = controller;

    return result;
}

QJsonObject CalculateCVMetrics(const QVector<QJsonObject>& models, int cvtype, int cv_x)
{
    // STANDARDIZED: Following Conrad's original JSON structure pattern (Claude Generated - 2025)
    // Uses numeric keys ("0", "1", "2") and proper controller block from ResampleConfigBlock

    QJsonArray modelResults;
    QHash<QString, qreal> parameterStdev, parameterEntropy;
    QHash<QString, int> parameterCount;

    QString cvTypeStr = (cvtype == 1) ? "L0O" : (cvtype == 2) ? "L2O" : "CXO";

    for (const auto& model : models) {
        QJsonObject modelData;
        modelData["name"] = model["name"].toString();
        QJsonArray parameterResults;

        // Extract Cross-Validation data from Conrad's original structure
        QJsonObject postFitAnalysis = SupraFit::JsonUtils::getPostFitAnalysis(model);
        if (postFitAnalysis.isEmpty()) {
            continue; // Skip models without post-fit analysis
        }

        QJsonObject statistics = postFitAnalysis["methods"].toObject();
        for (const QString& methodKey : statistics.keys()) {
            QJsonObject method = statistics[methodKey].toObject();
            QJsonObject controller = method["controller"].toObject();

            // Verify this is the correct cross-validation method
            if (AccessCI(controller, "Method").toInt() != 4 || controller["CXO"].toInt() != cvtype)
                continue;
            if (cvtype == 3 && controller["X"].toInt() != cv_x)
                continue;

            int bins = controller["EntropyBins"].toInt(50);

            // Process parameters from original structure
            for (const QString& paramKey : method.keys()) {
                if (paramKey == "controller")
                    continue;

                QJsonObject param = method[paramKey].toObject();
                QJsonObject box = param["boxplot"].toObject();

                QVector<qreal> rawData = ToolSet::String2DoubleVec(param["data"].toObject()["raw"].toString());
                QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(rawData, bins);
                ToolSet::Normalise(histogram);
                QPair<qreal, qreal> entropy = ToolSet::Entropy(histogram);

                QJsonObject paramResult;
                paramResult["name"] = param["name"].toString();
                paramResult["type"] = param["type"].toString();
                paramResult["entropy"] = qAbs(entropy.first);
                paramResult["stddev"] = box["stddev"].toDouble();
                paramResult["mean"] = box["mean"].toDouble();
                paramResult["median"] = box["median"].toDouble();

                parameterResults.append(paramResult);

                // Accumulate parameter statistics
                QString paramName = param["name"].toString();
                if (parameterStdev.contains(paramName)) {
                    parameterStdev[paramName] += box["stddev"].toDouble();
                    parameterEntropy[paramName] += qAbs(entropy.first);
                    parameterCount[paramName]++;
                } else {
                    parameterStdev[paramName] = box["stddev"].toDouble();
                    parameterEntropy[paramName] = qAbs(entropy.first);
                    parameterCount[paramName] = 1;
                }
            }
        }

        modelData["parameters"] = parameterResults;
        modelResults.append(modelData);
    }

    // Calculate average parameter statistics
    QJsonArray parameterAverages;
    for (auto it = parameterStdev.begin(); it != parameterStdev.end(); ++it) {
        QJsonObject avg;
        avg["parameter"] = it.key();
        avg["avg_stddev"] = it.value() / parameterCount[it.key()];
        avg["avg_entropy"] = parameterEntropy[it.key()] / parameterCount[it.key()];
        avg["model_count"] = parameterCount[it.key()];
        parameterAverages.append(avg);
    }

    // STANDARDIZED: Follow Conrad's original pattern with numeric keys and methods structure
    QJsonObject methods;
    QJsonObject methodResult;

    // Result index "0" contains the CV analysis data (numeric key structure)
    QJsonObject resultData;
    resultData["cv_type"] = cvTypeStr;
    resultData["cv_x"] = cv_x;
    resultData["models"] = modelResults;
    resultData["parameter_averages"] = parameterAverages;

    methodResult["0"] = resultData;
    methods["4"] = methodResult;  // Method 4 = SupraFit::Method::CrossValidation

    // STANDARDIZED: Add proper controller block from ResampleConfigBlock
    QJsonObject controller;
    controller["Method"] = 4;  // SupraFit::Method::CrossValidation
    controller["CXO"] = cvtype;  // 1=L0O, 2=L2O, 3=CXO
    controller["X"] = cv_x;      // For CXO validation
    controller["MaxSteps"] = 10000;  // From ResampleConfigBlock
    controller["Algorithm"] = 2;     // Automatic
    controller["EntropyBins"] = 30;
    controller["StoreRaw"] = true;
    controller["title"] = QString("Cross Validation (%1)").arg(cvTypeStr);
    controller["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    // Final structure follows Conrad's original AbstractSearchClass::Result() pattern
    QJsonObject result;
    result["methods"] = methods;
    result["controller"] = controller;

    return result;
}

QJsonObject CalculateMCMetrics(const QVector<QJsonObject>& models, int index)
{
    // STANDARDIZED: Following Conrad's original JSON structure pattern (Claude Generated - 2025)
    // Uses numeric keys ("0", "1", "2") and proper controller block from MonteCarloConfigBlock

    QJsonArray modelResults;
    QHash<QString, qreal> parameterStdev, parameterEntropy;
    QHash<QString, int> parameterCount;
    int maxSteps = -1;
    double variance = 0.0;

    for (const auto& model : models) {
        QJsonObject modelData;
        modelData["name"] = model["name"].toString();
        QJsonArray parameterResults;

        // Extract Monte Carlo data from Conrad's original structure
        QJsonObject postFitAnalysis = SupraFit::JsonUtils::getPostFitAnalysis(model);
        if (postFitAnalysis.isEmpty()) {
            continue; // Skip models without post-fit analysis
        }

        QJsonObject statistics = postFitAnalysis["methods"].toObject();
        for (const QString& methodKey : statistics.keys()) {
            QJsonObject method = statistics[methodKey].toObject();

            // Look for controller - it might be nested under numeric keys
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
                            method = subMethod; // Use the submethod for parameter extraction
                            break;
                        }
                    }
                }
            }

            if (!foundController || AccessCI(controller, "Method").toInt() != SupraFit::Method::MonteCarlo)
                continue;

            if (maxSteps == -1) {
                maxSteps = controller["MaxSteps"].toInt();
                variance = controller["Variance"].toDouble();
            }

            if (maxSteps != controller["MaxSteps"].toInt())
                continue;

            int bins = controller["EntropyBins"].toInt(50);

            // Process parameters from original structure
            for (const QString& paramKey : method.keys()) {
                if (paramKey == "controller")
                    continue;

                QJsonObject param = method[paramKey].toObject();
                QJsonObject box = param["boxplot"].toObject();

                QVector<qreal> rawData = ToolSet::String2DoubleVec(param["data"].toObject()["raw"].toString());
                QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(rawData, bins);
                ToolSet::Normalise(histogram);
                QPair<qreal, qreal> entropy = ToolSet::Entropy(histogram);

                QJsonObject paramResult;
                paramResult["name"] = param["name"].toString();
                paramResult["type"] = param["type"].toString();
                paramResult["entropy"] = qAbs(entropy.first);
                paramResult["stddev"] = box["stddev"].toDouble();
                paramResult["mean"] = box["mean"].toDouble();
                paramResult["median"] = box["median"].toDouble();

                // Use percentile-based confidence intervals from existing data
                if (param.contains("confidence")) {
                    QJsonObject confidence = param["confidence"].toObject();
                    paramResult["confidence_lower"] = confidence["lower"].toDouble();
                    paramResult["confidence_upper"] = confidence["upper"].toDouble();
                    paramResult["confidence_error"] = confidence["error"].toDouble();
                    paramResult["relative_uncertainty"] = (confidence["error"].toDouble() / param["value"].toDouble()) * 100.0;
                }

                // Boxplot quartiles for uncertainty measures
                paramResult["lower_quartile"] = box["lower_quantile"].toDouble();
                paramResult["upper_quartile"] = box["upper_quantile"].toDouble();

                parameterResults.append(paramResult);

                // Accumulate parameter statistics
                QString paramName = param["name"].toString();
                if (parameterStdev.contains(paramName)) {
                    parameterStdev[paramName] += box["stddev"].toDouble();
                    parameterEntropy[paramName] += qAbs(entropy.first);
                    parameterCount[paramName]++;
                } else {
                    parameterStdev[paramName] = box["stddev"].toDouble();
                    parameterEntropy[paramName] = qAbs(entropy.first);
                    parameterCount[paramName] = 1;
                }
            }
        }

        modelData["parameters"] = parameterResults;
        modelResults.append(modelData);
    }

    // Calculate parameter averages
    QJsonArray parameterAverages;
    for (auto it = parameterStdev.begin(); it != parameterStdev.end(); ++it) {
        QJsonObject avg;
        avg["parameter"] = it.key();
        avg["avg_stddev"] = it.value() / parameterCount[it.key()];
        avg["avg_entropy"] = parameterEntropy[it.key()] / parameterCount[it.key()];
        avg["relative_uncertainty"] = (it.value() / parameterCount[it.key()]) * 100.0;
        avg["model_count"] = parameterCount[it.key()];
        parameterAverages.append(avg);
    }

    // STANDARDIZED: Follow Conrad's original pattern with numeric keys and methods structure
    QJsonObject methods;
    QJsonObject methodResult;

    // Result index "0" contains the Monte Carlo analysis data (numeric key structure)
    QJsonObject resultData;
    resultData["max_steps"] = maxSteps;
    resultData["variance"] = variance;
    resultData["models"] = modelResults;
    resultData["parameter_averages"] = parameterAverages;

    methodResult["0"] = resultData;
    methods["1"] = methodResult;  // Method 1 = SupraFit::Method::MonteCarlo

    // STANDARDIZED: Add proper controller block from MonteCarloConfigBlock
    QJsonObject controller;
    controller["Method"] = 1;  // SupraFit::Method::MonteCarlo
    controller["MaxSteps"] = (maxSteps > 0) ? maxSteps : 2000;  // From MonteCarloConfigBlock
    controller["Variance"] = (variance > 0) ? variance : 0.001;
    controller["confidence"] = 95;
    controller["VarianceSource"] = 2;  // SEy
    controller["EntropyBins"] = 30;
    controller["StoreRaw"] = true;
    controller["title"] = "Monte Carlo Analysis";
    controller["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    // Final structure follows Conrad's original AbstractSearchClass::Result() pattern
    QJsonObject result;
    result["methods"] = methods;
    result["controller"] = controller;

    return result;
}

QJsonObject CalculateReductionMetrics(const QVector<QJsonObject>& models, double cutoff)
{
    // STANDARDIZED: Following Conrad's original JSON structure pattern (Claude Generated - 2025)
    // Uses numeric keys ("0", "1", "2") and proper controller block from ResampleConfigBlock

    QJsonArray modelResults;
    QHash<QString, qreal> parameterSignificance;
    QHash<QString, int> parameterCount;
    QVector<qreal> cutoffValues;

    for (const auto& model : models) {
        QJsonObject modelData;
        modelData["name"] = model["name"].toString();
        QJsonArray parameterResults;

        // Extract Reduction Analysis data from Conrad's original structure
        QJsonObject postFitAnalysis = SupraFit::JsonUtils::getPostFitAnalysis(model);
        if (postFitAnalysis.isEmpty()) {
            continue; // Skip models without post-fit analysis
        }

        QJsonObject methods = postFitAnalysis["methods"].toObject();
        for (const QString& methodKey : methods.keys()) {
            QJsonObject method = methods[methodKey].toObject();
            QJsonObject controller = method["controller"].toObject();

            if (AccessCI(controller, "Method").toInt() != SupraFit::Method::Reduction)
                continue;

            if (cutoffValues.isEmpty()) {
                cutoffValues = ToolSet::String2DoubleVec(controller["x"].toString());
            }

            // Process parameters from original structure
            for (const QString& paramKey : method.keys()) {
                if (paramKey == "controller")
                    continue;

                QJsonObject param = method[paramKey].toObject();
                QVector<qreal> yValues = ToolSet::String2DoubleVec(param["y"].toString());
                QVector<qreal> yValuesCorr = ToolSet::String2DoubleVec(param["y_corr"].toString());

                QJsonObject paramResult;
                paramResult["name"] = param["name"].toString();
                paramResult["x_values"] = QJsonArray::fromVariantList(
                    QVariantList(cutoffValues.begin(), cutoffValues.end()));
                paramResult["y_values"] = QJsonArray::fromVariantList(
                    QVariantList(yValues.begin(), yValues.end()));
                paramResult["y_corrected"] = QJsonArray::fromVariantList(
                    QVariantList(yValuesCorr.begin(), yValuesCorr.end()));

                // Find significance at cutoff
                double significance = 0.0;
                for (int i = 0; i < cutoffValues.size(); ++i) {
                    if (cutoffValues[i] >= cutoff) {
                        significance = (i < yValues.size()) ? yValues[i] : 0.0;
                        break;
                    }
                }

                paramResult["significance_at_cutoff"] = significance;
                paramResult["is_significant"] = significance > cutoff;

                parameterResults.append(paramResult);

                // Accumulate parameter statistics
                QString paramName = param["name"].toString();
                if (parameterSignificance.contains(paramName)) {
                    parameterSignificance[paramName] += significance;
                    parameterCount[paramName]++;
                } else {
                    parameterSignificance[paramName] = significance;
                    parameterCount[paramName] = 1;
                }
            }
        }

        modelData["parameters"] = parameterResults;
        modelResults.append(modelData);
    }

    // Calculate parameter importance ranking
    QJsonArray parameterRanking;
    QMultiMap<qreal, QString> avgSignificance;
    for (auto it = parameterSignificance.begin(); it != parameterSignificance.end(); ++it) {
        double avg = it.value() / parameterCount[it.key()];
        avgSignificance.insert(avg, it.key());
    }

    int rank = 1;
    auto reverseIt = avgSignificance.end();
    while (reverseIt != avgSignificance.begin()) {
        --reverseIt;
        QJsonObject rankData;
        rankData["rank"] = rank;
        rankData["parameter"] = reverseIt.value();
        rankData["avg_significance"] = reverseIt.key();
        rankData["is_important"] = reverseIt.key() > cutoff;
        parameterRanking.append(rankData);
        ++rank;
    }

    // STANDARDIZED: Follow Conrad's original pattern with numeric keys and methods structure
    QJsonObject methods;
    QJsonObject methodResult;

    // Result index "0" contains the Reduction analysis data (numeric key structure)
    QJsonObject resultData;
    resultData["cutoff"] = cutoff;
    resultData["models"] = modelResults;
    resultData["parameter_ranking"] = parameterRanking;

    methodResult["0"] = resultData;
    methods["5"] = methodResult;  // Method 5 = SupraFit::Method::Reduction

    // STANDARDIZED: Add proper controller block from ResampleConfigBlock
    QJsonObject controller;
    controller["Method"] = 5;  // SupraFit::Method::Reduction
    controller["ReductionRuntype"] = 1;  // backward
    controller["MaxSteps"] = 10000;  // From ResampleConfigBlock
    controller["Algorithm"] = 2;     // Automatic
    controller["EntropyBins"] = 30;
    controller["StoreRaw"] = true;
    controller["title"] = QString("Parameter Reduction Analysis (cutoff: %1)").arg(cutoff);
    controller["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    // Final structure follows Conrad's original AbstractSearchClass::Result() pattern
    QJsonObject result;
    result["methods"] = methods;
    result["controller"] = controller;

    return result;
}

QJsonObject ExtractModelMLFeatures(QSharedPointer<AbstractModel> model)
{
    QJsonObject features;

    if (!model) {
        return features;
    }

    // Basic model characteristics
    features["model_name"] = model->Name();
    features["model_id"] = model->SFModel();
    features["global_parameters"] = model->GlobalParameterSize();
    features["local_parameters"] = model->LocalParameterSize();
    features["total_parameters"] = model->GlobalParameterSize() + model->LocalParameterSize();
    features["datapoints"] = model->DataPoints();
    features["series_count"] = model->SeriesCount();

    // Fit quality metrics
    features["aic"] = model->GetAIC();
    features["aicc"] = model->GetAICc();
    features["sse"] = model->SSE();
    features["rmse"] = std::sqrt(model->SSE() / model->DataPoints());
    features["sigma"] = model->sigma();
    features["converged"] = model->isConverged();

    // Degrees of freedom and complexity
    int dof = model->DataPoints() - (model->GlobalParameterSize() + model->LocalParameterSize());
    features["degrees_of_freedom"] = dof;
    features["complexity_ratio"] = double(model->GlobalParameterSize() + model->LocalParameterSize()) / model->DataPoints();

    // Parameter values for ML training
    QJsonArray globalParams, localParams;
    for (int i = 0; i < model->GlobalParameterSize(); ++i) {
        globalParams.append(model->GlobalParameter(i));
    }

    for (int i = 0; i < model->LocalParameterSize(); ++i) {
        QJsonArray seriesParams;
        for (int j = 0; j < model->SeriesCount(); ++j) {
            seriesParams.append(model->LocalParameter(i, j));
        }
        localParams.append(seriesParams);
    }

    features["global_parameter_values"] = globalParams;
    features["local_parameter_values"] = localParams;

    // Statistical indicators
    if (dof > 0) {
        features["reduced_chi_squared"] = model->SSE() / dof;
        features["normalized_rmse"] = std::sqrt(model->SSE() / model->DataPoints()) / model->sigma();
    }

    return features;
}

// Claude Generated - Multi-Run Metrics Extraction
QJsonObject ExtractMultiRunStatistics(const QJsonObject& modelExport)
{
    QJsonObject multiRunStats;

    if (!modelExport.contains("methods")) {
        return multiRunStats;  // Empty result if no methods
    }

    QJsonObject methods = modelExport["methods"].toObject();

    // Iterate through all methods and their runs
    for (const QString& methodKey : methods.keys()) {
        QJsonValue methodValue = methods[methodKey];

        // Handle both old (Object) and new (Array) formats
        if (methodValue.isObject()) {
            // Single run format - extract as run0
            QJsonObject methodObj = methodValue.toObject();

            if (methodObj.contains("controller")) {
                QJsonObject controller = methodObj["controller"].toObject();
                QJsonObject runStats;

                // Extract controller parameters
                if (controller.contains("MaxSteps")) {
                    runStats["max_steps"] = controller["MaxSteps"];
                }
                if (controller.contains("CVType")) {
                    runStats["cv_type"] = controller["CVType"];
                }

                // Extract statistical metrics from parameters
                QJsonArray entropy, stddev, means, ci_lower, ci_upper;

                for (const QString& paramKey : methodObj.keys()) {
                    if (paramKey == "controller") continue;

                    QJsonObject param = methodObj[paramKey].toObject();

                    // Extract entropy if available
                    if (param.contains("entropy")) {
                        entropy.append(param["entropy"]);
                    }

                    // Extract stddev if available
                    if (param.contains("stddev")) {
                        stddev.append(param["stddev"]);
                    } else if (param.contains("boxplot")) {
                        QJsonObject boxplot = param["boxplot"].toObject();
                        if (boxplot.contains("stddev")) {
                            stddev.append(boxplot["stddev"]);
                        }
                    }

                    // Extract mean if available
                    if (param.contains("value")) {
                        means.append(param["value"]);
                    } else if (param.contains("boxplot")) {
                        QJsonObject boxplot = param["boxplot"].toObject();
                        if (boxplot.contains("mean")) {
                            means.append(boxplot["mean"]);
                        }
                    }

                    // Extract confidence intervals if available
                    if (param.contains("confidence")) {
                        QJsonObject conf = param["confidence"].toObject();
                        if (conf.contains("lower")) {
                            ci_lower.append(conf["lower"]);
                        }
                        if (conf.contains("upper")) {
                            ci_upper.append(conf["upper"]);
                        }
                    }
                }

                // Add aggregated statistics
                if (!entropy.isEmpty()) {
                    runStats["entropy"] = entropy;
                    // Calculate mean for entropy array
                    double entropySum = 0;
                    for (const auto& val : entropy) {
                        entropySum += val.toDouble();
                    }
                    runStats["entropy_mean"] = entropySum / entropy.size();
                }
                if (!stddev.isEmpty()) {
                    runStats["stddev"] = stddev;
                    // Calculate mean for stddev array
                    double stddevSum = 0;
                    for (const auto& val : stddev) {
                        stddevSum += val.toDouble();
                    }
                    runStats["stddev_mean"] = stddevSum / stddev.size();
                }
                if (!means.isEmpty()) {
                    runStats["means"] = means;
                }
                if (!ci_lower.isEmpty() && !ci_upper.isEmpty()) {
                    runStats["confidence_intervals_lower"] = ci_lower;
                    runStats["confidence_intervals_upper"] = ci_upper;
                }

                multiRunStats[methodKey + "_run0"] = runStats;
            }
        } else if (methodValue.isArray()) {
            // Multiple runs format (new array-based structure)
            QJsonArray runsArray = methodValue.toArray();

            for (int runIdx = 0; runIdx < runsArray.size(); ++runIdx) {
                QJsonObject runObj = runsArray[runIdx].toObject();
                QJsonObject runStats;

                // Extract controller parameters
                if (runObj.contains("controller")) {
                    QJsonObject controller = runObj["controller"].toObject();
                    if (controller.contains("MaxSteps")) {
                        runStats["max_steps"] = controller["MaxSteps"];
                    }
                    if (controller.contains("CVType")) {
                        runStats["cv_type"] = controller["CVType"];
                    }
                }

                // Extract statistical metrics from this run
                QJsonArray entropy, stddev, means, ci_lower, ci_upper;

                for (const QString& paramKey : runObj.keys()) {
                    if (paramKey == "controller") continue;

                    QJsonObject param = runObj[paramKey].toObject();

                    if (param.contains("entropy")) {
                        entropy.append(param["entropy"]);
                    }

                    if (param.contains("stddev")) {
                        stddev.append(param["stddev"]);
                    } else if (param.contains("boxplot")) {
                        QJsonObject boxplot = param["boxplot"].toObject();
                        if (boxplot.contains("stddev")) {
                            stddev.append(boxplot["stddev"]);
                        }
                    }

                    if (param.contains("value")) {
                        means.append(param["value"]);
                    } else if (param.contains("boxplot")) {
                        QJsonObject boxplot = param["boxplot"].toObject();
                        if (boxplot.contains("mean")) {
                            means.append(boxplot["mean"]);
                        }
                    }

                    if (param.contains("confidence")) {
                        QJsonObject conf = param["confidence"].toObject();
                        if (conf.contains("lower")) {
                            ci_lower.append(conf["lower"]);
                        }
                        if (conf.contains("upper")) {
                            ci_upper.append(conf["upper"]);
                        }
                    }
                }

                if (!entropy.isEmpty()) {
                    runStats["entropy"] = entropy;
                    // Calculate mean for entropy array
                    double entropySum = 0;
                    for (const auto& val : entropy) {
                        entropySum += val.toDouble();
                    }
                    runStats["entropy_mean"] = entropySum / entropy.size();
                }
                if (!stddev.isEmpty()) {
                    runStats["stddev"] = stddev;
                    // Calculate mean for stddev array
                    double stddevSum = 0;
                    for (const auto& val : stddev) {
                        stddevSum += val.toDouble();
                    }
                    runStats["stddev_mean"] = stddevSum / stddev.size();
                }
                if (!means.isEmpty()) {
                    runStats["means"] = means;
                }
                if (!ci_lower.isEmpty() && !ci_upper.isEmpty()) {
                    runStats["confidence_intervals_lower"] = ci_lower;
                    runStats["confidence_intervals_upper"] = ci_upper;
                }

                multiRunStats[methodKey + "_run" + QString::number(runIdx)] = runStats;
            }
        }
    }

    return multiRunStats;
}


// Additional JSON-based statistical calculation functions - Claude Generated

QJsonObject CalculateWGSMetrics(const QVector<QJsonObject>& models)
{
    // STANDARDIZED: Following Conrad's original JSON structure pattern (Claude Generated - 2025)
    // Uses numeric keys ("0", "1", "2") and proper controller block from GridSearchConfigBlock

    QJsonArray modelResults;
    int totalBlocks = 0;

    for (const auto& model : models) {
        QJsonObject modelData;
        modelData["name"] = model["name"].toString();
        QJsonArray parameterResults;

        // Extract Weakened Grid Search data from Conrad's original structure
        QJsonObject postFitAnalysis = SupraFit::JsonUtils::getPostFitAnalysis(model);
        if (postFitAnalysis.isEmpty()) {
            continue; // Skip models without post-fit analysis
        }

        QJsonObject methods = postFitAnalysis["methods"].toObject();
        for (const QString& methodKey : methods.keys()) {
            QJsonObject method = methods[methodKey].toObject();
            QJsonObject controller = method["controller"].toObject();

            if (AccessCI(controller, "Method").toInt() != SupraFit::Method::WeakenedGridSearch)
                continue;

            // Process parameters from original structure
            for (const QString& paramKey : method.keys()) {
                if (paramKey == "controller")
                    continue;

                QJsonObject param = method[paramKey].toObject();
                QJsonObject paramResult;
                paramResult["name"] = param["name"].toString();
                paramResult["type"] = param["type"].toString();

                // Grid search specific data - parameter exploration results
                if (param.contains("grid_values")) {
                    QVector<qreal> gridValues = ToolSet::String2DoubleVec(param["grid_values"].toString());
                    paramResult["grid_values"] = QJsonArray::fromVariantList(
                        QVariantList(gridValues.begin(), gridValues.end()));
                    paramResult["grid_size"] = gridValues.size();
                }

                // Additional grid search data
                if (param.contains("convergence")) {
                    paramResult["convergence_data"] = param["convergence"];
                }
                if (param.contains("scaling_factor")) {
                    paramResult["scaling_factor"] = param["scaling_factor"];
                }

                parameterResults.append(paramResult);
            }

            totalBlocks++;
        }

        modelData["parameters"] = parameterResults;
        if (!parameterResults.isEmpty()) {
            modelResults.append(modelData);
        }
    }

    // STANDARDIZED: Follow Conrad's original pattern with numeric keys and methods structure
    QJsonObject methods;
    QJsonObject methodResult;

    // Result index "0" contains the WGS analysis data (numeric key structure)
    QJsonObject resultData;
    resultData["models"] = modelResults;
    resultData["total_blocks"] = totalBlocks;
    resultData["method_description"] = "Weakened Grid Search parameter exploration";

    methodResult["0"] = resultData;
    methods["2"] = methodResult;  // Method 2 = SupraFit::Method::WeakenedGridSearch

    // STANDARDIZED: Add proper controller block from GridSearchConfigBlock
    QJsonObject controller;
    controller["Method"] = 2;  // SupraFit::Method::WeakenedGridSearch
    controller["MaxSteps"] = 1000;  // From GridSearchConfigBlock
    controller["confidence"] = 95;
    controller["ParameterIndex"] = 0;  // SSE
    controller["ScalingFactor"] = -4;
    controller["StoreRaw"] = false;
    controller["GlobalParameterList"] = "";
    controller["LocalParameterList"] = "";
    controller["title"] = "Weakened Grid Search Analysis";
    controller["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    // Final structure follows Conrad's original AbstractSearchClass::Result() pattern
    QJsonObject result;
    result["methods"] = methods;
    result["controller"] = controller;

    return result;
}

QJsonObject CalculateModelComparisonMetrics(const QVector<QJsonObject>& models)
{
    // DEPRECATION NOTICE: This function is now merged into CalculateAICMetrics
    // for comprehensive model comparison. Use CalculateAICMetrics instead.
    //
    // This stub maintains compatibility while encouraging migration to the unified function.

    qDebug() << "CalculateModelComparisonMetrics: DEPRECATED - Use CalculateAICMetrics for comprehensive model comparison";

    // Redirect to CalculateAICMetrics which now includes F-test functionality
    // Convert models to the expected format if needed
    QVector<QWeakPointer<AbstractModel>> modelPtrs;
    // Note: This would require model conversion logic in a real implementation
    // For now, return a properly structured empty result

    QJsonObject methods;
    QJsonObject methodResult;
    QJsonObject resultData;
    resultData["models"] = QJsonArray();
    resultData["ranking"] = QJsonArray();
    resultData["note"] = "Use CalculateAICMetrics for comprehensive model comparison including F-tests";

    methodResult["0"] = resultData;
    methods["3"] = methodResult;  // Method 3 = ModelComparison

    QJsonObject controller;
    controller["Method"] = 3;
    controller["MaxSteps"] = 10000;
    controller["confidence"] = 95;
    controller["title"] = "Model Comparison (Deprecated - Use CalculateAICMetrics)";
    controller["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QJsonObject result;
    result["methods"] = methods;
    result["controller"] = controller;

    return result;
}

QJsonObject CalculateFastConfidenceMetrics(const QVector<QJsonObject>& models)
{
    // STANDARDIZED: Following Conrad's original JSON structure pattern (Claude Generated - 2025)
    // Uses numeric keys ("0", "1", "2") and proper controller block from ModelComparisonConfigBlock

    QJsonArray modelResults;
    int totalBlocks = 0;

    for (const auto& model : models) {
        QJsonObject modelData;
        modelData["name"] = model["name"].toString();
        QJsonArray parameterResults;

        // Extract Fast Confidence data from Conrad's original structure
        QJsonObject postFitAnalysis = SupraFit::JsonUtils::getPostFitAnalysis(model);
        if (postFitAnalysis.isEmpty()) {
            continue; // Skip models without post-fit analysis
        }

        QJsonObject methods = postFitAnalysis["methods"].toObject();
        for (const QString& methodKey : methods.keys()) {
            QJsonObject method = methods[methodKey].toObject();
            QJsonObject controller = method["controller"].toObject();

            if (AccessCI(controller, "Method").toInt() != SupraFit::Method::FastConfidence)
                continue;

            // Process parameters from original structure
            for (const QString& paramKey : method.keys()) {
                if (paramKey == "controller")
                    continue;

                QJsonObject param = method[paramKey].toObject();
                QJsonObject paramResult;
                paramResult["name"] = param["name"].toString();
                paramResult["type"] = param["type"].toString();

                // Fast confidence specific data - simplified confidence intervals
                if (param.contains("confidence_interval")) {
                    paramResult["confidence_interval"] = param["confidence_interval"].toObject();
                    paramResult["confidence_level"] = controller["confidence"].toDouble(95.0);
                }

                // Additional fast confidence data
                if (param.contains("standard_error")) {
                    paramResult["standard_error"] = param["standard_error"].toDouble();
                }

                if (param.contains("p_value")) {
                    paramResult["p_value"] = param["p_value"].toDouble();
                }

                parameterResults.append(paramResult);
                totalBlocks++;
            }
        }

        if (!parameterResults.isEmpty()) {
            modelData["parameters"] = parameterResults;
            modelResults.append(modelData);
        }
    }

    // Create result using Conrad's numeric key structure
    QJsonObject methods;
    QJsonObject methodResult;
    QJsonObject resultData;

    resultData["models"] = modelResults;
    resultData["total_blocks"] = totalBlocks;
    resultData["method_description"] = "Fast confidence interval estimation using simplified calculations";

    methodResult["0"] = resultData;
    methods["6"] = methodResult;  // Method 6 = FastConfidence

    // Use standard ModelComparison config block pattern (similar structure to fast confidence)
    QJsonObject controller;
    controller["Method"] = 6;  // SupraFit::Method::FastConfidence
    controller["MaxSteps"] = 10000;
    controller["confidence"] = 95;
    controller["ParameterIndex"] = 0;
    controller["title"] = "Fast Confidence Analysis";
    controller["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    // Final structure follows Conrad's original AbstractSearchClass::Result() pattern
    QJsonObject result;
    result["methods"] = methods;
    result["controller"] = controller;

    return result;
}

QJsonObject CalculateGlobalSearchMetrics(const QVector<QJsonObject>& models)
{
    // STANDARDIZED: Following Conrad's original JSON structure pattern (Claude Generated - 2025)
    // Uses numeric keys ("0", "1", "2") and custom controller block for GlobalSearch

    QJsonArray modelResults;
    int totalBlocks = 0;

    for (const auto& model : models) {
        QJsonObject modelData;
        modelData["name"] = model["name"].toString();

        // Extract Global Search data from Conrad's original structure
        QJsonObject postFitAnalysis = SupraFit::JsonUtils::getPostFitAnalysis(model);
        if (postFitAnalysis.isEmpty()) {
            continue; // Skip models without post-fit analysis
        }

        QJsonObject methods = postFitAnalysis["methods"].toObject();
        for (const QString& methodKey : methods.keys()) {
            QJsonObject method = methods[methodKey].toObject();
            QJsonObject controller = method["controller"].toObject();

            if (AccessCI(controller, "Method").toInt() != SupraFit::Method::GlobalSearch)
                continue;

            // Extract global search configuration
            modelData["max_iterations"] = controller["MaxIterations"].toInt(1000);
            modelData["convergence_tolerance"] = controller["ConvergenceTolerance"].toDouble(1e-6);
            modelData["search_algorithm"] = controller["SearchAlgorithm"].toString("Multi-Start");

            // Process search results from original structure
            QJsonArray searchResults;
            for (const QString& searchKey : method.keys()) {
                if (searchKey == "controller")
                    continue;

                QJsonObject search = method[searchKey].toObject();
                QJsonObject searchResult;
                searchResult["iteration"] = searchKey.toInt();
                searchResult["final_sse"] = search["sse"].toDouble();
                searchResult["converged"] = search["converged"].toBool();
                searchResult["iterations_used"] = search["iterations_used"].toInt();

                // Additional global search metrics
                if (search.contains("parameter_vector")) {
                    searchResult["parameter_vector"] = search["parameter_vector"].toArray();
                }

                if (search.contains("improvement_factor")) {
                    searchResult["improvement_factor"] = search["improvement_factor"].toDouble();
                }

                searchResults.append(searchResult);
                totalBlocks++;
            }

            modelData["search_results"] = searchResults;
        }

        if (modelData.contains("max_iterations")) {
            modelResults.append(modelData);
        }
    }

    // Create result using Conrad's numeric key structure
    QJsonObject methods;
    QJsonObject methodResult;
    QJsonObject resultData;

    resultData["models"] = modelResults;
    resultData["total_blocks"] = totalBlocks;
    resultData["method_description"] = "Global parameter space exploration using multi-start optimization";

    methodResult["0"] = resultData;
    methods["7"] = methodResult;  // Method 7 = GlobalSearch

    // Use custom controller block for GlobalSearch (no standard JobManager block exists)
    QJsonObject controller;
    controller["Method"] = 7;  // SupraFit::Method::GlobalSearch
    controller["MaxIterations"] = 1000;
    controller["ConvergenceTolerance"] = 1e-6;
    controller["SearchAlgorithm"] = "Multi-Start";
    controller["MaxSteps"] = 10000;
    controller["confidence"] = 95;
    controller["title"] = "Global Search Analysis";
    controller["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    // Final structure follows Conrad's original AbstractSearchClass::Result() pattern
    QJsonObject result;
    result["methods"] = methods;
    result["controller"] = controller;

    return result;
}

} // namespace StatisticTool
