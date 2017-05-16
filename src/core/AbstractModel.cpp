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

AbstractModel::AbstractModel(const DataClass *data) : DataClass(data), m_corrupt(false), m_last_p(1), m_f_value(1), m_last_parameter(0), m_last_freedom(0), m_converged(false)
{    
    setActiveSignals(QVector<int>(SeriesCount(), 1).toList());
    
    m_model_signal = new DataTable(SeriesCount(),DataPoints(), this);
    m_model_error = new DataTable(SeriesCount(),DataPoints(), this);
    
    m_data = data; 
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

void AbstractModel::SetValue(int i, int j, qreal value)
{
#warning dont forget me
    if(IndependentVariableSize() != 1) //FIXME just for now
        if(!ActiveSignals(j) || !DependentModel()->isChecked(j,i))
            return;
    if(std::isnan(value) || std::isinf(value))
    {
        value = 0;
        m_corrupt = true;
    }
    if(Type() != 3)
    {
        m_model_signal->data(j,i) = value;
        m_model_error->data(j,i) = m_model_signal->data(j,i) - DependentModel()->data(j,i);
        m_sum_absolute += qAbs(m_model_signal->data(j,i) - DependentModel()->data(j,i));
        m_sum_squares += qPow(m_model_signal->data(j,i) - DependentModel()->data(j,i), 2);
        m_mean += m_model_signal->data(j,i) - DependentModel()->data(j,i);
        m_used_variables++;
    }
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
    for(int z = 0; z < SeriesCount(); ++z)
        error += SumOfErrors(z);
    return error;
}

qreal AbstractModel::CalculateVariance()
{
    qreal v = 0;
    int count = 0;
    for(int i = 0; i < DataPoints(); ++i)
    {
        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(DependentModel()->isChecked(j,i))
            {
                v += qPow(m_model_error->data(j,i) - m_mean, 2);
                count++;
            }
        }
    }
    return v/(count -1 );
}

QList<double>   AbstractModel::getCalculatedModel()
{
    QList<double > x;
    for(int j = 0; j < SeriesCount(); ++j)
    {
        for(int i = 0; i < DataPoints(); ++i)
        {
            
            if(ActiveSignals(j) == 1 || IndependentVariableSize() == 1) //FIXME just that it works now
                x.append( m_model_signal->data(j,i)); 
        }
    }
    return x;
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

void AbstractModel::SetSingleParameter(double value, int parameter)
{
    if(parameter < m_opt_para.size())
        *m_opt_para[parameter] = value;
}

void AbstractModel::setParameter(const QVector<qreal>& parameter)
{
    if(parameter.size() != m_opt_para.size())
        return;
    for(int i = 0; i < parameter.size(); ++i)
        if(m_locked_parameters[i])
            *m_opt_para[i] = parameter[i];
}

qreal AbstractModel::getLocalParameter(int parameter, int series) const
{
    if(series >= m_local_parameter->rowCount() || parameter >= m_local_parameter->columnCount())
        return 0;
    else
        return m_local_parameter->data(parameter, series);
}

QVector<qreal > AbstractModel::getLocalParameterColumn(int parameter) const
{
    QVector<qreal> column;
    if(parameter >= m_local_parameter->columnCount())
        return column;
    else
    {
        for(int i = 0; i < m_local_parameter->rowCount(); ++i)
            column << m_local_parameter->data(parameter, i); 
    }
    return column;
}

void AbstractModel::setLocalParameter(qreal value, int parameter, int series)
{
    if(parameter < m_local_parameter->rowCount() && series < m_local_parameter->columnCount())
        m_local_parameter->data(parameter, series) = value;
}

void AbstractModel::setLocalParameterColumn(const QVector<qreal> &vector, int parameter)
{
    if(parameter < m_local_parameter->columnCount())
        m_local_parameter->setColumn(vector, parameter);
}

void AbstractModel::setLocalParameterColumn(const Vector& vector, int parameter)
{
    if(parameter < m_local_parameter->columnCount())
        m_local_parameter->setColumn(vector, parameter);
}

void AbstractModel::setLocalParameterSeries(const QVector<qreal>& vector, int series)
{
    if(series < m_local_parameter->rowCount())
        m_local_parameter->setRow(vector, series);
}

void AbstractModel::setLocalParameterSeries(const Vector& vector, int series)
{    
    if(series < m_local_parameter->rowCount())
        m_local_parameter->setRow(vector, series);
}

void AbstractModel::addGlobalParameter(QList<qreal>& parameter)
{
    for(int i = 0; i < parameter.size(); ++i)
    {
        m_opt_para << &parameter[i];    
    }
}

void AbstractModel::addGlobalParameter(int i)
{
    if(i < m_global_parameter.size())
        m_opt_para << &m_global_parameter[i]; 
 
}

void AbstractModel::addLocalParameter(int i)
{
    for(int j = 0; j < m_local_parameter->rowCount(); ++j)
        m_opt_para << &m_local_parameter->data(i, j);    
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



QJsonObject AbstractModel::ExportModel(bool statistics) const
{
    QJsonObject json, toplevel;
    QJsonObject constantObject;
    for(int i = 0; i < GlobalParameter().size(); ++i)
    {
        constantObject[QString::number(i)] = (QString::number(GlobalParameter()[i]));
    }
    json["globalParameter"] = constantObject;
    
    if(statistics)
    {
        QJsonObject statisticObject;
        for(int i = 0; i < m_cv_statistics.size(); ++i)
        {
            statisticObject["CVResult_"+QString::number(i)] = m_cv_statistics[i];
        }
        for(int i = 0; i < m_mc_statistics.size(); ++i)
        {
            statisticObject["MCResult_"+QString::number(i)] = m_mc_statistics[i];
        }
        for(int i = 0; i < m_moco_statistics.size(); ++i)
        {
            statisticObject["MoCoResult_"+QString::number(i)] = m_moco_statistics[i];
        }
        json["statistics"] = statisticObject;
    }

    if(LocalParameterSize())
    {
        QJsonObject localParameter;
        for(int i = 0; i < m_local_parameter->rowCount(); ++i)
            if(ActiveSignals(i))
                localParameter[QString::number(i)] = (ToolSet::DoubleList2String(m_local_parameter->Row(i)));

        json["localParameter"] = localParameter;   
    }
    toplevel["data"] = json;
    toplevel["model"] = m_name;  
//     qDebug() << m_last_optimization;
    toplevel["runtype"] = m_last_optimization;
    toplevel["sum_of_squares"] = m_sum_squares;
    toplevel["sum_of_absolute"] = m_sum_absolute;
    toplevel["mean_error"] = m_mean;
    toplevel["variance"] = m_variance;
    toplevel["standard_error"] = m_stderror;
    toplevel["converged"] = m_converged;
    return toplevel;
}

void AbstractModel::ImportModel(const QJsonObject &topjson, bool override)
{
    if(topjson[m_name].isNull())
    {
        qWarning() << "file doesn't contain any " + m_name;
        return;
    }
    if(topjson["model"] != Name())
    {
        qWarning() << "Models don't fit!";
        return;
    }  
    QJsonObject json = topjson["data"].toObject();
    
    QList<int > active_signals;
    QList<qreal> constants; 
    QJsonObject constantsObject = json["globalParameter"].toObject();
    for (int i = 0; i < GlobalParameter().size(); ++i) 
    {
        constants << constantsObject[QString::number(i)].toString().toDouble();
    }
    setGlobalParameter(constants);
    QStringList keys = json["statistics"].toObject().keys();
    
    if(keys.size() > 9)
    {
        QCollator collator;
        collator.setNumericMode(true);
        std::sort(
            keys.begin(),
                  keys.end(),
                  [&collator](const QString &key1, const QString &key2)
                  {
                      return collator.compare(key1, key2) < 0;
                  });
    }
    if(override)
    {
        m_cv_statistics.clear();
        m_moco_statistics.clear();
        m_mc_statistics.clear();
    }
    for(const QString &str : qAsConst(keys))
    {
        if(str.contains("CV"))
            m_cv_statistics << json["statistics"].toObject()[str].toObject();
        else if(str.contains("MC"))
            m_mc_statistics << json["statistics"].toObject()[str].toObject();
        else if(str.contains("MoCo"))
            m_moco_statistics << json["statistics"].toObject()[str].toObject();
    }
    
    /*
     * Here goes SupraFit 2 data handling
     */
    if(LocalParameterSize())
    {
        QJsonObject localParameter= json["localParameter"].toObject();
        keys = localParameter.keys();
        
        QCollator collator;
        collator.setNumericMode(true);
        std::sort(
            keys.begin(),
                keys.end(),
                [&collator](const QString &key1, const QString &key2)
                {
                    return collator.compare(key1, key2) < 0;
                });
        int i = 0;
        for(const QString &str: qAsConst(keys))
        {
            QVector<qreal > localVector;
            localVector = ToolSet::String2DoubleVec(localParameter[str].toString());
            m_local_parameter->setRow(localVector, i);
            ++i;
        }
    }
    
    /*
     * This will be SupraFit 1 legacy data handling
     */
    
//     QList<qreal> pureShift;
//     QJsonObject pureShiftObject = json["pureShift"].toObject();
//     for (int i = 0; i < m_pure_signals_parameter.rows(); ++i) 
//     {
//         pureShift << pureShiftObject[QString::number(i)].toString().toDouble();
//         if(!pureShiftObject[QString::number(i)].isNull())
//             active_signals <<  1;
//         else
//             active_signals <<  0;
//         
//     }
//     setPureSignals(pureShift);
//     
//     
//     for(int i = 0; i < GlobalParameter().size(); ++i)
//     {
//         QList<qreal> shifts;
//         QJsonObject object = json["shift_" + QString::number(i)].toObject();
//         for(int j = 0; j < m_pure_signals_parameter.rows(); ++j)
//         {
//             shifts << object[QString::number(j)].toString().toDouble();
//         }
//         setComplexSignals(shifts, i);
//     }
//     setActiveSignals(active_signals);
    
    if(topjson["runtype"].toInt() != 0)
        OptimizeParameters(static_cast<OptimizationType>(topjson["runtype"].toInt()));
    
    m_converged = topjson["converged"].toBool();
    
    Calculate();
}

#include "AbstractModel.moc"
