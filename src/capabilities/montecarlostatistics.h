/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include "abstractsearchclass.h"

#include "src/core/models.h"

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

class Minimizer;
class QThreadPool;

class MCConfig : public AbstractConfig
{
public:
    int maxsteps = 1000;
    double variance = 1e-2;
    bool original = false;
    bool bootstrap = false;
    QVector<qreal > indep_variance;
};

class MonteCarloThread : public AbstractSearchThread
{
 Q_OBJECT
 
public:
    MonteCarloThread(const MCConfig &config);
    MonteCarloThread();
    ~MonteCarloThread();
    virtual void run();
    inline QJsonObject OptimizedParameter() const { return m_optimized; }
    inline QList<qreal > Constants() const { return m_constants; }
    void setDataTable(DataTable *table);
    inline QJsonObject Model() const { return m_model->ExportModel(); }
    inline bool Finished() const { return m_finished; }
    inline void setIndex(int index) { m_index = index; }
    inline int Index() const { return m_index; }
    
private:
    QSharedPointer<Minimizer> m_minimizer;
    QJsonObject m_optimized;
    MCConfig m_config;
    QList<qreal > m_constants;
    bool m_finished;
    int m_index;
};

class MonteCarloStatistics : public AbstractSearchClass
{
    Q_OBJECT
public:
    MonteCarloStatistics(const MCConfig &config, QObject *parent = 0);
    ~MonteCarloStatistics();
    
    inline void setConfig(const MCConfig &config) { m_config = config; }
    void Evaluate();

public slots:
    void Interrupt() override;
    
private:
    QVector<QPointer <MonteCarloThread > > GenerateData();
    void Collect(const QVector<QPointer <MonteCarloThread > > &threads);
    virtual QJsonObject Controller() const override;
    std::mt19937 rng;
    std::normal_distribution<double> Phi;
    std::uniform_int_distribution<int> Uni;
    DataTable *m_table;
    MCConfig m_config;
    bool m_generate;
    int m_steps;
};
