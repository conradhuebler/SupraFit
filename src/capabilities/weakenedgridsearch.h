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
    /* Step length for numerical evaluation */
    qreal increment = 0;

    /* Maximal number of steps to be evaluated */
    int maxsteps = 1e3;

    /* SSE threshold defined by f-Statistics */
    qreal maxerror = 0;

    /* Confidence in % */
    qreal confidence = 95;

    /* Corresponding f Value */
    qreal f_value = 0;

    /* Threshold for convergency in SSE */
    qreal ErrorConvergency = 1e-10;

    /* Maximal steps which are allowed to be above the SSE threshold, while continueing the evaluation */
    int OvershotCounter = 5;

    /* Maximal number of steps, where the error is allowed to decrease, analyse not-converged systems, flat surfaces */
    int ErrorDecreaseCounter = 50;

    /* Amount for all error changes below the threshold error_conv, while keeping the evaluation running */
    int ErrorConvergencyCounter = 5;

    /* Set scaling factor single step size */
    /* The factor determines the step length as follows:
     * delta = 10^(ceil(log10(parameter) + (-4) ))
     * this ensures correct scaling and always something like 10^(N)
     */
    int ScalingFactor = -4;

    bool relax = true;
    bool fisher_statistic = false;

    /* Store intermediate results, may result in large json blocks */
    bool intermediate = false;
    QList<int> global_param, local_param;
};

class WGSearchThread : public AbstractSearchThread {
    Q_OBJECT

public:
    WGSearchThread(const WGSConfig& config);
    virtual ~WGSearchThread() override;

    inline void setParameterId(int index) { m_index = index; }

    virtual void run() override;
    inline void setUp() { m_direction = +1.0; }
    inline void setDown() { m_direction = -1.0; }
    inline float Direction() const { return m_direction; }
    inline int ParameterId() const { return m_index; }
    inline QList<qreal> XSeries() const { return m_x; }
    inline QList<qreal> YSeries() const { return m_y; }
    inline QList<QJsonObject> IntermediateResults() const { return m_models; }
    inline QJsonObject Result() const { return m_result; }

    inline bool Converged() const { return m_converged; }
    inline bool Finished() const { return m_finished; }
    inline bool Stationary() const { return m_stationary; }
    inline int Steps() const { return m_steps; }

    inline qreal Last() const { return m_last; }
    inline void setIncrement(qreal increment) { m_increment = increment; }
    void setModel(QSharedPointer<AbstractModel> model) { m_model = model->Clone(); }

private:
    void Calculate();
    double m_last;
    int m_index, m_steps;
    float m_direction;
    QList<QJsonObject> m_models;
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
    virtual ~WeakenedGridSearch() override;
    inline void setConfig(const WGSConfig& config) { m_config = config; }
    inline bool CV() { return m_cv; }
    bool ConfidenceAssesment();
    void setParameter(const QJsonObject& json);

public slots:
    virtual void Interrupt() override;

private:
    QPointer<WGSearchThread> CreateThread(int index, bool direction);
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
