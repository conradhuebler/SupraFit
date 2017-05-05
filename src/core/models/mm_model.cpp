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



#include "src/core/AbstractModel.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"

#include <QtMath>

#include <QtCore/QCollator>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QDebug>
#include <QtCore/QDateTime>
#include <cmath>
#include <cfloat>
#include <iostream>

#include "mm_model.h"

Michaelis_Menten_Model::Michaelis_Menten_Model(const DataClass *data) : AbstractModel(data)
{
    setName(tr("Michaelis Menten"));
    m_complex_signal_parameter = Eigen::MatrixXd::Zero(SignalCount(), 1);
    InitialGuess();
    
    
    m_constant_names = QStringList() << tr("vmax") << tr("Km");
}

Michaelis_Menten_Model::Michaelis_Menten_Model(const AbstractModel* model) : AbstractModel(model)
{
    setName(tr("Michaelis Menten"));
    m_complex_signal_parameter = Eigen::MatrixXd::Zero(SignalCount(), 1);
    InitialGuess();
        
    m_constant_names = QStringList() << tr("vmax") << tr("Km");
}


Michaelis_Menten_Model::~Michaelis_Menten_Model() 
{
    
}

void Michaelis_Menten_Model::InitialGuess()
{
    m_vmax = 4;
    m_Km = 2;
    m_global_parameter = QList<qreal>() << m_vmax << m_Km;
    m_complex_signal_parameter.col(0) = SignalModel()->lastRow();
    m_pure_signals_parameter = SignalModel()->firstRow();
    setOptParamater(m_global_parameter);
    
//     QVector<qreal * > line1, line2;
//     for(int i = 0; i < m_pure_signals_parameter.size(); ++i)
//     {
//         line1 << &m_pure_signals_parameter(i, 0);
//         line2 << &m_complex_signal_parameter(i,0);
//     }
//     m_lim_para = QVector<QVector<qreal * > >()  << line1 << line2;
    
    AbstractModel::Calculate();
}

QVector<qreal> Michaelis_Menten_Model::OptimizeParameters_Private(OptimizationType type)
{    
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
        setOptParamater(m_global_parameter);

    if((type & OptimizationType::OptimizeShifts) == (OptimizationType::OptimizeShifts))
    {
         
        if((type & OptimizationType::UnconstrainedShifts) == OptimizationType::UnconstrainedShifts)
        {
//             addOptParameterList_fromConstant(0);
//             if((type & OptimizationType::IgnoreZeroConcentrations) != OptimizationType::IgnoreZeroConcentrations)
//                 addOptParameterList_fromPure(0);
        }
    }
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void Michaelis_Menten_Model::CalculateVariables(const QList<qreal > &constants)
{  
    m_corrupt = false;
    
    m_sum_absolute = 0;
    m_sum_squares = 0;
//     for(int i = 0; i < DataPoints(); ++i)
//     {
//         qreal host_0 = InitialHostConcentration(i);
//         qreal guest_0 = InitialGuestConcentration(i);
//         qreal host = HostConcentration(host_0, guest_0, constants);
//         qreal complex = host_0 -host;
//         Vector vector(4);
//         vector(0) = i + 1;
//         vector(1) = host;
//         vector(2) = guest_0 - complex;
//         vector(3) = complex;
//         SetConcentration(i, vector);
//         for(int j = 0; j < SignalCount(); ++j)
//         {
//             qreal value = host/host_0*m_pure_signals_parameter(j, 0)+ complex/host_0*m_complex_signal_parameter(j,0);
//             SetSignal(i, j, value);    
//         }
//     }
    emit Recalculated();
}


QSharedPointer<AbstractModel > Michaelis_Menten_Model::Clone() const
{
    QSharedPointer<Michaelis_Menten_Model > model = QSharedPointer<Michaelis_Menten_Model>(new Michaelis_Menten_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
    
}


QJsonObject Michaelis_Menten_Model::ExportModel(bool statistics) const
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
    
    return toplevel;
}

void Michaelis_Menten_Model::ImportModel(const QJsonObject &topjson, bool override)
{
    if(topjson[m_name].isNull())
    {
        qWarning() << "file doesn't contain any " + m_name;
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
    
    if(topjson["runtype"].toInt() != 0)
        OptimizeParameters(static_cast<OptimizationType>(topjson["runtype"].toInt()));
    
    
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
    setActiveSignals(active_signals);
    Calculate();
}

QPair<qreal, qreal> Michaelis_Menten_Model::Pair(int i, int j) const
{
    if(i >= 1)
        return QPair<qreal, qreal>(0,0);
    return QPair<qreal, qreal>(GlobalParameter(i), m_complex_signal_parameter(j,i));
}

#include "mm_model.moc"
