// llmclient.h - OpenAI-compatible chat completions with tool calling.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - The transport, and nothing else. It knows how to send a
// conversation plus a tool catalogue and how to read back what the model said or
// which tool it wants called. It does not run tools, does not decide whether it is
// allowed to, and keeps no conversation of its own -- that is LlmSession's job.
// Modelled on qurcuma's src/llm/llmclient.h; tested against Ollama at
// http://localhost:11434/v1.
//
// One thing worth being clear about: the model never executes anything. It names a
// tool and its arguments; SupraFit runs it and sends the result back as another
// message. There is no discovery either -- the whole catalogue travels with every
// single request, which is why keeping it small matters.
#pragma once

#include "llmconfig.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class LlmClient : public QObject {
    Q_OBJECT

public:
    explicit LlmClient(QObject* parent = nullptr);
    ~LlmClient() override;

    /// Switch the endpoint. Clears a session model override (setModel), so a model chosen for the
    /// previous profile never leaks into the new one.
    void setProfile(const LlmProfile& profile);
    LlmProfile profile() const { return m_profile; }

    /// Key for the Authorization header. Empty means the header is omitted, which
    /// is what a local endpoint wants.
    void setApiKey(const QString& key);

    /// POST one chat completion. @p messages and @p tools are already in the
    /// OpenAI shape. Exactly one of finished()/failed() follows.
    void send(const QJsonArray& messages, const QJsonArray& tools = {});

    /// Override the profile's model for this session. Empty falls back to the
    /// profile's own, and if that is empty too a send() fails with a clear reason.
    void setModel(const QString& model);

    QString model() const { return m_model.isEmpty() ? m_profile.model : m_model; }

    /// Ask the endpoint which models it serves (GET /v1/models). One request.
    /// modelsListed() follows in any case; on a failure modelListFailed() before it.
    void listModels();

    /// Ollama only (LlmPresets::isOllama): ask <root>/api/show for each of @p models and report
    /// tool support and context length through modelInfo(). Other endpoints: nothing happens,
    /// because they offer no such information. Claude Generated (WP10).
    void queryCapabilities(const QStringList& models);

    /// Parse Ollama's /api/show answer. Claude Generated.
    static LlmModelInfo parseOllamaShow(const QString& id, const QJsonObject& show);
    /// Model ids of an OpenAI /models answer, in order, without duplicates.
    static QStringList parseModelList(const QByteArray& body);
    /// Plain-words explanation of a failed request to @p profile (HTTP status 0 = no answer).
    static QString describeFailure(int httpStatus, const QString& networkError, const LlmProfile& profile);

    /// Abort the request in flight. failed() is not emitted for a cancellation.
    void cancel();
    bool isBusy() const { return m_reply != nullptr; }

signals:
    /// The assistant message: either "content" with text, or "tool_calls" with
    /// what the model wants run, or both.
    void finished(const QJsonObject& assistantMessage);
    /// Transport, HTTP or protocol failure, in words a user can act on.
    void failed(const QString& error);
    /// Answer to listModels(); empty when the endpoint offers no listing.
    void modelsListed(const QStringList& models);
    /// listModels() failed; @p error says what to check. Claude Generated.
    void modelListFailed(const QString& error);
    /// Capability information for one model (queryCapabilities). Claude Generated.
    void modelInfo(const LlmModelInfo& info);

    /// A piece of the answer, while streaming. The complete message still arrives
    /// through finished(), so a consumer may ignore these entirely.
    void contentChunk(const QString& text);
    /// A piece of the model's reasoning, while streaming. Deliberately kept apart
    /// from the answer: it is worth showing, and worth NOT sending back in the
    /// next request.
    void reasoningChunk(const QString& text);

private:
    void handleReply();
    void handleStreamData();
    void consumeStreamLine(const QByteArray& line);
    /// Build the assistant message from what the stream delivered.
    QJsonObject assembleStreamedMessage() const;
    void finishStream();
    void handleModelList();

    QNetworkAccessManager* m_network = nullptr;
    QNetworkReply* m_reply = nullptr;
    QNetworkReply* m_listReply = nullptr;
    LlmProfile m_profile;
    QString m_model;   ///< session override; empty = use the profile's
    QString m_apiKey;

    // Streaming state, reset per request.
    bool m_streaming = false;
    QByteArray m_streamBuffer;      ///< bytes not yet forming a whole line
    QString m_streamedContent;
    QString m_streamedReasoning;
    /// Tool calls accumulated by their delta index. OpenAI splits `arguments`
    /// across chunks and only the index ties the pieces together; Ollama sends the
    /// whole call at once. Both end up here.
    QMap<int, QJsonObject> m_streamedToolCalls;
    bool m_streamFinished = false;
};
