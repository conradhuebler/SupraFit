/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <charts.h>

#include "src/core/toolset.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QThread>

#include <QtCharts/QBoxPlotSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QVXYModelMapper>
#include <QtCharts/QXYSeries>

class DataClass;
class DataTable;
class QStandardItemModel;
class AbstractModel;

// Forward declare LineSeries from CuteChart
class LineSeries;

class ChartWrapper : public QObject {
    Q_OBJECT

public:
    ChartWrapper(QObject* parent = 0);
    virtual ~ChartWrapper() override;

    void setData(QSharedPointer<DataClass> model);
    void addWrapper(const QWeakPointer<ChartWrapper>& wrapper);

    inline void setDataTable(DataTable* table) { m_table = table; }
    inline int SeriesSize() const { return m_stored_series.size(); }
    inline QPointer<QXYSeries> Series(int i) { return m_stored_series[i]; }
    inline void setSeries(QPointer<QXYSeries> series, int i) { m_stored_series[i] = series; }
    QColor color(int i) const;
    void TransformModel(QSharedPointer<DataClass> model);
    QString ColorList() const;
    bool setColorList(const QString& str);

    QList<QPointer<QScatterSeries>> CloneSeries(bool swap = false) const;
    QList<QWeakPointer<ChartWrapper>> m_stored_wrapper;

    static QColor ColorCode(int i);

    // === STATE MANAGEMENT API - Claude Generated ===
    void setSeriesVisible(int index, bool visible);
    bool isSeriesVisible(int index) const;
    void setAllSeriesVisible(bool visible);
    int seriesCount() const;

    // === JSON INTEGRATION API - Claude Generated ===
    QJsonObject getVisualState() const;
    void setVisualState(const QJsonObject& state);
    void saveVisualState();
    void loadVisualState();

    // === INTERNAL STATE APPLICATION - Claude Generated ===
    void applyStateToSeries(); // Apply current state to Qt series objects

    inline QString XLabel()
    {
        CheckWorking();
        return m_working.toStrongRef()->XLabel();
    }

    inline QString YLabel()
    {
        CheckWorking();
        return m_working.toStrongRef()->YLabel();
    }

public slots:
    void UpdateModel();
    void MakeSeries();
    void showSeries(int i);
    void SetBlocked(int blocked);

private:
    QPointer<const DataTable> m_table;
    QList<QPointer<QXYSeries>> m_stored_series;
    QWeakPointer<DataClass> m_stored_data;
    QWeakPointer<DataClass> m_stored_model;
    QWeakPointer<DataClass> m_working;

    // === STATE OWNERSHIP - Claude Generated ===
    QVector<bool> m_seriesVisible; // Single source of truth
    QJsonObject m_visualState; // Complete UI state storage
    bool m_defaultVisibility = true; // Claude Generated: Default visibility for deferred operations

    bool m_blocked, m_transformed, m_flipable;
    void InitaliseSeries();
    void CheckWorking();
    void updateVisualStateJson(); // Internal: sync state to JSON - Claude Generated

signals:
    void ModelChanged();
    void stopAnimiation();
    void restartAnimation();
    void ShowSeries(int i);
    void ModelTransformed();
    void SeriesAdded(int i);

    // === REACTIVE SIGNALS - Claude Generated ===
    void seriesVisibilityChanged(int index, bool visible);
    void visualStateChanged(const QJsonObject& state);
};
