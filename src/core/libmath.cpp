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
