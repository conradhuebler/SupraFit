// chatdock.h - Chat dock for the LLM analysis agent.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Same agent as `suprafit_cli --agent`, with a transcript,
// a profile and an autonomy selector, and a confirmation dialog for tools that the
// chosen autonomy does not allow to run unasked. Works on the projects held by the
// ProjectManager, i.e. the ones open in the GUI.
#pragma once

#include <QDockWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTextBrowser;

class LlmClient;
class LlmSession;
class ToolDispatcher;
class ToolRegistry;

class ChatDock : public QDockWidget {
    Q_OBJECT

public:
    explicit ChatDock(QWidget* parent = nullptr);
    ~ChatDock() override;

private slots:
    void send();
    void stop();
    void reloadProfiles();

private:
    bool prepareClient();
    void setRunning(bool running);
    void appendLine(const QString& html);

    QComboBox *m_profile, *m_autonomy;
    QTextBrowser* m_transcript;
    QLineEdit* m_input;
    QPushButton *m_send, *m_stop;
    QLabel* m_status;

    ToolRegistry* m_registry;
    ToolDispatcher* m_dispatcher;
    LlmClient* m_client;
    LlmSession* m_session;
    bool m_streamed = false;
    bool m_atLineStart = true;
};
