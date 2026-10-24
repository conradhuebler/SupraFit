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


#include <algorithm>

#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSettings>

#include "modelusage.h"

namespace ModelUsage {

namespace {
const QString kGroup = QStringLiteral("add_model_bar");

QJsonObject Counters()
{
    return QJsonDocument::fromJson(QSettings().value(kGroup + QStringLiteral("/usage")).toString().toUtf8()).object();
}

// Stoichiometries and a technique's own models are counted per technique, scripted presets and files for all.
QString CounterKey(ModelCatalog::Technique technique, const ModelCatalog::Entry& entry)
{
    const bool perTechnique = entry.kind == ModelCatalog::Kind::Stoichiometry || entry.kind == ModelCatalog::Kind::FixedModel;
    return (perTechnique ? ModelCatalog::Info(technique).key : QStringLiteral("any")) + QLatin1Char('|') + entry.key;
}

QString CounterKey(ModelCatalog::Technique technique, const QString& entryKey)
{
    const bool perTechnique = entryKey.startsWith(QLatin1String("stoich:")) || entryKey.startsWith(QLatin1String("model:"));
    return (perTechnique ? ModelCatalog::Info(technique).key : QStringLiteral("any")) + QLatin1Char('|') + entryKey;
}

// The entry with \p key for the data, if there is one.
bool EntryForKey(const QString& key, int columns, ModelCatalog::Entry* out)
{
    for (const ModelCatalog::Entry& entry : Candidates(columns))
        if (entry.key == key) {
            *out = entry;
            return true;
        }
    return false;
}
}

QStringList DefaultFixed()
{
    QStringList keys;
    const QList<ModelCatalog::Stoichiometry> list = ModelCatalog::Stoichiometries();
    for (int i = 0; i < list.size() && i < 4; ++i)
        keys << QStringLiteral("stoich:%1").arg(list[i].key);
    return keys;
}

Preferences Load()
{
    QSettings settings;
    Preferences preferences;
    preferences.mode = settings.value(kGroup + QStringLiteral("/mode")).toString() == QLatin1String("fixed") ? Mode::Fixed : Mode::Frequent;
    preferences.count = qBound(1, settings.value(kGroup + QStringLiteral("/count"), 4).toInt(), 20);
    preferences.fixed = settings.contains(kGroup + QStringLiteral("/fixed")) ? settings.value(kGroup + QStringLiteral("/fixed")).toStringList() : DefaultFixed();
    preferences.technique = settings.value(kGroup + QStringLiteral("/technique")).toString();
    return preferences;
}

void Save(const Preferences& preferences)
{
    QSettings settings;
    settings.setValue(kGroup + QStringLiteral("/mode"), preferences.mode == Mode::Fixed ? QStringLiteral("fixed") : QStringLiteral("frequent"));
    settings.setValue(kGroup + QStringLiteral("/count"), qBound(1, preferences.count, 20));
    settings.setValue(kGroup + QStringLiteral("/fixed"), preferences.fixed);
    settings.setValue(kGroup + QStringLiteral("/technique"), preferences.technique);
}

void Record(ModelCatalog::Technique technique, const ModelCatalog::Entry& entry)
{
    QJsonObject counters = Counters();
    const QString key = CounterKey(technique, entry);
    counters[key] = counters.value(key).toInt() + 1;
    QSettings settings;
    settings.setValue(kGroup + QStringLiteral("/usage"), QString::fromUtf8(QJsonDocument(counters).toJson(QJsonDocument::Compact)));
    if (entry.kind == ModelCatalog::Kind::Stoichiometry || entry.kind == ModelCatalog::Kind::FixedModel)
        settings.setValue(kGroup + QStringLiteral("/last_technique"), ModelCatalog::Info(technique).key);
}

int Used(ModelCatalog::Technique technique, const QString& entryKey)
{
    return Counters().value(CounterKey(technique, entryKey)).toInt();
}

int UsedTotal(const QString& entryKey)
{
    const QJsonObject counters = Counters();
    const QString suffix = QLatin1Char('|') + entryKey;
    int total = 0;
    for (auto it = counters.constBegin(); it != counters.constEnd(); ++it)
        if (it.key().endsWith(suffix))
            total += it.value().toInt();
    return total;
}

void Reset()
{
    QSettings settings;
    settings.remove(kGroup + QStringLiteral("/usage"));
    settings.remove(kGroup + QStringLiteral("/last_technique"));
}

ModelCatalog::Technique DefaultTechnique(int independentColumns)
{
    using ModelCatalog::Technique;
    const QList<Technique> fitting = ModelCatalog::TechniquesFor(independentColumns);
    const Preferences preferences = Load();
    const QString key = preferences.technique.isEmpty() ? QSettings().value(kGroup + QStringLiteral("/last_technique")).toString() : preferences.technique;
    if (!key.isEmpty()) {
        const Technique wanted = ModelCatalog::TechniqueFromKey(key, Technique::NMR);
        if (ModelCatalog::Info(wanted).key == key && fitting.contains(wanted))
            return wanted;
    }
    if (fitting.contains(Technique::NMR))
        return Technique::NMR;
    return fitting.isEmpty() ? Technique::NMR : fitting.first();
}

ModelCatalog::Technique TechniqueFor(const ModelCatalog::Entry& entry, int independentColumns)
{
    using ModelCatalog::Technique;
    if (entry.techniques.isEmpty())
        return DefaultTechnique(independentColumns); // a scripted preset or a file: no technique of its own
    // the technique the entry was used with most
    Technique best = entry.techniques.first();
    int most = 0;
    for (const Technique t : entry.techniques)
        if (Used(t, entry.key) > most) {
            most = Used(t, entry.key);
            best = t;
        }
    if (most > 0)
        return best;
    // never used: the fixed technique, the one used last, NMR - each if the entry is offered for it
    const Preferences preferences = Load();
    const QString last = QSettings().value(kGroup + QStringLiteral("/last_technique")).toString();
    for (const QString& key : { preferences.technique, last }) {
        if (key.isEmpty())
            continue;
        const Technique wanted = ModelCatalog::TechniqueFromKey(key, Technique::NMR);
        if (ModelCatalog::Info(wanted).key == key && entry.techniques.contains(wanted))
            return wanted;
    }
    return entry.techniques.contains(Technique::NMR) ? Technique::NMR : entry.techniques.first();
}

QList<ModelCatalog::Entry> Candidates(int independentColumns)
{
    QList<ModelCatalog::Entry> list = ModelCatalog::AllEntries(independentColumns);
    // model files are known from having been used (the counters hold their path), as long as they exist
    const QJsonObject counters = Counters();
    const QString prefix = QStringLiteral("any|file:");
    QStringList files;
    for (auto it = counters.constBegin(); it != counters.constEnd(); ++it)
        if (it.key().startsWith(prefix))
            files << it.key().mid(prefix.size());
    files.sort();
    for (const QString& file : files)
        if (QFileInfo::exists(file))
            list << ModelCatalog::ModelFileEntry(file);
    return list;
}

QList<ModelCatalog::Entry> Ranked(int independentColumns)
{
    QList<ModelCatalog::Entry> list = Candidates(independentColumns);
    std::stable_sort(list.begin(), list.end(), [](const ModelCatalog::Entry& a, const ModelCatalog::Entry& b) {
        return UsedTotal(a.key) > UsedTotal(b.key);
    });
    return list;
}

QList<ModelCatalog::Entry> Quick(int independentColumns)
{
    const Preferences preferences = Load();
    QList<ModelCatalog::Entry> shown;
    if (preferences.mode == Mode::Fixed) {
        for (const QString& key : preferences.fixed) {
            ModelCatalog::Entry entry;
            if (EntryForKey(key, independentColumns, &entry))
                shown << entry; // an entry the data does not offer is left out, not an error
        }
        return shown;
    }
    shown = Ranked(independentColumns);
    while (shown.size() > preferences.count)
        shown.removeLast();
    return shown;
}
}
