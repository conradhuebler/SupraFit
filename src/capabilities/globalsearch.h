/*
 * <one line to give the program's name and a brief idea of what it does.>
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

struct GlobalSearchResult {
    QVector<QVector<double>> m_input;
    QVector<double> m_error;
    QVector<double> m_corr_coeff;
    QVector<QJsonObject> m_models;
};

struct GSResult {
    QVector<qreal> initial, optimised;
    qreal SumError;
    QJsonObject model;
    bool valid, converged;
};

class GSConfig : public AbstractConfig {
public:
    inline GSConfig(OptimizerConfig config)
        : AbstractConfig(config)
    {
    }
    inline GSConfig() {}
    bool initial_guess = true;
    bool optimize = true;
    QVector<QVector<qreal>> parameter;
    QList<int> ignored_parameter;
};

class SearchBatch : public AbstractSearchThread {
    Q_OBJECT

public:
    SearchBatch(const GSConfig& config, QPointer<GlobalSearch> parent);
    ~SearchBatch() {}

    virtual void run() override;
    inline QList<GSResult> Result() { return m_result; }
    inline void setModel(const QSharedPointer<AbstractModel> model)
    {
        m_model = model->Clone();
        guess = m_model->ExportModel(false, false);
    }

private:
    NonLinearFitThread* m_thread;

    void optimise();
    QJsonObject guess;
    QList<GSResult> m_result;
    QPointer<GlobalSearch> m_parent;
    GSConfig m_config;
    bool m_finished, m_checked;
    QSharedPointer<AbstractModel> m_model;
};

class GlobalSearch : public AbstractSearchClass {
    Q_OBJECT

public:
    GlobalSearch( QObject* parent = 0);
    ~GlobalSearch();

    inline void setConfig(const GSConfig& config) { m_config = config; }
    inline QVector<QVector<qreal>> InputList() const { return m_full_list; }
    QList<QJsonObject> SearchGlobal();
    void ExportResults(const QString& filename, double threshold, bool allow_invalid);
    QVector<qreal> DemandParameter();
    inline QList<GSResult> Result() { return m_result; }

public slots:
    virtual void Interrupt() override;

private:
    QVector<QVector<double>> ParamList();
    void ConvertList(const QVector<QVector<double>>& full_list);
    // void Scan(const QVector<QVector<double>>& list);
    virtual QJsonObject Controller() const override;
    QList<GSResult> m_result;

    quint64 m_time_0;
    int m_time, m_max_count;
    QVector<QVector<double>> m_full_list;
    GlobalSearchResult last_result;
    double error_max;
    bool m_allow_break;
    GSConfig m_config;
    QQueue<QVector<qreal>> m_input;

signals:
    void InterruptAll();
};
