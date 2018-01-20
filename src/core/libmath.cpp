/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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


#include <QtGlobal>
#include <QtMath>
#include <QDebug>
#include <QPair>

#include <iostream>
#include <cmath>

#include "src/core/AbstractModel.h"

#include "libmath.h"

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


LinearRegression LeastSquares(const QVector<qreal> &x, const QVector<qreal> &y)
{
    LinearRegression regression;
    
    if(x.size() != y.size())
        return regression;
#ifdef _DEBUG
    qDebug() << x << y;
#endif
    // http://www.bragitoff.com/2015/09/c-program-to-linear-fit-the-data-using-least-squares-method/ //
    qreal xsum = 0, x2sum = 0, ysum = 0, xysum = 0;                //variables for sums/sigma of xi,yi,xi^2,xiyi etc
    int n = x.size();
    for (int i = 0; i < n; ++i)
    {
        xsum += x[i];                        //calculate sigma(xi)
        ysum +=y[i];                        //calculate sigma(yi)
        x2sum += (x[i]*x[i]);                //calculate sigma(x^2i)
        xysum += x[i]*y[i];                    //calculate sigma(xi*yi)
    }
    regression.m = (n*xysum-xsum*ysum)/(n*x2sum-xsum*xsum);            //calculate slope
    regression.n = (x2sum*ysum-xsum*xysum)/(x2sum*n-xsum*xsum);            //calculate intercept
    qreal mean_x = xsum/double(n);
    qreal mean_y = ysum/double(n);
    qreal x_ = 0;
    qreal y_ = 0;
    qreal xy_ = 0;
    regression.x = x;
    regression.y = y;
    for (int i = 0 ; i < n; ++i)
    {
        qreal y_head = regression.m*x[i]+regression.n;  
        regression.y_head << y_head;
        regression.sum_err += (y_head-y[i])*(y_head-y[i]);
        x_ += (x[i]-mean_x)*(x[i]-mean_x);
        y_ += (y[i]-mean_y)*(y[i]-mean_y);
        xy_ += (x[i]-mean_x)*(y[i]-mean_y);
    }
    regression.R = (xy_/qSqrt(x_*y_))*(xy_/qSqrt(x_*y_));
    
#ifdef _DEBUG
    qDebug() << QString(" -- y = " + QString::number(regression.m) +"x+"+ QString::number(regression.n) + " -- R2 = " + QString::number(regression.R) + " Sum Error: " + QString::number(regression.sum_err));
#endif
    return regression;
}

QMap<qreal, MultiRegression> LeastSquares(const QVector<qreal> &x, const QVector<qreal> &y, int functions)
{
    QMap<qreal, MultiRegression> regressions;
    if(functions == 1)
    {
        LinearRegression regression = LeastSquares(x,y);
        MultiRegression reg;
        reg.regressions << regression;
        reg.sum_err = regression.sum_err;
        reg.start << 0 <<  x.size() - 1;
        regressions.insert(reg.sum_err, reg);
    }
    else if(functions == 2)
    {
        for(int i = 2; i < x.size() - 1; ++i)
        {
            QVector<qreal> x_i, x_j, y_i, y_j;
            for(int i_i = 0; i_i < i; ++i_i)
            {
                x_i << x[i_i];
                y_i << y[i_i];
            } 
            for(int j_j = i; j_j < x.size(); ++j_j)
            {
                x_j << x[j_j];
                y_j << y[j_j];
            }
            
            LinearRegression reg_i = LeastSquares(x_i,y_i);
            LinearRegression reg_j = LeastSquares(x_j,y_j);
            MultiRegression reg;
            reg.regressions << reg_i;
            reg.regressions << reg_j;
            reg.start << 0 << i << i - 1 << x.size() - 1 ;
            reg.sum_err = reg_i.sum_err + reg_j.sum_err;
            regressions.insert(reg.sum_err, reg);
        }
    }
    return regressions;
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
