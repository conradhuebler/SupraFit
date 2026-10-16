// llmpresets.h - Built-in endpoint presets (providers) for new LLM profiles.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 (UI redesign WP10). A preset fills a new profile with a provider's
// OpenAI-compatible base URL, the NAME of the environment variable for its key, and sensible
// defaults; the user may change everything afterwards. Presets never carry a key.
//
// Base URLs were checked against the providers' documentation on 2026-10-16; where only
// third-party guides confirmed one, the preset is marked unverified and says so in its note.
#pragma once

#include "llmconfig.h"

#include <QString>
#include <QVector>

struct LlmPreset {
    QString id;              ///< stable key, stored in the profile ("preset")
    QString name;            ///< display name, e.g. "Ollama (local)"
    QString baseUrl;         ///< OpenAI-compatible base, ends before /chat/completions
    QString apiKeyEnv;       ///< NAME of the key variable; empty for local servers
    QString reasoningField;  ///< "", "think" (Ollama) or "reasoning_effort"
    QString note;            ///< short hint for the user
    QString suggestedModel;  ///< optional, may be empty
    bool stream = true;
    bool local = false;      ///< runs on this machine: no data leaves it
    bool verified = true;    ///< base URL confirmed in the provider's own documentation

    /// A profile filled from this preset; @p profileName defaults to the preset name.
    LlmProfile toProfile(const QString& profileName = QString()) const;
};

namespace LlmPresets {
/// All presets, the default (Ollama local) first and "Custom" last.
const QVector<LlmPreset>& catalog();
/// Preset by id, or nullptr.
const LlmPreset* find(const QString& id);
/// Id of the default preset ("ollama").
QString defaultId();
/// True when @p profile talks to an Ollama server (preset or the usual port), which offers
/// per-model capability information on <root>/api/show.
bool isOllama(const LlmProfile& profile);
}
