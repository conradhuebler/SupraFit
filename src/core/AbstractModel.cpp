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
#include <cmath>
#include <cfloat>
#include <iostream>
#include "AbstractModel.h"

AbstractTitrationModel::AbstractTitrationModel(const DataClass *data) : DataClass(data), m_corrupt(false)
{
    m_constant_names << tr("no constants");
    setActiveSignals(QVector<int>(SignalCount(), 1).toList());
    ptr_concentrations = data->Concentration();
    
    m_model_signal = new DataTable(SignalCount(),DataPoints());
    m_model_error = new DataTable(SignalCount(),DataPoints());
    
    m_pure_signals = SignalModel()->firstRow();
    m_data = data;
    connect(m_data, SIGNAL(recalculate()), this, SLOT(CalculateSignal()));
    
}

AbstractTitrationModel::~AbstractTitrationModel()
{
    
}

void AbstractTitrationModel::adress() const
{
    std::cout << "We are at " << this;
    std::cout << "\t" << m_data;
    std::cout << "\t "<< m_data->Concentration();
    std::cout << "\t" << Concentration() << std::endl;
}


QVector<double>   AbstractTitrationModel::getCalculatedSignals(QList<int > active_signal)
{
    if(active_signal.size() < SignalCount() && ActiveSignals().size() < SignalCount())
        active_signal = QVector<int>(SignalCount(), 1).toList();
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
    m_locked_parameters.clear();
    for(int i = 0; i < variables.size(); ++i)
        m_locked_parameters << 1;
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
    {
        constantObject[QString::number(i)] = (QString::number(Constants()[i]));
        if(i < m_statistics.size())
        {
            QJsonObject statistic;
            statistic["max"] = QString::number(m_statistics[i].max);
            statistic["min"] = QString::number(m_statistics[i].min);
            statistic["error"] = QString::number(m_statistics[i].error);
            statistic["integ_1"] = QString::number(m_statistics[i].integ_1);
            statistic["integ_5"] = QString::number(m_statistics[i].integ_5);
            constantObject[QString::number(i)+"_statistic"] = statistic;
        }
    }
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
    toplevel["runtype"] = m_last_optimization;
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
    
    QList<int > active_signals = QVector<int>(SignalCount(), 0).toList();
    QVector<qreal> constants; 
    QJsonObject constantsObject = json["constants"].toObject();
    for (int i = 0; i < Constants().size(); ++i) {
        
        constants << constantsObject[QString::number(i)].toString().toDouble();
        
        StatisticResult result;
        QJsonObject statistic = constantsObject[QString::number(i) + "_statistic"].toObject();
        result.max =  statistic["max"].toString().toDouble();
        result.min =  statistic["min"].toString().toDouble();
        result.error =  statistic["error"].toString().toDouble();
        result.integ_1 =  statistic["integ_1"].toString().toDouble();
        result.integ_5 =  statistic["integ_5"].toString().toDouble();
        setStatistic(result, i);
    }
    setConstants(constants);
    m_last_optimization = static_cast<OptimizationType>(topjson["runtype"].toInt()); 
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
double AbstractTitrationModel::IncrementParameter(double increment, int parameter)
{
    if(parameter < m_opt_para.size())
        *m_opt_para[parameter] += increment; 
    return *m_opt_para[parameter];
}

void AbstractTitrationModel::MiniShifts()
{
    QList<int > active_signal;
    if(ActiveSignals().size() < SignalCount())
        active_signal = QVector<int>(SignalCount(), 1).toList();
    else
        active_signal = ActiveSignals();
    double cut_error = 1;
    for(int j = 0; j < m_lim_para.size(); ++j)
    {
        for(int i = 0; i < SignalCount(); ++i)    
        {
            if(active_signal[i] == 1)
            {
                if(m_model_error->data(i, 0) < cut_error && j == 0)
                    *m_lim_para[j][i] -= m_model_error->data(i,0);
                if(m_model_error->data(i, m_model_error->rowCount() -1 ) < cut_error && j == 1)
                    *m_lim_para[j][i] -= m_model_error->data(i, m_model_error->rowCount() -1 );   
            }
        }
    }
}

void AbstractTitrationModel::setStatistic(const StatisticResult &result, int i)
{
    if(i < m_statistics.size())
        m_statistics[i] = result;
    else
        m_statistics << result; 
    emit StatisticChanged(result, i);
}

#include "AbstractModel.moc"
