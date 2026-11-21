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

#include <cmath>
#include <cstring>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QtEndian>

#include "jeolreader.h"

namespace {
const char kMagic[] = "JEOL.NMR";

void setError(QString* error, const QString& message)
{
    if (error)
        *error = message;
}

/// bounds-checked reader of the bytes of one file
class Bytes {
public:
    explicit Bytes(const QByteArray& bytes)
        : m_data(reinterpret_cast<const uchar*>(bytes.constData()))
        , m_size(bytes.size())
    {
    }
    inline qint64 size() const { return m_size; }
    /// true if [offset, offset + length) lies inside the data (no overflow)
    inline bool contains(qint64 offset, qint64 length) const { return offset >= 0 && length >= 0 && offset <= m_size && length <= m_size - offset; }
    inline uchar u8(qint64 offset) const { return m_data[offset]; }
    quint32 u32(qint64 offset, bool little) const { return little ? qFromLittleEndian<quint32>(m_data + offset) : qFromBigEndian<quint32>(m_data + offset); }
    qint32 i32(qint64 offset, bool little) const { return qint32(u32(offset, little)); }
    quint16 u16(qint64 offset, bool little) const { return little ? qFromLittleEndian<quint16>(m_data + offset) : qFromBigEndian<quint16>(m_data + offset); }
    quint64 u64(qint64 offset, bool little) const { return little ? qFromLittleEndian<quint64>(m_data + offset) : qFromBigEndian<quint64>(m_data + offset); }
    double f64(qint64 offset, bool little) const
    {
        const quint64 bits = u64(offset, little);
        double value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
    float f32(qint64 offset, bool little) const
    {
        const quint32 bits = u32(offset, little);
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
    /// text field: Latin-1 up to the first NUL, trimmed
    QString text(qint64 offset, int length) const
    {
        int n = 0;
        while (n < length && m_data[offset + n] != 0)
            ++n;
        return QString::fromLatin1(reinterpret_cast<const char*>(m_data + offset), n).trimmed();
    }

private:
    const uchar* m_data;
    qint64 m_size;
};

/// signed 4-bit value of a nibble
int signedNibble(int nibble)
{
    return nibble >= 8 ? nibble - 16 : nibble;
}

QString unitName(int unit)
{
    switch (unit) {
    case 4:
        return QStringLiteral("°C");
    case 6:
        return QStringLiteral("°");
    case 13:
        return QStringLiteral("Hz");
    case 14:
        return QStringLiteral("K");
    case 24:
        return QStringLiteral("%");
    case 26:
        return QStringLiteral("ppm");
    case 28:
        return QStringLiteral("s");
    case 31:
        return QStringLiteral("T");
    case 35:
        return QStringLiteral("dB");
    default:
        return QString();
    }
}

QString prefixName(int prefix)
{
    switch (prefix) {
    case -3:
        return QStringLiteral("G");
    case -2:
        return QStringLiteral("M");
    case -1:
        return QStringLiteral("k");
    case 1:
        return QStringLiteral("m");
    case 2:
        return QStringLiteral("µ");
    case 3:
        return QStringLiteral("n");
    case 4:
        return QStringLiteral("p");
    default:
        return QString();
    }
}

/// numbers of a blank-separated text parameter ("2 54 73"); false if a field is not an integer
bool integerList(const QString& text, QVector<int>* list)
{
    list->clear();
    const QStringList fields = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    for (const QString& field : fields) {
        bool ok = false;
        const int v = field.toInt(&ok);
        if (!ok)
            return false;
        *list << v;
    }
    return !list->isEmpty();
}
} // namespace

namespace JeolReader {

double Parameter::siValue() const
{
    return value * std::pow(10.0, -3.0 * prefix);
}

QString Parameter::display() const
{
    switch (type) {
    case Type::Text:
        return text;
    case Type::Integer:
    case Type::Float: {
        const QString unitText = prefixName(prefix) + unitName(unit);
        const QString number = type == Type::Integer ? QString::number(qint64(value)) : QString::number(value, 'g', 10);
        return unitText.isEmpty() ? number : QString("%1 %2").arg(number, unitText);
    }
    case Type::Complex:
        return QString("(%1, %2)").arg(value, 0, 'g', 10).arg(imaginary, 0, 'g', 10);
    case Type::Infinity:
        return QStringLiteral("infinity");
    }
    return QString();
}

const Parameter* Data::parameter(const QString& name) const
{
    for (const Parameter& p : parameters)
        if (p.name == name)
            return &p;
    return nullptr;
}

double firFilterGroupDelay(const QVector<int>& orders, const QVector<int>& factors)
{
    if (orders.isEmpty() || orders.size() != factors.size())
        return std::numeric_limits<double>::quiet_NaN();
    double delay = 0;
    for (int i = 0; i < orders.size(); ++i) {
        double product = 1;
        for (int j = i; j < factors.size(); ++j) {
            if (factors[j] < 1)
                return std::numeric_limits<double>::quiet_NaN();
            product *= factors[j];
        }
        if (orders[i] < 1)
            return std::numeric_limits<double>::quiet_NaN();
        delay += 0.5 * double(orders[i] - 1) / product;
    }
    return delay;
}

bool hasMagic(const QByteArray& bytes)
{
    return bytes.size() >= 8 && std::memcmp(bytes.constData(), kMagic, 8) == 0;
}

bool isJeolFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    return hasMagic(file.read(8));
}

bool readBytes(const QByteArray& blob, Data* out, QString* error, const QString& name)
{
    const QString label = name.isEmpty() ? QStringLiteral("JEOL file") : name;
    auto fail = [&](const QString& reason) {
        setError(error, QString("%1: %2").arg(label, reason));
        return false;
    };
    const Bytes b(blob);
    if (!hasMagic(blob))
        return fail(QStringLiteral("not a JEOL Delta file (no \"JEOL.NMR\" at the start)"));
    if (b.size() < HeaderSize)
        return fail(QString("file truncated: %1 bytes, the header alone has %2").arg(b.size()).arg(HeaderSize));

    Data data;
    data.source = name;
    // the header is big-endian whatever the flag says; the flag is the order of parameters and data
    const uchar endian = b.u8(8);
    if (endian > 1)
        return fail(QString("unknown byte order flag %1").arg(endian));
    data.littleEndian = endian == 1;
    data.majorVersion = b.u8(9);
    data.minorVersion = b.u16(10, false);
    const int dimensions = b.u8(12);
    const int typeByte = b.u8(14);
    data.dataType = typeByte >> 6;
    const int format = typeByte & 0x3f;
    data.axisType = b.u8(24);
    data.axisPrefix = signedNibble(b.u8(32) >> 4);
    data.axisUnit = b.u8(33);
    data.title = b.text(48, 124);
    if (dimensions != 1 || format != 1)
        return fail(QString("%1-dimensional data (format %2) are not supported, only 1D spectra or FIDs").arg(dimensions).arg(format));
    if (data.dataType != 0 && data.dataType != 1)
        return fail(QString("data type %1 is not supported (only 64- and 32-bit floating point)").arg(data.dataType));
    if (data.axisType != 1 && data.axisType != 3)
        return fail(QString("axis type %1 is not supported (only real and complex data)").arg(data.axisType));

    const qint64 points = b.u32(176, false);
    for (int d = 1; d < 8; ++d) {
        const quint32 extra = b.u32(176 + 4 * d, false);
        if (extra > 1)
            return fail(QString("%1 points in dimension %2: multidimensional data are not supported").arg(extra).arg(d + 1));
    }
    data.points = points;
    data.firstPoint = b.u32(208, false);
    data.lastPoint = b.u32(240, false);
    data.axisStart = b.f64(272, false);
    data.axisStop = b.f64(336, false);
    data.axisTitle = b.text(808, 32);
    data.baseFrequency = b.f64(1064, false);
    data.zeroPoint = b.f64(1128, false);
    if (points < 2)
        return fail(QString("%1 data point(s)").arg(points));
    if (data.firstPoint > data.lastPoint || data.lastPoint >= points)
        return fail(QString("valid points %1..%2 outside the %3 stored points").arg(data.firstPoint).arg(data.lastPoint).arg(points));

    const quint64 totalSize = b.u64(1320, false);
    if (totalSize > quint64(b.size()))
        return fail(QString("file truncated: %1 bytes of %2").arg(b.size()).arg(totalSize));

    // ---- parameter section
    const qint64 paramStart = b.u32(1212, false);
    const qint64 paramLength = b.u32(1216, false);
    if (paramStart < HeaderSize || paramLength < 16 || !b.contains(paramStart, paramLength))
        return fail(QString("parameter section %1 + %2 bytes outside the file of %3 bytes").arg(paramStart).arg(paramLength).arg(b.size()));
    const bool le = data.littleEndian;
    const quint32 recordSize = b.u32(paramStart, le);
    const qint64 low = b.u32(paramStart + 4, le), high = b.u32(paramStart + 8, le);
    if (recordSize != 64)
        return fail(QString("parameter records of %1 bytes are not supported (64 expected)").arg(recordSize));
    if (high < low || 16 + 64 * (high - low + 1) > paramLength)
        return fail(QString("parameter index %1..%2 does not fit the parameter section of %3 bytes").arg(low).arg(high).arg(paramLength));
    for (qint64 i = 0; i <= high - low; ++i) {
        const qint64 r = paramStart + 16 + 64 * i;
        Parameter p;
        p.name = b.text(r + 36, 28);
        if (p.name.isEmpty())
            continue;
        p.prefix = signedNibble(b.u8(r + 6) >> 4);
        p.unit = b.u8(r + 7);
        const quint32 type = b.u32(r + 32, le);
        switch (type) {
        case 0:
            p.type = Parameter::Type::Text;
            p.text = b.text(r + 16, 16);
            break;
        case 1:
            p.type = Parameter::Type::Integer;
            p.value = b.i32(r + 16, le);
            break;
        case 2:
            p.type = Parameter::Type::Float;
            p.value = b.f64(r + 16, le);
            break;
        case 3:
            p.type = Parameter::Type::Complex;
            p.value = b.f64(r + 16, le);
            p.imaginary = b.f64(r + 24, le);
            break;
        case 4:
            p.type = Parameter::Type::Infinity;
            p.value = std::numeric_limits<double>::infinity();
            break;
        default:
            p.type = Parameter::Type::Text; // unknown value type: kept for display, no value
            p.text = QString("(value type %1)").arg(type);
            break;
        }
        data.parameters << p;
    }

    // acquisition parameters used for processing
    auto number = [&data](const QString& key) {
        const Parameter* p = data.parameter(key);
        return p && (p->type == Parameter::Type::Integer || p->type == Parameter::Type::Float) ? p->siValue() : std::numeric_limits<double>::quiet_NaN();
    };
    auto text = [&data](const QString& key) {
        const Parameter* p = data.parameter(key);
        return p && p->type == Parameter::Type::Text ? p->text : QString();
    };
    data.nucleus = text("X_DOMAIN");
    if (data.nucleus.isEmpty())
        data.nucleus = data.axisTitle;
    data.frequency = number("X_FREQ") / 1e6;
    if (!std::isfinite(data.frequency) || data.frequency <= 0)
        data.frequency = data.baseFrequency;
    data.sweep = number("X_SWEEP");
    {
        // X_OFFSET is stored in ppm (unit 26); a value in Hz is converted with the frequency
        const Parameter* p = data.parameter("X_OFFSET");
        if (p && (p->type == Parameter::Type::Float || p->type == Parameter::Type::Integer))
            data.offset = p->unit == 13 ? p->siValue() / data.frequency : p->value;
    }
    data.acquisitionTime = number("X_ACQ_DURATION");
    data.acquisitionDelay = number("ACQ_DELAY");
    data.temperature = number("temp_get");
    data.scans = std::isfinite(number("SCANS")) ? int(number("SCANS")) : 0;
    data.solvent = text("solvent");
    data.instrument = text("inst_model_number");
    data.experiment = text("experiment");
    data.digitalFilter = text("DIGITAL_FILTER").compare("TRUE", Qt::CaseInsensitive) == 0;
    {
        QVector<int> orders, factors;
        if (integerList(text("orders"), &orders) && integerList(text("factors"), &factors) && !orders.isEmpty() && orders.first() == orders.size() - 1) {
            data.filterOrders = orders;
            data.filterFactors = factors;
            if (data.digitalFilter)
                data.groupDelay = firFilterGroupDelay(orders.mid(1), factors);
        }
    }

    // ---- data section
    const qint64 dataStart = b.u32(1284, false);
    const quint64 dataLength = b.u64(1288, false);
    const int sections = data.axisType == 3 ? 2 : 1;
    const int width = data.dataType == 0 ? 8 : 4;
    if (dataStart < HeaderSize || dataLength > quint64(b.size()) || !b.contains(dataStart, qint64(dataLength)))
        return fail(QString("data section %1 + %2 bytes outside the file of %3 bytes").arg(dataStart).arg(dataLength).arg(b.size()));
    // points < 2^32 and sections * width <= 16: no overflow in 64 bits
    const qint64 needed = qint64(sections) * width * points;
    if (needed > qint64(dataLength))
        return fail(QString("data section of %1 bytes holds fewer than the %2 x %3 points of the header").arg(dataLength).arg(sections).arg(points));
    const qint64 count = data.lastPoint - data.firstPoint + 1;
    data.values.resize(count);
    for (qint64 i = 0; i < count; ++i) {
        const qint64 k = data.firstPoint + i;
        const qint64 re = dataStart + width * k;
        const qint64 im = dataStart + width * (points + k);
        const double real = width == 8 ? b.f64(re, le) : double(b.f32(re, le));
        const double imag = sections == 2 ? (width == 8 ? b.f64(im, le) : double(b.f32(im, le))) : 0.0;
        if (!std::isfinite(real) || !std::isfinite(imag))
            return fail(QString("point %1 is not a finite number").arg(k + 1));
        data.values(i) = std::complex<double>(real, imag);
    }
    if (out)
        *out = data;
    return true;
}

bool readFile(const QString& filename, Data* data, QString* error)
{
    QFile file(filename);
    const QString name = QFileInfo(filename).fileName();
    if (!file.open(QIODevice::ReadOnly)) {
        setError(error, QString("%1: %2").arg(filename, file.errorString()));
        return false;
    }
    if (file.size() > MaxFileSize) {
        setError(error, QString("%1: %2 bytes, larger than the %3 bytes read at most").arg(name).arg(file.size()).arg(MaxFileSize));
        return false;
    }
    const QByteArray blob = file.readAll();
    if (!readBytes(blob, data, error, name))
        return false;
    if (data)
        data->source = filename;
    return true;
}

} // namespace JeolReader
