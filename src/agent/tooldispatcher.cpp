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
    if (!m_registry->tool(name, spec))
        return ToolResult::failure(QStringLiteral("unknown tool '%1'").arg(name));

    QElapsedTimer timer;
    timer.start();
    ToolResult result;
    bool marshalled = false;

    if (spec.affinity == ToolAffinity::Any || QThread::currentThread() == thread()) {
        // Direct call: Any-affinity tools run on the caller's thread, and a Gui
        // tool called from the GUI thread must not deadlock on itself.
        result = m_registry->invoke(name, args);
    } else {
        // Marshall onto this object's (GUI) thread and wait with a timeout.
        marshalled = true;
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

        const bool finished = m_timeoutMs > 0
            ? state->semaphore.tryAcquire(1, m_timeoutMs)
            : (state->semaphore.acquire(1), true);
        if (finished && state->done) {
            result = state->result;
        } else {
            result = ToolResult::failure(
                QStringLiteral("tool '%1' timed out after %2 ms on the GUI thread").arg(name).arg(m_timeoutMs));
        }
    }

    ++m_callCount;
    emit callRecorded(name, args, result.ok, timer.elapsed(), callOrigin());
    return result;
}
