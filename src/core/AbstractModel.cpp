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




#include "libmath.h"
#include "src/core/dataclass.h"
#include "src/core/toolset.h"

#include <QtMath>

#include <QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QDateTime>
#include <QStandardItemModel>
#include <QtCharts/QVXYModelMapper>
#include <QApplication>
#include <cmath>
#include <cfloat>
#include <iostream>
#include "AbstractModel.h"

AbstractTitrationModel::AbstractTitrationModel(const DataClass *data) : DataClass(data),  m_repaint(false), m_debug(false), m_inform_config_changed(true), m_corrupt(false), m_pending(false)
{
    m_constant_names << tr("no constants");
    //     m_active_signals = 
    setActiveSignals(QVector<int>(SignalCount(), 1));
    ptr_concentrations = data->Concentration();
    //     for(int i = 0; i < DataPoints(); ++i)
    //     {
    m_model_signal = new DataTable(SignalCount(),DataPoints());
    m_model_error = new DataTable(SignalCount(),DataPoints());
    //         m_signals[&m_data[i]] = 0;//FIXME no contign... signals
    //     }
    
    m_plot_model = new QStandardItemModel(DataPoints(), SignalCount()+1);
    
    m_plot_error = new QStandardItemModel(DataPoints(), SignalCount()+1);
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
    
    m_pure_signals = SignalModel()->firstRow();
    m_data = data;
    connect(m_data, SIGNAL(recalculate()), this, SLOT(CalculateSignal()));
    
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

void AbstractTitrationModel::adress() const
{
    std::cout << "We are at " << this;
    std::cout << "\t" << m_data;
    std::cout << "\t "<< m_data->Concentration();
    std::cout << "\t" << Concentration() << std::endl;
}


QVector<double>   AbstractTitrationModel::getCalculatedSignals(QVector<int > active_signal)
{
    if(active_signal.size() < SignalCount() && ActiveSignals().size() < SignalCount())
        active_signal = QVector<int>(SignalCount(), 1);
    else
        active_signal = ActiveSignals();
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

QVector<qreal> AbstractTitrationModel::OptimizeParameters(OptimizationType type)
{
    clearOptParameter();
    QVector<qreal > variables =  OptimizeParameters_Private(type);
    m_locked_parameters = QVector<int>(variables.size(), 1);
    return variables;
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
    
    if(i >= Size() || i >= ActiveSignals().size())
        return sum;
    
    if(ActiveSignals()[i] == 0)
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
    if(std::isnan(value) || std::isinf(value))
    {
        value = 0;
        m_corrupt = true;
    }
    if(Type() != 3)
    {
        m_model_signal->data(j,i) = value;
        m_model_error->data(j,i) = m_model_signal->data(j,i) - SignalModel()->data(j,i); //var;
    }
    
}

void AbstractTitrationModel::UpdatePlotModels()
{
    if(m_pending)
        return;
    m_pending = true;
    for(int i = 0; i < DataPoints(); ++i)
    {
        QString x = QString::number(XValue(i));
        m_plot_model->item(i, 0)->setData(x, Qt::DisplayRole);
        m_plot_error->item(i, 0)->setData(x, Qt::DisplayRole);
        for(int j = 0; j < SignalCount(); ++j)
        {
            m_plot_model->item(i, j+1)->setData(QString::number(m_model_signal->data(j,i)), Qt::DisplayRole);
            m_plot_error->item(i, j+1)->setData(QString::number(m_model_error->data(j,i)), Qt::DisplayRole);
        }
    }
    m_pending = false;
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
        if(m_locked_parameters[i])
            *m_opt_para[i] = parameter[i];
}



QJsonObject AbstractTitrationModel::ExportJSON() const
{
    
    QJsonObject json, toplevel;
    QJsonObject constantObject;
    for(int i = 0; i < Constants().size(); ++i)
        constantObject[QString::number(i)] = (QString::number(Constants()[i]));
    
    json["constants"] = constantObject;
    
    QJsonObject pureShiftObject;
    for(int i = 0; i < m_pure_signals.size(); ++i)
        if(ActiveSignals()[i])
            pureShiftObject[QString::number(i)] = (QString::number(m_pure_signals[i]));
        
        json["pureShift"] = pureShiftObject;   
    
    
    for(int i = 0; i < Constants().size(); ++i)
    {
        
        QJsonObject object;
        for(int j = 0; j < m_pure_signals.size(); ++j)
        {
            if(ActiveSignals()[j])
            {
                qreal value = Pair(i, j).second;
                object[QString::number(j)] =  QString::number(value);
            }
            json["shift_" + QString::number(i)] = object;
        }
    }
    
    toplevel["data"] = json;
    toplevel["model"] = m_name;    
    return toplevel;
}

void AbstractTitrationModel::ImportJSON(const QJsonObject &topjson)
{
    if(topjson[m_name].isNull())
    {
        qWarning() << "file doesn't contain any " + m_name;
        return;
    }
    QJsonObject json = topjson["data"].toObject();
    
    QVector<int > active_signals = QVector<int>(SignalCount(), 0);
    QVector<qreal> constants; 
    QJsonObject constantsObject = json["constants"].toObject();
    for (int i = 0; i < Constants().size(); ++i) {
        
        constants << constantsObject[QString::number(i)].toString().toDouble();
    }
    setConstants(constants);
    
    QVector<qreal> pureShift;
    QJsonObject pureShiftObject = json["pureShift"].toObject();
    for (int i = 0; i < m_pure_signals.size(); ++i) 
    {
        if(!pureShiftObject[QString::number(i)].isUndefined())
        {
            pureShift << pureShiftObject[QString::number(i)].toString().toDouble();
            active_signals[i] = 1;
        }
    }
    setPureSignals(pureShift);
    
    
    for(int i = 0; i < Constants().size(); ++i)
    {
        QVector<qreal> shifts;
        QJsonObject object = json["shift_" + QString::number(i)].toObject();
        for(int j = 0; j < m_pure_signals.size(); ++j)
        {
            if(!object[QString::number(i)].isNull())
            {
                shifts << object[QString::number(j)].toString().toDouble();
            }
        }
        setComplexSignals(shifts, i);
    }
    setActiveSignals(active_signals);
}
qreal AbstractTitrationModel::ModelError() const
{
    qreal error = 0;
    for(int z = 0; z < SignalCount(); ++z)
        error += SumOfErrors(z);
    return error;
}
void AbstractTitrationModel::IncrementParameter(double increment, int parameter)
{
    if(parameter < m_opt_para.size())
        *m_opt_para[parameter] += increment; 
}


#include "AbstractModel.moc"
