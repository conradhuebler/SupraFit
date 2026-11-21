/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QElapsedTimer>
#include <QtWidgets/QApplication>

class Waiter {
public:
    inline Waiter() { QApplication::setOverrideCursor(QCursor(Qt::WaitCursor)); }
    inline ~Waiter() { QApplication::restoreOverrideCursor(); }
};

/*! \brief Wait cursor for work that is sometimes slow (Claude Generated, UX package 1).
 *
 * Shows the wait cursor only if the previous run of the same work took longer than threshold ms, and
 * stores the duration of this run in *lastMs. Fast work (a step of a spin box) does not make the
 * cursor flicker; work that became slow (a lineshape fit, many FIDs) gets the cursor from its next run on. */
class AdaptiveWaiter {
public:
    /*! \brief force: show the cursor anyway (work known to be slow, e.g. a lineshape fit). */
    inline explicit AdaptiveWaiter(qint64* lastMs, bool force = false, qint64 threshold = 300)
        : m_last(lastMs)
        , m_active(force || (lastMs && *lastMs > threshold))
    {
        if (m_active)
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        m_timer.start();
    }
    inline ~AdaptiveWaiter()
    {
        if (m_last)
            *m_last = m_timer.elapsed();
        if (m_active)
            QApplication::restoreOverrideCursor();
    }
    AdaptiveWaiter(const AdaptiveWaiter&) = delete;
    AdaptiveWaiter& operator=(const AdaptiveWaiter&) = delete;

private:
    qint64* m_last;
    bool m_active;
    QElapsedTimer m_timer;
};
