#include "exprtkinterpreter.h"

ExprTkInterpreter::ExprTkInterpreter()
{
}
/*
void ExprTkInterpreter::setGlobal(const Matrix& matrix, const QStringList& names)
{
    m_global_parameter = matrix;
    m_global_names = names;

    // Initialisiere globale Variablen
    for (int i = 0; i < names.size(); ++i) {
        m_variables[names[i]] = matrix(0, i);
    }
    UpdateVariables();
}

void ExprTkInterpreter::setLocal(const Matrix& matrix)
{
    m_local_parameter = matrix;
    UpdateVariables();
}

void ExprTkInterpreter::UpdateVariables()
{
    m_symbol_table.clear();

    // Aktualisiere alle Variablen im Symbol Table
    for (auto& pair : m_variables) {
        m_symbol_table.add_variable(pair.first.toStdString(), pair.second);
    }

    // Aktualisiere globale Parameter
    for (int i = 0; i < m_global_names.size(); ++i) {
        QString name = m_global_names[i];
        m_variables[name] = m_global_parameter(0, i);
        m_symbol_table.add_variable(name.toStdString(), m_variables[name]);
    }

    m_expression.register_symbol_table(m_symbol_table);
}
*/
bool ExprTkInterpreter::compile(const QString& formula)
{
    return m_parser.compile(formula.toStdString(), m_expression);
}

double ExprTkInterpreter::evaluate(const QString& formula, int& error)
{
    error = 0;

    // Versuche zuerst, ob es eine reine Zahl ist
    bool ok;
    double number = formula.toDouble(&ok);
    if (ok)
        return number;

    // Wenn nicht, kompiliere und evaluiere den Ausdruck
    if (!compile(formula)) {
        error = 1;
        return 0.0;
    }

    try {
        return m_expression.value();
    } catch (...) {
        error = 1;
        return 0.0;
    }
}
