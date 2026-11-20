/*
 * JeolReader - reading JEOL Delta NMR data files (.jdf)
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

// Claude Generated: JEOL import of roadmap/input_data_editing.md (findings: docs/jeol_import_2026-11-20.md)

#pragma once

#include <complex>
#include <limits>

#include <Eigen/Dense>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QVector>

/*! \brief Reader of JEOL Delta data files (.jdf), one-dimensional data only (Claude Generated).
 *
 * File layout (all offsets in bytes from the start of the file):
 * - Header, 1360 bytes, ALWAYS big-endian: magic "JEOL.NMR" (0), endian flag (8: 0 = big, 1 = little
 *   endian; it applies to the parameter and data sections, not to the header), version (9, 10-11),
 *   number of dimensions (12), data type/format (14: upper 2 bits 0 = 64-bit float, 1 = 32-bit float;
 *   lower 6 bits 1 = one-dimensional), axis type per dimension (24: 1 real, 3 complex), axis unit per
 *   dimension (32, two bytes: SI prefix and power, unit code: 28 = second, 13 = hertz, 26 = ppm),
 *   title (48, 124 bytes), points per dimension (176, uint32), first/last valid point (208/240),
 *   axis start/stop (272/336, double), axis titles (808, 32 bytes each), base frequency per dimension
 *   [MHz] (1064, double), parameter section start/length (1212/1216), data section start (1284) and
 *   length (1288, uint64), context (experiment source text, 1296/1304), total file size (1320, uint64).
 * - Parameter section: 16-byte header (record size = 64, lowest and highest index, total size), then
 *   records of 64 bytes: 4 class bytes, unit scaler (2), five unit entries (2 bytes each), value
 *   (16 bytes: text, int32, double or two doubles), value type (4: 0 text, 1 integer, 2 float,
 *   3 complex, 4 infinity), name (28 bytes, blank padded).
 * - Data section of a 1D file: all real parts, then (complex data) all imaginary parts, each
 *   'points' values of the data type in the byte order of the endian flag.
 *
 * Everything read from the file is checked against the file size before it is used; a header value
 * never sizes an allocation on its own (the data section must lie inside the file). Multidimensional
 * data, other data formats and value types are refused with "not supported".
 */
namespace JeolReader {

/*! \brief Largest file read (bytes); larger files are refused before they are read. */
constexpr qint64 MaxFileSize = qint64(512) * 1024 * 1024;

/*! \brief Size of the fixed header (bytes). */
constexpr int HeaderSize = 1360;

/*! \brief One parameter of the parameter section. */
struct Parameter {
    enum class Type { Text = 0,
        Integer = 1,
        Float = 2,
        Complex = 3,
        Infinity = 4 };
    QString name; ///< as stored, trimmed (e.g. "X_FREQ", "solvent")
    Type type = Type::Text;
    QString text; ///< value of a text parameter (trimmed)
    double value = std::numeric_limits<double>::quiet_NaN(); ///< numeric value in the stored unit incl. its prefix (6 for "6 µs")
    double imaginary = 0; ///< imaginary part of a complex value
    int prefix = 0; ///< SI prefix code of the first unit entry (signed nibble: 2 = micro, -1 = kilo)
    int unit = 0; ///< unit code of the first unit entry (13 Hz, 26 ppm, 28 s, 4 °C, 6 degree)
    /*! \brief value() scaled by the SI prefix: 6 µs -> 6e-6 (s). */
    double siValue() const;
    /*! \brief Value with unit for display ("6 µs", "CHLOROFORM-D", "65536"). */
    QString display() const;
};

/*! \brief Contents of a 1D JEOL file. */
struct Data {
    QString title; ///< header title (sample name)
    QString source; ///< file name
    bool littleEndian = false; ///< byte order of parameter and data sections
    int majorVersion = 0, minorVersion = 0;
    int dataType = 0; ///< 0 = 64-bit float, 1 = 32-bit float
    int axisType = 0; ///< 1 = real, 3 = complex
    int axisUnit = 0; ///< unit code of the data axis: 28 = s (time domain, FID), 13 = Hz, 26 = ppm
    int axisPrefix = 0;
    qint64 points = 0; ///< number of stored points
    qint64 firstPoint = 0, lastPoint = 0; ///< valid range of the stored points
    double axisStart = 0, axisStop = 0; ///< axis values of the first and last point (seconds for a FID)
    QString axisTitle; ///< nucleus of the axis ("Proton")
    double baseFrequency = 0; ///< header base frequency [MHz]
    double zeroPoint = 0; ///< header zero point of the axis

    // ---- acquisition parameters (NaN or empty if the parameter is not in the file)
    QString nucleus; ///< X_DOMAIN
    double frequency = std::numeric_limits<double>::quiet_NaN(); ///< X_FREQ, spectrometer frequency of the nucleus [MHz]
    double sweep = std::numeric_limits<double>::quiet_NaN(); ///< X_SWEEP, spectral width [Hz]
    double offset = std::numeric_limits<double>::quiet_NaN(); ///< X_OFFSET, carrier position [ppm]
    double acquisitionTime = std::numeric_limits<double>::quiet_NaN(); ///< X_ACQ_DURATION [s]
    double acquisitionDelay = std::numeric_limits<double>::quiet_NaN(); ///< ACQ_DELAY [s]
    double temperature = std::numeric_limits<double>::quiet_NaN(); ///< temp_get [°C]
    int scans = 0; ///< SCANS
    QString solvent; ///< solvent
    QString instrument; ///< inst_model_number
    QString experiment; ///< experiment
    bool digitalFilter = false; ///< DIGITAL_FILTER = TRUE
    QVector<int> filterOrders; ///< orders: number of stages, then the order of each stage
    QVector<int> filterFactors; ///< factors: decimation factor of each stage
    /*! \brief Group delay of the digital filter [points]: ½ Σ_i (order_i - 1) / Π_{j≥i} factor_j,
     * the delay of a cascade of linear-phase FIR stages expressed in output points; NaN if the file has
     * no complete filter description or no digital filter. */
    double groupDelay = std::numeric_limits<double>::quiet_NaN();

    QVector<Parameter> parameters; ///< all records in file order
    Eigen::VectorXcd values; ///< valid points (firstPoint..lastPoint); imaginary part 0 for real data

    /*! \brief True for time-domain data (axis unit seconds): a FID. */
    inline bool isTimeDomain() const { return axisUnit == 28; }
    /*! \brief Parameter of this name (case-sensitive; JEOL has both "X_ACQ_TIME"-like and lower case names), nullptr if none. */
    const Parameter* parameter(const QString& name) const;
};

/*! \brief Reads a JEOL file from memory; false with *error (no partial result) for a wrong magic,
 * an offset or length outside the data, a truncated file, or an unsupported layout. Claude Generated */
bool readBytes(const QByteArray& bytes, Data* data, QString* error = nullptr, const QString& name = QString());

/*! \brief Reads the file (refused above MaxFileSize); see readBytes. Claude Generated */
bool readFile(const QString& filename, Data* data, QString* error = nullptr);

/*! \brief True if the bytes start with the magic "JEOL.NMR". */
bool hasMagic(const QByteArray& bytes);

/*! \brief True if the file starts with the magic "JEOL.NMR" (reads 8 bytes). */
bool isJeolFile(const QString& filename);

/*! \brief Group delay [points] of a cascade of FIR stages: ½ Σ_i (orders[i] - 1) / Π_{j≥i} factors[j].
 * orders and factors have one entry per stage (the leading stage count of the file's "orders" removed);
 * NaN for empty or inconsistent lists. Claude Generated */
double firFilterGroupDelay(const QVector<int>& orders, const QVector<int>& factors);

} // namespace JeolReader
