
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


#ifndef global_H
#define global_H


enum OptimizationType{
        ComplexationConstants = 0x01,
        UnconstrainedShifts = 0x02,
        ConstrainedShifts = 0x04,
        IntermediateShifts = 0x08,
        IgnoreZeroConcentrations = 0x16
    };
    
inline OptimizationType operator|(OptimizationType a, OptimizationType b)
        {return static_cast<OptimizationType>(static_cast<int>(a) | static_cast<int>(b));}
        
class QString;

extern int printLevel;

void PrintMessage(const QString &str, int Level);


#endif // global_H
