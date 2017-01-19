/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifdef experimental
#include <QtGlobal>
#include <QtMath>
#include <QDebug>
#include <cmath>
#include <QPair>
#include <iostream>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/NonLinearOptimization>

#include "equal_system.h"

/*
 * This is an experimental solver for concentration systems, which will be implemented in the future
 */

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

/* 
 * This seems not to work as expected.
 */

/*
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
*/
#endif
