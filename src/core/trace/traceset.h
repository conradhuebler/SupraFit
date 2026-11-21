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

#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include "xytrace.h"

/*! \brief Ordered traces of one experiment (one spectrum per titration step).
 *
 * Every trace has an id that stays the same when traces are added, removed or moved (a QUuid string
 * unless the caller gives one). The order is the order of the titration steps.
 *
 * Claude Generated
 */
class TraceSet {
public:
    TraceSet() = default;

    inline int size() const { return m_traces.size(); }
    inline bool isEmpty() const { return m_traces.isEmpty(); }
    void clear();

    /*! \brief Appends the trace; returns its id (a new QUuid if id is empty), an empty string if
     * the given id is already in use (nothing is added then). */
    QString add(const XYTrace& trace, const QString& id = QString());
    /*! \brief Inserts at position (clamped to [0, size()]); id rules as add(). */
    QString insert(int position, const XYTrace& trace, const QString& id = QString());
    /*! \brief Removes the trace with this id; false if there is none. */
    bool remove(const QString& id);
    /*! \brief Moves the trace at from to position to; false for an index out of range. */
    bool move(int from, int to);

    /*! \brief Trace at position i (0 <= i < size(), asserted). */
    const XYTrace& at(int i) const;
    /*! \brief Replaces the trace at position i, keeping its id; false out of range. */
    bool replace(int i, const XYTrace& trace);
    inline QString idAt(int i) const { return m_ids.value(i); }
    inline int indexOf(const QString& id) const { return m_ids.indexOf(id); }
    inline bool contains(const QString& id) const { return m_ids.contains(id); }
    /*! \brief Trace with this id; an empty trace if there is none. */
    XYTrace trace(const QString& id) const;
    inline QStringList ids() const { return m_ids; }

    /*! \brief x range covered by every non-empty trace: [max of the xMin, min of the xMax].
     * False if there is no non-empty trace or the ranges do not overlap (lo > hi). */
    bool commonRange(double* lo, double* hi) const;

    /*! \brief True if all traces have the same number of points and x_i agrees with that of the first
     * trace within relTolerance * |span of the first axis| (exact for a zero span). */
    bool hasCommonAxis(double relTolerance = 1e-9) const;

    /*! \brief All traces linearly interpolated onto one axis: the points of the first trace inside
     * commonRange().
     *
     * Ids, names and axes are kept. The first trace keeps its values bit for bit, a trace on the same
     * axis as well (interpolation at a node returns the node value). No extrapolation is needed
     * because the axis lies inside every trace. Every trace must be valid with a monotone axis (two
     * or more points); otherwise, or without a common range, the result is empty and *error says why.
     */
    TraceSet resampleToCommonAxis(QString* error = nullptr) const;

private:
    static QString newId();
    QStringList m_ids;
    QVector<XYTrace> m_traces;
};
