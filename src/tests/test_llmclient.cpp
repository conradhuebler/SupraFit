/*
 * SupraFit - LlmConfig and LlmClient against a stub server (Phase 2, no net)
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* Phase 2 of roadmap/llm_agent_analysis.md: the LLM transport is tested against a
 * stub HTTP server inside the test process (test_stubserver.h). No network is
 * touched -- ctest must not depend on an endpoint being reachable, and a test
 * that silently passes when the machine is offline is worse than no test.
 *
 * What is pinned (request shape, headers, tool-calls, errors, streaming, Ollama
 * quirks): the request names the model and carries messages + tools + tool_choice;
 * no Authorization header without a key, Bearer when there is one; a tool_calls
 * answer arrives as a normal assistant message (content:"" next to it, arguments
 * as a JSON string); endpoint errors surface as Klartext; streaming assembles
 * content/reasoning/tool_calls and never sends reasoning back; a fragmented
 * tool call joins by index. Modelled on qurcuma's test_llmclient.cpp.
 *
 * moc-Trap (hit and bisected 2026-10-11, /usr/lib/qt6/moc 6.11.2): moc loses the
 * Q_OBJECT class and the .moc comes out empty (link fails with undefined vtable)
 * when the file contains a RAW string R"(...)" with either (a) escaped quotes \"
 * inside it, or (b) a line containing // -- both as soon as such a raw string
 * precedes the class. Plain strings with \" or // are harmless. So: JSON payloads
 * with escaped quotes go into PLAIN string concatenation ("..." "...", one
 * balanced literal per line), and multi-line raw strings only carry payloads
 * without any // and without any \".
 * Claude Generated (2026). */

#include <QtTest/QtTest>
#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>

#include "src/agent/llmclient.h"
#include "src/agent/llmconfig.h"

#include "test_stubserver.h"

namespace {

/// Run one request to completion. Returns true on finished(), false on failed().
bool roundTrip(LlmClient& client, const QJsonArray& messages, const QJsonArray& tools,
               QJsonObject& message, QString& error)
{
    QEventLoop loop;
    bool ok = false;
    QObject::connect(&client, &LlmClient::finished, &loop, [&](const QJsonObject& m) {
        message = m;
        ok = true;
        loop.quit();
    });
    QObject::connect(&client, &LlmClient::failed, &loop, [&](const QString& e) {
        error = e;
        ok = false;
        loop.quit();
    });
    QTimer::singleShot(5000, &loop, [&loop] { loop.quit(); });  // never hang the suite
    client.send(messages, tools);
    loop.exec();
    return ok;
}

LlmProfile stubProfile(const StubServer& server, bool stream = false)
{
    LlmProfile profile;
    profile.name = QStringLiteral("stub");
    profile.baseUrl = server.baseUrl();
    profile.model = QStringLiteral("test-model");
    profile.stream = stream;
    return profile;
}

const QJsonArray s_messages{
    QJsonObject{ { QStringLiteral("role"), QStringLiteral("user") },
                 { QStringLiteral("content"), QStringLiteral("hello") } }
};

const QJsonArray s_tools{
    QJsonObject{
        { QStringLiteral("type"), QStringLiteral("function") },
        { QStringLiteral("function"),
          QJsonObject{ { QStringLiteral("name"), QStringLiteral("get_model_fit") } } } }
};

}  // namespace

class LlmClientTest : public QObject {
    Q_OBJECT

private slots:
    // --- LlmConfig ----------------------------------------------------------

    void configLoadsWellFormed()
    {
        LlmConfig config;
        QString error;
        // moc-Trap: a MULTI-LINE raw string whose lines contain "//" (here: the
        // URLs) makes moc lose the class; the generated .moc comes out empty and
        // the link fails with an undefined vtable. Plain string concatenation
        // with one balanced literal per line is safe (measured with
        // /usr/lib/qt6/moc 6.11, 2026-10-11). Keep this rule for future tests.
        const QByteArray json =
            "{"
            "\"active_profile\": \"hosted\","
            "\"profiles\": ["
            "{\"name\":\"local\",\"base_url\":\"http://localhost:11434/v1\",\"model\":\"ornith-1.5:9b\"},"
            "{\"name\":\"hosted\",\"base_url\":\"https://api.example.com/v1/\",\"model\":\"gpt\","
            "\"api_key_env\":\"SUPRAFIT_TEST_KEY\",\"supports_vision\":true}"
            "]"
            "}";
        const bool ok = config.loadFromJson(json, &error);
        QVERIFY2(ok, qPrintable(error));
        QCOMPARE(config.profiles().size(), 2);

        LlmProfile active;
        QVERIFY(config.activeProfile(active));
        QCOMPARE(active.name, QStringLiteral("hosted"));
        QVERIFY(active.supportsVision);
        QCOMPARE(active.chatCompletionsUrl(), QStringLiteral("https://api.example.com/v1/chat/completions"));

        qputenv("SUPRAFIT_TEST_KEY", "sk-from-environment");
        QCOMPARE(LlmConfig::apiKeyFor(active), QStringLiteral("sk-from-environment"));

        LlmProfile keyless;
        QVERIFY(config.profile(QStringLiteral("local"), keyless));
        QVERIFY(LlmConfig::apiKeyFor(keyless).isEmpty());
    }

    void configRefusesKeyInFile()
    {
        // A key written into the config file is a leak waiting to be committed.
        // Refuse the whole file rather than silently ignore it.
        LlmConfig config;
        QString error;
        const bool ok = config.loadFromJson(
            R"JSON({"profiles":[{"name":"x","base_url":"u","model":"m","api_key":"sk-oops"}]})JSON",
            &error);
        QVERIFY(!ok);
        QVERIFY(error.contains(QStringLiteral("api_key")));
    }

    void configRefusesMalformed()
    {
        LlmConfig config;
        QString error;
        QVERIFY(!config.loadFromJson(R"JSON({"profiles":[{"name":"x"}]})JSON", &error));
        QVERIFY(!config.loadFromJson("not json at all", &error));
        QVERIFY(error.contains(QStringLiteral("JSON")));
    }

    void configWritesExampleIfMissing()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("llm.json"));
        QString error;
        QVERIFY(LlmConfig::writeExampleIfMissing(path, &error));

        // The example must parse and must contain a local profile -- the FIRST
        // experience on a fresh machine. (The earlier version of this test
        // appended a "// user edit" line and reloaded, which parses to nothing:
        // comments are not JSON. "Not overwritten" is checked below by size
        // and content, not by re-parsing.)
        LlmConfig config;
        QVERIFY2(config.load(path, &error), qPrintable(error));
        LlmProfile local;
        QVERIFY(config.profile(QStringLiteral("local"), local));
        QCOMPARE(local.baseUrl, QStringLiteral("http://localhost:11434/v1"));

        // The hosted profile is there, names its key variable and never holds a key.
        LlmProfile hosted;
        QVERIFY(config.profile(QStringLiteral("openai"), hosted));
        QCOMPARE(hosted.baseUrl, QStringLiteral("https://api.openai.com/v1"));
        QCOMPARE(hosted.apiKeyEnv, QStringLiteral("OPENAI_API_KEY"));
        QVERIFY(!LlmConfig::isLocalEndpoint(hosted));
        QVERIFY(LlmConfig::isLocalEndpoint(local));
        qunsetenv("OPENAI_API_KEY");
        QVERIFY(LlmConfig::missingKeyMessage(hosted).contains(QStringLiteral("OPENAI_API_KEY")));
        QVERIFY(LlmConfig::missingKeyMessage(local).isEmpty());  // a local profile needs no key
        qputenv("OPENAI_API_KEY", "sk-test");
        QVERIFY(LlmConfig::missingKeyMessage(hosted).isEmpty());
        qunsetenv("OPENAI_API_KEY");

        // Second call must not overwrite an existing file: append a marker line
        // and verify it survives a writeExampleIfMissing call.
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Append));
        f.write("user-edit-line\n");
        f.close();
        QVERIFY(LlmConfig::writeExampleIfMissing(path, &error));
        QVERIFY(f.open(QIODevice::ReadOnly));
        QVERIFY(f.readAll().contains("user-edit-line"));
    }

    // --- LlmClient: request shape -------------------------------------------

    void requestShapeAndNoAuthWithoutKey()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmClient client;
        client.setProfile(stubProfile(server));
        server.reply(200, R"JSON({"choices":[{"message":{"role":"assistant","content":"hi"}}]})JSON");

        QJsonObject message;
        QString error;
        const bool ok = roundTrip(client, s_messages, s_tools, message, error);
        QVERIFY2(ok, qPrintable(error));
        QCOMPARE(message.value(QStringLiteral("content")).toString(), QStringLiteral("hi"));

        const QJsonObject sent = QJsonDocument::fromJson(server.requestBody).object();
        QCOMPARE(sent.value(QStringLiteral("model")).toString(), QStringLiteral("test-model"));
        QCOMPARE(sent.value(QStringLiteral("messages")).toArray().size(), 1);
        QCOMPARE(sent.value(QStringLiteral("tools")).toArray().size(), 1);
        QCOMPARE(sent.value(QStringLiteral("tool_choice")).toString(), QStringLiteral("auto"));
        QVERIFY(!server.requestHeaders.toLower().contains("authorization"));
    }

    void authHeaderWithKeyAndNoToolsFieldWithoutTools()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmClient client;
        client.setProfile(stubProfile(server));
        client.setApiKey(QStringLiteral("sk-secret"));
        server.reply(200, R"JSON({"choices":[{"message":{"role":"assistant","content":"ok"}}]})JSON");

        QJsonObject message;
        QString error;
        roundTrip(client, s_messages, {}, message, error);
        QVERIFY(server.requestHeaders.contains("Authorization: Bearer sk-secret"));
        const QJsonObject sent = QJsonDocument::fromJson(server.requestBody).object();
        QVERIFY(!sent.contains(QStringLiteral("tools")));
    }

    void toolCallArrivesAsAssistantMessage()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmClient client;
        client.setProfile(stubProfile(server));
        // Ollama quirk: content is "" (empty string, not null) next to tool_calls,
        // and arguments is a JSON *string*, not an object. Built programmatically:
        // brace counting in a concatenated raw string produced "unterminated
        // object" twice before this test finally passed.
        QJsonObject call;
        call.insert(QStringLiteral("id"), QStringLiteral("call_1"));
        call.insert(QStringLiteral("type"), QStringLiteral("function"));
        call.insert(QStringLiteral("function"),
            QJsonObject{
                { QStringLiteral("name"), QStringLiteral("compare_models_aic") },
                { QStringLiteral("arguments"), QStringLiteral("{\"project_id\":\"p1\"}") } });
        QJsonObject assistantMessage;
        assistantMessage.insert(QStringLiteral("role"), QStringLiteral("assistant"));
        assistantMessage.insert(QStringLiteral("content"), QStringLiteral(""));
        assistantMessage.insert(QStringLiteral("tool_calls"), QJsonArray{ call });
        // OpenAI shape: choices[0] wraps the message object under "message" -- the
        // earlier programmatic attempt nested the message DIRECTLY in the choices
        // array, so the client read an empty "message" and the test saw 0 calls.
        QJsonObject choice;
        choice.insert(QStringLiteral("message"), assistantMessage);
        server.reply(200,
            QJsonDocument(QJsonObject{ { QStringLiteral("choices"), QJsonArray{ choice } } })
                .toJson(QJsonDocument::Compact));

        QJsonObject message;
        QString error;
        const bool ok = roundTrip(client, s_messages, s_tools, message, error);
        QVERIFY2(ok, qPrintable(error));
        const QJsonArray calls = message.value(QStringLiteral("tool_calls")).toArray();
        QCOMPARE(calls.size(), 1);
        QCOMPARE(calls.first().toObject().value(QStringLiteral("function")).toObject()
                     .value(QStringLiteral("name")).toString(),
            QStringLiteral("compare_models_aic"));
    }

    // --- LlmClient: failures -------------------------------------------------

    void endpointErrorMessageIsPreferred()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmClient client;
        client.setProfile(stubProfile(server));
        server.reply(401, R"JSON({"error":{"message":"Incorrect API key provided"}})JSON");
        QJsonObject message;
        QString error;
        const bool ok = roundTrip(client, s_messages, {}, message, error);
        qDebug() << "401 test: ok=" << ok << " error=" << error;
        QVERIFY(!ok);
        QVERIFY2(error.contains(QStringLiteral("Incorrect API key")),
            qPrintable(QStringLiteral("error was: %1").arg(error)));
    }

    void httpErrorIsReported()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmClient client;
        client.setProfile(stubProfile(server));
        server.reply(502, "<html><body>Bad Gateway</body></html>", "text/html");
        QJsonObject message;
        QString error;
        QVERIFY(!roundTrip(client, s_messages, {}, message, error));
        QVERIFY(error.contains(QStringLiteral("502")));
    }

    void emptyChoicesIsFailure()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmClient client;
        client.setProfile(stubProfile(server));
        server.reply(200, R"JSON({"choices":[]})JSON");
        QJsonObject message;
        QString error;
        QVERIFY(!roundTrip(client, s_messages, {}, message, error));
        QVERIFY(error.contains(QStringLiteral("choices")));
    }

    void sendWithoutProfileFailsImmediately()
    {
        LlmClient client;  // no profile at all
        QJsonObject message;
        QString error;
        QVERIFY(!roundTrip(client, s_messages, {}, message, error));
        QVERIFY(error.contains(QStringLiteral("profile")));
    }

    void sendWithoutModelFailsImmediately()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile modelless = stubProfile(server);
        modelless.model.clear();
        LlmClient client;
        client.setProfile(modelless);
        QJsonObject message;
        QString error;
        QVERIFY(!roundTrip(client, s_messages, {}, message, error));
        QVERIFY(error.contains(QStringLiteral("no model")));
    }

    // --- model discovery ------------------------------------------------------

    void modelListingIsRead()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmClient client;
        client.setProfile(stubProfile(server));
        server.reply(200,
            R"JSON({"object":"list","data":[{"id":"zeta:latest"},)JSON"
            R"JSON({"id":"alpha:cloud"},{"id":"mid:9b"}]})JSON");

        QEventLoop loop;
        QStringList got;
        QObject::connect(&client, &LlmClient::modelsListed, &loop, [&](const QStringList& m) {
            got = m;
            loop.quit();
        });
        QTimer::singleShot(5000, &loop, [&loop] { loop.quit(); });
        client.listModels();
        loop.exec();

        QCOMPARE(got.size(), 3);
        got.sort();
        QCOMPARE(got, QStringList({ QStringLiteral("alpha:cloud"), QStringLiteral("mid:9b"), QStringLiteral("zeta:latest") }));
    }

    // --- streaming ------------------------------------------------------------
    // Shapes measured against Ollama (qurcuma, 2026-09): lines of "data: {json}"
    // closed by "data: [DONE]", with reasoning in delta.reasoning.

    void streamedAnswerCompletes()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmClient client;
        client.setProfile(stubProfile(server, /*stream=*/true));
        server.reply(200,
            "data: {\"choices\":[{\"delta\":{\"role\":\"assistant\",\"content\":\"Hel\"}}]}\n"
            "data: {\"choices\":[{\"delta\":{\"content\":\"lo \"}}]}\n"
            "\n"
            ": a comment line that must be ignored\n"
            "data: {\"choices\":[{\"delta\":{\"content\":\"world\"}}]}\n"
            "data: [DONE]\n",
            "text/event-stream");

        QStringList chunks;
        QObject::connect(&client, &LlmClient::contentChunk, &client,
            [&chunks](const QString& c) { chunks << c; });
        QJsonObject message;
        QString error;
        const bool ok = roundTrip(client, s_messages, {}, message, error);
        QVERIFY2(ok, qPrintable(error));
        QCOMPARE(chunks.size(), 3);
        QCOMPARE(message.value(QStringLiteral("content")).toString(), QStringLiteral("Hello world"));
    }

    void reasoningIsSeparateAndNotResent()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmClient client;
        client.setProfile(stubProfile(server, /*stream=*/true));
        server.reply(200,
            "data: {\"choices\":[{\"delta\":{\"reasoning\":\"17\"}}]}\n"
            "data: {\"choices\":[{\"delta\":{\"reasoning\":\"*3\"}}]}\n"
            "data: {\"choices\":[{\"delta\":{\"content\":\"51\"}}]}\n"
            "data: [DONE]\n",
            "text/event-stream");

        QString thinking;
        QObject::connect(&client, &LlmClient::reasoningChunk, &client,
            [&thinking](const QString& c) { thinking += c; });
        QJsonObject message;
        QString error;
        roundTrip(client, s_messages, {}, message, error);
        QCOMPARE(thinking, QStringLiteral("17*3"));
        QCOMPARE(message.value(QStringLiteral("content")).toString(), QStringLiteral("51"));
        QVERIFY(!message.contains(QStringLiteral("reasoning")));
    }

    void fragmentedToolCallJoinsByIndex()
    {
        // OpenAI splits function.arguments across chunks; only the index ties
        // them. Each chunk is its own SSE event, so one COMPLETE JSON object per
        // "data:" line -- the fragmenting is across events, never within one
        // (the first version of this test put two half-JSONs on two data: lines,
        // which is not SSE at all; found 2026-10-11).
        StubServer server;
        QVERIFY(server.isListening());
        LlmClient client;
        client.setProfile(stubProfile(server, /*stream=*/true));
        server.reply(200,
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_9\","
            "\"type\":\"function\",\"function\":{\"name\":\"fit_model\","
            "\"arguments\":\"{\\\"project_id\\\":\"}}]}}]}\n"
            "\n"
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,"
            "\"function\":{\"arguments\":\"\\\"p1\\\"}\"}}]}}]}\n"
            "data: [DONE]\n",
            "text/event-stream");

        QJsonObject message;
        QString error;
        const bool ok = roundTrip(client, s_messages, s_tools, message, error);
        QVERIFY2(ok, qPrintable(error));
        const QJsonArray calls = message.value(QStringLiteral("tool_calls")).toArray();
        QCOMPARE(calls.size(), 1);
        const QJsonObject fn = calls.first().toObject().value(QStringLiteral("function")).toObject();
        QCOMPARE(fn.value(QStringLiteral("name")).toString(), QStringLiteral("fit_model"));
        QCOMPARE(fn.value(QStringLiteral("arguments")).toString(), QStringLiteral("{\"project_id\":\"p1\"}"));
        QCOMPARE(calls.first().toObject().value(QStringLiteral("id")).toString(), QStringLiteral("call_9"));
    }
};

QTEST_GUILESS_MAIN(LlmClientTest)
#include "test_llmclient.moc"
