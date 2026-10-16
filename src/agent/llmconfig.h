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

#include <QJsonObject>
#include <QMap>
#include <QMetaType>
#include <QString>
#include <QStringList>
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
    QString preset;          ///< id of the preset the profile was made from (llmpresets.h); may be empty
    QJsonObject extra;       ///< keys this version does not know, written back unchanged on save

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

    /// The configuration a first start writes: one "local" profile from the Ollama preset
    /// (127.0.0.1:11434) plus the explanatory _note. Claude Generated.
    static LlmConfig defaultConfig();

    /// Replace the profiles / the active profile (endpoint manager). Claude Generated.
    void setProfiles(const QVector<LlmProfile>& profiles) { m_profiles = profiles; }
    void setActiveProfileName(const QString& name) { m_active = name; }
    /// The file content: known fields, unknown keys of the file and of each profile
    /// preserved, never a key. Claude Generated.
    QByteArray toJson() const;
    /// Write toJson() atomically (temporary file + rename). Claude Generated.
    bool save(const QString& path, QString* error = nullptr) const;

    /// Display name of @p profile: the profile name, with the preset name when they differ.
    static QString displayName(const LlmProfile& profile);

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
    QJsonObject m_rootExtra; ///< unknown top-level keys (e.g. _note), preserved on save
};

/*! \brief Which model a profile uses, independent of any widget. Claude Generated (WP10).
 *
 * After a switch to a profile: the model last chosen for that profile, else the profile's default
 * model, else the first one the endpoint listed. A choice sticks until the user changes it; the
 * remembered models are stored by the caller (the dock keeps them in QSettings). */
class LlmModelSelection {
public:
    void setRemembered(const QMap<QString, QString>& remembered) { m_remembered = remembered; }
    QMap<QString, QString> remembered() const { return m_remembered; }
    /// Remember @p model for @p profileName (empty forgets it).
    void remember(const QString& profileName, const QString& model);
    QString rememberedFor(const QString& profileName) const { return m_remembered.value(profileName); }
    /// The model to use for @p profile given the models the endpoint @p listed (may be empty).
    QString choose(const LlmProfile& profile, const QStringList& listed = QStringList()) const;

private:
    QMap<QString, QString> m_remembered;
};

// Claude Generated 2026 - Crosses from the client's thread to the GUI.
Q_DECLARE_METATYPE(LlmModelInfo)
