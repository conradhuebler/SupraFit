/*
 * SupraFit CLI - Machine-learning training-data export
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Stateless helper extracted from the SupraFitCli god-class as part of the CLI
 * decomposition (TECHNICAL_DEBT.md D3). Marked sections are Claude Generated.
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

#include <QtCore/QString>
#include <QtCore/QVector>

/**
 * @brief Exports compact ML training data from ML-pipeline result files.
 *
 * Stateless helper extracted from SupraFitCli (D3): static methods that read
 * ML-pipeline JSON files via MLFeatureExtractor and write the neural-network
 * training data through ProjectManager. No SupraFit state is touched. Claude Generated.
 */
class MlExport {
public:
    /** Export training data aggregated from multiple ML-pipeline files. */
    static bool exportMLTrainingData(const QVector<QString>& inputFiles, const QString& outputFile, bool excludeRawData = false);

    /** Export training data from a single ML-pipeline file. */
    static bool exportMLTrainingDataSingle(const QString& inputFile, const QString& outputFile, bool excludeRawData = false);

    /** Export training data from all JSON files in a directory. */
    static bool exportMLTrainingDataBatch(const QString& inputDirectory, const QString& outputFile, bool excludeRawData = false);
};
