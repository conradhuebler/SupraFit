// tooldispatcher.h - Runs a registered tool on the right thread, and records it.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - ToolRegistry::invoke() runs a handler on whatever thread
// asks. That is fine for a test and wrong for the application: the agent loop does
// not live on the GUI thread, and a handler reaching the GUI's project view from
// anywhere else is a data race -- silent, not a crash. Modelled on qurcuma's
// src/core/tooldispatcher.h.
//
// The dispatcher is the one place that knows about threads and about the audit
// trail. It must LIVE on the GUI thread; ToolAffinity::Gui handlers are posted to
// it and run there.
#pragma once

#include "tool.h"
#include "toolregistry.h"

#include <atomic>

#include <QObject>
#include <QString>

class ToolDispatcher : public QObject {
    Q_OBJECT

public:
    /// Default wait for a marshalled call before giving up.
    static constexpr int kDefaultTimeoutMs = 15000;

    /// @p registry is borrowed, not owned. Create this on the GUI thread -- that
    /// is the thread Gui-affinity handlers will run on.
    explicit ToolDispatcher(ToolRegistry* registry, QObject* parent = nullptr);

    /// How long dispatch() waits for a marshalled handler. 0 or less disables the
    /// limit (the call then waits indefinitely, as BlockingQueuedConnection would).
    void setTimeoutMs(int ms);
    int timeoutMs() const { return m_timeoutMs; }

    /// Ask any tool that is waiting to give up now. Read by waiting handlers on the
    /// agent loop's thread -- a queued cancel() cannot reach a loop that is itself
    /// blocked, so the flag is the only way out of a long wait. Claude Generated 2026.
    void requestInterrupt() { m_interrupted.store(true); }
    void clearInterrupt() { m_interrupted.store(false); }
    bool isInterrupted() const { return m_interrupted.load(); }

    /// Validate, run on the appropriate thread, and record the call.
    ///
    /// A ToolAffinity::Gui handler called from another thread is posted to this
    /// object's thread and waited for. A timeout comes back as a failed ToolResult
    /// rather than a hang -- the handler may still complete afterwards, its result
    /// is simply dropped.
    ToolResult dispatch(const QString& name, const QJsonObject& args);

    /// How many calls this dispatcher has run (audit/diagnostics).
    quint64 callCount() const { return m_callCount; }

    /// Who is making the calls on this thread: "assistant", "script", or empty for
    /// the operator's own use. Thread-local, so the agent loop and the GUI thread
    /// cannot overwrite each other's answer. Claude Generated 2026.
    static QString callOrigin();
    static void setCallOrigin(const QString& origin);

signals:
    /// After every call, whatever its outcome. @p origin travels with the signal
    /// rather than being read back by the receiver, because the origin is
    /// thread-local and a queued delivery runs on another thread.
    void callRecorded(const QString& name, const QJsonObject& args, bool ok, qint64 elapsedMs,
        const QString& origin);

private:
    ToolRegistry* m_registry = nullptr;
    int m_timeoutMs = kDefaultTimeoutMs;
    std::atomic<bool> m_interrupted { false };
    quint64 m_callCount = 0;
};
