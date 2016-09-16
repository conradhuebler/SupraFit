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
// #include <qmath.h>
#include <levmar/levmar.h>
#include <QtGlobal>
#include <QtMath>
#include <QDebug>
#include <cmath>
#include <QPair>
#include "core/data/modelclass.h"
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
};




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

struct mydata{
   AbstractTitrationModel *model;
};



void TitrationModel(double *p, double *x, int m, int n, void *data)
{

    Q_UNUSED(n);
    struct mydata *dptr;

    dptr=(struct mydata *)data; 
    QVector<qreal> parameter;
    for(int i = 0; i < m; ++i)
        parameter << p[i];
    qDebug() << parameter;
    dptr->model->setParamter(parameter);
 
    dptr->model->CalculateSignal();
    int index = 0;
    for(int j = 0; j < dptr->model->SignalCount(); ++j)
      {
        for(int i = 0; i < dptr->model->DataPoints(); ++i)
        {
            x[index] = dptr->model->ModelSignal()->data(j,i);
            index++;
        }
    }
}

int MinimizingComplexConstants(AbstractTitrationModel *model, int max_iter, QVector<qreal > &param)
{
    double opts[LM_OPTS_SZ], info[LM_INFO_SZ];
    opts[0]=LM_INIT_MU; opts[1]=1E-15; opts[2]=1E-15; opts[3]=1E-20;
    opts[4]=LM_DIFF_DELTA;
    struct mydata data;
    data.model = model;
    int index = 0;
    double x[data.model->DataPoints()*data.model->SignalCount()];
    for(int j = 0; j < data.model->SignalCount(); ++j)
    {
        for(int i = 0; i < data.model->DataPoints(); ++i)
        {
            x[index] = data.model->SignalModel()->data(j,i); 
            index++;
        }
    }
    

    QVector<double > parameter = param;
    double p[parameter.size()];
    for(int i = 0; i < parameter.size(); ++i)
    {
        p[i] =  parameter[i];
    }
    
    int m =  parameter.size();
    int n = model->DataPoints()*model->SignalCount();
    
    int nums = dlevmar_dif(TitrationModel, p, x, m, n, max_iter, opts, info, NULL, NULL, (void *)&data);

  printf("Levenberg-Marquardt returned in %g iter, reason %g, sumsq %g [%g]\n", info[5], info[6], info[1], info[0]);
  printf("Best fit parameters:" );    
  param.clear();
//   QVector<qreal> parameter_fertig;
    for(int i = 0; i < m; ++i)
        {
            param << p[i];
            std::cout << p[i] << " ";
        }
        std::cout << "\n";
    return nums;
}

