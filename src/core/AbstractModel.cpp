/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global.h"
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

AbstractModel::AbstractModel(DataClass *data) : DataClass(data), m_corrupt(false), m_last_p(1), m_f_value(1), m_last_parameter(0), m_last_freedom(0), m_converged(false), m_locked_model(false)
{    
    setActiveSignals(QVector<int>(SeriesCount(), 1).toList());
    
    m_model_signal = new DataTable(SeriesCount(),DataPoints(), this);
    m_model_error = new DataTable(SeriesCount(),DataPoints(), this);

    m_data = data; 
}

AbstractModel::AbstractModel(AbstractModel* other) :DataClass(other) , m_corrupt(other->m_corrupt), m_last_p(other->m_last_p), m_f_value(other->m_f_value), m_last_parameter(other->m_last_parameter), m_last_freedom(other->m_last_freedom), m_converged(other->m_converged), m_locked_model(other->m_locked_model)
{
    setOptimizerConfig(other->getOptimizerConfig());
    
    m_model_signal = other->m_model_signal;
    m_model_error = other->m_model_error;

    m_data = other->m_data; 
    
    m_local_parameter = other->m_local_parameter;
    m_active_signals = other->m_active_signals;
    m_locked_parameters = other->m_locked_parameters;
    m_model_options = other->m_model_options;
    m_enabled_parameter = other->m_enabled_parameter;
    m_global_parameter = other->m_global_parameter;
    
    m_mc_statistics = other->m_mc_statistics;
    m_wg_statistics = other->m_wg_statistics;
    m_moco_statistics = other->m_moco_statistics;
    
    m_sum_absolute = other->m_sum_absolute;
    m_sum_squares = other->m_sum_squares;
    m_variance = other->m_variance;
    m_mean = other->m_mean;
    m_stderror = other->m_stderror;
    m_SEy = other->m_SEy;
    m_chisquared = other->m_chisquared;
    m_covfit = other->m_covfit;
    m_used_variables = other->m_used_variables;
    
}

void AbstractModel::PrepareParameter(int global, int local)
{
    m_local_parameter = new DataTable(local, SeriesCount(), this);

    for(int i = 0; i < local; ++i)
        m_enabled_local << 0;

    for(int i = 0; i < global; ++i)
    {
        m_global_parameter << 0;
        m_enabled_global << 0;
    }

    addGlobalParameter(m_global_parameter);
    DeclareSystemParameter();
    DeclareOptions();
}

AbstractModel::~AbstractModel()
{
    if(m_model_signal)
        delete m_model_signal;
    
    if(m_model_error)
        delete m_model_error;
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
    for(int i = 0; i < m_enabled_local.size(); ++i)
        m_enabled_local[i] = 0;

    for(int i = 0; i < m_enabled_global.size(); ++i)
        m_enabled_global[i] = 0;
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
    /* if there are more than one series, we can active series, but with only one, these lines must be ignored*/
//     if(SeriesCount() != 1) 
    if(!ActiveSignals(j) || !DependentModel()->isChecked(j,i))
        return;
    
    if(std::isnan(value) || std::isinf(value))
    {
        value = 0;
        m_corrupt = true;
    }
    if(Type() != 3)
    {
        if(!m_locked_model)
            m_model_signal->data(j,i) = value;
        m_model_error->data(j,i) = m_model_signal->data(j,i) - DependentModel()->data(j,i);
        m_sum_absolute += qAbs(m_model_signal->data(j,i) - DependentModel()->data(j,i));
        m_sum_squares += qPow(m_model_signal->data(j,i) - DependentModel()->data(j,i), 2);
        m_mean += m_model_signal->data(j,i) - DependentModel()->data(j,i);
        m_used_variables++;
    }
}

void AbstractModel::Calculate()
{
    m_corrupt = false;
    m_mean = 0;
    m_variance = 0;
    m_used_variables = 0;
    m_stderror = 0;
    m_SEy = 0;
    m_chisquared = 0;
    m_covfit = 0;

    EvaluateOptions();
    m_data->LoadSystemParameter();
    CalculateVariables();
    
    m_mean /= qreal(m_used_variables);
    m_variance = CalculateVariance();
    m_stderror = qSqrt(m_variance)/qSqrt(m_used_variables);
    m_SEy = qSqrt(m_sum_squares/(m_used_variables-LocalParameterSize()-GlobalParameterSize()));
    m_chisquared = qSqrt(m_sum_squares/(m_used_variables-LocalParameterSize()-GlobalParameterSize() - 1));
    m_covfit = CalculateCovarianceFit();
    if(isCorrupt())
    {
        qDebug() << "Something went wrong during model calculation, most probably some numeric stuff";
    }
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
/*
qreal AbstractModel::CalculateVarianceData()
{
    qreal v = 0;
    int count = 0;
    for(int i = 0; i < DataPoints(); ++i)
    {
        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(DependentModel()->isChecked(j,i))
            {
                v += m_model_signal->data(j,i);
                count++;
            }
        }
    }
    double mean = v/double(v);
    for(int i = 0; i < DataPoints(); ++i)
    {
        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(DependentModel()->isChecked(j,i))
            {
                v += qPow(m_model_signal->data(j,i) - mean, 2);
                count++;
            }
        }
    }
    return v/(count -1 );
}*/

qreal AbstractModel::CalculateCovarianceFit()
{
    qreal model = 0, data = 0;
    int count = 0;
    qreal cov_data = 0, cov_model = 0;
    for(int i = 0; i < DataPoints(); ++i)
    {
        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(DependentModel()->isChecked(j,i))
            {
                model += m_model_signal->data(j,i);
                data  += DependentModel()->data(j,i);
                count++;
            }
        }
    }
    double mean_model = model/double(count);
    double mean_data = data/double(count);
    for(int i = 0; i < DataPoints(); ++i)
    {
        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(DependentModel()->isChecked(j,i))
            {
                cov_data += qPow(m_model_signal->data(j,i) - mean_model, 2);
                cov_model += qPow(DependentModel()->data(j,i) - mean_data, 2);
            }
        }
    }
    return cov_model/cov_data;
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

qreal AbstractModel::LocalParameter(int parameter, int series) const
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
        m_enabled_global[i] = 1;
    }
}

void AbstractModel::addGlobalParameter(int i)
{
    if(i < m_global_parameter.size())
        m_opt_para << &m_global_parameter[i]; 
    m_enabled_global[i] = 1;
}

void AbstractModel::addLocalParameter(int i)
{
    for(int j = 0; j < m_local_parameter->rowCount(); ++j)
        m_opt_para << &m_local_parameter->data(i, j);    
    m_enabled_local[i] = 1;
}

void AbstractModel::UpdateStatistic(const QJsonObject& object)
{
    QJsonObject controller = object["controller"].toObject();
    switch(controller["method"].toInt())
    {
        case  SupraFit::Statistic::WeakenedGridSearch:
            m_wg_statistics = object;
        break;
        
        case SupraFit::Statistic::ModelComparison:
            m_moco_statistics = object;
        break;
        
        case SupraFit::Statistic::FastConfidence:
                m_fast_confidence = object;
        break;
        
        case  SupraFit::Statistic::Reduction:
            m_reduction = object;
        break;
        
        case  SupraFit::Statistic::MonteCarlo:
        case  SupraFit::Statistic::CrossValidation:
            bool duplicate = false;
            for(int i = 0; i < m_mc_statistics.size(); ++i)
            {
                QJsonObject control = m_mc_statistics[i]["controller"].toObject();
                if(controller == control)
                {
                    duplicate = true;
                    m_mc_statistics[i] = object;
                }
            }
            if(!duplicate)
                m_mc_statistics << object;
        break;
    }
    emit StatisticChanged();
}


QString AbstractModel::Data2Text() const
{
    QString text;
    text += "#### Begin of Data Description ####\n";
    text += "Concentrations :   " + QString::number(DataPoints())  + "\n";
    for(int i = 0; i < IndependentModel()->columnCount(); ++i)
        text += " " + IndependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";   
    text += IndependentModel()->ExportAsString();
    text += "\n";
    text += "Signals :          " + QString::number(SeriesCount()) + "\n";
    for(int i = 0; i < DependentModel()->columnCount(); ++i)
        text += " " + DependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += DependentModel()->ExportAsString();
    text += "\n";
    text += Data2Text_Private();
    text += "#### End of Data Description #####\n";
    text += "******************************************************************************************************\n";
    return text;
}

QString AbstractModel::Model2Text() const
{
    QString text;
    text += "\n";
    text += "******************************************************************************************************\n";
    text += "#### Current Model Results #####\n";
    text += "Global parameter for model:\n";
    for(int i = 0; i < GlobalParameterSize(); ++i)
        text += GlobalParameterName(i) + ":\t" + formatedGlobalParameter(GlobalParameter(i), i)+ "\n";
    text += "\n";
    text += "Local parameter for model\n";
    text += m_local_parameter->ExportAsString();
    text += Model2Text_Private();
    text += "\n";
    text += ModelTable()->ExportAsString();
    text += "\n";
    text += "Errors obtained from currrent calculcation:\n";
    for(int i = 0; i < DependentModel()->columnCount(); ++i)
        text += " " + DependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += ErrorTable()->ExportAsString();
    text += "\n";
    text += "## Current Model Results Done ####\n";
    return text;
}

QString AbstractModel::Global2Text() const
{
    QString text;
    text += "\n";
    text += "******************************************************************************************************\n";
    text += "#### Current Model Results #####\n";
    text += "Equilibrium Model Calculation with complexation constants:\n";
    for(int i = 0; i < GlobalParameterSize(); ++i)
        text += " " + GlobalParameterName(i) + ":\t" + QString::number(GlobalParameter(i))+ "\n";
    for(int i = 0; i < IndependentModel()->columnCount(); ++i)
        text += " " + IndependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";

    text += "## Current Model Results Done ####\n";
    return text;
}

QString AbstractModel::Local2Text() const
{
    QString text;
    text += "\n";
    text += "******************************************************************************************************\n";
    text += "#### Current Model Results #####\n";
    text += "Equilibrium Model Calculation with complexation constants:\n";
#warning to be done
//     for(int i = 0; i < GlobalParameterSize(); ++i)
//         for(int j = 0; j< LocalParameterSize())
//             text += LocalParameter(i) + ":\t" + QString::number(GlobalParameter(i))+ "";
    return text;
}


QJsonObject AbstractModel::ExportModel(bool statistics, bool locked) const
{
    QJsonObject json, toplevel;
    QJsonObject constantObject, optionObject;
    QString names;
    for(int i = 0; i < GlobalParameter().size(); ++i)
    {
        constantObject[QString::number(i)] = (QString::number(GlobalParameter()[i]));
        names += GlobalParameterName(i) + "|";
    }
    names.chop(1);
    constantObject["names"] = names;
    json["globalParameter"] = constantObject;
    
    if(statistics)
    {
        QJsonObject statisticObject;
        for(int i = 0; i < m_mc_statistics.size(); ++i)
        {
            statisticObject[QString::number(SupraFit::Statistic::MonteCarlo) + ":"+QString::number(i)] = m_mc_statistics[i];
        }
        statisticObject[QString::number(SupraFit::Statistic::ModelComparison)] = m_moco_statistics;
        statisticObject[QString::number(SupraFit::Statistic::WeakenedGridSearch)] = m_wg_statistics;
        statisticObject[QString::number(SupraFit::Statistic::Reduction)] = m_reduction;
        statisticObject[QString::number(SupraFit::Statistic::FastConfidence)] = m_fast_confidence;
        json["statistics"] = statisticObject;
    }

    if(LocalParameterSize())
    {
        QJsonObject localParameter;
        for(int i = 0; i < m_local_parameter->rowCount(); ++i)
            if(ActiveSignals(i))
                localParameter[QString::number(i)] = (ToolSet::DoubleList2String(m_local_parameter->Row(i)));
            
        names = QString();
        for(int i = 0; i < LocalParameterSize(); ++i)
            names += LocalParameterName(i) + "|";
    
        names.chop(1);
        localParameter["names"] = names;
        json["localParameter"] = localParameter;   
    }
    
    for(const QString &str : getAllOptions())
        optionObject[str] = getOption(str);
    
    
    QJsonObject resultObject;
    if(m_locked_model || locked)
    {
        for(int i = 0; i < DataPoints(); ++i)
        {
            resultObject[QString::number(i)] = ToolSet::DoubleList2String(ModelTable()->Row(i));
        }
    }
    
    toplevel["data"] = json;
    toplevel["options"] = optionObject;
    toplevel["model"] = Name();  
    toplevel["runtype"] = m_last_optimization;
    toplevel["sum_of_squares"] = m_sum_squares;
    toplevel["sum_of_absolute"] = m_sum_absolute;
    toplevel["mean_error"] = m_mean;
    toplevel["variance"] = m_variance;
    toplevel["standard_error"] = m_stderror;
    toplevel["converged"] = m_converged;
    if(m_locked_model || locked)
    {
#ifdef _DEBUG
//         qDebug() << "Writing calculated data to json file";
#endif
        toplevel["locked_model"] = true;
        toplevel["result"] = resultObject;
    }
    return toplevel;
}

void AbstractModel::ImportModel(const QJsonObject &topjson, bool override)
{
#ifdef _DEBUG
            quint64 t0 = QDateTime::currentMSecsSinceEpoch();
#endif
    if(topjson[Name()].isNull())
    {
        qWarning() << "file doesn't contain any " + Name();
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
    QJsonObject globalParameter, optionObject;
    
    if(json.contains("globalParameter"))
        globalParameter = json["globalParameter"].toObject();
    else if(json.contains("constants"))
        globalParameter = json["constants"].toObject();
    else
    {
        qWarning() << "No global parameter found!";
    }
    for (int i = 0; i < GlobalParameter().size(); ++i) 
    {
        constants << globalParameter[QString::number(i)].toString().toDouble();
    }
    setGlobalParameter(constants);
    
    optionObject = topjson["options"].toObject();
    for(const QString &str : getAllOptions())
        setOption(str, topjson["options"].toObject()[str].toString());
    
    QStringList keys = json["statistics"].toObject().keys();
    QJsonObject statisticObject = json["statistics"].toObject();
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
        m_mc_statistics.clear();
    }
    
    m_moco_statistics = statisticObject[QString::number(SupraFit::Statistic::ModelComparison)].toObject();
    m_wg_statistics =  statisticObject[QString::number(SupraFit::Statistic::WeakenedGridSearch)].toObject();
    m_reduction = statisticObject[QString::number(SupraFit::Statistic::Reduction)].toObject();
    m_fast_confidence = statisticObject[QString::number(SupraFit::Statistic::FastConfidence)].toObject();
    
    for(const QString &str : qAsConst(keys))
    {
        if(str.contains(":"))
            m_mc_statistics << statisticObject[str].toObject();
    }

    if(json.contains("localParameter"))
    {
    /*
     * Here goes SupraFit 2 data handling
    */
        if(LocalParameterSize())
        {
            QJsonObject localParameter= json["localParameter"].toObject();
            for(int i = 0; i < SeriesCount(); ++i)
            {
                QVector<qreal > localVector;
                if(!localParameter[QString::number(i)].isNull())
                {
                    localVector = ToolSet::String2DoubleVec(localParameter[QString::number(i)].toString());    
                    active_signals <<  1;
                }
                else
                {
                    localVector = QVector<qreal>(LocalParameterSize(), 0);
                    active_signals <<  0;
                }
                m_local_parameter->setRow(localVector, i);
            }
        }
    }
    else if(json.contains("pureShift"))
    {
        /*
         * This is SupraFit 1 legacy data handling
         */
        for(int i = 0; i < SeriesCount(); ++i)
        {
            QVector<qreal> localSeries;
            QJsonObject pureShiftObject = json["pureShift"].toObject();
            
            localSeries << pureShiftObject[QString::number(i)].toString().toDouble();
            
            if(!pureShiftObject[QString::number(i)].isNull())
                active_signals <<  1;
            else
                active_signals <<  0;
            
            for(int j = 0; j < GlobalParameter().size(); ++j)
            {
                QJsonObject object = json["shift_" + QString::number(j)].toObject();
                localSeries << object[QString::number(i)].toString().toDouble();
                
            }
            m_local_parameter->setRow(localSeries, i);
        }
    }

    setActiveSignals(active_signals);
    if(topjson["runtype"].toInt() != 0)
        OptimizeParameters(static_cast<OptimizationType>(topjson["runtype"].toInt()));
    
    m_converged = topjson["converged"].toBool();
    
    if(topjson.contains("locked_model"))
    {
#ifdef _DEBUG
//         qDebug() << "Loaded calculated data from json file";
#endif
        m_locked_model = true;
        QJsonObject resultObject = topjson["result"].toObject();
        QStringList keys = resultObject.keys();
        
        QCollator collator;
        collator.setNumericMode(true);
        std::sort(
            keys.begin(),
                  keys.end(),
                  [&collator](const QString &key1, const QString &key2)
                  {
                      return collator.compare(key1, key2) < 0;
                  });
        for(const QString &str: qAsConst(keys))
        {
            QVector<qreal > concentrationsVector, signalVector;
            concentrationsVector = ToolSet::String2DoubleVec(resultObject[str].toString());
            int row = str.toInt();
            ModelTable()->setRow(concentrationsVector, row);
        }
    }
#ifdef _DEBUG
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "model importet within" << t1-t0 << " msecs";
#endif
    Calculate();
    
#ifdef _DEBUG
    quint64 t2 = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "calculation took " << t2-t1 << " msecs";
#endif
}

void AbstractModel::setOption(const QString& name, const QString& value)
{
    if(!m_model_options.contains(name) || name.isEmpty() || value.isEmpty() || name.isNull() || value.isNull())
        return; 
    m_model_options[name].value = value;
    OptimizeParameters(m_last_optimization);
}


AbstractModel & AbstractModel::operator=(const AbstractModel& other)
{
    setOptimizerConfig(other.getOptimizerConfig());
    
    m_model_signal = other.m_model_signal;
    m_model_error = other.m_model_error;

    m_data = other.m_data; 
    
    m_local_parameter = other.m_local_parameter;
    m_active_signals = other.m_active_signals;
    m_locked_parameters = other.m_locked_parameters;
    m_model_options = other.m_model_options;
    m_enabled_parameter = other.m_enabled_parameter;
    m_global_parameter = other.m_global_parameter;
    
    m_mc_statistics = other.m_mc_statistics;
    m_wg_statistics = other.m_wg_statistics;
    m_moco_statistics = other.m_moco_statistics;
    m_reduction = other.m_reduction;
    m_fast_confidence = other.m_fast_confidence;
    
    m_sum_absolute = other.m_sum_absolute;
    m_sum_squares = other.m_sum_squares;
    m_variance = other.m_variance;
    m_mean = other.m_mean;
    m_stderror = other.m_stderror;
    m_SEy = other.m_SEy;
    m_chisquared = other.m_chisquared;
    m_covfit = other.m_covfit;
    m_used_variables = other.m_used_variables;
    
    return *this;
}

AbstractModel * AbstractModel::operator=(const AbstractModel* other)
{
    setOptimizerConfig(other->getOptimizerConfig());
    
    m_model_signal = other->m_model_signal;
    m_model_error = other->m_model_error;

    m_data = other->m_data; 
    
    m_local_parameter = other->m_local_parameter;
    m_active_signals = other->m_active_signals;
    m_locked_parameters = other->m_locked_parameters;
    m_model_options = other->m_model_options;
    m_enabled_parameter = other->m_enabled_parameter;
    m_global_parameter = other->m_global_parameter;
    
    m_mc_statistics = other->m_mc_statistics;
    m_wg_statistics = other->m_wg_statistics;
    m_moco_statistics = other->m_moco_statistics;
    m_reduction = other->m_reduction;
    m_fast_confidence = other->m_fast_confidence;
    
    m_sum_absolute = other->m_sum_absolute;
    m_sum_squares = other->m_sum_squares;
    m_variance = other->m_variance;
    m_mean = other->m_mean;
    m_stderror = other->m_stderror;
    m_SEy = other->m_SEy;
    m_chisquared = other->m_chisquared;
    m_covfit = other->m_covfit;
    m_used_variables = other->m_used_variables;
    
    return this;
}
#include "AbstractModel.moc"
