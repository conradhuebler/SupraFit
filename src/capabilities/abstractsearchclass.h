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

#include "src/core/AbstractModel.h"

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QQueue>
#include <QtCore/QRunnable>
#include <QtCore/QSharedPointer>

#include <QtCore/QThreadPool>

typedef QPair<QPointer<DataTable>, QPointer<DataTable>> Pair;

class AbstractModel;

class AbstractSearchThread : public QObject, public QRunnable {
    Q_OBJECT

public:
    inline AbstractSearchThread()
        : m_interrupt(false)
    {
        setAutoDelete(false);
    }
    inline ~AbstractSearchThread() { m_model.clear(); }
    inline void setModel(const QSharedPointer<AbstractModel> model) { m_model = model->Clone(); }
    inline void setController(const QJsonObject& controller) { m_controller = controller; }
    inline QHash<int, QJsonObject> Models() const { return m_models; }

public slots:
    inline virtual void Interrupt() { m_interrupt = true; }

protected:
    QSharedPointer<AbstractModel> m_model;
    bool m_interrupt;
    QJsonObject m_controller;
    QHash<int, QJsonObject> m_models;

signals:
    void IncrementProgress(int msecs);
};

class AbstractSearchClass : public QObject {
    Q_OBJECT

public:
    AbstractSearchClass(QObject* parent = 0);
    ~AbstractSearchClass();

    virtual inline void setModel(const QSharedPointer<AbstractModel> model)
    {
        m_model = model->Clone();
    }

    virtual bool Run() = 0;
    inline void setController(const QJsonObject& controller) { m_controller = controller; }

    inline QList<QList<QPointF>> Series() const { return m_series; }
    inline QList<QJsonObject> Models() const { return m_models; }
    inline QList<QJsonObject> Results() const { return m_results; }
    QJsonObject Result() const;
    void ExportResults(const QString& filename);

    QHash<int, Pair> DemandCalc();

    virtual void clear();
public slots:
    virtual void Interrupt();

protected:
    QSharedPointer<AbstractModel> m_model;
    QJsonObject m_controller;
    QList<QJsonObject> m_models;
    QList<QJsonObject> m_results;

    QThreadPool* m_threadpool;
    QList<QList<QPointF>> m_series;
    bool m_interrupt;
    QQueue<QHash<int, Pair>> m_batch;

    virtual QJsonObject Controller() const { return m_controller; }
    QMutex mutex;

signals:
    void IncrementProgress(int msecs);
    void AnalyseFinished();
    void setMaximumSteps(int maxsteps);
    void Message(const QString& str);
};
