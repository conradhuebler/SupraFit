/*
 * <one line to give the library's name and an idea of what it does.>
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
#include "weakenedgridsearch.h"

#include <QtCore/QObject>
#include <QtCore/QRunnable>

const int update_intervall = 100;

const QJsonObject ModelComparisonConfigBlock{

    /* Maximal number of steps to be evaluated */
    { "MaxSteps", 1e4 }, // int

    /* Fast Confidence Maximal steps */
    { "MaxStepsFastConfidence", 1e4 }, // int

    /* Set scaling factor single step size */
    /* The factor determines the step length as follows:
     * delta = 10^(ceil(log10(parameter) + (-4) ))
     * this ensures correct scaling and always something like 10^(N)
     */
    { "FastConfidenceScaling", -4 }, // int

    /* SSE threshold defined by f-Statistics */
    { "MaxError", 0 }, //double

    /* Confidence in % */
    { "confidence", 95 }, //double

    /* Corresponding f Value */
    { "f_value", 0 }, // double

    /* Threshold for convergency in SSE */
    { "ErrorConvergency", 1e-10 }, // double

    /* Box Scaling Factor */
    { "BoxScalingFactor", 1.5 }, // double

    /* Define the global and local parameter to be tested - this list should not be empty 
     * when a grid search is performed, otherwise nothing happens at all, or it crashes ...*/
    { "GlobalParameterList", "" }, // strings, to be converted to QList<int>
    { "LocalParameterList", "" }, // strings, to be converted to QList<int>

    /* Series in FastConfidence */
    { "IncludeSeries", true }
};

class AbstractModel;

class MCThread : public AbstractSearchThread {
    Q_OBJECT

public:
    inline MCThread()
        : AbstractSearchThread()
    {
    }
    inline virtual ~MCThread() {}
    void setModel(QSharedPointer<AbstractModel> model)
    {
        m_model = model->Clone();
        m_model->setFast();
    }
    void run();
    QList<QJsonObject> Results() const { return m_results; }
    inline void setMaxSteps(int steps) { m_maxsteps = steps; }
    inline void setBox(const QVector<QVector<qreal>>& box) { m_box = box; }
    inline void setError(qreal error) { m_effective_error = error; }
    inline int Steps() const { return m_steps; }

private:
    QSharedPointer<AbstractModel> m_model;
    QList<QJsonObject> m_results;
    int m_maxsteps, m_steps = 0;

    QVector<QVector<qreal>> m_box;
    qreal m_effective_error;
};

class FCThread : public AbstractSearchThread {
    Q_OBJECT

public:
    FCThread(int parameter)
        : AbstractSearchThread()
        , m_parameter(parameter)
    {
    }
    virtual ~FCThread() override {}

    void setModel(QSharedPointer<AbstractModel> model) { m_model = model->Clone(); }
    virtual void run() override;
    inline qreal Lower() const { return m_lower; }
    inline qreal Upper() const { return m_upper; }
    inline QList<QPointF> Points() const { return m_points; }

private:
    qreal m_lower, m_upper;
    int m_parameter;
    qreal SingleLimit(int parameter_id, int direction);
    QList<QPointF> m_points;
    QMap<qreal, qreal> m_list_points;
};

class ModelComparison : public AbstractSearchClass {
    Q_OBJECT

public:
    ModelComparison(QObject* parent = 0);
    virtual ~ModelComparison() override;

    bool Confidence();
    bool FastConfidence();
    inline qreal Area() const { return m_ellipsoid_area; }
    inline void setResults(const QList<QJsonObject> results)
    {
        m_results = results;
        m_fast_finished = true;
    }
    virtual bool Run() override;

private:
    void StripResults(const QList<QJsonObject>& results);
    void MCSearch(const QVector<QVector<qreal>>& box);
    double SingleLimit(int parameter_id, int direction = 1);
    int m_steps = 0;
    QVector<QVector<qreal>> MakeBox();
    QJsonObject m_box;
    double m_effective_error, m_box_area, m_ellipsoid_area;
    QVector<QList<qreal>> m_data_global, m_data_local;
    bool m_fast_finished;
};
