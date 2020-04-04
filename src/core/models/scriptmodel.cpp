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

#include "src/core/models/AbstractModel.h"
#include "src/core/models/chaiinterpreter.h"
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
        QJsonObject exec = model["ChaiScript"].toObject();
        for (int i = 0; i < exec.size(); ++i)
            m_execute_chai << exec[QString::number(i + 1)].toString();
        m_python = false;
        m_chai = true;
    } else {
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
    m_interp.setDataPoints(DependentModel()->rowCount());
    m_interp.setSeriesCount(DependentModel()->columnCount());
    m_interp.setInputNames(m_input_names);
    m_interp.setExecute(m_execute_chai);
    m_interp.InitialiseChai();
#endif
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
    if (m_python)
        CalculatePython();
    else if (m_chai)
        CalculateChai();
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

    auto matrix = m_interp.Evaluate();
    for (int series = 0; series < SeriesCount(); ++series) {
        //std::vector<double> row = m_interp.EvaluateChaiSeries(series);
        for (int i = 0; i < DataPoints(); ++i) {
            //SetValue(i, series, row[i]);
            SetValue(i, series, matrix[series][i]);
        }
    }
    /*
    for (int i = 0; i < DataPoints(); ++i) {
        for (int j = 0; j < SeriesCount(); ++j) {
            SetValue(i, j, m_interp.EvaluateChai(j, i));
        }
    }*/
#else
    emit Info()->Warning(QString("It looks like you open a Scripted Model. Ok, unfortranately SupraFit was compiled without Chai Script Support."));
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
