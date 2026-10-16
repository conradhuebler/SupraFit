// chatlog.cpp - The visible chat of the agent dock as data: entries, export/import, history files.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 (UI redesign WP9).
#include "chatlog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

#include <algorithm>

namespace {

QString timeString(const QDateTime& time)
{
    return time.toString(Qt::ISODateWithMs);
}

QDateTime parseTime(const QString& text)
{
    return QDateTime::fromString(text, Qt::ISODateWithMs);
}

/// Drop sub-millisecond parts so a saved and reloaded time compares equal.
QDateTime normalised(const QDateTime& time)
{
    const QDateTime t = time.isValid() ? time : QDateTime::currentDateTime();
    return QDateTime::fromMSecsSinceEpoch(t.toMSecsSinceEpoch(), t.timeZone());
}

const char* kindKey(ChatEntry::Kind kind)
{
    switch (kind) {
    case ChatEntry::Kind::User:
        return "user";
    case ChatEntry::Kind::Assistant:
        return "assistant";
    case ChatEntry::Kind::Tool:
        return "tool";
    case ChatEntry::Kind::Notice:
        return "notice";
    }
    return "notice";
}

bool kindFromKey(const QString& key, ChatEntry::Kind* kind)
{
    for (ChatEntry::Kind k : { ChatEntry::Kind::User, ChatEntry::Kind::Assistant, ChatEntry::Kind::Tool, ChatEntry::Kind::Notice }) {
        if (key == QLatin1String(kindKey(k))) {
            *kind = k;
            return true;
        }
    }
    return false;
}

void setError(QString* error, const QString& text)
{
    if (error)
        *error = text;
}

} // namespace

bool ChatEntry::operator==(const ChatEntry& other) const
{
    return kind == other.kind && text == other.text && time == other.time && toolName == other.toolName
        && toolArgs == other.toolArgs && toolOk == other.toolOk && toolEffect == other.toolEffect
        && toolResult == other.toolResult && toolTruncated == other.toolTruncated
        && toolDurationMs == other.toolDurationMs && approval == other.approval && decidedBy == other.decidedBy;
}

ChatLog::ChatLog()
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_created(normalised(QDateTime::currentDateTime()))
    , m_updated(m_created)
{
}

QString ChatLog::title() const
{
    for (const ChatEntry& entry : m_entries) {
        if (entry.kind != ChatEntry::Kind::User)
            continue;
        QString line = entry.text.section(QLatin1Char('\n'), 0, 0).simplified();
        if (line.size() > 60)
            line = line.left(57) + QStringLiteral("...");
        return line;
    }
    return QString();
}

ChatEntry& ChatLog::append(ChatEntry::Kind kind, const QString& text, const QDateTime& time)
{
    ChatEntry entry;
    entry.kind = kind;
    entry.text = text;
    entry.time = normalised(time);
    m_entries.append(entry);
    return m_entries.last();
}

ChatEntry& ChatLog::appendTool(const QString& name, const QJsonObject& args, bool ok, const QString& summary,
    const QDateTime& time)
{
    ChatEntry& entry = append(ChatEntry::Kind::Tool, summary, time);
    entry.toolName = name;
    entry.toolArgs = args;
    entry.toolOk = ok;
    return entry;
}

QString ChatLog::lastText(ChatEntry::Kind kind) const
{
    for (auto it = m_entries.crbegin(); it != m_entries.crend(); ++it) {
        if (it->kind == kind)
            return it->text;
    }
    return QString();
}

QString ChatLog::kindName(ChatEntry::Kind kind)
{
    return QString::fromLatin1(kindKey(kind));
}

QJsonObject ChatLog::toJson() const
{
    QJsonArray entries;
    for (const ChatEntry& entry : m_entries) {
        QJsonObject e;
        e.insert(QStringLiteral("kind"), QString::fromLatin1(kindKey(entry.kind)));
        e.insert(QStringLiteral("text"), entry.text);
        e.insert(QStringLiteral("time"), timeString(entry.time));
        if (entry.kind == ChatEntry::Kind::Tool) {
            e.insert(QStringLiteral("tool"), entry.toolName);
            e.insert(QStringLiteral("args"), entry.toolArgs);
            e.insert(QStringLiteral("ok"), entry.toolOk);
            if (!entry.toolEffect.isEmpty())
                e.insert(QStringLiteral("effect"), entry.toolEffect);
            if (!entry.toolResult.isEmpty())
                e.insert(QStringLiteral("result"), entry.toolResult);
            if (entry.toolTruncated)
                e.insert(QStringLiteral("truncated"), true);
            if (entry.toolDurationMs >= 0)
                e.insert(QStringLiteral("duration_ms"), double(entry.toolDurationMs));
            if (!entry.approval.isEmpty())
                e.insert(QStringLiteral("approval"), entry.approval);
            if (!entry.decidedBy.isEmpty())
                e.insert(QStringLiteral("decided_by"), entry.decidedBy);
        }
        entries.append(e);
    }
    QJsonObject object;
    object.insert(QStringLiteral("format"), QStringLiteral("suprafit-chat"));
    object.insert(QStringLiteral("version"), FormatVersion);
    object.insert(QStringLiteral("id"), m_id);
    object.insert(QStringLiteral("created"), timeString(m_created));
    object.insert(QStringLiteral("profile"), m_profile);
    object.insert(QStringLiteral("model"), m_model);
    object.insert(QStringLiteral("updated"), timeString(m_updated));
    if (!m_appVersion.isEmpty())
        object.insert(QStringLiteral("app_version"), m_appVersion);
    if (!m_gitCommit.isEmpty())
        object.insert(QStringLiteral("git_commit"), m_gitCommit);
    if (!m_endpointHost.isEmpty())
        object.insert(QStringLiteral("endpoint_host"), m_endpointHost);
    if (!m_autonomy.isEmpty())
        object.insert(QStringLiteral("autonomy"), m_autonomy);
    if (!m_projects.isEmpty())
        object.insert(QStringLiteral("projects"), QJsonArray::fromStringList(m_projects));
    object.insert(QStringLiteral("entries"), entries);
    object.insert(QStringLiteral("conversation"), m_conversation);
    return object;
}

bool ChatLog::fromJson(const QJsonObject& object, ChatLog* log, QString* error)
{
    if (!log)
        return false;
    if (object.value(QStringLiteral("format")).toString() != QStringLiteral("suprafit-chat")) {
        setError(error, QObject::tr("Not a SupraFit chat file"));
        return false;
    }
    const int version = object.value(QStringLiteral("version")).toInt(-1);
    if (version < 1 || version > FormatVersion) {
        setError(error, QObject::tr("Unsupported chat file version %1").arg(version));
        return false;
    }
    ChatLog result;
    result.m_id = object.value(QStringLiteral("id")).toString();
    result.m_created = parseTime(object.value(QStringLiteral("created")).toString());
    if (result.m_id.isEmpty() || !result.m_created.isValid()) {
        setError(error, QObject::tr("The chat file lacks its id or date"));
        return false;
    }
    result.m_profile = object.value(QStringLiteral("profile")).toString();
    result.m_model = object.value(QStringLiteral("model")).toString();
    result.m_updated = parseTime(object.value(QStringLiteral("updated")).toString());
    if (!result.m_updated.isValid())
        result.m_updated = result.m_created; // version 1
    result.m_appVersion = object.value(QStringLiteral("app_version")).toString();
    result.m_gitCommit = object.value(QStringLiteral("git_commit")).toString();
    result.m_endpointHost = object.value(QStringLiteral("endpoint_host")).toString();
    result.m_autonomy = object.value(QStringLiteral("autonomy")).toString();
    for (const QJsonValue& p : object.value(QStringLiteral("projects")).toArray())
        result.m_projects.append(p.toString());
    const QJsonValue entries = object.value(QStringLiteral("entries"));
    if (!entries.isArray()) {
        setError(error, QObject::tr("The chat file has no entries"));
        return false;
    }
    for (const QJsonValue& value : entries.toArray()) {
        const QJsonObject e = value.toObject();
        ChatEntry entry;
        if (!kindFromKey(e.value(QStringLiteral("kind")).toString(), &entry.kind)) {
            setError(error, QObject::tr("Unknown entry kind '%1'").arg(e.value(QStringLiteral("kind")).toString()));
            return false;
        }
        entry.text = e.value(QStringLiteral("text")).toString();
        entry.time = parseTime(e.value(QStringLiteral("time")).toString());
        if (entry.kind == ChatEntry::Kind::Tool) {
            entry.toolName = e.value(QStringLiteral("tool")).toString();
            entry.toolArgs = e.value(QStringLiteral("args")).toObject();
            entry.toolOk = e.value(QStringLiteral("ok")).toBool(true);
            entry.toolEffect = e.value(QStringLiteral("effect")).toString();
            entry.toolResult = e.value(QStringLiteral("result")).toString();
            entry.toolTruncated = e.value(QStringLiteral("truncated")).toBool(false);
            entry.toolDurationMs = qint64(e.value(QStringLiteral("duration_ms")).toDouble(-1));
            entry.approval = e.value(QStringLiteral("approval")).toString();
            entry.decidedBy = e.value(QStringLiteral("decided_by")).toString();
        }
        result.m_entries.append(entry);
    }
    result.m_conversation = object.value(QStringLiteral("conversation")).toArray();
    *log = result;
    return true;
}

QString ChatLog::toMarkdown() const
{
    QString md;
    const QString title = this->title();
    md += QStringLiteral("# SupraFit chat") + (title.isEmpty() ? QString() : QStringLiteral(": ") + title) + QStringLiteral("\n\n");
    md += QStringLiteral("*Started ") + m_created.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    if (!m_profile.isEmpty())
        md += QStringLiteral(" · profile ") + m_profile;
    if (!m_model.isEmpty())
        md += QStringLiteral(" · model ") + m_model;
    md += QStringLiteral("*\n\n");

    for (const ChatEntry& entry : m_entries) {
        const QString time = entry.time.toString(QStringLiteral("HH:mm:ss"));
        switch (entry.kind) {
        case ChatEntry::Kind::User:
            md += QStringLiteral("## You · ") + time + QStringLiteral("\n\n") + entry.text.trimmed() + QStringLiteral("\n\n");
            break;
        case ChatEntry::Kind::Assistant:
            md += QStringLiteral("## Assistant · ") + time + QStringLiteral("\n\n") + entry.text.trimmed() + QStringLiteral("\n\n");
            break;
        case ChatEntry::Kind::Tool: {
            const QString args = QString::fromUtf8(QJsonDocument(entry.toolArgs).toJson(QJsonDocument::Compact));
            md += QStringLiteral("> tool `") + entry.toolName + QStringLiteral("` ") + args
                + (entry.toolOk ? QStringLiteral(" - ok") : QStringLiteral(" - failed"));
            if (!entry.approval.isEmpty())
                md += QStringLiteral(" [") + entry.approval + (entry.decidedBy.isEmpty() ? QString() : QStringLiteral(" by ") + entry.decidedBy) + QStringLiteral("]");
            if (!entry.text.isEmpty())
                md += QStringLiteral(": ") + QString(entry.text).replace(QLatin1Char('\n'), QLatin1Char(' '));
            md += QStringLiteral(" (") + time + QStringLiteral(")\n\n");
            break;
        }
        case ChatEntry::Kind::Notice:
            md += QStringLiteral("> *") + QString(entry.text).replace(QLatin1Char('\n'), QLatin1Char(' ')) + QStringLiteral("* (") + time + QStringLiteral(")\n\n");
            break;
        }
    }
    return md;
}

bool ChatLog::save(const QString& path, QString* error) const
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(error, file.errorString());
        return false;
    }
    file.write(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        setError(error, file.errorString());
        return false;
    }
    return true;
}

bool ChatLog::load(const QString& path, ChatLog* log, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(error, file.errorString());
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(error, QObject::tr("Damaged chat file: %1").arg(parseError.errorString()));
        return false;
    }
    return fromJson(document.object(), log, error);
}

bool ChatLog::operator==(const ChatLog& other) const
{
    return m_id == other.m_id && m_created == other.m_created && m_profile == other.m_profile
        && m_model == other.m_model && m_updated == other.m_updated && m_appVersion == other.m_appVersion
        && m_gitCommit == other.m_gitCommit && m_endpointHost == other.m_endpointHost
        && m_autonomy == other.m_autonomy && m_projects == other.m_projects && m_entries == other.m_entries
        && m_conversation == other.m_conversation;
}

QString ChatLog::capResult(const QString& text, bool* truncated)
{
    if (text.size() <= MaxToolResultChars) {
        if (truncated)
            *truncated = false;
        return text;
    }
    if (truncated)
        *truncated = true;
    return text.left(MaxToolResultChars) + QStringLiteral("\n[truncated: %1 of %2 characters kept]").arg(MaxToolResultChars).arg(text.size());
}

QJsonObject ChatLog::toProjectBlock(const QVector<ChatLog>& chats)
{
    QJsonArray array;
    for (const ChatLog& chat : chats)
        array.append(chat.toJson());
    QJsonObject block;
    block.insert(QStringLiteral("format"), QStringLiteral("suprafit-agent-chats"));
    block.insert(QStringLiteral("version"), 1);
    block.insert(QStringLiteral("chats"), array);
    return block;
}

QVector<ChatLog> ChatLog::fromProjectBlock(const QJsonValue& value, QStringList* warnings)
{
    QVector<ChatLog> chats;
    auto warn = [warnings](const QString& text) {
        if (warnings)
            warnings->append(text);
    };
    if (value.isUndefined() || value.isNull())
        return chats;
    const QJsonObject block = value.toObject();
    if (block.value(QStringLiteral("format")).toString() != QStringLiteral("suprafit-agent-chats")) {
        warn(QObject::tr("The agent chats of this project are not readable (unknown format); they were skipped."));
        return chats;
    }
    if (block.value(QStringLiteral("version")).toInt() > 1)
        warn(QObject::tr("The agent chats were written by a newer SupraFit; reading what is readable."));
    int index = 0;
    for (const QJsonValue& chat : block.value(QStringLiteral("chats")).toArray()) {
        ChatLog log;
        QString error;
        if (ChatLog::fromJson(chat.toObject(), &log, &error))
            chats.append(log);
        else
            warn(QObject::tr("Agent chat %1 of this project was skipped: %2").arg(index + 1).arg(error));
        ++index;
    }
    return chats;
}

// ---------------------------------------------------------------------------------------------

ChatHistoryStore::ChatHistoryStore(const QString& directory, int cap)
    : m_directory(directory)
    , m_cap(qMax(1, cap))
{
}

QString ChatHistoryStore::defaultDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/chats");
}

bool ChatHistoryStore::save(const ChatLog& log, QString* error) const
{
    if (log.isEmpty())
        return true;
    if (!QDir().mkpath(m_directory)) {
        setError(error, QObject::tr("Cannot create %1").arg(m_directory));
        return false;
    }
    if (!log.save(m_directory + QLatin1Char('/') + log.id() + QStringLiteral(".json"), error))
        return false;
    prune();
    return true;
}

QVector<ChatHistoryStore::Item> ChatHistoryStore::list() const
{
    QVector<Item> items;
    const QDir dir(m_directory);
    for (const QFileInfo& info : dir.entryInfoList({ QStringLiteral("*.json") }, QDir::Files)) {
        ChatLog log;
        if (!ChatLog::load(info.absoluteFilePath(), &log))
            continue;
        items.append({ info.absoluteFilePath(), log.title(), log.created() });
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.created > b.created; });
    return items;
}

void ChatHistoryStore::prune() const
{
    // over the cap, damaged files are dropped first, then the oldest chats (by file time)
    QDir dir(m_directory);
    QFileInfoList files = dir.entryInfoList({ QStringLiteral("*.json") }, QDir::Files, QDir::Time);
    if (files.size() > m_cap) {
        QFileInfoList readable;
        for (const QFileInfo& info : files) {
            ChatLog probe;
            if (ChatLog::load(info.absoluteFilePath(), &probe))
                readable.append(info);
            else
                QFile::remove(info.absoluteFilePath());
        }
        files = readable;
    }
    while (files.size() > m_cap) {
        QFile::remove(files.last().absoluteFilePath());
        files.removeLast();
    }
}

// ---------------------------------------------------------------------------------------------

InputHistory::InputHistory(int cap)
    : m_cap(qMax(1, cap))
{
}

void InputHistory::setItems(const QStringList& items)
{
    m_items = items;
    while (m_items.size() > m_cap)
        m_items.removeFirst();
    m_index = -1;
}

void InputHistory::push(const QString& text)
{
    m_index = -1;
    if (text.trimmed().isEmpty() || (!m_items.isEmpty() && m_items.last() == text))
        return;
    m_items.append(text);
    while (m_items.size() > m_cap)
        m_items.removeFirst();
}

QString InputHistory::previous(const QString& current)
{
    if (m_items.isEmpty())
        return current;
    if (m_index < 0) {
        m_draft = current;
        m_index = m_items.size() - 1;
    } else if (m_index > 0) {
        --m_index;
    }
    return m_items.at(m_index);
}

QString InputHistory::next()
{
    if (m_index < 0)
        return m_draft;
    if (m_index < m_items.size() - 1) {
        ++m_index;
        return m_items.at(m_index);
    }
    m_index = -1;
    return m_draft;
}
