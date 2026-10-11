/*
 * SupraFit - tool dispatcher: thread marshalling, timeout, audit (headless, no net)
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

/* Phase 1 of roadmap/llm_agent_analysis.md: the dispatcher is the one place that
 * knows about threads. ToolRegistry::invoke() runs a handler on whatever thread
 * asks -- fine for a test, wrong for the application, because the agent loop does
 * not live on the GUI thread. This test pins: Any-affinity tools run on the
 * caller's thread, Gui-affinity tools called from another thread are marshalled
 * onto the dispatcher's thread and waited for, a marshalled timeout comes back as
 * a failed ToolResult rather than a hang, and every call is recorded with origin.
 * Claude Generated (2026). */

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QThread>
#include <QtTest/QtTest>

#include <thread>

#include "src/agent/tool.h"
#include "src/agent/tooldispatcher.h"
#include "src/agent/toolregistry.h"

namespace {

QJsonObject emptySchema()
{
    QJsonObject schema;
    schema.insert(QStringLiteral("type"), QStringLiteral("object"));
    schema.insert(QStringLiteral("properties"), QJsonObject());
    return schema;
}

ToolSpec makeSpec(const QString& name, ToolAffinity affinity, std::function<ToolResult(const QJsonObject&)> handler)
{
    ToolSpec spec;
    spec.name = name;
    spec.description = QStringLiteral("test tool");
    spec.category = QStringLiteral("test");
    spec.paramSchema = emptySchema();
    spec.effect = ToolEffect::Read;
    spec.affinity = affinity;
    spec.handler = std::move(handler);
    return spec;
}

// QThread::currentThreadId() returns Qt::HANDLE (void*); compare addresses as strings.
QString threadIdString()
{
    return QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
}

}  // namespace

class ToolDispatcherTest : public QObject {
    Q_OBJECT

private:
    ToolRegistry m_registry;

    void registerTools()
    {
        QVERIFY(m_registry.add(makeSpec(QStringLiteral("any_tool"), ToolAffinity::Any,
            [](const QJsonObject&) {
                QJsonObject data;
                data.insert(QStringLiteral("thread"), threadIdString());
                return ToolResult::success(data);
            })));
        QVERIFY(m_registry.add(makeSpec(QStringLiteral("gui_tool"), ToolAffinity::Gui,
            [](const QJsonObject&) {
                QJsonObject data;
                data.insert(QStringLiteral("thread"), threadIdString());
                return ToolResult::success(data);
            })));
        QVERIFY(m_registry.add(makeSpec(QStringLiteral("slow_gui_tool"), ToolAffinity::Gui,
            [](const QJsonObject&) {
                // 250 ms: long enough that a 100 ms timeout fires first (dispatch
                // must return at ~100 ms, well below this), short enough that
                // draining the pending handler at the end of the test does not
                // slow the suite down.
                QThread::msleep(250);
                return ToolResult::success();
            })));
        // bad_args_tool has a schema requiring "name", so calling without it fails validation
        ToolSpec badArgs = makeSpec(QStringLiteral("bad_args_tool"), ToolAffinity::Any,
            [](const QJsonObject&) { return ToolResult::success(); });
        QJsonObject schema = badArgs.paramSchema;
        QJsonObject props;
        props.insert(QStringLiteral("name"), QJsonObject{ { QStringLiteral("type"), QStringLiteral("string") } });
        schema.insert(QStringLiteral("properties"), props);
        schema.insert(QStringLiteral("required"), QJsonArray{ QStringLiteral("name") });
        badArgs.paramSchema = schema;
        QVERIFY(m_registry.add(badArgs));
    }

private slots:
    void initTestCase()
    {
        registerTools();
    }

    void anyAffinityRunsOnCallingThread()
    {
        ToolDispatcher dispatcher(&m_registry);
        const QString callerThread = threadIdString();
        const ToolResult result = dispatcher.dispatch(QStringLiteral("any_tool"), QJsonObject());
        QVERIFY(result.ok);
        QCOMPARE(result.data.value(QStringLiteral("thread")).toString(), callerThread);
    }

    void guiAffinityCalledFromSameThreadDoesNotDeadlock()
    {
        // A Gui tool called while already on the dispatcher's thread must run
        // directly; BlockingQueuedConnection would deadlock on itself.
        ToolDispatcher dispatcher(&m_registry);
        const ToolResult result = dispatcher.dispatch(QStringLiteral("gui_tool"), QJsonObject());
        QVERIFY(result.ok);
        QCOMPARE(result.data.value(QStringLiteral("thread")).toString(), threadIdString());
    }

    void guiAffinityMarshallsFromWorkerThread()
    {
        ToolDispatcher dispatcher(&m_registry);
        const QString guiThread = threadIdString();

        // The worker runs its own event loop so the queued invocation is delivered;
        // inside it, dispatch() marshalls the Gui-affinity handler onto the main
        // (dispatcher) thread and blocks on a semaphore until it has run there.
        // The main thread must keep delivering events while we wait, hence the
        // processEvents loop below instead of a plain worker.wait().
        QThread worker;
        worker.start();
        struct Result {
            ToolResult result;
            bool dispatched = false;
        };
        auto* result = new Result();
        QMetaObject::invokeMethod(&worker, [&]() {
            result->result = dispatcher.dispatch(QStringLiteral("gui_tool"), QJsonObject());
            result->dispatched = true;
            worker.quit();
        }, Qt::QueuedConnection);

        QElapsedTimer wall;
        wall.start();
        while (!result->dispatched && wall.elapsed() < 5000)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QVERIFY2(result->dispatched, "worker did not finish within 5 s -- marshalling deadlocked");
        QVERIFY(worker.wait(2000));

        QVERIFY(result->dispatched);
        QVERIFY(result->result.ok);
        QCOMPARE(result->result.data.value(QStringLiteral("thread")).toString(), guiThread);
        delete result;
    }

    void marshalledTimeoutReturnsFailureNotHang()
    {
        // The scenario: a Gui-affinity handler (250 ms sleep) is called from a
        // worker thread while the dispatcher has a 100 ms timeout. dispatch() must
        // come back after ~100 ms with a failed ToolResult, not hang for the full
        // handler duration and not crash when the handler finally finishes.
        //
        // IMPORTANT SIMPLIFICATION (after debugging 2026-10-11): the earlier
        // version of this test spun processEvents on the main thread while the
        // worker was inside dispatch(). That delivered the marshalled handler ON
        // THE MAIN THREAD, which then slept 250 ms -- and only released the
        // semaphore AFTER the worker's 100 ms timeout had already fired. The
        // worker therefore observed a TIMEOUT failure, but by the time it stored
        // the result the main thread had finished the handler and the wait loop
        // had exited; the observed result was the handler's success because the
        // timeout path and the handler completion raced. The test could not
        // distinguish "timeout fired" from "handler completed first".
        //
        // The fix: do NOT spin the main event loop while the worker is inside
        // dispatch(). The queued handler then never runs during the wait, the
        // semaphore is never released, and the 100 ms timeout is the ONLY way the
        // worker's dispatch() can return. That is the timeout path under test,
        // isolated from handler completion. Afterwards we drain the pending
        // handler on the main thread so no queued invocation outlives the
        // dispatcher.
        ToolDispatcher dispatcher(&m_registry);
        dispatcher.setTimeoutMs(100);

        struct Result {
            ToolResult result;
            qint64 elapsedMs = 0;
        };
        auto* result = new Result();
        std::thread worker([&dispatcher, result]() {
            // Measure dispatch() ALONE: a real 100 ms timeout shows ~100-150 ms;
            // a bug that waits for the handler shows the full 250 ms.
            QElapsedTimer dispatchTimer;
            dispatchTimer.start();
            result->result = dispatcher.dispatch(QStringLiteral("slow_gui_tool"), QJsonObject());
            result->elapsedMs = dispatchTimer.elapsed();
        });
        worker.join();  // no main-loop spinning: the marshalled handler cannot run

        // A timeout is a failed ToolResult, not a hang: dispatch() came back with
        // "timed out". elapsedMs is scoped to dispatch() alone, so a real 100 ms
        // timeout shows ~100-150 ms -- a bug that silently waited for the handler
        // would show the full 250 ms.
        QVERIFY(!result->result.ok);
        QVERIFY(result->result.error.contains(QStringLiteral("timed out")));
        // 100 ms timeout, well below the 250 ms handler duration.
        QVERIFY2(result->elapsedMs < 200,
            qPrintable(QStringLiteral("timeout took %1 ms, expected ~100 ms").arg(result->elapsedMs)));

        // Drain the still-pending handler on the main thread (~250 ms) so no queued
        // invocation outlives the dispatcher.
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 2000);
        delete result;
    }

    void validationErrorComesBackAsFailedResult()
    {
        ToolDispatcher dispatcher(&m_registry);
        const ToolResult result = dispatcher.dispatch(QStringLiteral("bad_args_tool"), QJsonObject());
        QVERIFY(!result.ok);
        QVERIFY(result.error.contains(QStringLiteral("invalid arguments")));
    }

    void unknownToolComesBackAsFailedResult()
    {
        ToolDispatcher dispatcher(&m_registry);
        const ToolResult result = dispatcher.dispatch(QStringLiteral("ghost"), QJsonObject());
        QVERIFY(!result.ok);
        QVERIFY(result.error.contains(QStringLiteral("unknown tool")));
    }

    void everyCallIsRecordedWithOrigin()
    {
        ToolDispatcher dispatcher(&m_registry);
        struct Record {
            QString name;
            bool ok = false;
            QString origin;
        };
        QList<Record> records;
        // Explicit DirectConnection: callRecorded is emitted synchronously inside
        // dispatch() on the calling (main) thread, and this lambda captures
        // `records` by reference on the stack of the test function -- a queued
        // delivery would run it after dispatch() returned, racing the stack frame.
        QObject::connect(&dispatcher, &ToolDispatcher::callRecorded, this,
            [&](const QString& name, const QJsonObject&, bool ok, qint64, const QString& origin) {
                records.append({ name, ok, origin });
            }, Qt::DirectConnection);

        ToolDispatcher::setCallOrigin(QStringLiteral("assistant"));
        QVERIFY(dispatcher.dispatch(QStringLiteral("any_tool"), QJsonObject()).ok);
        QCOMPARE(records.size(), 1);  // the record arrived synchronously, before return
        ToolDispatcher::setCallOrigin(QString());
        QVERIFY(!dispatcher.dispatch(QStringLiteral("ghost"), QJsonObject()).ok);
        QCOMPARE(records.size(), 2);  // both calls recorded, unknown tool included

        QCOMPARE(records.size(), 2);
        QCOMPARE(records[0].name, QStringLiteral("any_tool"));
        QCOMPARE(records[0].ok, true);
        QCOMPARE(records[0].origin, QStringLiteral("assistant"));
        QCOMPARE(records[1].name, QStringLiteral("ghost"));
        QCOMPARE(records[1].ok, false);
        QCOMPARE(records[1].origin, QString());
        QCOMPARE(dispatcher.callCount(), 2ull);
    }

    void callOriginIsThreadLocal()
    {
        ToolDispatcher::setCallOrigin(QStringLiteral("main-origin"));

        // Run the origin check on a plain std::thread: no Qt event loop involved,
        // so there is no chance the call is silently delivered on the main thread.
        QString workerOrigin = QStringLiteral("unset");
        std::thread worker([&]() { workerOrigin = ToolDispatcher::callOrigin(); });
        worker.join();

        QVERIFY(workerOrigin.isEmpty());  // the worker thread has its own (empty) origin
        QCOMPARE(ToolDispatcher::callOrigin(), QStringLiteral("main-origin"));
        ToolDispatcher::setCallOrigin(QString());
    }
};

QTEST_GUILESS_MAIN(ToolDispatcherTest)
#include "test_tooldispatcher.moc"
