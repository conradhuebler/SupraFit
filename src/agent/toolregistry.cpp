// toolregistry.cpp - The one catalogue of callable operations.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Modelled on qurcuma's src/core/toolregistry.cpp. The
// schema check is ~80 lines against QJsonObject: no JSON-Schema library, the build
// already carries enough dependencies.

#include "toolregistry.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>

namespace {

bool isValidTypeName(const QString& type)
{
    static const QSet<QString> types = { QStringLiteral("string"), QStringLiteral("number"),
        QStringLiteral("integer"), QStringLiteral("boolean"), QStringLiteral("array"),
        QStringLiteral("object") };
    return types.contains(type);
}

}  // namespace

ToolRegistry& ToolRegistry::instance()
{
    // The registry is created once at startup before any worker thread exists;
    // function-local static makes the first construction thread-safe (C++11).
    static ToolRegistry* instance = new ToolRegistry();
    return *instance;
}

bool ToolRegistry::add(const ToolSpec& spec, QString* error)
{
    QString localError;
    if (spec.name.isEmpty()) {
        localError = QStringLiteral("tool name is empty");
    } else if (!spec.handler) {
        localError = QStringLiteral("tool '%1' has no handler").arg(spec.name);
    } else if (!isValidSchema(spec.paramSchema, &localError)) {
        localError = QStringLiteral("tool '%1' has a malformed schema: %2").arg(spec.name, localError);
    } else {
        QMutexLocker lock(&m_mutex);
        if (m_tools.contains(spec.name))
            localError = QStringLiteral("tool '%1' is already registered").arg(spec.name);
        else {
            m_tools.insert(spec.name, spec);
            return true;
        }
    }
    if (error)
        *error = localError;
    return false;
}

bool ToolRegistry::contains(const QString& name) const
{
    QMutexLocker lock(&m_mutex);
    return m_tools.contains(name);
}

bool ToolRegistry::tool(const QString& name, ToolSpec& out) const
{
    QMutexLocker lock(&m_mutex);
    auto it = m_tools.constFind(name);
    if (it == m_tools.constEnd())
        return false;
    out = it.value();
    return true;
}

QStringList ToolRegistry::names() const
{
    QMutexLocker lock(&m_mutex);
    QStringList names = m_tools.keys();
    names.sort();
    return names;
}

int ToolRegistry::catalogueBytes() const
{
    // The catalogue as it goes out in the `tools:` field of every request: one
    // {"type":"function","function":{name,description,parameters}} per tool.
    QJsonArray catalogue;
    const QVector<ToolSpec> specs = all();
    for (const ToolSpec& spec : specs) {
        QJsonObject function;
        function.insert(QStringLiteral("name"), spec.name);
        function.insert(QStringLiteral("description"), spec.description);
        function.insert(QStringLiteral("parameters"), spec.paramSchema);
        QJsonObject entry;
        entry.insert(QStringLiteral("type"), QStringLiteral("function"));
        entry.insert(QStringLiteral("function"), function);
        catalogue.append(entry);
    }
    return QJsonDocument(catalogue).toJson(QJsonDocument::Compact).size();
}

QVector<ToolSpec> ToolRegistry::all() const
{
    QMutexLocker lock(&m_mutex);
    return m_tools.values().toVector();
}

int ToolRegistry::size() const
{
    QMutexLocker lock(&m_mutex);
    return m_tools.size();
}

void ToolRegistry::clear()
{
    QMutexLocker lock(&m_mutex);
    m_tools.clear();
}

ToolValidation ToolRegistry::validate(const QString& name, const QJsonObject& args) const
{
    ToolSpec spec;
    if (!tool(name, spec))
        return { false, QStringLiteral("unknown tool '%1'").arg(name) };
    return validateAgainst(spec.paramSchema, args);
}

ToolResult ToolRegistry::invoke(const QString& name, const QJsonObject& args) const
{
    ToolSpec spec;
    if (!tool(name, spec))
        return ToolResult::failure(QStringLiteral("unknown tool '%1'").arg(name));
    const ToolValidation v = validateAgainst(spec.paramSchema, args);
    if (!v.ok)
        return ToolResult::failure(QStringLiteral("invalid arguments for '%1': %2").arg(name, v.error));
    if (!spec.available || spec.available())
        return spec.handler(args);
    return ToolResult::failure(QStringLiteral("tool '%1' is not available in the current state").arg(name));
}

bool ToolRegistry::isValidSchema(const QJsonObject& schema, QString* error)
{
    QString localError;
    auto fail = [&](const QString& msg) -> bool {
        localError = msg;
        if (error)
            *error = msg;
        return false;
    };

    if (!schema.contains(QStringLiteral("type")) || schema.value(QStringLiteral("type")).toString() != QStringLiteral("object"))
        return fail(QStringLiteral("schema must have type \"object\""));

    const QJsonObject properties = schema.value(QStringLiteral("properties")).toObject();
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        const QJsonObject prop = it.value().toObject();
        if (!prop.contains(QStringLiteral("type")) || !isValidTypeName(prop.value(QStringLiteral("type")).toString()))
            return fail(QStringLiteral("property '%1' has no valid type").arg(it.key()));
        if (prop.contains(QStringLiteral("enum"))) {
            if (!prop.value(QStringLiteral("enum")).isArray() || prop.value(QStringLiteral("enum")).toArray().isEmpty())
                return fail(QStringLiteral("property '%1': enum must be a non-empty array").arg(it.key()));
        }
        const QString type = prop.value(QStringLiteral("type")).toString();
        if (prop.contains(QStringLiteral("minimum")) && type != QStringLiteral("number") && type != QStringLiteral("integer"))
            return fail(QStringLiteral("property '%1': minimum is only valid for numbers").arg(it.key()));
        if (prop.contains(QStringLiteral("maximum")) && type != QStringLiteral("number") && type != QStringLiteral("integer"))
            return fail(QStringLiteral("property '%1': maximum is only valid for numbers").arg(it.key()));
    }

    if (schema.contains(QStringLiteral("required"))) {
        const QJsonArray required = schema.value(QStringLiteral("required")).toArray();
        for (const QJsonValue& v : required) {
            if (!v.isString())
                return fail(QStringLiteral("required entries must be strings"));
            if (!properties.contains(v.toString()))
                return fail(QStringLiteral("required property '%1' is not declared in properties").arg(v.toString()));
        }
    }

    // Reject keys outside the honoured subset: a schema must not promise the model
    // more than the validator enforces.
    static const QSet<QString> allowed = { QStringLiteral("type"), QStringLiteral("properties"),
        QStringLiteral("required") };
    for (auto it = schema.begin(); it != schema.end(); ++it) {
        if (!allowed.contains(it.key()))
            return fail(QStringLiteral("unsupported schema key '%1'").arg(it.key()));
    }
    static const QSet<QString> allowedProp = { QStringLiteral("type"), QStringLiteral("description"),
        QStringLiteral("enum"), QStringLiteral("minimum"), QStringLiteral("maximum") };
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        const QJsonObject prop = it.value().toObject();
        for (auto pit = prop.begin(); pit != prop.end(); ++pit) {
            if (!allowedProp.contains(pit.key()))
                return fail(QStringLiteral("unsupported key '%1' in property '%2'").arg(pit.key(), it.key()));
        }
    }

    if (error)
        *error = localError;
    return localError.isEmpty();
}

ToolValidation ToolRegistry::validateAgainst(const QJsonObject& schema, const QJsonObject& args)
{
    // Unknown keys are an error: a model that invents a parameter must be told,
    // not quietly obeyed with it dropped.
    const QJsonObject properties = schema.value(QStringLiteral("properties")).toObject();
    for (auto it = args.begin(); it != args.end(); ++it) {
        if (!properties.contains(it.key()))
            return { false, QStringLiteral("unknown parameter '%1'").arg(it.key()) };
    }

    const QJsonArray required = schema.value(QStringLiteral("required")).toArray();
    for (const QJsonValue& v : required) {
        if (!args.contains(v.toString()))
            return { false, QStringLiteral("missing required parameter '%1'").arg(v.toString()) };
    }

    for (auto it = properties.begin(); it != properties.end(); ++it) {
        const QString& name = it.key();
        if (!args.contains(name))
            continue;
        const QJsonObject prop = it.value().toObject();
        const QJsonValue value = args.value(name);
        const QString type = prop.value(QStringLiteral("type")).toString();

        bool typeOk = false;
        if (type == QStringLiteral("string"))       typeOk = value.isString();
        else if (type == QStringLiteral("number"))  typeOk = value.isDouble();
        else if (type == QStringLiteral("integer")) typeOk = value.isDouble() && value.toDouble() == std::floor(value.toDouble());
        else if (type == QStringLiteral("boolean")) typeOk = value.isBool();
        else if (type == QStringLiteral("array"))   typeOk = value.isArray();
        else if (type == QStringLiteral("object"))  typeOk = value.isObject();
        if (!typeOk)
            return { false, QStringLiteral("parameter '%1' must be of type %2").arg(name, type) };

        if (prop.contains(QStringLiteral("enum"))) {
            const QJsonArray allowed = prop.value(QStringLiteral("enum")).toArray();
            bool found = false;
            for (const QJsonValue& a : allowed) {
                if (a == value) { found = true; break; }
            }
            if (!found)
                return { false, QStringLiteral("parameter '%1' must be one of the enum values").arg(name) };
        }
        if ((type == QStringLiteral("number") || type == QStringLiteral("integer")) && value.isDouble()) {
            const double number = value.toDouble();
            if (prop.contains(QStringLiteral("minimum")) && number < prop.value(QStringLiteral("minimum")).toDouble())
                return { false, QStringLiteral("parameter '%1' is below the minimum").arg(name) };
            if (prop.contains(QStringLiteral("maximum")) && number > prop.value(QStringLiteral("maximum")).toDouble())
                return { false, QStringLiteral("parameter '%1' is above the maximum").arg(name) };
        }
    }
    return { true, QString() };
}
