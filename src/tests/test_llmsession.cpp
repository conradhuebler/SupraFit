/*
 * SupraFit - the agent loop, driven through the real LlmClient (Phase 3, no net)
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

/* Phase 3 of roadmap/llm_agent_analysis.md: the agent loop, driven through the REAL
 * LlmClient against the stub endpoint (test_stubserver.h). Deliberately not against
 * a fake client: the question worth answering is whether client and session
 * together do the right thing -- whether a tool_calls block really reaches the
 * dispatcher and the result goes back in the shape the protocol wants. A fake
 * would skip that seam. Modelled on qurcuma's test_llmsession.cpp.
 *
 * SupraFit has no LogHub yet (that is a later optional WP), so the session is
 * constructed with the plain registry + dispatcher; the audit signal of the
 * dispatcher is checked instead. Same moc-trap rule as test_llmclient.cpp:
 * JSON payloads with escaped quotes go into PLAIN string concatenation, never a
 * raw string, and multi-line raw strings must not contain //.
 * Claude Generated (2026). */

#include <QtTest/QtTest>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QWaitCondition>

#include "src/agent/llmclient.h"
#include "src/agent/llmsession.h"
#include "src/agent/tooldispatcher.h"
#include "src/agent/toolregistry.h"

#include "test_stubserver.h"

class LlmSessionTest : public QObject {
    Q_OBJECT

private:
    ToolRegistry m_registry;
    ToolDispatcher m_dispatcher { &m_registry };

    int m_readCalls = 0;
    int m_computeCalls = 0;
    QStringList m_auditNames;
    int m_auditRefusals = 0;  ///< audit entries with ok=false (refusals + unknown tools)

    static QJsonObject obj(const char* json)
    {
        return QJsonDocument::fromJson(QByteArray(json)).object();
    }

    /// An assistant answer that asks for a tool call. Built from plain string
    /// concatenation: escaped quotes inside a RAW string break moc (see the note
    /// in test_llmclient.cpp). callId lets each call have a unique id.
    static QByteArray callResponse(const char* name, const QByteArray& argsJsonLiteral,
                                   const char* callId = "call_1")
    {
        QByteArray args(argsJsonLiteral);
        args.replace("\"", "\\\"");
        return QByteArray("{\"choices\":[{\"message\":{\"role\":\"assistant\",\"content\":null,"
                          "\"tool_calls\":[{\"id\":\"")
            + QByteArray(callId) + QByteArray("\",\"type\":\"function\",\"function\":{\"name\":\"")
            + QByteArray(name) + QByteArray("\",\"arguments\":\"") + args
            + QByteArray("\"}}]}}]}");
    }

    static QByteArray textResponse(const char* text)
    {
        return QByteArray("{\"choices\":[{\"message\":{\"role\":\"assistant\",\"content\":\"") + text
            + QByteArray("\"}}]}");
    }

    /// Run one turn to completion. Refusals (recordRefusal) are delivered to the
    /// dispatcher's thread by a QUEUED invocation; pump the event loop after the
    /// turn so they land before the next turn's asserts read m_auditNames.
    void runTurn(LlmSession& session, const QString& question, QString& answer, QString& error)
    {
        QEventLoop loop;
        answer.clear();
        error.clear();
        connect(&session, &LlmSession::assistantMessage, &loop,
            [&answer](const QString& text) { answer = text; });
        connect(&session, &LlmSession::finished, &loop, [&loop] { loop.quit(); });
        connect(&session, &LlmSession::failed, &loop, [&error, &loop](const QString& e) {
            error = e;
            loop.quit();
        });
        QTimer::singleShot(8000, &loop, [&loop] { loop.quit(); });
        session.ask(question);
        loop.exec();
        // Drain queued recordRefusal() invocations posted during the turn.
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }

private slots:
    void initTestCase()
    {
        connect(&m_dispatcher, &ToolDispatcher::callRecorded, this,
            [this](const QString& name, const QJsonObject&, bool ok, qint64, const QString&) {
                m_auditNames.append(name);
                if (!ok)
                    ++m_auditRefusals;
            }, Qt::DirectConnection);

        {
            ToolSpec spec;
            spec.name = QStringLiteral("get_answer");
            spec.description = QStringLiteral("Return a number");
            spec.category = QStringLiteral("test");
            spec.effect = ToolEffect::Read;
            spec.paramSchema = obj("{\"type\":\"object\",\"properties\":{\"n\":{\"type\":\"integer\"}}}");
            spec.handler = [this](const QJsonObject& args) {
                ++m_readCalls;
                return ToolResult::success(QJsonObject{ { QStringLiteral("answer"), args.value(QStringLiteral("n")).toInt(7) } });
            };
            QVERIFY(m_registry.add(spec));
        }
        {
            ToolSpec spec;
            spec.name = QStringLiteral("fit_model_test");
            spec.description = QStringLiteral("Fit a model (compute)");
            spec.category = QStringLiteral("test");
            spec.effect = ToolEffect::Compute;  // needs approval
            spec.paramSchema = obj("{\"type\":\"object\"}");
            spec.handler = [this](const QJsonObject&) {
                ++m_computeCalls;
                return ToolResult::success(QJsonObject{ { QStringLiteral("started"), true } });
            };
            QVERIFY(m_registry.add(spec));
        }
    }

    // --- a plain answer, no tools -------------------------------------------

    void plainAnswerComesThrough()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;  // canned bodies here are plain JSON, not SSE

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);
        session.setSystemPrompt(QStringLiteral("You are a test."));

        QString answer, error;
        const int readsBefore = m_readCalls;
        server.reply(200, textResponse("just words"));
        runTurn(session, QStringLiteral("hello"), answer, error);
        QCOMPARE(answer, QStringLiteral("just words"));
        QCOMPARE(m_readCalls, readsBefore);

        const QJsonObject sent = QJsonDocument::fromJson(server.requests.first()).object();
        const QJsonArray tools = sent.value(QStringLiteral("tools")).toArray();
        QCOMPARE(tools.size(), 2);
        const QJsonObject first = tools.first().toObject().value(QStringLiteral("function")).toObject();
        QVERIFY(first.contains(QStringLiteral("parameters")));
        QCOMPARE(sent.value(QStringLiteral("messages")).toArray().size(), 2);
    }

    // --- a Read tool: runs unattended, result goes back ----------------------

    void readToolRunsUnattended()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);
        session.setSystemPrompt(QStringLiteral("You are a test."));

        QString answer, error;
        const int readsBefore = m_readCalls;
        session.reset();
        server.requests.clear();
        server.enqueue(callResponse("get_answer", "{\"n\":42}"));
        server.enqueue(textResponse("the answer is 42"));
        runTurn(session, QStringLiteral("what is it"), answer, error);

        QCOMPARE(m_readCalls, readsBefore + 1);
        QCOMPARE(answer, QStringLiteral("the answer is 42"));
        QCOMPARE(server.requests.size(), 2);

        const QJsonArray second = QJsonDocument::fromJson(server.requests.at(1))
                                      .object().value(QStringLiteral("messages")).toArray();
        bool sawToolMessage = false;
        bool sawAssistantCall = false;
        for (const QJsonValue& v : second) {
            const QJsonObject m = v.toObject();
            if (m.value(QStringLiteral("role")).toString() == QLatin1String("tool")) {
                sawToolMessage = true;
                QCOMPARE(m.value(QStringLiteral("tool_call_id")).toString(), QStringLiteral("call_1"));
                QVERIFY(m.value(QStringLiteral("content")).toString().contains(QStringLiteral("42")));
            }
            if (m.contains(QStringLiteral("tool_calls")))
                sawAssistantCall = true;
        }
        QVERIFY(sawToolMessage);
        QVERIFY(sawAssistantCall);
    }

    // --- a Compute tool without an approval policy: refused ------------------

    void computeToolWithoutPolicyIsRefused()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);

        QString answer, error;
        const int computeBefore = m_computeCalls;
        session.reset();
        server.requests.clear();
        server.enqueue(callResponse("fit_model_test", "{}"));
        server.enqueue(textResponse("understood, I will not"));
        runTurn(session, QStringLiteral("fit it"), answer, error);

        QCOMPARE(m_computeCalls, computeBefore);
        const QJsonArray second = QJsonDocument::fromJson(server.requests.at(1))
                                      .object().value(QStringLiteral("messages")).toArray();
        bool told = false;
        for (const QJsonValue& v : second) {
            const QJsonObject m = v.toObject();
            if (m.value(QStringLiteral("role")).toString() == QLatin1String("tool")
                && m.value(QStringLiteral("content")).toString().contains(QStringLiteral("approval"))) {
                told = true;
            }
        }
        QVERIFY(told);
    }

    // --- with a policy that says yes ------------------------------------------

    void approvedComputeToolRuns()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);

        QString approvedTool;
        session.setApprovalPolicy([&approvedTool](const ToolSpec& spec, const QJsonObject&) {
            approvedTool = spec.name;
            return true;
        });

        QString answer, error;
        const int computeBefore = m_computeCalls;
        session.reset();
        server.requests.clear();
        server.enqueue(callResponse("fit_model_test", "{}"));
        server.enqueue(textResponse("started"));
        runTurn(session, QStringLiteral("fit it"), answer, error);

        QCOMPARE(m_computeCalls, computeBefore + 1);
        QCOMPARE(approvedTool, QStringLiteral("fit_model_test"));
    }

    // --- a policy that says no -------------------------------------------------

    void declinedComputeToolDoesNotRun()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);
        session.setApprovalPolicy([](const ToolSpec&, const QJsonObject&) { return false; });

        QString answer, error;
        const int computeBefore = m_computeCalls;
        session.reset();
        server.requests.clear();
        server.enqueue(callResponse("fit_model_test", "{}"));
        server.enqueue(textResponse("fine"));
        runTurn(session, QStringLiteral("fit it"), answer, error);
        QCOMPARE(m_computeCalls, computeBefore);
    }

    // --- InProgram autonomy: Compute runs without asking, FileWrite still asks ---

    void inProgramAutonomyRunsComputeWithoutAsking()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);
        session.setAutonomy(ToolAutonomy::InProgram);
        // No approval policy set: at InProgram, Compute needs NO approval at all.
        // (Read/Display also need none at any autonomy.)

        QString answer, error;
        const int computeBefore = m_computeCalls;
        session.reset();
        server.requests.clear();
        server.enqueue(callResponse("fit_model_test", "{}"));
        server.enqueue(textResponse("started"));
        runTurn(session, QStringLiteral("fit it"), answer, error);
        QCOMPARE(m_computeCalls, computeBefore + 1);
    }

    // --- a tool the model invented ---------------------------------------------

    void inventedToolIsAnsweredWithError()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);

        QString answer, error;
        session.reset();
        server.requests.clear();
        server.enqueue(callResponse("no_such_tool", "{}"));
        server.enqueue(textResponse("my mistake"));
        runTurn(session, QStringLiteral("go"), answer, error);
        const QJsonArray second = QJsonDocument::fromJson(server.requests.at(1))
                                      .object().value(QStringLiteral("messages")).toArray();
        bool told = false;
        for (const QJsonValue& v : second) {
            if (v.toObject().value(QStringLiteral("content")).toString()
                    .contains(QStringLiteral("unknown tool"))) {
                told = true;
            }
        }
        QVERIFY(told);
    }

    // --- arguments that are not JSON -------------------------------------------

    void unparseableArgumentsNeverReachHandler()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);

        // A fresh session is needed here so this test's request index (at(1))
        // reflects exactly this turn. All earlier tests run on their own sessions.
        QString answer, error;
        const int readsBefore = m_readCalls;
        session.reset();
        server.requests.clear();
        // Plain string concatenation: raw strings with escaped quotes break moc
        // (see the note in test_llmclient.cpp).
        server.enqueue(QByteArray("{\"choices\":[{\"message\":{\"role\":\"assistant\",\"content\":null,"
                                  "\"tool_calls\":[{\"id\":\"call_1\",\"type\":\"function\","
                                  "\"function\":{\"name\":\"get_answer\",\"arguments\":\"{not json\"}}]}}]}"));
        server.enqueue(textResponse("sorry"));
        runTurn(session, QStringLiteral("go"), answer, error);
        QCOMPARE(m_readCalls, readsBefore);

        const QJsonArray second = QJsonDocument::fromJson(server.requests.at(1))
                                      .object().value(QStringLiteral("messages")).toArray();
        bool told = false;
        bool hasCallId = false;
        for (const QJsonValue& v : second) {
            const QJsonObject m = v.toObject();
            if (m.value(QStringLiteral("role")).toString() == QLatin1String("tool")) {
                // The session refuses malformed args BEFORE the handler runs; the
                // model must be told what was wrong, and the tool message must still
                // carry the call id so the history stays protocol-valid.
                if (m.value(QStringLiteral("content")).toString().contains(QStringLiteral("not a JSON object")))
                    told = true;
                if (m.value(QStringLiteral("tool_call_id")).toString() == QLatin1String("call_1"))
                    hasCallId = true;
            }
        }
        QVERIFY(told);
        QVERIFY(hasCallId);
    }

    // --- the shapes a real endpoint actually sends ------------------------------
    // Observed against Ollama (qurcuma, 2026-09): alongside tool_calls the content
    // field is an empty STRING, not null, and arguments is a JSON string. Both are
    // pinned here so a future refactor cannot quietly treat "" as an answer and end
    // the turn without running anything.

    void emptyContentNextToToolCallsStillRunsTool()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);

        QString answer, error;
        session.setSystemPrompt(QStringLiteral("You are a test."));
        server.requests.clear();
        const int readsBefore = m_readCalls;
        // Built programmatically so the empty-string content + string arguments
        // quirks are explicit and the brace nesting cannot silently drift again.
        QJsonObject call;
        call.insert(QStringLiteral("id"), QStringLiteral("call_t65agluy"));
        call.insert(QStringLiteral("type"), QStringLiteral("function"));
        call.insert(QStringLiteral("function"),
            QJsonObject{
                { QStringLiteral("name"), QStringLiteral("get_answer") },
                { QStringLiteral("arguments"), QStringLiteral("{}") } });
        QJsonObject assistantMessage;
        assistantMessage.insert(QStringLiteral("role"), QStringLiteral("assistant"));
        assistantMessage.insert(QStringLiteral("content"), QStringLiteral(""));
        assistantMessage.insert(QStringLiteral("tool_calls"), QJsonArray{ call });
        QJsonObject choice;
        choice.insert(QStringLiteral("message"), assistantMessage);
        choice.insert(QStringLiteral("finish_reason"), QStringLiteral("tool_calls"));
        server.enqueue(QJsonDocument(QJsonObject{ { QStringLiteral("choices"), QJsonArray{ choice } } })
                           .toJson(QJsonDocument::Compact));
        server.enqueue(textResponse("42 models"));
        runTurn(session, QStringLiteral("how many"), answer, error);
        QCOMPARE(m_readCalls, readsBefore + 1);
        QCOMPARE(answer, QStringLiteral("42 models"));
    }

    // --- a loop that will not converge ------------------------------------------
    //
    // The cap ends the turn with one more round WITHOUT a catalogue, so the model
    // has to write its answer from the results already in the history.

    void iterationCapForcesClosingRound()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);
        session.setMaxIterations(3);

        QString answer, error;
        session.reset();
        server.requests.clear();
        for (int i = 0; i < 3; ++i)
            server.enqueue(callResponse("get_answer", "{\"n\":1}"));
        server.enqueue(textResponse("42, from what I gathered"));
        runTurn(session, QStringLiteral("loop forever"), answer, error);

        QCOMPARE(server.requests.size(), 4);
        const QJsonObject closing = QJsonDocument::fromJson(server.requests.last()).object();
        QVERIFY(!closing.contains(QStringLiteral("tools")));

        const QJsonArray history = closing.value(QStringLiteral("messages")).toArray();
        int toolResults = 0;
        int toolCalls = 0;
        for (const QJsonValue& value : history) {
            const QJsonObject entry = value.toObject();
            if (entry.value(QStringLiteral("role")).toString() == QLatin1String("tool"))
                ++toolResults;
            toolCalls += entry.value(QStringLiteral("tool_calls")).toArray().size();
        }
        QCOMPARE(toolResults, toolCalls);
        QCOMPARE(answer, QStringLiteral("42, from what I gathered"));
    }

    // --- the closing round is not an invitation to call more tools -------------

    void closingRoundCallingToolDoesNotOpenAnother()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);
        session.setMaxIterations(2);

        QString answer, error;
        session.reset();
        server.requests.clear();
        for (int i = 0; i < 4; ++i)
            server.enqueue(callResponse("get_answer", "{\"n\":1}"));
        runTurn(session, QStringLiteral("never stop"), answer, error);
        // 2 tool rounds + 1 closing round. The model calls a tool in the closing
        // round anyway (4 enqueued, only 3 consumed); the turn must still end.
        QCOMPARE(server.requests.size(), 3);
        QVERIFY(!answer.isEmpty());
    }

    // --- transport failure -------------------------------------------------------

    void transportFailureSurfacesAsFailed()
    {
        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);

        QString answer, error;
        session.reset();
        session.setMaxIterations(12);
        server.reply(500, "<html>nope</html>", "text/html");
        runTurn(session, QStringLiteral("go"), answer, error);
        QVERIFY(!error.isEmpty());
    }

    // --- a tool that cannot work now is not in the catalogue ----------------------

    void unavailableToolIsLeftOutOfCatalogue()
    {
        ToolSpec spec;
        spec.name = QStringLiteral("only_sometimes");
        spec.description = QStringLiteral("Available only when the flag is set");
        spec.category = QStringLiteral("test");
        spec.effect = ToolEffect::Read;
        spec.paramSchema = obj("{\"type\":\"object\"}");
        spec.available = [] { return false; };
        spec.handler = [](const QJsonObject&) { return ToolResult::success(); };
        QVERIFY(m_registry.add(spec));

        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        LlmClient client;
        client.setProfile(profile);
        LlmSession session(&client, &m_registry, &m_dispatcher);

        QString answer, error;
        session.reset();
        session.setMaxIterations(12);
        server.requests.clear();
        server.reply(200, textResponse("nothing to do"));
        runTurn(session, QStringLiteral("hello"), answer, error);

        const QJsonArray tools = QJsonDocument::fromJson(server.requests.first())
                                     .object()
                                     .value(QStringLiteral("tools"))
                                     .toArray();
        bool offered = false;
        bool othersOffered = false;
        for (const QJsonValue& entry : tools) {
            const QString name = entry.toObject().value(QStringLiteral("function"))
                                     .toObject().value(QStringLiteral("name")).toString();
            if (name == QLatin1String("only_sometimes"))
                offered = true;
            else
                othersOffered = true;
        }
        QVERIFY(!offered);
        QVERIFY(othersOffered);
        QVERIFY(m_registry.catalogueBytes() > 0);
    }

    // --- the agent loop must not be the thread that has to wake it ---------------
    //
    // The discriminating check: a tool that waits for the main thread. With the
    // session on the main thread, the wait could never be woken (delivering the
    // wake-up is the blocked thread's own job). On a worker thread it passes.

    void toolWaitingForMainThreadIsWoken()
    {
        QMutex mutex;
        QWaitCondition condition;
        bool released = false;

        ToolSpec spec;
        spec.name = QStringLiteral("wait_for_main");
        spec.description = QStringLiteral("Wait until the main thread says so");
        spec.category = QStringLiteral("test");
        spec.effect = ToolEffect::Read;
        spec.affinity = ToolAffinity::Any;
        spec.paramSchema = obj("{\"type\":\"object\"}");
        spec.handler = [&mutex, &condition, &released](const QJsonObject&) {
            QMutexLocker lock(&mutex);
            QElapsedTimer clock;
            clock.start();
            while (!released && clock.elapsed() < 4000)
                condition.wait(&mutex, 100);
            return released ? ToolResult::success(QJsonObject{ { QStringLiteral("woken"), true } })
                            : ToolResult::failure(QStringLiteral("never woken"));
        };
        QVERIFY(m_registry.add(spec));

        StubServer server;
        QVERIFY(server.isListening());
        LlmProfile profile;
        profile.name = QStringLiteral("stub");
        profile.baseUrl = server.baseUrl();
        profile.model = QStringLiteral("test-model");
        profile.stream = false;

        QThread thread;
        thread.setObjectName(QStringLiteral("llm-test"));
        auto* threadedClient = new LlmClient;
        auto* threadedSession = new LlmSession(threadedClient, &m_registry, &m_dispatcher);
        threadedClient->setProfile(profile);
        threadedSession->setSystemPrompt(QStringLiteral("You are a test."));
        threadedClient->moveToThread(&thread);
        threadedSession->moveToThread(&thread);
        thread.start();

        server.requests.clear();
        server.enqueue(callResponse("wait_for_main", "{}"));
        server.enqueue(textResponse("woken"));

        QString threadedAnswer;
        bool waitToolSucceeded = false;
        QEventLoop loop;
        connect(threadedSession, &LlmSession::assistantMessage, &loop,
            [&threadedAnswer](const QString& text) { threadedAnswer = text; });
        connect(threadedSession, &LlmSession::toolFinished, &loop,
            [&waitToolSucceeded](const QString& name, const ToolResult& result) {
                if (name == QLatin1String("wait_for_main"))
                    waitToolSucceeded = result.ok;
            });
        connect(threadedSession, &LlmSession::finished, &loop, [&loop] { loop.quit(); });
        connect(threadedSession, &LlmSession::failed, &loop, [&loop] { loop.quit(); });
        QTimer::singleShot(8000, &loop, [&loop] { loop.quit(); });

        // The main thread stays free to do this, which is the whole point.
        QTimer::singleShot(200, [&mutex, &condition, &released] {
            QMutexLocker lock(&mutex);
            released = true;
            condition.wakeAll();
        });

        QMetaObject::invokeMethod(threadedSession, [threadedSession] {
            threadedSession->ask(QStringLiteral("go"));
        }, Qt::QueuedConnection);
        loop.exec();

        QVERIFY(waitToolSucceeded);
        QVERIFY(released);
        QCOMPARE(threadedAnswer, QStringLiteral("woken"));

        thread.quit();
        QVERIFY(thread.wait(2000));
        delete threadedSession;
        delete threadedClient;
    }

    // --- every dispatched tool left an audit record --------------------------------

    void everyDispatchedToolIsAudited()
    {
        // m_auditNames accumulated across all turns in this test run via the
        // dispatcher's callRecorded signal (DirectConnection, same thread). The
        // expected count per tool is the SUM over every test that ran a turn
        // calling it, not just this test's own turns -- hence the comment per tool.
        // get_answer: 1 (readToolRunsUnattended) + 3 (iterationCap: 3 tool rounds)
        //   + 4 attempted-with-cap (closingRoundCallingToolDoesNotOpenAnother: 2 tool
        //     rounds run, the closing round's calls never reach the dispatcher)
        //   + 1 (emptyContentNextToToolCallsStillRunsTool) = 7 dispatched.
        // The 2 in unparseableArgumentsNeverReachHandler were REFUSED before
        // dispatch (malformed args), so they are NOT in the audit.
        // get_answer dispatched: 1 (readToolRunsUnattended) + 3 (iterationCap) + 2
        // (closingRoundCallingToolDoesNotOpenAnother: only the first 2 of 4 calls run,
        // the closing round's call never reaches the dispatcher) + 1
        // (emptyContentNextToToolCallsStillRunsTool) = 7. The malformed-args call in
        // unparseableArgumentsNeverReachHandler is refused BEFORE dispatch, so it
        // is NOT counted here.
        //
        // Relational invariant rather than a hardcoded number: every successful
        // get_answer handler call goes through the dispatcher exactly once, so the
        // audit count must equal the handler's own count. The handler count is the
        // ground truth; the audit must agree with it.
        // get_answer: handler ran m_readCalls times; the audit must record exactly
        // that many DISPATCHES plus one REFUSAL for the malformed-args turn. So:
        // audit count == handler count + (1 if the malformed-args turn ran).
        // The malformed-args turn always runs before this test (alphabetical order),
        // so in a full run the +1 applies.
        QCOMPARE(m_auditNames.count(QStringLiteral("get_answer")), m_readCalls + 1);
        // fit_model_test: 2 real dispatches (approved + InProgram autonomy) + 2
        // REFUSALS (no-policy + declined) = 4 audit entries. The audit must show
        // attempts, not only successes -- that is the point of recording refusals.
        // With ok kept per entry, the split is exact:
        QCOMPARE(m_auditNames.count(QStringLiteral("fit_model_test")),
                 m_computeCalls + 2);
        // In a full run: the refusals recorded are exactly
        //   - 1x fit_model_test (no approval policy set)
        //   - 1x fit_model_test (policy declined)
        //   - 1x no_such_tool  (unknown tool)
        //   - 1x get_answer     (malformed arguments, refused before dispatch)
        //   = 4 refusal audit entries. Standalone runs record fewer.
        if (m_readCalls > 1)
            QCOMPARE(m_auditRefusals, 4);
        // wait_for_main runs on the worker thread (toolWaitingForMainThreadIsWoken);
        // when run standalone it does not run at all, so the audit only sees it in
        // a full run. Assert conditionally: if the worker test ran, the audit has it.
        // (Standalone: 0 is correct. Full run: 1.)
        if (m_readCalls > 1)  // a full run happened: worker test ran too
            QCOMPARE(m_auditNames.count(QStringLiteral("wait_for_main")), 1);
        // An unknown-tool call is recorded too (the dispatcher records the miss,
        // so the audit trail shows a model naming tools that do not exist).
        // Only present in a full run (inventedToolIsAnsweredWithError), because
        // the test runs alphabetically and inventedTool... runs before this one.
        // Detect "full run" by a tool that only an earlier full-run test dispatches.
        if (m_auditNames.contains(QStringLiteral("get_answer")) && m_readCalls > 1)
            QCOMPARE(m_auditNames.count(QStringLiteral("no_such_tool")), 1);
        // only_sometimes was registered but NEVER dispatched (filtered from the
        // catalogue by its availability predicate), so it must NOT be in the audit.
        QVERIFY(!m_auditNames.contains(QStringLiteral("only_sometimes")));
        // In a full run there is always at least one dispatched tool.
        if (m_readCalls > 0)
            QCOMPARE(m_auditNames.isEmpty(), false);
    }
};

QTEST_GUILESS_MAIN(LlmSessionTest)
#include "test_llmsession.moc"
