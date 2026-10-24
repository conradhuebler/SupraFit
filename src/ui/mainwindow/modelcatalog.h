/*
 * Model catalogue: which models can be added for which technique and stoichiometry.
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

#include "src/global.h"

/*! \brief What can be added to a workspace, in one place: the techniques (NMR, UV/Vis, fluorescence, calorimetry,
 * kinetics), the stoichiometries and the model each (technique, stoichiometry) pair stands for.
 *
 * The Model menu used to list the same four stoichiometries again under every technique; the Add-Model bar
 * (roadmap/add_model_bar.md) offers ONE list of stoichiometries and a technique next to it. This catalogue is the
 * common source: pure data and rules, no widgets, so it can be tested without a window. A stoichiometry stands for
 * the fixed model of the technique where there is one (1:1, 2:1 / 1:1, 1:1 / 1:2, 2:1 / 1:1 / 1:2) and otherwise
 * for the flexible `*_any` model of the technique with the reaction system as preset. Claude Generated. */
namespace ModelCatalog {

enum class Technique {
    NMR,
    UVVis,
    Fluorescence,
    Calorimetry,
    Kinetics
};

struct TechniqueInfo {
    Technique technique;
    QString key; ///< stable, for settings ("nmr", "uvvis", "fluorescence", "calorimetry", "kinetics")
    QString name; ///< "NMR Titration"
    QString shortName; ///< "NMR"
};

/*! \brief The techniques this build offers, in the order of the Model menu. */
QList<TechniqueInfo> Techniques();
TechniqueInfo Info(Technique technique);
/*! \brief The technique with the settings key \p key, \p fallback if there is none. */
Technique TechniqueFromKey(const QString& key, Technique fallback = Technique::NMR);
/*! \brief The techniques that fit data with \p independentColumns independent columns: one column is calorimetry
 * or kinetics, two or more are host-guest(-...) titrations (NMR, UV/Vis, fluorescence). 0 (unknown): all. */
QList<Technique> TechniquesFor(int independentColumns);

/*! \brief A stoichiometry, independent of the technique. */
struct Stoichiometry {
    QString key; ///< stable ("1:1", "2:1/1:1", "1:1/1:2", "2:1/1:1/1:2", "1:1+A2", ..., "competitive", "ternary")
    QString title; ///< what the user reads ("1:1", "2:1 / 1:1", "1:1, A2")
    QString tooltip; ///< the reactions
    QString reactions; ///< as the reaction editor takes them, one per line
    int components = 2; ///< free components (= independent columns of the data for the flexible models)
    int fixedIndex = -1; ///< 0..3 for the four established stoichiometries that every technique has as fixed model, else -1
};
/*! \brief The stoichiometries in the order of the reaction editor's presets. */
QList<Stoichiometry> Stoichiometries();

/*! \brief What to hand to the code that adds a model. */
struct Choice {
    enum class Kind {
        None,
        FixedModel, ///< `model` is the model id; no definition is needed
        AnyModelWithReactions, ///< `model` is a `*_any` id, `reactions` the preset (the dialog is skipped)
        ScriptPreset, ///< `scriptPreset` indexes ScriptModel::Presets()
        ModelFile ///< `file` is a model file
    };
    Kind kind = Kind::None;
    int model = -1;
    QString reactions;
    int scriptPreset = -1;
    QString file;
    bool valid() const { return kind != Kind::None; }
};

enum class Kind {
    Stoichiometry,
    FixedModel,
    ScriptPreset,
    ModelFile
};

/*! \brief One thing that can be added (and later put in the quick bar). */
struct Entry {
    QString key; ///< stable, for counters and the quick bar: "stoich:1:1", "model:15", "script:Hill", "file:<path>"
    Kind kind = Kind::Stoichiometry;
    QString title;
    QString tooltip;
    Choice choice; ///< for a stoichiometry the choice of the first technique; ChoiceFor() gives it for any
    QList<Technique> techniques; ///< the techniques that offer this entry (empty for scripted presets and files)
};

/*! \brief The fixed models of \p technique for the four established stoichiometries, in the order 1:1, 2:1 / 1:1,
 * 1:1 / 1:2, 2:1 / 1:1 / 1:2 (empty for kinetics). */
QList<int> FixedModels(Technique technique);
/*! \brief The flexible `*_any` model of \p technique, -1 if there is none. */
int AnyModel(Technique technique);

/*! \brief What the pair (\p technique, stoichiometry \p key) stands for; Kind::None if the technique has no model for
 * it (calorimetry takes reaction systems of two components only). */
Choice ForStoichiometry(Technique technique, const QString& key);

/*! \brief Everything \p technique offers for data with \p independentColumns independent columns (0: unknown):
 * its stoichiometries - those whose component count matches the columns - and the further fixed models of the
 * technique (the special calorimetry models, the kinetic models). Empty if the technique does not fit the data. */
QList<Entry> Entries(Technique technique, int independentColumns);

/*! \brief Everything that can be added for data with \p independentColumns independent columns (0: unknown), each
 * entry once: the stoichiometries with all the techniques that offer them (a click on a stoichiometry picks one of
 * these, a menu offers the others), the models of one technique alone (special calorimetry and kinetic models) and the
 * scripted presets. The order is that of the first technique that has an entry. */
QList<Entry> AllEntries(int independentColumns);

/*! \brief What \p entry stands for when added for \p technique (the entry's own choice if it does not depend on one). */
Choice ChoiceFor(const Entry& entry, Technique technique);

/*! \brief The predefined scripted models (Michaelis-Menten, Hill, binding signals ...) that fit data with
 * \p independentColumns columns (0: all); independent of the technique. */
QList<Entry> ScriptPresetEntries(int independentColumns);

/*! \brief An entry for a model file. */
Entry ModelFileEntry(const QString& file);
}
