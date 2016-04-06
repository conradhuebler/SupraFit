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

#include "core/libmath.h"
#include <QtMath>

#include <QDebug>
#include "cmath"
#include "modelclass.h"

AbstractTitrationModel::AbstractTitrationModel(const DataClass *data, QObject *parent) : DataClass(data), QObject(parent), m_concentrations(false)
{
    for(int i = 0; i < m_data.size(); ++i)
    {
        m_signals[&m_data[i]] = QVector<qreal>(Size(), 0);
//         m_signals[&m_data[i]] = 0;//FIXME no contign... signals
    }
    
    m_pure_signals.resize(Size());   
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        m_pure_signals[i] = m_data.first().Data()[i];
    }
}

AbstractTitrationModel::~AbstractTitrationModel()
{

}

bool AbstractTitrationModel::MinimizeConstants()
{
    QVector<int > vars(MaxVars(), 0);
    
    for(int i = 0; i < ConstantSize(); ++i)
        vars[i] = 1;
    return Minimize(vars);
}

bool AbstractTitrationModel::MinimizeSignals()
{
    QVector<int > vars(MaxVars(), 0);
    
    for(int i = ConstantSize(); i < m_pure_signals.size(); ++i)
        vars[i] = 1;
    
    return Minimize(vars);
}

QVector< QPointer< QtCharts::QLineSeries > > AbstractTitrationModel::Signals(int c) const
{
 QVector< QPointer< QtCharts::QLineSeries > > series;
    if(Size() == 0)
        series;
    
    for(int i = 0; i < m_data.size() - 1; ++i)
    {
         if(i == 0)
         {
//              int j = 0;
            for(int j = 0; j < Size(); ++j)
            {
             series.append(new QtCharts::QLineSeries());
             series.last()->setName("Signal " + QString::number(j + 1));
            }
         }
        for(int j = 0; j < Size(); ++j) // FIXME
        {
/*         if(c == 1)   
            series[j]->append(m_data[i].Conc1(), m_signals.value(m_data[i]));
         else if(c == 2)
            series[j]->append(m_data[i].Conc2(),  m_signals[&m_data[i]]);
         else if(c == 3)*/
             series[j]->append(m_data[i].Conc2()/m_data[i].Conc1(),  m_signals[&(m_data[i])][j]);/*
         else if(c == 4)
             series[j]->append(m_data[i].Conc1()/m_data[i].Conc2(),  m_signals[&m_data[i]]);
         else
             series[j]->append(m_data[i].Conc1(),  m_signals[&m_data[i]]);*/
        }
    }
    return series;
}

ItoIModel::ItoIModel(const DataClass *data, QObject *parent) : AbstractTitrationModel(data, parent), m_K11(3)
{
    m_ItoI_signals.resize(m_pure_signals.size());
    
    for(int i = 0; i <  m_ItoI_signals.size(); ++i)
    {
         m_ItoI_signals[i] = m_data.last().Data()[i];
    }
    
//     m_ItoI_signals.first() = m_data.last().Data().first();
//     qDebug() <<  m_data.last().Data();
//     for(int i = 0; i < m_pure_signals.size(); ++i)
//     {
//         m_ItoI_signals[i] = m_data[i].Data().last(); //FIXME no contign... signals
//         qDebug() <<  m_data[i].Data();
//     }
}

ItoIModel::~ItoIModel() 
{

}

qreal ItoIModel::Minimize(QVector<int > vars)
{
    return 0.1;
}

qreal ItoIModel::ComplexConcentration(qreal host_0, qreal guest_0)
{
    qreal complex;
    qreal a, b, c;
    a = m_K11;
    b = -1*(m_K11*host_0+m_K11*guest_0+1);
    c = m_K11*guest_0*host_0;
    complex = MinRoot(a,b,c);
    return complex;
}


qreal ItoIModel::GuestConcentration(qreal host_0, qreal guest_0)
{
    return guest_0 - ComplexConcentration(host_0, guest_0);
}


void ItoIModel::CalculateSignal()
{  
    
    //FIXME redundant, make it simpler
    for(int i = 0; i < m_data.size(); ++i)
    {
        qreal host_0, guest_0;
        if(m_concentrations)
        {
            host_0 = m_data[i].Conc1();
            guest_0 = m_data[i].Conc2();
        }else
        {
            host_0 = m_data[i].Conc2();
            guest_0 = m_data[i].Conc1();   
        }
        qreal complex = ComplexConcentration(host_0, guest_0);
//         qreal guest = GuestConcentration(host_0, guest_0);
        qreal host = host_0 - complex;
//         qreal complex = ComplexConcentration(host_0, guest_0);
        for(int j = 0; j < Size(); ++j)
            {
            m_signals[&m_data[i]][j] = host/host_0*m_pure_signals[j] + complex/host_0*m_ItoI_signals[j];  //FIXME no contign... signals
//             qDebug() << host <<  complex   << m_signals[&m_data[i]][j] << m_pure_signals[j] << m_ItoI_signals[j] << host_0 << guest_0;
            }
  
    }
}

void ItoIModel::setConstants(QVector< qreal > list)
{
    if(list.isEmpty())
        return;
    m_K11 = pow10(list.first());
}

void ItoIModel::setPureSignals(QVector< qreal > list)
{
    m_pure_signals = list;
}

void ItoIModel::setComplexSignals(QVector< qreal > list, int i)
{
    if(i == 1)
        m_ItoI_signals = list;
}


QPair< qreal, qreal > ItoIModel::Pair(int i, int j)
{
    Q_UNUSED(i);
    if(j < m_ItoI_signals.size()) 
    {
        return QPair<qreal, qreal>(log10(m_K11), m_ItoI_signals[j]);
    }
    return QPair<qreal, qreal>(0, 0);
}

#include "modelclass.moc"
