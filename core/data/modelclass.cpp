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
#include <QStandardItemModel>
#include <QtCharts/QVXYModelMapper>
#include <QApplication>
#include "cmath"
#include "modelclass.h"

AbstractTitrationModel::AbstractTitrationModel(const DataClass *data, QObject *parent) : DataClass(data), QObject(parent), m_repaint(false), m_plotmode(AbstractTitrationModel::GH), m_debug(false)
{
    
    qDebug() << DataPoints() << Size();
    
    ptr_concentrations = data->Concentration();
    for(int i = 0; i < DataPoints(); ++i)
    {
        m_signals[&m_data[i]] = QVector<qreal>(Size(), 0);
        m_difference << QVector<qreal>(Size(), 0);
//         m_signals[&m_data[i]] = 0;//FIXME no contign... signals
    }
    
    m_model_model = new QStandardItemModel(DataPoints(), Size() + 1);
    m_error_model = new QStandardItemModel(DataPoints(), Size() + 1); 
    m_signal_model = new QStandardItemModel(DataPoints(), Size() + 1); 
    for(int i = 0; i < DataPoints(); ++i)    
        for(int j = 0; j <= Size(); ++j)
        {
            QString x = QString::number(XValue(i));
            QStandardItem *item;
            if(j == 0)
                item = new QStandardItem(x);
            else
                item = new QStandardItem(QString::number(0));
           
            m_model_model->setItem(i, j, item);
            if(j == 0)
                item = new QStandardItem(x);
            else
                item = new QStandardItem(QString::number(0));
            m_error_model->setItem(i, j, item);
            
            if(j == 0)
                item = new QStandardItem(x);
            else
                item = new QStandardItem(QString::number(m_data[i].Data()[j - 1]));
             m_signal_model->setItem(i, j, item);  
            
        }
    for(int j = 0; j < Size(); ++j)
    {
        QPointer<QtCharts::QVXYModelMapper> model = new QtCharts::QVXYModelMapper;
        model->setModel(m_model_model);
        model->setXColumn(0);
        model->setYColumn(j + 1);
        m_model_mapper << model;
        
        QPointer<QtCharts::QVXYModelMapper> error = new QtCharts::QVXYModelMapper;
        error->setModel(m_error_model);
        error->setXColumn(0);
        error->setYColumn(j + 1);
        m_error_mapper << error;     
        
        QPointer<QtCharts::QVXYModelMapper> signal = new QtCharts::QVXYModelMapper;
        signal->setModel(m_signal_model);
        signal->setXColumn(0);
        signal->setYColumn(j + 1);
        m_signal_mapper << signal;     
        
    }
    m_pure_signals.resize(Size());   
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        m_pure_signals[i] = m_data.first().Data()[i];
    }

    
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

qreal AbstractTitrationModel::SumOfErrors(int i) const
{
    qreal sum = 0;

    if(i >= Size())
        return sum;
    
    for(int j = 0; j < DataPoints(); ++j)
    {
        sum += qPow(m_difference[j][i],2);
    }
    return sum;
}

qreal AbstractTitrationModel::XValue(int i) const
{

    switch(m_plotmode){
            case AbstractTitrationModel::G:
                if(*ptr_concentrations)
                    return m_data[i].Conc1();
                else
                    return m_data[i].Conc2();
            break;
            case AbstractTitrationModel::H:   
                if(!(*ptr_concentrations))
                    return m_data[i].Conc1();
                else
                    return m_data[i].Conc2();
            case AbstractTitrationModel::HG:
                if(*ptr_concentrations)
                    return m_data[i].Conc1()/m_data[i].Conc2();
                else
                    return m_data[i].Conc2()/ m_data[i].Conc1();                
            break;    
            case AbstractTitrationModel::GH:
                if(!(*ptr_concentrations))
                    return m_data[i].Conc1()/m_data[i].Conc2();
                else
                    return m_data[i].Conc2()/ m_data[i].Conc1();                   
            break;    
        };
        return 0;
}


void AbstractTitrationModel::SetSignal(int i, int j, qreal value)
{
     if(m_debug)
        qDebug() << i << j << value;
        
    m_signals[&m_data[i]][j] = value;
    m_difference[i][j] =  m_signals[&m_data[i]][j] - m_data[i].Data()[j];   
    if(m_repaint)
    {
    QStandardItem *item = m_model_model->item(i, j + 1);
        item->setData(value, Qt::DisplayRole);
        item = m_model_model->item(i, 0);
        item->setData(XValue(i), Qt::DisplayRole);

    QStandardItem *item2 = m_error_model->item(i, j + 1);
        item2->setData(m_difference[i][j], Qt::DisplayRole);
        item2 = m_error_model->item(i, 0);
        item2->setData(XValue(i), Qt::DisplayRole);
    }   
}

qreal AbstractTitrationModel::Minimize(QVector< int > vars)
{
    return 0;
}

QVector< qreal > AbstractTitrationModel::df()
{
    QVector<qreal > derivate;
    
    for(int i = 0; i < m_opt_para.size(); ++i)
    {    
        qreal y1 = 0;
        qreal y2 = 0; 
    
        *m_opt_para[i] +=  + 0.00001;
     
        CalculateSignal();
            
        for(int k = 0; k < Size(); ++k)
            y1 += SumOfErrors(k);
  
        *m_opt_para[i] -=  -  0.00001;

        CalculateSignal();
        for(int k = 0; k < Size(); ++k)    
            y2+= SumOfErrors(k);  
        derivate <<  (y2-y1)/(0.0002);
        
        *m_opt_para[i] +=  + 0.00001;
    }
    
    return derivate;
}


qreal AbstractTitrationModel::MiniSingleConst(QVector<qreal > &steps)
{
    qreal norm = 0;
            QVector<qreal > derviate = df();
//             qDebug() << derviate;
            for(int i = 0; i < derviate.size(); ++i)
            {
                norm += derviate[i]*derviate[i];
                CalculateSignal();
                    qreal old_error = 0;
                    for(int k = 0; k < Size(); ++k)    
                        old_error+= SumOfErrors(k);
//                 qreal error =  qAbs(derviate[i]*0.0002);
                qreal oldval = *m_opt_para[i];
//                 qreal a = 1000*steps[i];
                if(qAbs(derviate[i]) < 1e-12)
                    qDebug() << "ignoring";
                else
                {
//                   qDebug() << "old value " << *m_opt_para[i] << "new value " << *m_opt_para[i] - a*error/derviate[i];// << "1st deriv" << qSqrt(norm) ;
//                   *m_opt_para[i] -= a*error/derviate[i] ;
                    qreal b = 1000;
                    qreal tk = 1/b/steps[i];
                    *m_opt_para[i] -= tk/derviate[i] ;
                    CalculateSignal();
                    qreal new_error = 0;
                    for(int k = 0; k < Size(); ++k)    
                        new_error+= SumOfErrors(k);                  
                    if(qAbs(new_error) > qAbs(old_error))
                    {
                        *m_opt_para[i] = oldval;
                         steps[i] /= 2;
                    }
                }    
            }
            CalculateSignal();
            qreal error = 0;
            for(int k = 0; k < Size(); ++k)    
                error+= SumOfErrors(k);
//              qDebug() << Constants() << error;;
//             qDebug() << qSqrt(norm);
            return qSqrt(norm);

}


qreal AbstractTitrationModel::Minimize()
{
    m_repaint = false;
    QVector<qreal > opt_para;
    QVector<qreal > step = Constants();
    QVector<qreal > errors;
    bool iter = true;
    int i = 0, maxiter = 300;
    while(iter)
    {
//             qreal norm = MiniSingleConst(step); 
            qreal norm = MiniSingleConst(QVector<qreal>() << iter << iter); 
            iter = (i < maxiter && norm > 1e-8); 
//             qDebug() <<  (i < maxiter) << (norm > 1e-8) << step;
            ++i;    
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    }
                
    for(int j = 0; j < m_lim_para[0].size(); ++j)
    {
        if(m_difference[0][j] < 1)
            *m_lim_para[0][j] -= m_difference[0][j];
    } 
    m_repaint = true;
    m_debug = true;
//     CalculateSignal();
    m_debug = false;
   
//     qDebug() << Constants();
//     qDebug() << "baem";
//     CalculateSignal();
    return 0;
}


ItoI_Model::ItoI_Model(const DataClass *data, QObject *parent) : AbstractTitrationModel(data, parent), m_K11(4)
{
    setName(tr("1:1-Model"));
    m_ItoI_signals.resize(m_pure_signals.size());
    
    for(int i = 0; i <  m_ItoI_signals.size(); ++i)
    {
         m_ItoI_signals[i] = m_data.last().Data()[i];
    }
    CalculateSignal();
    
    m_opt_para << &m_K11;
    QVector<qreal * > line1, line2;
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        line1 << &m_pure_signals[i];
        line2 << &m_ItoI_signals[i];
    }
    m_lim_para << line1 << line2;
    
    AbstractTitrationModel::Minimize();
    qDebug() << Constants();
    m_repaint = true;
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
 return AbstractTitrationModel::Minimize();
}

qreal ItoI_Model::HostConcentration(qreal host_0, qreal guest_0, QVector< qreal > constants)
{
    if(constants.size() == 0)
        return host_0;
    qreal K11 = qPow(10, constants.first());
    qreal a, b, c;
    qreal complex;
    a = K11;
    b = -1*(K11*host_0+K11*guest_0+1);
    c = K11*guest_0*host_0;
    complex = MinQuadraticRoot(a,b,c);
    return host_0 - complex;
}

void ItoI_Model::CalculateSignal(QVector<qreal > constants)
{  
    for(int i = 0; i < DataPoints(); ++i)
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
        qreal host = HostConcentration(host_0, guest_0, constants);
        qreal complex = host_0 -host;
        for(int j = 0; j < Size(); ++j)
        {
            qreal value = host/host_0*m_pure_signals[j] + complex/host_0*m_ItoI_signals[j];
            SetSignal(i, j, value);    
        }
    }
    if(m_repaint)
        emit Recalculated();
}

void ItoI_Model::setConstants(QVector< qreal > list)
{
    if(list.isEmpty())
        return;
    m_K11 = list.first();
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
        return QPair<qreal, qreal>(m_K11, m_ItoI_signals[j]);
    }
    return QPair<qreal, qreal>(0, 0);
}


IItoI_ItoI_Model::IItoI_ItoI_Model(const DataClass* data, QObject* parent) : AbstractTitrationModel(data, parent)
{
    
    ItoI_Model *model = new ItoI_Model(data, this);
    m_K11 = model->Constants()[model->ConstantSize() -1];
    m_K21 = m_K11/2;
    delete model;
    qDebug() << Constants();
    setName(tr("2:1/1:1-Model"));
    m_ItoI_signals.resize(m_pure_signals.size());
    m_IItoI_signals.resize(m_pure_signals.size());
    for(int i = 0; i <  m_ItoI_signals.size(); ++i)
    {
         m_ItoI_signals[i] = m_data.last().Data()[i];
         m_IItoI_signals[i] = m_data.first().Data()[i];
    }
    
       
    m_opt_para << &m_K21 << &m_K11;
    QVector<qreal * > line1, line2, line3;
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        line1 << &m_pure_signals[i];
        line2 << &m_ItoI_signals[i];
        line3 << &m_IItoI_signals[i];
    }
    m_lim_para << line1 << line2;
    m_opt_vec << line3;
    CalculateSignal();
    AbstractTitrationModel::Minimize();
        
    m_repaint = true;
    qDebug() << Constants();
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
    m_K21 = list[0];
    m_K11 = list[1];   
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

    if(constants.size() == 0)
        constants = Constants();
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
            
        qreal K21= qPow(10, constants.first());
        qreal K11 = qPow(10, constants.last());
        qreal host = HostConcentration(host_0, guest_0, constants);
        qreal guest = guest_0/(K11*host+K11*K21*host*host+1);
        qreal complex_11 = K11*host*guest;
        qreal complex_21 = K11*K21*host*host*guest;
        

        for(int j = 0; j < Size(); ++j)
            {
                qreal value = host/host_0*m_pure_signals[j] + complex_11/host_0*m_ItoI_signals[j]+ 2*complex_21/host_0*m_IItoI_signals[j];
                SetSignal(i, j, value);
            }
  
    }
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
    return  AbstractTitrationModel::Minimize();
}


ItoI_ItoII_Model::ItoI_ItoII_Model(const DataClass* data, QObject* parent) : AbstractTitrationModel(data, parent)
{
    
    ItoI_Model *model = new ItoI_Model(data, this);
    m_K12 = model->Constants()[model->ConstantSize() -1];
    m_K11 = m_K12/2;
    delete model;
    qDebug() << Constants();
    setName(tr("1:1/1:2-Model"));
    m_ItoI_signals.resize(m_pure_signals.size());
    m_ItoII_signals.resize(m_pure_signals.size());
    for(int i = 0; i <  m_ItoI_signals.size(); ++i)
    {
         m_ItoI_signals[i] = (m_data.first().Data()[i] + m_data.last().Data()[i])/2;
         m_ItoII_signals[i] = m_data.last().Data()[i];
    }
    
       
    m_opt_para << &m_K11 << &m_K12;
    QVector<qreal * > line1, line2, line3;
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        line1 << &m_pure_signals[i];
        line2 << &m_ItoI_signals[i];
        line3 << &m_ItoII_signals[i];
    }
    m_lim_para << line1 << line2;
    m_opt_vec << line3;
    CalculateSignal();

    AbstractTitrationModel::Minimize();
        
    m_repaint = true;
//     qDebug() << Constants();
}
ItoI_ItoII_Model::~ItoI_ItoII_Model()
{

}

void ItoI_ItoII_Model::setComplexSignals(QVector< qreal > list, int i)
{
    if(i == 1)
        m_ItoII_signals = list;
    if(i == 2)
        m_ItoI_signals = list;
}

void ItoI_ItoII_Model::setConstants(QVector< qreal > list)
{
     if(list.size() < 2)
        return;
    m_K12 = list[1];
    m_K11 = list[0];   
}

qreal ItoI_ItoII_Model::HostConcentration(qreal host_0, qreal guest_0, QVector<qreal > constants)
{
    
    if(constants.size() < 2)
        return host_0;
    
    qreal K12= qPow(10, constants.last());
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
    
    qreal K12= qPow(10, constants.last());
    qreal K11 = qPow(10, constants.first());
//     qreal a , b, c;
    qreal a = K11*K12;
    qreal b = K11*(2*K12*host_0-K12*guest_0+1);
    qreal c = K11*(host_0-guest_0)+1;
//     qDebug() << a << b << c;
    qreal guest = MinCubicRoot(a,b,c, -guest_0);
    return guest;
}

void ItoI_ItoII_Model::CalculateSignal(QVector<qreal > constants)
{
    if(constants.size() == 0)
        constants = Constants();
   
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
            
        qreal K12= qPow(10, constants.last());
        qreal K11 = qPow(10, constants.first());
        
        qreal host = HostConcentration(host_0, guest_0, constants);
        qreal guest = GuestConcentration(host_0, guest_0, constants);
        qreal complex_11 = K11*host*guest;
        qreal complex_12 = K11*K12*host*guest*guest;
//         qDebug() << host_0 << guest_0 << host << guest << complex_11 << complex_12;

        for(int j = 0; j < Size(); ++j)
            {
                qreal value = host/host_0*m_pure_signals[j] + complex_11/host_0*m_ItoI_signals[j]+ complex_12/host_0*m_ItoII_signals[j];
                SetSignal(i, j, value);
            }
  
    }
    if(m_repaint)
        emit Recalculated();
}

void ItoI_ItoII_Model::setPureSignals(QVector< qreal > list)
{
    m_pure_signals = list;
}


QPair< qreal, qreal > ItoI_ItoII_Model::Pair(int i, int j)
{
        if(i == 0)
        {
            

        if(j < m_ItoII_signals.size()) 
        {
            return QPair<qreal, qreal>(Constants()[i], m_ItoII_signals[j]);
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


qreal ItoI_ItoII_Model::Minimize(QVector< int > vars)
{
    return  AbstractTitrationModel::Minimize();
}


#include "modelclass.moc"
