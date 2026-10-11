// llmconfig.cpp - Endpoint profiles for the LLM client.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Modelled on qurcuma's src/llm/llmconfig.cpp. Plain JSON,
// no QSettings, no key material in the file -- a profile names the environment
// variable the key comes from.

#include "llmconfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
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
    const QByteArray example = R"JSON({
  "_note": [
    "One entry per endpoint. This file never holds a key: a profile names the",
    "ENVIRONMENT VARIABLE the key comes from, so this file can be shared or",
    "committed to a repo without leaking anything. A local endpoint usually",
    "needs no key at all. The model may stay empty -- the client then asks the",
    "endpoint (GET /v1/models) and the user picks."
  ],
  "active_profile": "local",
  "profiles": [
    {
      "name": "local",
      "base_url": "http://localhost:11434/v1",
      "model": "",
      "stream": true,
      "reasoning_field": "think",
      "max_tool_iterations": 50,
      "request_timeout_ms": 120000
    }
  ]
})JSON";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = QStringLiteral("cannot write %1: %2").arg(path, file.errorString());
        return false;
    }
    file.write(example);
    return true;
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

QString LlmConfig::apiKeyFor(const LlmProfile& profile)
{
    if (profile.apiKeyEnv.isEmpty())
        return QString();
    return QProcessEnvironment::systemEnvironment().value(profile.apiKeyEnv);
}
