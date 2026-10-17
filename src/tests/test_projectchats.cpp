/*
 * test_projectchats.cpp - Agent chats stored in project files; the audit trail of a session.
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Claude Generated 2026 (UI redesign WP11). Real reference project, local stub server, no
 * network. No raw string literals in this file (moc trap, see test_llmclient.cpp).
 */

#include <QtTest/QtTest>

#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>

#include <cutechat/chatlog.h>
#include <cutechat/chatrecorder.h>
#include "src/agent/projectchats.h"
#include <cutechat/llmclient.h>
#include <cutechat/llmsession.h>
#include <cutechat/tooldispatcher.h>
#include <cutechat/toolregistry.h>
#include "src/core/jsonhandler.h"
#include "src/core/projectmanager.h"

#include "test_stubserver.h"

#ifndef SUPRAFIT_SOURCE_DIR
#define SUPRAFIT_SOURCE_DIR "."
#endif

class ProjectChatsTest : public QObject {
    Q_OBJECT

    static QString reference() { return QStringLiteral(SUPRAFIT_SOURCE_DIR) + QStringLiteral("/data/samples/reference/simulated_1_1.json"); }

    static ChatLog auditedChat(const QString& projectId)
    {
        ChatLog chat;
        chat.setProfile(QStringLiteral("local"));
        chat.setModel(QStringLiteral("glm-5.2:cloud"));
        chat.setEndpointHost(QStringLiteral("127.0.0.1"));
        chat.setAutonomy(QStringLiteral("ask"));
        chat.setAppVersion(QStringLiteral("2.x"), QStringLiteral("abc123"));
        chat.addContextId(projectId);
        chat.append(ChatEntry::Kind::User, QStringLiteral("Fit the 1:1 model"));
        ChatEntry& tool = chat.appendTool(QStringLiteral("fit_model"), QJsonObject{ { QStringLiteral("project_id"), projectId } }, true, QStringLiteral("converged"));
        tool.toolEffect = QStringLiteral("compute");
        tool.toolResult = ChatLog::capResult(QString(ChatLog::MaxToolResultChars + 10, QLatin1Char('x')), &tool.toolTruncated);
        tool.toolDurationMs = 1234;
        tool.approval = QStringLiteral("approved");
        tool.decidedBy = QStringLiteral("user");
        chat.append(ChatEntry::Kind::Assistant, QStringLiteral("| Model | SSE |\n|---|---|\n| 1:1 | 0.0127 |"));
        return chat;
    }

    /// The ids of the projects loaded now that were not loaded before.
    static QString loadNew(const QString& path)
    {
        const QStringList before = SupraFit::ProjectManager::instance().getLoadedProjectIds();
        if (!SupraFit::ProjectManager::instance().loadProject(path))
            return QString();
        for (const QString& id : SupraFit::ProjectManager::instance().getLoadedProjectIds())
            if (!before.contains(id))
                return id;
        return SupraFit::ProjectManager::instance().getLoadedProjectIds().value(0);
    }

    static QJsonObject readFile(const QString& path)
    {
        QJsonObject object;
        JsonHandler::ReadJsonFile(object, path);
        return object;
    }

private slots:
    void initTestCase()
    {
        ChatLog::setFormatPrefix(QStringLiteral("suprafit")); // SupraFit writes its own tags (projectchats.cpp)
    }

    void auditFieldsRoundTripAndCap()
    {
        const ChatLog chat = auditedChat(QStringLiteral("{p}"));
        const ChatEntry& tool = chat.entries().at(1);
        QVERIFY(tool.toolTruncated);
        QVERIFY(tool.toolResult.contains(QStringLiteral("[truncated:")));
        QVERIFY(tool.toolResult.size() < ChatLog::MaxToolResultChars + 100);

        const QJsonObject block = ChatLog::toBlock({ chat });
        QStringList warnings;
        const QVector<ChatLog> back = ChatLog::fromBlock(block, &warnings);
        QVERIFY(warnings.isEmpty());
        QCOMPARE(back.size(), 1);
        QVERIFY(back.first() == chat);
        QCOMPARE(back.first().entries().at(1).approval, QStringLiteral("approved"));
        QCOMPARE(back.first().entries().at(1).toolDurationMs, qint64(1234));
        QCOMPARE(back.first().contextIds(), QStringList({ QStringLiteral("{p}") }));

        // nothing secret: no key fields anywhere
        const QString text = QString::fromUtf8(QJsonDocument(block).toJson());
        QVERIFY(!text.contains(QStringLiteral("api_key")));
        QVERIFY(!text.contains(QStringLiteral("_API_KEY")));

        // version 1 chats (no audit fields) still load
        QJsonObject v1 = chat.toJson();
        v1.insert(QStringLiteral("version"), 1);
        v1.remove(QStringLiteral("updated"));
        ChatLog old;
        QVERIFY(ChatLog::fromJson(v1, &old));
        QCOMPARE(old.updated(), old.created());
    }

    void damagedAndFutureBlocksAreTolerated()
    {
        QJsonObject block = ChatLog::toBlock({ auditedChat(QStringLiteral("{p}")) });
        QJsonArray chats = block.value(QStringLiteral("chats")).toArray();
        chats.append(QJsonObject{ { QStringLiteral("format"), QStringLiteral("suprafit-chat") }, { QStringLiteral("version"), 99 } });
        chats.append(QStringLiteral("garbage"));
        block.insert(QStringLiteral("chats"), chats);
        block.insert(QStringLiteral("version"), 7);
        QStringList warnings;
        const QVector<ChatLog> back = ChatLog::fromBlock(block, &warnings);
        QCOMPARE(back.size(), 1);
        QCOMPARE(warnings.size(), 3); // newer block + two skipped chats

        QVERIFY(ChatLog::fromBlock(QJsonValue(QStringLiteral("nonsense")), &warnings).isEmpty());
        QVERIFY(ChatLog::fromBlock(QJsonValue(), nullptr).isEmpty());
    }

    void projectSaveLoadKeepsChats()
    {
        QVERIFY(QFile::exists(reference()));
        // an old file without the key loads and has no chats
        const QString id = loadNew(reference());
        QVERIFY(!id.isEmpty());
        QVERIFY(ProjectChats::load(id).isEmpty());
        QVERIFY(!SupraFit::ProjectManager::instance().hasUnsavedAttachments(id));

        const ChatLog chat = auditedChat(id);
        ProjectChats::store(id, chat);
        QVERIFY(SupraFit::ProjectManager::instance().hasUnsavedAttachments(id));
        ProjectChats::store(id, chat); // same chat again: replaced, not duplicated
        QCOMPARE(ProjectChats::load(id).size(), 1);

        QTemporaryDir dir;
        for (const QString& name : { QStringLiteral("p.suprafit"), QStringLiteral("p.json") }) {
            const QString path = dir.filePath(name);
            QVERIFY(SupraFit::ProjectManager::instance().saveProject(path, id));
            QVERIFY(!SupraFit::ProjectManager::instance().hasUnsavedAttachments(id));
            const QJsonObject file = readFile(path);
            QVERIFY(file.contains(QStringLiteral("data")));
            QVERIFY(file.contains(QStringLiteral("model_0")) || file.contains(QStringLiteral("model_1")));
            QVERIFY(file.contains(ChatLog::blockKey()));

            SupraFit::ProjectManager::instance().removeProject(id);
            const QString again = loadNew(path);
            QCOMPARE(again, id); // same project uuid
            const QVector<ChatLog> chats = ProjectChats::load(again);
            QCOMPARE(chats.size(), 1);
            QVERIFY(chats.first() == chat);
        }

        // an unknown block of a newer version survives a load -> save round trip untouched
        SupraFit::ProjectManager::instance().setProjectAttachment(id, QStringLiteral("future_block"), QJsonObject{ { QStringLiteral("x"), 1 } });
        const QString path = dir.filePath(QStringLiteral("future.json"));
        QVERIFY(SupraFit::ProjectManager::instance().saveProject(path, id));
        SupraFit::ProjectManager::instance().removeProject(id);
        QCOMPARE(loadNew(path), id);
        QCOMPARE(SupraFit::ProjectManager::instance().projectAttachment(id, QStringLiteral("future_block")).toObject().value(QStringLiteral("x")).toInt(), 1);

        // a damaged chat block: the project still loads, the chats are skipped
        QJsonObject damaged = readFile(path);
        damaged.insert(ChatLog::blockKey(), QStringLiteral("not a block"));
        const QString damagedPath = dir.filePath(QStringLiteral("damaged.json"));
        {
            QFile f(damagedPath);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(QJsonDocument(damaged).toJson());
        }
        SupraFit::ProjectManager::instance().removeProject(id);
        QCOMPARE(loadNew(damagedPath), id);
        QStringList warnings;
        QVERIFY(ProjectChats::load(id, &warnings).isEmpty());
        QCOMPARE(warnings.size(), 1);

        ProjectChats::clear(id);
        QVERIFY(SupraFit::ProjectManager::instance().projectAttachment(id, ChatLog::blockKey()).isUndefined());
        SupraFit::ProjectManager::instance().removeProject(id);
    }

    void recorderWritesTheAuditTrail()
    {
        ToolRegistry registry;
        ToolDispatcher dispatcher(&registry);
        {
            ToolSpec spec;
            spec.name = QStringLiteral("read_it");
            spec.description = QStringLiteral("read");
            spec.category = QStringLiteral("test");
            spec.effect = ToolEffect::Read;
            spec.paramSchema = QJsonDocument::fromJson(QByteArray("{\"type\":\"object\",\"properties\":{\"project_id\":{\"type\":\"string\"}}}")).object();
            spec.handler = [](const QJsonObject&) { return ToolResult::success(QJsonObject{ { QStringLiteral("value"), 42 } }); };
            QVERIFY(registry.add(spec));
        }
        {
            ToolSpec spec;
            spec.name = QStringLiteral("change_it");
            spec.description = QStringLiteral("mutate");
            spec.category = QStringLiteral("test");
            spec.effect = ToolEffect::Mutate;
            spec.paramSchema = QJsonDocument::fromJson(QByteArray("{\"type\":\"object\"}")).object();
            spec.handler = [](const QJsonObject&) { return ToolResult::success(QJsonObject(), QStringLiteral("changed")); };
            QVERIFY(registry.add(spec));
        }

        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("m");
        profile.stream = false;
        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &registry, &dispatcher);
        session.setAutonomy(ToolAutonomy::Ask);

        ChatLog chat;
        ChatRecorder recorder(&session, &registry);
        recorder.setContextArguments({ QStringLiteral("project_id") });
        recorder.setLog(&chat);
        recorder.setAutonomy(ToolAutonomy::Ask);
        bool answer = true;
        session.setApprovalPolicy(recorder.wrapApproval([&answer](const ToolSpec&, const QJsonObject&) { return answer; }, QStringLiteral("user")));

        auto call = [](const char* name, const char* id, const QByteArray& args) {
            QByteArray escaped(args);
            escaped.replace("\"", "\\\"");
            return QByteArray("{\"choices\":[{\"message\":{\"role\":\"assistant\",\"content\":null,\"tool_calls\":[{\"id\":\"") + id
                + QByteArray("\",\"type\":\"function\",\"function\":{\"name\":\"") + name + QByteArray("\",\"arguments\":\"") + escaped
                + QByteArray("\"}}]}}]}");
        };
        const QByteArray done("{\"choices\":[{\"message\":{\"role\":\"assistant\",\"content\":\"done\"}}]}");
        server.enqueue(call("read_it", "c1", QByteArray("{\"project_id\":\"{proj}\"}")));
        server.enqueue(call("change_it", "c2", QByteArray("{}")));
        server.enqueue(done);

        QEventLoop loop;
        connect(&session, &LlmSession::finished, &loop, &QEventLoop::quit);
        connect(&session, &LlmSession::failed, &loop, &QEventLoop::quit);
        QTimer::singleShot(8000, &loop, &QEventLoop::quit);
        session.ask(QStringLiteral("go"));
        loop.exec();

        QCOMPARE(chat.entries().size(), 2);
        const ChatEntry& read = chat.entries().at(0);
        QCOMPARE(read.toolName, QStringLiteral("read_it"));
        QCOMPARE(read.toolEffect, QStringLiteral("read"));
        QCOMPARE(read.approval, QStringLiteral("not_required"));
        QVERIFY(read.decidedBy.startsWith(QStringLiteral("autonomy:")));
        QVERIFY(read.toolResult.contains(QStringLiteral("42")));
        QVERIFY(read.toolDurationMs >= 0);
        const ChatEntry& change = chat.entries().at(1);
        QCOMPARE(change.toolEffect, QStringLiteral("mutate"));
        QCOMPARE(change.approval, QStringLiteral("approved"));
        QCOMPARE(change.decidedBy, QStringLiteral("user"));
        QVERIFY(change.toolOk);
        QCOMPARE(chat.contextIds(), QStringList({ QStringLiteral("{proj}") }));

        // refused by the user
        answer = false;
        server.enqueue(call("change_it", "c3", QByteArray("{}")));
        server.enqueue(done);
        session.ask(QStringLiteral("again"));
        loop.exec();
        const ChatEntry& refused = chat.entries().last();
        QCOMPARE(refused.toolName, QStringLiteral("change_it"));
        QVERIFY(!refused.toolOk);
        QCOMPARE(refused.approval, QStringLiteral("refused"));
        QCOMPARE(refused.decidedBy, QStringLiteral("user"));
    }
};

QTEST_GUILESS_MAIN(ProjectChatsTest)
#include "test_projectchats.moc"
