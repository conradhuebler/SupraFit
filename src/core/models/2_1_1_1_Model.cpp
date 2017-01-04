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



#include "src/core/models.h"
#include "src/core/libmath.h"
#include <QtMath>

#include <QDebug>
#include <QtCore/QDateTime>
#include <QStandardItemModel>
#include <QtCharts/QVXYModelMapper>
#include <QApplication>
#include <cmath>
#include <cfloat>
#include <iostream>
#include "2_1_1_1_Model.h"

ItoI_ItoII_Model::ItoI_ItoII_Model(const DataClass* data) : AbstractTitrationModel(data)
{
    
    
    setName(tr("1:1/1:2-Model"));
    InitialGuess();
    setOptParamater(m_complex_constants);
    CalculateSignal();
    m_repaint = true;
//     qDebug() << Constants();
}
ItoI_ItoII_Model::~ItoI_ItoII_Model()
{

}


void ItoI_ItoII_Model::InitialGuess()
{
    m_repaint = false;
    
    ItoI_Model *model = new ItoI_Model(m_data);
    m_K12 = model->Constants()[model->ConstantSize() -1];
    m_K11 = m_K12/2;
    delete model;
        
    m_complex_constants = QVector<qreal>() << m_K11 << m_K12;
    setOptParamater(m_complex_constants);
    m_ItoI_signals.resize(m_pure_signals.size());
    m_ItoII_signals.resize(m_pure_signals.size());
    for(int i = 0; i < SignalCount(); ++i)
    {
         m_ItoI_signals[i] = ( SignalModel()->data(i,0) +  SignalModel()->data(i,SignalCount() - 1))/2;
         m_ItoII_signals[i] = SignalModel()->data(i,SignalCount() - 1);
    }
        
//     m_opt_para = QVector<double * >() << &m_K11 << &m_K12;
    QVector<qreal * > line1, line2, line3;
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        line1 << &m_pure_signals[i];
        line2 << &m_ItoI_signals[i];
        line3 << &m_ItoII_signals[i];
    }
    m_lim_para = QVector<QVector<qreal * > >() << line1 << line2;
    m_opt_vec = QVector<QVector<qreal * > >() << line3;
//     m_opt_para << line3;
    
    CalculateSignal();
       
    m_repaint = true;
}

QVector<QVector<qreal> > ItoI_ItoII_Model::AllShifts()
{
    
        QVector<QVector<qreal> > Shifts;
    Shifts << m_pure_signals;
    Shifts << m_ItoI_signals;
    Shifts << m_ItoII_signals;
    return Shifts;
    
}

void ItoI_ItoII_Model::MiniShifts()
{
    QVector<int > active_signal;
    if(ActiveSignals().size() < SignalCount())
        active_signal = QVector<int>(SignalCount(), 1);
    else
        active_signal = ActiveSignals();
    
    QVector<qreal > parameter = m_ItoI_signals;
    clearOptParameter();
    setOptParamater(m_ItoI_signals);
    
//     for(int iter = 0; iter < 100; ++iter)
//     {
        QVector<qreal > signal_0, signal_1;
        signal_0 = m_model_error->firstRow();
        signal_1 = m_model_error->lastRow();
        for(int j = 0; j < m_lim_para.size(); ++j)
        {
            for(int i = 0; i < SignalCount(); ++i)    
            {
                if(active_signal[i] == 1)
                {
                    if(m_model_error->firstRow()[i] < 1 && j == 0)
                        *m_lim_para[j][i] -= m_model_error->firstRow()[i];
                    if(m_model_error->lastRow()[i] < 1 && j == 1)
                        *m_lim_para[j][i] -= m_model_error->lastRow()[i];
                }
            }
        }
        
//         MinimizingComplexConstants(this, m_opt_config.LevMar_Shifts_PerIter, parameter, m_opt_config);
//     }
    setComplexSignals(parameter, 1);
    setOptParamater(m_complex_constants);
}


void ItoI_ItoII_Model::setComplexSignals(QVector< qreal > list, int i)
{
    for(int j = 0; j < list.size(); ++j)
    {
    if(i == 1 && j < m_ItoI_signals.size())
        m_ItoI_signals[j] = list[j];        
    if(i == 2 && j < m_ItoII_signals.size())
        m_ItoII_signals[j] = list[j];
    }
}

void ItoI_ItoII_Model::setConstants(QVector< qreal > list)
{
     if(list.size() != m_complex_constants.size())
        return;
     for(int i = 0; i < list.size(); ++i)
         m_complex_constants[i] = list[i];
}

qreal ItoI_ItoII_Model::HostConcentration(qreal host_0, qreal guest_0, QVector<qreal > constants)
{
    
    if(constants.size() < 2)
        return host_0;
    
    qreal K12 = qPow(10, constants.last());
    qreal K11 = qPow(10, constants.first());    
    qreal guest = GuestConcentration(host_0, guest_0, constants);
    qreal host;
    host = host_0/(K11*guest+K11*K12*guest*guest+1);
    return host;
}

qreal ItoI_ItoII_Model::GuestConcentration(qreal host_0, qreal guest_0, QVector< qreal > constants)
{
        
    if(constants.size() < 2)
        return guest_0;
    
    qreal K12 = qPow(10, constants.last());
    qreal K11 = qPow(10, constants.first());
    qreal a = K11*K12;
    qreal b = K11*(2*K12*host_0-K12*guest_0+1);
    qreal c = K11*(host_0-guest_0)+1;
    qreal guest = MinCubicRoot(a,b,c, -guest_0);
    return guest;
}

void ItoI_ItoII_Model::CalculateSignal(QVector<qreal > constants)
{
    m_corrupt = false;
    if(constants.size() == 0)
        constants = Constants();
   qDebug() << constants << Constants();
    for(int i = 0; i < DataPoints(); ++i)
    {
         qreal host_0, guest_0;
        if(*ptr_concentrations)
        {
            host_0 = ConcentrationModel()->data(0,i);
            guest_0 = ConcentrationModel()->data(1,i);
        }else
        {
            host_0 = ConcentrationModel()->data(1,i);
            guest_0 = ConcentrationModel()->data(0,i);
        }
            
        qreal K12= qPow(10, constants.last());
        qreal K11 = qPow(10, constants.first());
        
        qreal host = HostConcentration(host_0, guest_0, constants);
        qreal guest = GuestConcentration(host_0, guest_0, constants);
        qreal complex_11 = K11*host*guest;
        qreal complex_12 = K11*K12*host*guest*guest;
//         qDebug() << host_0 << guest_0 << host << guest << complex_11 << complex_12;

        for(int j = 0; j < SignalCount(); ++j)
            {
                qreal value = host/host_0*m_pure_signals[j] + complex_11/host_0*m_ItoI_signals[j]+ complex_12/host_0*m_ItoII_signals[j];
                SetSignal(i, j, value);
            }
  
    }
    if(m_repaint)
    {
          UpdatePlotModels();
          emit Recalculated();
    }
}

void ItoI_ItoII_Model::setPureSignals(const QVector< qreal > &list)
{
    for(int i = 0; i < list.size(); ++i)
        if(i < m_pure_signals.size())
            {
                qDebug() << m_pure_signals[i] << list[i];
                m_pure_signals[i] = list[i];
            }
}


QPair< qreal, qreal > ItoI_ItoII_Model::Pair(int i, int j) const
{
    if(i == 0)
    {
        if(j < m_ItoI_signals.size()) 
        {
            return QPair<qreal, qreal>(Constants()[i], m_ItoI_signals[j]);
        }          
    }
    else if(i == 1)
    {
        
        
        if(j < m_ItoII_signals.size()) 
        {
            return QPair<qreal, qreal>(Constants()[i], m_ItoII_signals[j]);
        } 
        
    }
    return QPair<qreal, qreal>(0, 0);
}
#include "2_1_1_1_Model.moc"
