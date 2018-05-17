/*
 * This file handles all optimization functions
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

#pragma once

#include "src/global.h"

#include "src/core/AbstractModel.h"

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QSharedPointer>
#include <QtCore/QThreadPool>

class AbstractModel;

class NonLinearFitThread : public QObject, public QRunnable {
    Q_OBJECT

public:
    NonLinearFitThread(bool exchange_statistics = true);
    ~NonLinearFitThread();
    void setModel(const QSharedPointer<AbstractModel> model, bool clone = true);

    QSharedPointer<AbstractModel> Model() const { return m_model; }
    inline void setOptimizationRun(OptimizationType runtype) { m_runtype = runtype; }
    virtual void run();
    inline QJsonObject ConvergedParameter() { return m_last_parameter; }
    inline QJsonObject BestIntermediateParameter() const { return m_best_intermediate; }
    void setParameter(const QJsonObject& json);
    inline void setOptimizerConfig(const OptimizerConfig& config) { m_opt_config = config; }
    inline bool Converged() const { return m_converged; }
    inline qreal SumOfError() const { return m_sum_error; }

private:
    QSharedPointer<AbstractModel> m_model;
    QJsonObject m_last_parameter, m_best_intermediate;
    int NonLinearFit(OptimizationType runtype);
    inline int NonLinearFit() { return NonLinearFit(m_runtype); }
    OptimizerConfig m_opt_config;
    bool m_converged;
    int m_steps;
    OptimizationType m_runtype;
    bool m_exc_statistics;
    qreal m_sum_error;

signals:
    void Message(const QString& str, int priority);
    void Warning(const QString& str, int priority);
    void finished(int msecs);

private slots:
    void Print(const QString& message);
};

class Minimizer : public QObject {
    Q_OBJECT
public:
    Minimizer(bool exchange_statistics = true, QObject* parent = 0);
    ~Minimizer();
    void setModel(const QSharedPointer<AbstractModel> model);
    void setModelCloned(const QSharedPointer<AbstractModel> model);
    int Minimize(OptimizationType runtype);
    int Minimize(OptimizationType runtype, const QList<int>& locked);
    void setOptimizerConfig(const OptimizerConfig& config)
    {
        m_opt_config = config;
        m_inform_config_changed = true;
    }
    inline OptimizerConfig getOptimizerConfig() const { return m_opt_config; }
    void addToHistory();
    inline QJsonObject Parameter() const { return m_last_parameter; }
    void setParameter(const QJsonObject& json, const QList<int>& locked);
    void setParameter(const QJsonObject& json);
    QPointer<NonLinearFitThread> addJob(const QSharedPointer<AbstractModel> model, OptimizationType runtype, bool start = true);
    inline qreal SumOfError() const { return m_sum_error; }
    inline QSharedPointer<AbstractModel> Model() const { return m_model; }

private:
    QSharedPointer<AbstractModel> m_model;
    OptimizerConfig m_opt_config;
    bool m_inform_config_changed;
    QString OptPara2String() const;
    QJsonObject m_last_parameter;
    bool m_exc_statistics;
    qreal m_sum_error;

signals:
    void Message(const QString& str, int priority);
    void Warning(const QString& str, int priority);
    void RequestCrashFile();
    void RequestRemoveCrashFile();
    void InsertModel(const QJsonObject& model, int active);
};
