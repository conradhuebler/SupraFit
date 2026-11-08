/*
 * XYTrace - one raw curve (spectrum, thermogram, NMR trace) as a value type
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
#include <limits>
#include <vector>

#include "tracejson.h"
#include "xytrace.h"

namespace {
const double kNaN = std::numeric_limits<double>::quiet_NaN();
}

XYTrace::XYTrace(const Eigen::VectorXd& x, const Eigen::VectorXd& y, const QString& name)
    : m_x(x)
    , m_y(y)
    , m_name(name)
{
    updateOrientation();
}

void XYTrace::setData(const Eigen::VectorXd& x, const Eigen::VectorXd& y)
{
    m_x = x;
    m_y = y;
    updateOrientation();
}

void XYTrace::updateOrientation()
{
    m_orientation = Orientation::Undefined;
    const Eigen::Index n = m_x.size();
    if (n < 2)
        return;
    bool ascending = true, descending = true;
    for (Eigen::Index i = 1; i < n && (ascending || descending); ++i) {
        // comparisons with NaN are false: a NaN makes the axis Undefined
        ascending = ascending && (m_x(i) > m_x(i - 1));
        descending = descending && (m_x(i) < m_x(i - 1));
    }
    if (ascending)
        m_orientation = Orientation::Ascending;
    else if (descending)
        m_orientation = Orientation::Descending;
}

bool XYTrace::isValid(QString* reason) const
{
    if (m_x.size() != m_y.size()) {
        if (reason)
            *reason = QString("x has %1 values, y has %2").arg(m_x.size()).arg(m_y.size());
        return false;
    }
    for (Eigen::Index i = 0; i < m_x.size(); ++i) {
        if (!std::isfinite(m_x(i)) || !std::isfinite(m_y(i))) {
            if (reason)
                *reason = QString("point %1 is not finite").arg(i);
            return false;
        }
    }
    if (m_x.size() >= 2 && m_orientation == Orientation::Undefined) {
        if (reason)
            *reason = QString("x is not strictly monotone");
        return false;
    }
    return true;
}

double XYTrace::xMin() const
{
    if (m_x.size() == 0)
        return kNaN;
    if (m_orientation == Orientation::Ascending)
        return m_x(0);
    if (m_orientation == Orientation::Descending)
        return m_x(m_x.size() - 1);
    return m_x.minCoeff();
}

double XYTrace::xMax() const
{
    if (m_x.size() == 0)
        return kNaN;
    if (m_orientation == Orientation::Ascending)
        return m_x(m_x.size() - 1);
    if (m_orientation == Orientation::Descending)
        return m_x(0);
    return m_x.maxCoeff();
}

Eigen::Index XYTrace::nearestIndex(double x) const
{
    const Eigen::Index n = m_x.size();
    if (n == 0 || std::isnan(x))
        return -1;

    if (m_orientation == Orientation::Undefined) {
        Eigen::Index best = -1;
        double diff = std::numeric_limits<double>::infinity();
        for (Eigen::Index i = 0; i < n; ++i) {
            const double d = std::abs(x - m_x(i));
            if (d < diff) { // NaN points never win, ties keep the lower index
                diff = d;
                best = i;
            }
        }
        return best;
    }

    const double* begin = m_x.data();
    const double* end = begin + n;
    // first index whose x is not "before" the target in axis direction
    const double* it = (m_orientation == Orientation::Ascending)
        ? std::lower_bound(begin, end, x)
        : std::lower_bound(begin, end, x, [](double a, double b) { return a > b; });
    const Eigen::Index i = it - begin;
    if (i == 0)
        return 0;
    if (i == n)
        return n - 1;
    const double before = std::abs(x - m_x(i - 1));
    const double after = std::abs(m_x(i) - x);
    return before <= after ? i - 1 : i;
}

XYTrace XYTrace::range(double a, double b) const
{
    XYTrace result = *this;
    const double lo = std::min(a, b), hi = std::max(a, b);
    std::vector<Eigen::Index> keep;
    if (!std::isnan(a) && !std::isnan(b)) {
        const Eigen::Index n = std::min(m_x.size(), m_y.size());
        keep.reserve(n);
        for (Eigen::Index i = 0; i < n; ++i)
            if (m_x(i) >= lo && m_x(i) <= hi)
                keep.push_back(i);
    }
    Eigen::VectorXd x(Eigen::Index(keep.size())), y(Eigen::Index(keep.size()));
    for (std::size_t k = 0; k < keep.size(); ++k) {
        x(Eigen::Index(k)) = m_x(keep[k]);
        y(Eigen::Index(k)) = m_y(keep[k]);
    }
    result.setData(x, y);
    return result;
}

Eigen::Index XYTrace::segment(double x) const
{
    const Eigen::Index n = m_x.size();
    const double* begin = m_x.data();
    const double* end = begin + n;
    // upper_bound: first node strictly "after" x in axis direction; the segment starts one before
    const double* it = (m_orientation == Orientation::Ascending)
        ? std::upper_bound(begin, end, x)
        : std::upper_bound(begin, end, x, [](double a, double b) { return a > b; });
    Eigen::Index i = (it - begin) - 1;
    return std::clamp<Eigen::Index>(i, 0, n - 2);
}

double XYTrace::interpolate(double x) const
{
    const Eigen::Index n = std::min(m_x.size(), m_y.size());
    if (n == 0 || std::isnan(x) || m_x.size() != m_y.size())
        return kNaN;
    if (n == 1)
        return x == m_x(0) ? m_y(0) : kNaN;
    if (m_orientation == Orientation::Undefined)
        return kNaN;
    if (x < xMin() || x > xMax())
        return kNaN;

    const Eigen::Index i = segment(x);
    if (x == m_x(i))
        return m_y(i);
    if (x == m_x(i + 1))
        return m_y(i + 1);
    const double t = (x - m_x(i)) / (m_x(i + 1) - m_x(i));
    return m_y(i) + t * (m_y(i + 1) - m_y(i));
}

Eigen::VectorXd XYTrace::interpolate(const Eigen::VectorXd& xs) const
{
    Eigen::VectorXd result(xs.size());
    for (Eigen::Index i = 0; i < xs.size(); ++i)
        result(i) = interpolate(xs(i));
    return result;
}

QList<QPointF> XYTrace::points() const
{
    QList<QPointF> list;
    const Eigen::Index n = std::min(m_x.size(), m_y.size());
    list.reserve(n);
    for (Eigen::Index i = 0; i < n; ++i)
        list << QPointF(m_x(i), m_y(i));
    return list;
}

QJsonObject XYTrace::toJson() const
{
    QJsonObject json;
    json["x"] = TraceJson::vectorToString(m_x);
    json["y"] = TraceJson::vectorToString(m_y);
    json["name"] = m_name;
    if (!m_source.isEmpty())
        json["source"] = m_source;
    if (!m_x_label.isEmpty())
        json["xlabel"] = m_x_label;
    if (!m_x_unit.isEmpty())
        json["xunit"] = m_x_unit;
    if (!m_y_label.isEmpty())
        json["ylabel"] = m_y_label;
    if (!m_y_unit.isEmpty())
        json["yunit"] = m_y_unit;
    return json;
}

bool XYTrace::fromJson(const QJsonObject& json, XYTrace* trace, QString* error)
{
    Eigen::VectorXd x, y;
    QString reason;
    if (!TraceJson::vectorFromJson(json.value("x"), &x, &reason)) {
        if (error)
            *error = QString("x: %1").arg(reason);
        return false;
    }
    if (!TraceJson::vectorFromJson(json.value("y"), &y, &reason)) {
        if (error)
            *error = QString("y: %1").arg(reason);
        return false;
    }
    if (x.size() != y.size()) {
        if (error)
            *error = QString("x has %1 values, y has %2").arg(x.size()).arg(y.size());
        return false;
    }
    if (trace) {
        *trace = XYTrace(x, y, json.value("name").toString());
        trace->setSource(json.value("source").toString());
        trace->setXAxis(json.value("xlabel").toString(), json.value("xunit").toString());
        trace->setYAxis(json.value("ylabel").toString(), json.value("yunit").toString());
    }
    return true;
}
