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
#include "src/core/AbstractModel.h"
#include "src/global.h"

#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>

class Minimizer;
class QPointF;

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
    inline bool CV() { return m_cv; }
    void setParameter(const QJsonObject& json);
    virtual bool Run() override;

public slots:
    virtual void Interrupt() override;

private:
    QPointer<WGSearchThread> CreateThread(int index, bool direction);
    bool allow_break, m_cv, m_StoreIntermediate = false;
    QVector<QVector<qreal>> MakeBox() const;
    void MCSearch(const QVector<QVector<qreal>>& box);
    void Search(const QVector<QVector<qreal>>& box);
    void StripResults(const QList<QJsonObject>& results);

signals:
    void StopSubThreads();
};
