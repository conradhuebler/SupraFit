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
#include "1_1_1_2_Model.h"

IItoI_ItoI_Model::IItoI_ItoI_Model(const DataClass* data) : AbstractTitrationModel(data)
{
    
   
    setName(tr("2:1/1:1-Model"));


    InitialGuess();   
    
    setOptParamater(m_complex_constants);
    
//     m_opt_para << line3;
    CalculateSignal();

    m_repaint = true;
    qDebug() << Constants();
}
IItoI_ItoI_Model::~IItoI_ItoI_Model()
{

}

void IItoI_ItoI_Model::InitialGuess()
{
    m_repaint = false;
    
    ItoI_Model *model = new ItoI_Model(m_data);
    m_K11 = model->Constants()[model->ConstantSize() -1];
    m_K21 = m_K11/2;
    delete model;
    
    m_complex_constants = QVector<qreal>() << m_K21 << m_K11;
    setOptParamater(m_complex_constants);
    for(int i = 0; i < SignalCount(); ++i)
    {
         m_ItoI_signals <<SignalModel()->lastRow()[i];
         m_IItoI_signals << SignalModel()->firstRow()[i];
    }
    
    QVector<qreal * > line1, line2, line3;
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        line1 << &m_pure_signals[i];
        line2 << &m_ItoI_signals[i];
        line3 << &m_IItoI_signals[i];
    }
    m_lim_para = QVector<QVector<qreal * > >() << line1 << line2;
    m_opt_vec = QVector<QVector<qreal * > >()  << line3;
    
    CalculateSignal();
    m_repaint = true;
}


void IItoI_ItoI_Model::MiniShifts()
{
    QVector<int > active_signal;
    if(ActiveSignals().size() < SignalCount())
        active_signal = QVector<int>(SignalCount(), 1);
    else
        active_signal = ActiveSignals();
    QVector<qreal > parameter = m_IItoI_signals;
    clearOptParameter();
    setOptParamater(m_IItoI_signals);
    //     for(int iter = 0; iter < m_opt_config.LevMar_Shifts_PerIter; ++iter)
    //     {
    QVector<qreal >signal_0, signal_1;
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
        
        //         }
        
        
    }
    //
    setComplexSignals(parameter, 0);
//     setOptParamater(m_complex_constants);
}

QVector<QVector<qreal> > IItoI_ItoI_Model::AllShifts()
{
    
        QVector<QVector<qreal> > Shifts;
    Shifts << m_pure_signals;
    Shifts << m_ItoI_signals;
    Shifts << m_IItoI_signals;
    return Shifts;
    
}

void IItoI_ItoI_Model::setComplexSignals(QVector< qreal > list, int i)
{
    for(int j = 0; j < list.size(); ++j)
    {
        
    if(i == 0 && j < m_IItoI_signals.size())
        m_IItoI_signals[j] = list[j];
    if(i == 1 && j < m_ItoI_signals.size())
        m_ItoI_signals[j] = list[j];
    }
}

void IItoI_ItoI_Model::setConstants(QVector< qreal > list)
{
     if(list.size() != m_complex_constants.size())
        return;
     for(int i = 0; i < list.size(); ++i)
         m_complex_constants[i] = list[i];
}

qreal IItoI_ItoI_Model::HostConcentration(qreal host_0, qreal guest_0, QVector<qreal > constants)
{
    
    if(constants.size() < 2)
        return host_0;
    qreal K21= qPow(10, constants.first());
    qreal K11 = qPow(10, constants.last());
    qreal host;
    qreal a, b, c;
    a = K11*K21;
    b = K11*(2*K21*guest_0-K21*host_0+1);
    c = K11*(guest_0-host_0)+1;
    host = MinCubicRoot(a,b,c, -host_0);
    return host;
}

void IItoI_ItoI_Model::CalculateSignal(QVector<qreal > constants)
{
    m_corrupt = false;
    if(constants.size() == 0)
        constants = Constants();
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
            
        qreal K21= qPow(10, constants.first());
        qreal K11 = qPow(10, constants.last());
        qreal host = HostConcentration(host_0, guest_0, constants);
        qreal guest = guest_0/(K11*host+K11*K21*host*host+1);
        qreal complex_11 = K11*host*guest;
        qreal complex_21 = K11*K21*host*host*guest;
        

        for(int j = 0; j < SignalCount(); ++j)
            {
                qreal value = host/host_0*m_pure_signals[j] + complex_11/host_0*m_ItoI_signals[j]+ 2*complex_21/host_0*m_IItoI_signals[j];
                SetSignal(i, j, value);
            }
  
    }
//     if(m_repaint)
    {
//         UpdatePlotModels();
        std::cout << "vier-eins" << std::endl;
         emit Recalculated();
    }
}

void IItoI_ItoI_Model::setPureSignals(const QVector< qreal > &list)
{
    for(int i = 0; i < list.size(); ++i)
        if(i < m_pure_signals.size())
            m_pure_signals[i] = list[i];
}


QPair< qreal, qreal > IItoI_ItoI_Model::Pair(int i, int j) const
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



void test_II_ItoI_Model::CalculateSignal(QVector<qreal> constants)
{
    // Highly experimental stuff
    
      m_corrupt = false;
    if(constants.size() == 0)
        constants = Constants();
     QVector<qreal> host_0, guest_0;
    for(int i = 0; i < DataPoints(); ++i)
    {
        
        if(*ptr_concentrations)
        {
            host_0 << ConcentrationModel()->data(0,i);
            guest_0 << ConcentrationModel()->data(1,i);
        }else
        {
            host_0 << ConcentrationModel()->data(1,i);
            guest_0 << ConcentrationModel()->data(0,i);
        }
    }
    
        qreal K21= qPow(10, constants.first());
        qreal K11 = qPow(10, constants.last());
        QVector<double > A_equ, B_equ;

//         SolveEqualSystem(host_0, guest_0, K11, K11*K21, A_equ, B_equ);
        
//         qreal host = HostConcentration(host_0, guest_0, constants);
//         qreal guest = guest_0/(K11*host+K11*K21*host*host+1);
        
        
        for(int i = 0; i  < DataPoints(); ++i)
        {
        
            qreal host = A_equ[i];
            qreal guest = B_equ[i]; //concentration.last();
            qreal complex_11 = K11*host*guest;
            qreal complex_21 = K11*K21*host*host*guest;
        
        for(int j = 0; j < SignalCount(); ++j)
            {
                qreal value = host/host_0[i]*m_pure_signals[j] + complex_11/host_0[i]*m_ItoI_signals[j]+ 2*complex_21/host_0[i]*m_IItoI_signals[j];
                SetSignal(i, j, value);
            }
        }
    
    if(m_repaint)
    {
        UpdatePlotModels();
        emit Recalculated();
    }  
}

QSharedPointer<AbstractTitrationModel > IItoI_ItoI_Model::Clone() const
{
    QSharedPointer<IItoI_ItoI_Model > model = QSharedPointer<IItoI_ItoI_Model>(new IItoI_ItoI_Model(this), &QObject::deleteLater);
    return model;
    
}

#include "1_1_1_2_Model.moc"
