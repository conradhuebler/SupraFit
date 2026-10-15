// llmsession.h - The agent loop: ask, run what the model names, answer again.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - This is where the pieces meet. It keeps the conversation,
// builds the tool catalogue from the registry, sends it with every request, and
// when the model names a tool it runs it through the dispatcher and sends the
// result back as another message. The model itself executes nothing.
// Modelled on qurcuma's src/llm/llmsession.h.
//
// It also owns the approval policy, because that is a decision about a
// conversation, not about a tool: the same fit_model is unremarkable in one
// context and worth a question in another. ToolEffect is what it keys off.
#pragma once

#include "tool.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>

#include <functional>

class LlmClient;
class ToolDispatcher;
class ToolRegistry;

class LlmSession : public QObject {
    Q_OBJECT

public:
    /// Everything is borrowed. @p client must outlive the session.
    LlmSession(LlmClient* client, ToolRegistry* registry, ToolDispatcher* dispatcher,
               QObject* parent = nullptr);

    /// Asked for every tool whose effect needs approval at the current autonomy.
    /// Return false to refuse; the model is told it was refused and can carry on
    /// differently.
    using ApprovalFn = std::function<bool(const ToolSpec&, const QJsonObject& args)>;
    /// Without one, anything beyond Read and Display is refused. The safe default
    /// is the one where nobody has to remember to switch it on.
    void setApprovalPolicy(ApprovalFn approve);

    /// The autonomy level: how much the assistant may do without being asked.
    void setAutonomy(ToolAutonomy autonomy) { m_autonomy = autonomy; }
    ToolAutonomy autonomy() const { return m_autonomy; }

    void setSystemPrompt(const QString& prompt);
    /// Cap on send/run rounds within one turn. Reached, the model is asked once
    /// more without any tools, so the turn ends with an answer built from what it
    /// already gathered instead of with the work thrown away.
    void setMaxIterations(int iterations);
    int maxIterations() const { return m_maxIterations; }

    /// Largest tool result, in characters, that goes into the conversation. Beyond
    /// it the payload is cut and marked, because history is resent every round and
    /// one fat result is then paid for again and again.
    void setMaxToolResultChars(int chars) { m_maxToolResultChars = chars; }

    void ask(const QString& userMessage);
    void cancel();
    /// Forget the conversation. The system prompt stays.
    void reset();

    bool isBusy() const { return m_busy; }
    QJsonArray conversation() const { return m_messages; }

    /// Keep the conversation across ask() calls, so a follow-up question sees the
    /// earlier turns (a chat). Off by default: every ask() starts fresh with the
    /// system prompt only. Claude Generated.
    void setKeepHistory(bool keep) { m_keepHistory = keep; }
    bool keepHistory() const { return m_keepHistory; }
    /// Replace the conversation, e.g. with one restored from a saved chat. The
    /// next ask() continues it (even without keepHistory); a missing system prompt
    /// is put in front. Ignored while busy. Claude Generated.
    void setConversation(const QJsonArray& messages);

signals:
    void assistantMessage(const QString& text);
    /// Pieces of the answer and of the model's reasoning as they arrive. Only
    /// emitted while streaming; assistantMessage() always follows.
    void assistantChunk(const QString& text);
    void reasoningChunk(const QString& text);
    void toolStarted(const QString& name, const QJsonObject& args);
    void toolFinished(const QString& name, const ToolResult& result);
    void toolRefused(const QString& name, const QString& reason);
    void finished();
    void failed(const QString& error);
    void busyChanged(bool busy);

private:
    void sendRound();
    void onClientFinished(const QJsonObject& assistantMessage);
    void onClientFailed(const QString& error);
    void runToolCalls(const QJsonArray& toolCalls);
    /// Answer every pending tool_call with a refusal, so the history stays valid
    /// when the loop stops before the calls are run.
    void declineRemainingCalls(const QJsonArray& toolCalls, const QString& reason);
    QJsonArray toolCatalogue() const;
    void setBusy(bool busy);
    void endTurn();

    LlmClient* m_client = nullptr;
    ToolRegistry* m_registry = nullptr;
    ToolDispatcher* m_dispatcher = nullptr;

    ApprovalFn m_approve;
    ToolAutonomy m_autonomy = ToolAutonomy::Ask;
    QString m_systemPrompt;
    QJsonArray m_messages;
    int m_maxIterations = 50;
    int m_iteration = 0;
    /// Set for the one round that runs without tools, to force a closing answer.
    bool m_finalRound = false;
    int m_maxToolResultChars = 8000;
    bool m_busy = false;
    bool m_keepHistory = false;
    bool m_continueNext = false;  ///< set by setConversation(): the next ask() appends
};
