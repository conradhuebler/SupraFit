/*
 * SupraFit - ExprTk scripting backend + scripting-engine factory
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

#include "src/global_config.h"

#include "exprtkinterpreter.h"
#include "scriptingengine.h"

#ifdef _Models
#include "chaiengine.h"
#endif

// Additional backends are wired here as they are ported to the ScriptingEngine interface
// (guarded by _Models / Use_Duktape / _Python). ExprTk is always compiled. Claude Generated.

QString ScriptBackendName(ScriptBackend backend)
{
    switch (backend) {
    case ScriptBackend::ExprTk:
        return QStringLiteral("ExprTk");
    case ScriptBackend::ChaiScript:
        return QStringLiteral("ChaiScript");
    case ScriptBackend::Duktape:
        return QStringLiteral("Duktape");
    case ScriptBackend::Python:
        return QStringLiteral("Python");
    case ScriptBackend::QJS:
        return QStringLiteral("QJS");
    }
    return QStringLiteral("ExprTk");
}

ScriptBackend ScriptBackendFromString(const QString& name, bool* ok)
{
    const QString n = name.trimmed().toLower();
    if (ok)
        *ok = true;
    if (n == "exprtk")
        return ScriptBackend::ExprTk;
    if (n == "chaiscript" || n == "chai")
        return ScriptBackend::ChaiScript;
    if (n == "duktape" || n == "duk")
        return ScriptBackend::Duktape;
    if (n == "python" || n == "py")
        return ScriptBackend::Python;
    if (n == "qjs" || n == "qjsengine" || n == "javascript" || n == "js")
        return ScriptBackend::QJS;
    if (ok)
        *ok = false;
    return ScriptBackend::ExprTk;
}

QStringList AvailableScriptBackends()
{
    QStringList list;
    list << ScriptBackendName(ScriptBackend::ExprTk);
#ifdef _Models
    list << ScriptBackendName(ScriptBackend::ChaiScript);
#endif
#ifdef Use_Duktape
    list << ScriptBackendName(ScriptBackend::Duktape);
#endif
#ifdef _Python
    list << ScriptBackendName(ScriptBackend::Python);
#endif
    return list;
}

std::unique_ptr<ScriptingEngine> MakeScriptingEngine(ScriptBackend backend, bool* fellBack)
{
    if (fellBack)
        *fellBack = false;

    switch (backend) {
    case ScriptBackend::ExprTk:
        return std::make_unique<ExprTkEngine>();
#ifdef _Models
    case ScriptBackend::ChaiScript:
        return std::make_unique<ChaiEngine>();
#endif
    // TODO: return the ported Duktape / Python / QJS engines here, each behind its build flag.
    // Until then, those requests fall back to ExprTk.
#ifndef _Models
    case ScriptBackend::ChaiScript:
#endif
    case ScriptBackend::Duktape:
    case ScriptBackend::Python:
    case ScriptBackend::QJS:
    default:
        if (fellBack)
            *fellBack = true;
        return std::make_unique<ExprTkEngine>();
    }
}
