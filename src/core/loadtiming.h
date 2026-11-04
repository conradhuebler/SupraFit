/*
 * SupraFit - accumulated wall-clock timing of a project load
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
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QString>

/**
 * @brief Where the time of opening a project file goes, summed per section.
 *
 * Opening a file runs the same steps many times (one model widget per model, one chart update per
 * model, one tree rebuild per signal) and partly after LoadFile has returned (queued signals), so a
 * sequential recorder like PhaseTiming cannot show it. A section is timed by a scope object; equal
 * names add up, nested sections count in both. Reset() before a load, Sections() after it
 * (src/tests/benchmark_projectload prints them). The scopes sit in GUI-thread code of a load only.
 *
 * Off unless SUPRAFIT_LOAD_TIMING=1 is set: a disabled Scope costs one static bool test.
 *
 * Claude Generated.
 */
namespace LoadTiming {

/*! \brief Whether load timing was requested. Checked once per process. */
inline bool Enabled()
{
    static const bool enabled = qEnvironmentVariableIntValue("SUPRAFIT_LOAD_TIMING") == 1;
    return enabled;
}

/*! \brief Accumulated (ms, calls) per section name (GUI thread only). */
inline QMap<QString, QPair<double, int>>& Sections()
{
    static QMap<QString, QPair<double, int>> sections;
    return sections;
}

/*! \brief Forget everything measured so far. */
inline void Reset()
{
    Sections().clear();
}

/*! \brief Times the enclosing scope and adds it to the section of its name. */
class Scope {
public:
    explicit Scope(const char* name)
        : m_name(name)
    {
        if (Enabled())
            m_timer.start();
    }
    ~Scope()
    {
        if (!Enabled())
            return;
        auto& entry = Sections()[QString::fromLatin1(m_name)];
        entry.first += m_timer.nsecsElapsed() / 1e6;
        entry.second += 1;
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    const char* m_name;
    QElapsedTimer m_timer;
};

} // namespace LoadTiming
