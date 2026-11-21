/*
 * TraceSet - ordered collection of XYTrace (the steps of a titration)
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

// Claude Generated: WP1 of roadmap/input_data_editing.md

#include <algorithm>
#include <cmath>

#include <QtCore/QDebug>
#include <QtCore/QUuid>

#include "traceset.h"

QString TraceSet::newId()
{
    return QUuid::createUuid().toString();
}

void TraceSet::clear()
{
    m_ids.clear();
    m_traces.clear();
}

QString TraceSet::add(const XYTrace& trace, const QString& id)
{
    return insert(m_traces.size(), trace, id);
}

QString TraceSet::insert(int position, const XYTrace& trace, const QString& id)
{
    QString key = id.isEmpty() ? newId() : id;
    if (m_ids.contains(key)) {
        qWarning() << "TraceSet: id" << key << "is already in use, trace not added";
        return QString();
    }
    position = std::clamp(position, 0, int(m_traces.size()));
    m_ids.insert(position, key);
    m_traces.insert(position, trace);
    return key;
}

bool TraceSet::remove(const QString& id)
{
    const int i = m_ids.indexOf(id);
    if (i < 0)
        return false;
    m_ids.removeAt(i);
    m_traces.removeAt(i);
    return true;
}

bool TraceSet::move(int from, int to)
{
    if (from < 0 || to < 0 || from >= m_traces.size() || to >= m_traces.size())
        return false;
    m_ids.move(from, to);
    m_traces.move(from, to);
    return true;
}

const XYTrace& TraceSet::at(int i) const
{
    Q_ASSERT(i >= 0 && i < m_traces.size());
    return m_traces.at(i);
}

bool TraceSet::replace(int i, const XYTrace& trace)
{
    if (i < 0 || i >= m_traces.size())
        return false;
    m_traces[i] = trace;
    return true;
}

XYTrace TraceSet::trace(const QString& id) const
{
    const int i = m_ids.indexOf(id);
    return i < 0 ? XYTrace() : m_traces.at(i);
}

bool TraceSet::commonRange(double* lo, double* hi) const
{
    bool any = false;
    double low = 0, high = 0;
    for (const XYTrace& t : m_traces) {
        if (t.size() == 0)
            continue;
        if (!any) {
            low = t.xMin();
            high = t.xMax();
            any = true;
        } else {
            low = std::max(low, t.xMin());
            high = std::min(high, t.xMax());
        }
    }
    if (!any || !(low <= high))
        return false;
    if (lo)
        *lo = low;
    if (hi)
        *hi = high;
    return true;
}

bool TraceSet::hasCommonAxis(double relTolerance) const
{
    if (m_traces.size() < 2)
        return true;
    const Eigen::VectorXd& first = m_traces.first().x();
    const double span = first.size() ? std::abs(first(first.size() - 1) - first(0)) : 0.0;
    const double tolerance = relTolerance * span;
    for (const XYTrace& t : m_traces) {
        if (t.x().size() != first.size() || t.y().size() != first.size())
            return false;
        for (Eigen::Index i = 0; i < first.size(); ++i)
            if (!(std::abs(t.x()(i) - first(i)) <= tolerance))
                return false;
    }
    return true;
}

TraceSet TraceSet::resampleToCommonAxis(QString* error) const
{
    auto fail = [error](const QString& message) {
        if (error)
            *error = message;
        return TraceSet();
    };
    if (m_traces.isEmpty())
        return fail("no traces");
    for (int i = 0; i < m_traces.size(); ++i) {
        QString reason;
        if (!m_traces[i].isValid(&reason))
            return fail(QString("trace %1 (%2): %3").arg(i + 1).arg(m_traces[i].name(), reason));
        if (!m_traces[i].isMonotone())
            return fail(QString("trace %1 (%2) has fewer than two points").arg(i + 1).arg(m_traces[i].name()));
    }
    double lo = 0, hi = 0;
    if (!commonRange(&lo, &hi))
        return fail("the traces do not share an x range");

    const XYTrace axis = m_traces.first().range(lo, hi);
    TraceSet result;
    for (int i = 0; i < m_traces.size(); ++i) {
        XYTrace t = m_traces[i];
        t.setData(axis.x(), m_traces[i].interpolate(axis.x()));
        result.m_ids << m_ids[i];
        result.m_traces << t;
    }
    return result;
}
