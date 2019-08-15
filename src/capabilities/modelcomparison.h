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
    int m_maxsteps, m_steps = 0, m_ParameterIndex = 0;
    qreal m_MaxParameter;
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

    void setModel(QSharedPointer<AbstractModel> model) { m_model = model->Clone(false); }
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

public slots:
    void Interrupt() override;

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

signals:
    void StopSubThreads();
};
