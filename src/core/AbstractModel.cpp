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

AbstractTitrationModel::AbstractTitrationModel(const DataClass *data) : DataClass(data),  m_repaint(false), m_debug(false), m_inform_config_changed(true), m_corrupt(false)
{
    
    qDebug() << DataPoints() << Size();
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
        m_model_error->data(j,i) = m_model_signal->data(j,i) - SignalModel()->data(j,i);
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

QString AbstractTitrationModel::OptPara2String() const
{
    QString result;
    result += "\n";
    result += "|***********************************************************************************|\n";
    result += "|********************General Config for Optimization********************************|\n";
    result += "|Maximal number of Iteration: " + QString::number(m_opt_config.MaxIter) + "|\n";
    result += "|Shifts will be optimized for zero and saturation concentration: " + bool2YesNo(m_opt_config.OptimizeBorderShifts) + "|\n";
    result += "|Shifts will be optimized for any other concentration: " + bool2YesNo(m_opt_config.OptimizeIntermediateShifts) + "|\n";
    result += "|No. of LevenbergMarquadt Steps to optimize constants each Optimization Step: " + QString::number(m_opt_config.LevMar_Constants_PerIter) + "|\n";
    result += "|No. of LevenbergMarquadt Steps to optimize shifts each Optimization Step: " + QString::number(m_opt_config.LevMar_Shifts_PerIter) + "|\n";
    result += "\n";
    result += "|********************LevenbergMarquadt Configuration********************************|\n";
    result += "|scale factor for initial \\mu {opts[0]}}" + QString::number(m_opt_config.LevMar_mu) + "|\n";
    result += "|stopping thresholds for ||J^T e||_inf, \\mu = {opts[1]}" + QString::number(m_opt_config.LevMar_Eps1) + "|\n";
    result += "|stopping thresholds for ||Dp||_2 = {opts[2]}" +  QString::number(m_opt_config.LevMar_Eps2) + "|\n";
    result += "|stopping thresholds for ||e||_2 = {opts[3]}" + QString::number(m_opt_config.LevMar_Eps3) + "|\n";
    result += "|step used in difference approximation to the Jacobian: = {opts[4]}" + QString::number(m_opt_config.LevMar_Delta) + "|\n";
    result += "|********************LevenbergMarquadt Configuration********************************|\n";
    result += "\n";
    return result;
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


QVector<qreal> AbstractTitrationModel::Minimize()
{ 
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_repaint = false;
    QVector<qreal> constants = Constants();
    QVector<qreal> old_para_constant = Constants();
    
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    QString OptPara;
    OptPara += "Starting Optimization Run for " + m_name +"\n";
    if(m_inform_config_changed)
    {
        OptPara += OptPara2String();
        m_inform_config_changed = false;
    }
    emit Message(OptPara, 2);
    bool convergence = false;
    bool constants_convergence = false;
    //     bool shift_convergence = false;
    bool error_convergence = false;
    int iter = 0;
    bool allow_loop = true;
    bool process_stopped = false;
    while((allow_loop && !convergence))
    {
        iter++;   
        if(iter > m_opt_config.MaxIter - 1)
            allow_loop = false;
        //         QApplication::processEvents();
        emit Message("***** Begin iteration " + QString::number(iter) + "\n", 4);
        QVector<qreal > old_constants = constants;
        qreal old_error = 0;
        for(int z = 0; z < MaxVars(); ++z)
            old_error += SumOfErrors(z);
        
        
        MinimizingComplexConstants(this, m_opt_config.LevMar_Constants_PerIter, constants, m_opt_config);
        MiniShifts(); 
        
        qreal error = 0;
        for(int z = 0; z < MaxVars(); ++z)
            error += SumOfErrors(z);
        
        qreal constant_diff = 0;
        QString constant_string;
        for(int z = 0; z < constants.size(); ++z)
        {
            if(constants[z] < 0)
            {
                emit Message("*** Something quite seriosly happend to the complexation constant. ***\n", 4);
                emit Message("*** At least one fall below zero, will stop optimization now and restore old values. ***\n", 4);
                if(isCorrupt())
                    emit Message("*** Calculated signals seems corrupt (infinity or not even a number (nan)). ***\n", 4);
                emit Warning("Something quite seriosly happend to the complexation constant.\nAt least one fall below zero, will stop optimization now and restore old values.", 0);
                allow_loop = false;
                process_stopped = true;
                break;
            }
            constant_diff += qAbs(old_constants[z] - constants[z]);
            constant_string += QString::number(constants[z]) + " ** ";
        }
        
        if(constant_diff < m_opt_config.Constant_Convergence)
        {
            if(!constants_convergence)
                emit Message("*** Change in complexation constants signaling convergence! ***", 3);
            constants_convergence = true;
        }
        else
            constants_convergence = false;
        
        if(qAbs(error - old_error) < m_opt_config.Error_Convergence)
        {
            if(!error_convergence)
                emit Message("*** Change in sum of error signaling convergence! ***", 3);
            error_convergence = true;
        }
        else
            error_convergence = false;
        emit Message("*** Change in complexation constant " + QString::number(constant_diff) + " | Convergence at "+ QString::number(m_opt_config.Constant_Convergence)+ " ***\n", 4);
        emit Message("*** New resulting contants " + constant_string + "\n", 5);
        
        emit Message("*** Change in error for model " + QString::number(qAbs(error - old_error)) + " | Convergence at "+ QString::number(m_opt_config.Error_Convergence)+"***\n", 4);
        
        emit Message("*** New resulting error " + QString::number(error) + "\n", 5);
        convergence = error_convergence & constants_convergence;
        
        emit Message("***** End iteration " + QString::number(iter) + "\n", 6);
    } 
    
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit Message("Full calculation took  " + QString::number(t1-t0) + " msecs", 3);
    if(!convergence && !process_stopped)
        emit Warning("Optimization did not convergence within " + QString::number(iter) + " cycles, sorry", 1);
    if(process_stopped)
    {
        setConstants(old_para_constant);
        constants = old_para_constant;
    }else{
        emit Message("*** Finished after " + QString::number(iter) + " cycles.***", 2);
        emit Message("*** Convergence reached  " + bool2YesNo(convergence) + "  ****\n", 3);
        setConstants(constants);
        
        QString message = "Using Signals";
        qreal error = 0;
        for(int i = 0; i < ActiveSignals().size(); ++i)
            if(ActiveSignals()[i])
            {
                message += " " + QString::number(i + 1) + " ";
                error += SumOfErrors(i);
            }
            message += "got results: ";
        for(int i = 0; i < Constants().size(); ++i)
            message += "Constant "+ QString(i)+ " " +QString::number(Constants()[i]) +" ";
        message += "Sum of Error is " + QString::number(error);
        message += "\n";
        Message(message, 2);
        
        m_repaint = true;
        CalculateSignal();
    }
    
    QApplication::restoreOverrideCursor();
    return constants;
}


QJsonObject AbstractTitrationModel::ExportJSON(bool IncludeLevelName) const
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
    
    toplevel[m_name] = json;
    if(IncludeLevelName)
        return toplevel;
    else
        return json;
}

void AbstractTitrationModel::ImportJSON(const QJsonObject &topjson)
{
    QJsonObject json;
    qDebug() << topjson.keys();
    qDebug() << QJsonDocument(topjson).toJson();
    if(topjson.contains(m_name))
        json = topjson[m_name].toObject();
    else
    {
        qWarning() << "file doesn't contain any " + m_name;
        return;
    }
    QVector<int > active_signals = QVector<int>(SignalCount(), 0);
    json = topjson[m_name].toObject();
    qDebug() << QJsonDocument(json).toJson();
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

void AbstractTitrationModel::LoadJSON(const QJsonObject &json)
{
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
        if(!pureShiftObject[QString::number(i)].isNull())
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
            if(!object[QString::number(i)].isUndefined())
            {
                shifts << object[QString::number(j)].toString().toDouble();
            }
        }
        setComplexSignals(shifts, i);
        
    }
    setActiveSignals(active_signals);
}

#include "AbstractModel.moc"
