// chatrecorder.cpp - Writes the audit trail of an agent session into a ChatLog.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 (UI redesign WP11).
#include "chatrecorder.h"

#include "toolregistry.h"

#include "src/core/projectmanager.h"

#include <QJsonDocument>

namespace {
QString summaryOf(const ToolResult& result)
{
    if (!result.ok)
        return result.error;
    QString text = result.text.isEmpty() ? QString::fromUtf8(QJsonDocument(result.data).toJson(QJsonDocument::Compact)) : result.text;
    text = text.simplified();
    if (text.size() > 300)
        text = text.left(297) + QStringLiteral("...");
    return text;
}
} // namespace

ChatRecorder::ChatRecorder(LlmSession* session, const ToolRegistry* registry, QObject* parent)
    : QObject(parent)
    , m_session(session)
    , m_registry(registry)
{
    connect(session, &LlmSession::toolStarted, this, [this](const QString& n, const QJsonObject& a) { started(n, a); });
    connect(session, &LlmSession::toolFinished, this, [this](const QString&, const ToolResult& r) { finished(r); });
    connect(session, &LlmSession::toolRefused, this, [this](const QString& n, const QString& why) { refused(n, why); });
}

QString ChatRecorder::effectName(ToolEffect effect)
{
    switch (effect) {
    case ToolEffect::Read:
        return QStringLiteral("read");
    case ToolEffect::Display:
        return QStringLiteral("display");
    case ToolEffect::Mutate:
        return QStringLiteral("mutate");
    case ToolEffect::Compute:
        return QStringLiteral("compute");
    case ToolEffect::FileWrite:
        return QStringLiteral("file_write");
    case ToolEffect::Process:
        return QStringLiteral("process");
    }
    return QString();
}

LlmSession::ApprovalFn ChatRecorder::wrapApproval(LlmSession::ApprovalFn inner, const QString& decidedBy)
{
    return [this, inner, decidedBy](const ToolSpec& spec, const QJsonObject& args) {
        const bool approved = inner ? inner(spec, args) : false;
        if (m_log) {
            if (ChatEntry* entry = m_log->entry(m_pending)) {
                entry->approval = approved ? QStringLiteral("approved") : QStringLiteral("refused");
                entry->decidedBy = decidedBy;
            }
        }
        return approved;
    };
}

void ChatRecorder::started(const QString& name, const QJsonObject& args)
{
    if (!m_log)
        return;
    ChatEntry& entry = m_log->appendTool(name, args, true, QString());
    ToolSpec spec;
    if (m_registry && m_registry->tool(name, spec))
        entry.toolEffect = effectName(spec.effect);
    const QString project = args.value(QStringLiteral("project_id")).toString();
    m_log->addProjectUuid(project);
    m_log->touch();
    m_pending = m_log->entries().size() - 1;
    m_timer.start();
    if (onToolAdded)
        onToolAdded(m_pending);
}

void ChatRecorder::complete(ChatEntry* entry)
{
    entry->toolDurationMs = m_timer.isValid() ? m_timer.elapsed() : -1;
    if (entry->approval.isEmpty()) {
        // nobody was asked: the chosen autonomy allowed the call (or it was refused before asking)
        ToolSpec spec;
        if (m_registry && m_registry->tool(entry->toolName, spec) && !needsApprovalAt(spec.effect, m_autonomy)) {
            entry->approval = QStringLiteral("not_required");
            entry->decidedBy = QStringLiteral("autonomy:") + toolAutonomyName(m_autonomy);
        }
    }
    m_log->touch();
}

void ChatRecorder::finished(const ToolResult& result)
{
    if (!m_log)
        return;
    ChatEntry* entry = m_log->entry(m_pending);
    if (!entry)
        return;
    entry->toolOk = result.ok;
    entry->text = summaryOf(result);
    QString full;
    if (!result.ok)
        full = result.error;
    else {
        full = result.text;
        if (!result.data.isEmpty())
            full += (full.isEmpty() ? QString() : QStringLiteral("\n")) + QString::fromUtf8(QJsonDocument(result.data).toJson(QJsonDocument::Compact));
    }
    entry->toolResult = ChatLog::capResult(full, &entry->toolTruncated);
    complete(entry);
    const int index = m_pending;
    m_pending = -1;
    if (onToolChanged)
        onToolChanged(index);
}

void ChatRecorder::refused(const QString& name, const QString& reason)
{
    if (!m_log)
        return;
    ChatEntry* entry = m_log->entry(m_pending);
    if (!entry) {
        // refused before it started (malformed arguments): still an attempt the audit shows
        ChatEntry& added = m_log->appendTool(name, QJsonObject(), false, reason);
        added.toolResult = reason;
        added.approval = QStringLiteral("refused");
        added.decidedBy = QStringLiteral("session");
        m_log->touch();
        if (onToolAdded)
            onToolAdded(m_log->entries().size() - 1);
        return;
    }
    entry->toolOk = false;
    entry->text = QObject::tr("refused: %1").arg(reason);
    entry->toolResult = reason;
    if (entry->approval.isEmpty()) {
        entry->approval = QStringLiteral("refused");
        entry->decidedBy = reason.contains(QLatin1String("no approval policy")) ? QStringLiteral("policy") : QStringLiteral("session");
    }
    complete(entry);
    const int index = m_pending;
    m_pending = -1;
    if (onToolChanged)
        onToolChanged(index);
}

namespace ProjectChats {

QVector<ChatLog> load(const QString& projectId, QStringList* warnings)
{
    return ChatLog::fromProjectBlock(SupraFit::ProjectManager::instance().projectAttachment(projectId, ChatLog::projectKey()), warnings);
}

void store(const QString& projectId, const ChatLog& chat)
{
    if (projectId.isEmpty() || chat.isEmpty())
        return;
    QVector<ChatLog> chats = load(projectId);
    bool replaced = false;
    for (ChatLog& existing : chats) {
        if (existing.id() == chat.id()) {
            existing = chat;
            replaced = true;
        }
    }
    if (!replaced)
        chats.append(chat);
    SupraFit::ProjectManager::instance().setProjectAttachment(projectId, ChatLog::projectKey(), ChatLog::toProjectBlock(chats));
}

void clear(const QString& projectId)
{
    SupraFit::ProjectManager::instance().setProjectAttachment(projectId, ChatLog::projectKey(), QJsonValue());
}

} // namespace ProjectChats
