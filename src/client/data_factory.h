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

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QPointer>

class DataClass;

/**
 * @brief Stateless data-table generation/loading helpers extracted from SupraFitCli (D3).
 *
 * Every method is static and operates purely on its JSON config argument (and DataGenerator /
 * FileHandler); no SupraFit CLI state is touched. The SupraFitCli data-generation orchestrators
 * (GenerateData*, GenerateDataWithModularStructure) delegate the per-table work here. Claude Generated.
 */
class DataFactory {
public:
    /** Validate a DataGenerator config block; true if it can drive generation. */
    static bool validateDataGeneratorConfig(const QJsonObject& config);

    /** Build an independent-variable data table (JSON) from an "Independent" generator config. */
    static QJsonObject generateIndependentDataTable(const QJsonObject& independentConfig);

    /** Load a data table (JSON) from a file per a "Source":"file" config block. */
    static QJsonObject loadDataTableFromFile(const QJsonObject& fileConfig);

    /** Apply configured (e.g. Gaussian) noise to a data table in-place; returns the same pointer. */
    static QPointer<DataClass> applyNoise(QPointer<DataClass> data, const QJsonObject& noiseConfig, bool isIndependent);
};
