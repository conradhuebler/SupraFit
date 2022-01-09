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
#pragma once

#include <duk_config.h>
#include <duktape.h>

#include <string>
#include <vector>

#include <Eigen/Dense>

typedef Eigen::MatrixXd Matrix;

class DuktapeModelInterpreter {
public:
    DuktapeModelInterpreter();
    ~DuktapeModelInterpreter();

    void Initialise();
    double Evaluate(int i, int j);
    const char* Evaluate(const char* c);
    void Finalise();

    void setInput(const Matrix& matrix) { m_input = matrix; }
    void setModel(const Matrix& matrix) { m_model = matrix; }

    void setGlobal(const Matrix& matrix, const std::vector<std::string>& names)
    {
        m_global_parameter = matrix;
        m_global_names = names;
    }
    void Update();

    void setLocal(const Matrix& matrix) { m_local_parameter = matrix; }

    //void setExecute(const QStringList& execute) { m_execute = execute; }
    //void Run(const QString& string);

private:
    int m_index = 0;
    Eigen::MatrixXd m_input, m_model, m_global_parameter, m_local_parameter;
    std::vector<std::string> m_global_names, m_local_names;
    //  QStringList m_execute;
    duk_context* m_dukectx;
};
#endif
