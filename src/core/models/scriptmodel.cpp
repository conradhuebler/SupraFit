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

#include "src/core/models/AbstractModel.h"
#include "src/core/models/chaiinterpreter.h"
#include "src/core/models/dukmodelinterpreter.h"
#include "src/core/models/pymodelinterpreter.h"

#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include <cmath>
#include <iostream>

#include <libpeakpick/mathhelper.h>
#include <libpeakpick/peakpick.h>

#include "scriptmodel.h"

ScriptModel::ScriptModel(DataClass* data)
    : AbstractModel(data)
{
    m_complete = false;
}

ScriptModel::ScriptModel(AbstractModel* data)
    : AbstractModel(data)
{
    m_complete = false;
}

ScriptModel::~ScriptModel()
{
}

void ScriptModel::DefineModel(QJsonObject model)
{
    if (model.contains("ScriptModel"))
        model = model["ScriptModel"].toObject();
    if (model.contains("GlobalParameterSize"))
        m_global_parameter_size = model["GlobalParameterSize"].toInt();
    else
        return;

    if (model.contains("GlobalParameterNames"))
        m_global_parameter_names = model["GlobalParameterNames"].toString().split("|");

    if (model.contains("LocalParameterSize"))
        m_local_parameter_size = model["LocalParameterSize"].toInt();
    else
        return;

    if (model.contains("LocalParameterNames"))
        m_local_parameter_names = model["LocalParameterNames"].toString().split("|");

    if (model.contains("InputSize"))
        m_input_size = model["InputSize"].toInt();
    else
        return;

    if (model.contains("Python")) {
        QJsonObject exec = model["Python"].toObject();
        for (int i = 0; i < exec.size(); ++i)
            m_execute_python << exec[QString::number(i + 1)].toString();
        m_python = true;
    }
    if (model.contains("ChaiScript")) {
      // m_execute_chai.clear();
      QStringList strings;
      QJsonObject exec = model["ChaiScript"].toObject();
      for (const QString &key : exec.keys())
        // for (int i = 0; i < exec.size(); ++i)
        strings << exec[key].toString();
      m_chai_execute = strings.join("\n");
      m_python = false;
      m_chai = true;
    }

    if (model.contains("Duktape")) {
        QJsonObject exec = model["Duktape"].toObject();
        for (int i = 0; i < exec.size(); ++i)
            m_execute_duktape << exec[QString::number(i + 1)].toString();
        m_python = false;
        m_chai = false;
        m_duktape = true;
    }
    if (!m_python && !m_chai && !m_duktape) {
        emit Message("Nothing to do, lets just start it.", 1);
        return;
    }

    m_name_cached = model["Name"].toString();
    m_name = model["Name"].toString();

    m_input_names = model["InputNames"].toString().split("|");
    m_depmodel_names = model["DepModelNames"].toString().split("|");

    m_model_definition = model;
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());

    for (int i = 0; i < m_input_names.size(); ++i)
        IndependentModel()->setHeaderData(i, Qt::Horizontal, m_input_names[i], Qt::DisplayRole);

    for (int i = 0; i < m_depmodel_names.size(); ++i)
        DependentModel()->setHeaderData(i, Qt::Horizontal, m_depmodel_names[i], Qt::DisplayRole);
#ifdef _Models
    m_interp.setInput(IndependentModel()->Table());
    m_interp.setGlobal(GlobalParameter()->Table(), m_global_parameter_names);
    m_interp.setLocal(LocalParameter()->Table());
    m_interp.setInputNames(m_input_names);
    m_interp.setExecute(m_execute_chai);
    m_interp.InitialiseChai();
#endif

#ifdef Use_Duktape
    m_duktapeinterp.Initialise();
#endif
}

void ScriptModel::UpdateExecute(const QString &execute) {
  m_chai_execute = execute;
  QJsonObject json;
  QStringList lines = m_chai_execute.split("\n");
  for (int i = 0; i < lines.size(); ++i)
    json[QString::number(i)] = lines[i];
  m_model_definition["ChaiScript"] = json;
}

void ScriptModel::InitialGuess_Private()
{
    InitialiseRandom();
    /*
    QVector<qreal> x, y;

    for (int i = 1; i < DataPoints(); ++i) {
        x << 1 / IndependentModel()->data(0, i);
        y << 1 / DependentModel()->data(0, i);
    }

    PeakPick::LinearRegression regress = LeastSquares(x, y);
    double m_vmax = 1 / regress.n;
    double m_Km = regress.m * m_vmax;
    (*GlobalTable())[0] = m_vmax;
    (*GlobalTable())[1] = m_Km;
    Calculate();*/
}

void ScriptModel::CalculateVariables()
{
    //if (m_python)
    //    CalculatePython();
    //else if (m_chai)
    CalculateChai();
    // else if(m_duktape)
    //CalculateDuktape();
}

void ScriptModel::CalculatePython()
{
#ifdef _Python
    PyModelInterpreter interp;

    interp.setInput(IndependentModel()->Table());
    interp.setModel(ModelTable()->Table());
    interp.setGlobal(GlobalParameter()->Table(), m_global_parameter_names);
    interp.setLocal(LocalParameter()->Table());
    interp.setExecute(m_execute_python);
    interp.InitialisePython();

    for (int i = 0; i < DataPoints(); ++i) {
        for (int j = 0; j < SeriesCount(); ++j) {
            SetValue(i, j, interp.EvaluatePython(j, i));
        }
    }

    interp.FinalisePython();
#else
    emit Info()->Warning(QString("It looks like you open a Scripted Model. Ok, unfortunately SupraFit was compiled without Pyhton Script Support. Beside, the Python Models are not working yet."));
    m_complete = false;
#endif
}

void ScriptModel::CalculateChai()
{
#ifdef _Models
    m_interp.setGlobal(GlobalParameter()->Table(), m_global_parameter_names);
    m_interp.setLocal(LocalParameter()->Table());
    m_interp.UpdateChai();
    // QString execute = m_execute_chai.join("\n");
    for (int series = 0; series < SeriesCount(); ++series) {
        for (int i = 0; i < DataPoints(); ++i) {
          QString cache = m_chai_execute;
          for (int parameter = 0; parameter < InputParameterSize();
               ++parameter) {
            cache.replace(
                m_input_names[parameter],
                QString::number((IndependentModel()->data(parameter, i))));
          }
          int error = 0;
          double result = m_interp.Evaluate(cache.toUtf8(), error);
          if (error == 1) {
            cache.replace("var", "");
            result = m_interp.Evaluate(cache.toUtf8(), error);
          }
          SetValue(i, series, result);
        }
    }
#else
    emit Info()->Warning(QString("It looks like you open a Scripted Model. Ok, unfortranately SupraFit was compiled without Chai Script Support."));
    m_complete = false;
#endif
}

void ScriptModel::CalculateDuktape()
{

#ifdef Use_Duktape
QString execute = m_execute_chai.join("\n");
    std::vector<std::string> list;
    for (const QString& str : m_global_parameter_names)
        list.push_back(str.toStdString());

    m_duktapeinterp.setGlobal(GlobalParameter()->Table(), list);
    m_duktapeinterp.Update();
    for (int series = 0; series < SeriesCount(); ++series) {
        for (int i = 0; i < DataPoints(); ++i) {
            //QString calculate = QString("%1*%2/(%3+%2)").arg(km).arg(IndependentModel()->data(0, i)).arg(vmax);
            QString calculate = QString("vmax*%1/(Km+%1)").arg(IndependentModel()->data(0, i));

            // QString calculate = QString("%1*%3/(%2+%3)").arg(GlobalParameter()->Table()(0,0)).arg(GlobalParameter()->Table()(0,1)).arg(IndependentModel()->data(0, i));
            // qDebug() << calculate;
            QString t = m_execute_chai[0];
            QString cache = execute;
            cache.replace("S", QString::number((IndependentModel()->data(0, i))));
            QString result(m_duktapeinterp.Evaluate(cache.toUtf8()));
            //QString result(m_duktapeinterp.Evaluate(calculate.toUtf8()));
            double val = result.toDouble();
            SetValue(i, series, val); //row[i]);
        }
    }
    // m_duktapeinterp.Finalise();

#else
    emit Info()->Warning(QString("It looks like you open a Scripted Model. Ok, unfortranately SupraFit was compiled without Duktape Script Support."));
    m_complete = false;
#endif
}

QSharedPointer<AbstractModel> ScriptModel::Clone(bool statistics)
{
    QSharedPointer<ScriptModel> model = QSharedPointer<ScriptModel>(new ScriptModel(this), &QObject::deleteLater);
    model.data()->DefineModel(m_model_definition);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "scriptmodel.moc"
