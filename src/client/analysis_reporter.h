/*
 * SupraFit CLI - Model statistics reporting (console tables)
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Stateless formatting helpers extracted from the SupraFitCli god-class as part of
 * the CLI decomposition (TECHNICAL_DEBT.md D3). Marked sections are Claude Generated.
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
#include <QtCore/QVector>

#include "src/core/model_statistics.h"

/**
 * @brief Formats fitted-model statistics and post-processing results as console tables.
 *
 * Stateless helper extracted from SupraFitCli (D3): every method is static and operates
 * purely on project/model JSON, printing to stdout via fmt. No SupraFit state is touched.
 * Uses the shared ModelStatistics type from model_statistics.h (D4). Claude Generated.
 */
class AnalysisReporter {
public:
    /** Extract statistics for one model from its model_X JSON object. */
    static ModelStatistics extractModelStatistics(const QString& key, const QJsonObject& modelObj);

    /** Print the compact model-comparison table (SSE/AIC/params/post-processing counts). */
    static void displayModelStatisticsTable(const QVector<ModelStatistics>& models);

    /** Print detailed post-processing method results per model. */
    static void displayPostProcessingDetails(const QVector<ModelStatistics>& models);

    /** Print an analysis of a GenerateData config block (read-only; recurses into nested configs). */
    static void analyzeGenerateDataConfig(const QJsonObject& generateDataConfig);

    /** Validate a GenerateData config block, printing warnings/errors. */
    static void validateGenerateDataConfig(const QJsonObject& config);

    /** Print fitted parameters of the current project's models (project loaded via ProjectManager). */
    static bool ExtractModelParameters(const QString& modelIndexStr = QString());

private:
    static void displayPostProcessingMethod(const QString& methodName, const QString& emoji,
                                            int blockCount, const QJsonObject& methodData, int methodType);
};
