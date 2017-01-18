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

/*
class Function
{
    
public:
    Function(AbstractTitrationModel *model):m_model(model) { ModelSignals = m_model->getSignals(m_model->ActiveSignals()); }
    Variables value(const VectorXd &value)
    {
        QVector<qreal> parameter(value.size());
        for(int i = 0; i < value.size(); ++i)
            parameter[i] = value(i);
        
        m_model->setParamter(parameter);
        m_model->CalculateSignal();
        Variables CalculatedSignals = m_model->getCalculatedSignals();
        qreal error = 0;
        for( int i = 0; i < CalculatedSignals.size(); ++i)
        {
            error += sqrt((CalculatedSignals[i] - ModelSignals[i])*(CalculatedSignals[i] - ModelSignals[i]));
        }
        Variables vector;
        vector << error;
//         std::cout << error << std::endl;
        return vector;
    }
    Variables ModelSignals;
    AbstractTitrationModel *m_model;
};


class BFGSSolver
{
public:
    BFGSSolver(Function *func) : m_function(func) { }
    bool Minimize(VectorXd &parameter)
    {
        qreal old_error = m_function->value(parameter).first();
        qreal new_error = old_error;
        qreal tk = 1;
        MatrixXd hessian(parameter.size(),parameter.size());
        int maxiter = 100;
        for(int i = 0; i < parameter.size(); ++i)
            for(int j = 0; j < parameter.size(); ++j)
                if(i == j)
                    hessian(i,j) = 1;
                else
                    hessian(i,j) = 0;
        for(int iter = 0; iter < maxiter; ++iter)
        {
        VectorXd grad = NumericalDifferenz(parameter);
//         std::cout << "Norm of the gradient is " << grad.squaredNorm() << std::endl;
        if(qAbs(grad.squaredNorm()) < 1e-8)
        {
            return true;
        }
        
//         std::cout << "Initial Hessian" << std::endl;
//         std::cout << hessian << std::endl;
//         std::cout << "Initial Parameter" << std::endl;
//         std::cout << parameter << std::endl;
        /*
        Variables tmp_y = m_function->value(parameter);
        VectorXd y(tmp_y.size());
        for(int k = 0; k < tmp_y.size(); ++k)
            y(k) = tmp_y[k];
        VectorXd dk = (grad).lu().solve(y);
        VectorXd param_2 = (parameter + dk).transpose();*/
        /*
         * Not working bgfs
         */
        
        
        /*VectorXd dk = (-1*hessian).lu().solve(grad);
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
        VectorXd yk = (NumericalDifferenz(param_2) - grad);
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
       /* new_error = m_function->value(param_2).first();
       if(new_error < old_error)
            parameter = param_2;
//         hessian = hessian_2;  
        }
        return true;
    }
private:
    VectorXd NumericalDifferenz(const VectorXd &value)
    {
        qreal diff = 1e-7;
        VectorXd grad(value.size());
        
        for(int i = 0; i < value.size(); ++i)
        {
            VectorXd input = value;
            qreal y1 = 0, y2 = 0;
            
            input[i] += diff;
//             std::cout << "parameter to go 1 ";
//             std::cout << input << std::endl;
            y1 = m_function->value(input).first();
            input[i] -= 2*diff;
//              std::cout << "parameter to go 2 ";
//             std::cout << input << std::endl;
            y2 = m_function->value(input).first();
            grad(i) = (y1-y2)/2/diff;
           
        }
        
        return grad;
    };
    Function *m_function;
};

*/
   
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

    // Create solver and function object
    LBFGSSolver<double> solver(pam);
    Function fun(model);
//     fun.m_model = model;

    // Initial guess
//     VectorXd x = VectorXd::Zero(n);
    // x will be overwritten to be the best point found
    double fx;
    int niter = solver.minimize(fun, parameter, fx);
    
    
   /* 
    
    Function *function = new Function(model);
    
    BFGSSolver solver(function);
    bool value = solver.Minimize(parameter);
    */
    for(int i = 0; i < parameter.size(); ++i)
        param[i] = parameter(i);
    
    return niter;
}

#endif
