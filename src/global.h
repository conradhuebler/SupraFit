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

#include <QtCore/QVector>

#include <cmath>
namespace SupraFit{
    
    enum Statistic{
        MonteCarlo = 1,
        WeakenedGridSearch = 2,
        ModelComparison = 3,
        CrossValidation = 4,
        Reduction = 5,
        FastConfidence = 6
    };
    
    
    
    enum {
        ItoI = 1,
        IItoI_ItoI = 2,
        ItoI_ItoII = 3,
        IItoI_ItoI_ItoII = 4,
        Michaelis_Menten = 5,
        First_Order_Kinetics = 6,
        ScriptedModel = 10,
        itc_ItoI = 11,
        fl_ItoI = 12,
        fl_IItoI_ItoI = 13,
        fl_ItoI_ItoII = 14
    };
    
    struct ConfidenceBar
    {
        qreal lower = 0;
        qreal upper = 0;
    };

    struct BoxWhisker
    {
        QList<qreal> mild_outliers, extreme_outliers;
        qreal lower_whisker = 0;
        qreal upper_whisker = 0;
        qreal lower_quantile = 0;
        qreal upper_quantile = 0;
        qreal median = 0;
        qreal mean = 0;
        int count = 0;
        
        inline qreal UpperNotch() const { return median+(1.58*(upper_quantile-lower_quantile)/sqrt(count)); }
        inline qreal LowerNotch() const { return median-(1.58*(upper_quantile-lower_quantile)/sqrt(count)); }
    };

}

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
