/*
 * TraceIO - reading x-y traces (spectra) from ASCII/CSV exports
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

// Claude Generated: WP3 of roadmap/input_data_editing.md

#pragma once

#include <Eigen/Dense>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include "xytrace.h"

/*! \brief Readers of text exports of spectra (Claude Generated).
 *
 * ToolSet::LoadXYFile (blank separated only, '#' anywhere drops the line) and ToolSet::LoadCSVFile
 * (only the ';'-separated XYDATA block of one instrument) do not read the exports of NMR programs, so
 * these readers accept:
 * - UTF-8 with or without byte order mark, LF or CRLF line ends;
 * - comment and header lines: every line that is not a row of numbers (e.g. "# ...", "ppm,intensity");
 * - fields separated by tab, ';', blanks or ',';
 * - decimal comma ("7,25") where the field separator is not a comma;
 * - a descending axis (ppm), kept as it is.
 *
 * A line of numbers is split at tab or ';' if it has one, else at blanks, else at commas. With tab, ';'
 * or blanks as separator a comma inside a field is a decimal comma. A line with commas only is a
 * comma-separated row if it has a '.' or more than one comma ("7.25,1.0e3", "1,2,3"); a single comma
 * without '.' is a decimal comma ("7,25" = one number).
 */
namespace TraceIO {

/*! \brief Numbers of a text table. */
struct NumericTable {
    Eigen::MatrixXd data; ///< rows of numbers that have the most frequent number of columns
    QStringList header; ///< lines before the first row of numbers (comments, column titles)
    int skipped = 0; ///< lines after the header that were not used (other column count, text)
};

/*! \brief Parses one line into numbers; false if a field is not a number (or the line is empty). */
bool parseNumberLine(const QString& line, QVector<double>* numbers);

/*! \brief Rows of numbers of text; false with *error if there is none. */
bool readNumericTable(const QString& text, NumericTable* table, QString* error = nullptr);

/*! \brief Text of a file (UTF-8, byte order mark removed; Latin-1 if it is not valid UTF-8). */
bool readText(const QString& filename, QString* text, QString* error = nullptr);

/*! \brief Traces of a text table.
 *
 * - two or more columns: column 0 is x, every further column one trace (a single spectrum is the usual
 *   two-column export, several columns a matrix of spectra over one axis);
 * - one column: intensities of a TopSpin "totxt" export, the axis from the header values
 *   "LEFT = a ppm" ... "RIGHT = b ppm" (first point at LEFT, last at RIGHT); without them an error.
 * The traces are named name (one trace) or "name [k]" (column k); their x axis must be strictly monotone.
 */
bool tracesFromTable(const NumericTable& table, const QString& name, QVector<XYTrace>* traces, QString* error = nullptr);

/*! \brief readText + readNumericTable + tracesFromTable; source of each trace = filename. */
bool readTraceFile(const QString& filename, QVector<XYTrace>* traces, QString* error = nullptr);

} // namespace TraceIO
