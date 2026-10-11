// tooldispatcher.cpp - Runs a registered tool on the right thread, and records it.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Modelled on qurcuma's src/core/tooldispatcher.cpp.
// Qt knows no timeout for BlockingQueuedConnection, so a marshalled call is posted
// as a queued invocation and waited on with QSemaphore::tryAcquire; the state
// lives in a shared_ptr so a timeout never describes freed memory.

#include "tooldispatcher.h"

#include <QElapsedTimer>
#include <QMetaObject>
#include <QSemaphore>
#include <QThread>

#include <memory>

namespace {
thread_local QString t_callOrigin;
}  // namespace

ToolDispatcher::ToolDispatcher(ToolRegistry* registry, QObject* parent)
    : QObject(parent)
    , m_registry(registry)
{
}

void ToolDispatcher::setTimeoutMs(int ms)
{
    m_timeoutMs = ms;
}

QString ToolDispatcher::callOrigin()
{
    return t_callOrigin;
}

void ToolDispatcher::setCallOrigin(const QString& origin)
{
    t_callOrigin = origin;
}

ToolResult ToolDispatcher::dispatch(const QString& name, const QJsonObject& args)
{
    if (!m_registry)
        return ToolResult::failure(QStringLiteral("no tool registry configured"));

    ToolSpec spec;
    if (!m_registry->tool(name, spec)) {
        // Even a miss is a call: count it and record it, so the audit trail shows
        // the model naming tools that do not exist (a model-quality signal).
        ++m_callCount;
        const ToolResult miss = ToolResult::failure(QStringLiteral("unknown tool '%1'").arg(name));
        emit callRecorded(name, args, false, 0, callOrigin());
        return miss;
    }

    QElapsedTimer timer;
    timer.start();
    ToolResult result;

    if (spec.affinity == ToolAffinity::Any || QThread::currentThread() == thread()) {
        // Direct call: Any-affinity tools run on the caller's thread, and a Gui
        // tool called from the GUI thread must not deadlock on itself.
        result = m_registry->invoke(name, args);
    } else {
        // Marshall onto this object's (GUI) thread and wait with a timeout.
        struct SharedState {
            QSemaphore semaphore;
            ToolResult result;
            bool done = false;
        };
        auto state = std::make_shared<SharedState>();

        QMetaObject::invokeMethod(this,
            [this, state, name, args]() {
                state->result = m_registry->invoke(name, args);
                state->done = true;
                state->semaphore.release();
            },
            Qt::QueuedConnection);

        // Wait for the marshalled handler with a deadline. The caller's thread
        // must be able to observe an interrupt while it blocks, so we poll the
        // semaphore in 50 ms slices against the deadline instead of a single
        // tryAcquire with the full timeout.
        //
        // BUGFIX (found by test_tooldispatcher, 2026-10-11): the first version
        // waited with tryAcquire(1, m_timeoutMs) and then derived `finished`
        // from state->done alone. On a timeout the handler keeps running on the
        // GUI thread, and when the test's processEvents loop then delivered it,
        // the handler finished and released the semaphore -- but the wait had
        // already returned a timeout failure... which was correct. The actual
        // defect the test exposed: the single tryAcquire blocked the caller for
        // the FULL timeout even when m_interrupted was set, because the interrupt
        // check only ran after tryAcquire returned. An operator pressing Stop
        // during a long marshalled call would not be heard until the timeout
        // expired. Polling in 50 ms slices fixes that: interrupt is observed within
        // 50 ms, and the deadline is still honoured.
        const qint64 deadline = timer.elapsed() + qMax(0, m_timeoutMs);
        bool acquired = false;
        bool interrupted = false;
        if (m_timeoutMs > 0) {
            while (!(acquired = state->semaphore.tryAcquire(1, 50))) {
                if (m_interrupted.load()) {
                    interrupted = true;
                    break;
                }
                if (timer.elapsed() >= deadline)
                    break;  // timed out
            }
        } else {
            state->semaphore.acquire(1);
            acquired = true;
        }

        // A timeout is a failed ToolResult, not a hang. The handler may still
        // complete later; its result is then dropped (state is shared_ptr, so the
        // memory stays valid), but the call still counts and is still recorded.
        //
        // Race note: the handler sets done=true BEFORE releasing the semaphore,
        // so `acquired && state->done` is safe. On a timeout the semaphore is NOT
        // consumed; if the handler finishes later, its release sits in the
        // semaphore and the shared_ptr keeps the state alive until the lambda's
        // captured copy is destroyed on the GUI thread.
        if (acquired && state->done) {
            result = state->result;
        } else if (interrupted) {
            result = ToolResult::failure(
                QStringLiteral("tool '%1' was interrupted").arg(name));
        } else {
            // Timed out, or acquired without done visible (should not happen given
            // the release order, but treat as timeout rather than crash).
            result = ToolResult::failure(
                QStringLiteral("tool '%1' timed out after %2 ms on the GUI thread").arg(name).arg(m_timeoutMs));
        }
    }

    ++m_callCount;
    emit callRecorded(name, args, result.ok, timer.elapsed(), callOrigin());
    return result;
}
