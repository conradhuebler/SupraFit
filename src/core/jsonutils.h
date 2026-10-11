#pragma once

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

#include "src/global.h"

#include <QtCore/QJsonObject>
#include <QtCore/QVector>

namespace SupraFit {

/**
 * @brief Unified data access layer for SupraFit JSON handling
 * 
 * This class provides a single, unified interface for accessing statistical
 * data from SupraFit JSON structures, eliminating redundancy across Type A,
 * Type B, and Type C file formats documented in SUPRAFIT_JSON_FORMAT.md
 * 
 * Claude Generated - 2025-09-01
 */
class JsonUtils {
public:
    /**
     * @brief Get post-fit analysis object from any SupraFit JSON structure
     * 
     * This function handles the different access paths for statistical data:
     * - Type A (Standard .suprafit/.json): object.post_fit_analysis
     * - Type B (Direct Analysis): constructs from object.methods
     * - Type C (ML-Pipeline): constructs from object.data.methods
     * 
     * @param object The JSON object to extract post-fit analysis from
     * @return QJsonObject containing post_fit_analysis structure, or empty object if not found
     */
    static QJsonObject getPostFitAnalysis(const QJsonObject& object);

    /**
     * @brief Get statistical method results from a model object
     * 
     * Unified access to specific statistical method data regardless of JSON structure.
     * Handles both "results" sub-object and direct access patterns.
     * 
     * @param modelObject JSON object representing a model
     * @param method SupraFit::Method enum value for the statistical method
     * @return QJsonObject containing the statistical method results, or empty object if not found
     */
    static QJsonObject getStatisticalMethod(const QJsonObject& modelObject, SupraFit::Method method);

    /**
     * @brief Extract raw parameter distribution from parameter object
     * 
     * Extracts space-separated parameter distribution values from the data.raw field
     * used in Monte Carlo and other statistical analyses.
     * 
     * @param paramObject JSON object representing a parameter with distribution data
     * @return QVector<qreal> containing the parameter distribution values
     */
    static QVector<qreal> getParameterDistribution(const QJsonObject& paramObject);

    /**
     * @brief Check if object contains valid post-fit analysis structure
     * 
     * @param object The JSON object to validate
     * @return true if object contains accessible statistical data
     */
    static bool hasPostFitAnalysis(const QJsonObject& object);

    /**
     * @brief Get all available statistical methods from a model object
     * 
     * @param modelObject JSON object representing a model
     * @return QStringList containing method IDs that are available in the model
     */
    static QStringList getAvailableStatisticalMethods(const QJsonObject& modelObject);

    /**
     * @brief Extract compact ML features from raw statistical data - Claude Generated
     * 
     * Analyzes raw parameter samples and extracts compact statistical features suitable for ML:
     * - Shannon entropy (configurable bins)
     * - Relative uncertainty
     * - Parameter confidence score
     * - Distribution characteristics (without raw data)
     * 
     * @param statisticalMethodData JSON object containing raw statistical analysis data
     * @param entropyBins Number of bins for entropy calculation (default: 50)
     * @return QJsonObject containing compact ML features for each parameter
     */
    static QJsonObject extractCompactMLFeatures(const QJsonObject& statisticalMethodData, int entropyBins = 50);
};

}