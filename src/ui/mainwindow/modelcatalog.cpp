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

#include <climits>

#include <QtCore/QHash>

#include <QtCore/QFileInfo>
#include <QtCore/QObject>

#include "src/core/models/scriptmodel.h"
#include "src/core/reactionparser.h"
#include "src/ui/widgets/reactioneditorwidget.h"

#include "modelcatalog.h"

namespace ModelCatalog {

namespace {
// Keys in the order of the reaction editor's presets (headers left out); not translated, they are stored in settings.
const char* const kStoichiometryKeys[] = {
    "1:1", "2:1/1:1", "1:1/1:2", "2:1/1:1/1:2", // the established stoichiometries
    "1:1+A2", "2:1/1:1+A2", "1:1/1:2+A2", "2:1/1:1/1:2+A2", // with host dimerisation
    "competitive", "ternary" // three components
};

int MaxComponents(Technique technique)
{
    return technique == Technique::Calorimetry ? 2 : INT_MAX; // the calorimetry models are host/guest only
}
}

QList<TechniqueInfo> Techniques()
{
    QList<TechniqueInfo> list;
    list << TechniqueInfo{ Technique::NMR, QStringLiteral("nmr"), QObject::tr("NMR Titration"), QObject::tr("NMR") };
    list << TechniqueInfo{ Technique::UVVis, QStringLiteral("uvvis"), QObject::tr("UV/Vis Titration"), QObject::tr("UV/Vis") };
    list << TechniqueInfo{ Technique::Fluorescence, QStringLiteral("fluorescence"), QObject::tr("Fluorescence Titration"), QObject::tr("Fluorescence") };
#ifdef ITC_Models
    list << TechniqueInfo{ Technique::Calorimetry, QStringLiteral("calorimetry"), QObject::tr("Calorimetry"), QObject::tr("ITC") };
#endif
#ifdef Kinetic_Models
    list << TechniqueInfo{ Technique::Kinetics, QStringLiteral("kinetics"), QObject::tr("Kinetics"), QObject::tr("Kinetics") };
#endif
    return list;
}

TechniqueInfo Info(Technique technique)
{
    for (const TechniqueInfo& info : Techniques())
        if (info.technique == technique)
            return info;
    return TechniqueInfo{ technique, QString(), QString(), QString() };
}

Technique TechniqueFromKey(const QString& key, Technique fallback)
{
    for (const TechniqueInfo& info : Techniques())
        if (info.key == key)
            return info.technique;
    return fallback;
}

QList<Technique> TechniquesFor(int independentColumns)
{
    QList<Technique> list;
    for (const TechniqueInfo& info : Techniques()) {
        const bool titration = info.technique == Technique::NMR || info.technique == Technique::UVVis || info.technique == Technique::Fluorescence;
        if (independentColumns <= 0 || (titration && independentColumns >= 2) || (!titration && independentColumns == 1))
            list << info.technique;
    }
    return list;
}

QList<Stoichiometry> Stoichiometries()
{
    QList<Stoichiometry> list;
    int index = 0;
    for (const ReactionEditorWidget::Preset& preset : ReactionEditorWidget::Presets()) {
        if (preset.isHeader())
            continue;
        if (index >= int(sizeof(kStoichiometryKeys) / sizeof(kStoichiometryKeys[0])))
            break; // a preset the catalogue does not know yet: it needs a key first
        Stoichiometry s;
        s.key = QString::fromLatin1(kStoichiometryKeys[index]);
        s.title = preset.name;
        s.reactions = preset.reactions;
        s.tooltip = QString(preset.reactions).replace('\n', QStringLiteral(" ; "));
        s.components = ReactionParser::Parse(preset.reactions).components.size();
        s.fixedIndex = index < 4 ? index : -1;
        list << s;
        ++index;
    }
    return list;
}

QList<int> FixedModels(Technique technique)
{
    switch (technique) {
    case Technique::NMR:
        return { SupraFit::nmr_ItoI, SupraFit::nmr_IItoI_ItoI, SupraFit::nmr_ItoI_ItoII, SupraFit::nmr_IItoI_ItoI_ItoII };
    case Technique::UVVis:
        return { SupraFit::uv_vis_ItoI, SupraFit::uv_vis_IItoI_ItoI, SupraFit::uv_vis_ItoI_ItoII, SupraFit::uv_vis_IItoI_ItoI_ItoII };
    case Technique::Fluorescence:
        return { SupraFit::fl_ItoI, SupraFit::fl_IItoI_ItoI, SupraFit::fl_ItoI_ItoII, SupraFit::fl_IItoI_ItoI_ItoII };
    case Technique::Calorimetry:
        return { SupraFit::itc_ItoI, SupraFit::itc_IItoI, SupraFit::itc_ItoII, SupraFit::itc_IItoII };
    case Technique::Kinetics:
        break;
    }
    return {};
}

int AnyModel(Technique technique)
{
    switch (technique) {
    case Technique::NMR:
        return SupraFit::nmr_any;
    case Technique::UVVis:
        return SupraFit::uvvis_any;
    case Technique::Fluorescence:
        return SupraFit::fl_any;
    case Technique::Calorimetry:
        return SupraFit::itc_any;
    case Technique::Kinetics:
        break;
    }
    return -1;
}

Choice ForStoichiometry(Technique technique, const QString& key)
{
    for (const Stoichiometry& s : Stoichiometries()) {
        if (s.key != key)
            continue;
        Choice choice;
        if (s.fixedIndex >= 0 && s.fixedIndex < FixedModels(technique).size()) {
            choice.kind = Choice::Kind::FixedModel;
            choice.model = FixedModels(technique).at(s.fixedIndex);
        } else if (AnyModel(technique) >= 0 && s.components <= MaxComponents(technique)) {
            choice.kind = Choice::Kind::AnyModelWithReactions;
            choice.model = AnyModel(technique);
            choice.reactions = s.reactions;
        }
        return choice;
    }
    return Choice();
}

namespace {
Entry FixedEntry(int model)
{
    Entry entry;
    entry.key = QStringLiteral("model:%1").arg(model);
    entry.kind = Kind::FixedModel;
    entry.title = Model2Name(SupraFit::Model(model));
    entry.tooltip = entry.title;
    entry.choice.kind = Choice::Kind::FixedModel;
    entry.choice.model = model;
    return entry;
}
}

Choice ChoiceFor(const Entry& entry, Technique technique)
{
    if (entry.kind == Kind::Stoichiometry)
        return ForStoichiometry(technique, entry.key.mid(QStringLiteral("stoich:").size()));
    return entry.choice;
}

QList<Entry> AllEntries(int independentColumns)
{
    QList<Entry> merged;
    QHash<QString, int> position;
    for (const Technique technique : TechniquesFor(independentColumns)) {
        for (Entry entry : Entries(technique, independentColumns)) {
            if (entry.kind == Kind::FixedModel) // a model of one technique alone: that technique is its own
                entry.techniques = { technique };
            if (position.contains(entry.key)) {
                Entry& known = merged[position[entry.key]];
                if (!known.techniques.contains(technique))
                    known.techniques << technique;
            } else {
                position.insert(entry.key, merged.size());
                merged << entry;
            }
        }
    }
    merged << ScriptPresetEntries(independentColumns);
    return merged;
}

QList<Entry> Entries(Technique technique, int independentColumns)
{
    QList<Entry> list;
    const bool titration = technique == Technique::NMR || technique == Technique::UVVis || technique == Technique::Fluorescence;
    if (independentColumns > 0 && !TechniquesFor(independentColumns).contains(technique))
        return list;

    if (titration || technique == Technique::Calorimetry) {
        for (const Stoichiometry& s : Stoichiometries()) {
            const Choice choice = ForStoichiometry(technique, s.key);
            if (!choice.valid())
                continue;
            // the fixed models are host/guest (two columns); a flexible model needs a column per component
            if (titration && independentColumns > 0) {
                const bool fits = choice.kind == Choice::Kind::FixedModel ? independentColumns == 2 : independentColumns == s.components;
                if (!fits)
                    continue;
            }
            Entry entry;
            entry.key = QStringLiteral("stoich:%1").arg(s.key);
            entry.kind = Kind::Stoichiometry;
            entry.title = s.title;
            entry.tooltip = s.tooltip;
            entry.choice = choice;
            entry.techniques << technique;
            list << entry;
        }
    }

    // the models that belong to the technique alone (as the Model menu has them)
    QList<int> further;
#ifdef ITC_Models
    if (technique == Technique::Calorimetry)
        further << SupraFit::itc_n_ItoII << SupraFit::itc_blank << SupraFit::TianModel;
#endif
#ifdef Kinetic_Models
    if (technique == Technique::Kinetics)
        further << SupraFit::Michaelis_Menten << SupraFit::MonoMolecularModel << SupraFit::BiMolecularModel << SupraFit::FlexMolecularModel << SupraFit::TianModel << SupraFit::DecayRates;
#endif
    for (int model : further) {
        Entry entry = FixedEntry(model);
        entry.techniques << technique;
        list << entry;
    }
    return list;
}

QList<Entry> ScriptPresetEntries(int independentColumns)
{
    QList<Entry> list;
    const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
    for (int i = 0; i < presets.size(); ++i) {
        if (independentColumns > 0 && presets[i].inputSize > independentColumns)
            continue; // needs more independent columns than the data has
        Entry entry;
        entry.key = QStringLiteral("script:%1").arg(presets[i].name);
        entry.kind = Kind::ScriptPreset;
        entry.title = presets[i].name;
        entry.tooltip = QObject::tr("%1 \u2014 needs %2 independent column(s)").arg(presets[i].name).arg(presets[i].inputSize);
        entry.choice.kind = Choice::Kind::ScriptPreset;
        entry.choice.scriptPreset = i;
        list << entry;
    }
    return list;
}

Entry ModelFileEntry(const QString& file)
{
    Entry entry;
    entry.key = QStringLiteral("file:%1").arg(file);
    entry.kind = Kind::ModelFile;
    entry.title = QFileInfo(file).completeBaseName();
    entry.tooltip = file;
    entry.choice.kind = Choice::Kind::ModelFile;
    entry.choice.file = file;
    return entry;
}
}
