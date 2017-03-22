/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#ifndef MONTECARLOSTATISTICS_H
#define MONTECARLOSTATISTICS_H
#include "src/core/models.h"

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

class Minimizer;
class QThreadPool;

struct MCConfig
{
    int maxsteps = 1000;
    double variance = 1e-2;
    bool original = false;
    bool bootstrap = false;
    OptimizerConfig optimizer_config;
    OptimizationType runtype;
};

class MonteCarloThread : public QObject, public QRunnable
{
 Q_OBJECT
 
public:
    MonteCarloThread(const MCConfig &config, QObject *parent = 0);
    ~MonteCarloThread();
    inline void setModel(const QSharedPointer<AbstractTitrationModel> model) { m_model = model->Clone(); }
    virtual void run();
    inline QJsonObject OptimizedParameter() const { return m_optimized; }
    inline QList<qreal > Constants() const { return m_constants; }
    void setDataTable(DataTable *table);
    inline QJsonObject Model() const { return m_model->ExportJSON(); }
private:
    QSharedPointer<Minimizer> m_minimizer;
    QSharedPointer<AbstractTitrationModel> m_model;
    QJsonObject m_optimized;
    MCConfig m_config;
    QList<qreal > m_constants;
    
signals:
    void IncrementProgress(int time);
};

class MonteCarloStatistics : public QObject
{
    Q_OBJECT
public:
    MonteCarloStatistics(const MCConfig &config, QObject *parent = 0);
    ~MonteCarloStatistics();
    
    inline void setModel(const QSharedPointer<AbstractTitrationModel> model) { m_model = model->Clone(); }
    inline void setConfig(const MCConfig &config) { m_config = config; }
    void Evaluate();
    void ExportResults(const QString &filename);
     
    inline QList<QJsonObject > getResult() const { return m_mc_results; }
    inline QList<QList<QPointF> > getSeries() const { return m_series; }
    inline QList<QJsonObject > Models() const { return m_models; }
    
   
    
public slots:
    void Interrupt();
    void AnalyseData(qreal error = 5);
    
private:
    QVector<QPointer <MonteCarloThread > > GenerateData();
    void Collect(const QVector<QPointer <MonteCarloThread > > &threads);
    
    void ExtractFromJson(int i, const QString &string);
    QJsonObject MakeJson(QList<qreal > &list, qreal error);
    QSharedPointer<AbstractTitrationModel> m_model;
    QThreadPool *m_threadpool;
    QList<QList<QPointF> > m_series;
    QList<QJsonObject > m_mc_results;
    QList<QJsonObject > m_models;
    std::mt19937 rng;
    std::normal_distribution<double> Phi;
    std::uniform_int_distribution<int> Uni;
    DataTable *m_table;
    MCConfig m_config;
    QVector<QList<qreal > > m_constant_list, m_shift_list;
    bool m_generate;
    int m_steps;

signals:
    void IncrementProgress(int time);
    void AnalyseFinished();
};

#endif // MONTECARLOSTATISTICS_H
