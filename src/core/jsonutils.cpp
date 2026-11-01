/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "jsonutils.h"

#include "src/core/toolset.h"

#include <QtCore/QStringList>
#include <cmath>

namespace SupraFit {

QJsonObject JsonUtils::getPostFitAnalysis(const QJsonObject& object)
{
    // Type A (Standard SupraFit): direct post_fit_analysis object
    if (object.contains("post_fit_analysis")) {
        return object["post_fit_analysis"].toObject();
    } 
    // Type A variant: object.data.methods structure  
    else if (object.contains("data") && object["data"].toObject().contains("methods")) {
        QJsonObject postFitAnalysis;
        postFitAnalysis["analysis_completed"] = true;
        postFitAnalysis["methods"] = object["data"].toObject()["methods"].toObject();
        return postFitAnalysis;
    } 
    // Type B (Direct Analysis): root-level methods object
    else if (object.contains("methods")) {
        QJsonObject postFitAnalysis;
        postFitAnalysis["analysis_completed"] = true;
        postFitAnalysis["methods"] = object["methods"].toObject();
        return postFitAnalysis;
    }
    
    return QJsonObject();
}

/*! \brief The entry of a "methods" object that holds the analysis \p method. An exported model numbers its analyses one after
 * the other (0 fast confidence, then every run of every method), so the key is not the method: the controller of an entry
 * says which method it is. Entries without controller (the older files) are taken by their key as the method.
 * Claude Generated. */
static QString MethodEntryKey(const QJsonObject& methods, SupraFit::Method method)
{
    for (auto it = methods.begin(); it != methods.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        if (entry.contains("controller") && entry["controller"].toObject().contains("Method")
            && entry["controller"].toObject()["Method"].toInt() == int(method))
            return it.key();
    }
    const QString byKey = QString::number(method);
    if (methods.contains(byKey) && !methods[byKey].toObject().contains("controller"))
        return byKey;
    return QString();
}

QJsonObject JsonUtils::getStatisticalMethod(const QJsonObject& modelObject, SupraFit::Method method)
{
    QJsonObject postFitAnalysis = getPostFitAnalysis(modelObject);
    if (postFitAnalysis.isEmpty()) {
        return QJsonObject();
    }

    QJsonObject methods = postFitAnalysis["methods"].toObject();
    const QString methodId = MethodEntryKey(methods, method);

    if (!methodId.isEmpty()) {
        QJsonObject methodData = methods[methodId].toObject();
        
        // Handle both "results" sub-object and direct access patterns
        if (methodData.contains("results")) {
            return methodData["results"].toObject();
        } else {
            return methodData;
        }
    }

    return QJsonObject();
}

QVector<qreal> JsonUtils::getParameterDistribution(const QJsonObject& paramObject)
{
    if (paramObject.contains("data") && paramObject["data"].toObject().contains("raw")) {
        QString rawDataString = paramObject["data"].toObject()["raw"].toString();
        return ToolSet::String2DoubleVec(rawDataString);
    }
    
    return QVector<qreal>();
}

bool JsonUtils::hasPostFitAnalysis(const QJsonObject& object)
{
    return !getPostFitAnalysis(object).isEmpty();
}

QStringList JsonUtils::getAvailableStatisticalMethods(const QJsonObject& modelObject)
{
    QJsonObject postFitAnalysis = getPostFitAnalysis(modelObject);
    if (postFitAnalysis.isEmpty()) {
        return QStringList();
    }

    // the method ids of the entries (see MethodEntryKey), every method once
    QJsonObject methods = postFitAnalysis["methods"].toObject();
    QStringList ids;
    for (auto it = methods.begin(); it != methods.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        QString id = it.key();
        if (entry.contains("controller") && entry["controller"].toObject().contains("Method"))
            id = QString::number(entry["controller"].toObject()["Method"].toInt());
        if (!entry.isEmpty() && !ids.contains(id))
            ids << id;
    }
    return ids;
}

QJsonObject JsonUtils::extractCompactMLFeatures(const QJsonObject& statisticalMethodData, int entropyBins)
{
    QJsonObject compactFeatures;
    
    // Process each parameter in the statistical method data - Claude Generated
    for (auto it = statisticalMethodData.begin(); it != statisticalMethodData.end(); ++it) {
        QString paramKey = it.key();
        
        // Skip non-parameter keys (controller, etc.)
        if (paramKey == "controller" || paramKey == "max_steps" || paramKey == "models" || 
            paramKey == "parameter_averages" || paramKey == "variance" || paramKey == "cv_type" || 
            paramKey == "cv_x" || paramKey == "debug_info") {
            continue;
        }
        
        QJsonObject paramData = it.value().toObject();
        QJsonObject parameterFeatures;
        
        // Extract basic parameter info
        if (paramData.contains("name")) {
            parameterFeatures["name"] = paramData["name"];
        }
        if (paramData.contains("type")) {
            parameterFeatures["type"] = paramData["type"];
        }
        if (paramData.contains("index")) {
            parameterFeatures["index"] = paramData["index"];
        }
        if (paramData.contains("value")) {
            parameterFeatures["best_fit_value"] = paramData["value"];
        }
        
        // Extract compact statistical features from boxplot data
        if (paramData.contains("boxplot")) {
            QJsonObject boxplot = paramData["boxplot"].toObject();
            
            if (boxplot.contains("count")) {
                parameterFeatures["sample_count"] = boxplot["count"];
            }
            if (boxplot.contains("mean")) {
                parameterFeatures["mean"] = boxplot["mean"];
            }
            if (boxplot.contains("stddev")) {
                parameterFeatures["standard_deviation"] = boxplot["stddev"];
                
                // Calculate relative uncertainty (CV) - Claude Generated
                double mean = boxplot["mean"].toDouble();
                double stddev = boxplot["stddev"].toDouble();
                if (mean != 0.0) {
                    parameterFeatures["relative_uncertainty"] = std::abs(stddev / mean);
                }
            }
            if (boxplot.contains("median")) {
                parameterFeatures["median"] = boxplot["median"];
            }
        }
        
        // Extract confidence interval data
        if (paramData.contains("confidence")) {
            QJsonObject confidence = paramData["confidence"].toObject();
            
            if (confidence.contains("lower") && confidence.contains("upper")) {
                double lower = confidence["lower"].toDouble();
                double upper = confidence["upper"].toDouble();
                double range = upper - lower;
                parameterFeatures["confidence_range"] = range;
                
                // Calculate confidence score (narrower range = higher confidence) - Claude Generated
                // the range is taken relative to the best fit; a best fit of exactly zero uses the mean of the
                // samples, and without a scale the range itself
                if (paramData.contains("value")) {
                    const double bestFit = paramData["value"].toDouble();
                    const double mean = paramData["boxplot"].toObject()["mean"].toDouble();
                    const double scale = bestFit != 0.0 ? std::abs(bestFit) : (mean != 0.0 ? std::abs(mean) : 1.0);
                    parameterFeatures["confidence_score"] = 1.0 / (1.0 + std::abs(range / scale));
                }
                
                parameterFeatures["confidence_lower"] = lower;
                parameterFeatures["confidence_upper"] = upper;
            }
            if (confidence.contains("error")) {
                parameterFeatures["confidence_level"] = confidence["error"];
            }
        }
        
        // Calculate Shannon entropy from histogram data - Claude Generated
        if (paramData.contains("y")) {
            QString yString = paramData["y"].toString();
            QStringList yValues = yString.split(" ", Qt::SkipEmptyParts);
            
            double entropy = 0.0;
            double totalCount = 0.0;
            
            // Calculate total count and entropy
            for (const QString& yVal : yValues) {
                double count = yVal.toDouble();
                totalCount += count;
            }
            
            if (totalCount > 0.0) {
                for (const QString& yVal : yValues) {
                    double count = yVal.toDouble();
                    if (count > 0.0) {
                        double probability = count / totalCount;
                        entropy -= probability * std::log2(probability);
                    }
                }
                parameterFeatures["shannon_entropy"] = entropy;
                parameterFeatures["entropy_bins_used"] = yValues.size();
            }
        }
        
        compactFeatures[paramKey] = parameterFeatures;
    }
    
    // Add global method information
    if (statisticalMethodData.contains("controller")) {
        QJsonObject controller = statisticalMethodData["controller"].toObject();
        controller.remove("raw"); // the raw samples are what the compact form leaves out
        compactFeatures["method_config"] = controller;
    }
    
    return compactFeatures;
}

}