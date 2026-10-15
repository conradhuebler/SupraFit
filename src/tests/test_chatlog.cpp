/*
 * test_chatlog.cpp - ChatLog export/import, history store and input history (headless).
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Claude Generated 2026 (UI redesign WP9). No raw string literals in this file (moc trap,
 * see test_llmclient.cpp); payloads are plain concatenated strings.
 */

#include <QtTest/QtTest>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>

#include "src/agent/chatlog.h"

class ChatLogTest : public QObject {
    Q_OBJECT

    static ChatLog sampleLog()
    {
        ChatLog log;
        log.setProfile(QStringLiteral("local"));
        log.setModel(QStringLiteral("glm-5.2:cloud"));
        const QDateTime t0 = QDateTime::currentDateTime();
        log.append(ChatEntry::Kind::User, QStringLiteral("Which model fits best?\nSecond line"), t0);
        log.appendTool(QStringLiteral("compare_models"), QJsonObject{ { QStringLiteral("project"), 1 } }, true,
            QStringLiteral("4 models ranked"), t0.addSecs(2));
        log.appendTool(QStringLiteral("fit_model"), QJsonObject{ { QStringLiteral("model"), QStringLiteral("x") } }, false,
            QStringLiteral("unknown model"), t0.addSecs(3));
        const QString answer = QStringLiteral("## Ranking\n\n")
            + QStringLiteral("| Model | lgK | SSE |\n|---|---|---|\n| 1:1 | 2.90 | 0.0127 |\n| 1:2 | 3.38 | 0.0119 |\n\n")
            + QStringLiteral("- **1:1** is enough\n\n```\nfit_model 1\n```\n");
        log.append(ChatEntry::Kind::Assistant, answer, t0.addSecs(5));
        log.append(ChatEntry::Kind::Notice, QStringLiteral("Remote endpoint"), t0.addSecs(6));
        QJsonArray conversation;
        conversation.append(QJsonObject{ { QStringLiteral("role"), QStringLiteral("system") }, { QStringLiteral("content"), QStringLiteral("You are a test.") } });
        conversation.append(QJsonObject{ { QStringLiteral("role"), QStringLiteral("user") }, { QStringLiteral("content"), QStringLiteral("Which model fits best?") } });
        log.setConversation(conversation);
        return log;
    }

private slots:
    void jsonRoundTripIsExact()
    {
        const ChatLog log = sampleLog();
        const QJsonObject json = log.toJson();
        QCOMPARE(json.value(QStringLiteral("version")).toInt(), ChatLog::FormatVersion);
        QVERIFY(!json.contains(QStringLiteral("api_key")));
        QVERIFY(!QString::fromUtf8(QJsonDocument(json).toJson()).contains(QStringLiteral("key"), Qt::CaseInsensitive));

        ChatLog restored;
        QString error;
        QVERIFY2(ChatLog::fromJson(json, &restored, &error), qPrintable(error));
        QVERIFY(restored == log);
        QCOMPARE(restored.toJson(), json);

        // through a file as well
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("chat.json"));
        QVERIFY(log.save(path, &error));
        ChatLog loaded;
        QVERIFY2(ChatLog::load(path, &loaded, &error), qPrintable(error));
        QVERIFY(loaded == log);
        QCOMPARE(loaded.entries().size(), 5);
        QCOMPARE(loaded.entries().at(2).toolOk, false);
        QCOMPARE(loaded.title(), QStringLiteral("Which model fits best?"));
    }

    void markdownKeepsTablesAndToolLines()
    {
        const QString md = sampleLog().toMarkdown();
        QVERIFY(md.startsWith(QStringLiteral("# SupraFit chat: Which model fits best?")));
        QVERIFY(md.contains(QStringLiteral("## You · ")));
        QVERIFY(md.contains(QStringLiteral("## Assistant · ")));
        QVERIFY(md.contains(QStringLiteral("| 1:2 | 3.38 | 0.0119 |")));
        QVERIFY(md.contains(QStringLiteral("|---|---|---|")));
        QVERIFY(md.contains(QStringLiteral("> tool `compare_models` {\"project\":1} - ok: 4 models ranked")));
        QVERIFY(md.contains(QStringLiteral("> tool `fit_model`")));
        QVERIFY(md.contains(QStringLiteral(" - failed: unknown model")));
        QVERIFY(md.contains(QStringLiteral("```\nfit_model 1\n```")));
        QVERIFY(md.contains(QStringLiteral("profile local")));
    }

    void damagedFilesAreRejected()
    {
        QTemporaryDir dir;
        const QString broken = dir.filePath(QStringLiteral("broken.json"));
        {
            QFile f(broken);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("{ \"format\": \"suprafit-chat\", \"version\": 1, ");
        }
        ChatLog log;
        QString error;
        QVERIFY(!ChatLog::load(broken, &log, &error));
        QVERIFY(!error.isEmpty());

        QJsonObject wrong = sampleLog().toJson();
        wrong.insert(QStringLiteral("format"), QStringLiteral("other"));
        QVERIFY(!ChatLog::fromJson(wrong, &log, &error));
        QJsonObject future = sampleLog().toJson();
        future.insert(QStringLiteral("version"), ChatLog::FormatVersion + 1);
        QVERIFY(!ChatLog::fromJson(future, &log, &error));
        QJsonObject badKind = sampleLog().toJson();
        QJsonArray entries = badKind.value(QStringLiteral("entries")).toArray();
        QJsonObject first = entries.first().toObject();
        first.insert(QStringLiteral("kind"), QStringLiteral("robot"));
        entries.replace(0, first);
        badKind.insert(QStringLiteral("entries"), entries);
        QVERIFY(!ChatLog::fromJson(badKind, &log, &error));
        QVERIFY(!ChatLog::load(dir.filePath(QStringLiteral("missing.json")), &log, &error));
    }

    void historyStoreKeepsTheNewestUpToTheCap()
    {
        QTemporaryDir dir;
        ChatHistoryStore store(dir.filePath(QStringLiteral("chats")), 5);
        QString error;
        QStringList ids;
        for (int i = 0; i < 8; ++i) {
            ChatLog log;
            log.append(ChatEntry::Kind::User, QStringLiteral("question %1").arg(i));
            QVERIFY2(store.save(log, &error), qPrintable(error));
            ids << log.id();
            QTest::qWait(15); // distinct file times
        }
        // empty chats are not stored
        QVERIFY(store.save(ChatLog(), &error));
        // a damaged file does not break the listing
        {
            QFile f(dir.filePath(QStringLiteral("chats/zz-broken.json")));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("not json");
        }
        const QVector<ChatHistoryStore::Item> items = store.list();
        QCOMPARE(items.size(), 5);
        QCOMPARE(items.first().title, QStringLiteral("question 7"));
        QCOMPARE(items.last().title, QStringLiteral("question 3"));
        store.prune();
        QCOMPARE(QDir(store.directory()).entryList({ QStringLiteral("*.json") }, QDir::Files).size(), 5);

        // saving the same chat again replaces its file
        ChatLog again;
        ChatLog::load(items.first().path, &again);
        again.append(ChatEntry::Kind::Assistant, QStringLiteral("answer"));
        QVERIFY(store.save(again, &error));
        QCOMPARE(store.list().size(), 5);
    }

    void inputHistoryRecallsAndCaps()
    {
        InputHistory history(3);
        QCOMPARE(history.previous(QStringLiteral("draft")), QStringLiteral("draft"));
        history.push(QStringLiteral("a"));
        history.push(QStringLiteral("b"));
        history.push(QStringLiteral("b")); // immediate repeat skipped
        history.push(QStringLiteral("   ")); // empty skipped
        history.push(QStringLiteral("c"));
        history.push(QStringLiteral("d")); // "a" drops out
        QCOMPARE(history.items(), QStringList({ QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("d") }));

        QCOMPARE(history.previous(QStringLiteral("typing")), QStringLiteral("d"));
        QCOMPARE(history.previous(QStringLiteral("ignored")), QStringLiteral("c"));
        QCOMPARE(history.previous(QString()), QStringLiteral("b"));
        QCOMPARE(history.previous(QString()), QStringLiteral("b")); // stays at the oldest
        QCOMPARE(history.next(), QStringLiteral("c"));
        QCOMPARE(history.next(), QStringLiteral("d"));
        QCOMPARE(history.next(), QStringLiteral("typing")); // back to the draft
        QVERIFY(!history.isBrowsing());

        InputHistory restored(3);
        restored.setItems({ QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4") });
        QCOMPARE(restored.items().size(), 3);
        QCOMPARE(restored.items().first(), QStringLiteral("2"));
    }
};

QTEST_GUILESS_MAIN(ChatLogTest)
#include "test_chatlog.moc"
