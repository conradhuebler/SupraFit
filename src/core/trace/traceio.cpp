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

#include <cmath>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QLocale>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringDecoder>

#include "traceio.h"

namespace {
void setError(QString* error, const QString& message)
{
    if (error)
        *error = message;
}

/// one field as a number; decimalComma: ',' is the decimal separator of this field
bool toNumber(QString field, bool decimalComma, double* value)
{
    field = field.trimmed();
    if (field.size() >= 2 && (field.startsWith('"') || field.startsWith('\'')) && field.endsWith(field.front()))
        field = field.mid(1, field.size() - 2).trimmed();
    if (field.isEmpty())
        return false;
    if (decimalComma)
        field.replace(',', '.');
    bool ok = false;
    const double v = QLocale::c().toDouble(field, &ok);
    if (!ok || !std::isfinite(v))
        return false;
    *value = v;
    return true;
}

/// "LEFT = 12.3 ppm" in the header lines; false if not there
bool headerValue(const QStringList& header, const QString& key, double* value)
{
    const QRegularExpression re(QString("\\b%1\\s*=\\s*([-+]?[0-9]+(?:[.,][0-9]*)?(?:[eE][-+]?[0-9]+)?)").arg(key));
    for (const QString& line : header) {
        const QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch())
            return toNumber(match.captured(1), true, value);
    }
    return false;
}
} // namespace

namespace TraceIO {

bool parseNumberLine(const QString& line, QVector<double>* numbers)
{
    const QString text = line.trimmed();
    if (text.isEmpty())
        return false;
    QStringList fields;
    bool decimalComma = true;
    if (text.contains('\t') || text.contains(';')) {
        fields = text.split(QRegularExpression("[\\t;]"), Qt::KeepEmptyParts);
        // a trailing separator gives an empty last field
        while (!fields.isEmpty() && fields.last().trimmed().isEmpty())
            fields.removeLast();
    } else if (text.contains(QRegularExpression("\\s"))) {
        // blanks, possibly "1.0, 2.0" (comma followed by a blank): then the comma separates
        if (text.contains(QRegularExpression(",\\s")) && text.contains('.')) {
            fields = text.split(',', Qt::SkipEmptyParts);
            decimalComma = false;
        } else
            fields = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    } else if (text.contains(',')) {
        if (text.contains('.') || text.count(',') > 1) {
            fields = text.split(',', Qt::KeepEmptyParts);
            decimalComma = false;
        } else
            fields = QStringList{ text }; // "7,25": one number with a decimal comma
    } else
        fields = QStringList{ text };

    QVector<double> result;
    result.reserve(fields.size());
    for (const QString& field : fields) {
        double v = 0;
        if (!toNumber(field, decimalComma, &v))
            return false;
        result << v;
    }
    if (result.isEmpty())
        return false;
    if (numbers)
        *numbers = result;
    return true;
}

bool readNumericTable(const QString& text, NumericTable* table, QString* error)
{
    QStringList header;
    QVector<QVector<double>> rows;
    QHash<int, int> counts;
    int skipped = 0;
    const QStringList lines = text.split('\n');
    for (QString line : lines) {
        if (line.endsWith('\r'))
            line.chop(1);
        QVector<double> numbers;
        if (parseNumberLine(line, &numbers)) {
            rows << numbers;
            counts[numbers.size()]++;
        } else if (rows.isEmpty())
            header << line;
        else if (!line.trimmed().isEmpty())
            ++skipped;
    }
    if (rows.isEmpty()) {
        setError(error, "no rows of numbers found");
        return false;
    }
    int columns = 0, best = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        // most frequent column count; on a tie the wider table
        if (it.value() > best || (it.value() == best && it.key() > columns)) {
            best = it.value();
            columns = it.key();
        }
    }
    Eigen::MatrixXd data(best, columns);
    int r = 0;
    for (const QVector<double>& row : rows) {
        if (row.size() != columns) {
            ++skipped;
            continue;
        }
        for (int c = 0; c < columns; ++c)
            data(r, c) = row[c];
        ++r;
    }
    if (table) {
        table->data = data;
        table->header = header;
        table->skipped = skipped;
    }
    return true;
}

bool readText(const QString& filename, QString* text, QString* error)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(error, QString("%1: %2").arg(filename, file.errorString()));
        return false;
    }
    const QByteArray blob = file.readAll();
    QStringDecoder decoder(QStringDecoder::Utf8);
    QString result = decoder(blob);
    if (decoder.hasError())
        result = QString::fromLatin1(blob);
    if (result.startsWith(QChar(0xFEFF)))
        result.remove(0, 1);
    if (text)
        *text = result;
    return true;
}

bool tracesFromTable(const NumericTable& table, const QString& name, QVector<XYTrace>* traces, QString* error)
{
    QVector<XYTrace> result;
    const Eigen::Index rows = table.data.rows(), cols = table.data.cols();
    if (rows < 2) {
        setError(error, QString("%1: fewer than two data points").arg(name));
        return false;
    }
    if (cols == 1) {
        double left = 0, right = 0;
        if (!headerValue(table.header, "LEFT", &left) || !headerValue(table.header, "RIGHT", &right)) {
            setError(error, QString("%1: one column of numbers without an axis (LEFT/RIGHT in the header)").arg(name));
            return false;
        }
        Eigen::VectorXd x(rows);
        for (Eigen::Index i = 0; i < rows; ++i)
            x(i) = left + (right - left) * double(i) / double(rows - 1);
        result << XYTrace(x, table.data.col(0), name);
    } else {
        const Eigen::VectorXd x = table.data.col(0);
        for (Eigen::Index c = 1; c < cols; ++c)
            result << XYTrace(x, table.data.col(c), cols == 2 ? name : QString("%1 [%2]").arg(name).arg(c));
    }
    for (const XYTrace& trace : result) {
        QString reason;
        if (!trace.isValid(&reason) || !trace.isMonotone()) {
            setError(error, QString("%1: %2").arg(name, reason.isEmpty() ? QString("the x axis is not strictly monotone") : reason));
            return false;
        }
    }
    if (traces)
        *traces = result;
    return true;
}

bool readTraceFile(const QString& filename, QVector<XYTrace>* traces, QString* error)
{
    QString text;
    if (!readText(filename, &text, error))
        return false;
    NumericTable table;
    const QString name = QFileInfo(filename).fileName();
    QString reason;
    if (!readNumericTable(text, &table, &reason)) {
        setError(error, QString("%1: %2").arg(name, reason));
        return false;
    }
    QVector<XYTrace> result;
    if (!tracesFromTable(table, name, &result, error))
        return false;
    for (XYTrace& trace : result)
        trace.setSource(filename);
    if (traces)
        *traces = result;
    return true;
}

} // namespace TraceIO
