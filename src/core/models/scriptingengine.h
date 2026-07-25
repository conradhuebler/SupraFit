/*
 * SupraFit - scripting engine interface for user-defined (scripted) models
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

#pragma once

#include <memory>

#include <QtCore/QString>
#include <QtCore/QStringList>

/**
 * @brief Backend flavours a ScriptModel can evaluate its equation with.
 *
 * ExprTk is the fast, always-compiled default; the others are optional and gated by the
 * build flags @c _Models (ChaiScript), @c Use_Duktape and @c _Python. Selection is exposed to the
 * user through the model JSON @c "Engine" field. Claude Generated.
 */
class SpeciationEngine;

enum class ScriptBackend {
    ExprTk = 0,
    ChaiScript,
    Duktape,
    Python,
    QJS
};

/**
 * @brief Backend-agnostic evaluator for a single scalar model equation.
 *
 * The contract is designed so the fit hot loop does @b zero per-point string work for every
 * backend: the formula is compiled @b once in prepare(), every bindable identifier (inputs, global
 * and local parameters, and — for speciation models — species/free-component concentrations) is
 * resolved to an integer @c slot once via slotFor(), and the loop only writes doubles through
 * set(slot,·) before calling evaluate(). Concrete engines: ExprTkEngine (and later Chai/Duktape/
 * Python/QJS wrappers). Claude Generated.
 */
class ScriptingEngine {
public:
    virtual ~ScriptingEngine() = default;

    /** @brief Human-readable backend id, e.g. "ExprTk". */
    virtual QString id() const = 0;

    /**
     * @brief Compile @p formula once, binding every name in @p variableNames as a variable.
     * @return true on successful compilation. Names must be valid identifiers for the backend.
     */
    virtual bool prepare(const QString& formula, const QStringList& variableNames) = 0;

    /** @brief Resolve a bound variable name to its stable slot index, or -1 if unknown. */
    virtual int slotFor(const QString& name) const = 0;

    /** @brief Write a value into a pre-resolved slot (O(1), no lookup). */
    virtual void set(int slot, double value) = 0;

    /** @brief Evaluate the compiled formula against the current slot values. @p error set on failure. */
    virtual double evaluate(int& error) = 0;

    /** @brief Deep copy with its own compiled expression — required for per-thread evaluation. */
    virtual std::unique_ptr<ScriptingEngine> clone() const = 0;

    /**
     * @brief Give the script access to a native equilibrium solver, callable as
     *        @c spec_solve(t0,t1,…) (solve this point's mass balance for the given component totals),
     *        @c spec_free(i) (free concentration of component i) and @c spec_conc(j) (species j).
     *
     * Must be called BEFORE prepare(), because the functions have to exist when the formula is
     * compiled. The stability constants are pushed into @p engine by the model, not by the script.
     * Backends without native solver support ignore this. Claude Generated.
     */
    virtual void setSpeciation(SpeciationEngine* engine) { Q_UNUSED(engine) }

    /** @brief Current data-point index, so spec_solve() can warm-start from this point's previous
     * solution across fit iterations (a much nearer seed than a cold start). Claude Generated. */
    virtual void setSpeciationPoint(int index) { Q_UNUSED(index) }

    /** @brief Diagnostic from the last failed prepare() — the backend's parser error. Claude Generated. */
    virtual QString lastError() const { return QString(); }

    /** @brief Whether the engine offers a whole-column vector fast path (ExprTk). */
    virtual bool supportsVector() const { return false; }

    /** @brief Whether the backend compiled and is usable in this build. */
    virtual bool available() const { return true; }
};

/** @brief Canonical backend name ("ExprTk", "ChaiScript", "Duktape", "Python", "QJS"). CG. */
QString ScriptBackendName(ScriptBackend backend);

/** @brief Parse a backend name (case-insensitive); sets @p ok when non-null. Defaults to ExprTk. CG. */
ScriptBackend ScriptBackendFromString(const QString& name, bool* ok = nullptr);

/** @brief Backends actually compiled into this build (always contains ExprTk). CG. */
QStringList AvailableScriptBackends();

/**
 * @brief Instantiate the requested backend, falling back to ExprTk when it is not compiled in.
 * @param fellBack when non-null, set to true if the requested backend was unavailable.
 */
std::unique_ptr<ScriptingEngine> MakeScriptingEngine(ScriptBackend backend, bool* fellBack = nullptr);
