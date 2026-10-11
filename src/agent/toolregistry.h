// toolregistry.h - The one catalogue of callable operations.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Every consumer goes through here: the LLM session sends
// the schemas to the model and runs what it names, a future palette or script dock
// lists them, and ctest drives tools headlessly. Registration is by the module that
// owns the capability, so "GUI-free" is a property of this registry and of the core
// tools, not something every tool has to pretend to be. Modelled on qurcuma's
// src/core/toolregistry.h.
//
// This file deliberately has no notion of approval or thread marshalling; that is
// the dispatcher's job. What lives here is the catalogue and the contract check on
// arguments.
#pragma once

#include "tool.h"

#include <QHash>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QVector>

/// Outcome of checking an argument object against a tool's schema.
struct ToolValidation {
    bool ok = false;
    QString error;  ///< empty when ok; names the offending parameter otherwise
};

class ToolRegistry {
public:
    ToolRegistry() = default;

    /// Process-wide catalogue. Tests build their own instances instead.
    static ToolRegistry& instance();

    /// Register @p spec. Fails, without registering, when the name is empty or
    /// already taken, when the handler is null, or when the schema is malformed --
    /// a bad schema is a startup error here rather than a surprise at call time.
    bool add(const ToolSpec& spec, QString* error = nullptr);

    bool contains(const QString& name) const;
    /// Copy of the registered spec. Returns false when @p name is unknown.
    bool tool(const QString& name, ToolSpec& out) const;
    /// All registered names, sorted, so catalogues and tests are reproducible.
    QStringList names() const;

    /// Size of the tool catalogue in bytes of compact JSON, as it goes out in the
    /// `tools:` field of every request. The recurring cost of the whole feature,
    /// and it grows with each tool, so it is measured rather than estimated.
    int catalogueBytes() const;
    QVector<ToolSpec> all() const;
    int size() const;
    void clear();

    /// Check @p args against the tool's schema without running anything.
    ToolValidation validate(const QString& name, const QJsonObject& args) const;

    /// Validate, then run the handler. Argument errors come back as a failed
    /// ToolResult, so a caller has one error path rather than two.
    ToolResult invoke(const QString& name, const QJsonObject& args) const;

    /// Is @p schema a parameter schema this registry can honour? Exposed so tests
    /// (and tool authors) can check a schema without registering it.
    ///
    /// The honoured subset is deliberately small:
    ///   { "type": "object",
    ///     "properties": { "<name>": { "type": "string|number|integer|boolean|array|object",
    ///                                 "description": "...",   // optional
    ///                                 "enum": [...],          // optional
    ///                                 "minimum": n,           // optional, numbers only
    ///                                 "maximum": n } },       // optional, numbers only
    ///     "required": [ "<name>", ... ] }                     // optional
    /// Anything else in the schema is rejected rather than silently ignored, so a
    /// schema never promises the model more than the validator enforces.
    static bool isValidSchema(const QJsonObject& schema, QString* error = nullptr);

    /// Check @p args against @p schema. Unknown keys are an error: a model that
    /// invents a parameter must be told, not quietly obeyed with it dropped.
    static ToolValidation validateAgainst(const QJsonObject& schema, const QJsonObject& args);

private:
    mutable QMutex m_mutex;
    QHash<QString, ToolSpec> m_tools;
};
