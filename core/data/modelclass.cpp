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

AbstractTitrationModel::AbstractTitrationModel(const DataClass *data, QObject *parent) : DataClass(data), m_repaint(false), m_debug(false)
{
    
    qDebug() << DataPoints() << Size();
    m_active_signals = QVector<int>(SignalCount(), 1);
    ptr_concentrations = data->Concentration();
//     for(int i = 0; i < DataPoints(); ++i)
//     {
        m_model_signal = new DataTable(SignalCount(),DataPoints());
        m_model_error = new DataTable(SignalCount(),DataPoints());
//         m_signals[&m_data[i]] = 0;//FIXME no contign... signals
//     }
    
    m_plot_model = new QStandardItemModel(DataPoints(), SignalCount()+1);
    m_plot_error = new QStandardItemModel(DataPoints(), SignalCount()+1);
    for(int j = 0; j < SignalCount(); ++j)
    {
            QPointer<QtCharts::QVXYModelMapper> model = new QtCharts::QVXYModelMapper;
            model->setModel(m_plot_model);
            model->setXColumn(0);
            model->setYColumn(j + 1);
            m_model_mapper << model;
            
            QPointer<QtCharts::QVXYModelMapper> error = new QtCharts::QVXYModelMapper;
            error->setModel(m_plot_error);
            error->setXColumn(0);
            error->setYColumn(j + 1);
            m_error_mapper << error;     
    }

        m_pure_signals = m_signal_model->firstRow();
}

AbstractTitrationModel::~AbstractTitrationModel()
{
    for(int i = 0; i < m_model_mapper.size(); ++i)
    {
     delete m_model_mapper[i]->series();
     delete m_error_mapper[i]->series();
    }
    qDeleteAll( m_model_mapper );
    qDeleteAll( m_error_mapper );
//     qDeleteAll( m_signal_mapper );
//     qDeleteAll( m_opt_vec );
//     qDeleteAll( m_lim_para );
}

QVector<double>   AbstractTitrationModel::getCalculatedSignals(QVector<int > active_signal)
{
    if(active_signal.size() < SignalCount() && m_active_signals.size() < SignalCount())
        active_signal = QVector<int>(SignalCount(), 1);
    else
        active_signal = m_active_signals;
    QVector<double> x(DataPoints()*SignalCount(), 0);
    int index = 0;
        for(int j = 0; j < SignalCount(); ++j)
    {
        for(int i = 0; i < DataPoints(); ++i)
        {
            if(active_signal[j] == 1)
                x[index] = m_model_signal->data(j,i); 
            index++;
        }
    }
    return x;
}
void AbstractTitrationModel::setOptParamater(QVector<qreal> &parameter)
{
    clearOptParameter();
    for(int i = 0; i < parameter.size(); ++i)
        m_opt_para << &parameter[i];
}

void AbstractTitrationModel::setOptParamater(qreal& parameter)
{   
    clearOptParameter();
    m_opt_para << &parameter;
}


void AbstractTitrationModel::addOptParameter(QVector<qreal>& parameter)
{
    for(int i = 0; i < parameter.size(); ++i)
        m_opt_para << &parameter[i];    
}
void AbstractTitrationModel::addOptParameter(qreal& value)
{
    m_opt_para << & value;
}

void AbstractTitrationModel::clearOptParameter()
{
    m_opt_para.clear();
}


qreal AbstractTitrationModel::SumOfErrors(int i) const
{
    qreal sum = 0;

    if(i >= Size() || i >= m_active_signals.size())
        return sum;
    
    if(m_active_signals[i] == 0)
        return sum;
    for(int j = 0; j < DataPoints(); ++j)
    {
        sum += qPow(m_model_error->data(i,j),2);
    }
    return sum;
}

void AbstractTitrationModel::SetSignal(int i, int j, qreal value)
{

    if(m_debug)
        qDebug() << i << j << value;
        
    if(Type() != 3)
    {
        m_model_signal->data(j,i) = value;
        m_model_error->data(j,i) = m_model_signal->data(j,i) - m_signal_model->data(j,i);
    }

}

void AbstractTitrationModel::UpdatePlotModels()
{
    qDebug() << m_model_mapper.size() << m_error_mapper.size() << m_plot_model->rowCount() << m_plot_model->columnCount();
    QStandardItem *item;
    
    for(int i = 0; i < DataPoints(); ++i)
     {
         QString x = QString::number(XValue(i));
         item = new QStandardItem(x);
         m_plot_model->setItem(i, 0, item);
         item = new QStandardItem(x);
         m_plot_error->setItem(i, 0, item);
            for(int j = 0; j < SignalCount(); ++j)
            {
                item = new QStandardItem(QString::number(m_model_signal->data(j,i)));
                m_plot_model->setItem(i, j+1, item);
                item = new QStandardItem(QString::number(m_model_error->data(j,i)));
                m_plot_error->setItem(i, j+1, item);
            }
    }
}

QVector<double> AbstractTitrationModel::Parameter() const
{
    QVector<double > parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void AbstractTitrationModel::setParamter(const QVector<qreal>& parameter)
{
    if(parameter.size() != m_opt_para.size())
        return;
    for(int i = 0; i < parameter.size(); ++i)
        *m_opt_para[i] = parameter[i];
}


QVector<qreal> AbstractTitrationModel::Minimize(QVector< int > vars)
{
    return QVector<qreal>();
}


QVector<qreal> AbstractTitrationModel::Minimize(int max)
{ 
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_repaint = false;
    QVector<qreal> constants = Constants();
    qDebug() << constants;
    for(int i = 0; i < max; ++i)
    {
        QApplication::processEvents();
        QVector<qreal > old_cons = constants;
        MinimizingComplexConstants(this, 100, constants);
        MiniShifts(); 
        if(old_cons == constants)
        {
            emit Message("***Finished after " + QString::number(i) + " cycles.***");
            break;
        }
    } 
    emit Message("***Finished after " + QString::number(max) + " cycles.***");
    setConstants(constants);

    QString message = "Using Signals";
    qreal error = 0;
    for(int i = 0; i < m_active_signals.size(); ++i)
        if(m_active_signals[i])
        {
            message += " " +QString::number(i + 1) +" ";
            error += SumOfErrors(i);
        }
    message += "got results: ";
        for(int i = 0; i < Constants().size(); ++i)
            message += "Constant "+ QString(i)+ " " +QString::number(Constants()[i]) +" ";
    message += "Sum of Error is " + QString::number(error);
    message += "\n";
    Message(message);
    m_repaint = true;
    CalculateSignal();

    QApplication::restoreOverrideCursor();
    return constants;
}

ItoI_Model::ItoI_Model(const DataClass *data, QObject *parent) : AbstractTitrationModel(data, parent), m_K11(4)
{
    setName(tr("1:1-Model"));
    
    m_ItoI_signals = m_signal_model->lastRow();
    CalculateSignal();

    setOptParamater(m_K11);
//     m_opt_para << &m_K11;
    QVector<qreal * > line1, line2;
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
//         m_opt_para << &m_pure_signals[i]<< &m_ItoI_signals[i];
        line1 << &m_pure_signals[i];
        line2 << &m_ItoI_signals[i];
    }
    m_lim_para << line1 << line2;
    qDebug() << m_lim_para << m_lim_para.size();
    for(int i = 0; i < m_lim_para.size(); ++i)
        for(int j = 0; j < m_lim_para[i].size(); ++j)
        qDebug() << *m_lim_para[i][j] << m_lim_para[i][j];
    AbstractTitrationModel::Minimize(20);
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

void ItoI_Model::MiniShifts()
{
    QVector<int > active_signal;
    if(m_active_signals.size() < SignalCount())
        active_signal = QVector<int>(SignalCount(), 1);
    else
        active_signal = m_active_signals;
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
        }
}

QVector<QVector<qreal> > ItoI_Model::AllShifts()
{
    QVector<QVector<qreal> > Shifts;
    Shifts << m_pure_signals;
    Shifts << m_ItoI_signals;
    return Shifts;
}


// qreal ItoI_Model::Minimize(QVector<int > vars)
// {
//  return AbstractTitrationModel::Minimize();
// }

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
    qDebug() << "constants" << constants;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal host_0, guest_0;
        if(*ptr_concentrations)
        {
            host_0 = m_concentration_model->data(0,i);
            guest_0 = m_concentration_model->data(1,i);
        }else
        {
            host_0 = m_concentration_model->data(1,i);
            guest_0 = m_concentration_model->data(0,i);
        }
        qreal host = HostConcentration(host_0, guest_0, constants);
        qreal complex = host_0 -host;
        for(int j = 0; j < SignalCount(); ++j)
        {
            qreal value = host/host_0*m_pure_signals[j] + complex/host_0*m_ItoI_signals[j];
            SetSignal(i, j, value);    
        }
    }
    
    if(m_repaint)
    {
        UpdatePlotModels();
        emit Recalculated();
    }
}

void ItoI_Model::setConstants(QVector< qreal > list)
{
    if(list.isEmpty())
        return;
    m_K11 = list.first();
}

void ItoI_Model::setPureSignals(const QVector< qreal > &list)
{
    for(int i = 0; i < list.size(); ++i)
        if(i < m_pure_signals.size())
            m_pure_signals[i] = list[i];
        else if(Type() == 3)
            m_pure_signals<<list[i];
}

void ItoI_Model::setComplexSignals(QVector< qreal > list, int i)
{
    if(list.size() << m_ItoI_signals.size())
    {
        for(int i = 0; i < list.size(); ++i)
            m_ItoI_signals[i] = list[i];
    }
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
    m_complex_constants << m_K21 << m_K11;
    qDebug() << Constants();
    setName(tr("2:1/1:1-Model"));

    for(int i = 0; i < SignalCount(); ++i)
    {
         m_ItoI_signals <<m_signal_model->lastRow()[i];
         m_IItoI_signals << m_signal_model->firstRow()[i];
    }
       
    setOptParamater(m_complex_constants);
    QVector<qreal * > line1, line2, line3;
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        line1 << &m_pure_signals[i];
        line2 << &m_ItoI_signals[i];
        line3 << &m_IItoI_signals[i];
    }
    m_lim_para << line1 << line2;
    m_opt_vec << line3;
//     m_opt_para << line3;
    CalculateSignal();
    AbstractTitrationModel::Minimize(20);
        
    m_repaint = true;
    qDebug() << Constants();
}
IItoI_ItoI_Model::~IItoI_ItoI_Model()
{

}

void IItoI_ItoI_Model::MiniShifts()
{
    QVector<int > active_signal;
    if(m_active_signals.size() < SignalCount())
        active_signal = QVector<int>(SignalCount(), 1);
    else
        active_signal = m_active_signals;
    
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
            
        }
        
        QVector<qreal > parameter = m_IItoI_signals;
        clearOptParameter();
        setOptParamater(m_IItoI_signals);
        qDebug() << parameter;
        MinimizingComplexConstants(this, 100, parameter);
        setComplexSignals(parameter, 1);
        
//         m_opt_para << &m_K21 << &m_K11;
        setOptParamater(m_complex_constants);
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
        
    if(i == 1 && j < m_IItoI_signals.size())
        m_IItoI_signals[j] = list[j];
    if(i == 2&& j < m_ItoI_signals.size())
        m_ItoI_signals[j] = list[j];
    }
}

void IItoI_ItoI_Model::setConstants(QVector< qreal > list)
{
     if(list.size() != m_complex_constants.size())
        return;
     for(int i = 0; i < list.size(); ++i)
         m_complex_constants[i] = list[i];
//     m_K21 = list[0];
//     m_K11 = list[1];   
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
    for(int i = 0; i < DataPoints(); ++i)
    {
         qreal host_0, guest_0;
        if(*ptr_concentrations)
        {
            host_0 = m_concentration_model->data(0,i);
            guest_0 = m_concentration_model->data(1,i);
        }else
        {
            host_0 = m_concentration_model->data(1,i);
            guest_0 = m_concentration_model->data(0,i);
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
    if(m_repaint)
    {
        UpdatePlotModels();
        emit Recalculated();
    }
}

void IItoI_ItoI_Model::setPureSignals(const QVector< qreal > &list)
{
    for(int i = 0; i < list.size(); ++i)
        if(i < m_pure_signals.size())
            m_pure_signals[i] = list[i];
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


QVector<qreal> IItoI_ItoI_Model::Minimize(QVector< int > vars)
{
    QVector<qreal> zahl = AbstractTitrationModel::Minimize(20);
//     qDebug() << m_K11 << m_K21;
    return  zahl;
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
    for(int i = 0; i < SignalCount(); ++i)
    {
         m_ItoI_signals[i] = ( m_signal_model->data(i,0) +  m_signal_model->data(i,SignalCount() - 1))/2;
         m_ItoII_signals[i] = m_signal_model->data(i,SignalCount() - 1);
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
    m_opt_para <<line3;
    CalculateSignal();

    AbstractTitrationModel::Minimize(20);
        
    m_repaint = true;
//     qDebug() << Constants();
}
ItoI_ItoII_Model::~ItoI_ItoII_Model()
{

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
    if(m_active_signals.size() < SignalCount())
        active_signal = QVector<int>(SignalCount(), 1);
    else
        active_signal = m_active_signals;
    
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
        }
}


void ItoI_ItoII_Model::setComplexSignals(QVector< qreal > list, int i)
{
    for(int j = 0; j < list.size(); ++j)
    {
        
    if(i == 2 && j < m_ItoII_signals.size())
        m_ItoII_signals[j] = list[j];
    if(i == 1 && j < m_ItoI_signals.size())
        m_ItoI_signals[j] = list[j];
    }
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
    if(constants.size() == 0)
        constants = Constants();
   
    for(int i = 0; i < DataPoints(); ++i)
    {
         qreal host_0, guest_0;
        if(*ptr_concentrations)
        {
            host_0 = m_concentration_model->data(0,i);
            guest_0 = m_concentration_model->data(1,i);
        }else
        {
            host_0 = m_concentration_model->data(1,i);
            guest_0 = m_concentration_model->data(0,i);
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


QVector<qreal> ItoI_ItoII_Model::Minimize(QVector< int > vars)
{
    return  AbstractTitrationModel::Minimize(20);
}
#include "modelclass.moc"
