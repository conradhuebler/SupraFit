// suprafitchathost.h - SupraFit's side of the CuteChat chat dock.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - What the generic ChatDock (external CuteChat library) needs from the
// application: the analysis system prompt, the project/analysis/inspect tools, chats stored in the
// project file (ProjectChatBackend, context id = project uuid), the app version for the chat
// record and the wording that names projects and fits.
#pragma once

#include <cutechat/chathost.h>

#include "src/agent/projectchats.h"

class SupraFitChatHost : public ChatHost {
public:
    SupraFitChatHost();

    QString systemPrompt() const override;
    void registerTools(ToolRegistry& registry) override;
    ChatBackend* chatBackend() override { return &m_backend; }
    bool storeInBackend() const override;
    QStringList contextArguments() const override { return { QStringLiteral("project_id") }; }
    QString appVersion() const override;
    QString appCommit() const override;
    ChatUiTexts uiTexts() const override;
    bool approveToolCall(const ToolSpec& spec, const QJsonObject& args, QWidget* parent) override;

private:
    ProjectChatBackend m_backend;
};
