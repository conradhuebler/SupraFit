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

#include "src/core/AbstractModel.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "src/ui/widgets/modelhistorywidget.h"

#include <QApplication>
#include <QtCore/QDateTime>

#include "minimizer.h"
NonLinearFitThread::NonLinearFitThread()
{
  setAutoDelete(false);  
    
}

NonLinearFitThread::~NonLinearFitThread()
{
    
    
}

void NonLinearFitThread::run()
{
    FastFit();
}


void NonLinearFitThread::setModel(const QSharedPointer<AbstractTitrationModel> model)
{
    m_model = model->Clone();
    connect(m_model.data(), SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(m_model.data(), SIGNAL(Warning(QString, int)), this, SIGNAL(Warning(QString, int)), Qt::DirectConnection);
}

void NonLinearFitThread::setParameter(const QJsonObject &json)
{
     m_model->ImportJSON(json);
}


void NonLinearFitThread::FastFit()
{
    int result = -1;
    QVector<qreal> constants = m_model->Constants();
    QVector<qreal> old_para_constant = m_model->Constants();
    

    bool convergence = false;
    bool constants_convergence = false;
    bool error_convergence = false;
    int iter = 0;
    bool allow_loop = true;
    bool process_stopped = false;
    int max_convergence = 0;
    while((allow_loop && !convergence))
    {
        iter++;   
        if(iter > m_opt_config.MaxIter - 1)
            allow_loop = false;

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        emit Message("***** Begin iteration " + QString::number(iter) + "\n", 4);
        QVector<qreal > old_constants = constants;
        qreal old_error = m_model->ModelError();
        
        m_opt_config.LevMar_Constants_PerIter = 10;
        qDebug() << "the first input" << m_model->ExportJSON() << constants;
        max_convergence += MinimizingComplexConstants(m_model, m_opt_config.LevMar_Constants_PerIter, constants, m_opt_config);
        m_model->setOptParamater(constants);
        m_model->MiniShifts(); 
        QVector<qreal *> optconst = m_model->getOptConstants();
        QVector<qreal > blub;
        for(int i = 0; i < optconst.size(); ++i)
            blub << *optconst[i];
        if(blub.size() > 0)
             MinimizingComplexConstants(m_model, m_opt_config.LevMar_Constants_PerIter, blub, m_opt_config);
        m_model->setOptParamater(constants);
//         if(max_convergence == 30)
//             allow_loop = false;
        qreal error = m_model->ModelError();
        
        qreal constant_diff = 0;
        QString constant_string;
        for(int z = 0; z < constants.size(); ++z)
        {
            if(constants[z] < 0)
            {
                emit Message("*** Something quite seriosly happend to the complexation constant. ***\n", 4);
                emit Message("*** At least one fall below zero, will stop optimization now and restore old values. ***\n", 4);
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
    qDebug() << constants << "after optimization";
    
    if(!convergence && !process_stopped)
        emit Warning("Optimization did not convergence within " + QString::number(iter) + " cycles, sorry", 1);
    if(process_stopped)
    {
        m_model->setConstants(old_para_constant);
        constants = old_para_constant;
    }else{
        emit Message("*** Finished after " + QString::number(iter) + " cycles.***", 2);
        emit Message("*** Convergence reached  " + ToolSet::bool2YesNo(convergence) + "  ****\n", 3);
        m_model->setConstants(constants);
        
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
        Message(message, 2);
        
//         m_repaint = true;
//         m_model->CalculateSignal();
        result = 1;
        
    }
}

void NonLinearFitThread::DifferenceFitSignalConstants()
{
}

void NonLinearFitThread::NonLinearFitComplexConstants()
{
}

void NonLinearFitThread::NonLinearFitSignalConstants()
{
}



QJsonObject NonLinearFitThread::Parameter() const
{
    return m_model->ExportJSON();
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
    result += "|Shifts will be optimized for zero and saturation concentration: " + ToolSet::bool2YesNo(m_opt_config.OptimizeBorderShifts) + "|\n";
    result += "|Shifts will be optimized for any other concentration: " + ToolSet::bool2YesNo(m_opt_config.OptimizeIntermediateShifts) + "|\n";
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

int Minimizer::Minimize()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
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
    QThreadPool *threadpool = QThreadPool::globalInstance();
    threadpool->start(thread);
    if(!threadpool->waitForDone())
    {
     qDebug() << "wired happend";   
    }
    m_last_parameter= thread->Parameter();
    delete thread;
    emit RequestRemoveCrashFile();
    addToHistory();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit Message("Full calculation took  " + QString::number(t1-t0) + " msecs", 3);
    
    QApplication::restoreOverrideCursor();
}

void Minimizer::setModel(const QSharedPointer<AbstractTitrationModel> model)
{
    m_model = model;
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
//     m_history[m_history.size()] = element;
}

#include "minimizer.moc"
