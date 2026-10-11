/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 -  2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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

// Claude Generated: Debug LineSeries to track setVisible() calls
class DebugLineSeries : public LineSeries {
    Q_OBJECT
public:
    explicit DebugLineSeries(QObject* parent = nullptr)
        : LineSeries()
    {
        qDebug() << "🔍 TRACE DebugLineSeries constructed";
        setParent(parent);
        // Connect to our own visibleChanged signal to track changes
        connect(this, &QAbstractSeries::visibleChanged, this, [this]() {
            qDebug() << "🔍 TRACE DebugLineSeries visibilityChanged: now visible=" << this->isVisible()
                     << "- thread:" << QThread::currentThread();
        });
    }
};

ChartWrapper::ChartWrapper(QObject* parent)
    : QObject(parent)
    , m_blocked(false)
    , m_transformed(false)
{
}

ChartWrapper::~ChartWrapper()
{
    for (int i = 0; i < m_stored_series.size(); ++i) {
        if (m_stored_series[i]) {
            m_stored_series[i]->clear();
            delete m_stored_series[i];
        }
    }

    m_stored_data.clear();
    m_stored_model.clear();
    m_working.clear();
#ifdef DEBUG_ON
    qDebug() << "Deleting chartwrapper";
#endif
}

void ChartWrapper::setData(QSharedPointer<DataClass> model)
{
    m_stored_data = model;
    m_working = m_stored_data;
    // can we make this more compact ?
    if (qobject_cast<AbstractModel*>(m_stored_data))
        connect(qobject_cast<AbstractModel*>(m_stored_data.toStrongRef().data()), &AbstractModel::Recalculated, this, &ChartWrapper::UpdateModel);
    // else if (qobject_cast<DataClass*>(m_stored_data))
    connect(m_stored_data.toStrongRef().data()->Info(), &DataClassPrivateObject::Update, this, &ChartWrapper::UpdateModel);

    InitaliseSeries();
    UpdateModel();
}

void ChartWrapper::addWrapper(const QWeakPointer<ChartWrapper>& wrapper)
{
    if (m_stored_wrapper.contains(wrapper))
        return;

    m_stored_wrapper << wrapper;

    for (int i = 0; i < wrapper.toStrongRef().data()->SeriesSize(); ++i) {
        QPointer<ScatterSeries> series = new ScatterSeries;
        for (const QPointF& point : wrapper.toStrongRef().data()->Series(i)->points())
            series->append(point);

        series->setMarkerSize(qobject_cast<ScatterSeries*>(wrapper.toStrongRef().data()->Series(i))->markerSize() * 0.75);
        series->setMarkerShape(qobject_cast<ScatterSeries*>(wrapper.toStrongRef().data()->Series(i))->markerShape());
        // series->setColor(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->color());
        // series->setBorderColor(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->borderColor());
        // series->setBrush(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->brush());

        connect(wrapper.toStrongRef().data(), &ChartWrapper::ModelChanged, wrapper.toStrongRef().data()->Series(i), [series, wrapper, i]() {
            if (!wrapper.toStrongRef().data() || !series) {
#ifdef DEBUG_ON
                qDebug() << "series already left the building";
#endif
                return;
            }

            series->clear();
            for (const QPointF& point : wrapper.toStrongRef().data()->Series(i)->points())
                series->append(point);

            // series->setMarkerSize(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->markerSize()*0.75);
            // series->setMarkerShape(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->markerShape());
            // series->setColor(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->color());
            // series->setBrush(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->brush());
        });

        m_stored_series << series.data();
        emit SeriesAdded(m_stored_series.size() - 1);
    }
}

void ChartWrapper::InitaliseSeries()
{
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG InitialiseSeries: Starting - working:" << (m_working.isNull() ? "null" : "valid")
             << "stored_series.size()=" << m_stored_series.size();
#endif

    if (!m_working) {
        m_working = m_stored_data;
        m_transformed = false;
    }

    if (m_stored_series.isEmpty()) {

        int serie = m_working.toStrongRef().data()->SeriesCount();

#ifdef DEBUG_ON
        qDebug() << "🔍 DEBUG InitialiseSeries: Creating" << serie << "series";
#endif

        // Claude Generated: Initialize visibility state alongside series creation
        m_seriesVisible.clear();

        for (int j = 0; j < serie; ++j) {
            QPointer<QXYSeries> series;
            if (qobject_cast<AbstractModel*>(m_working.toStrongRef().data()))
                series = new DebugLineSeries; // Claude Generated: Using DebugLineSeries for visibility tracking
            else
                series = new ScatterSeries;
            m_stored_series << series;

            // Claude Generated: Connect LineSeries visibility signals to ChartWrapper state management
            if (auto lineSeries = qobject_cast<LineSeries*>(series)) {
                // Claude Generated: Track initial visibility state
                qDebug() << "🔍 TRACK LineSeries" << j << "created with initial visible=" << lineSeries->isVisible();

                // Claude Generated: Connect to visibility change signal for tracking
                connect(lineSeries, &QAbstractSeries::visibleChanged,
                    this, [this, j, lineSeries]() {
                        qDebug() << "🔍 TRACK LineSeries" << j << "visibleChanged signal fired: visible=" << lineSeries->isVisible();
                    });

                // Existing visibility management connection
                connect(lineSeries, &LineSeries::visibilityChangeRequested,
                    this, [this, j](bool visible) {
                        // Use ChartWrapper state management instead of direct setVisible()
                        setSeriesVisible(j, visible);
                    });
            }

            // Claude Generated: Initialize visibility state - ALWAYS start with true to preserve initial LineSeries visibility
            m_seriesVisible.append(true);
        }

#ifdef DEBUG_ON
        qDebug() << "🔍 DEBUG InitialiseSeries: Initialized" << m_seriesVisible.size()
                 << "visibility states, calling updateVisualStateJson() and applyStateToSeries()";
#endif

        // Update JSON state after initialization
        updateVisualStateJson();

        // Claude Generated: Apply initial visibility state to newly created series
        applyStateToSeries();
    }
#ifdef DEBUG_ON
    else {
        qDebug() << "🔍 DEBUG InitialiseSeries: Series already exist, skipping initialization";
    }
#endif
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
    int step = qMax(1, rows / maxdata);
    for (int i = 0; i < rows; i += step) {
        double x = m_working.toStrongRef().data()->PrintOutIndependent(i);
        if (!m_working.toStrongRef().data()->IndependentModel()->isChecked(i))
            continue;
        for (int j = 0; j < cols; ++j) {
            if (m_working.toStrongRef().data()->DependentModel()->isChecked(i, j)) {
                if (j >= m_stored_series.size())
                    continue;
                series[j].append(QPointF(x, m_table->data(i, j)));
            }
        }
    }
    for (int j = 0; j < m_stored_series.size(); ++j) {
        m_stored_series[j]->replace(series[j]);
    }

    // Claude Generated: Apply visibility state after series data update
    // This ensures series visibility is restored after fitting/data updates
    applyStateToSeries();
}

QList<QPointer<QScatterSeries>> ChartWrapper::CloneSeries(bool swap) const
{
    // Claude Generated: Add null pointer safety check to prevent crashes
    // This fixes crash #1: ChartWrapper::CloneSeries (this=0x0)
    // if (!this) {
    //    qWarning() << "ChartWrapper::CloneSeries: null this pointer!";
    //    return QList<QPointer<QScatterSeries>>();
    //}

    QList<QPointer<QScatterSeries>> series;
    for (int i = 0; i < m_stored_series.size(); ++i) {
        // Claude Generated: Add null pointer check for individual series
        if (!m_stored_series[i] || !m_stored_series[i]->isVisible())
            continue;

        QPointer<QScatterSeries> serie = new QScatterSeries();
        serie->append(m_stored_series[i]->points());
        serie->setColor(m_stored_series[i]->color());
        serie->setName(m_stored_series[i]->name());
        connect(m_stored_series[i].data(), &QAbstractSeries::nameChanged, serie.data(), [serie, i, this]() {
            if (serie && m_stored_series[i])
                serie->setName(m_stored_series[i]->name());
        });
        if (swap) {
            QPointer<QScatterSeries> s = new QScatterSeries;
            for (auto i : serie->points())
                s->append(QPointF(i.y(), i.x()));
            series << s;
        } else
            series << serie;
    }
    return series;
}

QColor ChartWrapper::color(int i) const
{
    if (m_stored_series.size() <= i)
        return ColorCode(i);
    else {
        QPointer<QXYSeries> series = m_stored_series[i];
        if (!series)
            return ColorCode(i);
        return m_stored_series[i]->color();
    }
}

QString ChartWrapper::ColorList() const
{
    QString list;
    for (int i = 0; i < m_stored_series.size(); ++i)
        list += color(i).name() + "|";
    list.chop(1);
    return list;
}

bool ChartWrapper::setColorList(const QString& str)
{
    QStringList colors = str.split("|");
    if (colors.size() != m_stored_series.size())
        return false;
    for (int i = 0; i < m_stored_series.size(); ++i)
        m_stored_series[i]->setColor(QColor(colors[i]));

    return true;
}

void ChartWrapper::TransformModel(QSharedPointer<DataClass> model)
{
    m_stored_model = model;
    /*
    if (!m_transformed)
        m_stored_model = model;
    else
        return;*/
    connect(m_stored_model.toStrongRef().data(), SIGNAL(Recalculated()), this, SLOT(UpdateModel()));
    m_working = m_stored_model;
    m_transformed = true;
    MakeSeries(); // Claude Generated - Fixed cutecharts restructuring bug: series data wasn't being populated
    emit ModelTransformed();
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

void ChartWrapper::showSeries(int i)
{
    // Claude Generated: Refactored to work with new state management system
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG showSeries: Called with index" << i
             << "seriesVisible.size()=" << m_seriesVisible.size()
             << "stored_series.size()=" << m_stored_series.size();
#endif

    if (i == -1) {
        // Backward compatibility: Apply current state to all series
#ifdef DEBUG_ON
        qDebug() << "🔍 DEBUG showSeries: Calling applyStateToSeries() for all series";
#endif
        applyStateToSeries();
    } else if (i >= 0 && i < m_seriesVisible.size()) {
        // Set individual series visible
#ifdef DEBUG_ON
        qDebug() << "🔍 DEBUG showSeries: Setting series" << i << "visible=true";
#endif
        setSeriesVisible(i, true);
    }
    emit ShowSeries(i);
}

void ChartWrapper::CheckWorking()
{
    if (!m_working) {
        m_working = m_stored_data;
        m_transformed = false;
    }
}

// === STATE MANAGEMENT IMPLEMENTATION - Claude Generated ===

void ChartWrapper::setSeriesVisible(int index, bool visible)
{
    if (index < 0 || index >= m_seriesVisible.size())
        return;

    if (m_seriesVisible[index] != visible) {
        m_seriesVisible[index] = visible;

        // Sofortiges Update - KEINE Timer!
        if (index < m_stored_series.size() && m_stored_series[index]) {
            m_stored_series[index]->setVisible(visible);
        }

        updateVisualStateJson();
        emit seriesVisibilityChanged(index, visible);
    }
}

bool ChartWrapper::isSeriesVisible(int index) const
{
    if (index < 0 || index >= m_seriesVisible.size())
        return true; // Default to visible
    return m_seriesVisible[index];
}

void ChartWrapper::setAllSeriesVisible(bool visible)
{
    // Claude Generated: Add safety checks to prevent crashes
    if (!this) {
        qWarning() << "ChartWrapper::setAllSeriesVisible: null this pointer!";
        return;
    }
    qDebug() << "🔍 DEBUG setAllSeriesVisible: Called with visible=" << visible;

    // Claude Generated: Handle empty visibility array by initializing series first
    if (m_seriesVisible.size() == 0 && m_stored_series.size() > 0) {
        qDebug() << "🔧 FIX setAllSeriesVisible: Visibility array empty but series exist - initializing visibility states";
        // Initialize visibility array to match existing series
        m_seriesVisible.clear();
        for (int i = 0; i < m_stored_series.size(); ++i) {
            m_seriesVisible.append(true); // Default to visible
        }
        updateVisualStateJson();
    }

    // Claude Generated: Handle case where neither array is initialized yet - defer the operation
    if (m_seriesVisible.size() == 0 && m_stored_series.size() == 0) {
        qDebug() << "🔄 DEFER setAllSeriesVisible: No series yet - storing desired state for later application";
        // Store the desired visibility state for when series are created
        m_defaultVisibility = visible;
        return;
    }

    for (int i = 0; i < m_seriesVisible.size(); ++i) {
        if (m_seriesVisible[i] != visible) {
            m_seriesVisible[i] = visible;

            // Apply immediately to Qt series
            if (i < m_stored_series.size() && m_stored_series[i]) {
                m_stored_series[i]->setVisible(visible);
            }

            emit seriesVisibilityChanged(i, visible);
        }
    }
    updateVisualStateJson();
}

int ChartWrapper::seriesCount() const
{
    return m_seriesVisible.size();
}

void ChartWrapper::applyStateToSeries()
{
#ifdef DEBUG_ON
    qDebug() << "🔍 DEBUG applyStateToSeries: DISABLED TO PREVENT VISIBILITY CONFLICTS";
    qDebug() << "🔍 DEBUG applyStateToSeries: seriesVisible.size()=" << m_seriesVisible.size()
             << "stored_series.size()=" << m_stored_series.size();
#endif

    // Claude Generated: DIRECT FIX - Disable applyStateToSeries to prevent visibility override
    // The problem: This function fights against ChartWidget's explicit setVisible(true) calls
    // The solution: Let Qt Charts and ChartWidget handle visibility directly (like RegressionDialog)

    // COMMENTED OUT: Original logic that causes visibility conflicts
    /*
    for (int i = 0; i < qMin(m_seriesVisible.size(), m_stored_series.size()); ++i) {
        if (m_stored_series[i]) {
            m_stored_series[i]->setVisible(m_seriesVisible[i]);
        }
    }
    */

#ifdef DEBUG_ON
    qDebug() << "🎯 DIRECT FIX: applyStateToSeries disabled - letting Qt Charts handle visibility naturally";
#endif
}

void ChartWrapper::updateVisualStateJson()
{
    // Update the internal JSON state with current visibility
    QStringList visibility;
    for (bool visible : m_seriesVisible) {
        visibility << (visible ? "1" : "0");
    }
    m_visualState["active_series"] = visibility.join(" ");

    emit visualStateChanged(m_visualState);
}

// === JSON INTEGRATION IMPLEMENTATION - Claude Generated ===

QJsonObject ChartWrapper::getVisualState() const
{
    QJsonObject state = m_visualState; // Start with current state

    // Ensure active_series is up to date
    QStringList visibility;
    for (bool visible : m_seriesVisible) {
        visibility << (visible ? "1" : "0");
    }
    state["active_series"] = visibility.join(" ");

    return state;
}

void ChartWrapper::setVisualState(const QJsonObject& state)
{
    m_visualState = state;

    // Parse active_series string
    QString activeStr = state["active_series"].toString();
    QStringList parts = activeStr.split(" ", Qt::SkipEmptyParts);

    // Resize visibility array to match
    m_seriesVisible.clear();
    for (const QString& part : parts) {
        m_seriesVisible.append(part == "1");
    }

    // Ensure we have at least as many visibility states as series
    while (m_seriesVisible.size() < m_stored_series.size()) {
        m_seriesVisible.append(true); // Default new series to visible
    }

    // Apply state immediately to Qt series
    applyStateToSeries();

    emit visualStateChanged(state);
}

void ChartWrapper::saveVisualState()
{
    // This could be extended to save to file if needed
    updateVisualStateJson();
}

void ChartWrapper::loadVisualState()
{
    // This could be extended to load from file if needed
    // For now, just ensure state is applied
    applyStateToSeries();
}

#include "chartwrapper.moc"
