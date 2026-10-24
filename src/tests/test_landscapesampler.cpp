/*
 * SupraFit - tests of the metadynamics-like landscape sampler
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
 * WP-A of roadmap/landscape_sampling.md: the sampler runs on analytic surfaces, no model involved. Claude Generated.
 */

#include <cmath>

#include <QtTest/QtTest>

#include "src/capabilities/landscapesampler.h"

namespace {
using V = std::vector<double>;

/*! sum of squares around c with curvatures k: SSE0 + sum k_i (x_i - c_i)^2 */
LandscapeSampler::Function quadratic(V c, V k, double sse0 = 1.0)
{
    return [=](const V& x) {
        double s = sse0;
        for (size_t i = 0; i < x.size(); ++i)
            s += k[i] * (x[i] - c[i]) * (x[i] - c[i]);
        return s;
    };
}

/*! two wells at x = -1 and x = +1 separated by a barrier of `barrier` at x = 0, harmonic in y */
LandscapeSampler::Function doubleWell(double barrier)
{
    return [=](const V& x) { return barrier * (x[0] * x[0] - 1) * (x[0] * x[0] - 1) + 5.0 * x[1] * x[1]; };
}

LandscapeSettings quick()
{
    LandscapeSettings s;
    s.temperature = 1.0;
    s.hillHeight = 0.5;
    s.biasTemperature = 10;
    s.stepSize = 0.04;
    s.stride = 20;
    s.minSteps = 2000;
    s.maxSteps = 60000;
    s.patience = 60;
    s.minHillFraction = 0.5; // plain surfaces need little bias; the barrier tests ask for 0.1
    s.bins = 40;
    return s;
}

bool reachesRightWell(const LandscapeResult& r)
{
    return std::any_of(r.minima.begin(), r.minima.end(), [](const LandscapeCell& c) { return c.x[0] > 0.5 && c.sse < 1.0; });
}
}

class TestLandscapeSampler : public QObject {
    Q_OBJECT

private slots:
    // The profile of a quadratic is the parabola; the run stops by patience, not by the hard cap.
    void quadraticProfileAndTermination()
    {
        const auto f = quadratic({ 0.3, -0.2 }, { 40.0, 10.0 });
        LandscapeSampler sampler(f, { -1, -1 }, { 1, 1 });
        LandscapeSettings s = quick();
        s.sseCut = 1.0 + 12.0;
        sampler.setSettings(s);
        const LandscapeResult r = sampler.Run({ -0.1, 0.1 });

        QCOMPARE(r.reason, LandscapeResult::Stop::Patience);
        QVERIFY2(std::abs(r.best[0] - 0.3) < 0.05 && std::abs(r.best[1] + 0.2) < 0.1,
            qPrintable(QString("best %1 %2").arg(r.best[0]).arg(r.best[1])));
        QVERIFY(r.bestSSE < 1.05);

        // profile in x at the bin around 0.3 + 0.2: SSE0 + 40 * 0.04 = 2.6 (+ binning slack of one bin = 0.05)
        const double dx = 2.0 / s.bins;
        for (int b = 0; b < s.bins; ++b) {
            const double centre = -1 + (b + 0.5) * dx;
            const double lowest = 1.0 + 40.0 * std::pow(std::max(0.0, std::abs(centre - 0.3) - 0.5 * dx), 2);
            if (std::isfinite(r.profile[0][b]) && lowest < 1.0 + 12.0)
                QVERIFY2(r.profile[0][b] >= lowest - 1e-9 && r.profile[0][b] < lowest + 40.0 * 2 * dx + 0.3,
                    qPrintable(QString("bin %1: profile %2, parabola %3").arg(b).arg(r.profile[0][b]).arg(lowest)));
        }
    }

    // No accepted point is above the wall, and the wall limits what is explored.
    void wallIsRespected()
    {
        LandscapeSampler sampler(quadratic({ 0, 0 }, { 40.0, 40.0 }), { -1, -1 }, { 1, 1 });
        LandscapeSettings s = quick();
        s.sseCut = 1.0 + 4.0;
        sampler.setSettings(s);
        const LandscapeResult r = sampler.Run({ 0, 0 });
        QVERIFY(r.maxAcceptedSSE <= s.sseCut);
        // radius sqrt(4/40) = 0.32 of a box of half-width 1: well below the full 40 x 40 cells
        QVERIFY2(r.cellsVisited < 40 * 40 / 4, qPrintable(QString::number(r.cellsVisited)));
    }

    void startAboveTheWallIsReported()
    {
        LandscapeSampler sampler(quadratic({ 0 }, { 10.0 }), { -1 }, { 1 });
        LandscapeSettings s = quick();
        s.sseCut = 2.0;
        sampler.setSettings(s);
        QCOMPARE(sampler.Run({ 1.0 }).reason, LandscapeResult::Stop::StartAboveCut);
    }

    // A barrier of 30 kT is never crossed without bias and is crossed with it.
    void biasCrossesTheBarrier()
    {
        const auto f = doubleWell(30.0);
        LandscapeSettings s = quick();
        s.sseCut = 36.0;
        s.maxSteps = 120000;
        s.patience = 150;
        s.minHillFraction = 0.1;

        {
            LandscapeSampler plain(f, { -1.8, -1 }, { 1.8, 1 });
            LandscapeSettings p = s;
            p.hillHeight = 0;
            p.maxSteps = 40000;
            plain.setSettings(p);
            const LandscapeResult r = plain.Run({ -1, 0 });
            QVERIFY2(!reachesRightWell(r), "without bias the walker must stay in its well");
        }
        {
            LandscapeSampler biased(f, { -1.8, -1 }, { 1.8, 1 });
            biased.setSettings(s);
            const LandscapeResult r = biased.Run({ -1, 0 });
            QVERIFY2(reachesRightWell(r),
                qPrintable(QString("second well not found, %1 steps, %2 hills").arg(r.steps).arg(r.hills)));
            // the minimum candidates of both wells are reported, best first
            QVERIFY(r.minima.size() >= 2);
        }
    }

    // Langevin dynamics with finite-difference gradients find the same second well.
    void langevinCrossesTheBarrier()
    {
        const auto f = doubleWell(30.0);
        LandscapeSettings s = quick();
        s.sampler = LandscapeSettings::Sampler::Langevin;
        s.stepSize = 0.02; // explicit Euler: stepSize^2 width^2 k / (2 kT) must stay below ~1 (k = 240 in the wells)
        s.sseCut = 36.0;
        s.maxSteps = 200000;
        s.patience = 150;
        s.minHillFraction = 0.1;
        LandscapeSampler sampler(f, { -1.8, -1 }, { 1.8, 1 });
        sampler.setSettings(s);
        const LandscapeResult r = sampler.Run({ -1, 0 });
        QVERIFY2(reachesRightWell(r), qPrintable(QString("steps %1, acceptance %2").arg(r.steps).arg(r.acceptance)));
        QVERIFY(r.evaluations > r.steps); // the gradient costs evaluations
    }

    // One seed proves little: the barrier must fall for every seed of both samplers (worst case pinned).
    void barrierFallsForEverySeed()
    {
        const auto f = doubleWell(30.0);
        for (const auto sampler : { LandscapeSettings::Sampler::Metropolis, LandscapeSettings::Sampler::Langevin }) {
            int found = 0;
            long worstSteps = 0;
            for (uint64_t seed = 1; seed <= 20; ++seed) {
                LandscapeSettings s = quick();
                s.sampler = sampler;
                s.stepSize = sampler == LandscapeSettings::Sampler::Langevin ? 0.02 : 0.04;
                s.sseCut = 36.0;
                s.maxSteps = 200000;
                s.patience = 150;
                s.minHillFraction = 0.1;
                s.seed = seed;
                LandscapeSampler run(f, { -1.8, -1 }, { 1.8, 1 });
                run.setSettings(s);
                const LandscapeResult r = run.Run({ -1, 0 });
                if (reachesRightWell(r))
                    ++found;
                else
                    qInfo() << "seed" << seed << "reason" << int(r.reason) << "steps" << r.steps << "hills" << r.hills
                            << "acceptance" << r.acceptance << "cells" << r.cellsVisited << "best" << r.bestSSE;
                worstSteps = std::max(worstSteps, r.steps);
            }
            qInfo() << (sampler == LandscapeSettings::Sampler::Langevin ? "Langevin" : "Metropolis")
                    << "second well found in" << found << "of 20 seeds, longest run" << worstSteps << "steps";
            QCOMPARE(found, 20);
        }
    }

    // Well-tempering: hills shrink while the bias grows.
    void hillsShrinkWhenWellTempered()
    {
        LandscapeSampler sampler(quadratic({ 0, 0 }, { 5.0, 5.0 }), { -1, -1 }, { 1, 1 });
        LandscapeSettings s = quick();
        s.sseCut = 6.0;
        s.minSteps = 30000;
        s.maxSteps = 30000;
        sampler.setSettings(s);
        const LandscapeResult r = sampler.Run({ 0, 0 });
        QVERIFY2(r.lastHillHeight < 0.5 * s.hillHeight, qPrintable(QString::number(r.lastHillHeight)));
        QVERIFY(sampler.BiasAt({ 0, 0 }) > 0);
        QVERIFY(sampler.FreeEnergyAt({ 0, 0 }) < 0);
    }

    // A direction without any gradient (flat valley) is covered and the run ends by patience;
    // on a box too large to cover in the budget it ends by the hard cap.
    void flatValleyTerminates()
    {
        const auto f = [](const V& x) { return 1.0 + 20.0 * x[0] * x[0] + 0.0 * x[1]; };
        {
            LandscapeSampler sampler(f, { -1, 0 }, { 1, 1 });
            LandscapeSettings s = quick();
            s.sseCut = 3.0;
            s.bins = 20;
            sampler.setSettings(s);
            const LandscapeResult r = sampler.Run({ 0, 0.5 });
            QCOMPARE(r.reason, LandscapeResult::Stop::Patience);
            QVERIFY(std::isfinite(r.profile[1].front()) && std::isfinite(r.profile[1].back()));
        }
        {
            LandscapeSampler sampler(f, { -1, 0 }, { 1, 1e6 });
            LandscapeSettings s = quick();
            s.sseCut = 3.0;
            s.maxSteps = 5000;
            s.minSteps = 5000;
            sampler.setSettings(s);
            QCOMPARE(sampler.Run({ 0, 0.5 }).reason, LandscapeResult::Stop::MaxSteps);
        }
    }

    // Four variables use the hill list instead of the grid.
    void fourVariablesUseTheHillList()
    {
        const V c{ 0.2, -0.3, 0.1, 0.4 };
        LandscapeSampler sampler(quadratic(c, { 30, 30, 30, 30 }), { -1, -1, -1, -1 }, { 1, 1, 1, 1 });
        LandscapeSettings s = quick();
        s.sseCut = 4.0;
        s.bins = 10;
        s.maxSteps = 20000;
        s.minSteps = 1000;
        s.patience = 40;
        sampler.setSettings(s);
        const LandscapeResult r = sampler.Run({ 0.25, -0.25, 0.15, 0.35 });
        QVERIFY(r.hills > 0);
        QVERIFY2(r.bestSSE < 1.0 + 30.0 * 4 * 0.04, qPrintable(QString::number(r.bestSSE)));
    }

    void sameSeedSameRun()
    {
        const auto f = doubleWell(10.0);
        LandscapeSettings s = quick();
        s.sseCut = 14;
        s.maxSteps = 5000;
        s.minSteps = 5000;
        LandscapeSampler a(f, { -1.8, -1 }, { 1.8, 1 }), b(f, { -1.8, -1 }, { 1.8, 1 });
        a.setSettings(s);
        b.setSettings(s);
        const LandscapeResult ra = a.Run({ -1, 0 }), rb = b.Run({ -1, 0 });
        QCOMPARE(ra.evaluations, rb.evaluations);
        QCOMPARE(ra.bestSSE, rb.bestSSE);
        QCOMPARE(ra.cellsVisited, rb.cellsVisited);
    }

    void interruptEndsTheRun()
    {
        LandscapeSampler sampler(quadratic({ 0 }, { 1.0 }), { -1 }, { 1 });
        int polls = 0;
        sampler.setInterrupt([&] { return ++polls > 50; });
        const LandscapeResult r = sampler.Run({ 0 });
        QCOMPARE(r.reason, LandscapeResult::Stop::Interrupted);
        QVERIFY(r.steps <= 51);
    }

    void unusableSettingsThrow()
    {
        LandscapeSampler sampler(quadratic({ 0 }, { 1.0 }), { -1 }, { 1 });
        LandscapeSettings s;
        s.temperature = 0;
        sampler.setSettings(s);
        QVERIFY_EXCEPTION_THROWN(sampler.Run({ 0 }), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(LandscapeSampler(quadratic({ 0 }, { 1.0 }), { 1 }, { -1 }), std::invalid_argument);
    }

    // ---- WP-B: confidence region from the profile ----

    void confidenceLevelIsTheFTestThreshold()
    {
        // same formula as AbstractModel::ErrorfTestThreshold: SSE (F p / (n - p) + 1)
        QCOMPARE(LandscapeConfidenceLevel(2.0, 12, 2, 4.0), 3.6);
    }

    // Correlated parameters: the interval is the profile (minimum over the other variable), analytically
    // c +- sqrt((L - SSE0) / (a - h^2/b)), wider than the section through the minimum c +- sqrt((L - SSE0) / a).
    void profileIntervalOfACorrelatedParabola()
    {
        const double a = 60, b = 40, h = 35, sse0 = 1.0;
        const V c{ 0.1, -0.2 };
        const auto f = [=](const V& x) {
            const double dx = x[0] - c[0], dy = x[1] - c[1];
            return sse0 + a * dx * dx + b * dy * dy + 2 * h * dx * dy;
        };
        const double level = LandscapeConfidenceLevel(sse0, 12, 2, 3.0); // 1.6
        const double halfX = std::sqrt((level - sse0) / (a - h * h / b));
        const double halfY = std::sqrt((level - sse0) / (b - h * h / a));
        const double section = std::sqrt((level - sse0) / a);
        QVERIFY(halfX - section > 0.04); // the test can tell profile and section apart

        // the worst deviation over several seeds is what counts, not one run
        const double tolerance = 0.01; // measured worst case 0.0025
        double worst = 0;
        for (uint64_t seed = 1; seed <= 10; ++seed) {
            LandscapeSampler sampler(f, { -1, -1 }, { 1, 1 });
            LandscapeSettings s = quick();
            s.sseCut = 2.4; // above the level, otherwise the profile cannot cross it
            s.bins = 120;
            s.maxSteps = 200000;
            s.patience = 100;
            s.seed = seed;
            sampler.setSettings(s);
            const LandscapeResult r = sampler.Run({ 0.1, -0.2 });
            const auto intervals = LandscapeIntervals(r, level, { -1, -1 }, { 1, 1 });

            QCOMPARE(intervals[0].size(), size_t(1));
            QCOMPARE(intervals[1].size(), size_t(1));
            const LandscapeSegment &sx = intervals[0][0], &sy = intervals[1][0];
            QVERIFY(sx.Bounded() && sy.Bounded());
            const double dev = std::max({ std::abs(sx.lower - (c[0] - halfX)), std::abs(sx.upper - (c[0] + halfX)),
                std::abs(sy.lower - (c[1] - halfY)), std::abs(sy.upper - (c[1] + halfY)) });
            worst = std::max(worst, dev);
        }
        qInfo() << "worst deviation of an interval end over 10 seeds:" << worst << "(bin width" << 2.0 / 120 << ")";
        QVERIFY2(worst < tolerance, qPrintable(QString("worst deviation %1").arg(worst)));
    }

    // A level above the box is cut by the box and says so.
    void boxEdgesAreMarked()
    {
        LandscapeSampler sampler(quadratic({ 0 }, { 1.0 }), { -1 }, { 1 });
        LandscapeSettings s = quick();
        s.sseCut = 10;
        sampler.setSettings(s);
        const LandscapeResult r = sampler.Run({ 0 });
        const auto intervals = LandscapeIntervals(r, 5.0, { -1 }, { 1 }); // sqrt(4) = 2 > half box
        QCOMPARE(intervals[0].size(), size_t(1));
        QCOMPARE(intervals[0][0].lowerEdge, LandscapeSegment::Edge::Box);
        QCOMPARE(intervals[0][0].upperEdge, LandscapeSegment::Edge::Box);
        QCOMPARE(intervals[0][0].lower, -1.0);
        QCOMPARE(intervals[0][0].upper, 1.0);
    }

    // Too short a run leaves the neighbours of the stretch unvisited: not bounded, and it says so.
    void unsampledEdgesAreMarked()
    {
        LandscapeSampler sampler(quadratic({ 0, 0 }, { 40.0, 40.0 }), { -1, -1 }, { 1, 1 });
        LandscapeSettings s = quick();
        s.sseCut = 1.0 + 4.0;
        s.maxSteps = 200;
        s.minSteps = 200;
        sampler.setSettings(s);
        const LandscapeResult r = sampler.Run({ 0, 0 });
        const auto intervals = LandscapeIntervals(r, 3.0, { -1, -1 }, { 1, 1 });
        QVERIFY(!intervals[0].empty());
        QVERIFY(!intervals[0][0].Bounded());
    }

    // Two minima within the level are two stretches, one around each well.
    void separateMinimaAreSeparateSegments()
    {
        LandscapeSampler sampler(doubleWell(30.0), { -1.8, -1 }, { 1.8, 1 });
        LandscapeSettings s = quick();
        s.sseCut = 36.0;
        s.maxSteps = 200000;
        s.patience = 150;
        s.minHillFraction = 0.1;
        s.bins = 60;
        sampler.setSettings(s);
        const LandscapeResult r = sampler.Run({ -1, 0 });
        const auto intervals = LandscapeIntervals(r, 2.0, { -1.8, -1 }, { 1.8, 1 });
        QCOMPARE(intervals[0].size(), size_t(2));
        // 30 (x^2 - 1)^2 ~ 120 (x -+ 1)^2 = 2 at +-0.129 around each well
        for (int w = 0; w < 2; ++w) {
            const double centre = w ? 1.0 : -1.0;
            QVERIFY2(std::abs(intervals[0][w].lower - (centre - 0.129)) < 0.06 && std::abs(intervals[0][w].upper - (centre + 0.129)) < 0.06,
                qPrintable(QString("well %1: [%2, %3]").arg(w).arg(intervals[0][w].lower).arg(intervals[0][w].upper)));
        }
    }

    void naturalTemperature()
    {
        QCOMPARE(LandscapeSampler::NaturalTemperature(0.5, 13, 3), 0.1);
    }
};

QTEST_MAIN(TestLandscapeSampler)
#include "test_landscapesampler.moc"
