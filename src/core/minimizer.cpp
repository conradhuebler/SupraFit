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
#include <QtCore/QTimer>

#include "minimizer.h"
NonLinearFitThread::NonLinearFitThread(bool exchange_statistics)
    : m_exc_statistics(exchange_statistics)
{
    setAutoDelete(false);
    connect(this, SIGNAL(Message(QString, int)), this, SLOT(Print(QString)));
}

NonLinearFitThread::~NonLinearFitThread()
{
    m_model.clear();
}

void NonLinearFitThread::start()
{
    m_running = true;
    QTimer::singleShot(0, this, &NonLinearFitThread::run);
}

void NonLinearFitThread::run()
{
    m_running = true;
    m_steps = 0;
    m_converged = false;
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    NonLinearFit();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit finished(t1 - t0);
    m_running = false;
}

void NonLinearFitThread::setModel(const QSharedPointer<AbstractModel> model, bool clone)
{
    if (clone) {
        m_model = model->Clone();
        m_model->setDescription("Optimiser Model");
    } else
        m_model = model;
    m_model->Calculate();
    m_best_intermediate = m_model->ExportModel(m_exc_statistics);
    m_last_parameter = m_model->ExportModel(m_exc_statistics);
    m_model->setLockedParameter(model->LockedParameters());
    connect(m_model.data(), SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(m_model.data(), SIGNAL(Warning(QString, int)), this, SIGNAL(Warning(QString, int)), Qt::DirectConnection);
}

void NonLinearFitThread::setParameter(const QJsonObject& json)
{
    m_model->ImportModel(json);
}

int NonLinearFitThread::NonLinearFit()
{
    QList<int> locked = m_model->LockedParameters();
    QVector<qreal> parameter = m_model->OptimizeParameters();
    if (parameter.isEmpty())
        return 0;
    if (locked.size() == parameter.size())
        m_model->setLockedParameter(locked);
    int iter = NonlinearFit(m_model, parameter);
    m_sum_error = m_model->SumofSquares();
    m_last_parameter = m_model->ExportModel(m_exc_statistics);
    m_best_intermediate = m_model->ExportModel(m_exc_statistics);
    m_converged = (iter < m_model.data()->getOptimizerConfig()["MaxLevMarInter"].toInt());

    return iter;
}

void NonLinearFitThread::Print(const QString& message)
{
#ifdef _DEBUG
//    qDebug() << message;
#else
    Q_UNUSED(message)
#endif
}

Minimizer::Minimizer(bool exchange_statistics, QObject* parent)
    : QObject(parent)
    , m_exc_statistics(exchange_statistics)
    , m_inform_config_changed(true)
{
}

Minimizer::~Minimizer()
{
    // m_model.clear();
}

QString Minimizer::OptPara2String() const
{
    QString result;
    /*
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
    result += "|Minipack Gtol" + QString::number(m_opt_config.LevMar_Gtol) + "|\n";
    result += "|Minipack Ftol" + QString::number(m_opt_config.LevMar_Ftol) + "|\n";
    result += "|Minipack epsfcn" + QString::number(m_opt_config.LevMar_epsfcn) + "|\n";
    result += "|********************LevenbergMarquadt Configuration********************************|\n";
    result += "\n";*/
    return result;
}

int Minimizer::Minimize(const QList<int>& locked)
{
    m_model->setLockedParameter(locked);
    return Minimize();
}

int Minimizer::Minimize()
{
    emit RequestCrashFile();
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    QString OptPara;
    OptPara += "Starting Optimization Run for " + m_model->Name() + "\n";
    if (m_inform_config_changed) {
        OptPara += OptPara2String();
        m_inform_config_changed = false;
    }
    emit Message(OptPara, 2);
    NonLinearFitThread* thread = new NonLinearFitThread(m_exc_statistics);
    connect(thread, SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(thread, SIGNAL(Warning(QString, int)), this, SIGNAL(Warning(QString, int)), Qt::DirectConnection);
    thread->setModel(m_model);
    //thread->run();
    thread->start();
    while (thread->Running())
        QCoreApplication::processEvents();
    bool converged = thread->Converged();
    if (converged)
        m_last_parameter = thread->ConvergedParameter();
    else
        m_last_parameter = thread->BestIntermediateParameter();
    m_sum_error = thread->SumOfError();
    delete thread;
    m_model->ImportModel(m_last_parameter);
    emit RequestRemoveCrashFile();
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    emit Message("Full calculation took  " + QString::number(t1 - t0) + " msecs", 3);
    return converged;
}

QPointer<NonLinearFitThread> Minimizer::addJob(const QSharedPointer<AbstractModel> model, bool start)
{
    QPointer<NonLinearFitThread> thread = new NonLinearFitThread(m_exc_statistics);
    thread->setModel(model);
    if (start)
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

void Minimizer::setParameter(const QJsonObject& json, const QList<int>& locked)
{
    m_model->ImportModel(json);
    m_model->setLockedParameter(locked);
}

void Minimizer::setParameter(const QJsonObject& json)
{
    m_model->ImportModel(json);
}

#include "minimizer.moc"
