/*
 * Model usage: how often each model was added, and what the quick bar shows because of it.
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
 */

#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "modelcatalog.h"

/*! \brief What the Add-Model bar offers first: the entries the operator adds most, or a list the operator put
 * together. Pure logic over ModelCatalog and QSettings (group "add_model_bar"), no widgets.
 *
 * Counting: every model that is added bumps a counter of its entry. A stoichiometry or a technique's own model is
 * counted per technique (1:1 for NMR and 1:1 for UV/Vis are different habits), a scripted preset or a model file for
 * all techniques together. An entry (1:1) is ranked by all its uses; the bar shows the `count` most used entries,
 * entries that were never used follow in the order of the catalogue, so a fresh installation shows the four
 * established stoichiometries. A click on an entry uses the technique it was used with most (NMR, a fixed technique or
 * the one used last while it never was used); the bar offers the other techniques in a menu. In the fixed mode the bar
 * shows exactly the entries of a stored list. Claude Generated. */
namespace ModelUsage {

enum class Mode {
    Frequent, ///< the `count` most used entries
    Fixed ///< the entries of `fixed`, in that order
};

struct Preferences {
    Mode mode = Mode::Frequent;
    int count = 4; ///< entries shown in the frequent mode
    QStringList fixed; ///< entry keys of the fixed mode; a key of a stoichiometry means it for the technique on screen
    QString technique; ///< technique key to start with; empty: the one used last
};

Preferences Load();
void Save(const Preferences& preferences);
/*! \brief The fixed list a fresh installation starts from: the four established stoichiometries. */
QStringList DefaultFixed();

/*! \brief A model of \p entry was added for \p technique: count it. Stoichiometries and the models of a technique
 * also make \p technique the one used last; scripted presets and model files do not. */
void Record(ModelCatalog::Technique technique, const ModelCatalog::Entry& entry);
int Used(ModelCatalog::Technique technique, const QString& entryKey);
/*! \brief All uses of an entry, whatever the technique. */
int UsedTotal(const QString& entryKey);
/*! \brief Forget all counts and the technique used last. */
void Reset();

/*! \brief The technique to start with for data with \p independentColumns columns: the fixed one or the one used
 * last, if it fits the data, else NMR if it fits, else the first that does. */
ModelCatalog::Technique DefaultTechnique(int independentColumns);

/*! \brief The technique a click on \p entry uses: the one the entry was used with most; if it never was used, the
 * fixed technique, else the one used last, else NMR, each if the entry is offered for it, else the first that offers
 * it. */
ModelCatalog::Technique TechniqueFor(const ModelCatalog::Entry& entry, int independentColumns);

/*! \brief Everything that can be added for the data: the catalogue's entries and the model files that have been used
 * (and still exist). */
QList<ModelCatalog::Entry> Candidates(int independentColumns);
/*! \brief Candidates, the most used first; equal counts stay in the order of the catalogue. */
QList<ModelCatalog::Entry> Ranked(int independentColumns);
/*! \brief What the quick bar shows, by the preferences. */
QList<ModelCatalog::Entry> Quick(int independentColumns);
}
