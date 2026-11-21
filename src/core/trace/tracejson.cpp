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

#include <QtCore/QJsonArray>
#include <QtCore/QLocale>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>

#include <cmath>
#include <vector>

#include "tracejson.h"

namespace TraceJson {

QString vectorToString(const Eigen::VectorXd& vector)
{
    QStringList parts;
    parts.reserve(vector.size());
    for (Eigen::Index i = 0; i < vector.size(); ++i) {
        const double v = vector(i);
        // QString::number drops the sign of -0.0; written explicitly so the bits survive
        parts << ((v == 0 && std::signbit(v)) ? QStringLiteral("-0") : QString::number(v, 'g', QLocale::FloatingPointShortest));
    }
    return parts.join(QLatin1Char(' '));
}

bool vectorFromJson(const QJsonValue& value, Eigen::VectorXd* out, QString* error)
{
    std::vector<double> values;
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        values.reserve(array.size());
        for (int i = 0; i < array.size(); ++i) {
            if (!array[i].isDouble()) {
                if (error)
                    *error = QString("entry %1 of the array is not a number").arg(i);
                return false;
            }
            values.push_back(array[i].toDouble());
        }
    } else if (value.isString() || value.isUndefined() || value.isNull()) {
        // a missing key reads as an empty vector, as the old String2DoubleEigVec(QString()) did
        static const QRegularExpression whitespace(QStringLiteral("\\s+"));
        const QStringList tokens = value.toString().split(whitespace, Qt::SkipEmptyParts);
        values.reserve(tokens.size());
        for (const QString& token : tokens) {
            bool ok = false;
            const double d = token.trimmed().toDouble(&ok);
            if (!ok) {
                if (error)
                    *error = QString("'%1' is not a number").arg(token.left(32));
                return false;
            }
            values.push_back(d);
        }
    } else {
        if (error)
            *error = QString("expected a string or an array of numbers");
        return false;
    }
    if (out)
        *out = Eigen::Map<const Eigen::VectorXd>(values.data(), Eigen::Index(values.size()));
    return true;
}

} // namespace TraceJson
