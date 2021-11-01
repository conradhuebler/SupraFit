/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifdef _Python
#include <Python.h>

#include <iostream>

#include "pymodelinterpreter.h"

PyModelInterpreter::PyModelInterpreter()
{
    /*
        PyObject *pName, *pModule, *pFunc;
        PyObject *pArgs, *pValue;
        int i;*/

    /*      if (argc < 3) {
            fprintf(stderr,"Usage: call pythonfile funcname [args]\n");
            return 1;
        }
    */
}

void PyModelInterpreter::Run(const QString& string)
{
    // std::cout << string.toStdString() << std::endl;
    PyRun_SimpleString(QString(string).toLocal8Bit());
}

void PyModelInterpreter::InitialisePython()
{
    Py_Initialize();

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
    //Run("print(model)");
    //Run("print(input)");
}

double PyModelInterpreter::EvaluatePython(int i, int j)
{

    PyObject* m_moduleMainString;
    PyObject* m_moduleMain;

    m_moduleMainString = PyUnicode_FromString("__main__");
    m_moduleMain = PyImport_Import(m_moduleMainString);
    PyObject* func = PyObject_GetAttrString(m_moduleMain, "Calculate");
    PyObject* args = PyTuple_Pack(2, PyLong_FromDouble(i), PyLong_FromDouble(j));

    PyObject* result = PyObject_CallObject(func, args);
    return PyFloat_AsDouble(result);
}

void PyModelInterpreter::FinalisePython()
{
    //Run("print(model);\n");

    /*
    PyRun_SimpleString(
        "def mul(a, b):                                 \n"\
        "   print(a)                                    \n"\
        "   print(b)                                \n"\
        "   return float(model[a][b])                          \n"\
    );
  */

    Py_Finalize();
}
#endif
