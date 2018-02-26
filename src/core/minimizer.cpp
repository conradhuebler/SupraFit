/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QCoreApplication>
#include <QtCore/QDateTime>

#include "minimizer.h"
NonLinearFitThread::NonLinearFitThread(bool exchange_statistics): m_exc_statistics(exchange_statistics), m_runtype(OptimizationType::ComplexationConstants)
{
  setAutoDelete(false);  
  connect(this, SIGNAL(Message(QString, int)), this, SLOT(Print(QString)));
}

NonLinearFitThread::~NonLinearFitThread()
{
    
    
}

void NonLinearFitThread::run()
{
    m_steps = 0;
    m_converged = false;
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    NonLinearFit();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit finished(t1-t0);
}


void NonLinearFitThread::setModel(const QSharedPointer<AbstractModel> model)
{
    m_model = model->Clone();
    m_model->Calculate();
    m_best_intermediate = m_model->ExportModel(m_exc_statistics);
    m_last_parameter = m_model->ExportModel(m_exc_statistics);
    m_model->setLockedParameter(model->LockedParamters());
    connect(m_model.data(), SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(m_model.data(), SIGNAL(Warning(QString, int)), this, SIGNAL(Warning(QString, int)), Qt::DirectConnection);
}

void NonLinearFitThread::setParameter(const QJsonObject &json)
{
     m_model->ImportModel(json);
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
    m_sum_error = m_model->SumofSquares();
    m_last_parameter = m_model->ExportModel(m_exc_statistics);
    m_best_intermediate = m_model->ExportModel(m_exc_statistics);
    if(iter < m_model.data()->getOptimizerConfig().MaxIter)
        m_converged = true;
    else
        m_converged = false;
    return iter;
}

void NonLinearFitThread::Print(const QString& message)
{
#ifdef _DEBUG
//     qDebug() << message;
#endif
}



Minimizer::Minimizer(bool exchange_statistics, QObject* parent) : QObject(parent), m_exc_statistics(exchange_statistics),m_inform_config_changed(true)
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
    NonLinearFitThread *thread = new NonLinearFitThread(m_exc_statistics);
    connect(thread, SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(thread, SIGNAL(Warning(QString, int)), this, SIGNAL(Warning(QString, int)), Qt::DirectConnection);
    thread->setModel(m_model);
    thread->setOptimizationRun(runtype);
    thread->run();  
    bool converged = thread->Converged();
    if(converged)
        m_last_parameter = thread->ConvergedParameter();
    else
        m_last_parameter = thread->BestIntermediateParameter();
    m_sum_error = thread->SumOfError();
    delete thread;
    m_model->ImportModel(m_last_parameter);
    emit RequestRemoveCrashFile();
    addToHistory();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit Message("Full calculation took  " + QString::number(t1-t0) + " msecs", 3);
    return converged;
}

QPointer<NonLinearFitThread> Minimizer::addJob(const QSharedPointer<AbstractModel> model, OptimizationType runtype, bool start)
{
    QPointer<NonLinearFitThread> thread = new NonLinearFitThread(m_exc_statistics);
    thread->setModel(model);
    thread->setOptimizationRun(runtype);
    if(start)
        QThreadPool::globalInstance()->start(thread);
    else
        emit thread->finished(1);
    return thread;
}


void Minimizer::setModel(const QSharedPointer<AbstractModel> model)
{
    m_model = model;
}

void Minimizer::setModelCloned(const QSharedPointer<AbstractModel> model)
{
    m_model = model->Clone();
}

void Minimizer::setParameter(const QJsonObject& json, const QList<int> &locked)
{
    m_model->ImportModel(json);
    m_model->setLockedParameter(locked);
}

void Minimizer::setParameter(const QJsonObject& json)
{
    m_model->ImportModel(json);
}

void Minimizer::addToHistory()
{
    QJsonObject model = m_model->ExportModel();
    int active = 0;
    for(int i = 0; i < m_model->ActiveSignals().size(); ++i)
        active += m_model->ActiveSignals()[i];
    emit InsertModel(model, active);
}

#include "minimizer.moc"
