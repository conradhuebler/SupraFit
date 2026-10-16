// llmpresets.cpp - Built-in endpoint presets (providers) for new LLM profiles.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 (UI redesign WP10).
#include "llmpresets.h"

#include <QUrl>

LlmProfile LlmPreset::toProfile(const QString& profileName) const
{
    LlmProfile profile;
    profile.name = profileName.isEmpty() ? name : profileName;
    profile.preset = id;
    profile.baseUrl = baseUrl;
    profile.apiKeyEnv = apiKeyEnv;
    profile.reasoningField = reasoningField;
    profile.stream = stream;
    profile.model = suggestedModel;
    profile.requestTimeoutMs = local ? 120000 : 180000;
    return profile;
}

namespace LlmPresets {

const QVector<LlmPreset>& catalog()
{
    static const QVector<LlmPreset> presets = [] {
        QVector<LlmPreset> v;
        auto add = [&v](const char* id, const char* name, const char* url, const char* keyEnv, const char* reasoning,
                       bool local, bool verified, const QString& note) {
            LlmPreset p;
            p.id = QString::fromLatin1(id);
            p.name = QString::fromUtf8(name);
            p.baseUrl = QString::fromLatin1(url);
            p.apiKeyEnv = QString::fromLatin1(keyEnv);
            p.reasoningField = QString::fromLatin1(reasoning);
            p.local = local;
            p.verified = verified;
            p.note = note;
            v.append(p);
        };
        add("ollama", "Ollama (local)", "http://127.0.0.1:11434/v1", "", "think", true, true,
            QStringLiteral("Local Ollama server, no key. The address is 127.0.0.1 on purpose: \"localhost\" may "
                           "resolve to ::1 first, where another server (for example an ssh tunnel) can listen. "
                           "Tool support per model is read from Ollama's /api/show."));
        add("lmstudio", "LM Studio (local)", "http://127.0.0.1:1234/v1", "", "", true, true,
            QStringLiteral("Local LM Studio server (start it in the Developer tab), no key."));
        add("llamacpp", "llama.cpp server (local)", "http://127.0.0.1:8080/v1", "", "", true, true,
            QStringLiteral("Local llama-server, no key. Tool calls need a model with a tool-call template "
                           "(start the server with --jinja)."));
        add("openai", "OpenAI", "https://api.openai.com/v1", "OPENAI_API_KEY", "", false, true,
            QStringLiteral("Hosted, needs a key. Set the reasoning field to reasoning_effort only for models "
                           "that accept it (o-series, gpt-5); other models reject the field."));
        add("anthropic", "Anthropic (OpenAI compatibility)", "https://api.anthropic.com/v1", "ANTHROPIC_API_KEY", "",
            false, true,
            QStringLiteral("Hosted, needs a key. Anthropic's OpenAI-compatible layer, documented as meant for "
                           "testing and comparing; strict tool schemas are ignored."));
        add("gemini", "Google Gemini (OpenAI compatibility)", "https://generativelanguage.googleapis.com/v1beta/openai",
            "GEMINI_API_KEY", "", false, true,
            QStringLiteral("Hosted, needs a key. Accepts reasoning_effort for thinking models."));
        add("openrouter", "OpenRouter", "https://openrouter.ai/api/v1", "OPENROUTER_API_KEY", "", false, false,
            QStringLiteral("Hosted gateway, needs a key; model ids carry the provider prefix (e.g. openai/...). "
                           "Unverified: the base URL was confirmed by third-party guides only."));
        add("mistral", "Mistral", "https://api.mistral.ai/v1", "MISTRAL_API_KEY", "", false, false,
            QStringLiteral("Hosted, needs a key. Unverified: the base URL was confirmed by third-party guides only."));
        add("groq", "Groq", "https://api.groq.com/openai/v1", "GROQ_API_KEY", "", false, true,
            QStringLiteral("Hosted, needs a key. Some OpenAI parameters (logprobs, n > 1) are rejected."));
        add("together", "Together AI", "https://api.together.ai/v1", "TOGETHER_API_KEY", "", false, true,
            QStringLiteral("Hosted, needs a key; model ids are namespaced. Older documentation uses "
                           "https://api.together.xyz/v1."));
        add("deepseek", "DeepSeek", "https://api.deepseek.com/v1", "DEEPSEEK_API_KEY", "", false, true,
            QStringLiteral("Hosted, needs a key. Unverified: the documentation does not confirm the model "
                           "listing at /models; type the model (deepseek-chat, deepseek-reasoner) if the list stays empty."));
        add("custom", "Custom", "", "", "", false, true,
            QStringLiteral("Any OpenAI-compatible server: enter the base URL (usually ending in /v1) and, if it "
                           "needs one, the name of the environment variable holding the key."));
        return v;
    }();
    return presets;
}

const LlmPreset* find(const QString& id)
{
    for (const LlmPreset& p : catalog()) {
        if (p.id == id)
            return &p;
    }
    return nullptr;
}

QString defaultId()
{
    return QStringLiteral("ollama");
}

bool isOllama(const LlmProfile& profile)
{
    if (profile.preset == QLatin1String("ollama"))
        return true;
    return QUrl(profile.baseUrl).port() == 11434;
}

} // namespace LlmPresets
