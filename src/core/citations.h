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

#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>

/**
 * @brief Small registry of literature references, keyed by a short method id.
 *
 * A model reports which methods it actually used (AbstractModel::CitationKeys), and the results view
 * asks Citation to format a "Please cite" block. The SupraFit reference is always implied; extra
 * keys (e.g. "musketeer" when the BFGS speciation solver ran) are appended so users cite the methods
 * behind their numbers. Claude Generated.
 */
namespace Citation {

struct Reference {
    QString key; ///< short id, e.g. "suprafit", "musketeer"
    QString text; ///< plain-text citation
    QString doi; ///< bare DOI (may be empty)
};

/** @brief Look up a reference by key; unknown keys return an empty Reference. */
Reference Get(const QString& key);

/** @brief Plain-text "Please cite" block for @p keys (unknown keys skipped); empty if none. */
QString FormatPlain(const QStringList& keys);

/** @brief HTML "Please cite" block for @p keys with DOI links; empty if none. */
QString FormatHtml(const QStringList& keys);

} // namespace Citation
