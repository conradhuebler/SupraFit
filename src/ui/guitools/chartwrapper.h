/*
 * <ChartWrapper - Simplified drop-in replacement for legacy complex visibility system>
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

/**
 * @brief ChartWrapper - Simplified drop-in replacement following RegressionDialog pattern
 *
 * Claude Generated: Complete rewrite focusing on simplicity and direct CuteChart usage
 * Key principles:
 * - Trust Qt Charts for visibility management
 * - Minimal abstraction layers
 * - Direct delegation instead of complex state management
 * - Follow RegressionDialog's successful pattern
 */
class ChartWrapper : public QObject {
    Q_OBJECT

public:
    ChartWrapper(QObject* parent = 0);
    virtual ~ChartWrapper() override;

    // === CORE DATA API - Claude Generated ===
    void setData(QSharedPointer<DataClass> model);
    void addWrapper(const QWeakPointer<ChartWrapper>& wrapper);

    inline void setDataTable(DataTable* table) { m_table = table; }
    inline int SeriesSize() const
    {
#ifdef DEBUG_ON
        qDebug() << "📊 NEW ChartWrapper::SeriesSize() returning" << m_stored_series.size();
#endif
        return m_stored_series.size();
    }
    inline QPointer<QXYSeries> Series(int i)
    {
        return (i >= 0 && i < m_stored_series.size()) ? m_stored_series[i] : nullptr;
    }
    inline void setSeries(QPointer<QXYSeries> series, int i)
    {
        if (i >= 0 && i < m_stored_series.size()) {
            m_stored_series[i] = series;
        }
    }

    // === VISUAL API - Claude Generated ===
    QColor color(int i) const;
    void TransformModel(QSharedPointer<DataClass> model);
    QString ColorList() const;
    bool setColorList(const QString& str);
    QList<QPointer<QScatterSeries>> CloneSeries(bool swap = false) const;
    static QColor ColorCode(int i);

    // === SIMPLIFIED VISIBILITY API - Claude Generated ===
    // Direct delegation to Qt Charts - no complex state management
    void setSeriesVisible(int index, bool visible);
    bool isSeriesVisible(int index) const;
    void setAllSeriesVisible(bool visible);
    int seriesCount() const;

    // === LEGACY COMPATIBILITY API - Claude Generated ===
    // Minimal JSON support for backward compatibility
    QJsonObject getVisualState() const;
    void setVisualState(const QJsonObject& state);
    void saveVisualState() { /* No-op - Qt Charts handles state */ }
    void loadVisualState() { /* No-op - Qt Charts handles state */ }
    void applyStateToSeries() { /* No-op - Legacy compatibility */ }

    // === POINT HIDING API - Claude Generated ===
    // New feature: selective point hiding following simple pattern
    void hidePoint(int seriesIndex, int pointIndex);
    void showPoint(int seriesIndex, int pointIndex);
    void togglePoint(int seriesIndex, int pointIndex);
    bool isPointVisible(int seriesIndex, int pointIndex) const;
    QVector<int> getHiddenPoints(int seriesIndex) const;

    // === LEGACY COMPATIBILITY ACCESSORS - Claude Generated ===
    inline QString XLabel()
    {
        CheckWorking();
        auto workingData = m_working.toStrongRef();
        return workingData ? workingData->XLabel() : QString();
    }

    inline QString YLabel()
    {
        CheckWorking();
        auto workingData = m_working.toStrongRef();
        return workingData ? workingData->YLabel() : QString();
    }

    QList<QWeakPointer<ChartWrapper>> m_stored_wrapper;

public slots:
    void UpdateModel();
    void MakeSeries();
    void showSeries(int i);
    void SetBlocked(int blocked);

private:
    // === CORE DATA STORAGE - Claude Generated ===
    // All model references are WEAK to prevent circular dependencies and enable proper cleanup
    QPointer<const DataTable> m_table;
    QList<QPointer<QXYSeries>> m_stored_series;
    QWeakPointer<DataClass> m_stored_data;   // WEAK: Allow model to be destroyed
    QWeakPointer<DataClass> m_stored_model;  // WEAK: Allow model to be destroyed  
    QWeakPointer<DataClass> m_working;       // WEAK: Allow model to be destroyed

    // === POINT HIDING STORAGE - Claude Generated ===
    struct SeriesPointData {
        QVector<QPointF> originalData; // All original points
        QVector<bool> pointVisible; // Visibility per point
        bool cacheValid = false;
        mutable QVector<QPointF> visibleCache;

        QVector<QPointF> getVisiblePoints() const;
        void invalidateCache() { cacheValid = false; }
    };
    QVector<SeriesPointData> m_pointData;

    // === LEGACY STATE - Claude Generated ===
    bool m_blocked = false;
    bool m_transformed = false;
    bool m_flipable = false;

    // === PRIVATE METHODS - Claude Generated ===
    void InitaliseSeries();
    void CheckWorking();
    void updateSeriesDisplay(int seriesIndex); // Rebuild series with visible points only

signals:
    void ModelChanged();
    void stopAnimiation();
    void restartAnimation();
    void ShowSeries(int i);
    void ModelTransformed();
    void SeriesAdded(int i);

    // === NEW SIGNALS - Claude Generated ===
    void seriesVisibilityChanged(int index, bool visible);
    void pointVisibilityChanged(int seriesIndex, int pointIndex, bool visible);
    void visualStateChanged(const QJsonObject& state);
};