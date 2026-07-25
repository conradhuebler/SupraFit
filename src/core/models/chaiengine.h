/*
 * SupraFit - ChaiScript backend for user-defined (scripted) models
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

#include "src/global_config.h"

#ifdef _Models

#include <map>
#include <memory>
#include <vector>

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <chaiscript/chaiscript.hpp>

#include "src/core/models/chaiinterpreter.h" // SupraFitChaiMathModule()
#include "src/core/models/scriptingengine.h"

// From libmath.h — forward declared to keep this header light.
qreal AnalyticCubicRoot(qreal a, qreal b, qreal c, qreal d);

/**
 * @brief ChaiScript-backed ScriptingEngine — parses once, binds variables by reference.
 *
 * This is the FAIR comparison against ExprTk: the equation is parsed to an AST a single time and that
 * AST is re-evaluated, and every bindable name is added as a ChaiScript global referencing a stable
 * C++ double (@c m_values, sized once in prepare() and never resized). The hot loop therefore only
 * writes doubles — no string substitution, no re-parsing. (The legacy ChaiInterpreter did the
 * opposite: it re-eval'd a freshly string-substituted expression for every data point, which is where
 * "ChaiScript is slow" came from and would have made any benchmark meaningless.)
 *
 * ChaiScript stays a tree-walking interpreter, so it is expected to lose against ExprTk's compiled
 * expression — the point is to measure by how much, honestly. Claude Generated.
 */
class ChaiEngine final : public ScriptingEngine {
public:
    ChaiEngine() = default;

    QString id() const override { return QStringLiteral("ChaiScript"); }

    bool prepare(const QString& formula, const QStringList& variableNames) override
    {
        m_formula = formula;
        m_names = variableNames;
        m_index.clear();
        m_compiled = false;
        m_last_error.clear();
        m_ast.reset();

        // Sized once; the engine hands ChaiScript pointers into this buffer, so it must not reallocate.
        m_values.assign(variableNames.size(), 0.0);
        m_chai = std::make_unique<chaiscript::ChaiScript>();
        // sqrt/pow/exp/... are not in the ChaiScript core — without the math extras the engine could
        // only do plain arithmetic, which would make any comparison against ExprTk meaningless. CG.
        m_chai->add(SupraFitChaiMathModule());
        // Same primitive library as the ExprTk backend, so a model stays portable between engines. CG.
        m_chai->add(chaiscript::fun(&AnalyticCubicRoot), "cubic_root");

        try {
            for (int i = 0; i < variableNames.size(); ++i) {
                m_index[variableNames[i]] = i;
                m_chai->add_global(chaiscript::var(&m_values[i]), variableNames[i].toStdString());
            }
            m_ast = m_chai->parse(formula.toStdString());
        } catch (const std::exception& e) {
            m_last_error = QString::fromStdString(e.what());
            return false;
        } catch (...) {
            m_last_error = QStringLiteral("unknown ChaiScript parse error");
            return false;
        }
        m_compiled = static_cast<bool>(m_ast);
        return m_compiled;
    }

    int slotFor(const QString& name) const override
    {
        auto it = m_index.find(name);
        return it == m_index.end() ? -1 : it->second;
    }

    void set(int slot, double value) override
    {
        if (slot >= 0 && slot < static_cast<int>(m_values.size()))
            m_values[slot] = value;
    }

    double evaluate(int& error) override
    {
        if (!m_compiled || !m_ast || !m_chai) {
            error = 1;
            return 0.0;
        }
        error = 0;
        try {
            const chaiscript::Boxed_Value result = m_chai->eval(*m_ast);
            return chaiscript::boxed_cast<double>(result);
        } catch (const chaiscript::exception::bad_boxed_cast&) {
            try { // an integer-valued expression is still a perfectly good result
                return static_cast<double>(chaiscript::boxed_cast<int>(m_chai->eval(*m_ast)));
            } catch (...) {
                error = 1;
                return 0.0;
            }
        } catch (...) {
            error = 1;
            return 0.0;
        }
    }

    std::unique_ptr<ScriptingEngine> clone() const override
    {
        auto copy = std::make_unique<ChaiEngine>();
        copy->prepare(m_formula, m_names);
        return copy;
    }

    QString lastError() const override { return m_last_error; }

private:
    std::unique_ptr<chaiscript::ChaiScript> m_chai;
    chaiscript::AST_NodePtr m_ast;

    std::vector<double> m_values; ///< stable storage referenced by the ChaiScript globals
    std::map<QString, int> m_index; ///< name -> slot
    QStringList m_names;
    QString m_formula;
    QString m_last_error;
    bool m_compiled = false;
};

#endif // _Models
