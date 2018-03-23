/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/models.h"
#include "src/core/toolset.h"

#include <Eigen/Dense>

#include <QDebug>
#include <QtMath>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>

#include <cmath>
#include <functional>

#include "fl_2_1_1_1_1_2_Model.h"

fl_IItoI_ItoI_ItoII_Model::fl_IItoI_ItoI_ItoII_Model(DataClass* data): AbstractTitrationModel(data)
{
    m_threadpool = new QThreadPool(this);
    for(int i = 0; i < DataPoints(); ++i)
        m_solvers << new IItoI_ItoI_ItoII_Solver();

    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

fl_IItoI_ItoI_ItoII_Model::~fl_IItoI_ItoI_ItoII_Model()
{
    qDeleteAll(m_solvers);
}

void fl_IItoI_ItoI_ItoII_Model::DeclareOptions()
{
    QStringList cooperativity = QStringList() << "full" << "noncooperative" << "additive" << "statistical";
    addOption(Cooperativity2_1, "Cooperativity 2:1", cooperativity);
    cooperativity = QStringList() << "full" << "noncooperative" << "additive" << "statistical";
    addOption(Cooperativity1_2, "Cooperativity 1:2", cooperativity);
}

void fl_IItoI_ItoI_ItoII_Model::EvaluateOptions()
{
    QString cooperativitiy = getOption(Cooperativity2_1);
    {
        auto global_coop = [this](){
            this->m_global_parameter[0] = log10(double(0.25)*qPow(10,this->m_global_parameter[1]));
        };

        auto local_coop = [this]()
        {
            for(int i = 0; i < this->SeriesCount(); ++i)
                this->m_local_parameter->data(1,i) = 2*(this->m_local_parameter->data(2,i)-this->m_local_parameter->data(0,i))+this->m_local_parameter->data(0,i);
        };

        if(cooperativitiy == "noncooperative")
        {
            global_coop();
        }else if(cooperativitiy == "additive")
        {
            local_coop();
        }else if(cooperativitiy == "statistical")
        {
            local_coop();
            global_coop();
        }
    }

    cooperativitiy = getOption(Cooperativity1_2);
    {
         auto global_coop = [this](){
                this->m_global_parameter[2] = log10(double(0.25)*qPow(10,this->m_global_parameter[1]));
         };

         auto local_coop = [this]()
         {
                for(int i = 0; i < this->SeriesCount(); ++i)
                    this->m_local_parameter->data(3,i) = 2*(this->m_local_parameter->data(2,i)-this->m_local_parameter->data(0,i))+this->m_local_parameter->data(0,i);
         };

        if(cooperativitiy == "noncooperative")
        {
            global_coop();
        }else if(cooperativitiy == "additive")
        {
            local_coop();
        }else if(cooperativitiy == "statistical")
        {
            local_coop();
            global_coop();
        }
    }

}

void fl_IItoI_ItoI_ItoII_Model::InitialGuess()
{
    qreal K11 = Guess_1_1();
    m_global_parameter = QList<qreal>() << 2 << K11 << 2;

    qreal factor = 1;

    m_local_parameter->setColumn(DependentModel()->firstRow()*factor, 0);
    m_local_parameter->setColumn(DependentModel()->firstRow()*factor, 1);
    m_local_parameter->setColumn(DependentModel()->lastRow()*factor, 2);
    m_local_parameter->setColumn(DependentModel()->lastRow()*factor, 3);
    m_local_parameter->setColumn(DependentModel()->lastRow()*factor, 4);

    Calculate();
}

void fl_IItoI_ItoI_ItoII_Model::CalculateVariables()
{
    m_sum_absolute = 0;
    m_sum_squares = 0;

    qreal K21= qPow(10, GlobalParameter().first());
    qreal K11 =qPow(10, GlobalParameter()[1]);
    qreal K12= qPow(10, GlobalParameter().last());
    m_constants_pow = QList<qreal >() << K21 << K11 << K12;

    QVector<qreal > F0(SeriesCount());

    int maxthreads =qApp->instance()->property("threads").toInt();
    m_threadpool->setMaxThreadCount(maxthreads);
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        
        m_solvers[i]->setInput(host_0, guest_0);
        m_solvers[i]->setConfig(m_opt_config);
        m_solvers[i]->setConstants(m_constants_pow);
        m_threadpool->start(m_solvers[i]);
    }

    m_threadpool->waitForDone();

    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0 = InitialHostConcentration(i);
        if(!m_solvers[i]->Ok())
        {
#ifdef _DEBUG
            qDebug() << "Numeric didn't work out well, mark model as corrupt! - Dont panic. Not everything is lost ...";
            qDebug() << m_solvers[i]->Ok() << InitialHostConcentration(i) << InitialGuestConcentration(i);
#endif
            m_corrupt = true;
            if(m_opt_config.skip_not_converged_concentrations)
            {
#ifdef _DEBUG
            qDebug() << "Ok, I skip the current result ...";
#endif
                continue;
            }
        }
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

        if(!m_fast)
            SetConcentration(i, vector);

        qreal value = 0;
        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(i == 0)
            {
                F0[j] = host_0*m_local_parameter->data(0, j);
                value = F0[j];
            }else
                value = host*m_local_parameter->data(0, j) + 2*complex_21*m_local_parameter->data(1, j) + complex_11*m_local_parameter->data(2, j) + complex_12*m_local_parameter->data(3, j);

            SetValue(i, j, value*1e3);
        }
    }
}

QSharedPointer<AbstractModel> fl_IItoI_ItoI_ItoII_Model::Clone()
{
    QSharedPointer<fl_IItoI_ItoI_ItoII_Model > model = QSharedPointer<fl_IItoI_ItoI_ItoII_Model>(new fl_IItoI_ItoI_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

QVector<qreal> fl_IItoI_ItoI_ItoII_Model::OptimizeParameters_Private(OptimizationType type)
{
    if((OptimizationType::GlobalParameter & type) == OptimizationType::GlobalParameter)
    {
        addGlobalParameter(m_global_parameter);
    }
    
     if((type & OptimizationType::LocalParameter) == (OptimizationType::LocalParameter))
    {
        
        //if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
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


MassResults fl_IItoI_ItoI_ItoII_Model::MassBalance(qreal A, qreal B)
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

qreal fl_IItoI_ItoI_ItoII_Model::Y(qreal x, const QVector<qreal> &parameter)
{
    if(3 != parameter.size())
        return 0;
    qreal b21 = parameter[0];
    qreal b11 = parameter[1];
    qreal b12 = parameter[2];

    qreal epsilon = 1e-12;

    auto calc_a = [](double b, double b11, double b21, double b12){
        double x1 = b21;
        double x2 = 2*b12*b+b11;
        double x3 = -1;
        return MaxQuadraticRoot(x1,x2,x3);
    };

    auto calc_b = [](double a, double b11, double b21, double b12, double x){
        double x1 = b12;
        double x2 = 2*b21*a+b11;
        double x3 = -x;
        return MaxQuadraticRoot(x1,x2,x3);
    };

    qreal A = x/2;
    qreal B = 0;
    qreal a_1 = 0, b_1 = 0;
    int i;
    for(i = 0; i < 150; ++i)
    {
        a_1 = A;
        b_1 = B;
        B = calc_b(A, b11, b21, b12, x);
        if(B < 0)
            B *= -1;

        A = calc_a(B, b11, b21, b12);
        if(A < 0)
            A *= -1;

        if(qAbs(b21*a_1*a_1*b_1-b21*A*A*B) < epsilon && qAbs(b12*a_1*b_1*b_1-b12*A*B*B) < epsilon && qAbs(b11*a_1*b_1 - b11 * A*B) < epsilon)
            break;
    }
#ifdef _DEBUG
    std::cout << a_1 << " "<< b_1 << " " << b11*a_1*b_1 << " " << b21*a_1*a_1*b_1 << " " << b12*a_1*b_1*b_1 << std::endl;
    std::cout << A << " "<< B << " " << b11*A*B << " " << b21*A*A*B << " " << b12*A*B*B << std::endl;
    std::cout << "last Change: " << qAbs(b21*a_1*a_1*b_1-b21*A*A*B) << " " << qAbs(b12*a_1*b_1*b_1-b12*A*B*B) << " " << qAbs(b11*a_1*b_1 * b11 * A*B)  << std::endl;
    std::cout << "Guess A: " << x/2 << " .. Final A: " << A << " .. Iterations:" << i<< std::endl;
#endif
    return 1./(A + b11*A*B + b12*A*B*B + 2*b21*A*A*B);
}


qreal fl_IItoI_ItoI_ItoII_Model::BC50() const
{
    qreal b21 = qPow(10,GlobalParameter(0)+GlobalParameter(1));
    qreal b11 = qPow(10,GlobalParameter(1));
    qreal b12 = qPow(10,GlobalParameter(1)+GlobalParameter(2));

    QVector<qreal> parameter;
    parameter << b21 << b11 << b12;
    std::function<qreal(qreal, const QVector<qreal> &)> function = Y;
    qreal integ = ToolSet::SimpsonIntegrate(0, 1, function, parameter);
    return double(1)/double(2)/integ;
}

qreal fl_IItoI_ItoI_ItoII_Model::Y_0(qreal x, const QVector<qreal> &parameter)
{
    if(3 != parameter.size())
        return 0;
    qreal b21 = parameter[0];
    qreal b11 = parameter[1];
    qreal b12 = parameter[2];

    qreal epsilon = 1e-12;

    auto calc_a = [](double b, double b11, double b21, double b12){
        double x1 = b21;
        double x2 = 2*b12*b+b11;
        double x3 = -1;
        return MaxQuadraticRoot(x1,x2,x3);
    };

    auto calc_b = [](double a, double b11, double b21, double b12, double x){
        double x1 = b12;
        double x2 = 2*b21*a+b11;
        double x3 = -x;
        return MaxQuadraticRoot(x1,x2,x3);
    };

    qreal A = x/2;
    qreal B = 0;
    qreal a_1 = 0, b_1 = 0;
    int i;
    for(i = 0; i < 150; ++i)
    {
        a_1 = A;
        b_1 = B;
        B = calc_b(A, b11, b21, b12, x);
        if(B < 0)
            B *= -1;

        A = calc_a(B, b11, b21, b12);
        if(A < 0)
            A *= -1;

        if(qAbs(b21*a_1*a_1*b_1-b21*A*A*B) < epsilon && qAbs(b12*a_1*b_1*b_1-b12*A*B*B) < epsilon && qAbs(b11*a_1*b_1 - b11 * A*B) < epsilon)
            break;
    }
#ifdef _DEBUG
    std::cout << a_1 << " "<< b_1 << " " << b11*a_1*b_1 << " " << b21*a_1*a_1*b_1 << " " << b12*a_1*b_1*b_1 << std::endl;
    std::cout << A << " "<< B << " " << b11*A*B << " " << b21*A*A*B << " " << b12*A*B*B << std::endl;
    std::cout << "last Change: " << qAbs(b21*a_1*a_1*b_1-b21*A*A*B) << " " << qAbs(b12*a_1*b_1*b_1-b12*A*B*B) << " " << qAbs(b11*a_1*b_1 - b11 * A*B)  << std::endl;
    std::cout << "Guess A: " << x/2 << " .. Final A: " << A << " .. Iterations:" << i<< std::endl;
#endif
    return A;
}


qreal fl_IItoI_ItoI_ItoII_Model::BC50SF() const
{
    qreal b21 = qPow(10,GlobalParameter(0)+GlobalParameter(1));
    qreal b11 = qPow(10,GlobalParameter(1));
    qreal b12 = qPow(10,GlobalParameter(1)+GlobalParameter(2));

    QVector<qreal> parameter;
    parameter << b21 << b11 << b12;
    std::function<qreal(qreal, const QVector<qreal> &)> function = Y_0;
    qreal integ = ToolSet::SimpsonIntegrate(0, 1, function, parameter);
    return integ;
}
