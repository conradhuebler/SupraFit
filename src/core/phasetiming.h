/*
 * SupraFit - end-to-end phase timing for long-running jobs
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
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <cstdio>

/**
 * @brief Wall-clock breakdown of a statistical job, from the button click to the rendered chart.
 *
 * A statistical run is spread over four objects — ModelWidget starts it, JobManager dispatches,
 * MonteCarloStatistics (or a sibling) computes, and the results widget renders — so a scoped timer
 * cannot see the whole thing. This is a process-wide, single-run recorder: Begin() at the click,
 * Mark() at each phase boundary, End() once the chart is up.
 *
 * The motivation is a concrete observation: the progress bar covers ONLY the fitting phase. Table
 * generation runs on the GUI thread before the first worker starts, and the statistics evaluation
 * plus the results rendering run after the progress dialog was already hidden. Those phases cost the
 * same no matter how fast the model is, so for a fast model they dominate the wall time while the
 * bar appears to freeze. This makes that split measurable instead of a matter of feel.
 *
 * Off unless SUPRAFIT_PHASE_TIMING=1 is set in the environment, so it costs nothing in normal use.
 * Output goes to stdout (and is deliberately not routed through the message system, which would
 * itself show up in the measurement).
 *
 * Claude Generated.
 */
namespace PhaseTiming {

struct Phase {
    QString name;
    qint64 ms;
};

namespace detail {
    inline QMutex mutex;
    inline QElapsedTimer timer;
    inline QVector<Phase> phases;
    inline QString label;
    inline qint64 last = 0;
    inline bool running = false;
}

/*! \brief Whether phase timing was requested. Checked once per process. */
inline bool Enabled()
{
    static const bool enabled = qEnvironmentVariableIntValue("SUPRAFIT_PHASE_TIMING") == 1;
    return enabled;
}

/*! \brief Start a run. A second Begin() without End() restarts (a job was aborted). */
inline void Begin(const QString& what)
{
    if (!Enabled())
        return;
    QMutexLocker locker(&detail::mutex);
    detail::label = what;
    detail::phases.clear();
    detail::timer.start();
    detail::last = 0;
    detail::running = true;
}

/*! \brief Close the phase that ends here and name it. No-op when no run is active. */
inline void Mark(const QString& phase)
{
    if (!Enabled())
        return;
    QMutexLocker locker(&detail::mutex);
    if (!detail::running)
        return;
    const qint64 now = detail::timer.elapsed();
    detail::phases.push_back({ phase, now - detail::last });
    detail::last = now;
}

/*! \brief Finish the run and print the breakdown, each phase with its share of the total. */
inline void End()
{
    if (!Enabled())
        return;
    QMutexLocker locker(&detail::mutex);
    if (!detail::running)
        return;
    detail::running = false;
    const qint64 total = detail::timer.elapsed();

    std::printf("\n=== SupraFit phase timing: %s ===\n", qPrintable(detail::label));
    std::printf("  %-44s %10s %8s\n", "phase", "ms", "share");
    for (const Phase& phase : detail::phases) {
        std::printf("  %-44s %10lld %7.1f%%\n", qPrintable(phase.name),
            static_cast<long long>(phase.ms), total > 0 ? 100.0 * phase.ms / total : 0.0);
    }
    std::printf("  %-44s %10lld %7.1f%%\n", "TOTAL (click -> chart on screen)",
        static_cast<long long>(total), 100.0);
    std::fflush(stdout);
    detail::phases.clear();
}

} // namespace PhaseTiming
