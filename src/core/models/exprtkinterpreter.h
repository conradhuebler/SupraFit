#pragma once

#include <Eigen/Dense>
#include <QStringList>
#include <external/exprtk.hpp>

typedef Eigen::MatrixXd Matrix;

class ExprTkInterpreter {
public:
    ExprTkInterpreter();
    //~ExprTkInterpreter() = default;

    bool PrepareFormula(const QString& formula,
        const QStringList& input_names,
        const QStringList& global_names)
    {
        if (formula == m_current_formula && m_formula_compiled)
            return true;

        m_symbol_table.clear();
        m_x_variables.clear();
        m_global_variables.clear();
        m_local_variables.clear();

        // Erstelle Variablen für die X-Werte
        for (const QString& name : input_names) {
            m_x_variables[name] = new double(0.0);
            m_symbol_table.add_variable(name.toStdString(), *m_x_variables[name]);
        }

        // Erstelle Variablen für globale Parameter
        for (const QString& name : global_names) {
            m_global_variables[name] = new double(0.0);
            m_symbol_table.add_variable(name.toStdString(), *m_global_variables[name]);
        }

        m_expression.register_symbol_table(m_symbol_table);
        m_formula_compiled = m_parser.compile(formula.toStdString(), m_expression);
        m_current_formula = formula;

        return m_formula_compiled;
    }

    void setGlobal(const Matrix& matrix, const QStringList& names)
    {
        for (int i = 0; i < names.size(); ++i) {
            auto it = m_global_variables.find(names[i]);
            if (it != m_global_variables.end()) {
                *(it->second) = matrix(0, i);
            }
        }
    }

    void setLocal(const Matrix& matrix)
    {
        // Falls lokale Parameter verwendet werden
        for (auto& pair : m_local_variables) {
            // Implementiere entsprechende Logik
        }
    }

    double evaluate(const QVector<QPair<QString, double>>& x_values, int& error)
    {
        error = 0;

        // Update X-Werte direkt über die Pointer
        for (const auto& pair : x_values) {
            auto it = m_x_variables.find(pair.first);
            if (it != m_x_variables.end()) {
                *(it->second) = pair.second;
            }
        }

        try {
            return m_expression.value();
        } catch (...) {
            error = 1;
            return 0.0;
        }
    }

    ~ExprTkInterpreter()
    {
        for (auto& pair : m_x_variables) {
            delete pair.second;
        }
        for (auto& pair : m_global_variables) {
            delete pair.second;
        }
        for (auto& pair : m_local_variables) {
            delete pair.second;
        }
    }

    // void setGlobal(const Matrix& matrix, const QStringList& names);
    // void setLocal(const Matrix& matrix);
    // void setInput(const Matrix& matrix) { m_input = matrix; }
    // void setInputNames(const QStringList& names) { m_input_names = names; }

    bool compile(const QString& formula);
    double evaluate(const QString& formula, int& error);

    // Hilfsfunktionen
    // double Input(int i, int j) const { return m_input(j, i); }
    void UpdateVariables();

private:
    exprtk::symbol_table<double> m_symbol_table;
    exprtk::expression<double> m_expression;
    exprtk::parser<double> m_parser;

    std::map<QString, double*> m_x_variables; // Für Input-Variablen (X1, X2, ...)
    std::map<QString, double*> m_global_variables; // Für globale Parameter (A1, A2, ...)
    std::map<QString, double*> m_local_variables; // Für lokale Parameter
    bool m_formula_compiled = false;
    QString m_current_formula;
};
