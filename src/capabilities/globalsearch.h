/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include "src/capabilities/abstractsearchclass.h"
#include "src/capabilities/montecarlostatistics.h"

#include "src/core/models.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QQueue>
#include <QtCore/QVector>

class AbstractModel;
class GlobalSearch;
class Minimizer;
class NonLinearFitThread;

class SearchBatch : public AbstractSearchThread {
    Q_OBJECT

public:
    SearchBatch(QPointer<GlobalSearch> parent);
    virtual ~SearchBatch() override {}

    virtual void run() override;
    inline QList<QJsonObject> Result() { return m_result; }
    inline void setModel(const QSharedPointer<AbstractModel> model)
    {
        m_model = model->Clone();
        guess = m_model->ExportModel(false, false);
    }

private:
    NonLinearFitThread* m_thread;

    void optimise();
    QJsonObject guess;
    QPointer<GlobalSearch> m_parent;
    QList<QJsonObject> m_result;
    bool m_finished, m_checked;
    /*
protected:
    QSharedPointer<AbstractModel> m_model; */
};

class GlobalSearch : public AbstractSearchClass {
    Q_OBJECT

public:
    GlobalSearch( QObject* parent = 0);
    virtual ~GlobalSearch() override;

    inline QVector<QVector<qreal>> InputList() const { return m_full_list; }
    void ExportResults(const QString& filename, double threshold, bool allow_invalid);
    QVector<qreal> DemandParameter();
    void clear() override;
    virtual bool Run() override;

public slots:
    virtual void Interrupt() override;

private:
    QVector<QVector<double>> ParamList();
    void ConvertList(const QVector<QVector<double>>& full_list);
    virtual QJsonObject Controller() const override;

    quint64 m_time_0;
    int m_time, m_max_count;
    QVector<QVector<double>> m_full_list;
    double error_max;
    bool m_allow_break;
    QQueue<QVector<qreal>> m_input;

signals:
    void InterruptAll();
};
