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

#pragma once

#include <Eigen/Dense>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QString>

/*! \brief One measured curve y(x): a UV-Vis spectrum (x = wavelength), an NMR trace (x = chemical
 * shift) or a thermogram (x = time).
 *
 * Value type; the data are not checked on construction (files may contain anything), isValid() says
 * whether the trace can be processed. A valid trace has x and y of the same length, only finite
 * values and a STRICTLY monotone x axis, ascending or descending (NMR axes run in ppm from high to
 * low). The orientation is determined once when the data are set.
 *
 * Claude Generated
 */
class XYTrace {
public:
    /*! \brief Direction of the x axis. Undefined: fewer than two points or not strictly monotone. */
    enum class Orientation {
        Undefined,
        Ascending,
        Descending
    };

    XYTrace() = default;
    XYTrace(const Eigen::VectorXd& x, const Eigen::VectorXd& y, const QString& name = QString());

    /*! \brief Replaces the data (x and y may have different lengths; isValid() then says so). */
    void setData(const Eigen::VectorXd& x, const Eigen::VectorXd& y);

    inline const Eigen::VectorXd& x() const { return m_x; }
    inline const Eigen::VectorXd& y() const { return m_y; }
    /*! \brief Number of points (of y; equals that of x for a valid trace). */
    inline Eigen::Index size() const { return m_y.size(); }
    inline bool isEmpty() const { return m_x.size() == 0 && m_y.size() == 0; }

    inline QString name() const { return m_name; }
    inline void setName(const QString& name) { m_name = name; }
    /*! \brief Where the data come from (file path or directory). */
    inline QString source() const { return m_source; }
    inline void setSource(const QString& source) { m_source = source; }
    inline QString xLabel() const { return m_x_label; }
    inline QString xUnit() const { return m_x_unit; }
    inline void setXAxis(const QString& label, const QString& unit)
    {
        m_x_label = label;
        m_x_unit = unit;
    }
    inline QString yLabel() const { return m_y_label; }
    inline QString yUnit() const { return m_y_unit; }
    inline void setYAxis(const QString& label, const QString& unit)
    {
        m_y_label = label;
        m_y_unit = unit;
    }

    /*! \brief True for equal lengths, finite values and a strictly monotone x (or < 2 points).
     * An empty trace is valid. On false, *reason names the first problem. */
    bool isValid(QString* reason = nullptr) const;

    inline Orientation orientation() const { return m_orientation; }
    /*! \brief True if x is strictly monotone (ascending or descending) with at least two points. */
    inline bool isMonotone() const { return m_orientation != Orientation::Undefined; }

    /*! \brief Smallest/largest x value (NaN for an empty trace). */
    double xMin() const;
    double xMax() const;

    /*! \brief Index of the point whose x is nearest to x; ties go to the lower index; -1 for an
     * empty trace or a NaN x. Binary search on a monotone axis (either direction), linear scan
     * otherwise (NaN points are skipped). */
    Eigen::Index nearestIndex(double x) const;

    /*! \brief The points with min(a,b) <= x <= max(a,b), in their order, as a new trace with the
     * same name/axes. A range without points (or a NaN bound) gives an empty trace. */
    XYTrace range(double a, double b) const;

    /*! \brief Linear interpolation of y at x.
     *
     * Exact y at a node. Outside [xMin, xMax] the result is NaN: no extrapolation, the caller has to
     * restrict to the measured range (e.g. TraceSet::commonRange). NaN also for an empty trace or one
     * whose x is not strictly monotone. A single point gives its y at its own x and NaN elsewhere.
     */
    double interpolate(double x) const;
    /*! \brief interpolate() at each entry of xs. */
    Eigen::VectorXd interpolate(const Eigen::VectorXd& xs) const;

    /*! \brief Points for charts. */
    QList<QPointF> points() const;

    /*! \brief {"x", "y", "name"} + "source", "xlabel", "xunit", "ylabel", "yunit" if set;
     * x and y lossless (TraceJson::vectorToString). */
    QJsonObject toJson() const;
    /*! \brief Reads toJson() and the old 6-digit string format; false (with *error) for a value that
     * is not a number or for x and y of different lengths. */
    static bool fromJson(const QJsonObject& json, XYTrace* trace, QString* error = nullptr);

private:
    void updateOrientation();
    /// index i with x(i) <= x <= x(i+1) on the axis (monotone axis, x inside the range)
    Eigen::Index segment(double x) const;

    Eigen::VectorXd m_x, m_y;
    Orientation m_orientation = Orientation::Undefined;
    QString m_name, m_source, m_x_label, m_x_unit, m_y_label, m_y_unit;
};
