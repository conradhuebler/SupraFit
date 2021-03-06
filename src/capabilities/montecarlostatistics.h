/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "abstractsearchclass.h"

#include "src/core/models/models.h"

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

class Minimizer;
class MonteCarloStatistics;
class NonLinearFitThread;
class QThreadPool;

class MonteCarloThread : public AbstractSearchThread {
    Q_OBJECT

public:
    MonteCarloThread();
    virtual ~MonteCarloThread() override;
    virtual void run() override;
    inline QJsonObject OptimizedParameter() const { return m_optimized; }
    void setDataTable(QPointer<DataTable> table);
    void setIndepTable(QPointer<DataTable> table);
    inline QJsonObject Model() const { return m_model->ExportModel(); }
    inline bool Finished() const { return m_finished; }
    inline void setIndex(int index) { m_index = index; }
    inline int Index() const { return m_index; }

private:
    NonLinearFitThread* m_fit_thread;

    QJsonObject m_optimized;
    QJsonObject m_config;
    bool m_finished;
    int m_index;
};

class MonteCarloBatch : public AbstractSearchThread {
    Q_OBJECT

public:
    MonteCarloBatch(QPointer<AbstractSearchClass> parent);
    virtual ~MonteCarloBatch() override;
    virtual void run() override;
    inline bool Finished() const { return m_finished; }
    inline void setChecked(bool checked) { m_checked = checked; }
    inline int Counter() { return m_counter; }
    inline int Timer() { return m_indiv_time; }

private:
    int optimise(int key = 0);
    NonLinearFitThread* m_fit_thread;

    QPointer<AbstractSearchClass> m_parent;
    bool m_finished, m_checked;
    QJsonObject m_controller;
    int m_counter = 0, m_indiv_time = 0;
};

class MonteCarloStatistics : public AbstractSearchClass {
    Q_OBJECT
public:
    MonteCarloStatistics(QObject* parent = 0);
    ~MonteCarloStatistics();

    virtual bool Run() override;

public slots:
    void Interrupt() override;

private:
    QVector<QPointer<MonteCarloBatch>> GenerateData();
    QVector<QPointer<DataTable>> m_ptr_table;
    void Collect(const QVector<QPointer<MonteCarloBatch>>& threads);

    std::mt19937 rng;
    std::normal_distribution<double> Phi;
    std::uniform_int_distribution<int> Uni;
    DataTable* m_table;
    bool m_generate;
    int m_steps;
    qint64 m_t0 = 0;
signals:
    void InterruptAll();
};
