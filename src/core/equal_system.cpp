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

#include "src/core/models.h"

#include <QtGlobal>
#include <QtMath>
#include <QDebug>
#include <cmath>
#include <QPair>
#include <iostream>
#include <QtCore/QPointer>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/NonLinearOptimization>

#include "equal_system.h"

int MyScripteEqualSystem::operator()(const Eigen::VectorXd &parameter, Eigen::VectorXd &fvec) const
{
        qreal A = parameter(0);
        qreal B = parameter(1); 
        
        Vector balance = m_model->MassBalance(A, B);
        
        fvec = parameter + balance - Concen_0;

        return 0;
}

ConcentrationSolver::ConcentrationSolver(QPointer<AbstractTitrationModel> model) : m_model(model)
{
    setAutoDelete(false);
    functor = new MyScripteEqualSystem(2, 2, m_model);
}

ConcentrationSolver::~ConcentrationSolver()
{
    delete functor;
}

void ConcentrationSolver::setInput(double A_0, double B_0)
{
    m_A_0 = A_0;
    m_B_0 = B_0;
}

void ConcentrationSolver::run()
{
    SolveEqualSystem(m_A_0, m_B_0);
}

int ConcentrationSolver::SolveEqualSystem(double A_0, double B_0)
{
       
    if(A_0 == 0 || B_0 == 0)
    {
        m_concentration= QList<double>()<<  A_0 << B_0;
        return 1;
    }
    Eigen::VectorXd parameter(2);
    parameter(0) = A_0;
    parameter(1) = B_0;
    
    Eigen::VectorXd Concen_0(2);
        Concen_0(0) = A_0;
        Concen_0(1) = B_0;
        
    functor->Concen_0 = Concen_0;

    Eigen::NumericalDiff<MyScripteEqualSystem> numDiff(*functor);
    Eigen::LevenbergMarquardt<Eigen::NumericalDiff<MyScripteEqualSystem> > lm(numDiff);
    int iter = 0;
    Eigen::LevenbergMarquardtSpace::Status status = lm.minimizeInit(parameter);
      do {
          for(int i = 0; i < 2; ++i)
            if(parameter(i) < 0)
            {
                std::cout << "numeric error (below zero): " << i << std::endl;
                parameter(i) = qAbs(parameter(i));
            }else if(parameter(i) > Concen_0(i))
            {
                std::cout << "numeric error (above init): " << i << std::endl;
                qreal diff = (parameter(i) -Concen_0(i));
                parameter(i) = diff;
            }
         status = lm.minimizeOneStep(parameter);
         iter++;
      } while (status == -1);
    for(int i = 0; i < 2; ++i)
        if(parameter(i) < 0 || parameter(i) > Concen_0(i))
            std::cout << "final numeric error " << i << " " << parameter(i) << " " << Concen_0(i) << std::endl;
    m_concentration = QList<double>() << double(parameter(0)) << double(parameter(1));
    return iter;
}


