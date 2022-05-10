/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2020 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifdef _Models

#include <QtDebug>

//#define CHAISCRIPT_NO_THREADS

#include <chaiscript/chaiscript.hpp>
//#include <chaiscript/chaiscript_threading.hpp>
#include <external/ChaiScript_Extras/include/chaiscript/extras/math.hpp>

#include <iostream>

#include "chaiinterpreter.h"

ChaiInterpreter::ChaiInterpreter()
{

    m_chaiinterpreter.add(chaiscript::type_conversion<int, double>());
    // m_chaiinterpreter.add(chaiscript::type_conversion<double, int>());

    auto mathlib = chaiscript::extras::math::bootstrap();
    m_chaiinterpreter.add(mathlib);

    // chai.add(chaiscript::bootstrap::standard_library::vector_type<std::vector<double>>("vector"));
    // chai.add(chaiscript::vector_conversion<std::vector<double>>());
    // chai.add(chaiscript::vector_conversion<std::vector<chaiscript::Boxed_Value>>());

    /*
    chai.add(chaiscript::type_conversion<std::vector<chaiscript::Boxed_Value>,
    std::vector<std::vector<double>>> ([](const
    std::vector<chaiscript::Boxed_Value> &t_bvs) {
    std::vector<std::vector<double>> ret; for (const auto& bv : t_bvs) {
          ret.emplace_back (chaiscript::boxed_cast<std::vector<double>> (bv));
        }
        return ret;
      }));
      */
}

void ChaiInterpreter::AdressFunction(void* function, const QString& name)
{
    //chai.add(chaiscript::fun(function), name.toStdString());
}

double ChaiInterpreter::Input(int i, int j) const
{
    return m_input(j, i);
}

double ChaiInterpreter::Model(int i, int j) const
{
    return m_model(j, i);
}

void ChaiInterpreter::InitialiseChai()
{
    if (m_counter)
        return;

    m_counter++;
    std::vector<double> vector;

    for (int i = 0; i < m_global_names.size(); ++i) {
      m_chaiinterpreter.add_global_const(
          chaiscript::const_var(m_global_parameter(0, i)),
          m_global_names[i].toStdString());
    }

    for (const QString& name : m_input_names)
      m_chaiinterpreter.add_global_const(chaiscript::const_var(vector),
                                         name.toStdString());

    //  for (const QString& string : m_execute)
    //      chai.eval(string.toStdString());
    //chai.eval(m_execute.join("\n").toStdString());
    //m_Calculate = chai.eval<std::function<double(int, int)>>("Calculate");
    //m_Calculate = chai.eval<std::function<std::vector<double>(int)>>("Calculate");
}

void ChaiInterpreter::UpdateChai()
{
    for (int i = 0; i < m_global_names.size(); ++i) {
      m_chaiinterpreter.set_global(
          chaiscript::const_var(m_global_parameter(0, i)),
          m_global_names[i].toStdString());
    }

    for (int j = 0; j < m_input_names.size(); ++j) {
        std::vector<double> vector;
        for (int i = 0; i < m_input.rows(); ++i)
            vector.push_back(m_input.row(i)[j]);
        m_chaiinterpreter.set_global(chaiscript::const_var(vector),
                                     m_input_names[j].toStdString());
    }
}

double ChaiInterpreter::Evaluate(const char *c, int &errorcode) {
  double result = 0.0;
  try {
    auto boxed = m_chaiinterpreter.eval(c);
    result = chaiscript::boxed_cast<double>(boxed);
  } catch (const chaiscript::exception::eval_error &error) {
#ifdef _DEBUG
      qDebug() << "mist" << c;
#endif
      errorcode = 1;
  } catch (const chaiscript::exception::bad_boxed_cast &cast) {
#ifdef _DEBUG
      qDebug() << "another mist";
#endif
  }

  return result;
}

std::vector<double> ChaiInterpreter::EvaluateChaiSeries(int series)
{
    auto lala = m_Calculate(series);
    return lala;
}

double ChaiInterpreter::EvaluateChai(int i, int j)
{
    //chai.add(chaiscript::var(i), "i");
    //chai.add(chaiscript::var(j), "j");
    //return chai.eval<double>("Calculate(i,j)");
    return 0.0; //m_Calculate(i, j);
}
#endif
