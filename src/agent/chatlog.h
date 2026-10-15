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

    bool operator==(const ChatEntry& other) const;
};

class ChatLog {
public:
    static constexpr int FormatVersion = 1;

    ChatLog();

    QString id() const { return m_id; }
    QDateTime created() const { return m_created; }
    QString profile() const { return m_profile; }
    QString model() const { return m_model; }
    void setProfile(const QString& profile) { m_profile = profile; }
    void setModel(const QString& model) { m_model = model; }

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

private:
    QString m_id;
    QDateTime m_created;
    QString m_profile, m_model;
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
