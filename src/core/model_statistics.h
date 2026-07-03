/*
 * SupraFit - Canonical per-model statistics value type
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Shared plain-data struct used by both the core AnalysisManager and the CLI
 * AnalysisReporter, replacing the two previously identical copies (D4). Claude Generated.
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

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QString>

/**
 * @brief Per-model fit + post-processing statistics extracted from a project's model_X JSON.
 *
 * Canonical shared type (D4): consolidates the formerly duplicate definitions in
 * analysis_manager.h and the CLI's AnalysisReporter. Claude Generated.
 */
struct ModelStatistics {
    QString key;    // Model identifier (e.g. "model_0")
    QString name;   // Model name
    QString status; // Convergence status
    bool hasValidStats = false;

    // Core fit statistics
    double sse = -1;    // Sum of Squared Errors
    double sae = -1;    // Sum of Absolute Errors
    double aic = -999;  // Akaike Information Criterion
    double aicc = -999; // Corrected AIC

    // Parameter counts
    int globalParams = -1; // Global parameters
    int localParams = -1;  // Local parameters

    // Post-processing method block counts
    int mcBlocks = 0;        // Monte Carlo
    int wgsBlocks = 0;       // Weakened Grid Search
    int modelCompBlocks = 0; // Model Comparison
    int cvBlocks = 0;        // Cross Validation
    int reductionBlocks = 0; // Reduction
    int fastConfBlocks = 0;  // Fast Confidence
    int globalBlocks = 0;    // Global Search
    int totalPPBlocks = 0;   // Total post-processing blocks

    // Cached post-processing metrics (JSON)
    QJsonObject postProcessingData;
};
