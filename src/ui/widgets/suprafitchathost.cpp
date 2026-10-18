// suprafitchathost.cpp - see suprafitchathost.h
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026
#include "suprafitchathost.h"

#include "src/agent/agenthelp.h"
#include "src/agent/agentrunner.h"
#include "src/agent/tools_analysis.h"
#include "src/agent/tools_inspect.h"
#include "src/agent/tools_project.h"
#include "src/core/projectmanager.h"
#include "src/version.h"

#include <QApplication>
#include <QJsonDocument>
#include <QMessageBox>

SupraFitChatHost::SupraFitChatHost()
{
    ChatLog::setFormatPrefix(QStringLiteral("suprafit")); // stored projects and exported chats keep their tags
}

QString SupraFitChatHost::systemPrompt() const
{
    return agentSystemPrompt();
}

void SupraFitChatHost::registerTools(ToolRegistry& registry)
{
    registerProjectTools(registry, &SupraFit::ProjectManager::instance());
    registerAnalysisTools(registry, &SupraFit::ProjectManager::instance());
    registerInspectTools(registry, &SupraFit::ProjectManager::instance());
}

QString SupraFitChatHost::helpText() const
{
    return agentHelpText();
}

bool SupraFitChatHost::storeInBackend() const
{
    return qApp->property("store_agent_chats").isValid() ? qApp->property("store_agent_chats").toBool() : true;
}

QString SupraFitChatHost::appVersion() const
{
    return git_tag;
}

QString SupraFitChatHost::appCommit() const
{
    return git_commit_hash;
}

ChatUiTexts SupraFitChatHost::uiTexts() const
{
    ChatUiTexts t;
    t.dockTitle = QObject::tr("Agent");
    t.inputPlaceholder = QObject::tr("Ask about the open projects…");
    t.askLabel = QObject::tr("Ask for fits");
    t.chatFileLabel = QObject::tr("SupraFit chat");
    t.approvalTitle = QObject::tr("Agent");
    t.noContextNotice = QObject::tr("No project open: this chat is not stored in a project file.");
    t.thisContextHistory = QObject::tr("This project");
    t.noChatsInContext = QObject::tr("No chats stored in this project");
    t.removeChatsAction = QObject::tr("Remove chats from this project…");
    t.removeChatsTip = QObject::tr("Project files contain the agent conversations; remove them before sharing a project if needed");
    t.removeChatsQuestion = QObject::tr("Remove all agent chats from this project? They disappear from the project file when it is saved next time; copies stay in the History under \"Other chats\".");
    t.chatsRemovedNotice = QObject::tr("The chats of this project were removed (save the project to make it permanent).");
    t.restoredFromContextNotice = QObject::tr("Chat of %1, stored with this project. The project state is the one saved with it; the conversation can be continued.");
    t.restoredFromHistoryNotice = QObject::tr("Chat of %1 restored from the history. It may belong to another project; ids mentioned above may refer to projects that are not open.");
    t.remoteEndpointNotice = QObject::tr("Remote endpoint %1: the conversation and the project data the agent reads are sent there.");
    t.remoteIdentityNotice = QObject::tr("Remote endpoint: questions and project data go to %1");
    t.noToolSupportWarning = QObject::tr("%1 reports no tool support; the agent needs tools to read the projects.");
    return t;
}

bool SupraFitChatHost::approveToolCall(const ToolSpec& spec, const QJsonObject& args, QWidget* parent)
{
    const QString text = QObject::tr("The agent wants to run '%1'.\n\n%2\n\nAllow?")
                             .arg(spec.name, QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact)));
    return QMessageBox::question(parent, QObject::tr("Agent"), text) == QMessageBox::Yes;
}
