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

const QJsonObject GridSearchConfigBlock{

    /* Maximal number of steps to be evaluated */
    { "MaxSteps", 1e3 }, // int

    /* SSE threshold defined by f-Statistics */
    { "MaxError", 0 }, //double

    /* Confidence in % */
    { "confidence", 95 }, //double

    /* Corresponding f Value */
    { "f_value", 0 }, // double

    /* Threshold for convergency in SSE */
    { "ErrorConvergency", 1e-10 }, // double

    /* Maximal steps which are allowed to be above the SSE threshold, while continueing the evaluation */
    { "OvershotCounter", 5 }, // int

    /* Maximal number of steps, where the error is allowed to decrease, analyse not-converged systems, flat surfaces */
    { "ErrorDecreaseCounter", 50 }, // int

    /* Amount for all error changes below the threshold error_conv, while keeping the evaluation running */
    { "ErrorConvergencyCounter", 5 }, // int

    /* Set scaling factor single step size */
    /* The factor determines the step length as follows:
     * delta = 10^(ceil(log10(parameter) + (-4) ))
     * this ensures correct scaling and always something like 10^(N)
     */
    { "ScalingFactor", -4 }, // int

    /* Store intermediate results, may result in large json blocks */
    { "intermediate", false }, //bool

    /* Define the global and local parameter to be tested - this list should not be empty 
     * when a grid search is performed, otherwise nothing happens at all, or it crashes ...*/
    { "GlobalParameterList", "" }, // strings, to be converted to QList<int>
    { "LocalParameterList", "" } // strings, to be converted to QList<int>
};

class WGSearchThread : public AbstractSearchThread {
    Q_OBJECT

public:
    WGSearchThread(const QJsonObject& controller);
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
    void setModel(QSharedPointer<AbstractModel> model) { m_model = model->Clone(); }

    inline int OvershotCounter() const { return m_OvershotCounter; }
    inline int ErrorDecreaseCounter() const { return m_ErrorDecreaseCounter; }
    inline int ErrorConvergencyCounter() const { return m_ErrorConvergencyCounter; }

private:
    void Calculate();
    double m_last;
    int m_index, m_steps;
    float m_direction;
    QList<QJsonObject> m_models;
    QList<qreal> m_x, m_y;
    QJsonObject m_controller;
    QJsonObject m_result;
    qreal m_ModelError, m_MaxError, m_ErrorConvergency;
    int m_OvershotCounter = 0, m_ErrorDecreaseCounter = 0, m_ErrorConvergencyCounter = 0;
    int m_MaxSteps, m_MaxOvershotCounter, m_MaxErrorDecreaseCounter, m_MaxErrorConvergencyCounter, m_ScalingFactor;
    bool m_stationary, m_finished, m_converged;
};

class WeakenedGridSearch : public AbstractSearchClass {
    Q_OBJECT

public:
    WeakenedGridSearch(QObject* parent = 0);
    virtual ~WeakenedGridSearch() override;
    /*
    inline void setConfig(const QJsonObject& controller)
    {
        m_controller = controller;
        m_StoreIntermediate = controller["StoreIntermediate"].toBool();
    }*/
    inline bool CV() { return m_cv; }
    bool ConfidenceAssesment();
    void setParameter(const QJsonObject& json);
    void clear() override;

public slots:
    virtual void Interrupt() override;

private:
    QPointer<WGSearchThread> CreateThread(int index, bool direction);
    bool allow_break, m_cv, m_StoreIntermediate = false;
    QVector<QVector<qreal>> MakeBox() const;
    void MCSearch(const QVector<QVector<qreal>>& box);
    void Search(const QVector<QVector<qreal>>& box);
    void StripResults(const QList<QJsonObject>& results);
    virtual QJsonObject Controller() const override;

signals:
    void StopSubThreads();
};
