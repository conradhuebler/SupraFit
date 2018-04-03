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
#include "src/core/AbstractModel.h"
#include "src/global.h"

#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>

class Minimizer;
class QPointF;

class WGSConfig : public AbstractConfig {
public:
    qreal increment = 0;
    int maxsteps = 1e4;
    qreal maxerror = 0;
    qreal confidence = 95;
    qreal f_value = 0;
    qreal error_conv = 1e-10;
    bool relax = true;
    bool fisher_statistic = false;
    QList<int> global_param, local_param;
};

class WGSearchThread : public AbstractSearchThread {
    Q_OBJECT

public:
    WGSearchThread(const WGSConfig& config);
    ~WGSearchThread();

    inline void setParameterId(int index) { m_index = index; }

    virtual void run() override;
    inline void setUp() { m_direction = +1.0; }
    inline void setDown() { m_direction = -1.0; }
    inline float Direction() const { return m_direction; }
    inline int ParameterId() const { return m_index; }
    inline QList<qreal> XSeries() const { return m_x; }
    inline QList<qreal> YSeries() const { return m_y; }
    inline QHash<qreal, QJsonObject> IntermediateResults() const { return m_models; }
    inline QJsonObject Result() const { return m_result; }
    inline bool Converged() const { return m_converged; }
    inline qreal Last() const { return m_last; }
    inline void setIncrement(qreal increment) { m_increment = increment; }
    void setModel(QSharedPointer<AbstractModel> model) { m_model = model->Clone(); }

private:
    void Calculate();
    double m_last;
    int m_index, m_steps;
    float m_direction;
    QHash<double, QJsonObject> m_models;
    QList<qreal> m_x, m_y;
    WGSConfig m_config;
    QJsonObject m_result;
    qreal m_error, m_increment;
    bool m_stationary, m_finished, m_converged;
};

class WeakenedGridSearch : public AbstractSearchClass {
    Q_OBJECT

public:
    WeakenedGridSearch(const WGSConfig& config, QObject* parent = 0);
    ~WeakenedGridSearch();
    inline void setConfig(const WGSConfig& config) { m_config = config; }
    inline bool CV() { return m_cv; }
    inline void setOptimizationRun(OptimizationType runtype) { m_type = runtype; }
    bool ConfidenceAssesment();
    void setParameter(const QJsonObject& json);

public slots:
    virtual void Interrupt() override;

private:
    QPointer<WGSearchThread> CreateThread(int index, bool direction);
    OptimizationType m_type;
    WGSConfig m_config;
    bool allow_break, m_cv;
    QVector<QVector<qreal>> MakeBox() const;
    void MCSearch(const QVector<QVector<qreal>>& box);
    void Search(const QVector<QVector<qreal>>& box);
    void StripResults(const QList<QJsonObject>& results);
    virtual QJsonObject Controller() const override;

signals:
    void StopSubThreads();
};
