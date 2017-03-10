/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/global_config.h"

#include "src/core/AbstractModel.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "src/ui/widgets/modelhistorywidget.h"

#include <QCoreApplication>
#include <QtCore/QDateTime>

#include "minimizer.h"
NonLinearFitThread::NonLinearFitThread():  m_runtype(OptimizationType::ComplexationConstants)
{
  setAutoDelete(false);  
    
}

NonLinearFitThread::~NonLinearFitThread()
{
    
    
}

void NonLinearFitThread::run()
{

    m_steps = 0;
    m_converged = false;
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
     if((m_runtype & OptimizationType::UnconstrainedShifts) == OptimizationType::UnconstrainedShifts)
        NonLinearFit();
    else if((m_runtype & OptimizationType::ConstrainedShifts) == OptimizationType::ConstrainedShifts)
        ConstrainedFit();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit finished(t1-t0);
}


void NonLinearFitThread::setModel(const QSharedPointer<AbstractTitrationModel> model)
{
    m_model = model->Clone();
    m_model->Calculate();
    m_best_intermediate = m_model->ExportJSON();
    m_last_parameter = m_model->ExportJSON();
    m_model->setLockedParameter(model->LockedParamters());
    connect(m_model.data(), SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(m_model.data(), SIGNAL(Warning(QString, int)), this, SIGNAL(Warning(QString, int)), Qt::DirectConnection);
}

void NonLinearFitThread::setParameter(const QJsonObject &json)
{
     m_model->ImportJSON(json);
}


void NonLinearFitThread::ConstrainedFit()
{   
    QList<qreal> old_para_constant = m_model->Constants();
    
    bool convergence = false;
    bool constants_convergence = false;
    bool error_convergence = false;
    int iter = 0;
    bool allow_loop = true;
    bool process_stopped = false;

    QList<int >locked = m_model->LockedParamters();
    QVector<qreal > parameter = m_model->OptimizeParameters(m_runtype);
    if(locked.size() == parameter.size())
        m_model->setLockedParameter(locked); 
    
    while((allow_loop && !convergence))
    {
        iter++;   
        if(iter > m_opt_config.MaxIter - 1)
            allow_loop = false;
        
        emit Message("***** Begin iteration " + QString::number(iter) + "\n", 4);
        QList<qreal > old_constants =  m_model->Constants();
        qreal old_error;
        if(m_opt_config.error_potenz == 2)
            old_error = m_model->SumofSquares();
        else
            old_error = m_model->SumofAbsolute();
        if(m_runtype & OptimizationType::ComplexationConstants)
        {
            if(NonLinearFit(OptimizationType::ComplexationConstants) == 1)
                m_model->ImportJSON(m_last_parameter);
        }
       
        m_model->MiniShifts(); 
        
        if(m_runtype & OptimizationType::IntermediateShifts)
        {
             if(NonLinearFit(OptimizationType::IntermediateShifts) == 1)
                 m_model->ImportJSON(m_last_parameter);
        }
        m_last_parameter = m_model->ExportJSON();
        qreal error;
        
        if(m_opt_config.error_potenz == 2)
            error = m_model->SumofSquares();
        else
            error = m_model->SumofAbsolute();
        
        QList<qreal> constants = m_model->Constants();
        qreal constant_diff = 0;
        QString constant_string;
        for(int z = 0; z < constants.size(); ++z)
        {
            if(constants[z] < 0)
            {
                emit Message("*** Something quite seriosly happend to the complexation constant. ***\n", 4);
                emit Message("*** At least one fall below zero, will stop optimization now and give the best intermediate result. ***\n", 4);
                if(m_model->isCorrupt())
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
        if(error < old_error)
            m_best_intermediate = m_model->ExportJSON();
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
        emit Message("***** End iteration " + QString::number(iter) + "\n", 6);
       
       convergence = error_convergence & constants_convergence;
    } 

    
    if(!convergence && !process_stopped)
        emit Warning("Optimization did not convergence within " + QString::number(iter) + " cycles, sorry", 1);
    if(process_stopped)
    {
        m_model->setConstants(old_para_constant);
    }else{
        emit Message("*** Finished after " + QString::number(iter) + " cycles.***", 2);
        emit Message("*** Convergence reached  " + ToolSet::bool2YesNo(convergence) + "  ****\n", 3);
        
        QString message = "Using Signals";
        qreal error = 0;
        for(int i = 0; i < m_model->ActiveSignals().size(); ++i)
            if(m_model->ActiveSignals()[i])
            {
                message += " " + QString::number(i + 1) + " ";
                error += m_model->SumOfErrors(i);
            }
            message += "got results: ";
        for(int i = 0; i < m_model->Constants().size(); ++i)
            message += "Constant "+ QString(i)+ " " +QString::number(m_model->Constants()[i]) +" ";
        message += "Sum of Error is " + QString::number(error);
        message += "\n";
        m_last_parameter = m_model->ExportJSON();
        m_converged = true;
        Message(message, 2);        
    }
}


int NonLinearFitThread::NonLinearFit(OptimizationType runtype)
{
    
    QList<int >locked = m_model->LockedParamters();
    QVector<qreal > parameter = m_model->OptimizeParameters(runtype);
    if(parameter.isEmpty())
        return 0;
    if(runtype == m_runtype)
    {
        if(locked.size() == parameter.size())
            m_model->setLockedParameter(locked); 
    }
    int iter = NonlinearFit(m_model, parameter);
    m_last_parameter = m_model->ExportJSON();
    m_converged = true;
    m_best_intermediate = m_model->ExportJSON();
    return iter;
}


Minimizer::Minimizer(QObject* parent) : QObject(parent), m_inform_config_changed(true)
{
}


Minimizer::~Minimizer()
{
}

QString Minimizer::OptPara2String() const
{
    QString result;
    result += "\n";
    result += "|***********************************************************************************|\n";
    result += "|********************General Config for Optimization********************************|\n";
    result += "|Maximal number of Iteration: " + QString::number(m_opt_config.MaxIter) + "|\n";
    result += "|No. of LevenbergMarquadt Steps to optimize constants each Optimization Step: " + QString::number(m_opt_config.LevMar_Constants_PerIter) + "|\n";
    result += "|No. of LevenbergMarquadt Steps to optimize shifts each Optimization Step: " + QString::number(m_opt_config.LevMar_Shifts_PerIter) + "|\n";
    result += "\n";
    result += "|********************LevenbergMarquadt Configuration********************************|\n";
    result += "|Minipack Factor " + QString::number(m_opt_config.LevMar_Factor) + "|\n";
    result += "|Minipack XTol" + QString::number(m_opt_config.LevMar_Xtol) + "|\n";
    result += "|Minipack Gtol" +  QString::number(m_opt_config.LevMar_Gtol) + "|\n";
    result += "|Minipack Ftol" + QString::number(m_opt_config.LevMar_Ftol) + "|\n";
    result += "|Minipack epsfcn" + QString::number(m_opt_config.LevMar_epsfcn) + "|\n";
    result += "|********************LevenbergMarquadt Configuration********************************|\n";
    result += "\n";
    return result;
}

int Minimizer::Minimize(OptimizationType runtype, const QList<int>& locked)
{
    m_model->setLockedParameter(locked);
    return Minimize(runtype);
}


int Minimizer::Minimize(OptimizationType runtype)
{
    
    emit RequestCrashFile();
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    QString OptPara;
    OptPara += "Starting Optimization Run for " + m_model->Name() +"\n";
    if(m_inform_config_changed)
    {
        OptPara += OptPara2String();
        m_inform_config_changed = false;
    }
    emit Message(OptPara, 2);
    NonLinearFitThread *thread = new NonLinearFitThread;
    connect(thread, SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(thread, SIGNAL(Warning(QString, int)), this, SIGNAL(Warning(QString, int)), Qt::DirectConnection);
    thread->setModel(m_model);
    thread->setOptimizationRun(runtype);
    thread->run();
    if(thread->Converged())
        m_last_parameter = thread->ConvergedParameter();
    else
        m_last_parameter = thread->BestIntermediateParameter();
    delete thread;
    m_model->ImportJSON(m_last_parameter);
    m_model->Calculate();
    emit RequestRemoveCrashFile();
    addToHistory();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit Message("Full calculation took  " + QString::number(t1-t0) + " msecs", 3);
    return 1;
}

QPointer<NonLinearFitThread> Minimizer::addJob(const QSharedPointer<AbstractTitrationModel> model, OptimizationType runtype, bool start)
{
    QPointer<NonLinearFitThread> thread = new NonLinearFitThread;
    thread->setModel(model);
    thread->setOptimizationRun(runtype);
    if(start)
        QThreadPool::globalInstance()->start(thread);
    else
        
    return thread;
}


void Minimizer::setModel(const QSharedPointer<AbstractTitrationModel> model)
{
    m_model = model;
}

void Minimizer::setModelCloned(const QSharedPointer<AbstractTitrationModel> model)
{
    m_model = model->Clone();
}

void Minimizer::setParameter(const QJsonObject& json, const QList<int> &locked)
{
    m_model->ImportJSON(json);
    m_model->setLockedParameter(locked);
}

void Minimizer::setParameter(const QJsonObject& json)
{
    m_model->ImportJSON(json);
}

void Minimizer::addToHistory()
{
    QJsonObject model = m_model->ExportJSON();
    ModelHistoryElement element;
    element.model = model;
    element.active_signals = m_model->ActiveSignals();
    qreal error = m_model->ModelError();
    
    element.error = error;
    emit InsertModel(element);
}

#include "minimizer.moc"
