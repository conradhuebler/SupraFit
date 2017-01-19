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

#ifdef USE_bfgsOptimizer
#include "LBFGSpp/include/LBFGS.h"
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/LU>
#include "src/core/AbstractModel.h"
#include "src/core/libmath.h"
#include <iostream>

using namespace Eigen;

typedef QVector<qreal > Variables;
using namespace LBFGSpp;


class Function
{
public:
    Function(QSharedPointer<AbstractTitrationModel> model) : m_model(model) {ModelSignals = m_model->getSignals(m_model->ActiveSignals());}
    double operator()(const VectorXd& value, VectorXd& grad)
    {
        grad = gradient(value);
        return var(value);
    }
    
    VectorXd gradient(const VectorXd& value)
    {
        qreal diff = 1e-5;
        VectorXd grad(value.size());        
        for(int i = 0; i < value.size(); ++i)
        {
            VectorXd input = value;
            qreal y1 = 0, y2 = 0;
            
            input[i] += diff;
            y1 = var(input);
            input[i] -= 2*diff;
            y2 = var(input);
            grad(i) = (y1-y2)/2/diff;
           
        }
        return grad;
    }
    
    double var(const VectorXd& value)
    {
        QVector<qreal> parameter(value.size());
        for(int i = 0; i < value.size(); ++i)
            parameter[i] = value(i);
        
        m_model->setParamter(parameter);
        m_model->CalculateSignal();
        return m_model->ModelError();
    }
    Variables ModelSignals;
    QSharedPointer<AbstractTitrationModel> m_model;
};    
      
#ifdef OWN_BFGS
class BFGSSolver
{
public:
    BFGSSolver(Function func) : m_function(func) { }
    int Minimize(VectorXd &parameter)
    {
        int iter = 0;
        VectorXd grad;
        qreal old_error = m_function(parameter, grad);
        qreal new_error = old_error;
        qreal tk = 1;
        MatrixXd hessian(parameter.size(),parameter.size());
        hessian = grad*grad.transpose();
        int maxiter = 100;
//         for(int i = 0; i < parameter.size(); ++i)
//             for(int j = 0; j < parameter.size(); ++j)
//                 if(i == j)
//                     hessian(i,j) = 1;
//                 else
//                     hessian(i,j) = 0;
        for(iter = 0; iter < maxiter; ++iter)
        {
        if(qAbs(grad.squaredNorm()) < 1e-8)
        {
            return true;
        }
        
/*        
        Variables tmp_y = m_function(parameter, grad);
        VectorXd y(tmp_y.size());
        for(int k = 0; k < tmp_y.size(); ++k)
            y(k) = tmp_y[k];
        VectorXd dk = (grad).lu().solve(y);
        VectorXd param_2 = (parameter + dk).transpose();
        */
        /*
         * Not working bgfs
         */
        
        
        VectorXd dk = (-1*hessian).lu().solve(grad);
//         std::cout << "solved eqn" << std::endl;
//         std::cout << dk << std::endl;
        
        VectorXd dktk = tk*dk;
        if(new_error > old_error)
        {
            tk *= 0.9;
        }
//         std::cout << parameter << std::endl << dktk << std::endl;
        VectorXd param_2 = (parameter + dktk).transpose();
//         std::cout << "New parameter " << std::endl;
//         std::cout << param_2 << std::endl;
        VectorXd grad2;
        m_function(param_2, grad2);
        VectorXd yk = (grad2 - grad);
        VectorXd temp = hessian*dk;
        MatrixXd temp2 = (dk-hessian*yk).transpose();
//         std::cout << temp2 << "temp2 " << std::endl;
        MatrixXd hessian_2 = hessian + yk*yk.transpose()/(yk.transpose()*dk)-hessian*dk*temp.transpose()/(dk.transpose()*hessian*dk);
        
        /*
         * Not working inverse bgfs
         */
        
       /* double squard = ((dk.transpose()*yk))*(dk.transpose()*yk);
        MatrixXd zahl = temp2*yk;
        MatrixXd prod = dk*dk.transpose();
        std::cout << prod << std::endl;
        MatrixXd first_part = (dk-hessian*yk)*dk.transpose();
        MatrixXd second_part = dk*temp2/(dk.transpose()*yk);
        std::cout << zahl << std::endl;
        MatrixXd third_part = zahl/squard*prod;
        MatrixXd hessian_2 = hessian + first_part + second_part;// - third_part;*/
       /* new_error = m_function->value(param_2).first();*/
       if(new_error < old_error)
            parameter = param_2;
//         hessian = hessian_2;  
        }
        return iter;
    }
private:
    Function m_function;
};

#endif
       
int NonlinearFit(QWeakPointer<AbstractTitrationModel> model, int max_iter, QVector<qreal > &param, const OptimizerConfig &config)
{
    Q_UNUSED(max_iter)
    Q_UNUSED(config)
    VectorXd parameter(param.size());
    for(int i = 0; i < param.size(); ++i)
        parameter(i) = param[i];
    
    LBFGSParam<double> pam;
    pam.epsilon = 1e-6;
    pam.max_iterations = 1;
    QString message = QString();
    message += "Starting BFGS for " + QString::number(parameter.size()) + " parameters:\n";
    message += "Old vector : ";
    foreach(double d, param)
    {
        message += QString::number(d) + " ";
    }
    message += "\n";
    model.data()->Message(message, 5);


    
    int iter = 0;
#ifdef OWN_BFGS   
    
    Function function (model);
    
    BFGSSolver solver(function);
    iter = solver.Minimize(parameter);
    
#endif
#ifndef OWN_BFGS
    // Create solver and function object
    LBFGSSolver<double> solver(pam);
    Function fun(model);

    double fx;
    iter = solver.minimize(fun, parameter, fx);
#endif

    for(int i = 0; i < parameter.size(); ++i)
        param[i] = parameter(i);
           QString result;
    result += "BFGS returned in  " + QString::number(iter) + " iter, sumsq " + QString::number(model.data()->ModelError()) + "\n";
    result += "New vector:";    
    for(int i = 0; i < param.size(); ++i)
    {
        result +=  QString::number(param[i]) + " ";
    }
    result += "\n";
    model.data()->Message(result, 4);
    
    return iter;
}

#endif
