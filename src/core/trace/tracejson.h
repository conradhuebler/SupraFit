/*
 * TraceJson - lossless JSON persistence of numeric vectors (raw traces)
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

// Claude Generated: WP1 of roadmap/input_data_editing.md

#pragma once

#include <Eigen/Dense>

#include <QtCore/QJsonValue>
#include <QtCore/QString>

/*! \brief Persistence of raw data vectors (x and y of a trace) in project files.
 *
 * Format: one string, values separated by a single blank, each value written as the SHORTEST decimal
 * that reads back to the identical double (QLocale::FloatingPointShortest, at most 17 significant
 * digits). Why a string and not a JSON array of numbers:
 *  - the blocks of spectra/thermograms have always been strings ("x", "y"), so SupraFit versions
 *    before WP1 still read the new files (they split on blanks and call toDouble);
 *  - "shortest" keeps measured values like "0.1234" at their own length: the file grows only where
 *    the data really carry 17 digits (computed or simulated values, about 2.5x the old 6-digit text).
 * Reading accepts that string (any precision, so also the old 6-digit format of
 * ToolSet::DoubleList2String) and, additionally, a JSON array of numbers.
 */
namespace TraceJson {

/*! \brief Writes the vector losslessly (see namespace comment). Claude Generated */
QString vectorToString(const Eigen::VectorXd& vector);

/*! \brief Reads a vector written by vectorToString, by the old ToolSet::DoubleList2String (6 digits)
 * or given as a JSON array of numbers.
 *
 * An empty string gives an empty vector. Any run of white space separates; "nan"/"inf" are read. A token that is not a
 * number (or an array entry that is not a number) is an error: returns false, sets *error and leaves
 * *out untouched (the old ToolSet::String2DoubleEigVec silently turned it into 0). Claude Generated
 */
bool vectorFromJson(const QJsonValue& value, Eigen::VectorXd* out, QString* error = nullptr);

} // namespace TraceJson
