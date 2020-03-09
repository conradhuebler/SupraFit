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

#include "src/core/models/AbstractModel.h"
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
#include <iomanip>
#include <iostream>

#include <libpeakpick/mathhelper.h>
#include <libpeakpick/peakpick.h>

#include "pymodel.h"

PyModel::PyModel(DataClass* data)
    : AbstractModel(data)
{
    m_complete = false;
}

PyModel::PyModel(AbstractModel* data)
    : AbstractModel(data)
{
    m_complete = false;
}

PyModel::~PyModel()
{
}

void PyModel::DefineModel(QJsonObject model)
{
    if (model.contains("PyModel"))
        model = model["PyModel"].toObject();
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

    if (model.contains("Execute")) {
        QJsonObject exec = model["Execute"].toObject();
        for (int i = 0; i < exec.size(); ++i)
            m_execute << exec[QString::number(i + 1)].toString();
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
}

void PyModel::InitialGuess_Private()
{

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
    Calculate();
}

void PyModel::CalculateVariables()
{
    /*
    for (int i = 0; i < DataPoints(); ++i) {
        qreal vmax = GlobalParameter(0);
        qreal Km = GlobalParameter(1);
        qreal S_0 = IndependentModel()->data(0, i);
        for (int j = 0; j < SeriesCount(); ++j) {
            qreal value = vmax * S_0 / (Km + S_0);
            SetValue(i, j, value);
        }
    }
    */

    PyModelInterpreter interp;

    interp.setInput(IndependentModel()->Table());
    interp.setModel(ModelTable()->Table());
    interp.setGlobal(GlobalParameter()->Table(), m_global_parameter_names);
    interp.setLocal(LocalParameter()->Table());
    interp.setExecute(m_execute);
    interp.InitialisePython();

    //std::cout << std::setprecision(6) << GlobalParameter(0) << " " << GlobalParameter(1) << std::endl;
    std::cout << std::setprecision(6) << GlobalParameter()->Table() << std::endl;

    for (int i = 0; i < DataPoints(); ++i) {
        qreal vmax = GlobalParameter(0);
        qreal Km = GlobalParameter(1);
        qreal S_0 = IndependentModel()->data(0, i);
        for (int j = 0; j < SeriesCount(); ++j) {
            qreal value2 = vmax * S_0 / (Km + S_0);
            qreal value = interp.EvaulatePython(j, i);
            if (qAbs(value2 - value) > 1e-5) {

                qDebug() << value - value2;
            }
            // SetValue(i, j, value2);
            SetValue(i, j, value2);
        }
    }

    interp.FinalisePython();
}

QSharedPointer<AbstractModel> PyModel::Clone(bool statistics)
{
    QSharedPointer<PyModel> model = QSharedPointer<PyModel>(new PyModel(this), &QObject::deleteLater);
    model.data()->DefineModel(m_model_definition);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "pymodel.moc"
