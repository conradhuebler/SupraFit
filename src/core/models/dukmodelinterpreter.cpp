/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2021 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifdef Use_Duktape
#include <duk_config.h>
#include <duktape.h>

#include <iostream>

#include "dukmodelinterpreter.h"

DuktapeModelInterpreter::DuktapeModelInterpreter()
{
}

DuktapeModelInterpreter::~DuktapeModelInterpreter()
{
}

/*
void DuktapeModelInterpreter::Run(const QString& string)
{
}
*/
void DuktapeModelInterpreter::Initialise()
{
    m_dukectx = duk_create_heap_default();
    /*
    for (int i = 0; i < m_global_names.size(); ++i) {
        Run(QString("del %1").arg(m_global_names[i]));
        QString execute = QString("%1 = %2").arg(m_global_names[i]).arg(m_global_parameter(0, i));
        Run(execute);
    }
    QString execute = QString("input = [[0 for x in range(%1)] for y in range(%2)] ").arg(m_input.cols()).arg(m_input.rows());
    Run(execute);
    for (int i = 0; i < m_input.cols(); ++i) {
        for (int j = 0; j < m_input.rows(); ++j) {
            QString execute = QString("input[%1][%2] = %3").arg(j).arg(i).arg(m_input(j, i));
            Run(execute);
        }
    }

    execute = QString("model = [[0 for x in range(%1)] for y in range(%2)] ").arg(m_model.cols()).arg(m_model.rows());
    Run(execute);
    for (int i = 0; i < m_model.cols(); ++i) {
        for (int j = 0; j < m_model.rows(); ++j) {
            QString execute = QString("model[%1][%2] = %3").arg(j).arg(i).arg(m_model(j, i));
            Run(execute);
        }
    }

    QString joined;

    for (int i = 0; i < m_execute.size(); ++i) {
        joined += m_execute[i] + "\n";
    }
    Run(joined);
    */
    // Run("print(model)");
    // Run("print(input)");
}

void DuktapeModelInterpreter::Update()
{
    std::cout << m_global_parameter << std::endl;
    for (int i = 0; i < m_global_names.size(); ++i) {
        std::string s = m_global_names[i].append(" = ").append(std::to_string(m_global_parameter(0, i)));
        const char x = ',';
        const char y = '.';
        std::replace(s.begin(), s.end(), x, y);
        // std::cout << s << std::endl;
        duk_push_string(m_dukectx, s.c_str());

        duk_int_t rc = duk_peval(m_dukectx);
        if (rc != 0) {
            duk_safe_to_stacktrace(m_dukectx, -1);
        } else {
            duk_safe_to_string(m_dukectx, -1);
        }
        // std::cout << (duk_get_string(m_dukectx, -1)) << std::endl;
        // chai.set_global(chaiscript::const_var(m_global_parameter(0, i)), m_global_names[i].toStdString());
    }
    /*
    for (int j = 0; j < m_input_names.size(); ++j) {
        std::vector<double> vector;
        for (int i = 0; i < m_input.rows(); ++i)
            vector.push_back(m_input.row(i)[j]);
        chai.set_global(chaiscript::const_var(vector), m_input_names[j].toStdString());
    }*/
}

const char* DuktapeModelInterpreter::Evaluate(const char* c)
{
    // Update();
    // std::cout << c << std::endl;
    duk_push_string(m_dukectx, c);
    duk_int_t rc = duk_peval(m_dukectx);
    if (rc != 0) {
        duk_safe_to_stacktrace(m_dukectx, -1);
    } else {
        duk_safe_to_string(m_dukectx, -1);
    }
    return (duk_get_string(m_dukectx, -1));
}

double DuktapeModelInterpreter::Evaluate(int i, int j)
{
    std::cout << "lala" << std::endl;
    return 0.0;
}

void DuktapeModelInterpreter::Finalise()
{
    duk_destroy_heap(m_dukectx);
}
#endif
