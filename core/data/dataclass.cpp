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
#include <QPointer>
#include <QDebug>
#include <QColor>
#include "dataclass.h"


DataPoint::DataPoint(qreal conc1, qreal conc2, QVector< qreal > data) : m_conc1(conc1), m_conc2(conc2), m_data(data)
{
    
}





DataPoint::DataPoint(const DataPoint* other)
{
    m_conc1 = other->Conc1();
    m_conc2 = other->Conc2();
    m_data = other->Data();
}

DataPoint::DataPoint(const DataPoint& other)
{
    m_conc1 = other.Conc1();
    m_conc2 = other.Conc2();
    m_data = other.Data();
}

DataPoint::DataPoint()
{

}

DataClass::DataClass() : m_maxsize(0)
{

}

DataClass::DataClass(int type) : m_type(type) , m_maxsize(0), m_concentrations(new bool(true))
{

    
}

DataClass::DataClass(const DataClass& other): m_maxsize(0), m_concentrations(new bool(true))
{
    m_type = other.Type();
//     m_concentrations = other.m_concentrations;
    for(int i = 0; i < other.DataPoints(); ++i)
        addPoint(other[i]);
}

DataClass::DataClass(const DataClass* other): m_maxsize(0), m_concentrations(new bool(true))
{
     m_type = other->Type();
//      m_concentrations = other->m_concentrations;
     for(int i = 0; i < other->DataPoints(); ++i)
         addPoint(other->operator[](i));
}

DataClass::~DataClass()
{
    
}

QColor DataClass::color(int i) const
{
    switch(i){
        case 0:
            return Qt::red;
        case 1:
            return Qt::blue;
        case 2:
            return Qt::green;
        case 3:
            return Qt::yellow;
        case 4:
            return Qt::darkRed;
        case 5:
            return Qt::darkBlue;
        case 6:
            return Qt::darkGreen;
        case 7:
            return Qt::magenta;
        case 8:
            return Qt::cyan;
        case 9:
            return Qt::darkYellow;
        case 10:
            return Qt::darkMagenta;
        case 11:
            return Qt::darkCyan;
        case 12:
            return Qt::gray;
        default:
            return Qt::darkGray;
    }
}


QVector< QPointer< QtCharts::QScatterSeries > > DataClass::Signals(int c) const
{
    

    QVector< QPointer< QtCharts::QScatterSeries > > series;
    if(Size() == 0)
        series;
    
    for(int i = 0; i < m_data.size(); ++i)
    {
         if(i == 0)
         {
            for(int j = 0; j < Size(); ++j)
            {
             series.append(new QtCharts::QScatterSeries());
             series.last()->setColor(color(j));
//              series.last()->setName("Signal " + QString::number(j + 1));
             series.last()->setMarkerShape(QtCharts::QScatterSeries::MarkerShapeRectangle);
             series.last()->setMarkerSize(10);
            }
         }
        for(int j = 0; j < Size(); ++j)
        {
         if(c == 1)   
            series[j]->append(m_data[i].Conc1(), m_data[i].Data()[j]);
         else if(c == 2)
            series[j]->append(m_data[i].Conc2(), m_data[i].Data()[j]);
         else if(c == 3)
             series[j]->append(m_data[i].Conc2()/m_data[i].Conc1(), m_data[i].Data()[j]);
         else if(c == 4)
             series[j]->append(m_data[i].Conc1()/m_data[i].Conc2(), m_data[i].Data()[j]);
         else
             series[j]->append(m_data[i].Conc1(), m_data[i].Data()[j]);
        }
    }
    return series;
}
void DataClass::SwitchConentrations()
{
    qDebug() << m_concentrations << *m_concentrations;
     *m_concentrations = !(*m_concentrations); 
     
}
