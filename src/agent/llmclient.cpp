// llmclient.cpp - OpenAI-compatible chat completions with tool calling.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - The transport only; see llmclient.h. Ollama quirks this
// code handles deliberately (measured by qurcuma against a live Ollama, 2026-09):
//  - a tool_calls response still carries content: "" (empty string, not null)
//  - function.arguments is a JSON *string*, not an object
//  - streaming reasoning arrives in delta.reasoning (NOT reasoning_content)
//  - a missing model is {"error":{"message":"model '...' not found"}}

#include "llmclient.h"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

LlmClient::LlmClient(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

LlmClient::~LlmClient()
{
    cancel();
}

void LlmClient::setProfile(const LlmProfile& profile)
{
    m_profile = profile;
}

void LlmClient::setApiKey(const QString& key)
{
    m_apiKey = key;
}

void LlmClient::setModel(const QString& model)
{
    m_model = model;
}

void LlmClient::send(const QJsonArray& messages, const QJsonArray& tools)
{
    cancel();

    const QString model = m_model.isEmpty() ? m_profile.model : m_model;
    if (model.isEmpty()) {
        emit failed(QStringLiteral("no model selected: set one in the profile or call setModel()"));
        return;
    }
    if (!m_profile.isValid()) {
        emit failed(QStringLiteral("invalid profile '%1': name and base_url are required").arg(m_profile.name));
        return;
    }

    QJsonObject body;
    body.insert(QStringLiteral("model"), model);
    body.insert(QStringLiteral("messages"), messages);
    if (!tools.isEmpty()) {
        body.insert(QStringLiteral("tools"), tools);
        body.insert(QStringLiteral("tool_choice"), QStringLiteral("auto"));
    }
    if (m_profile.stream)
        body.insert(QStringLiteral("stream"), true);
    if (!m_profile.reasoningField.isEmpty())
        body.insert(m_profile.reasoningField, QStringLiteral("medium"));

    QNetworkRequest request{ QUrl(m_profile.chatCompletionsUrl()) };
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_apiKey.isEmpty())
        request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());

    m_reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::finished, this, &LlmClient::handleReply);
    if (m_profile.stream) {
        m_streaming = true;
        m_streamBuffer.clear();
        m_streamedContent.clear();
        m_streamedReasoning.clear();
        m_streamedToolCalls.clear();
        m_streamFinished = false;
        connect(m_reply, &QNetworkReply::readyRead, this, &LlmClient::handleStreamData);
    }
}

void LlmClient::listModels()
{
    if (!m_profile.isValid()) {
        emit modelsListed(QStringList());
        return;
    }
    QString base = m_profile.baseUrl;
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    // base_url ends in /v1; the listing lives at the parent.
    const int v1 = base.lastIndexOf(QStringLiteral("/v1"));
    const QString modelsUrl = (v1 > 0 ? base.left(v1) : base) + QStringLiteral("/models");

    QNetworkRequest modelsRequest{ QUrl(modelsUrl) };
    if (!m_apiKey.isEmpty())
        modelsRequest.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
    m_listReply = m_network->get(modelsRequest);
    connect(m_listReply, &QNetworkReply::finished, this, &LlmClient::handleModelList);
}

void LlmClient::cancel()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void LlmClient::handleModelList()
{
    QNetworkReply* reply = m_listReply;
    m_listReply = nullptr;
    if (!reply)
        return;
    reply->deleteLater();

    QStringList models;
    if (reply->error() == QNetworkReply::NoError) {
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray data = doc.object().value(QStringLiteral("data")).toArray();
        for (const QJsonValue& v : data)
            models.append(v.toObject().value(QStringLiteral("id")).toString());
    }
    emit modelsListed(models);
}

void LlmClient::handleReply()
{
    QNetworkReply* reply = m_reply;
    m_reply = nullptr;
    if (!reply)
        return;
    reply->deleteLater();

    if (reply->error() == QNetworkReply::OperationCanceledError)
        return;  // cancel(): no failed() for a deliberate abort

    const QByteArray httpPayload = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        // Prefer an error the ENDPOINT itself bothered to write -- {"error":{"message":...}}
        // arrives over an HTTP failure status in every implementation tried
        // (Ollama, OpenAI, and their proxies). A proxy's HTML error page is not
        // parseable JSON; then say the status and quote a slice of what came
        // back, so the cause is visible instead of an opaque code.
        //
        // BUGFIX (found by endpointErrorMessageIsPreferred, 2026-10-11): the
        // payload here was read with a SECOND readAll() -- empty, because
        // readAll() was already consumed into httpPayload above. Now httpPayload
        // is used, which was read exactly once.
        const QByteArray payload = httpPayload;
        const QJsonDocument doc = QJsonDocument::fromJson(payload);
        QString message;
        if (doc.isObject() && doc.object().contains(QStringLiteral("error")))
            message = doc.object().value(QStringLiteral("error")).toObject()
                          .value(QStringLiteral("message")).toString();
        if (message.isEmpty()) {
            const QString slice = QString::fromStdString(payload.left(200).toStdString());
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            message = QStringLiteral("HTTP %1: %2").arg(status).arg(reply->errorString());
            if (!slice.isEmpty())
                message += QStringLiteral(" [%1]").arg(slice);
        }
        emit failed(message);
        return;
    }

    if (m_streaming) {
        // Drain whatever is left, then assemble the message from the stream.
        handleStreamData();
        if (!m_streamFinished)
            finishStream();
        return;
    }

    const QByteArray payload = httpPayload;  // already read above; do not readAll twice
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit failed(QStringLiteral("malformed response from endpoint: %1").arg(parseError.errorString()));
        return;
    }

    const QJsonObject root = doc.object();
    // Ollama and OpenAI both report errors as {"error":{"message":...}} with HTTP 200
    // in some configurations; treat it as a failure either way.
    if (root.contains(QStringLiteral("error"))) {
        const QString message = root.value(QStringLiteral("error")).toObject()
                                    .value(QStringLiteral("message")).toString();
        emit failed(message.isEmpty() ? QStringLiteral("endpoint reported an error") : message);
        return;
    }

    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        emit failed(QStringLiteral("endpoint returned no choices"));
        return;
    }
    emit finished(choices.first().toObject().value(QStringLiteral("message")).toObject());
}

void LlmClient::handleStreamData()
{
    if (!m_reply || !m_streaming)
        return;
    m_streamBuffer.append(m_reply->readAll());

    int newline = -1;
    while ((newline = m_streamBuffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_streamBuffer.left(newline).trimmed();
        m_streamBuffer.remove(0, newline + 1);
        if (!line.isEmpty())
            consumeStreamLine(line);
    }
}

void LlmClient::consumeStreamLine(const QByteArray& line)
{
    // SSE framing: "data: {json}" per event, "data: [DONE]" at the end.
    if (!line.startsWith("data:"))
        return;
    const QByteArray payload = line.mid(5).trimmed();
    if (payload == "[DONE]") {
        finishStream();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        return;

    const QJsonObject chunk = doc.object();
    if (chunk.contains(QStringLiteral("error"))) {
        const QString message = chunk.value(QStringLiteral("error")).toObject()
                                    .value(QStringLiteral("message")).toString();
        // Surface the error as a failed stream; finished() still assembles what arrived.
        m_streamedContent.append(QStringLiteral("\n[endpoint error: ") + message + QStringLiteral("]"));
        return;
    }

    const QJsonObject delta = chunk.value(QStringLiteral("choices")).toArray().first().toObject()
                                  .value(QStringLiteral("delta")).toObject();
    if (delta.isEmpty())
        return;

    const QString content = delta.value(QStringLiteral("content")).toString();
    if (!content.isEmpty()) {
        m_streamedContent.append(content);
        emit contentChunk(content);
    }
    // Ollama names it "reasoning"; OpenAI's reasoning models use "reasoning_content".
    // Accept both, and never send the reasoning back in the next request.
    QString reasoning = delta.value(QStringLiteral("reasoning")).toString();
    if (reasoning.isEmpty())
        reasoning = delta.value(QStringLiteral("reasoning_content")).toString();
    if (!reasoning.isEmpty()) {
        m_streamedReasoning.append(reasoning);
        emit reasoningChunk(reasoning);
    }

    const QJsonArray toolCalls = delta.value(QStringLiteral("tool_calls")).toArray();
    for (const QJsonValue& v : toolCalls) {
        const QJsonObject call = v.toObject();
        const int index = call.value(QStringLiteral("index")).toInt();
        QJsonObject& slot = m_streamedToolCalls[index];
        if (!slot.contains(QStringLiteral("id")) && call.contains(QStringLiteral("id")))
            slot.insert(QStringLiteral("id"), call.value(QStringLiteral("id")));
        if (!slot.contains(QStringLiteral("type")) && call.contains(QStringLiteral("type")))
            slot.insert(QStringLiteral("type"), call.value(QStringLiteral("type")));

        QJsonObject function = slot.value(QStringLiteral("function")).toObject();
        const QJsonObject callFunction = call.value(QStringLiteral("function")).toObject();
        if (callFunction.contains(QStringLiteral("name")))
            function.insert(QStringLiteral("name"), callFunction.value(QStringLiteral("name")));
        if (callFunction.contains(QStringLiteral("arguments")))
            function.insert(QStringLiteral("arguments"),
                function.value(QStringLiteral("arguments")).toString()
                    + callFunction.value(QStringLiteral("arguments")).toString());
        slot.insert(QStringLiteral("function"), function);
    }
}

void LlmClient::finishStream()
{
    if (m_streamFinished)
        return;
    m_streamFinished = true;
    emit finished(assembleStreamedMessage());
}

QJsonObject LlmClient::assembleStreamedMessage() const
{
    QJsonObject message;
    message.insert(QStringLiteral("role"), QStringLiteral("assistant"));
    message.insert(QStringLiteral("content"), m_streamedContent);

    if (!m_streamedToolCalls.isEmpty()) {
        QJsonArray toolCalls;
        // QMap iterates by index; emit in index order for reproducibility.
        const QList<int> indices = m_streamedToolCalls.keys();
        for (int index : indices) {
            QJsonObject call = m_streamedToolCalls.value(index);
            if (!call.contains(QStringLiteral("type")))
                call.insert(QStringLiteral("type"), QStringLiteral("function"));
            // Ollama sends arguments as a JSON string; keep the string form -- the
            // session parses it exactly once when dispatching.
            toolCalls.append(call);
        }
        message.insert(QStringLiteral("tool_calls"), toolCalls);
    }
    return message;
}
