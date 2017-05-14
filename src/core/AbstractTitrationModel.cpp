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
#include "AbstractTitrationModel.h"

AbstractTitrationModel::AbstractTitrationModel(const DataClass *data) : AbstractModel(data)
{
    m_last_optimization = static_cast<OptimizationType>(OptimizationType::ComplexationConstants | OptimizationType::OptimizeShifts | OptimizationType::UnconstrainedShifts);
    m_constant_names << tr("no constants");   
}

AbstractTitrationModel::~AbstractTitrationModel()
{

}

void AbstractTitrationModel::adress() const
{
    std::cout << "We are at " << this;
    std::cout << "\t" << m_data;
    for(int i = 0; i < m_opt_para.size(); ++i)
        std::cout << m_opt_para[i] << " ";
    std::cout << std::endl;
}

void AbstractTitrationModel::SetConcentration(int i, const Vector& equilibrium)
{
    if(!m_concentrations)
    {
        m_concentrations = new DataTable( equilibrium.rows(), DataPoints(), this);
    }
    m_concentrations->setRow(equilibrium, i);
}

void AbstractTitrationModel::addOptParameterList_fromConstant(int i)
{
    for(int j = 0; j < m_complex_signal_parameter.rows(); ++j)
    {
        m_opt_para << &m_complex_signal_parameter(j, i);    
    } 
}

void AbstractTitrationModel::addOptParameterList_fromPure(int i)
{
    for(int j = 0; j < m_pure_signals_parameter.rows(); ++j)
    {
        m_opt_para << &m_pure_signals_parameter(j, i);    
    } 
}

void AbstractTitrationModel::MiniShifts()
{
    double cut_error = 1;
    for(int j = 0; j < m_lim_para.size(); ++j)
    {
        for(int i = 0; i < SignalCount(); ++i)    
        {
            if(ActiveSignals(i) == 1)
            {
                if(m_model_error->data(i, 0) < cut_error && j == 0)
                    *m_lim_para[j][i] -= m_model_error->data(i,0);
                if(m_model_error->data(i, m_model_error->rowCount() -1 ) < cut_error && j == 1)
                    *m_lim_para[j][i] -= m_model_error->data(i, m_model_error->rowCount() -1 );   
            }
        }
    }
}

qreal AbstractTitrationModel::BC50()
{
    return 0;
}


MassResults AbstractTitrationModel::MassBalance(qreal A, qreal B)
{
    MassResults result;
    Vector values(1) ;
    values(0) = 0;
    result.MassBalance = values;
    return result;
}

void AbstractTitrationModel::setComplexSignals(const QList< qreal > &list, int i)
{
    if(!(i < m_complex_signal_parameter.cols()))
        return;
    for(int j = 0; j < list.size(); ++j)
    {
        if(j < m_complex_signal_parameter.rows())
            m_complex_signal_parameter(j,i) = list[j];
    }
}


void AbstractTitrationModel::setPureSignals(const QList<qreal>& list)
{
    for(int i = 0; i < list.size(); ++i)
        m_pure_signals_parameter(i,0) = list[i];
}


QJsonObject AbstractTitrationModel::ExportModel(bool statistics) const
{
    QJsonObject json, toplevel;
    QJsonObject constantObject;
    for(int i = 0; i < GlobalParameter().size(); ++i)
    {
        constantObject[QString::number(i)] = (QString::number(GlobalParameter()[i]));
    }
    json["constants"] = constantObject;
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
    QJsonObject pureShiftObject;
    for(int i = 0; i < m_pure_signals_parameter.rows(); ++i)
        if(ActiveSignals(i))
            pureShiftObject[QString::number(i)] = (QString::number(m_pure_signals_parameter(i)));
        
        json["pureShift"] = pureShiftObject;   
    
    
    for(int i = 0; i < GlobalParameter().size(); ++i)
    {
        
        QJsonObject object;
        for(int j = 0; j < m_pure_signals_parameter.rows(); ++j)
        {
            if(ActiveSignals(j))
            {
                qreal value = Pair(i, j).second;
                object[QString::number(j)] =  QString::number(value);
            }
            json["shift_" + QString::number(i)] = object;
        }
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

void AbstractTitrationModel::ImportModel(const QJsonObject &topjson, bool override)
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
    QJsonObject constantsObject = json["constants"].toObject();
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
    
    QList<qreal> pureShift;
    QJsonObject pureShiftObject = json["pureShift"].toObject();
    for (int i = 0; i < m_pure_signals_parameter.rows(); ++i) 
    {
        pureShift << pureShiftObject[QString::number(i)].toString().toDouble();
        if(!pureShiftObject[QString::number(i)].isNull())
            active_signals <<  1;
        else
            active_signals <<  0;
        
    }
    setPureSignals(pureShift);
    
    
    for(int i = 0; i < GlobalParameter().size(); ++i)
    {
        QList<qreal> shifts;
        QJsonObject object = json["shift_" + QString::number(i)].toObject();
        for(int j = 0; j < m_pure_signals_parameter.rows(); ++j)
        {
            shifts << object[QString::number(j)].toString().toDouble();
        }
        setComplexSignals(shifts, i);
    }
    setActiveSignals(active_signals);
    
    if(topjson["runtype"].toInt() != 0)
        OptimizeParameters(static_cast<OptimizationType>(topjson["runtype"].toInt()));
    
    m_converged = topjson["converged"].toBool();
    
    Calculate();
}

QPair<qreal, qreal> AbstractTitrationModel::Pair(int i, int j) const
{
    return QPair<qreal, qreal>(GlobalParameter(i), m_complex_signal_parameter(j,i));
}

QString AbstractTitrationModel::formatedGlobalParameter(qreal value) const
{
    QString string;
    string = QString::number(qPow(10,value));
    return string;
}



#include "AbstractTitrationModel.moc"
