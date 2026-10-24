/*
 * SupraFit - landscape sampling of a fitted model: global search and profile confidence intervals
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
 * Claude Generated (roadmap/landscape_sampling.md, WP-C).
 */

#pragma once

#include "src/capabilities/abstractsearchclass.h"
#include "src/capabilities/landscapesampler.h"

#include <QtCore/QJsonObject>
#include <QtCore/QSharedPointer>

#include <atomic>
#include <vector>

class AbstractModel;

/*! \brief SSE of a VarPro model as a function of its enabled global parameters (Claude Generated).
 *
 * The variables are the enabled global parameters (log10 beta of the reaction models). Each call sets them,
 * projects the linear local parameters out by (masked) least squares and returns the squared norm of the
 * residuals plus the boundary penalty, i.e. exactly the objective `VarProFit` minimises. SSE is therefore
 * already the profile over the linear parameters. The model is changed by every call: give each thread its
 * own clone.
 */
class ModelLandscape {
public:
    explicit ModelLandscape(QSharedPointer<AbstractModel> model);

    /*! \brief false, with the reason, for models without variable projection or without enabled globals. */
    bool Usable(QString* reason = nullptr) const;
    int Dimension() const { return int(m_index.size()); }
    /*! \brief Index of variable i among the global parameters of the model. */
    int GlobalIndex(int i) const { return m_index[i]; }
    /*! \brief The global parameters as they are now. */
    std::vector<double> Start() const;
    double operator()(const std::vector<double>& x);

private:
    QSharedPointer<AbstractModel> m_model;
    std::vector<int> m_index;
};

/*! \brief Walks the SSE landscape of a fitted model and reads confidence intervals from its profile (Claude Generated).
 *
 * Several walkers (`LandscapeSampler`, each on its own model clone) explore the box around the fitted global
 * parameters; their profiles are merged and cut at the F-test level, so each parameter gets the
 * likelihood-based interval (correlated parameters included), and other minima inside the wall are reported.
 * The model should be fitted before: the fitted point is the centre of the box and defines SSE_min.
 *
 * Controller keys (all optional):
 *  - `Confidence` percent, 95 (keys are read case-insensitively, as the job blocks write `confidence`)
 *  - `Polish` how many minima found by the walkers are refitted with the minimiser, 5
 *  - `Walkers` (threads), `Seed` 1, `MaxSteps` 50000, `Sampler` "Metropolis" | "Langevin"
 *  - `Range` half width of the box in log10 units for every variable; 0 (default) takes `BoxFactor` (4) times
 *    the extent of the confidence level along that variable alone
 *  - `MaxExpansions` how often the box of a variable is doubled and the walk repeated when an interval reaches
 *    the box (correlated constants have wider profile than section extents), 4
 *  - `CutFactor` wall at SSE_min + CutFactor (level - SSE_min), 3
 *  - `TemperatureFactor` kT = factor * 2 SSE_min/(n - p), 1
 *  - `Bins` resolution of profile and coverage, 60
 *
 * The result is one JSON object (`Result()["0"]`) with the intervals, profile, minimum candidates and the
 * statistics of every walker.
 */
class LandscapeSearch : public AbstractSearchClass {
    Q_OBJECT

public:
    explicit LandscapeSearch(QObject* parent = nullptr);
    ~LandscapeSearch() override;

    bool Run() override;

    /*! \brief Merged outcome of the last run. */
    const LandscapeResult& Merged() const { return m_merged; }
    /*! \brief Intervals of the last run per variable. */
    const std::vector<std::vector<LandscapeSegment>>& Intervals() const { return m_intervals; }
    double Level() const { return m_level; }
    const std::vector<double>& Lower() const { return m_lower; }
    const std::vector<double>& Upper() const { return m_upper; }

public slots:
    void Interrupt() override;

private:
    /*! One-sided distance from x0 along variable i until the landscape reaches `level` (doubling, then bisection). */
    static double Extent(ModelLandscape& landscape, const std::vector<double>& x0, int i, double sign, double level, double limit);

    LandscapeResult m_merged;
    std::vector<std::vector<LandscapeSegment>> m_intervals;
    double m_level = 0;
    std::vector<double> m_lower, m_upper;
    std::atomic<bool> m_stop{ false };
};
