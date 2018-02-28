 /*
  * Tools to calculate Equilibrium Concentrations for different models
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
 
#include <iostream>

#include "equil.h"
 
 IItoI_ItoI_ItoII_Solver::IItoI_ItoI_ItoII_Solver() : m_ok(false)
 {
     setAutoDelete(false);
 }
 
 IItoI_ItoI_ItoII_Solver::~IItoI_ItoI_ItoII_Solver()
 {
     
 }
 
 void IItoI_ItoI_ItoII_Solver::setInput(double A_0, double B_0)
 {
     m_A_0 = A_0;
     m_B_0 = B_0;
     m_concentration = QPair<double, double>(A_0,B_0);
 }
 
 void IItoI_ItoI_ItoII_Solver::run()
 {
     if(m_A_0 && m_B_0)
         m_concentration = HostConcentration(m_A_0, m_B_0);
     else
         m_ok = true;
 }
 
 
 QPair<double, double> IItoI_ItoI_ItoII_Solver::HostConcentration(double a0, double b0)
 {
     if(!a0 || !b0)
         return QPair<double, double>(a0,b0);
     
     qreal K21 = m_parameter[0];
     qreal K11 = m_parameter[1];
     qreal K12 = m_parameter[2];
     qreal b12 = K11*K12;
     qreal b21 = K11*K21;
     
     auto calc_a = [](double a0, long double b, double K11, double b21, double b12){
         long double x1 = 2*b21*b;
         long double x2 = b12*b*b+K11*b+1;
         long double x3 = -a0;
         long double a = MaxQuadraticRoot(x1,x2,x3);
         if(a < a0)
             return a;
         else
         {
             #ifdef _DEBUG
             std::cout << "a: " << a << " a0 " << a0  << std::endl;
             #endif
             return MinQuadraticRoot(x1,x2,x3);
         }
     };
     
     auto calc_b = [](double b0, long double a, double K11, double b21, double b12){
         long double x1 = 2*b12*a;
         long double x2 = b21*a*a+K11*a+1;
         long double x3 = -b0;
         long double b = MaxQuadraticRoot(x1,x2,x3);
         if(b < b0 )
             return b;
         else
         {
             #ifdef _DEBUG
             std::cout << "b: " << b << " b0: " << b0 << std::endl;
             #endif
             return MinQuadraticRoot(x1,x2,x3);
         }
     };
     long double epsilon = m_opt_config.concen_convergency;
     long double a = qMin(a0,b0)/K11*10;
     long double b = 0;
     long double a_1 = 0, b_1 = 0;
     int i = 0;
     for(i = 0; i < m_opt_config.single_iter; ++i)
     {
         a_1 = a;
         b_1 = b;
         b = calc_b(b0, a, K11, b21, b12);
         if(b < 0)
             b *= -1;
         
         a = calc_a(a0, b, K11, b21, b12);
         if(a < 0)
             a *= -1;
         if(qAbs(b21*a_1*a_1*b_1-b21*a*a*b) < epsilon && qAbs(b12*a_1*b_1*b_1-b12*a*b*b) < epsilon && qAbs(K11*a_1*b_1 - K11 * a*b) < epsilon)
             break;
     }
     #ifdef _DEBUG
     std::cout << a_1 << " "<< b_1 << " " << K11*a_1*b_1 << " " << b21*a_1*a_1*b_1 << " " << b12*a_1*b_1*b_1 << std::endl;
     std::cout << a << " "<< b << " " << K11*a*b << " " << b21*a*a*b << " " << b12*a*b*b << std::endl;
     std::cout << "Guess A: " << qMin(a0,b0)/K11*100 << " .. Final A: " << a << " .. Iterations:" << i<< std::endl;
     #endif
     m_ok =  (a < m_A_0) &&
             (b < m_B_0) &&
             (a > 0) &&
             (b > 0) &&
             i < m_opt_config.single_iter;

     return QPair<double, double>(a,b);
 }
 
