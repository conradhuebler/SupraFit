// chatdock.h - Chat dock for the LLM analysis agent.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Same agent as `suprafit_cli --agent`, with a chat transcript,
// a profile, model and autonomy selector, and a confirmation dialog for tools that the
// chosen autonomy does not allow to run unasked. Works on the projects held by the
// ProjectManager, i.e. the ones open in the GUI.
//
// UI redesign WP9: the transcript is a list of message widgets (questions right, answers
// left as bubbles with Markdown, tool calls as compact lines), the input is a growing
// multi-line field with history, chats are saved automatically and can be reopened,
// exported (Markdown, JSON) and imported. The data side lives in src/agent/chatlog.h.
#pragma once

#include <QDockWidget>
#include <QRect>
#include <QString>

#include "src/agent/chatlog.h"

class QComboBox;
class QLabel;
class QMenu;
class QPushButton;
class QTimer;
class QToolButton;

class ChatInput;
class ChatView;
class LlmClient;
class LlmSession;
class ToolDispatcher;
class ToolRegistry;

class ChatDock : public QDockWidget {
    Q_OBJECT

public:
    explicit ChatDock(QWidget* parent = nullptr);
    ~ChatDock() override;

    /// Show a saved chat (history or import file) and continue its conversation.
    bool openChat(const QString& path, QString* error = nullptr);
    /// Send @p text as if typed (used by the capture tooling as well).
    void askQuestion(const QString& text);
    /// Select the autonomy level by index (0 ask, 1 run in program, 2 full).
    void setAutonomyIndex(int index);
    /// The "more" menu (new chat, history, export, import, ...).
    QMenu* actionsMenu() const { return m_menu; }
    QMenu* historyMenu() const { return m_historyMenu; }
    bool isMaximized() const { return m_maximized; }

public slots:
    void toggleMaximized();
    void newChat();

private slots:
    void send();
    void stop();
    void reloadProfiles();
    void exportMarkdown();
    void exportJson();
    void importJson();
    void copyLastAnswer();
    void fillHistoryMenu();

private:
    bool prepareClient();
    void setRunning(bool running);
    void addNotice(const QString& text, bool error = false, bool store = true);
    void flushStream();
    void finishTurn();
    void autosave();
    void showLog(const ChatLog& log);
    QString lastDirectory() const;
    void rememberDirectory(const QString& file);

    QComboBox *m_profile, *m_model, *m_autonomy;
    QPushButton *m_send, *m_stop, *m_refresh;
    QToolButton *m_menuButton, *m_maxButton;
    QMenu *m_menu, *m_historyMenu;
    QLabel* m_status;
    ChatView* m_view;
    ChatInput* m_input;

    ToolRegistry* m_registry;
    ToolDispatcher* m_dispatcher;
    LlmClient* m_client;
    LlmSession* m_session;

    ChatLog m_log;
    ChatHistoryStore m_store;
    InputHistory m_inputHistory;

    // streaming: chunks are collected and shown at most every ~90 ms
    QTimer* m_streamTimer;
    QString m_streamText;
    bool m_streamOpen = false;
    int m_pendingTool = -1; ///< index in m_log of the tool line waiting for its result

    bool m_maximized = false;
    bool m_wasFloating = false;
    QRect m_previousGeometry;
};
