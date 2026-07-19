/*
 * SupraFit - ExprTk scripting backend for user-defined models
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

#include <map>
#include <memory>
#include <vector>

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <external/exprtk.hpp>

#include "src/core/models/scriptingengine.h"
#include "src/core/speciationengine.h"

// From libmath.h — forward declared so this header does not have to drag in AbstractModel.h.
qreal AnalyticCubicRoot(qreal a, qreal b, qreal c, qreal d);

/**
 * @brief ExprTk-backed ScriptingEngine — compiles once, binds variables by stable slot.
 *
 * Each bindable name gets a fixed index into @c m_values (sized once in prepare() and never
 * resized, so the pointers ExprTk holds via add_variable stay valid). The hot loop resolves each
 * name to its slot exactly once and then only writes doubles + calls value() — no per-point
 * allocation and no QString map lookups (the old ExprTkInterpreter did both per data point).
 * Claude Generated.
 */
class ExprTkEngine final : public ScriptingEngine {
    /*! \brief spec_solve(t0,t1,…) — run the native equilibrium solver for this point's component
     * totals. Variadic, so it adapts to the number of components. Claude Generated. */
    struct SolveFn final : public exprtk::igeneric_function<double> {
        typedef typename exprtk::igeneric_function<double>::parameter_list_t parameter_list_t;
        typedef typename exprtk::igeneric_function<double>::generic_type generic_type;
        typedef typename generic_type::scalar_view scalar_t;

        SpeciationEngine* speciation = nullptr;
        int point = -1; ///< data-point index for the per-point warm-start cache (<0 = cold start)

        inline double operator()(parameter_list_t params) override
        {
            if (!speciation)
                return 0.0;
            std::vector<double> totals(params.size());
            for (std::size_t i = 0; i < params.size(); ++i)
                totals[i] = scalar_t(params[i])();
            speciation->solve(totals, point);
            return static_cast<double>(params.size());
        }
    };

    /*! \brief spec_free(i) — free concentration of component i from the last spec_solve(). CG. */
    struct FreeFn final : public exprtk::ifunction<double> {
        SpeciationEngine* speciation = nullptr;
        FreeFn()
            : exprtk::ifunction<double>(1)
        {
        }
        inline double operator()(const double& index) override
        {
            if (!speciation)
                return 0.0;
            const std::vector<double>& c = speciation->FreeConcentrations();
            const int i = static_cast<int>(index + 0.5);
            return (i >= 0 && i < static_cast<int>(c.size())) ? c[i] : 0.0;
        }
    };

    /*! \brief spec_conc(j) — concentration of species j from the last spec_solve(). CG. */
    struct ConcFn final : public exprtk::ifunction<double> {
        SpeciationEngine* speciation = nullptr;
        ConcFn()
            : exprtk::ifunction<double>(1)
        {
        }
        inline double operator()(const double& index) override
        {
            if (!speciation)
                return 0.0;
            const std::vector<double>& c = speciation->SpeciesConcentrations();
            const int j = static_cast<int>(index + 0.5);
            return (j >= 0 && j < static_cast<int>(c.size())) ? c[j] : 0.0;
        }
    };

    /*! \brief cubic_root(a,b,c,d) — smallest non-negative real root of a x^3 + b x^2 + c x + d, in
     * closed form. Lets a script get an equilibrium concentration without writing an iteration. CG. */
    struct CubicFn final : public exprtk::ifunction<double> {
        CubicFn()
            : exprtk::ifunction<double>(4)
        {
        }
        inline double operator()(const double& a, const double& b, const double& c, const double& d) override
        {
            return AnalyticCubicRoot(a, b, c, d);
        }
    };

    /*! \brief quadratic_root(a,b,c) — smallest non-negative real root of a x^2 + b x + c. CG. */
    struct QuadraticFn final : public exprtk::ifunction<double> {
        QuadraticFn()
            : exprtk::ifunction<double>(3)
        {
        }
        inline double operator()(const double& a, const double& b, const double& c) override
        {
            if (std::abs(a) < 1e-30)
                return (std::abs(b) < 1e-30) ? 0.0 : -c / b;
            const double disc = b * b - 4.0 * a * c;
            if (disc < 0.0)
                return 0.0;
            const double s = std::sqrt(disc);
            const double r1 = (-b + s) / (2.0 * a);
            const double r2 = (-b - s) / (2.0 * a);
            const double lo = std::min(r1, r2), hi = std::max(r1, r2);
            return lo >= 0.0 ? lo : (hi >= 0.0 ? hi : lo);
        }
    };

public:
    ExprTkEngine() = default;

    QString id() const override { return QStringLiteral("ExprTk"); }

    void setSpeciation(SpeciationEngine* engine) override { m_speciation = engine; }
    void setSpeciationPoint(int index) override { m_solve_fn.point = index; }

    bool prepare(const QString& formula, const QStringList& variableNames) override
    {
        m_formula = formula;
        m_names = variableNames;
        m_index.clear();

        m_symbol_table.clear();
        // Size once, bind by reference; must not reallocate after this point.
        m_values.assign(variableNames.size(), 0.0);
        for (int i = 0; i < variableNames.size(); ++i) {
            m_index[variableNames[i]] = i;
            m_symbol_table.add_variable(variableNames[i].toStdString(), m_values[i]);
        }
        m_symbol_table.add_constants();

        // Primitive library: closed-form root finders, so a script never has to hand-roll an
        // iteration for an equilibrium concentration. Claude Generated.
        m_symbol_table.add_function("cubic_root", m_cubic_fn);
        m_symbol_table.add_function("quadratic_root", m_quadratic_fn);

        // Native equilibrium solver, callable from the script. Must be registered before compiling.
        if (m_speciation) {
            m_solve_fn.speciation = m_speciation;
            m_free_fn.speciation = m_speciation;
            m_conc_fn.speciation = m_speciation;
            m_symbol_table.add_function("spec_solve", m_solve_fn);
            m_symbol_table.add_function("spec_free", m_free_fn);
            m_symbol_table.add_function("spec_conc", m_conc_fn);
        }

        m_expression.register_symbol_table(m_symbol_table);
        m_compiled = m_parser.compile(formula.toStdString(), m_expression);
        // Keep the parser diagnostic so the user gets a real message instead of a silently zero model.
        m_last_error = m_compiled ? QString() : QString::fromStdString(m_parser.error());
        return m_compiled;
    }

    QString lastError() const override { return m_last_error; }

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
        if (!m_compiled) {
            error = 1;
            return 0.0;
        }
        error = 0;
        return m_expression.value();
    }

    std::unique_ptr<ScriptingEngine> clone() const override
    {
        auto copy = std::make_unique<ExprTkEngine>();
        copy->setSpeciation(m_speciation); // before prepare(): the functions must exist at compile time
        copy->prepare(m_formula, m_names);
        return copy;
    }

    bool supportsVector() const override { return false; }
    bool available() const override { return true; }

private:
    exprtk::symbol_table<double> m_symbol_table;
    exprtk::expression<double> m_expression;
    exprtk::parser<double> m_parser;

    std::vector<double> m_values; ///< stable per-variable storage bound into the symbol table
    std::map<QString, int> m_index; ///< name -> slot index
    QStringList m_names; ///< bindable names, index-aligned with m_values (kept for clone())
    QString m_formula;
    QString m_last_error; ///< parser diagnostic of the last failed compile
    bool m_compiled = false;

    SpeciationEngine* m_speciation = nullptr; ///< not owned; supplied by the model
    SolveFn m_solve_fn;
    FreeFn m_free_fn;
    ConcFn m_conc_fn;
    CubicFn m_cubic_fn;
    QuadraticFn m_quadratic_fn;
};
