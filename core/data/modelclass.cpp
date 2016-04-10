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

AbstractTitrationModel::AbstractTitrationModel(const DataClass *data, QObject *parent) : DataClass(data), QObject(parent), m_repaint(false)
{
    for(int i = 0; i < m_data.size(); ++i)
    {
        m_signals[&m_data[i]] = QVector<qreal>(Size(), 0);
        m_difference << QVector<qreal>(Size(), 0);
//         m_signals[&m_data[i]] = 0;//FIXME no contign... signals
    }
    
    m_pure_signals.resize(Size());   
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        m_pure_signals[i] = m_data.first().Data()[i];
    }

    ptr_concentrations = data->Concentration();
//     qDebug() << ptr_concentrations << m_concentrations;
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

QVector< QPointer< QtCharts::QScatterSeries > > AbstractTitrationModel::Signals(int c) const
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

QVector< QPointer< QtCharts::QLineSeries > > AbstractTitrationModel::Model(int c) const
{
 QVector< QPointer< QtCharts::QLineSeries > > series;
    if(Size() == 0)
        series;
    
    for(int i = 0; i < m_data.size(); ++i)
    {
         if(i == 0)
         {
//              int j = 0;
            for(int j = 0; j < Size(); ++j)
            {
             series.append(new QtCharts::QLineSeries());
             series.last()->setName("Signal " + QString::number(j + 1));
             series.last()->setColor(color(j));
            }
         }
        for(int j = 0; j < Size(); ++j) // FIXME
        {
         if(c == 1)   
            series[j]->append(m_data[i].Conc1(), m_signals[&(m_data[i])][j]);
         else if(c == 2)
            series[j]->append(m_data[i].Conc2(),  m_signals[&(m_data[i])][j]);
         else if(c == 3)
             series[j]->append(m_data[i].Conc2()/m_data[i].Conc1(),  m_signals[&(m_data[i])][j]);
         else if(c == 4)
             series[j]->append(m_data[i].Conc1()/m_data[i].Conc2(),  m_signals[&(m_data[i])][j]);
         else
             series[j]->append(m_data[i].Conc1(),  m_signals[&(m_data[i])][j]);
        }
    }
    return series;
}

QVector< QPointer< QtCharts::QLineSeries > > AbstractTitrationModel::ErrorBars(int c) const
{
QVector< QPointer< QtCharts::QLineSeries > > series;
    if(Size() == 0)
        series;
    
    for(int i = 0; i < m_data.size(); ++i)
    {
         if(i == 0)
         {
//              int j = 0;
            for(int j = 0; j < Size(); ++j)
            {
             series.append(new QtCharts::QLineSeries());
             series.last()->setName("Signal " + QString::number(j + 1));
             series.last()->setColor(color(j));
            }
         }
        for(int j = 0; j < Size(); ++j) // FIXME
        {
         if(c == 1)   
            series[j]->append(m_data[i].Conc1(), m_difference[i][j]);
         else if(c == 2)
            series[j]->append(m_data[i].Conc2(),  m_difference[i][j]);
         else if(c == 3)
             series[j]->append(m_data[i].Conc2()/m_data[i].Conc1(),  m_difference[i][j]);
         else if(c == 4)
             series[j]->append(m_data[i].Conc1()/m_data[i].Conc2(),  m_difference[i][j]);
         else
             series[j]->append(m_data[i].Conc1(),  m_difference[i][j]);
        }
    }
    return series;
}

qreal AbstractTitrationModel::SumOfErrors(int i) const
{
    qreal sum;

    if(i >= Size())
        return 0;

    for(int j = 0; j < DataPoints(); ++j)
    {
        sum += qPow(m_difference[j][i],2);
    }
//     qDebug() << sum;
    qreal zwei = sum;
    return zwei;
}


ItoI_Model::ItoI_Model(const DataClass *data, QObject *parent) : AbstractTitrationModel(data, parent), m_K11(10^3)
{
    setName(tr("1:1-Model"));
    m_ItoI_signals.resize(m_pure_signals.size());
    
    for(int i = 0; i <  m_ItoI_signals.size(); ++i)
    {
         m_ItoI_signals[i] = m_data.last().Data()[i];
    }
    CalculateSignal();
    Minimize(QVector<int >() << 1);
    
//     m_ItoI_signals.first() = m_data.last().Data().first();
//     qDebug() <<  m_data.last().Data();
//     for(int i = 0; i < m_pure_signals.size(); ++i)
//     {
//         m_ItoI_signals[i] = m_data[i].Data().last(); //FIXME no contign... signals
//         qDebug() <<  m_data[i].Data();
//     }
}

ItoI_Model::~ItoI_Model() 
{

}

qreal ItoI_Model::Minimize(QVector<int > vars)
{
    qreal old = m_K11;
    qreal step = 0.1;
    m_repaint = false;
    
    for(int j = 0; j < 100; ++j)
    {
        qreal y1 = 0;
        qreal  y2 = 0;
        qreal x1 = 0;
        qreal x2 = 0;
        qreal error;
        for(int i = 0; i < Size(); ++i)
            error += SumOfErrors(i);
//         qDebug() << "Error " << error;
    x1 = m_K11 + step*m_K11;
    m_K11 = m_K11 + step*m_K11;
    CalculateSignal();
    for(int i = 0; i < Size(); ++i)
        y1 += SumOfErrors(i);
    
    x2 = m_K11 - step*m_K11;
    m_K11 = m_K11 - step*m_K11;
    CalculateSignal();
    for(int i = 0; i < Size(); ++i)
        y2+= SumOfErrors(i);
//     qDebug() << m_K11 << x1 << x2 << "k-werte";
    qreal dydx = (y1-y2)/(x1-x2);
    if((y2 - y1) < 0)
        m_K11 = x2;
    else if((y2 - y1) > 0)
        m_K11 = x1;
    else
        step /= 10;
//     qDebug() << y1 << y2 << error;
    }
    m_repaint = true;
    return 0.1;
}

qreal ItoI_Model::ComplexConcentration(qreal host_0, qreal guest_0)
{
    qreal complex;
    qreal a, b, c;
    a = m_K11;
    b = -1*(m_K11*host_0+m_K11*guest_0+1);
    c = m_K11*guest_0*host_0;
    complex = MinQuadraticRoot(a,b,c);
    return complex;
}

qreal ItoI_Model::GuestConcentration(qreal host_0, qreal guest_0)
{
    return guest_0 - ComplexConcentration(host_0, guest_0);
}


void ItoI_Model::CalculateSignal()
{  
    
    //FIXME redundant, make it simpler
//     qDebug() << *ptr_concentrations << ptr_concentrations;
    for(int i = 0; i < m_data.size(); ++i)
    {
        qreal host_0, guest_0;
        if(*ptr_concentrations)
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
            m_difference[i][j] =  m_signals[&m_data[i]][j] - m_data[i].Data()[j];
//             qDebug() << host <<  complex   << m_signals[&m_data[i]][j] << m_pure_signals[j] << m_ItoI_signals[j] << host_0 << guest_0;
            }
  
    }
    if(m_repaint)
        emit Recalculated();
}

void ItoI_Model::setConstants(QVector< qreal > list)
{
    if(list.isEmpty())
        return;
    m_K11 = pow10(list.first());
}

void ItoI_Model::setPureSignals(QVector< qreal > list)
{
    m_pure_signals = list;
}

void ItoI_Model::setComplexSignals(QVector< qreal > list, int i)
{
    if(i == 1)
        m_ItoI_signals = list;
}


QPair< qreal, qreal > ItoI_Model::Pair(int i, int j)
{
    Q_UNUSED(i);
    if(j < m_ItoI_signals.size()) 
    {
        return QPair<qreal, qreal>(log10(m_K11), m_ItoI_signals[j]);
    }
    return QPair<qreal, qreal>(0, 0);
}


IItoI_ItoI_Model::IItoI_ItoI_Model(const DataClass* data, QObject* parent) : AbstractTitrationModel(data, parent), m_K11(qPow(10,3)), m_K21(qPow(10,2))
{
    setName(tr("2:1/1:1-Model"));
    m_ItoI_signals.resize(m_pure_signals.size());
    m_IItoI_signals.resize(m_pure_signals.size());
    for(int i = 0; i <  m_ItoI_signals.size(); ++i)
    {
         m_ItoI_signals[i] = m_data.last().Data()[i];
         m_IItoI_signals[i] = m_data.first().Data()[i];
    }
    CalculateSignal();
    Minimize(QVector<int >() << 1);
}
IItoI_ItoI_Model::~IItoI_ItoI_Model()
{

}

void IItoI_ItoI_Model::setComplexSignals(QVector< qreal > list, int i)
{
    if(i == 1)
        m_IItoI_signals = list;
    if(i == 2)
        m_ItoI_signals = list;
}

void IItoI_ItoI_Model::setConstants(QVector< qreal > list)
{
     if(list.size() < 2)
        return;
    m_K21 = pow10(list[0]);
    m_K11 = pow10(list[1]);   
}

qreal IItoI_ItoI_Model::HostConcentration(qreal host_0, qreal guest_0)
{
    qreal host;
    qreal a, b, c;
    a = m_K11*m_K21;
    b = m_K11*(2*m_K21*guest_0-m_K21*host_0+1);
    c = m_K11*(guest_0-host_0)+1;
//     qDebug() << a << b << c << -host_0;
    host = MinCubicRoot(a,b,c, -host_0);
    return host;
}


qreal IItoI_ItoI_Model::GuestConcentration(qreal host_0, qreal guest_0)
{
    qreal host;
    qreal a, b, c;
    a = m_K11*m_K21;
    b = m_K11*(2*m_K21*guest_0-m_K21*host_0+1);
    c = m_K11*(guest_0-host_0)+1;
//     qDebug() << a << b << c << -host_0;
    host = MinCubicRoot(a,b,c, -host_0);
    return guest_0/(m_K11*host+m_K11*m_K21*host*host+1);
}


void IItoI_ItoI_Model::CalculateSignal()
{
   for(int i = 0; i < m_data.size(); ++i)
    {
        qreal host_0, guest_0;
        if(*ptr_concentrations)
        {
            host_0 = m_data[i].Conc1();
            guest_0 = m_data[i].Conc2();
        }else
        {
            host_0 = m_data[i].Conc2();
            guest_0 = m_data[i].Conc1();   
        }
//         qDebug() << host_0 << guest_0 << m_K11 << m_K21;
//         qreal complex = ComplexConcentration(host_0, guest_0);
        qreal guest = GuestConcentration(host_0, guest_0);
        qreal host = HostConcentration(host_0, guest_0);
        qreal complex_11 = m_K11*host*guest;
        qreal complex_21 = m_K11*m_K21*host*host*guest;
        
//         qreal complex = ComplexConcentration(host_0, guest_0);
        for(int j = 0; j < Size(); ++j)
            {
            m_signals[&m_data[i]][j] = host/host_0*m_pure_signals[j] + complex_11/host_0*m_ItoI_signals[j]+ 2*complex_21/host_0*m_IItoI_signals[j];  //FIXME no contign... signals
            m_difference[i][j] =  m_signals[&m_data[i]][j] - m_data[i].Data()[j];
//             qDebug() << host << guest <<  complex_21 << complex_11   << m_signals[&m_data[i]][j] << m_pure_signals[j] << m_ItoI_signals[j] << host_0 << guest_0;
            }
  
    }
//     qDebug() << m_signals;
//       qDebug() << m_difference;
    if(m_repaint)
        emit Recalculated();
}

void IItoI_ItoI_Model::setPureSignals(QVector< qreal > list)
{
    m_pure_signals = list;
}


QPair< qreal, qreal > IItoI_ItoI_Model::Pair(int i, int j)
{
        if(i == 0)
        {
            

        if(j < m_IItoI_signals.size()) 
        {
            return QPair<qreal, qreal>(Constants()[i], m_IItoI_signals[j]);
        } 
            
        }else if(i == 1)
        {
           if(j < m_ItoI_signals.size()) 
        {
            return QPair<qreal, qreal>(Constants()[i], m_ItoI_signals[j]);
        }          
        }
    return QPair<qreal, qreal>(0, 0);
}


qreal IItoI_ItoI_Model::Minimize(QVector< int > vars)
{
    m_repaint = false;
    for(int k = 0; k < 10; ++k)
    {
        {
    qreal old = m_K11;
    qreal step = 0.1;
    
    
    for(int j = 0; j < 5; ++j)
    {
        qreal y1 = 0;
        qreal  y2 = 0;
        qreal x1 = 0;
        qreal x2 = 0;
        qreal error;
        for(int i = 0; i < Size(); ++i)
            error += SumOfErrors(i);
//         qDebug() << "Error " << error;
    x1 = m_K11 + step*m_K11;
    m_K11 = m_K11 + step*m_K11;
    CalculateSignal();
    for(int i = 0; i < Size(); ++i)
        y1 += SumOfErrors(i);
    
    x2 = m_K11 - step*m_K11;
    m_K11 = m_K11 - step*m_K11;
    CalculateSignal();
    for(int i = 0; i < Size(); ++i)
        y2+= SumOfErrors(i);
//     qDebug() << m_K11 << x1 << x2 << "k-werte";
    qreal dydx = (y1-y2)/(x1-x2);
    if((y2 - y1) < 0)
        m_K11 = x2;
    else if((y2 - y1) > 0)
        m_K11 = x1;
    else
        step /= 10;
//     qDebug() << y1 << y2 << error;
    }
        }
        {
            
       qreal old = m_K21;
    qreal step = 0.1;
    
    
    for(int j = 0; j < 5; ++j)
    {
        qreal y1 = 0;
        qreal  y2 = 0;
        qreal x1 = 0;
        qreal x2 = 0;
        qreal error;
        for(int i = 0; i < Size(); ++i)
            error += SumOfErrors(i);
//         qDebug() << "Error " << error;
    x1 = m_K21 + step*m_K21;
    m_K21 = m_K21 + step*m_K21;
    CalculateSignal();
    for(int i = 0; i < Size(); ++i)
        y1 += SumOfErrors(i);
    
    x2 = m_K21 - step*m_K21;
    m_K21 = m_K21 - step*m_K21;
    CalculateSignal();
    for(int i = 0; i < Size(); ++i)
        y2+= SumOfErrors(i);
//     qDebug() << m_K21 << x1 << x2 << "k-werte";
    qreal dydx = (y1-y2)/(x1-x2);
    if((y2 - y1) < 0)
        m_K21 = x2;
    else if((y2 - y1) > 0)
        m_K21 = x1;
    else
        step /= 10;
//     qDebug() << y1 << y2 << error;         
        }
        }
    }
    qDebug() << m_K21 << m_K11;
    m_repaint = true;
    return 0.1;
}



#include "modelclass.moc"
