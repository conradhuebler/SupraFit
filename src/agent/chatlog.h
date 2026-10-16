// chatlog.h - The visible chat of the agent dock as data: entries, export/import, history files.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 (UI redesign WP9). GUI-free, so the export/import round trip, the
// history store and the input history can be tested headless (src/tests/test_chatlog.cpp).
//
// A ChatLog holds two things: the entries the user saw (questions, answers, tool lines,
// notices, each with a time stamp) and the LlmSession message list, so a reopened chat can be
// continued. Neither carries an API key; the profile and model *names* are kept for the record.
#pragma once

#include <QDateTime>
#include <QTimeZone>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

/// One line of the visible transcript.
struct ChatEntry {
    enum class Kind {
        User,
        Assistant,
        Tool,
        Notice
    };
    Kind kind = Kind::Notice;
    QString text;          ///< message text (markdown for the assistant); notice text; tool: result summary
    QDateTime time;
    QString toolName;      ///< Tool only
    QJsonObject toolArgs;  ///< Tool only
    bool toolOk = true;    ///< Tool only
    // audit trail of a tool call (format version 2; all optional)
    QString toolEffect;    ///< read / display / mutate / compute / file_write / process
    QString toolResult;    ///< full result (text or JSON), capped at MaxToolResultChars
    bool toolTruncated = false;
    qint64 toolDurationMs = -1; ///< -1 = not measured
    QString approval;      ///< "", "not_required", "approved", "refused"
    QString decidedBy;     ///< "user", "policy", "autonomy:<level>", "session"

    bool operator==(const ChatEntry& other) const;
};

class ChatLog {
public:
    /// 1: entries + conversation; 2: audit fields (tool results, durations, approvals),
    /// app version, endpoint host, autonomy, project uuids. Version 1 files still load.
    static constexpr int FormatVersion = 2;
    /// Cap for one stored tool result (characters); beyond it the text is cut and marked.
    static constexpr int MaxToolResultChars = 65536;
    /// Key of the conversations block in a project file.
    static QString projectKey() { return QStringLiteral("agent_chats"); }

    ChatLog();

    QString id() const { return m_id; }
    QDateTime created() const { return m_created; }
    QString profile() const { return m_profile; }
    QString model() const { return m_model; }
    void setProfile(const QString& profile) { m_profile = profile; }
    void setModel(const QString& model) { m_model = model; }
    QDateTime updated() const { return m_updated; }
    void touch()
    {
        const QDateTime now = QDateTime::currentDateTime();
        m_updated = QDateTime::fromMSecsSinceEpoch(now.toMSecsSinceEpoch(), now.timeZone());
    }
    QString appVersion() const { return m_appVersion; }
    QString gitCommit() const { return m_gitCommit; }
    void setAppVersion(const QString& version, const QString& commit)
    {
        m_appVersion = version;
        m_gitCommit = commit;
    }
    /// Host of the endpoint (never a key or a full URL with credentials).
    QString endpointHost() const { return m_endpointHost; }
    void setEndpointHost(const QString& host) { m_endpointHost = host; }
    QString autonomy() const { return m_autonomy; }
    void setAutonomy(const QString& autonomy) { m_autonomy = autonomy; }
    /// Uuids of the projects this chat belongs to or touched (first = where it started).
    QStringList projectUuids() const { return m_projects; }
    void addProjectUuid(const QString& uuid)
    {
        if (!uuid.isEmpty() && !m_projects.contains(uuid))
            m_projects.append(uuid);
    }

    const QVector<ChatEntry>& entries() const { return m_entries; }
    bool isEmpty() const { return m_entries.isEmpty(); }
    /// First line of the first question (for the history list), shortened.
    QString title() const;

    ChatEntry& append(ChatEntry::Kind kind, const QString& text, const QDateTime& time = QDateTime());
    ChatEntry& appendTool(const QString& name, const QJsonObject& args, bool ok, const QString& summary,
        const QDateTime& time = QDateTime());
    /// Text of the last entry of @p kind (empty if there is none).
    QString lastText(ChatEntry::Kind kind) const;
    ChatEntry* lastEntry() { return m_entries.isEmpty() ? nullptr : &m_entries.last(); }
    ChatEntry* entry(int index) { return index >= 0 && index < m_entries.size() ? &m_entries[index] : nullptr; }

    /// LlmSession::conversation() of this chat, kept so a reopened chat can go on.
    QJsonArray conversation() const { return m_conversation; }
    void setConversation(const QJsonArray& conversation) { m_conversation = conversation; }

    /// Lossless JSON (version, id, timestamps, profile/model name, entries, conversation).
    QJsonObject toJson() const;
    /// Restore from toJson(); false with @p error for a wrong or damaged document.
    static bool fromJson(const QJsonObject& object, ChatLog* log, QString* error = nullptr);
    /// Readable Markdown: roles with time stamps, the assistant text as is (tables stay
    /// tables), tool calls as quoted lines.
    QString toMarkdown() const;

    bool save(const QString& path, QString* error = nullptr) const;
    static bool load(const QString& path, ChatLog* log, QString* error = nullptr);

    bool operator==(const ChatLog& other) const;

    static QString kindName(ChatEntry::Kind kind);

    /// Cut @p text to MaxToolResultChars and append a marker; @p truncated reports it.
    static QString capResult(const QString& text, bool* truncated);

    /// The conversations block stored in a project file under projectKey().
    static QJsonObject toProjectBlock(const QVector<ChatLog>& chats);
    /// Read a block written by toProjectBlock(). Damaged chats are skipped with a line in
    /// @p warnings; a newer block version is read as far as possible.
    static QVector<ChatLog> fromProjectBlock(const QJsonValue& block, QStringList* warnings = nullptr);

private:
    QString m_id;
    QDateTime m_created;
    QString m_profile, m_model;
    QDateTime m_updated;
    QString m_appVersion, m_gitCommit, m_endpointHost, m_autonomy;
    QStringList m_projects;
    QVector<ChatEntry> m_entries;
    QJsonArray m_conversation;
};

/// Saved chats in one directory (one JSON file per chat, newest kept, at most a cap).
class ChatHistoryStore {
public:
    struct Item {
        QString path;
        QString title;
        QDateTime created;
    };

    explicit ChatHistoryStore(const QString& directory = defaultDirectory(), int cap = 50);
    /// <AppDataLocation>/chats
    static QString defaultDirectory();

    QString directory() const { return m_directory; }
    int cap() const { return m_cap; }

    /// Write @p log as <id>.json (replacing an earlier save of the same chat) and remove the
    /// oldest files beyond the cap. Empty chats are not stored.
    bool save(const ChatLog& log, QString* error = nullptr) const;
    /// Readable chats, newest first; damaged files are skipped.
    QVector<Item> list() const;
    void prune() const;

private:
    QString m_directory;
    int m_cap;
};

/// Earlier inputs of the chat field, newest last, for Up/Down recall.
class InputHistory {
public:
    explicit InputHistory(int cap = 100);

    void setItems(const QStringList& items);
    QStringList items() const { return m_items; }
    int cap() const { return m_cap; }

    /// Store a sent input (skips empty text and an immediate repeat) and end browsing.
    void push(const QString& text);
    /// Step back (older) / forward (newer). @p current is what the field holds when browsing
    /// starts; stepping past the newest item returns it again.
    QString previous(const QString& current);
    QString next();
    bool isBrowsing() const { return m_index >= 0; }
    void resetBrowsing() { m_index = -1; }

private:
    QStringList m_items;
    int m_cap;
    int m_index = -1;
    QString m_draft;
};
