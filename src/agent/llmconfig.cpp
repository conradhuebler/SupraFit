// llmconfig.cpp - Endpoint profiles for the LLM client.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Modelled on qurcuma's src/llm/llmconfig.cpp. Plain JSON,
// no QSettings, no key material in the file -- a profile names the environment
// variable the key comes from.

#include "llmconfig.h"
#include "llmpresets.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QUrl>
#include <QStandardPaths>

QString LlmProfile::chatCompletionsUrl() const
{
    QString base = baseUrl;
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    return base + QStringLiteral("/chat/completions");
}

QString LlmConfig::defaultPath()
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return configDir + QStringLiteral("/llm.json");
}

bool LlmConfig::load(const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("cannot open %1: %2").arg(path, file.errorString());
        return false;
    }
    return loadFromJson(file.readAll(), error);
}

bool LlmConfig::loadFromJson(const QByteArray& json, QString* error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error)
            *error = QStringLiteral("malformed JSON: %1").arg(parseError.errorString());
        return false;
    }

    // A key written into the file is a leak waiting to be committed. Refuse the
    // whole file rather than silently ignore it: the user should know.
    const QJsonObject root = doc.object();
    if (root.contains(QStringLiteral("api_key")) || root.contains(QStringLiteral("apikey"))) {
        if (error)
            *error = QStringLiteral("config file must not contain a key; name the environment variable instead (api_key_env)");
        return false;
    }

    m_profiles.clear();
    m_active = root.value(QStringLiteral("active_profile")).toString();
    m_rootExtra = root;
    m_rootExtra.remove(QStringLiteral("active_profile"));
    m_rootExtra.remove(QStringLiteral("profiles"));

    const QJsonArray profiles = root.value(QStringLiteral("profiles")).toArray();
    for (const QJsonValue& v : profiles) {
        const QJsonObject p = v.toObject();
        // A key written into a PROFILE is the same leak as at the top level:
        // refuse with a message, not silently.
        if (p.contains(QStringLiteral("api_key")) || p.contains(QStringLiteral("apikey"))) {
            if (error)
                *error = QStringLiteral("profile '%1' must not contain a key; name the environment variable instead (api_key_env)")
                             .arg(p.value(QStringLiteral("name")).toString());
            m_profiles.clear();
            return false;
        }
        LlmProfile profile;
        profile.name = p.value(QStringLiteral("name")).toString();
        profile.baseUrl = p.value(QStringLiteral("base_url")).toString();
        profile.model = p.value(QStringLiteral("model")).toString();
        profile.apiKeyEnv = p.value(QStringLiteral("api_key_env")).toString();
        profile.supportsVision = p.value(QStringLiteral("supports_vision")).toBool(false);
        profile.stream = p.value(QStringLiteral("stream")).toBool(true);
        profile.reasoningField = p.value(QStringLiteral("reasoning_field")).toString(profile.reasoningField);
        profile.maxToolIterations = p.value(QStringLiteral("max_tool_iterations")).toInt(profile.maxToolIterations);
        profile.requestTimeoutMs = p.value(QStringLiteral("request_timeout_ms")).toInt(profile.requestTimeoutMs);
        profile.preset = p.value(QStringLiteral("preset")).toString();
        profile.extra = p;
        for (const char* known : { "name", "base_url", "model", "api_key_env", "supports_vision", "stream",
                 "reasoning_field", "max_tool_iterations", "request_timeout_ms", "preset" })
            profile.extra.remove(QString::fromLatin1(known));
        if (!profile.isValid()) {
            // An incomplete profile is refused and says what is missing -- a typo
            // in base_url otherwise produces a first run that fails for no reason.
            if (error)
                *error = QStringLiteral("profile '%1' is incomplete: %2")
                             .arg(profile.name.isEmpty() ? QStringLiteral("(no name)") : profile.name,
                                 profile.name.isEmpty() ? QStringLiteral("name is empty")
                                                        : QStringLiteral("base_url is empty"));
            m_profiles.clear();
            return false;
        }
        m_profiles.append(profile);
    }

    if (m_active.isEmpty() && !m_profiles.isEmpty())
        m_active = m_profiles.first().name;

    if (error)
        error->clear();
    return true;
}

bool LlmConfig::writeExampleIfMissing(const QString& path, QString* error)
{
    if (QFileInfo::exists(path))
        return true;

    // Create the parent directory; a fresh machine has no ~/.config/suprafit yet.
    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath())) {
        if (error)
            *error = QStringLiteral("cannot create directory %1").arg(info.absolutePath());
        return false;
    }

    // Parseable JSON with the guidance in "_note" keys: comments are NOT valid
    // JSON, and an example file that cannot load is a worse first experience
    // than one that can (bug found by configWritesExampleIfMissing, 2026-10-11).
    // Since WP10 the example is the default configuration (Ollama preset on 127.0.0.1).
    return defaultConfig().save(path, error);
}

LlmConfig LlmConfig::defaultConfig()
{
    LlmConfig config;
    const LlmPreset* ollama = LlmPresets::find(LlmPresets::defaultId());
    LlmProfile local = ollama->toProfile(QStringLiteral("local"));
    config.m_profiles.append(local);
    config.m_active = local.name;
    QJsonArray note;
    for (const char* line : {
             "One entry per endpoint. This file never holds a key: a profile names the",
             "ENVIRONMENT VARIABLE the key comes from, so this file can be shared or",
             "committed to a repo without leaking anything. A local endpoint usually",
             "needs no key at all. The model may stay empty -- the client then asks the",
             "endpoint (GET <base_url>/models) and the user picks.",
             "Ollama is addressed as 127.0.0.1, not localhost: localhost may resolve to ::1,",
             "where another server (e.g. an ssh tunnel) can listen.",
             "More providers: Agent dock menu > Endpoints... (presets for OpenAI, Gemini, ...)." })
        note.append(QString::fromLatin1(line));
    config.m_rootExtra.insert(QStringLiteral("_note"), note);
    return config;
}

QByteArray LlmConfig::toJson() const
{
    QJsonObject root = m_rootExtra;
    root.remove(QStringLiteral("api_key"));
    root.remove(QStringLiteral("apikey"));
    root.insert(QStringLiteral("active_profile"), m_active);
    QJsonArray profiles;
    for (const LlmProfile& p : m_profiles) {
        QJsonObject o = p.extra;
        o.remove(QStringLiteral("api_key")); // never, whatever came in
        o.remove(QStringLiteral("apikey"));
        o.insert(QStringLiteral("name"), p.name);
        o.insert(QStringLiteral("base_url"), p.baseUrl);
        o.insert(QStringLiteral("model"), p.model);
        if (!p.apiKeyEnv.isEmpty())
            o.insert(QStringLiteral("api_key_env"), p.apiKeyEnv);
        if (p.supportsVision)
            o.insert(QStringLiteral("supports_vision"), true);
        o.insert(QStringLiteral("stream"), p.stream);
        o.insert(QStringLiteral("reasoning_field"), p.reasoningField);
        o.insert(QStringLiteral("max_tool_iterations"), p.maxToolIterations);
        o.insert(QStringLiteral("request_timeout_ms"), p.requestTimeoutMs);
        if (!p.preset.isEmpty())
            o.insert(QStringLiteral("preset"), p.preset);
        profiles.append(o);
    }
    root.insert(QStringLiteral("profiles"), profiles);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool LlmConfig::save(const QString& path, QString* error) const
{
    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath())) {
        if (error)
            *error = QStringLiteral("cannot create directory %1").arg(info.absolutePath());
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QStringLiteral("cannot write %1: %2").arg(path, file.errorString());
        return false;
    }
    file.write(toJson());
    if (!file.commit()) {
        if (error)
            *error = QStringLiteral("cannot write %1: %2").arg(path, file.errorString());
        return false;
    }
    return true;
}

QString LlmConfig::displayName(const LlmProfile& profile)
{
    const LlmPreset* preset = LlmPresets::find(profile.preset);
    if (!preset || preset->name == profile.name || preset->id == QLatin1String("custom"))
        return profile.name;
    return QStringLiteral("%1 (%2)").arg(profile.name, preset->name);
}

void LlmModelSelection::remember(const QString& profileName, const QString& model)
{
    if (model.trimmed().isEmpty())
        m_remembered.remove(profileName);
    else
        m_remembered.insert(profileName, model.trimmed());
}

QString LlmModelSelection::choose(const LlmProfile& profile, const QStringList& listed) const
{
    const QString remembered = m_remembered.value(profile.name);
    if (!remembered.isEmpty())
        return remembered;
    if (!profile.model.isEmpty())
        return profile.model;
    return listed.isEmpty() ? QString() : listed.first();
}

bool LlmConfig::profile(const QString& name, LlmProfile& out) const
{
    for (const LlmProfile& p : m_profiles) {
        if (p.name == name) {
            out = p;
            return true;
        }
    }
    return false;
}

bool LlmConfig::activeProfile(LlmProfile& out) const
{
    if (!m_active.isEmpty() && profile(m_active, out))
        return true;
    if (!m_profiles.isEmpty()) {
        out = m_profiles.first();
        return true;
    }
    return false;
}

QString LlmConfig::missingKeyMessage(const LlmProfile& profile)
{
    if (profile.apiKeyEnv.isEmpty() || !apiKeyFor(profile).isEmpty())
        return QString();
    return QStringLiteral("profile '%1' needs the API key in the environment variable %2, which is not set")
        .arg(profile.name, profile.apiKeyEnv);
}

bool LlmConfig::isLocalEndpoint(const LlmProfile& profile)
{
    const QString host = QUrl(profile.baseUrl).host();
    return host == QLatin1String("localhost") || host == QLatin1String("127.0.0.1") || host == QLatin1String("::1");
}

QString LlmConfig::apiKeyFor(const LlmProfile& profile)
{
    if (profile.apiKeyEnv.isEmpty())
        return QString();
    return QProcessEnvironment::systemEnvironment().value(profile.apiKeyEnv);
}
