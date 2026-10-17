// projectchats.cpp - see projectchats.h
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026
#include "projectchats.h"

#include "src/core/projectmanager.h"

namespace {
/// SupraFit's files carry the "suprafit-chat" / "suprafit-agent-chats" tags since WP9/WP11.
void useSupraFitFormatTags()
{
    ChatLog::setFormatPrefix(QStringLiteral("suprafit"));
}
} // namespace

namespace ProjectChats {

QVector<ChatLog> load(const QString& projectId, QStringList* warnings)
{
    useSupraFitFormatTags();
    return ChatLog::fromBlock(SupraFit::ProjectManager::instance().projectAttachment(projectId, ChatLog::blockKey()), warnings);
}

void store(const QString& projectId, const ChatLog& chat)
{
    if (projectId.isEmpty() || chat.isEmpty())
        return;
    useSupraFitFormatTags();
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
    SupraFit::ProjectManager::instance().setProjectAttachment(projectId, ChatLog::blockKey(), ChatLog::toBlock(chats));
}

void clear(const QString& projectId)
{
    SupraFit::ProjectManager::instance().setProjectAttachment(projectId, ChatLog::blockKey(), QJsonValue());
}

} // namespace ProjectChats

bool ProjectChatBackend::accepts(const QString& contextId) const
{
    return !contextId.isEmpty() && SupraFit::ProjectManager::instance().getLoadedProjectIds().contains(contextId);
}
