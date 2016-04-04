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

#ifndef DATACLASS_H
#define DATACLASS_H

#include <QtCore/QObject>
#include <QtCore/QVector>
class DataClass : public QObject
{
    Q_OBJECT

public:
DataClass(int type = 1);
~DataClass();
enum { 
    DiscretData = 1,
    ContiuousData = 2
    };
    
private:
    QVector< qreal > m_gast, m_host;
    QVector< QVector< qreal >  > m_signals;
    int m_type;
};

#endif // DATACLASS_H
