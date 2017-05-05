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
#include <QtCore/QCollator>
#include <cmath>
#include <cfloat>
#include <iostream>
#include "AbstractModel.h"

AbstractModel::AbstractModel(const DataClass *data) : DataClass(data), m_corrupt(false), m_last_p(1), m_f_value(1), m_last_parameter(0), m_last_freedom(0)
{
    
    
}

AbstractModel::~AbstractModel()
{
    
    
}

QVector<qreal> AbstractModel::OptimizeParameters(OptimizationType type)
{
    clearOptParameter();
    QVector<qreal > variables =  OptimizeParameters_Private(type);
    for(int j = m_opt_para.size() - 1; j >= 0; --j)
    {
        if(variables[j] == 0)
        {
            variables.removeAt(j);
            m_opt_para.removeAt(j);
        }
    }
    m_locked_parameters.clear();
    for(int i = 0; i < variables.size(); ++i)
        m_locked_parameters << 1;
    m_last_optimization = type;
    return variables;
}

void AbstractModel::clearOptParameter()
{
    m_opt_para.clear();
}

void AbstractModel::setGlobalParameter(const QList<qreal> &list)
{
    if(list.size() != m_global_parameter.size())
        return;
    for(int i = 0; i < list.size(); ++i)
        m_global_parameter[i] = list[i];  
}

void AbstractModel::Calculate(const QList<qreal > &constants)
{
    m_corrupt = false;
    m_mean = 0;
    m_variance = 0;
    m_used_variables = 0;
    m_stderror = 0;
    CalculateVariables(constants);
    
    m_mean /= qreal(m_used_variables);
    m_variance = CalculateVariance();
    m_stderror = qSqrt(m_variance)/qSqrt(m_used_variables);
    emit Recalculated();
}

qreal AbstractModel::ModelError() const
{
    qreal error = 0;
    for(int z = 0; z < SignalCount(); ++z)
        error += SumOfErrors(z);
    return error;
}

qreal AbstractModel::CalculateVariance()
{
    qreal v = 0;
    int count = 0;
    for(int i = 0; i < DataPoints(); ++i)
    {
        for(int j = 0; j < SignalCount(); ++j)
        {
            if(SignalModel()->isChecked(j,i))
            {
                v += qPow(m_model_error->data(j,i) - m_mean, 2);
                count++;
            }
        }
    }
    return v/(count -1 );
}


qreal AbstractModel::SumOfErrors(int i) const
{
    qreal sum = 0;
    
    if(!ActiveSignals(i) || i >= Size())
        return sum;
    
    for(int j = 0; j < DataPoints(); ++j)
    {
        sum += qPow(m_model_error->data(i,j),2);
    }
    return sum;
}

qreal AbstractModel::finv(qreal p)
{
    /*
     * Lets cache the f-value, that if nothing changes, no integration is needed
     */
    if(!(p == m_last_p && m_last_parameter == Parameter() && m_last_freedom == Points()-Parameter()))
    {
        m_f_value = ToolSet::finv(p, Parameter(),Points()-Parameter());
        m_last_p = p;
        m_last_parameter = Parameter();
        m_last_freedom = Points()-Parameter();
    }   
    return m_f_value;
}

qreal AbstractModel::Error(qreal confidence, bool f)
{
    if(f)
    {
        qreal f_value = finv(confidence/100);
        return SumofSquares()*(f_value*Parameter()/(Points()-Parameter()) +1);
    } else {
        return SumofSquares()+SumofSquares()*confidence/double(100);
    }
}

void AbstractModel::setOptParamater(QList<qreal> &parameter)
{
    clearOptParameter();
    for(int i = 0; i < parameter.size(); ++i)
        m_opt_para << &parameter[i];
}

void AbstractModel::setOptParamater(qreal& parameter)
{   
    clearOptParameter();
    m_opt_para << &parameter;
}


void AbstractModel::addOptParameter(QList<qreal>& parameter)
{
    for(int i = 0; i < parameter.size(); ++i)
    {
        m_opt_para << &parameter[i];    
    }
}


void AbstractModel::SetSingleParameter(double value, int parameter)
{
    if(parameter < m_opt_para.size())
        *m_opt_para[parameter] = value;
}

void AbstractModel::setParamter(const QVector<qreal>& parameter)
{
    if(parameter.size() != m_opt_para.size())
        return;
    for(int i = 0; i < parameter.size(); ++i)
        if(m_locked_parameters[i])
            *m_opt_para[i] = parameter[i];
}

void AbstractModel::setCVStatistic(const QJsonObject &result, int i)
{
    if(i < m_cv_statistics.size())
        m_cv_statistics[i] = result;
    else
        m_cv_statistics << result; 
    emit StatisticChanged();
}

void AbstractModel::setMCStatistic(const QJsonObject &result, int i)
{
    if(i < m_mc_statistics.size())
        m_mc_statistics[i] = result;
    else
        m_mc_statistics << result; 
    emit StatisticChanged();
}

void AbstractModel::setMoCoStatistic(const QJsonObject &result, int i)
{
    if(i < m_moco_statistics.size())
        m_moco_statistics[i] = result;
    else
        m_moco_statistics << result; 
    emit StatisticChanged();
}


#include "AbstractModel.moc"
