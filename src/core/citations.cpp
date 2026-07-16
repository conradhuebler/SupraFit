/*
 * SupraFit - runtime citation hints for the methods a model actually uses
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

#include <QtCore/QVector>

#include "citations.h"

namespace Citation {

// Known references. "suprafit" is the base citation; method-specific keys are added by the models
// (e.g. "musketeer" whenever the BFGS equilibrium speciation solver was used). Claude Generated.
static const QVector<Reference>& registry()
{
    static const QVector<Reference> refs = {
        { QStringLiteral("suprafit"),
            QStringLiteral("C. Hübler, SupraFit - An Open Source Qt Based Fitting Application, "
                           "Chem. Methods 2022, 2, e202200006."),
            QStringLiteral("10.1002/cmtd.202200006") },
        { QStringLiteral("musketeer"),
            QStringLiteral("D. O. Soloviev, C. A. Hunter, Musketeer: a software tool for the analysis "
                           "of titration data, Chem. Sci. 2024, 15, 15299-15310."),
            QStringLiteral("10.1039/d4sc03354j") },
    };
    return refs;
}

Reference Get(const QString& key)
{
    for (const Reference& ref : registry()) {
        if (ref.key == key)
            return ref;
    }
    return Reference{};
}

QString FormatPlain(const QStringList& keys)
{
    QStringList lines;
    for (const QString& key : keys) {
        const Reference ref = Get(key);
        if (ref.key.isEmpty())
            continue;
        QString line = ref.text;
        if (!ref.doi.isEmpty())
            line += QStringLiteral(" DOI: %1").arg(ref.doi);
        lines << line;
    }
    if (lines.isEmpty())
        return QString();
    return QStringLiteral("Please cite:\n  ") + lines.join(QStringLiteral("\n  "));
}

QString FormatHtml(const QStringList& keys)
{
    QString body;
    for (const QString& key : keys) {
        const Reference ref = Get(key);
        if (ref.key.isEmpty())
            continue;
        body += QStringLiteral("<p>%1").arg(ref.text);
        if (!ref.doi.isEmpty())
            body += QStringLiteral(" <a href=\"https://doi.org/%1\">%1</a>").arg(ref.doi);
        body += QStringLiteral("</p>");
    }
    if (body.isEmpty())
        return QString();
    return QStringLiteral("<h4>Please cite</h4>") + body;
}

} // namespace Citation
