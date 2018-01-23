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

#pragma once

#include <QtGlobal>

enum OptimizationType{
        ComplexationConstants = 0x01,
        OptimizeShifts = 0x02,
        IgnoreZeroConcentrations = 0x04
    };
    
inline OptimizationType operator~ (OptimizationType a) { return (OptimizationType)~(int)a; }
inline OptimizationType operator| (OptimizationType a, OptimizationType b) { return (OptimizationType)((int)a | (int)b); }
inline OptimizationType operator& (OptimizationType a, OptimizationType b) { return (OptimizationType)((int)a & (int)b); }
inline OptimizationType operator^ (OptimizationType a, OptimizationType b) { return (OptimizationType)((int)a ^ (int)b); }
inline OptimizationType& operator|= (OptimizationType& a, OptimizationType b) { return (OptimizationType&)((int&)a |= (int)b); }
inline OptimizationType& operator&= (OptimizationType& a, OptimizationType b) { return (OptimizationType&)((int&)a &= (int)b); }
inline OptimizationType& operator^= (OptimizationType& a, OptimizationType b) { return (OptimizationType&)((int&)a ^= (int)b); }


struct OptimizerConfig
{
    int error_potenz = 2;
    
    int MaxIter = 75;
    int Sum_Convergence = 2;
    qreal Shift_Convergence = 1E-3;
    qreal Constant_Convergence = 1E-3;
    qreal Error_Convergence = 5E-7;
    
    int LevMar_Constants_PerIter = 1;
    int LevMar_Shifts_PerIter = 1;
    
    qreal LevMar_Factor = 100;
    qreal LevMar_Xtol = 1E-10;
    qreal LevMar_Gtol = 1E-10;
    qreal LevMar_Ftol = 1E-10;
    qreal LevMar_epsfcn = 1E-8;
};


        
class QString;

extern int printLevel;

void PrintMessage(const QString &str, int Level);
QString getDir();
void setLastDir(const QString &str);
