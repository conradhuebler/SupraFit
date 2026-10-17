// projectchats.h - Agent chats stored inside SupraFit project files.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 (UI redesign WP11; moved out of chatrecorder.h when the generic chat code
// went into the CuteChat library). The chats of a project live in the ProjectManager attachment
// ChatLog::blockKey() ("agent_chats") and are written with the project. ProjectChatBackend plugs
// this into the library's chat dock (ChatBackend); the free functions are used by the headless
// agent run (agentrunner.cpp). The JSON tags keep the "suprafit" prefix, so stored projects load
// as before.
#pragma once

#include <cutechat/chatbackend.h>
#include <cutechat/chatlog.h>

#include <QString>
#include <QStringList>
#include <QVector>

namespace ProjectChats {
/// Chats of project @p projectId; damaged ones are skipped with a line in @p warnings.
QVector<ChatLog> load(const QString& projectId, QStringList* warnings = nullptr);
/// Insert or replace @p chat (same id) in project @p projectId; marks the project unsaved.
void store(const QString& projectId, const ChatLog& chat);
/// Remove all chats from project @p projectId.
void clear(const QString& projectId);
}

/// ChatBackend over ProjectChats: the context id is the project uuid. Claude Generated.
class ProjectChatBackend : public ChatBackend {
public:
    QVector<ChatLog> load(const QString& contextId, QStringList* warnings = nullptr) override
    {
        return ProjectChats::load(contextId, warnings);
    }
    void store(const QString& contextId, const ChatLog& chat) override { ProjectChats::store(contextId, chat); }
    void clear(const QString& contextId) override { ProjectChats::clear(contextId); }
    /// Only while the project is still loaded.
    bool accepts(const QString& contextId) const override;
};
