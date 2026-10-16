/*
 * test_llmpresets.cpp - Endpoint presets, profile file round trip, model selection, model listing.
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Claude Generated 2026 (UI redesign WP10). Headless, no network beyond a local stub server.
 * No raw string literals in this file (moc trap, see test_llmclient.cpp).
 */

#include <QtTest/QtTest>

#include <QtCore/QEventLoop>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSet>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtNetwork/QTcpServer>

#include "src/agent/llmclient.h"
#include "src/agent/llmconfig.h"
#include "src/agent/llmpresets.h"

#include "test_stubserver.h"

class LlmPresetsTest : public QObject {
    Q_OBJECT

    struct Listing {
        QStringList models;
        QString error;
    };

    static Listing listFrom(const LlmProfile& profile)
    {
        LlmClient client;
        client.setProfile(profile);
        Listing result;
        QEventLoop loop;
        QObject::connect(&client, &LlmClient::modelListFailed, &loop, [&result](const QString& e) { result.error = e; });
        QObject::connect(&client, &LlmClient::modelsListed, &loop, [&](const QStringList& m) {
            result.models = m;
            loop.quit();
        });
        QTimer::singleShot(8000, &loop, &QEventLoop::quit);
        client.listModels();
        loop.exec();
        return result;
    }

private slots:
    void catalogIsSane()
    {
        const QVector<LlmPreset>& presets = LlmPresets::catalog();
        QVERIFY(presets.size() >= 10);
        QCOMPARE(presets.first().id, LlmPresets::defaultId());
        QCOMPARE(presets.first().baseUrl, QStringLiteral("http://127.0.0.1:11434/v1"));
        QCOMPARE(presets.first().reasoningField, QStringLiteral("think"));
        QCOMPARE(presets.last().id, QStringLiteral("custom"));
        QSet<QString> ids;
        for (const LlmPreset& p : presets) {
            QVERIFY2(!ids.contains(p.id), qPrintable(p.id));
            ids.insert(p.id);
            QVERIFY(!p.name.isEmpty());
            QVERIFY(!p.note.isEmpty());
            if (p.id == QLatin1String("custom"))
                continue;
            const QUrl url(p.baseUrl);
            QVERIFY2(url.isValid() && !url.host().isEmpty(), qPrintable(p.id));
            LlmProfile profile = p.toProfile();
            QCOMPARE(LlmConfig::isLocalEndpoint(profile), p.local);
            if (p.local) {
                QCOMPARE(url.host(), QStringLiteral("127.0.0.1")); // never "localhost" (::1 may be another server)
                QVERIFY(p.apiKeyEnv.isEmpty());
            } else {
                QCOMPARE(url.scheme(), QStringLiteral("https"));
                QVERIFY2(!p.apiKeyEnv.isEmpty(), qPrintable(p.id));
                QVERIFY(p.apiKeyEnv.endsWith(QLatin1String("_API_KEY")));
            }
            if (!p.verified)
                QVERIFY(p.note.contains(QLatin1String("Unverified")));
        }
        QVERIFY(LlmPresets::find(QStringLiteral("openai")));
        QVERIFY(!LlmPresets::find(QStringLiteral("nope")));
        QVERIFY(LlmPresets::isOllama(LlmPresets::find(QStringLiteral("ollama"))->toProfile()));
        QVERIFY(!LlmPresets::isOllama(LlmPresets::find(QStringLiteral("lmstudio"))->toProfile()));
    }

    void defaultConfigIsLocalOllama()
    {
        const LlmConfig config = LlmConfig::defaultConfig();
        LlmProfile active;
        QVERIFY(config.activeProfile(active));
        QCOMPARE(active.name, QStringLiteral("local"));
        QCOMPARE(active.baseUrl, QStringLiteral("http://127.0.0.1:11434/v1"));
        QVERIFY(LlmConfig::isLocalEndpoint(active));
        QVERIFY(QString::fromUtf8(config.toJson()).contains(QStringLiteral("_note")));
    }

    void saveKeepsUnknownKeysAndNeverAKey()
    {
        const QByteArray json = QByteArray("{\"_note\":[\"keep me\"],\"x_custom\":7,\"active_profile\":\"b\",")
            + "\"profiles\":[{\"name\":\"a\",\"base_url\":\"http://127.0.0.1:1/v1\",\"model\":\"m1\",\"future_flag\":true},"
            + "{\"name\":\"b\",\"base_url\":\"https://example.org/v1\",\"api_key_env\":\"EXAMPLE_API_KEY\",\"preset\":\"custom\",\"reasoning_field\":\"\"}]}";
        LlmConfig config;
        QString error;
        QVERIFY2(config.loadFromJson(json, &error), qPrintable(error));

        // edit: rename a profile, add one from a preset
        QVector<LlmProfile> profiles = config.profiles();
        profiles[0].model = QStringLiteral("m2");
        profiles.append(LlmPresets::find(QStringLiteral("groq"))->toProfile());
        config.setProfiles(profiles);
        config.setActiveProfileName(QStringLiteral("a"));

        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("sub/llm.json"));
        QVERIFY2(config.save(path, &error), qPrintable(error));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray written = file.readAll();
        QVERIFY(!written.toLower().contains("\"api_key\""));
        const QJsonObject root = QJsonDocument::fromJson(written).object();
        QCOMPARE(root.value(QStringLiteral("_note")).toArray().first().toString(), QStringLiteral("keep me"));
        QCOMPARE(root.value(QStringLiteral("x_custom")).toInt(), 7);
        QCOMPARE(root.value(QStringLiteral("profiles")).toArray().first().toObject().value(QStringLiteral("future_flag")).toBool(), true);

        LlmConfig reloaded;
        QVERIFY2(reloaded.load(path, &error), qPrintable(error));
        QCOMPARE(reloaded.activeProfileName(), QStringLiteral("a"));
        QCOMPARE(reloaded.profiles().size(), 3);
        LlmProfile a, b, groq;
        QVERIFY(reloaded.profile(QStringLiteral("a"), a));
        QVERIFY(reloaded.profile(QStringLiteral("b"), b));
        QVERIFY(reloaded.profile(QStringLiteral("Groq"), groq));
        QCOMPARE(a.model, QStringLiteral("m2"));
        QCOMPARE(b.apiKeyEnv, QStringLiteral("EXAMPLE_API_KEY"));
        QCOMPARE(b.reasoningField, QString());
        QCOMPARE(groq.preset, QStringLiteral("groq"));
        QCOMPARE(groq.baseUrl, QStringLiteral("https://api.groq.com/openai/v1"));

        // a second save of the reloaded config writes the same document
        QCOMPARE(QJsonDocument::fromJson(reloaded.toJson()), QJsonDocument::fromJson(written));

        // a key smuggled into the extras is never written
        QVector<LlmProfile> leaky = reloaded.profiles();
        leaky[0].extra.insert(QStringLiteral("api_key"), QStringLiteral("sk-secret"));
        reloaded.setProfiles(leaky);
        QVERIFY(!reloaded.toJson().contains("sk-secret"));
    }

    void modelSelectionRules()
    {
        LlmProfile withDefault;
        withDefault.name = QStringLiteral("local");
        withDefault.baseUrl = QStringLiteral("http://127.0.0.1:11434/v1");
        withDefault.model = QStringLiteral("glm-5.2:cloud");
        LlmProfile empty = withDefault;
        empty.name = QStringLiteral("other");
        empty.model.clear();
        const QStringList listed = { QStringLiteral("gpt-oss:20b"), QStringLiteral("glm-5.2:cloud") };

        LlmModelSelection selection;
        QCOMPARE(selection.choose(withDefault, listed), QStringLiteral("glm-5.2:cloud")); // default
        QCOMPARE(selection.choose(empty, listed), QStringLiteral("gpt-oss:20b"));         // first listed
        QCOMPARE(selection.choose(empty), QString());                                      // nothing known
        selection.remember(QStringLiteral("local"), QStringLiteral("gpt-oss:20b"));
        QCOMPARE(selection.choose(withDefault, listed), QStringLiteral("gpt-oss:20b"));   // remembered wins
        QCOMPARE(selection.choose(empty, listed), QStringLiteral("gpt-oss:20b"));         // per profile
        selection.remember(QStringLiteral("other"), QStringLiteral("my-own-model"));     // typed, not listed
        QCOMPARE(selection.choose(empty, listed), QStringLiteral("my-own-model"));
        selection.remember(QStringLiteral("local"), QString());                           // forget
        QCOMPARE(selection.choose(withDefault, listed), QStringLiteral("glm-5.2:cloud"));

        // the client drops a model override when the profile changes (the "only glm" bug)
        LlmClient client;
        client.setProfile(withDefault);
        client.setModel(QStringLiteral("gpt-oss:20b"));
        QCOMPARE(client.model(), QStringLiteral("gpt-oss:20b"));
        client.setProfile(withDefault); // same profile again: the choice stays
        QCOMPARE(client.model(), QStringLiteral("gpt-oss:20b"));
        client.setProfile(empty);
        QCOMPARE(client.model(), QString());
    }

    void listingIsDedupedAndFailuresAreExplained()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.apiKeyEnv = QStringLiteral("STUB_API_KEY");

        server.reply(200, QByteArray("{\"object\":\"list\",\"data\":[{\"id\":\"a\"},{\"id\":\"b\"},{\"id\":\"a\"}]}"));
        Listing ok = listFrom(profile);
        QCOMPARE(ok.models, QStringList({ QStringLiteral("a"), QStringLiteral("b") }));
        QVERIFY(ok.error.isEmpty());
        QVERIFY(server.requestHeaders.startsWith("GET /v1/models"));

        server.reply(401, QByteArray("{\"error\":{\"message\":\"no key\"}}"));
        Listing denied = listFrom(profile);
        QVERIFY(denied.models.isEmpty());
        QVERIFY2(denied.error.contains(QStringLiteral("STUB_API_KEY")), qPrintable(denied.error));

        server.reply(404, QByteArray("{}"));
        Listing missing = listFrom(profile);
        QVERIFY2(missing.error.contains(QStringLiteral("/v1")), qPrintable(missing.error));

        // a closed port: take a free one and close it again
        QTcpServer probe;
        QVERIFY(probe.listen(QHostAddress::LocalHost, 0));
        const quint16 port = probe.serverPort();
        probe.close();
        LlmProfile closed = profile;
        closed.baseUrl = QStringLiteral("http://127.0.0.1:%1/v1").arg(port);
        Listing refused = listFrom(closed);
        QVERIFY2(refused.error.contains(QStringLiteral("refused"), Qt::CaseInsensitive), qPrintable(refused.error));

        // a listing still in flight when the profile changes never reaches the new profile
        server.reply(200, QByteArray("{\"data\":[{\"id\":\"old-model\"}]}"));
        LlmClient client;
        client.setProfile(profile);
        int listed = 0;
        QObject::connect(&client, &LlmClient::modelsListed, &client, [&listed](const QStringList&) { ++listed; });
        client.listModels();
        client.setProfile(closed);
        QTest::qWait(300);
        QCOMPARE(listed, 0);
    }

    void ollamaCapabilitiesAreParsed()
    {
        const QJsonObject show = QJsonDocument::fromJson(QByteArray("{\"capabilities\":[\"completion\",\"tools\",\"thinking\"],")
                                                             + "\"model_info\":{\"general.architecture\":\"gptoss\",\"gptoss.context_length\":131072}}")
                                     .object();
        const LlmModelInfo info = LlmClient::parseOllamaShow(QStringLiteral("gpt-oss:20b"), show);
        QCOMPARE(info.id, QStringLiteral("gpt-oss:20b"));
        QVERIFY(info.detailsKnown);
        QVERIFY(info.supportsTools);
        QVERIFY(!info.supportsVision);
        QCOMPARE(info.contextLength, 131072);

        const LlmModelInfo plain = LlmClient::parseOllamaShow(QStringLiteral("x"),
            QJsonDocument::fromJson(QByteArray("{\"capabilities\":[\"completion\"]}")).object());
        QVERIFY(plain.detailsKnown);
        QVERIFY(!plain.supportsTools);
        const LlmModelInfo unknown = LlmClient::parseOllamaShow(QStringLiteral("y"), QJsonObject());
        QVERIFY(!unknown.detailsKnown);
    }
};

QTEST_GUILESS_MAIN(LlmPresetsTest)
#include "test_llmpresets.moc"
