/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/AbstractItcModel.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QThreadPool>

#include <cfloat>
#include <cmath>
#include <iostream>

#include "itc_2_2_Model.h"

itc_IItoII_Model::itc_IItoII_Model(DataClass* data)
    : AbstractItcModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());

    m_threadpool = new QThreadPool(this);
    for (int i = 0; i < DataPoints(); ++i)
        m_solvers << new IItoI_ItoI_ItoII_Solver();
}

itc_IItoII_Model::itc_IItoII_Model(AbstractItcModel* model)
    : AbstractItcModel(model)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    m_threadpool = new QThreadPool(this);
    for (int i = 0; i < DataPoints(); ++i)
        m_solvers << new IItoI_ItoI_ItoII_Solver();
}

itc_IItoII_Model::~itc_IItoII_Model()
{
    for (int i = 0; i < m_solvers.size(); ++i)
        if (m_solvers[i])
            delete m_solvers[i];
}

void itc_IItoII_Model::InitialGuess()
{
    (*GlobalTable())[0] = 6;
    (*GlobalTable())[1] = 6;
    (*GlobalTable())[2] = 6;

    LocalTable()->data(0, 0) = -4000;
    LocalTable()->data(1, 0) = -4000;
    LocalTable()->data(2, 0) = -4000;
    LocalTable()->data(3, 0) = -1000;
    LocalTable()->data(4, 0) = 1;
    LocalTable()->data(5, 0) = 1;

    AbstractModel::Calculate();
}

QVector<qreal> itc_IItoII_Model::OptimizeParameters_Private(OptimizationType type)
{
    if ((OptimizationType::GlobalParameter & type) == OptimizationType::GlobalParameter) {
        addGlobalParameter(0);
        addGlobalParameter(1);
        addGlobalParameter(2);
    }
    if ((OptimizationType::LocalParameter & type) == OptimizationType::LocalParameter) {
        addLocalParameter(0);
        addLocalParameter(1);
        addLocalParameter(2);

        QString dilution = getOption(Dilution);

        if (dilution == "auto") {
            addLocalParameter(3);
            addLocalParameter(4);
        }
        addLocalParameter(5);
    }
    QVector<qreal> parameter;
    for (int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void itc_IItoII_Model::CalculateVariables()
{
    if (!m_threadpool)
        return;

    m_sum_absolute = 0;
    m_sum_squares = 0;

    QString dil = getOption(Dilution);

    qreal dH21 = LocalTable()->data(0, 0);
    qreal dH11 = LocalTable()->data(1, 0);
    qreal dH12 = LocalTable()->data(2, 0);
    qreal dil_heat = LocalTable()->data(3, 0);
    qreal dil_inter = LocalTable()->data(4, 0);
    qreal fx = LocalTable()->data(5, 0);
    qreal V = m_V;

    qreal K21 = qPow(10, GlobalParameter(0));
    qreal K11 = qPow(10, GlobalParameter(1));
    qreal K12 = qPow(10, GlobalParameter(2));

    qreal complex_21_prev = 0, complex_11_prev = 0, complex_12_prev = 0;

    QList<qreal> constants_pow;
    constants_pow << K21 << K11 << K12;

    int maxthreads = qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        host_0 *= fx;

        qreal guest_0 = InitialGuestConcentration(i);
        m_solvers[i]->setInput(host_0, guest_0);
        m_solvers[i]->setConfig(m_opt_config);
        m_solvers[i]->setConstants(constants_pow);
        //  if(QThreadPool::globalInstance()->activeThreadCount())
        //      m_solvers[i]->run();
        // else
        m_threadpool->start(m_solvers[i]);
    }
    m_threadpool->waitForDone();

    for (int i = 0; i < DataPoints(); ++i) {
        if (!m_solvers[i]->Ok()) {
#ifdef _DEBUG
            qDebug() << "Numeric didn't work out well, mark model as corrupt! - Dont panic. Not everything is lost ...";
            qDebug() << m_solvers[i]->Ok() << InitialHostConcentration(i) << InitialGuestConcentration(i);
#endif
            m_corrupt = true;
            if (m_opt_config.skip_not_converged_concentrations) {
#ifdef _DEBUG
                qDebug() << "Ok, I skip the current result ...";
#endif
                continue;
            }
        }

        qreal guest_0 = InitialGuestConcentration(i);
        qreal dilution = 0;
        if (dil == "auto") {
            dilution = (guest_0 * dil_heat + dil_inter);
        }

        QPair<double, double> concentration = m_solvers[i]->Concentrations();

        qreal host = concentration.first;
        qreal guest = concentration.second;

        qreal complex_11 = K11 * host * guest;
        qreal complex_21 = K11 * K21 * host * host * guest;
        qreal complex_12 = K11 * K12 * host * guest * guest;

        Vector vector(6);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        vector(3) = complex_21;
        vector(4) = complex_11;
        vector(5) = complex_12;

        qreal v = IndependentModel()->data(0, i);

        if (!m_fast)
            SetConcentration(i, vector);

        qreal value = V * ((complex_21 - complex_21_prev * (1 - v / V)) * dH21 + ((complex_11 - complex_11_prev * (1 - v / V)) * dH11) + ((complex_12 - complex_12_prev * (1 - v / V)) * dH12));

        SetValue(i, 0, value + dilution);
        complex_21_prev = complex_21;
        complex_11_prev = complex_11;
        complex_12_prev = complex_12;
    }
}

QSharedPointer<AbstractModel> itc_IItoII_Model::Clone()
{
    QSharedPointer<AbstractItcModel> model = QSharedPointer<itc_IItoII_Model>(new itc_IItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    model.data()->setConcentrations(ConcentrationTable());
    return model;
}

#include "itc_2_2_Model.moc"
