// llmsession.cpp - The agent loop.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Modelled on qurcuma's src/llm/llmsession.cpp.

#include "llmsession.h"

#include "llmclient.h"
#include "tooldispatcher.h"
#include "toolregistry.h"

#include <QJsonDocument>
#include <QMetaObject>
#include <QSet>
#include <QThread>

namespace {

QJsonObject message(const QString& role, const QString& content)
{
    QJsonObject o;
    o.insert(QStringLiteral("role"), role);
    o.insert(QStringLiteral("content"), content);
    return o;
}

}  // namespace

LlmSession::LlmSession(LlmClient* client, ToolRegistry* registry, ToolDispatcher* dispatcher,
                       QObject* parent)
    : QObject(parent)
    , m_client(client)
    , m_registry(registry)
    , m_dispatcher(dispatcher)
{
    if (m_client) {
        connect(m_client, &LlmClient::finished, this, &LlmSession::onClientFinished);
        connect(m_client, &LlmClient::failed, this, &LlmSession::onClientFailed);
        connect(m_client, &LlmClient::contentChunk, this, &LlmSession::assistantChunk);
        connect(m_client, &LlmClient::reasoningChunk, this, &LlmSession::reasoningChunk);
    }
}

void LlmSession::setApprovalPolicy(ApprovalFn approve)
{
    m_approve = std::move(approve);
}

void LlmSession::setSystemPrompt(const QString& prompt)
{
    m_systemPrompt = prompt;
}

void LlmSession::setMaxIterations(int iterations)
{
    m_maxIterations = qMax(1, iterations);
}

void LlmSession::reset()
{
    m_messages = QJsonArray();
    m_iteration = 0;
    m_continueNext = false;
}

void LlmSession::setConversation(const QJsonArray& messages)
{
    if (m_busy)
        return;
    m_messages = messages;
    m_iteration = 0;
    m_continueNext = !messages.isEmpty();
}

void LlmSession::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged(busy);
}

QJsonArray LlmSession::toolCatalogue() const
{
    QJsonArray catalogue;
    if (!m_registry)
        return catalogue;

    // Which tools can do anything right now. The predicates may look at GUI-owned
    // state, so they are evaluated on the thread that owns it; this loop is on the
    // agent loop's. Blocking is safe here for the same reason the approval callback
    // is -- nothing on the GUI thread ever waits on this one.
    QSet<QString> unavailable;
    const auto collect = [this, &unavailable] {
        for (const ToolSpec& spec : m_registry->all()) {
            if (spec.available && !spec.available())
                unavailable.insert(spec.name);
        }
    };
    if (m_dispatcher && QThread::currentThread() != m_dispatcher->thread())
        QMetaObject::invokeMethod(m_dispatcher, collect, Qt::BlockingQueuedConnection);
    else
        collect();

    for (const ToolSpec& spec : m_registry->all()) {
        if (unavailable.contains(spec.name))
            continue;
        QJsonObject function;
        function.insert(QStringLiteral("name"), spec.name);
        function.insert(QStringLiteral("description"), spec.description);
        function.insert(QStringLiteral("parameters"), spec.paramSchema);

        QJsonObject entry;
        entry.insert(QStringLiteral("type"), QStringLiteral("function"));
        entry.insert(QStringLiteral("function"), function);
        catalogue.append(entry);
    }
    return catalogue;
}

void LlmSession::ask(const QString& userMessage)
{
    if (m_busy)
        return;
    // A fresh turn starts with a clean interrupt flag -- cancel() sets it, and a
    // new ask() must not inherit it (endTurn() no longer clears it, see there).
    if (m_dispatcher)
        m_dispatcher->clearInterrupt();
    const bool continueConversation = (m_keepHistory || m_continueNext) && !m_messages.isEmpty();
    m_continueNext = false;
    if (!continueConversation) {
        if (!m_messages.isEmpty() || m_systemPrompt.isEmpty())
            m_messages = QJsonArray();  // fresh turn keeps only the system prompt
        if (!m_systemPrompt.isEmpty())
            m_messages.prepend(message(QStringLiteral("system"), m_systemPrompt));
    } else if (!m_systemPrompt.isEmpty()
        && m_messages.first().toObject().value(QStringLiteral("role")).toString() != QStringLiteral("system")) {
        m_messages.prepend(message(QStringLiteral("system"), m_systemPrompt));
    }
    m_messages.append(message(QStringLiteral("user"), userMessage));
    m_iteration = 0;
    m_finalRound = false;
    setBusy(true);
    sendRound();
}

void LlmSession::cancel()
{
    if (!m_busy)
        return;
    // Set first, and unconditionally: a tool may be waiting right now, and the
    // queued cancel below cannot reach a loop that is itself blocked inside one.
    if (m_dispatcher)
        m_dispatcher->requestInterrupt();
    if (m_client)
        m_client->cancel();
    endTurn();
}

void LlmSession::endTurn()
{
    // BUGFIX (found by everyDispatchedToolIsAudited + 4-tool iteration-cap test,
    // 2026-10-11): cancel() used to clearInterrupt() here, so a cancelled session
    // could be re-asked and the NEXT turn would inherit a cleared flag -- fine.
    // But the real defect was the inverse: runToolCalls checked m_busy, which
    // cancel() had already cleared via endTurn(), so the loop broke mid-round and
    // left the remaining calls UNANSWERED -- a dangling tool_call in the history
    // makes the next request invalid. The interrupt flag is now the single source
    // of truth for cancellation, and endTurn() does NOT clear it; ask() clears it
    // at the start of a fresh turn.
    setBusy(false);
    emit finished();
}

void LlmSession::sendRound()
{
    ++m_iteration;
    // The closing round goes out without a catalogue, which is what makes it
    // closing: with no tools on offer the model has to write its answer.
    m_client->send(m_messages, m_finalRound ? QJsonArray() : toolCatalogue());
}

void LlmSession::onClientFailed(const QString& error)
{
    setBusy(false);
    emit failed(error);
}

void LlmSession::onClientFinished(const QJsonObject& assistantMessage)
{
    // The assistant's turn goes into the history verbatim, tool_calls included:
    // the protocol requires that every tool result refer back to a call the
    // history actually contains.
    m_messages.append(assistantMessage);

    const QJsonArray toolCalls = assistantMessage.value(QStringLiteral("tool_calls")).toArray();
    if (toolCalls.isEmpty()) {
        const QString text = assistantMessage.value(QStringLiteral("content")).toString();
        emit this->assistantMessage(text);
        endTurn();
        return;
    }

    if (m_finalRound) {
        // It was asked to answer without a catalogue and called a tool anyway.
        // Take whatever prose came with it rather than opening another round.
        const QString text = assistantMessage.value(QStringLiteral("content")).toString();
        emit this->assistantMessage(text.isEmpty()
                ? tr("The round budget ran out before an answer was written.")
                : text);
        endTurn();
        return;
    }

    if (m_iteration >= m_maxIterations) {
        // Not a dead end. The results gathered so far are in the history, and the
        // model is asked once more with no tools, so the turn ends with the answer
        // they support instead of with the work discarded.
        //
        // Every pending call still has to be answered first: the protocol requires
        // a tool result for each tool_call in the history, and a dangling one makes
        // the next request invalid.
        declineRemainingCalls(toolCalls,
            tr("the round budget for this turn (%1 rounds) is used up; no further tool will "
               "run. Answer now from the results already gathered.")
                .arg(m_maxIterations));
        m_messages.append(message(QStringLiteral("system"),
            tr("%1 rounds of tool calls used against a budget of %2. Answer now from the "
               "results already gathered; no further tools will run.")
                .arg(m_iteration).arg(m_maxIterations)));
        m_finalRound = true;
        sendRound();
        return;
    }

    runToolCalls(toolCalls);
    if (m_busy && !m_dispatcher->isInterrupted())
        sendRound();
}

void LlmSession::declineRemainingCalls(const QJsonArray& toolCalls, const QString& reason)
{
    for (const QJsonValue& value : toolCalls) {
        const QJsonObject call = value.toObject();
        const QString name = call.value(QStringLiteral("function")).toObject()
                                 .value(QStringLiteral("name")).toString();
        QJsonObject toolMessage;
        toolMessage.insert(QStringLiteral("role"), QStringLiteral("tool"));
        toolMessage.insert(QStringLiteral("tool_call_id"), call.value(QStringLiteral("id")).toString());
        toolMessage.insert(QStringLiteral("name"), name);
        toolMessage.insert(QStringLiteral("content"), reason);
        m_messages.append(toolMessage);
        emit toolRefused(name, reason);
    }
}

void LlmSession::runToolCalls(const QJsonArray& toolCalls)
{
    for (const QJsonValue& value : toolCalls) {
        if (m_dispatcher && m_dispatcher->isInterrupted()) {
            // Cancelled mid-round: answer the remaining calls with a refusal so the
            // history stays valid -- the protocol requires a tool result for every
            // tool_call it contains.
            declineRemainingCalls(QJsonArray{ value }, tr("cancelled by the user"));
            continue;
        }
        const QJsonObject call = value.toObject();
        const QString callId = call.value(QStringLiteral("id")).toString();
        const QJsonObject function = call.value(QStringLiteral("function")).toObject();
        const QString name = function.value(QStringLiteral("name")).toString();

        // Ollama sends arguments as a JSON *string*; OpenAI sends an object. Accept
        // both and parse exactly once, here.
        //
        // BUGFIX (found by unparseableArgumentsNeverReachHandler, 2026-10-11): the
        // string branch silently produced an EMPTY object on malformed JSON
        // (QJsonDocument::fromJson of "{not json" fails and .object() is empty),
        // so the handler ran with empty args -- a tool that takes no required
        // parameters then ran anyway, and a model sending garbage was never told.
        // Malformed or non-object arguments are now refused before the handler runs.
        QJsonObject args;
        QString argsError;
        const QJsonValue rawArgs = function.value(QStringLiteral("arguments"));
        if (rawArgs.isString()) {
            const QString text = rawArgs.toString();
            if (text.trimmed().isEmpty()) {
                // Empty arguments string is fine: same as {}.
            } else {
                QJsonParseError parseError;
                const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &parseError);
                if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                    argsError = tr("arguments are not a JSON object (%1)").arg(parseError.errorString());
                } else {
                    args = doc.object();
                }
            }
        } else if (rawArgs.isObject()) {
            args = rawArgs.toObject();
        } else if (!rawArgs.isUndefined()) {
            argsError = QStringLiteral("arguments must be a JSON object or a JSON string");
        }

        if (!argsError.isEmpty()) {
            const ToolResult failure = ToolResult::failure(argsError);
            m_messages.append([&] {
                QJsonObject toolMessage;
                toolMessage.insert(QStringLiteral("role"), QStringLiteral("tool"));
                toolMessage.insert(QStringLiteral("tool_call_id"), callId);
                toolMessage.insert(QStringLiteral("name"), name);
                toolMessage.insert(QStringLiteral("content"), argsError);
                return toolMessage;
            }());
            emit toolRefused(name, argsError);
            // Same audit path as every other refusal (see the refusal branch
            // below): a malformed-args call is an ATTEMPT the audit must show.
            if (m_dispatcher && QThread::currentThread() != m_dispatcher->thread())
                QMetaObject::invokeMethod(m_dispatcher,
                    [dispatcher = m_dispatcher, name, args] {
                        dispatcher->recordRefusal(name, args);
                    },
                    Qt::QueuedConnection);
            else if (m_dispatcher)
                m_dispatcher->recordRefusal(name, args);
            continue;
        }

        emit toolStarted(name, args);

        ToolSpec spec;
        QString refusal;
        if (!m_registry || !m_registry->tool(name, spec)) {
            refusal = tr("unknown tool '%1'").arg(name);
        } else if (needsApprovalAt(spec.effect, m_autonomy) && m_approve && !m_approve(spec, args)) {
            refusal = tr("the operator did not approve this call");
        } else if (needsApprovalAt(spec.effect, m_autonomy) && !m_approve) {
            // No approval callback: the safe default refuses anything beyond Read/Display.
            refusal = tr("this tool needs approval and no approval policy is set");
        }

        ToolResult result;
        if (!refusal.isEmpty()) {
            result = ToolResult::failure(refusal);
            emit toolRefused(name, refusal);
            // A refusal is an answer too: the model is told and can carry on
            // differently. It flows into the SAME tool-message path below as a
            // real result, so the history stays protocol-valid either way.
            //
            // BUGFIX (found by everyDispatchedToolIsAudited, 2026-10-11):
            // unknown-tool and refused calls used to bypass the dispatcher
            // entirely, so the audit trail never saw them -- a model repeatedly
            // naming a nonexistent tool left no trace. Refusals are now recorded
            // through the dispatcher's own audit signal.
            //
            // recordRefusal is a plain member, not a slot, so it can be invoked
            // directly. When the session runs on a worker thread we still want the
            // audit emission on the dispatcher's (GUI) thread, where
            // DirectConnection receivers expect it; otherwise call it directly.
            // Callers that assert on the audit must pump the event loop after the
            // turn so the queued invocation lands.
            if (m_dispatcher && QThread::currentThread() != m_dispatcher->thread())
                QMetaObject::invokeMethod(m_dispatcher,
                    [dispatcher = m_dispatcher, name, args] {
                        dispatcher->recordRefusal(name, args);
                    },
                    Qt::QueuedConnection);
            else if (m_dispatcher)
                m_dispatcher->recordRefusal(name, args);
        } else {
            result = m_dispatcher ? m_dispatcher->dispatch(name, args)
                                 : m_registry->invoke(name, args);
            emit toolFinished(name, result);
        }

        // The tool result goes back as its own message; cap it so one fat result
        // is not paid for again on every following round.
        QString content = result.ok
            ? QJsonDocument(result.data).toJson(QJsonDocument::Compact)
            : result.error;
        if (!result.text.isEmpty())
            content = result.text + QStringLiteral("\n") + content;
        if (content.size() > m_maxToolResultChars) {
            content = content.left(m_maxToolResultChars)
                + QStringLiteral("\n... [truncated, %1 of %2 characters]")
                      .arg(m_maxToolResultChars).arg(content.size());
        }

        QJsonObject toolMessage;
        toolMessage.insert(QStringLiteral("role"), QStringLiteral("tool"));
        toolMessage.insert(QStringLiteral("tool_call_id"), callId);
        toolMessage.insert(QStringLiteral("name"), name);
        toolMessage.insert(QStringLiteral("content"), content);
        m_messages.append(toolMessage);
    }
}
