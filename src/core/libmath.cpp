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
 #include <qmath.h>

#include "src/global_config.h"


#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/NonLinearOptimization>



#include <QtGlobal>
#include <QtMath>
#include <QDebug>
#include <cmath>
#include <QPair>
#include "src/core/AbstractModel.h"
#include <iostream>
namespace Cubic{
    qreal f(qreal x, qreal a, qreal b, qreal c, qreal d)
    {
        return (x*x*x*a +x*x*b + x*c + d);
    }
    
    qreal df(qreal x, qreal a, qreal b, qreal c)
    {
        return (3*x*x*a +2*x*b + c);
    }
}




qreal MinQuadraticRoot(qreal a, qreal b, qreal c)
{
    return (-b- qSqrt(qPow(b,2)-4*a*c))/(2*a);
}

QPair<qreal, qreal> QuadraticRoot(qreal a, qreal b, qreal c)
{
    QPair<qreal, qreal> pair(0,0);
    if((qPow(b,2)-4*a*c) < 0)
        return pair;
    pair.first = (-b- qSqrt(qPow(b,2)-4*a*c))/(2*a);
    pair.second = (-b+ qSqrt(qPow(b,2)-4*a*c))/(2*a);
    return pair;
}

qreal MinCubicRoot(qreal a, qreal b, qreal c, qreal d)
{
    qreal root1 = 0;
    qreal root2 = 0;
    qreal root3 = 0;
    
    qreal m_epsilon = 1e-8;
    int m_maxiter = 100;
    
    
    double guess_0;// = -p/2+qSqrt(p*p-q);
    double guess_1;// = -p/2-qSqrt(p*p-q);
    
    QPair<qreal, qreal> pair = QuadraticRoot(3*a, 2*b, c);
    guess_0 = pair.first;
    guess_1 = pair.second;
    
    //      qDebug() << guess_0 << " " << guess_1;
    
    double x = guess_0+1;
    double y = Cubic::f(x, a, b,c ,d);
    int i = 0;
    while(qAbs(y) > m_epsilon)
    {
        double dy = Cubic::df(x, a, b, c);
        x = x - y/dy;
        y = Cubic::f(x, a, b,c ,d);
        //  qDebug() << "x " << x << " y " << y;
        ++i;
        if(i > m_maxiter)
            break;
    }
    root1 = x;
//     qDebug() << i << "iterations for root 11111";
    x = (guess_0+guess_1)/2;
    y = Cubic::f(x, a, b,c ,d);
    i = 0;
    while(qAbs(y) > m_epsilon)
    {
        double dy = Cubic::df(x, a, b, c);
        x = x - y/dy;
        y = Cubic::f(x, a, b,c ,d);
        //     qDebug() <<"x " << x << " y " << y;
        ++i;   
        if(i > m_maxiter)
            break;
    }
    root2 = x;
    
    
    x = guess_1-1;
    y = Cubic::df(x, a, b, c);
//     qDebug() << i << "iterations for root 22222";
    i = 0;
    while(qAbs(y) > m_epsilon)
    {
        double dy = Cubic::df(x, a, b, c);
        x = x - y/dy;
        y = Cubic::df(x, a, b, c);
        //     qDebug() <<"x " << x << " y " << y;
        ++i;  
        if(i > m_maxiter)
            break;
    }
    root3 = x;
//     qDebug() << i << "iterations for root 33333";
    //     qDebug() << root1 << root2 << root3;
    if(root1 < 0)
    {
        if(root2 < 0)
            return root3;
        else
            return root2;
    }else if(root2 < 0)
    {
        if(root1 < 0)
            return root3;
        else
            return root1;      
    }else
    {
        if(root1 < 0)
            return root2;
        else
            return root1;    
    }
    
}






template<typename _Scalar, int NX = Eigen::Dynamic, int NY = Eigen::Dynamic>
struct EqualSystem
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
    
    EqualSystem(int inputs, int values) : m_inputs(inputs), m_values(values) {}
    
    int inputs() const { return m_inputs; }
    int values() const { return m_values; }
    
};

struct MyEqualSystem : EqualSystem<double>
{
    MyEqualSystem(int inputs, int values) : EqualSystem(inputs, values), no_parameter(inputs),  no_points(values) 
    {
        
    }
    int operator()(const Eigen::VectorXd &parameter, Eigen::VectorXd &fvec) const
    {
        fvec(0) = parameter(0) + parameter(0)*parameter(1)*beta_11 + 2*qPow(parameter(0),2)*parameter(1)*beta_21 - Concen_0(0);
        fvec(1) = parameter(1) + parameter(0)*parameter(1)*beta_11 +   qPow(parameter(0),2)*parameter(1)*beta_21 - Concen_0(1);
        return 0;
    }
    Eigen::VectorXd Concen_0;
    double A_0, B_0, beta_11, beta_21;
    int no_parameter;
    int no_points;
    int inputs() const { return no_parameter; } // There are two parameters of the model
    int values() const { return no_points; } // The number of observations
};

struct MyEqualSystemNumericalDiff : Eigen::NumericalDiff<MyEqualSystem> {};



int SolveEqualSystem(double A_0, double B_0, double beta_11, double beta_21, QVector<double > &concentration)
{
       
    Eigen::VectorXd parameter(2);
    parameter(0) = A_0;
    parameter(1) = B_0;
    
    Eigen::VectorXd Concen_0(2);
        Concen_0(0) = A_0;
        Concen_0(1) = B_0;
    MyEqualSystem functor(2, 2);
    functor.Concen_0 = Concen_0;
     functor.beta_11 = beta_11;
     functor.beta_21 = beta_21;
    Eigen::NumericalDiff<MyEqualSystem> numDiff(functor);
    Eigen::LevenbergMarquardt<Eigen::NumericalDiff<MyEqualSystem> > lm(numDiff);
    int iter = 0;
    Eigen::LevenbergMarquardtSpace::Status status = lm.minimizeInit(parameter);
      do {
         status = lm.minimizeOneStep(parameter);
         iter++;
      } while (status == -1);
  
    concentration << double(parameter(0)) << double(parameter(1));
    return iter;
}

template<typename _Scalar, int NX = Eigen::Dynamic, int NY = Eigen::Dynamic>
struct MultiEqualSystem
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
    
    MultiEqualSystem(int inputs, int values) : m_inputs(inputs), m_values(values) {}
    
    int inputs() const { return m_inputs; }
    int values() const { return m_values; }
    
};

struct MyMultiEqualSystem : MultiEqualSystem<double>
{
    MyMultiEqualSystem(int inputs, int values) : MultiEqualSystem(inputs, values), no_parameter(inputs),  no_points(values) 
    {
        
    }
    int operator()(const Eigen::VectorXd &parameter, Eigen::VectorXd &fvec) const
    {
        for(int i = 0; i < no_equations; ++i)
        {
            fvec(2*i) = parameter(2*i) + parameter(2*i)*parameter(2*i+1)*beta_11 + 2*qPow(parameter(2*i),2)*parameter(2*i+1)*beta_21 - Concen_0(2*i);
            fvec(2*i+1) = parameter(2*i+1) + parameter(2*i)*parameter(2*i+1)*beta_11 +   qPow(parameter(2*i),2)*parameter(2*i+1)*beta_21 - Concen_0(2*i+1);
        }
        return 0;
    }
    Eigen::VectorXd Concen_0;
    QVector<double >A_0, B_0;
    double beta_11, beta_21;
    int no_equations;
    int no_parameter;
    int no_points;
    int inputs() const { return no_parameter; } // There are two parameters of the model
    int values() const { return no_points; } // The number of observations
};

struct MyMultiEqualSystemNumericalDiff : Eigen::NumericalDiff<MyMultiEqualSystem> {};


int SolveEqualSystem(QVector<double >A_0, QVector<double> B_0, double beta_11, double beta_21, QVector<double > &A_equ, QVector<double > &B_equ)
{
       
    Eigen::VectorXd parameter(A_0.size()+B_0.size());
    Eigen::VectorXd Concen_0(A_0.size()+B_0.size());
    for(int i = 0; i < A_0.size(); ++i)
    {
        Concen_0(2*i) = A_0[i];
        parameter(2*i) = A_0[i];
        Concen_0(2*i+1) = B_0[i];
        parameter(2*i+1) = B_0[i];
    }
    
    MyMultiEqualSystem functor(A_0.size()+B_0.size(), A_0.size()+B_0.size());
    functor.Concen_0 = Concen_0;
     functor.beta_11 = beta_11;
     functor.beta_21 = beta_21;
     functor.no_equations = A_0.size();
    Eigen::NumericalDiff<MyMultiEqualSystem> numDiff(functor);
    Eigen::LevenbergMarquardt<Eigen::NumericalDiff<MyMultiEqualSystem> > lm(numDiff);
    int iter = 0;
    Eigen::LevenbergMarquardtSpace::Status status = lm.minimizeInit(parameter);
      do {
         status = lm.minimizeOneStep(parameter);
         iter++;
      } while (status == -1);
  
          
      for(int i = 0; i < A_0.size(); ++i)
    {
        A_equ << parameter(2*i); 
        B_equ << parameter(2*i+1);
    }
    
    
    return iter;
}

