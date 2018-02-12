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

#include "src/core/models.h"
#include "src/core/libmath.h"

#include <Eigen/Dense>

#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>
#include <QtCore/QThreadPool>
#include <QtCore/QRunnable>
#include <QtMath>
#include <QtCore/QJsonObject>
#include <QDebug>
#include <cmath>

#include "2_1_1_1_1_2_Model.h"

ConSolver::ConSolver(QPointer<AbstractTitrationModel> model) :m_model(model)
{
    setAutoDelete(false);
}

ConSolver::~ConSolver()
{

}

void ConSolver::setInput(double A_0, double B_0)
{
    m_A_0 = A_0;
    m_B_0 = B_0;
}

void ConSolver::run()
{
    m_concentration = HostConcentration(m_A_0, m_B_0);
}


QPair<double, double> ConSolver::HostConcentration(double a0, double b0)
{
    if(!a0 || !b0)
        return QPair<double, double>(a0,b0);

    qreal K21 = qPow(10, m_model.data()->GlobalParameter().first());
    qreal K11 = qPow(10, m_model.data()->GlobalParameter()[1]);
    qreal K12 = qPow(10, m_model.data()->GlobalParameter().last());
    qreal b12 = K11*K12;
    qreal b21 = K11*K21;

    auto calc_a = [](double a0, double b, double K11, double b21, double b12){
        double x1 = 2*b21*b;
        double x2 = b12*b*b+K11*b+1;
        double x3 = -a0;
        return MaxQuadraticRoot(x1,x2,x3);
    };

    auto calc_b = [](double b0, double a, double K11, double b21, double b12){
        double x1 = 2*b12*a;
        double x2 = b21*a*a+K11*a+1;
        double x3 = -b0;
        return MaxQuadraticRoot(x1,x2,x3);
    };

    qreal a = qMin(a0,b0)/2;
    qreal b = 0;
    qreal a_1 = 0, b_1 = 0;
    for(int i = 0; i < 50; ++i)
    {
        a_1 = a;
        b_1 = b;
        b = calc_b(b0, a, K11, b21, b12);
        if(b < 0)
            b *= -1;

        a = calc_a(a0, b, K11, b21, b12);
        if(a < 0)
            a *= -1;

        if(qAbs(a_1-a) < 1e-10 || qAbs(b_1-b) < 1e-10)
            break;
    }
    return QPair<double, double>(a,b);
}

IItoI_ItoI_ItoII_Model::IItoI_ItoI_ItoII_Model(DataClass* data): AbstractTitrationModel(data)
{
    m_threadpool = new QThreadPool(this);
    for(int i = 0; i < DataPoints(); ++i)
        m_solvers << new ConSolver(this);

    m_local_parameter = new DataTable(4, SeriesCount(), this);
    m_global_parameter << 1 << 1 << 1;
    addGlobalParameter(m_global_parameter);
    DeclareOptions();
}

IItoI_ItoI_ItoII_Model::~IItoI_ItoI_ItoII_Model()
{
    qDeleteAll(m_solvers);
}

void IItoI_ItoI_ItoII_Model::DeclareOptions()
{
    QStringList method = QStringList() << "NMR" << "UV/VIS";
    addOption("Method", method);
    //QStringList cooperativity = QStringList() << "full" << "noncooperative" << "additive" << "statistical";
    //addOption("Cooperativity", cooperativity);
}

void IItoI_ItoI_ItoII_Model::InitialGuess()
{
    m_K11 = Guess_1_1();
    m_global_parameter = QList<qreal>() << m_K11/4 << m_K11 << m_K11/4;

    qreal factor = 1;
    
    if(getOption("Method") == "UV/VIS")
    {
        factor = 1/InitialHostConcentration(0);
    }

    m_local_parameter->setColumn(DependentModel()->firstRow()*factor, 0);
    m_local_parameter->setColumn(DependentModel()->firstRow()*factor, 1);
    m_local_parameter->setColumn(DependentModel()->lastRow()*factor, 2);
    m_local_parameter->setColumn(DependentModel()->lastRow()*factor, 3);
    
    Calculate();
}

void IItoI_ItoI_ItoII_Model::CalculateVariables()
{
    m_corrupt = false;
    QString method = getOption("Method");
    m_sum_absolute = 0;
    m_sum_squares = 0;

    qreal K21= qPow(10, GlobalParameter().first());
    qreal K11 =qPow(10, GlobalParameter()[1]);
    qreal K12= qPow(10, GlobalParameter().last());
    m_constants_pow = QList<qreal >() << K21 << K11 << K12;


    int maxthreads =qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        
        m_solvers[i]->setInput(host_0, guest_0);
        m_threadpool->start(m_solvers[i]);
    }

    m_threadpool->waitForDone();

    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);

        QPair<double, double > concentration = m_solvers[i]->Concentrations();

        qreal host = concentration.first;
        qreal guest = concentration.second;
        
        qreal complex_11 = K11*host*guest;
        qreal complex_21 = K11*K21*host*host*guest;
        qreal complex_12 = K11*K12*host*guest*guest;

        Vector vector(6);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        vector(3) = complex_21;
        vector(4) = complex_11;
        vector(5) = complex_12;
        SetConcentration(i, vector);
        qreal value = 0;
        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(method == "NMR")
                value = host/host_0*m_local_parameter->data(0, j) + 2*complex_21/host_0*m_local_parameter->data(1, j) + complex_11/host_0*m_local_parameter->data(2, j) + complex_12/host_0*m_local_parameter->data(3, j);
            else if(method == "UV/VIS")
                value = host*m_local_parameter->data(0, j) + 2*complex_21*m_local_parameter->data(1, j) + complex_11*m_local_parameter->data(2, j) + complex_12*m_local_parameter->data(3, j);

            SetValue(i, j, value);
        }
    }
    
    emit Recalculated();
}

QSharedPointer<AbstractModel> IItoI_ItoI_ItoII_Model::Clone()
{
    QSharedPointer<IItoI_ItoI_ItoII_Model > model = QSharedPointer<IItoI_ItoI_ItoII_Model>(new IItoI_ItoI_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

QVector<qreal> IItoI_ItoI_ItoII_Model::OptimizeParameters_Private(OptimizationType type)
{
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
    {
        addGlobalParameter(m_global_parameter);
    }
    
     if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
        
        if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
            addLocalParameter(0);
        addLocalParameter(1);
        addLocalParameter(2);
        addLocalParameter(3);
    }
    
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


MassResults IItoI_ItoI_ItoII_Model::MassBalance(qreal A, qreal B)
{
    //     QMutexLocker (&mutex);
    MassResults result;
    qreal K11 = m_constants_pow[0];
    qreal K21 = m_constants_pow[1];
    qreal K12 = m_constants_pow[2];
    
    Vector values(2);
    
    qreal complex_21 = K11*K21*A*A*B;
    qreal complex_11 = K11*A*B;
    qreal complex_12 = K11*K12*A*B*B;
    
    values(0) = (2*complex_21 + complex_11 + complex_12) ;
    values(1) = (complex_21 + complex_11 + 2*complex_12) ;
    result.MassBalance = values;
    return result;
}

