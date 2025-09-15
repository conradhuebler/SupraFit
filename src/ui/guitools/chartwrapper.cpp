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

#include <charts.h>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/toolset.h"

#include <QtCharts/QAbstractSeries>
#include <QtCharts/QBoxPlotSeries>

#include <QtCore/QJsonObject>
#include <QtCore/QStringList>
#include <QtGui/QStandardItemModel>

#include "chartwrapper.h"

// Claude Generated: Simplified ChartWrapper following RegressionDialog's successful pattern
// Key insight: RegressionDialog works perfectly because it trusts CuteChart directly

ChartWrapper::ChartWrapper(QObject* parent)
    : QObject(parent)
    , m_blocked(false)
    , m_transformed(false)
{
#ifdef DEBUG_ON
    qDebug() << "🆕 NEW ChartWrapper: Simplified drop-in replacement created";
#endif
}

ChartWrapper::~ChartWrapper()
{
#ifdef DEBUG_ON
    qDebug() << "🗑️ NEW ChartWrapper: Destroyed";
#endif
}

void ChartWrapper::setData(QSharedPointer<DataClass> model)
{
    m_stored_data = model;
    m_working = model;

#ifdef DEBUG_ON
    qDebug() << "🔧 NEW ChartWrapper::setData: Data set, calling InitaliseSeries() and UpdateModel()";
#endif

    // Claude Generated: Connect signals like legacy code
    if (qobject_cast<AbstractModel*>(m_stored_data))
        connect(qobject_cast<AbstractModel*>(m_stored_data.toStrongRef().data()), &AbstractModel::Recalculated, this, &ChartWrapper::UpdateModel);

    connect(m_stored_data.toStrongRef().data()->Info(), &DataClassPrivateObject::Update, this, &ChartWrapper::UpdateModel);

    InitaliseSeries();
    UpdateModel();
}

void ChartWrapper::addWrapper(const QWeakPointer<ChartWrapper>& wrapper)
{
    m_stored_wrapper << wrapper;
}

void ChartWrapper::UpdateModel()
{
    CheckWorking();
    MakeSeries();
    emit ModelChanged();
}

void ChartWrapper::MakeSeries()
{
    if (!m_table)
        return;

    QVector<QList<QPointF>> series(m_stored_series.size());
    int rows = m_working.toStrongRef().data()->DataPoints();
    int cols = m_working.toStrongRef().data()->SeriesCount();
    int maxdata = qApp->instance()->property("MaxSeriesPoints").toInt();

    if (maxdata == 0)
        maxdata = 100000;

    int start = 0;
    int step = qMax(1, int(rows / maxdata));

    for (int i = start; i < rows; i += step) {
        // Claude Generated: Use PrintOutIndependent for x-coordinate like legacy code
        double x = m_working.toStrongRef().data()->PrintOutIndependent(i);
        if (!m_working.toStrongRef().data()->IndependentModel()->isChecked(i))
            continue;
        for (int j = 0; j < cols; ++j) {
            if (m_working.toStrongRef().data()->DependentModel()->isChecked(i, j)) {
                if (j >= m_stored_series.size())
                    continue;
                // Claude Generated: Use table data for y-coordinate like legacy code
                series[j].append(QPointF(x, m_table->data(i, j)));
            }
        }
    }

    // Claude Generated: Update series data following RegressionDialog pattern
    for (int j = 0; j < qMin(series.size(), m_stored_series.size()); ++j) {
        if (m_stored_series[j]) {
            QXYSeries* xySeries = qobject_cast<QXYSeries*>(m_stored_series[j]);
            if (xySeries) {
                xySeries->replace(series[j]);

                // Update point data for selective hiding feature
                if (j >= m_pointData.size()) {
                    m_pointData.resize(j + 1);
                }
                m_pointData[j].originalData = series[j];
                m_pointData[j].pointVisible.resize(series[j].size());
                m_pointData[j].pointVisible.fill(true); // All points visible by default
                m_pointData[j].invalidateCache();

#ifdef DEBUG_ON
                qDebug() << "🔧 NEW ChartWrapper: Updated series" << j << "with" << series[j].size() << "points, all visible by default";
#endif
            }
        }
    }
}

void ChartWrapper::InitaliseSeries()
{
#ifdef DEBUG_ON
    qDebug() << "🔍 NEW ChartWrapper::InitaliseSeries: ENTRY - stored_series.size()=" << m_stored_series.size();
#endif

    if (!m_stored_series.isEmpty()) {
#ifdef DEBUG_ON
        qDebug() << "↩️ NEW ChartWrapper::InitaliseSeries: EARLY RETURN - series already exist";
#endif
        return;
    }

    CheckWorking();
    if (!m_working) {
#ifdef DEBUG_ON
        qDebug() << "❌ NEW ChartWrapper::InitaliseSeries: NO WORKING DATA after CheckWorking()";
#endif
        return;
    }

    int serie = m_working.toStrongRef().data()->SeriesCount();

#ifdef DEBUG_ON
    qDebug() << "🆕 NEW ChartWrapper::InitaliseSeries: m_working->SeriesCount()=" << serie
             << "data type:" << m_working.toStrongRef().data()->metaObject()->className();
#endif

    for (int j = 0; j < serie; ++j) {
#ifdef DEBUG_ON
        qDebug() << "🔧 NEW ChartWrapper: Creating series" << j << "of" << serie;
#endif
        QPointer<QXYSeries> series;

        // Claude Generated: Create appropriate series type like RegressionDialog
        if (qobject_cast<AbstractModel*>(m_working.toStrongRef().data())) {
#ifdef DEBUG_ON
            qDebug() << "📈 NEW ChartWrapper: Creating LineSeries for AbstractModel";
#endif
            series = new LineSeries; // Use LineSeries for models
        } else {
#ifdef DEBUG_ON
            qDebug() << "📊 NEW ChartWrapper: Creating ScatterSeries for raw data";
#endif
            series = new ScatterSeries; // Use ScatterSeries for raw data
        }

        if (series) {
            // Claude Generated: NO COMPLEX VISIBILITY TRACKING - just trust Qt Charts
            series->setVisible(true); // RegressionDialog approach: simple and direct

            m_stored_series << series;

#ifdef DEBUG_ON
            qDebug() << "✅ NEW ChartWrapper: Created series" << j
                     << "type:" << series->metaObject()->className()
                     << "visible:" << series->isVisible()
                     << "stored_series.size() now:" << m_stored_series.size()
                     << "- NO debug tracking, trust Qt Charts!";
#endif
        } else {
#ifdef DEBUG_ON
            qDebug() << "❌ NEW ChartWrapper: FAILED to create series" << j;
#endif
        }
    }

    // Initialize point data storage
    m_pointData.resize(serie);

#ifdef DEBUG_ON
    qDebug() << "🎯 NEW ChartWrapper: InitaliseSeries complete - following successful RegressionDialog pattern";
#endif
}

void ChartWrapper::CheckWorking()
{
    if (!m_working) {
        m_working = m_stored_data;
        m_transformed = false;
    }
}

// === SIMPLIFIED VISIBILITY API - Claude Generated ===
// Direct delegation to Qt Charts - RegressionDialog pattern

void ChartWrapper::setSeriesVisible(int index, bool visible)
{
    if (index < 0 || index >= m_stored_series.size())
        return;

    if (m_stored_series[index]) {
        m_stored_series[index]->setVisible(visible);
        emit seriesVisibilityChanged(index, visible);

#ifdef DEBUG_ON
        qDebug() << "🎯 NEW ChartWrapper: Direct setVisible(" << visible
                 << ") on series" << index << "- following RegressionDialog pattern";
#endif
    }
}

bool ChartWrapper::isSeriesVisible(int index) const
{
    if (index < 0 || index >= m_stored_series.size() || !m_stored_series[index]) {
        return false;
    }

    return m_stored_series[index]->isVisible();
}

void ChartWrapper::setAllSeriesVisible(bool visible)
{
#ifdef DEBUG_ON
    qDebug() << "🎯 NEW ChartWrapper::setAllSeriesVisible(" << visible
             << ") - applying to" << m_stored_series.size() << "series";
#endif

    for (int i = 0; i < m_stored_series.size(); ++i) {
        setSeriesVisible(i, visible);
    }
}

int ChartWrapper::seriesCount() const
{
#ifdef DEBUG_ON
    qDebug() << "📊 NEW ChartWrapper::seriesCount() returning" << m_stored_series.size();
#endif
    return m_stored_series.size();
}

void ChartWrapper::showSeries(int i)
{
    // Claude Generated: Simplified showSeries - direct visibility control
    if (i == -1) {
        // Apply visibility to all series
        for (int j = 0; j < m_stored_series.size(); ++j) {
            if (m_stored_series[j] && !m_stored_series[j]->isVisible()) {
                m_stored_series[j]->setVisible(true);
            }
        }
    } else if (i >= 0 && i < m_stored_series.size()) {
        setSeriesVisible(i, true);
    }

    emit ShowSeries(i);
}

// === POINT HIDING API - Claude Generated ===

QVector<QPointF> ChartWrapper::SeriesPointData::getVisiblePoints() const
{
    if (!cacheValid) {
        visibleCache.clear();
        for (int i = 0; i < originalData.size(); ++i) {
            if (i < pointVisible.size() && pointVisible[i]) {
                visibleCache.append(originalData[i]);
            }
        }
        const_cast<SeriesPointData*>(this)->cacheValid = true;
    }
    return visibleCache;
}

void ChartWrapper::hidePoint(int seriesIndex, int pointIndex)
{
    if (seriesIndex < 0 || seriesIndex >= m_pointData.size())
        return;
    if (pointIndex < 0 || pointIndex >= m_pointData[seriesIndex].pointVisible.size())
        return;

    m_pointData[seriesIndex].pointVisible[pointIndex] = false;
    m_pointData[seriesIndex].invalidateCache();

    updateSeriesDisplay(seriesIndex);
    emit pointVisibilityChanged(seriesIndex, pointIndex, false);

#ifdef DEBUG_ON
    qDebug() << "🔍 NEW ChartWrapper: Hidden point" << pointIndex << "in series" << seriesIndex;
#endif
}

void ChartWrapper::showPoint(int seriesIndex, int pointIndex)
{
    if (seriesIndex < 0 || seriesIndex >= m_pointData.size())
        return;
    if (pointIndex < 0 || pointIndex >= m_pointData[seriesIndex].pointVisible.size())
        return;

    m_pointData[seriesIndex].pointVisible[pointIndex] = true;
    m_pointData[seriesIndex].invalidateCache();

    updateSeriesDisplay(seriesIndex);
    emit pointVisibilityChanged(seriesIndex, pointIndex, true);

#ifdef DEBUG_ON
    qDebug() << "🔍 NEW ChartWrapper: Shown point" << pointIndex << "in series" << seriesIndex;
#endif
}

void ChartWrapper::togglePoint(int seriesIndex, int pointIndex)
{
    if (isPointVisible(seriesIndex, pointIndex)) {
        hidePoint(seriesIndex, pointIndex);
    } else {
        showPoint(seriesIndex, pointIndex);
    }
}

bool ChartWrapper::isPointVisible(int seriesIndex, int pointIndex) const
{
    if (seriesIndex < 0 || seriesIndex >= m_pointData.size())
        return false;
    if (pointIndex < 0 || pointIndex >= m_pointData[seriesIndex].pointVisible.size())
        return false;

    return m_pointData[seriesIndex].pointVisible[pointIndex];
}

QVector<int> ChartWrapper::getHiddenPoints(int seriesIndex) const
{
    QVector<int> hidden;
    if (seriesIndex < 0 || seriesIndex >= m_pointData.size())
        return hidden;

    const auto& visible = m_pointData[seriesIndex].pointVisible;
    for (int i = 0; i < visible.size(); ++i) {
        if (!visible[i]) {
            hidden.append(i);
        }
    }
    return hidden;
}

void ChartWrapper::updateSeriesDisplay(int seriesIndex)
{
    if (seriesIndex < 0 || seriesIndex >= m_stored_series.size())
        return;
    if (seriesIndex >= m_pointData.size())
        return;

    QXYSeries* series = qobject_cast<QXYSeries*>(m_stored_series[seriesIndex]);
    if (!series)
        return;

    // Rebuild series with visible points only
    QVector<QPointF> visiblePoints = m_pointData[seriesIndex].getVisiblePoints();
    series->replace(visiblePoints);

#ifdef DEBUG_ON
    qDebug() << "🔧 NEW ChartWrapper: Updated series" << seriesIndex
             << "display with" << visiblePoints.size() << "visible points"
             << "out of" << m_pointData[seriesIndex].originalData.size() << "total";
#endif
}

// === LEGACY COMPATIBILITY API - Claude Generated ===

QJsonObject ChartWrapper::getVisualState() const
{
    QJsonObject state;

    // Simple active_series string for backward compatibility
    QStringList visibility;
    for (int i = 0; i < m_stored_series.size(); ++i) {
        visibility << (isSeriesVisible(i) ? "1" : "0");
    }
    state["active_series"] = visibility.join(" ");

    // Store hidden points for new feature
    for (int i = 0; i < m_pointData.size(); ++i) {
        QVector<int> hidden = getHiddenPoints(i);
        if (!hidden.isEmpty()) {
            QJsonArray hiddenArray;
            for (int pointIndex : hidden) {
                hiddenArray.append(pointIndex);
            }
            state[QString("series_%1_hidden").arg(i)] = hiddenArray;
        }
    }

    return state;
}

void ChartWrapper::setVisualState(const QJsonObject& state)
{
    // Restore series visibility
    if (state.contains("active_series")) {
        QString activeStr = state["active_series"].toString();
        QStringList parts = activeStr.split(" ");

        for (int i = 0; i < qMin(parts.size(), m_stored_series.size()); ++i) {
            bool visible = (parts[i] == "1");
            setSeriesVisible(i, visible);
        }
    }

    // Restore hidden points
    for (int i = 0; i < m_pointData.size(); ++i) {
        QString key = QString("series_%1_hidden").arg(i);
        if (state.contains(key)) {
            QJsonArray hiddenArray = state[key].toArray();

            for (const auto& value : hiddenArray) {
                hidePoint(i, value.toInt());
            }
        }
    }

    emit visualStateChanged(state);
}

// === COLOR AND UTILITY METHODS - Claude Generated ===
// Simplified versions of legacy methods

QColor ChartWrapper::color(int i) const
{
    if (i >= 0 && i < m_stored_series.size() && m_stored_series[i]) {
        return m_stored_series[i]->color();
    }
    return ColorCode(i);
}

QString ChartWrapper::ColorList() const
{
    QStringList colors;
    for (int i = 0; i < m_stored_series.size(); ++i) {
        colors << color(i).name();
    }
    return colors.join("|");
}

bool ChartWrapper::setColorList(const QString& str)
{
    QStringList colors = str.split("|");
    bool success = true;

    for (int i = 0; i < qMin(colors.size(), m_stored_series.size()); ++i) {
        if (m_stored_series[i]) {
            QColor color(colors[i]);
            if (color.isValid()) {
                m_stored_series[i]->setColor(color);
            } else {
                success = false;
            }
        }
    }

    return success;
}

void ChartWrapper::TransformModel(QSharedPointer<DataClass> model)
{
    m_stored_model = model;
    m_working = model;
    m_transformed = true;
    UpdateModel();
}

QList<QPointer<QScatterSeries>> ChartWrapper::CloneSeries(bool swap) const
{
    QList<QPointer<QScatterSeries>> cloned;

    for (const auto& series : m_stored_series) {
        if (series) {
            QPointer<QScatterSeries> clone = new ScatterSeries;
            QXYSeries* xySeries = qobject_cast<QXYSeries*>(series);
            if (xySeries) {
                QList<QPointF> points = xySeries->points();
                if (swap) {
                    // Swap x and y coordinates
                    for (auto& point : points) {
                        point = QPointF(point.y(), point.x());
                    }
                }
                clone->replace(points);
                clone->setColor(series->color());
            }
            cloned << clone;
        }
    }

    return cloned;
}

QColor ChartWrapper::ColorCode(int i)
{
    switch (i % 9) {
    case 0:
        return QColor::fromRgbF(0.84, 0.93, 0.55, 1).darker(130);
    case 1:
        return QColor::fromRgbF(0.35, 0.47, 0.75, 1);
    case 2:
        return QColor::fromRgbF(1, 0.69, 0.37, 1);
    case 3:
        return QColor::fromRgbF(0.3, 0.73, 0.82, 1);
    case 4:
        return QColor::fromRgbF(0.95, 0.61, 0.7, 1);
    case 5:
        return QColor::fromRgbF(0.38, 0.78, 0.89, 1);
    case 6:
        return QColor::fromRgbF(0.37, 0.79, 0.51, 1);
    case 7:
        return QColor::fromRgbF(0.55, 0.82, 0.72, 1);
    case 8:
        return QColor::fromRgbF(0.67, 0.88, 0.38, 1);
    default:
        return Qt::darkGray;
    }
}

void ChartWrapper::SetBlocked(int blocked)
{
    m_blocked = !blocked;
}

#include "chartwrapper.moc"