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

#include <QtCore/QRegularExpression>
#include <QtCore/QSet>

#include <algorithm>
#include <deque>
#include <limits>

#ifdef _Models
#include "chaiengine.h"
#endif

// Additional backends are wired here as they are ported to the ScriptingEngine interface
// (guarded by _Models / Use_Duktape / _Python). ExprTk is always compiled. Claude Generated.

QStringList ExprTkEngine::CollectSymbols(const QString& formula)
{
    // exprtk can enumerate the free variables of an expression without compiling it against a symbol
    // table. That is what lets the dialog derive the parameter list from the equation instead of
    // making the user declare it twice. Claude Generated.
    // The collection pass PARSES the expression, so every function it may call has to be known —
    // with an empty symbol table a single cubic_root() makes the whole collection fail and return
    // nothing. Register the primitive library and the speciation hooks (they are never called here,
    // only resolved) and the built-in constants, so pi & co. are not mistaken for parameters.
    exprtk::symbol_table<double> table;
    CubicFn cubic;
    QuadraticFn quadratic;
    SolveFn solve;
    FreeFn freeConc;
    ConcFn speciesConc;
    table.add_function("cubic_root", cubic);
    table.add_function("quadratic_root", quadratic);
    table.add_function("spec_solve", solve);
    table.add_function("spec_free", freeConc);
    table.add_function("spec_conc", speciesConc);
    table.add_constants();

    std::deque<std::string> symbols;
    if (!exprtk::collect_variables(formula.toStdString(), table, symbols))
        return QStringList();

    // Comments are stripped before anything is searched in the text: a name mentioned in a comment
    // must not decide the spelling or the ordering of a real parameter.
    QString code = formula;
    code.replace(QRegularExpression(QStringLiteral("/\\*.*?\\*/"), QRegularExpression::DotMatchesEverythingOption),
        QStringLiteral(" "));
    code.replace(QRegularExpression(QStringLiteral("//[^\n]*")), QStringLiteral(" "));

    // Script-local variables ("var t := …") are free variables to the collector but are NOT model
    // parameters — they are scratch values the script itself defines. exprtk is case-insensitive, so
    // the comparison is too.
    QSet<QString> declared;
    static const QRegularExpression varDecl(QStringLiteral("\\bvar\\s+([A-Za-z_][A-Za-z0-9_]*)"));
    QRegularExpressionMatchIterator it = varDecl.globalMatch(code);
    while (it.hasNext())
        declared.insert(it.next().captured(1).toLower());

    // exprtk hands the names back LOWER-CASED and alphabetically ordered. Recover the spelling that
    // actually occurs in the equation — that is the identifier the user reads and the model binds —
    // and order by first appearance so the derived parameter order follows the reading order.
    struct Symbol {
        QString name;
        qsizetype position;
    };
    QVector<Symbol> found;
    for (const std::string& s : symbols) {
        const QString lowered = QString::fromStdString(s);
        if (declared.contains(lowered))
            continue;
        const QRegularExpression occurrence(QStringLiteral("\\b") + QRegularExpression::escape(lowered)
                + QStringLiteral("\\b"),
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match = occurrence.match(code);
        found.push_back({ match.hasMatch() ? match.captured(0) : lowered,
            match.hasMatch() ? match.capturedStart(0) : std::numeric_limits<qsizetype>::max() });
    }
    std::stable_sort(found.begin(), found.end(),
        [](const Symbol& a, const Symbol& b) { return a.position < b.position; });

    QStringList names;
    for (const Symbol& symbol : found)
        names << symbol.name;
    return names;
}

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
