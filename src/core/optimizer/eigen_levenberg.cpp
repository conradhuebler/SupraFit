/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/AbstractModel.h"

#include <QtCore/QtGlobal>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/NonLinearOptimization>

#include <iostream>

#include "src/core/libmath.h"
typedef QList<qreal > Variables;

template<typename _Scalar, int NX = Eigen::Dynamic, int NY = Eigen::Dynamic>

struct Functor
{
    typedef _Scalar Scalar;
    enum {
        InputsAtCompileTime = NX,
        ValuesAtCompileTime = NY
    };
    typedef Eigen::Matrix<Scalar,InputsAtCompileTime,1> InputType;
    typedef Eigen::Matrix<Scalar,ValuesAtCompileTime,1> ValueType;
    typedef Eigen::Matrix<Scalar,ValuesAtCompileTime,InputsAtCompileTime> JacobianType;
    
    int m_inputs, m_values;
    
    inline Functor(int inputs, int values) : m_inputs(inputs), m_values(values) {}
    
    int inputs() const { return m_inputs; }
    int values() const { return m_values; }
    
};

struct MyFunctor : Functor<double>
{
    inline MyFunctor(int inputs, int values) : Functor(inputs, values), no_parameter(inputs),  no_points(values) 
    {
        
    }
    inline ~MyFunctor() { }
    inline int operator()(const Eigen::VectorXd &parameter, Eigen::VectorXd &fvec) const
    {
        QVector<qreal > param(inputs());
        for(int i = 0; i < inputs(); ++i)
            param[i] = parameter(i);
        
        model.data()->setParamter(param);
        model.data()->Calculate();
        Variables CalculatedSignals = model.data()->getCalculatedSignals();
        for( int i = 0; i < ModelSignals.size(); ++i)
        {
            if(m_potenz == 2)
                fvec(i) = CalculatedSignals[i] - ModelSignals[i];
            else if(m_potenz == 1)
                fvec(i) = qSqrt(qAbs((CalculatedSignals[i] - ModelSignals[i])));
        }
        return 0;
    }
    int no_parameter;
    int no_points;
    int m_potenz;
    Variables ModelSignals;
    QSharedPointer<AbstractTitrationModel> model;
    int inputs() const { return no_parameter; } // There are two parameters of the model
    int values() const { return no_points; } // The number of observations
};

struct MyFunctorNumericalDiff : Eigen::NumericalDiff<MyFunctor> {};

qreal Norm(const QList<qreal> v1, const QList<qreal> v2)
{
    qDebug() << v1 << v2;
    qreal norm = 0;
    if(v1.size() != v2.size())
        return norm;
    for(int i = 0; i < v1.size(); ++i)
        norm += qAbs(v1[i]-v2[i]);
    return norm;
}

int NonlinearFit(QWeakPointer<AbstractTitrationModel> model, QVector<qreal > &param)
{
    
    
    OptimizerConfig config = model.data()->getOptimizerConfig();
    Variables ModelSignals = model.data()->getSignals(model.data()->ActiveSignals());
    if(ModelSignals.size() == 0)
        return -1;
    Eigen::VectorXd parameter(param.size());
    for(int i = 0; i < param.size(); ++i)
        parameter(i) = param[i];
    
    QString message = QString();
    message += "Starting Levenberg-Marquardt for " + QString::number(parameter.size()) + " parameters:\n";
    message += "Old vector : ";
    for(double d: param)
    {
        message += QString::number(d) + " ";
    }
    message += "\n";
    model.data()->Message(message, 5);
    MyFunctor functor(param.size(), ModelSignals.size());
    functor.model = model;
    functor.ModelSignals = ModelSignals;
    functor.m_potenz = config.error_potenz;
    Eigen::NumericalDiff<MyFunctor> numDiff(functor);
    Eigen::LevenbergMarquardt<Eigen::NumericalDiff<MyFunctor> > lm(numDiff);
    int iter = 0;
    lm.parameters.factor = config.LevMar_Factor; //step bound for the diagonal shift, is this related to damping parameter, lambda?
    lm.parameters.maxfev = config.MaxIter;//max number of function evaluations
    lm.parameters.xtol = config.LevMar_Xtol; //tolerance for the norm of the solution vector
    lm.parameters.ftol = config.LevMar_Ftol; //tolerance for the norm of the vector function
    lm.parameters.gtol = config.LevMar_Gtol; // tolerance for the norm of the gradient of the error vector
    lm.parameters.epsfcn = config.LevMar_epsfcn; //error precision
    Eigen::LevenbergMarquardtSpace::Status status = lm.minimizeInit(parameter);
    qreal error_0 = 0; 
    qreal error_2 = 1;
    qreal norm = 1;
    QList<qreal> globalConstants;
    for (;iter < config.MaxIter && ((qAbs(error_0 -error_2) > config.Error_Convergence ) || norm > config.Constant_Convergence);++iter)
    {
        globalConstants.clear();
        for(int i = 0; i < model.data()->Constants().size(); ++i)
            globalConstants << model.data()->Constants()[i];
        error_0 = model.data()->SumofSquares();
        
        status = lm.minimizeOneStep(parameter);
        error_2 = model.data()->SumofSquares();
        norm = 0;
        for(int i = 0; i < globalConstants.size(); ++i)
            norm += qAbs(globalConstants[i]-model.data()->Constants()[i]);
    } 
    QString result;
    result += "Levenberg-Marquardt returned in  " + QString::number(iter) + " iter, sumsq " + QString::number(model.data()->ModelError()) + "\n";
    result += "Last Sum of Changes in complexation constants was " + QString::number(norm) + "\n";
    result += "New vector:";    
    for(int i = 0; i < param.size(); ++i)
    {
        result +=  QString::number(param[i]) + " ";
    }
    result += "\n";
    model.data()->Message(result, 4);
    
    for(int i = 0; i < functor.inputs(); ++i)
        param[i] = parameter(i);
    return iter;
}

