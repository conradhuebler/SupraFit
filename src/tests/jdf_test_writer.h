/*
 * SupraFit - writer of synthetic JEOL Delta (.jdf) files for the tests
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
 * Writes the layout read by JeolReader (see jeolreader.h): big-endian header of 1360 bytes, parameter
 * section of 64-byte records and the data section, both in the byte order of the endian flag.
 * Only for tests (Claude Generated).
 */

#pragma once

#include <cmath>
#include <complex>
#include <cstring>

#include <Eigen/Dense>

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QtEndian>

namespace JdfTest {

/// one parameter record: text, integer or float (unit code, SI prefix code)
struct Param {
    QString name;
    int type = 2; ///< 0 text, 1 integer, 2 float
    QString text;
    double value = 0;
    int unit = 0;
    int prefix = 0;
};

/// what to write; the defaults are those of a 500 MHz proton FID with a digital filter
struct Spec {
    bool littleEndian = true;
    int dimensions = 1;
    int dataType = 0; ///< 0 = float64, 1 = float32
    int axisType = 3; ///< 3 complex, 1 real
    int axisUnit = 28; ///< 28 s (FID), 26 ppm
    QString title = "synthetic";
    QVector<Param> params;
    Eigen::VectorXcd values; ///< stored points (JEOL convention, i.e. conjugated w.r.t. Fid)
    quint32 extraPoints2 = 1; ///< points of dimension 2 (> 1: a 2D file)
};

inline void put32(QByteArray& b, int offset, quint32 v, bool little)
{
    uchar* p = reinterpret_cast<uchar*>(b.data()) + offset;
    if (little)
        qToLittleEndian<quint32>(v, p);
    else
        qToBigEndian<quint32>(v, p);
}
inline void put64(QByteArray& b, int offset, quint64 v, bool little)
{
    uchar* p = reinterpret_cast<uchar*>(b.data()) + offset;
    if (little)
        qToLittleEndian<quint64>(v, p);
    else
        qToBigEndian<quint64>(v, p);
}
inline void putDouble(QByteArray& b, int offset, double v, bool little)
{
    quint64 bits;
    std::memcpy(&bits, &v, 8);
    put64(b, offset, bits, little);
}
inline void putFloat(QByteArray& b, int offset, float v, bool little)
{
    quint32 bits;
    std::memcpy(&bits, &v, 4);
    put32(b, offset, bits, little);
}
inline void putText(QByteArray& b, int offset, const QString& text, int length, char fill = 0)
{
    const QByteArray t = text.toLatin1().left(length);
    for (int i = 0; i < length; ++i)
        b[offset + i] = i < t.size() ? t[i] : fill;
}

/// the parameters of a FID: frequency [MHz], sweep [Hz], offset [ppm], points, filter (orders "2 54 73", factors "8 2")
inline QVector<Param> fidParams(double frequency, double sweep, double offset, int points, const QString& orders = "2 54 73", const QString& factors = "8 2")
{
    QVector<Param> p;
    p << Param{ "X_DOMAIN", 0, "Proton" };
    p << Param{ "X_FREQ", 2, QString(), frequency * 1e6, 13 };
    p << Param{ "X_SWEEP", 2, QString(), sweep, 13 };
    p << Param{ "X_OFFSET", 2, QString(), offset, 26 };
    p << Param{ "X_POINTS", 1, QString(), double(points) };
    p << Param{ "X_ACQ_DURATION", 2, QString(), points / sweep, 28 };
    p << Param{ "SCANS", 1, QString(), 16 };
    p << Param{ "DIGITAL_FILTER", 0, orders.isEmpty() ? "FALSE" : "TRUE" };
    if (!orders.isEmpty()) {
        p << Param{ "orders", 0, orders };
        p << Param{ "factors", 0, factors };
    }
    p << Param{ "x90", 2, QString(), 6.0, 28, 2 };
    p << Param{ "solvent", 0, "CHLOROFORM-D" };
    p << Param{ "temp_get", 2, QString(), 20.5, 4 };
    return p;
}

/// the bytes of a file
inline QByteArray write(const Spec& s)
{
    const bool le = s.littleEndian;
    const int header = 1360;
    const int paramStart = header;
    const int paramLength = 16 + 64 * s.params.size();
    const int dataStart = ((paramStart + paramLength + 511) / 512) * 512;
    const int sections = s.axisType == 3 ? 2 : 1;
    const int width = s.dataType == 0 ? 8 : 4;
    const int points = int(s.values.size());
    const int dataLength = sections * width * points;
    const int total = dataStart + dataLength;
    QByteArray b(total, '\0');
    std::memcpy(b.data(), "JEOL.NMR", 8);
    b[8] = le ? 1 : 0;
    b[9] = 1;
    b[11] = 2;
    b[12] = char(s.dimensions);
    b[13] = char(0x80);
    b[14] = char((s.dataType << 6) | (s.dimensions == 1 ? 1 : 2));
    b[24] = char(s.axisType);
    b[32] = 0x01;
    b[33] = char(s.axisUnit);
    putText(b, 48, s.title, 124);
    put32(b, 176, quint32(points), false);
    put32(b, 180, s.extraPoints2, false);
    for (int d = 2; d < 8; ++d)
        put32(b, 176 + 4 * d, 1, false);
    put32(b, 240, quint32(points - 1), false);
    putText(b, 808, "Proton", 32);
    put32(b, 1212, quint32(paramStart), false);
    put32(b, 1216, quint32(paramLength), false);
    put32(b, 1284, quint32(dataStart), false);
    put64(b, 1288, quint64(dataLength), false);
    put64(b, 1320, quint64(total), false);
    // parameters
    put32(b, paramStart, 64, le);
    put32(b, paramStart + 4, 0, le);
    put32(b, paramStart + 8, quint32(s.params.size() - 1), le);
    put32(b, paramStart + 12, quint32(paramLength - 16), le);
    for (int i = 0; i < s.params.size(); ++i) {
        const Param& p = s.params[i];
        const int r = paramStart + 16 + 64 * i;
        b[r + 6] = char(((p.prefix & 0xf) << 4) | 1);
        b[r + 7] = char(p.unit);
        if (p.type == 0)
            putText(b, r + 16, p.text, 16, ' ');
        else if (p.type == 1)
            put32(b, r + 16, quint32(qint32(p.value)), le);
        else
            putDouble(b, r + 16, p.value, le);
        put32(b, r + 32, quint32(p.type), le);
        putText(b, r + 36, p.name, 28, ' ');
    }
    for (int k = 0; k < points; ++k) {
        for (int c = 0; c < sections; ++c) {
            const double v = c == 0 ? s.values(k).real() : s.values(k).imag();
            const int offset = dataStart + width * (c * points + k);
            if (width == 8)
                putDouble(b, offset, v, le);
            else
                putFloat(b, offset, float(v), le);
        }
    }
    return b;
}

/// one Lorentzian line of a synthetic FID
struct Line {
    double ppm = 0;
    double amplitude = 1;
    double t2 = 0.5; ///< [s]; FWHM = 1/(π T2) Hz
};

/*! \brief Points of a FID in the Fid convention exp(+2πiνt) with the phase exp(i(ph0 + ph1·ν/sweep)) [degree,
 * pivot = carrier] and complex Gaussian noise of standard deviation noise (deterministic pseudo random numbers
 * from seed).
 * - groupDelay 0: samples at t_j = j/sweep (an analogue FID, the first point at t = 0);
 * - groupDelay > 0: what an ideal linear-phase digital filter delivers: the band-limited signal delayed by
 *   groupDelay points, made in the frequency domain (exact transform of the sampled decay, (1+z)/(2(1-z)) with
 *   z = exp((2πi(ν-ν_k) - 1/T2)/sweep), times exp(-2πi ν_k groupDelay/sweep)) and transformed back, so the points
 *   before t = 0 are the smooth lead-in of a real filter, not a step. */
inline Eigen::VectorXcd fidPoints(const QVector<Line>& lines, double frequency, double sweep, double offset, int points, double groupDelay,
    double ph0 = 0, double ph1 = 0, double noise = 0, quint32 seed = 1)
{
    const double pi = 3.14159265358979323846;
    Eigen::VectorXcd f = Eigen::VectorXcd::Zero(points);
    if (groupDelay > 0) {
        // spectrum on the FFT order of frequencies, then the inverse DFT (direct sum would be O(n²))
        Eigen::VectorXcd spectrum = Eigen::VectorXcd::Zero(points);
        for (const Line& line : lines) {
            const double nu = (line.ppm - offset) * frequency;
            const std::complex<double> phase = std::polar(line.amplitude, (ph0 + ph1 * nu / sweep) * pi / 180.0);
            for (int k = 0; k < points; ++k) {
                const double nuK = (k < points / 2 ? k : k - points) * sweep / points;
                const std::complex<double> z = std::exp(std::complex<double>(-1.0 / (line.t2 * sweep), 2 * pi * (nu - nuK) / sweep));
                spectrum(k) += phase * (1.0 + z) / (2.0 * (1.0 - z)) * std::polar(1.0, -2 * pi * nuK * groupDelay / sweep);
            }
        }
        // inverse transform by the forward radix-2 of the conjugate (points is a power of two in the tests)
        Eigen::VectorXcd c = spectrum.conjugate();
        const int n = points;
        for (int i = 1, j = 0; i < n; ++i) {
            int bit = n >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j |= bit;
            if (i < j)
                std::swap(c(i), c(j));
        }
        for (int length = 2; length <= n; length <<= 1)
            for (int start = 0; start < n; start += length)
                for (int k = 0; k < length / 2; ++k) {
                    const std::complex<double> w = std::polar(1.0, -2 * pi * k / length);
                    const std::complex<double> t = w * c(start + k + length / 2);
                    c(start + k + length / 2) = c(start + k) - t;
                    c(start + k) += t;
                }
        f = c.conjugate() / double(n);
    } else {
        for (const Line& line : lines) {
            const double nu = (line.ppm - offset) * frequency;
            const std::complex<double> phase = std::polar(line.amplitude, (ph0 + ph1 * nu / sweep) * pi / 180.0);
            for (int j = 0; j < points; ++j) {
                const double t = j / sweep;
                f(j) += phase * std::polar(std::exp(-t / line.t2), 2 * pi * nu * t);
            }
        }
    }
    if (noise > 0) {
        quint64 state = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        auto uniform = [&state]() {
            state = state * 6364136223846793005ULL + 1442695040888963407ULL;
            return (double((state >> 11) & ((1ULL << 53) - 1)) + 0.5) / double(1ULL << 53);
        };
        for (int j = 0; j < points; ++j) {
            const double r = std::sqrt(-2 * std::log(uniform())), a = 2 * pi * uniform();
            f(j) += noise * std::complex<double>(r * std::cos(a), r * std::sin(a));
        }
    }
    return f;
}

/// a JEOL FID file of these lines (stored conjugated, as JEOL does)
inline QByteArray fidFile(const QVector<Line>& lines, double frequency, double sweep, double offset, int points, double groupDelay,
    double ph0 = 0, double ph1 = 0, double noise = 0, bool littleEndian = true, const QString& orders = "2 54 73", const QString& factors = "8 2", quint32 seed = 1)
{
    Spec s;
    s.littleEndian = littleEndian;
    s.params = fidParams(frequency, sweep, offset, points, orders, factors);
    s.values = fidPoints(lines, frequency, sweep, offset, points, groupDelay, ph0, ph1, noise, seed).conjugate();
    return write(s);
}

inline bool save(const QString& filename, const QByteArray& bytes)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(bytes) == bytes.size();
}

} // namespace JdfTest
