// tool.h - Value types of the agent tool layer.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - A tool is a named operation with a typed parameter
// schema and a structured result. The same records feed three consumers: the LLM
// session (which sends the schemas to the model and executes what it names), a
// future command palette or script dock, and ctest. Modelled on qurcuma's
// src/core/tool.h (docs/WP-llm-tool-layer.md).
//
// Value types only -- no registry, no Qt Widgets -- so a module can declare tools
// without pulling the registry in.
#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QMetaType>
#include <QString>

#include <functional>

/// What running a tool does. This is what the approval policy keys off: Read and
/// Display run unattended, everything else asks once per tool and per session.
enum class ToolEffect {
    Read,       ///< reads state; changes nothing
    Display,    ///< changes what is shown (representation, colours, camera, selection)
    Mutate,     ///< changes the data (model parameters, project content)
    Compute,    ///< starts a calculation (fit, statistical post-processing)
    FileWrite,  ///< writes to disk
    Process     ///< starts an external program
};

/// How much the assistant may do without being asked. The operator sets it per
/// session; it is one switch over the effects above rather than a per-tool list,
/// so what it permits can be read off in one line.
enum class ToolAutonomy {
    Ask,        ///< Read and Display run; everything else asks
    InProgram,  ///< also Mutate and Compute: everything that stays inside SupraFit
    Full        ///< also FileWrite and Process: nothing asks
};

/// Does @p effect still need a human answer at @p autonomy? The single place the
/// rule lives, so the session's policy and its label cannot say different things.
bool needsApprovalAt(ToolEffect effect, ToolAutonomy autonomy);

/// Name of @p autonomy for a label and for the log.
QString toolAutonomyName(ToolAutonomy autonomy);

/// Which thread a handler must run on. SupraFit's fits are compute-heavy and run
/// off the GUI thread; anything reaching a widget or the GUI's project view is Gui,
/// and the dispatcher marshals those. Getting this wrong is a data race, not a
/// crash you would notice in testing -- which is why it is part of the contract
/// from the first commit.
enum class ToolAffinity {
    Any,  ///< safe to run on the calling thread
    Gui   ///< must run on the GUI thread
};

/// What a tool hands back. @a data is the machine-readable part, @a text an
/// optional human/model-readable summary. @a truncated says the tool had more to
/// give and stopped at its cap -- a reader can then page rather than assume it
/// saw everything.
struct ToolResult {
    bool ok = false;
    QJsonObject data;
    QString text;
    QByteArray image;       ///< raw image bytes; empty unless the tool renders one
    QString imageMimeType;  ///< e.g. "image/png"; empty when image is empty
    bool truncated = false;
    QString error;          ///< set when ok is false

    static ToolResult success(const QJsonObject& data = {}, const QString& text = {})
    {
        ToolResult r;
        r.ok = true;
        r.data = data;
        r.text = text;
        return r;
    }

    static ToolResult failure(const QString& error)
    {
        ToolResult r;
        r.ok = false;
        r.error = error;
        return r;
    }
};

/// One registered operation.
struct ToolSpec {
    QString name;         ///< unique, snake_case, e.g. "get_model_fit"
    QString description;  ///< what it does, for the model; the schema carries the how
    QString category;     ///< grouping for logs and a future palette

    /// JSON-Schema subset describing the argument object. Validated when the tool
    /// is registered, so a malformed schema fails at startup rather than at call
    /// time. See ToolRegistry for the subset that is actually honoured.
    QJsonObject paramSchema;

    ToolEffect effect = ToolEffect::Read;
    ToolAffinity affinity = ToolAffinity::Any;

    /// Can this tool do anything right now? Unset means always. The catalogue is
    /// sent with EVERY request, so a tool that cannot work in the current state is
    /// not worth paying for -- and leaving it out also stops a model calling it and
    /// having to be told no. Evaluated when the catalogue is built.
    std::function<bool()> available;

    std::function<ToolResult(const QJsonObject&)> handler;
};

/// Stable spellings for the catalogue and for logs.
QString toolEffectName(ToolEffect effect);
QString toolAffinityName(ToolAffinity affinity);

// Claude Generated 2026 - The agent loop runs on its own thread, so a ToolResult
// crosses one on its way back to the session. A queued connection needs the type
// registered or the signal is dropped with a runtime warning and no answer.
Q_DECLARE_METATYPE(ToolResult)
