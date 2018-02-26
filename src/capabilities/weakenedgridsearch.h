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

#pragma once

#include "abstractsearchclass.h"
#include "src/global.h"
#include "src/core/AbstractModel.h"

#include <QtCore/QRunnable>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>


class Minimizer;
class QPointF;

class WGSConfig : public AbstractConfig
{
public:
    qreal increment = 1e-3;
    int maxsteps = 1e4;
    qreal maxerror = 0;
    qreal confidence = 95;
    qreal f_value = 0;
    qreal error_conv = 1e-6;
    bool relax = true;
    bool fisher_statistic = false;
    QList<int> global_param, local_param;
};


class WGSearchThread : public AbstractSearchThread
{
 Q_OBJECT

  public:
    WGSearchThread(const WGSConfig &config);
    ~WGSearchThread();

    void setRange(double start, double end) { m_start = start; m_end = end; }
    inline void setParameterId(int index) { m_index = index; }

    virtual void run() override;

    inline QList<qreal> XSeries() const { return m_x; }
    inline QList<qreal> YSeries() const { return m_y; }
    inline QHash<qreal, QJsonObject> IntermediateResults() const { return m_models; }
private:
    void Calculate();

    double m_start, m_end;
    int m_index, m_steps;
    QHash<double, QJsonObject> m_models;
    QList<qreal > m_x, m_y;
    WGSConfig m_config;
    QJsonObject m_result;
    qreal m_error;
    QSharedPointer<Minimizer> m_minimizer;
    bool m_stationary, m_finished, m_converged;
};

class WeakenedGridSearchThread : public AbstractSearchThread
{
  Q_OBJECT
public:

    WeakenedGridSearchThread(const WGSConfig &config, bool check_convergence = true);
    ~WeakenedGridSearchThread();
    inline void SetParameterID( int id ) { m_parameter_id = id; }
    inline void setOptimizationRun(OptimizationType runtype) { m_type = runtype; }
    void setParameter(const QJsonObject &json);
    virtual void run();
    inline QJsonObject Result() const { return m_result; }
    inline bool Converged() const { return m_converged; }
    inline QJsonObject Model() const { return m_model->ExportModel(); }
    
    inline QList<qreal> XSeries() const { return m_x; }
    inline QList<qreal> YSeries() const { return m_y; }
    
private:
    qreal SumErrors(bool direction, double &integ_5, double &integ_1, QList<QPointF> &series);
    QSharedPointer<Minimizer> m_minimizer;
    OptimizationType m_type;
    int m_parameter_id;
    QJsonObject m_result;
    qreal m_error;
    bool m_converged, m_check_convergence;
    WGSConfig m_config;
    bool allow_break;
    QList<qreal > m_x, m_y;
};

class WeakenedGridSearch : public AbstractSearchClass
{
    Q_OBJECT
    
public:
    WeakenedGridSearch(const WGSConfig &config, QObject *parent = 0);
    ~WeakenedGridSearch();
    inline void setConfig(const WGSConfig &config) { m_config = config;}
    inline bool CV() { return m_cv; }
    inline void setOptimizationRun(OptimizationType runtype) { m_type = runtype; }
    bool ConfidenceAssesment();
    void setParameter(const QJsonObject &json);
   
public slots:
    virtual void Interrupt() override;
    
private:
    QSharedPointer<Minimizer> m_minimizer;
    OptimizationType m_type;
    WGSConfig m_config;
    bool allow_break, m_cv;
    QHash<QString, QList<qreal> > ConstantsFromThreads(QList< QPointer< WeakenedGridSearchThread > > &threads, bool store = false);
    QVector<QVector <qreal > > MakeBox() const;
    void MCSearch(const QVector<QVector<qreal> > &box);
    void Search(const QVector<QVector<qreal> > &box);
    void StripResults(const QList<QJsonObject > &results);
    virtual QJsonObject Controller() const override;

signals:
    void StopSubThreads();
};

