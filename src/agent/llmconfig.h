// llmconfig.h - Endpoint profiles for the LLM client.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - A plain JSON file at ~/.config/suprafit/llm.json, never
// QSettings: this is configuration a user edits by hand and copies between
// machines, and burying it in a settings backend makes both awkward.
// Modelled on qurcuma's src/llm/llmconfig.h.
//
// The file never holds a key. A profile names the ENVIRONMENT VARIABLE the key
// comes from, so the config can be shared, committed to a dotfiles repo, or
// attached to a bug report without leaking anything.
#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

/// What an endpoint says about one of its models.
struct LlmModelInfo {
    QString id;
    int contextLength = 0;        ///< tokens; 0 = not reported
    bool supportsTools = false;
    bool supportsVision = false;
    bool detailsKnown = false;    ///< false when the endpoint told us nothing
};

struct LlmProfile {
    QString name;            ///< how the profile is referred to, e.g. "local"
    QString baseUrl;         ///< e.g. "http://localhost:11434/v1"
    /// Preferred model. May be empty: which models exist is a property of the
    /// endpoint, not of this file, and guessing one produces a first run that
    /// fails for no reason. The client asks the endpoint and the user picks.
    QString model;
    QString apiKeyEnv;       ///< NAME of the env var holding the key; never the key
    bool supportsVision = false;  ///< may be sent images
    bool stream = true;           ///< SSE; off for an endpoint that cannot do it
    /// Ollama's own native field for reasoning effort is "think"; OpenAI-compatible
    /// servers take "reasoning_effort". Empty means the field is never sent.
    QString reasoningField = QStringLiteral("reasoning_effort");
    int maxToolIterations = 50;   ///< stop an agent loop that will not converge
    int requestTimeoutMs = 120000;

    bool isValid() const { return !name.isEmpty() && !baseUrl.isEmpty(); }
    /// Full endpoint for a chat completion, with exactly one slash at the join.
    QString chatCompletionsUrl() const;
};

class LlmConfig {
public:
    /// ~/.config/suprafit/llm.json (XDG config location).
    static QString defaultPath();

    /// Read @p path. Returns false and sets @p error on a missing or malformed
    /// file; a missing file is not a crime, so the caller can offer to write an
    /// example instead of failing hard.
    bool load(const QString& path, QString* error = nullptr);
    /// Parse from memory. Used by load() and by the tests.
    bool loadFromJson(const QByteArray& json, QString* error = nullptr);

    /// Write a commented example with one local (Ollama) profile. Does nothing if
    /// @p path already exists.
    static bool writeExampleIfMissing(const QString& path, QString* error = nullptr);

    QVector<LlmProfile> profiles() const { return m_profiles; }
    QString activeProfileName() const { return m_active; }
    bool profile(const QString& name, LlmProfile& out) const;
    /// The profile named by "active_profile", or the first one.
    bool activeProfile(LlmProfile& out) const;
    bool isEmpty() const { return m_profiles.isEmpty(); }

    /// Key for @p profile, read from the environment. Empty when the variable is
    /// unset or the profile names none (a local endpoint usually needs no key).
    static QString apiKeyFor(const LlmProfile& profile);

    /// Non-empty when the profile names a key variable that is not set in the environment: better to
    /// say so than to send the request without credentials and report a bare 401. Claude Generated.
    static QString missingKeyMessage(const LlmProfile& profile);

    /// True for an endpoint on this machine (localhost, 127.0.0.1, ::1): the project data stays here.
    static bool isLocalEndpoint(const LlmProfile& profile);

private:
    QVector<LlmProfile> m_profiles;
    QString m_active;
};

// Claude Generated 2026 - Crosses from the client's thread to the GUI.
Q_DECLARE_METATYPE(LlmModelInfo)
