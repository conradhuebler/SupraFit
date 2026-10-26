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

#include "landscapesearch.h"

#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>

#include <algorithm>
#include <cmath>
#include <random>

namespace {
constexpr int MaxSurfacePairs = 6; ///< 2D profiles kept in the result (4 variables: all pairs)
constexpr double ExtentLimit = 8.0; ///< log10 units: further than this the level counts as "not reached"

QString StopName(LandscapeResult::Stop reason)
{
    switch (reason) {
    case LandscapeResult::Stop::Patience:
        return QStringLiteral("patience");
    case LandscapeResult::Stop::MaxSteps:
        return QStringLiteral("max_steps");
    case LandscapeResult::Stop::Interrupted:
        return QStringLiteral("interrupted");
    case LandscapeResult::Stop::StartAboveCut:
        return QStringLiteral("start_above_cut");
    }
    return QString();
}

QString EdgeName(LandscapeSegment::Edge edge)
{
    switch (edge) {
    case LandscapeSegment::Edge::Crossing:
        return QStringLiteral("crossing");
    case LandscapeSegment::Edge::Box:
        return QStringLiteral("box");
    case LandscapeSegment::Edge::Unsampled:
        return QStringLiteral("unsampled");
    }
    return QString();
}

QJsonArray ToArray(const std::vector<double>& v)
{
    QJsonArray a;
    for (double x : v)
        a << x;
    return a;
}

/*! One walker: its own model clone, its own start and seed. */
class Walker : public QRunnable {
public:
    Walker(QSharedPointer<AbstractModel> model, std::vector<double> lower, std::vector<double> upper,
        LandscapeSettings settings, const std::atomic<bool>* stop)
        : m_model(std::move(model))
        , m_lower(std::move(lower))
        , m_upper(std::move(upper))
        , m_settings(settings)
        , m_stop(stop)
    {
        setAutoDelete(false);
    }
    /*! \brief Steps the walker has done (updated every 200 steps by the walker, read by the thread that waits for it); at the end the
     *  full number of steps, so that a walker that stopped early counts as complete. */
    std::atomic<long> stepsDone{ 0 };
    /*! \brief Points the walker accepts, (coordinates, SSE), for 2D profiles. Filled by the walker thread, read after waitForDone. */
    std::vector<std::vector<double>> points;

    void setStart(const std::vector<double>& x) { m_start = x; }
    void setRandomStart(double cut) { m_randomStartCut = cut; }

    void run() override
    {
        ModelLandscape landscape(m_model);
        LandscapeSampler sampler([&landscape](const std::vector<double>& x) { return landscape(x); }, m_lower, m_upper);
        sampler.setSettings(m_settings);
        sampler.setInterrupt([this] { return m_stop->load(); });
        sampler.setProgress([this](long step) { stepsDone.store(step); });
        if (m_settings.recordPoints)
            sampler.addListener([this](const std::vector<double>& x, double sse) {
                points.push_back(x);
                points.back().push_back(sse);
            });

        std::vector<double> start = m_start;
        if (m_randomStartCut > 0) { // somewhere below the wall, else the fitted point
            std::mt19937_64 rng(m_settings.seed);
            std::uniform_real_distribution<double> u(0.0, 1.0);
            for (int attempt = 0; attempt < 20000; ++attempt) { // the region below the wall is a small part of a large box
                std::vector<double> x(m_lower.size());
                for (size_t i = 0; i < x.size(); ++i)
                    x[i] = m_lower[i] + u(rng) * (m_upper[i] - m_lower[i]);
                if (landscape(x) < m_randomStartCut) {
                    start = x;
                    break;
                }
            }
        }
        m_start = start;
        m_result = sampler.Run(start);
        stepsDone.store(m_settings.maxSteps);
    }

    const LandscapeResult& Result() const { return m_result; }
    const std::vector<double>& Start() const { return m_start; }

private:
    QSharedPointer<AbstractModel> m_model;
    std::vector<double> m_lower, m_upper, m_start;
    LandscapeSettings m_settings;
    const std::atomic<bool>* m_stop;
    double m_randomStartCut = 0;
    LandscapeResult m_result;
};
}

ModelLandscape::ModelLandscape(QSharedPointer<AbstractModel> model)
    : m_model(std::move(model))
{
    if (!m_model)
        return;
    m_model->CalculateStatistics(false);
    m_model->setFast(true);
    for (int k = 0; k < m_model->GlobalParameterSize(); ++k)
        if (m_model->GlobalParameter()->isChecked(0, k))
            m_index.push_back(k);
}

bool ModelLandscape::Usable(QString* reason) const
{
    QString why;
    if (!m_model)
        why = QObject::tr("No model.");
    else if (!m_model->SupportsVarPro())
        why = QObject::tr("The landscape is sampled for models with variable projection only (the linear parameters are projected out).");
    else if (m_index.empty())
        why = QObject::tr("The model has no enabled global parameter.");
    if (reason)
        *reason = why;
    return why.isEmpty();
}

std::vector<double> ModelLandscape::Start() const
{
    std::vector<double> x;
    for (int k : m_index)
        x.push_back(m_model->GlobalParameter(k));
    return x;
}

double ModelLandscape::operator()(const std::vector<double>& x)
{
    for (size_t i = 0; i < m_index.size(); ++i)
        m_model->setGlobalParameter(x[i], m_index[i]);
    m_model->ProjectLinearParameters();
    m_model->Calculate();
    const QList<double> err = m_model->getCalculatedAbsoluteErrors();
    const QList<double> pen = m_model->getPenalty();
    double sse = 0;
    for (int i = 0; i < err.size(); ++i) {
        const double r = err[i] + (i < pen.size() ? pen[i] : 0.0);
        sse += r * r;
    }
    return sse;
}

LandscapeSearch::LandscapeSearch(QObject* parent)
    : AbstractSearchClass(parent)
{
}

LandscapeSearch::~LandscapeSearch() = default;

void LandscapeSearch::Interrupt()
{
    m_stop = true;
    AbstractSearchClass::Interrupt();
}

double LandscapeSearch::Extent(ModelLandscape& landscape, const std::vector<double>& x0, int i, double sign, double level, double limit)
{
    std::vector<double> x = x0;
    double inside = 0, outside = 0.05;
    while (outside < limit) {
        x[i] = x0[i] + sign * outside;
        if (!(landscape(x) <= level))
            break;
        inside = outside;
        outside *= 2;
    }
    if (outside >= limit)
        return limit;
    for (int iteration = 0; iteration < 20; ++iteration) {
        const double mid = 0.5 * (inside + outside);
        x[i] = x0[i] + sign * mid;
        (landscape(x) <= level ? inside : outside) = mid;
    }
    return 0.5 * (inside + outside);
}

bool LandscapeSearch::Run()
{
    m_stop = false;
    m_recentered = 0;
    m_carryEvaluations = 0;
    m_startSSE = 0;
    return Execute();
}

bool LandscapeSearch::Execute()
{
    m_merged = LandscapeResult();
    m_intervals.clear();
    m_results.clear(); // points of an older round are kept: they are part of the coverage; only the first round clears

    if (!m_model) {
        emit Message(tr("No model for the landscape search."));
        return false;
    }
    const qint64 t0 = QDateTime::currentMSecsSinceEpoch();

    ModelLandscape base(m_model->Clone(false));
    QString why;
    if (!base.Usable(&why)) {
        emit Message(why);
        return false;
    }
    const int d = base.Dimension();
    const std::vector<double> x0 = base.Start();
    const double sseMin = base(x0);
    if (m_recentered == 0)
        m_startSSE = sseMin;
    const int points = m_model->Points(), parameters = m_model->Parameter();
    if (!(sseMin > 1e-300) || points <= parameters) {
        emit Message(tr("The landscape search needs a fit with residuals (SSE %1, %2 points, %3 parameters).").arg(sseMin).arg(points).arg(parameters));
        return false;
    }

    const double confidence = AccessCI(m_controller, "Confidence").toDouble(95.0);
    m_level = LandscapeConfidenceLevel(sseMin, points, parameters, m_model->finv(confidence / 100.0));
    // Mode "Minima" looks for other minima: the wall is a multiple of the SSE of the fit (a wrong basin lies at several times the
    // optimum, not at the F-test level), the box is a fixed one around the fit and the search is not re-centred on a deeper
    // minimum - that one is listed first instead.
    const bool finder = AccessCI(m_controller, "Mode").toString(QStringLiteral("Confidence")) == QLatin1String("Minima");
    const double searchWall = std::max(1.5, AccessCI(m_controller, "SearchWall").toDouble(10.0));
    const double cutFactor = AccessCI(m_controller, "CutFactor").toDouble(3.0);
    const double cut = finder ? sseMin * searchWall : sseMin + cutFactor * (m_level - sseMin);

    // box: fixed half width, or a multiple of the extent of the level along each variable alone
    const double range = AccessCI(m_controller, "Range").toDouble(finder ? 6.0 : 0.0);
    const double boxFactor = AccessCI(m_controller, "BoxFactor").toDouble(4.0);
    const double maxRange = AccessCI(m_controller, "MaxRange").toDouble(10.0);
    std::vector<double> half(d, range);
    if (!(range > 0)) {
        emit Stage(tr("Setting up: how far the confidence region reaches along each constant"));
        emit setMaximumSteps(2 * d);
    }
    for (int i = 0; i < d; ++i)
        if (!(range > 0)) {
            QElapsedTimer timer;
            timer.start();
            const double up = Extent(base, x0, i, +1.0, m_level, ExtentLimit);
            emit IncrementProgress(int(std::max<qint64>(1, timer.restart())));
            QCoreApplication::processEvents();
            const double down = Extent(base, x0, i, -1.0, m_level, ExtentLimit);
            emit IncrementProgress(int(std::max<qint64>(1, timer.elapsed())));
            QCoreApplication::processEvents();
            half[i] = std::min(maxRange, boxFactor * std::max(std::max(up, down), 1e-3));
        }
    m_lower.assign(d, 0.0);
    m_upper.assign(d, 0.0);

    LandscapeSettings settings;
    const QString sampler = AccessCI(m_controller, "Sampler").toString("Metropolis");
    settings.sampler = sampler == QLatin1String("Langevin") ? LandscapeSettings::Sampler::Langevin : LandscapeSettings::Sampler::Metropolis;
    settings.temperature = AccessCI(m_controller, "TemperatureFactor").toDouble(1.0) * LandscapeSampler::NaturalTemperature(sseMin, points, parameters);
    if (finder) // the barriers of interest are as high as the wall: a temperature of the statistical scale would never leave the basin
        settings.temperature = AccessCI(m_controller, "SearchTemperature").toDouble(0.02) * (cut - sseMin);
    // integration step and finite-difference step, fractions of the sampled range; 0 takes the default
    const double step = AccessCI(m_controller, "StepSize").toDouble(0.0);
    settings.stepSize = step > 0 ? step : (settings.sampler == LandscapeSettings::Sampler::Langevin ? 0.04 : 0.05);
    const double gradientStep = AccessCI(m_controller, "GradientStep").toDouble(0.0);
    if (gradientStep > 0)
        settings.gradientStep = gradientStep;
    settings.centralGradient = AccessCI(m_controller, "CentralGradient").toBool(false);
    settings.langevinCorrection = AccessCI(m_controller, "LangevinCorrection").toBool(true);
    // bias and stopping: how fast the region fills, which barriers can be taken down and when a walker gives up
    const double hillHeightFactor = AccessCI(m_controller, "HillHeightFactor").toDouble(0.25);
    const double biasFactor = AccessCI(m_controller, "BiasFactor").toDouble(0.5);
    const double hillWidth = AccessCI(m_controller, "HillWidth").toDouble(0.05);
    settings.hillHeight = hillHeightFactor * settings.temperature;
    settings.biasTemperature = biasFactor * (cut - sseMin); // takes down barriers up to about 2.3 times this
    if (hillWidth > 0)
        settings.hillWidth = hillWidth;
    settings.minSteps = AccessCI(m_controller, "MinSteps").toInt(settings.minSteps);
    settings.patience = std::max(1, AccessCI(m_controller, "Patience").toInt(settings.patience));
    const double minHillFraction = AccessCI(m_controller, "MinHillFraction").toDouble(settings.minHillFraction);
    if (minHillFraction > 0)
        settings.minHillFraction = minHillFraction;
    settings.sseCut = cut;
    settings.maxSteps = AccessCI(m_controller, "MaxSteps").toInt(50000);
    settings.bins = AccessCI(m_controller, "Bins").toInt(60);
    settings.seed = uint64_t(AccessCI(m_controller, "Seed").toInt(1));

    m_lastSettings = settings;
    const bool recordPoints = d >= 2 && AccessCI(m_controller, "Surface").toBool(false);
    settings.recordPoints = recordPoints;

    int walkers = AccessCI(m_controller, "Walkers").toInt(qApp->instance()->property("threads").toInt());
    walkers = std::max(1, walkers);

    // The extent along one variable alone understates the profile extent of correlated constants: if a
    // stretch reaches the box, that variable's box is doubled and the walk repeated (fixed `Range`: never).
    const int maxExpansions = finder ? 0 : AccessCI(m_controller, "MaxExpansions").toInt(4);
    std::vector<std::unique_ptr<Walker>> run;
    QJsonArray walkerInfo;
    double evaluations = 0;
    int rounds = 0;
    for (;; ++rounds) {
        for (int i = 0; i < d; ++i) {
            m_lower[i] = x0[i] - half[i];
            m_upper[i] = x0[i] + half[i];
        }
        run.clear();
        QThreadPool pool;
        pool.setMaxThreadCount(walkers);
        for (int w = 0; w < walkers; ++w) {
            LandscapeSettings s = settings;
            s.seed = settings.seed + uint64_t(w) + 1000 * uint64_t(rounds);
            run.push_back(std::make_unique<Walker>(m_model->Clone(false), m_lower, m_upper, s, &m_stop));
            run.back()->setStart(x0);
            if (w > 0)
                run.back()->setRandomStart(cut);
            pool.start(run.back().get());
        }

        // One bar unit is 500 steps of one walker. The walkers run in their own threads; this one only reads their counters,
        // keeps the bar and the window alive and takes the stop request. A walker that ends early (nothing new for a long time)
        // counts as complete, the bar then jumps to its end.
        const long chunk = 500;
        const long unitsPerWalker = (settings.maxSteps + chunk - 1) / chunk;
        emit Stage(rounds == 0 ? tr("Sampling the landscape: %1 walkers, up to %2 steps each").arg(walkers).arg(settings.maxSteps)
                               : tr("Sampling again over a wider range (round %1): %2 walkers, up to %3 steps each").arg(rounds + 1).arg(walkers).arg(settings.maxSteps));
        emit setMaximumSteps(int(walkers * unitsPerWalker));
        std::vector<long> reported(run.size(), 0);
        long total = 0;
        QElapsedTimer clock;
        clock.start();
        const int parallel = std::max(1, std::min(walkers, QThreadPool::globalInstance()->maxThreadCount()));
        auto report = [&](bool final) {
            for (size_t w = 0; w < run.size(); ++w) {
                const long done = final ? unitsPerWalker : std::min(unitsPerWalker, run[w]->stepsDone.load() / chunk);
                while (reported[w] < done) {
                    ++reported[w];
                    ++total;
                    // the dialog divides by the number of threads: what one unit cost one thread, from the wall time so far
                    emit IncrementProgress(int(std::max<qint64>(1, clock.elapsed() * parallel / total)));
                }
            }
        };
        while (!pool.waitForDone(50)) {
            report(false);
            QCoreApplication::processEvents();
        }
        report(true);

        walkerInfo = QJsonArray();
        m_merged = LandscapeResult();
        for (size_t w = 0; w < run.size(); ++w) {
            const LandscapeResult& r = run[w]->Result();
            QJsonObject o;
            o["start"] = ToArray(run[w]->Start());
            o["reason"] = StopName(r.reason);
            o["steps"] = double(r.steps);
            o["evaluations"] = double(r.evaluations);
            o["acceptance"] = r.acceptance;
            o["best_sse"] = r.bestSSE;
            walkerInfo << o;
            if (recordPoints)
                m_points.insert(m_points.end(), run[w]->points.begin(), run[w]->points.end());
            if (w == 0)
                m_merged = r;
            else
                m_merged.Merge(r);
        }
        evaluations += double(m_merged.evaluations);
        m_merged.reason = run.front()->Result().reason;
        m_intervals = LandscapeIntervals(m_merged, m_level, m_lower, m_upper);

        bool expand = false;
        if (!(range > 0) && rounds < maxExpansions && !m_stop)
            for (int i = 0; i < d; ++i)
                for (const LandscapeSegment& seg : m_intervals[i])
                    if ((seg.lowerEdge == LandscapeSegment::Edge::Box || seg.upperEdge == LandscapeSegment::Edge::Box) && half[i] < maxRange - 1e-9) {
                        half[i] = std::min(2 * half[i], maxRange);
                        expand = true;
                        break;
                    }
        if (!expand)
            break;
    }

    // The walkers found a deeper minimum than the fit: the level, the box and the intervals belong to that one.
    const int maxRecentre = finder ? 0 : AccessCI(m_controller, "MaxRecentre").toInt(2);
    if (!m_stop && m_recentered < maxRecentre && m_merged.bestSSE < sseMin * (1.0 - 1e-3) && !m_merged.best.empty()) {
        emit Stage(tr("The walkers found a deeper minimum: refitting it"));
        QSharedPointer<AbstractModel> candidate = m_model->Clone(false);
        for (int i = 0; i < d; ++i)
            candidate->setGlobalParameter(m_merged.best[i], base.GlobalIndex(i));
        Minimizer minimizer(false);
        minimizer.setQuiet(true);
        minimizer.setModel(candidate);
        minimizer.Minimize();
        ModelLandscape deeper(candidate);
        const std::vector<double> xd = deeper.Start();
        const double sd = deeper(xd);
        if (std::isfinite(sd) && sd < sseMin * (1.0 - 1e-3)) {
            ++m_recentered;
            m_carryEvaluations += evaluations;
            for (int i = 0; i < d; ++i)
                m_model->setGlobalParameter(xd[i], base.GlobalIndex(i));
            return Execute();
        }
    }

    // Two constants: the walkers visit a path, not an area, and a height line needs whole squares of known cells. The SSE of the
    // cells in and around the region the walkers explored (the product of the ranges of bins the two profiles reached) is therefore
    // evaluated exactly, at their centres: at most bins^2 evaluations, bounded by the box. A cell keeps the lower of that value and
    // the lowest point a walker had in it. The same grid gives the exact profile of each constant (the minimum over the other one),
    // so the intervals no longer end where the walkers happened not to go. With more constants the profile of a variable is a minimum
    // over several others and cannot be filled in this way.
    std::vector<double> exactGrid;
    long exactEvaluations = 0;
    bool exact = false;
    if (d == 2 && !m_stop && AccessCI(m_controller, "ExactSurface").toBool(true)) {
        const int bins = settings.bins;
        int lo[2] = { bins, bins }, hi[2] = { -1, -1 };
        for (int v = 0; v < 2; ++v)
            for (int b = 0; b < bins; ++b)
                if (std::isfinite(m_merged.profile[v][b])) {
                    lo[v] = std::min(lo[v], b);
                    hi[v] = std::max(hi[v], b);
                }
        if (hi[0] >= 0 && hi[1] >= 0) {
            // the rectangle grows until its outermost cells lie above the wall (or at the edge of the box): the region below the wall,
            // and with it every interval, is then enclosed by cells that were evaluated
            int amin = std::max(0, lo[0] - 2), amax = std::min(bins - 1, hi[0] + 2);
            int bmin = std::max(0, lo[1] - 2), bmax = std::min(bins - 1, hi[1] + 2);
            exactGrid.assign(size_t(bins) * size_t(bins), std::numeric_limits<double>::infinity());
            std::vector<char> done(size_t(bins) * size_t(bins), 0);
            std::vector<double> at(2);
            // one bar unit is 25 cells; the number of cells is not known in advance (the rectangle grows), the maximum follows it
            const long cellsPerUnit = 25;
            QElapsedTimer gridClock;
            gridClock.start();
            long reportedUnits = 0;
            auto rectangleCells = [&]() { return long(amax - amin + 1) * long(bmax - bmin + 1); };
            emit Stage(tr("Evaluating the explored region of the two constants exactly"));
            emit setMaximumSteps(int((rectangleCells() + cellsPerUnit - 1) / cellsPerUnit));
            auto evaluateRectangle = [&]() {
                for (int a = amin; a <= amax && !m_stop; ++a)
                    for (int b = bmin; b <= bmax && !m_stop; ++b) {
                        const size_t cell = size_t(a) * bins + b;
                        if (done[cell])
                            continue;
                        at[0] = m_lower[0] + (a + 0.5) * (m_upper[0] - m_lower[0]) / bins;
                        at[1] = m_lower[1] + (b + 0.5) * (m_upper[1] - m_lower[1]) / bins;
                        const double sse = base(at);
                        ++exactEvaluations;
                        done[cell] = 1;
                        if (std::isfinite(sse))
                            exactGrid[cell] = sse;
                        if (exactEvaluations / cellsPerUnit > reportedUnits) {
                            ++reportedUnits;
                            emit IncrementProgress(int(std::max<qint64>(1, gridClock.elapsed() / reportedUnits)));
                            QCoreApplication::processEvents();
                        }
                    }
            };
            evaluateRectangle();
            const int grow = 3;
            for (bool growing = true; growing && !m_stop;) {
                growing = false;
                auto belowWall = [&](int a, int b) { return exactGrid[size_t(a) * bins + b] <= cut; };
                bool left = false, right = false, down = false, up = false;
                for (int b = bmin; b <= bmax; ++b) {
                    left |= amin > 0 && belowWall(amin, b);
                    right |= amax < bins - 1 && belowWall(amax, b);
                }
                for (int a = amin; a <= amax; ++a) {
                    down |= bmin > 0 && belowWall(a, bmin);
                    up |= bmax < bins - 1 && belowWall(a, bmax);
                }
                if (left)
                    amin = std::max(0, amin - grow);
                if (right)
                    amax = std::min(bins - 1, amax + grow);
                if (down)
                    bmin = std::max(0, bmin - grow);
                if (up)
                    bmax = std::min(bins - 1, bmax + grow);
                if (left || right || down || up) {
                    growing = true;
                    emit setMaximumSteps(int((rectangleCells() + cellsPerUnit - 1) / cellsPerUnit)); // more cells to do than planned
                    evaluateRectangle();
                }
            }
            exact = !m_stop;
            if (exact) { // exact profiles: the lowest cell of each row and column, where lower than what the walkers saw
                for (int a = amin; a <= amax; ++a)
                    for (int b = bmin; b <= bmax; ++b) {
                        const double sse = exactGrid[size_t(a) * bins + b];
                        if (sse < m_merged.profile[0][a]) {
                            m_merged.profile[0][a] = sse;
                            m_merged.profileAt[0][a] = m_lower[0] + (a + 0.5) * (m_upper[0] - m_lower[0]) / bins;
                        }
                        if (sse < m_merged.profile[1][b]) {
                            m_merged.profile[1][b] = sse;
                            m_merged.profileAt[1][b] = m_lower[1] + (b + 0.5) * (m_upper[1] - m_lower[1]) / bins;
                        }
                    }
                m_intervals = LandscapeIntervals(m_merged, m_level, m_lower, m_upper);
            }
        }
    }

    QJsonObject out;
    out["confidence"] = confidence;
    out["sse_min"] = sseMin;
    out["level"] = m_level;
    out["wall"] = cut;
    out["mode"] = finder ? QStringLiteral("Minima") : QStringLiteral("Confidence");
    out["datapoints"] = points;
    out["parameters"] = parameters;
    out["time_ms"] = double(QDateTime::currentMSecsSinceEpoch() - t0);
    out["evaluations"] = evaluations + m_carryEvaluations + double(exactEvaluations);
    out["exact_profile"] = exact; // two constants: the profiles and the surface are exact in and around the explored region
    out["recentered"] = m_recentered;
    out["start_sse"] = m_startSSE;
    out["rounds"] = rounds + 1;
    out["walkers"] = walkerInfo;
    out["best"] = ToArray(m_merged.best);
    out["best_sse"] = m_merged.bestSSE;

    QJsonObject set;
    set["sampler"] = sampler;
    set["step_size"] = settings.stepSize;
    set["gradient_step"] = settings.gradientStep;
    set["central_gradient"] = settings.centralGradient;
    set["langevin_correction"] = settings.langevinCorrection;
    set["cut_factor"] = cutFactor;
    set["mode"] = finder ? QStringLiteral("Minima") : QStringLiteral("Confidence");
    set["search_wall"] = searchWall;
    set["min_steps"] = settings.minSteps;
    set["patience"] = settings.patience;
    set["min_hill_fraction"] = settings.minHillFraction;
    set["bias_factor"] = biasFactor;
    set["hill_height_factor"] = hillHeightFactor;
    set["hill_width"] = settings.hillWidth;
    set["temperature"] = settings.temperature;
    set["hill_height"] = settings.hillHeight;
    set["bias_temperature"] = settings.biasTemperature;
    set["max_steps"] = settings.maxSteps;
    set["bins"] = settings.bins;
    set["surface"] = recordPoints;
    out["settings"] = set;

    QJsonArray variables;
    for (int i = 0; i < d; ++i) {
        QJsonObject v;
        v["index"] = base.GlobalIndex(i);
        v["name"] = m_model->GlobalParameterName(base.GlobalIndex(i));
        v["value"] = x0[i];
        v["box_lower"] = m_lower[i];
        v["box_upper"] = m_upper[i];
        QJsonArray segments;
        for (const LandscapeSegment& seg : m_intervals[i]) {
            QJsonObject s;
            s["lower"] = seg.lower;
            s["upper"] = seg.upper;
            s["lower_edge"] = EdgeName(seg.lowerEdge);
            s["upper_edge"] = EdgeName(seg.upperEdge);
            s["minimum"] = seg.minimum;
            s["bounded"] = seg.Bounded();
            segments << s;
        }
        v["segments"] = segments;
        QJsonArray profile;
        for (size_t b = 0; b < m_merged.profile[i].size(); ++b)
            if (std::isfinite(m_merged.profile[i][b]))
                profile << QJsonArray{ m_merged.profileAt[i][b], m_merged.profile[i][b] };
        v["profile"] = profile;
        variables << v;
    }
    // surfaces: the 2D profile of every pair of variables (at most MaxSurfacePairs: d (d - 1) / 2 grow quickly), one point per
    // visited bin; the result page draws the one that is chosen
    if (recordPoints) {
        const int bins = settings.bins;
        std::vector<std::pair<int, int>> pairs;
        for (int j = 1; j < d && int(pairs.size()) < MaxSurfacePairs; ++j)
            for (int i = 0; i < j && int(pairs.size()) < MaxSurfacePairs; ++i)
                pairs.emplace_back(i, j);
        std::vector<std::vector<double>> z(pairs.size(), std::vector<double>(size_t(bins) * size_t(bins), std::numeric_limits<double>::infinity()));
        std::vector<int> bin(d);
        for (const auto& p : m_points) {
            if (int(p.size()) != d + 1)
                continue;
            bool inside = true;
            for (int k = 0; k < d && inside; ++k) {
                bin[k] = int((p[k] - m_lower[k]) / (m_upper[k] - m_lower[k]) * bins);
                inside = bin[k] >= 0 && bin[k] < bins;
            }
            if (!inside)
                continue;
            for (size_t q = 0; q < pairs.size(); ++q) {
                double& cell = z[q][size_t(bin[pairs[q].first]) * size_t(bins) + size_t(bin[pairs[q].second])];
                cell = std::min(cell, p.back());
            }
        }
        if (!exactGrid.empty() && pairs.size() == 1) // two constants: the cells evaluated exactly fill the gaps between the visited ones
            for (size_t c = 0; c < z[0].size(); ++c)
                z[0][c] = std::min(z[0][c], exactGrid[c]);
        QJsonArray surfaces;
        for (size_t q = 0; q < pairs.size(); ++q) {
            const int i = pairs[q].first, j = pairs[q].second;
            QJsonArray points;
            for (int a = 0; a < bins; ++a)
                for (int b = 0; b < bins; ++b) {
                    const double sse = z[q][size_t(a) * bins + b];
                    if (!std::isfinite(sse))
                        continue;
                    QJsonArray point;
                    point << m_lower[i] + (a + 0.5) * (m_upper[i] - m_lower[i]) / bins;
                    point << m_lower[j] + (b + 0.5) * (m_upper[j] - m_lower[j]) / bins;
                    point << sse;
                    points << point;
                }
            QJsonObject surface;
            surface["x"] = i;
            surface["y"] = j;
            surface["bins"] = bins;
            surface["exact"] = exact && !exactGrid.empty(); // unvisited cells in the explored region were evaluated, no gaps
            surface["exact_evaluations"] = double(exactEvaluations);
            surface["points"] = points;
            surfaces << surface;
        }
        out["surfaces"] = surfaces;
        out["contour_max"] = finder ? cut / sseMin : std::max(4.0, 2.0 * (m_level / (sseMin > 0 ? sseMin : 1.0))); // relative to the fit, as the charts draw it
        out["contour_log"] = finder; // the lines of a search between the confidence limit and the wall are spaced geometrically
    }
    out["variables"] = variables;

    // minima: the best cells of the walkers, refitted with the minimiser and merged when they end up in one place
    struct Minimum {
        std::vector<double> x;
        double sse;
        QJsonObject model; ///< the refitted model, to load it into the project
    };
    std::vector<Minimum> polished;
    const int polish = AccessCI(m_controller, "Polish").toInt(finder ? 10 : 5);
    // The same minimum if the parameters agree, or if it is another point of the same flat valley: about the same SSE and no ridge on
    // the straight line between the two (a refit stops anywhere on a valley floor that is flat to its tolerance).
    auto sameBasin = [&](const std::vector<double>& a, double sseA, const std::vector<double>& b, double sseB) {
        bool same = true;
        for (int i = 0; i < d && same; ++i)
            same = std::abs(a[i] - b[i]) <= std::max(0.02, (m_upper[i] - m_lower[i]) / settings.bins);
        if (same && std::abs(sseA - sseB) <= 0.03 * std::max(sseA, sseB))
            return true;
        if (std::abs(sseA - sseB) > 0.03 * std::max(sseA, sseB))
            return false;
        const double ceiling = 1.03 * std::max(sseA, sseB);
        std::vector<double> at(d);
        for (int k = 1; k < 10; ++k) {
            for (int i = 0; i < d; ++i)
                at[i] = a[i] + (b[i] - a[i]) * k / 10.0;
            if (!(base(at) <= ceiling))
                return false;
        }
        return true;
    };
    std::vector<double> tolerance(d);
    for (int i = 0; i < d; ++i)
        tolerance[i] = std::max(0.02, (m_upper[i] - m_lower[i]) / settings.bins);
    // The candidates are sorted by SSE and a flat valley gives many that all end in the same place: one that already lies in the basin of
    // a minimum found is skipped without a refit (a few evaluations on the line between them instead of a fit). The tries are capped
    // at ten per minimum wanted.
    const size_t attempts = std::min(m_merged.minima.size(), size_t(std::max(1, polish) * 10));
    if (polish > 0 && attempts > 0 && !m_stop) {
        emit Stage(tr("Refitting the minima the walkers found"));
        emit setMaximumSteps(int(attempts));
    }
    QElapsedTimer refitClock;
    refitClock.start();
    for (size_t k = 0; k < attempts && int(polished.size()) < polish && !m_stop; ++k) {
        if (std::any_of(polished.begin(), polished.end(), [&](const Minimum& m) { return sameBasin(m.x, m.sse, m_merged.minima[k].x, m_merged.minima[k].sse); })) {
            emit IncrementProgress(1);
            continue;
        }
        QSharedPointer<AbstractModel> candidate = m_model->Clone(false);
        for (int i = 0; i < d; ++i)
            candidate->setGlobalParameter(m_merged.minima[k].x[i], base.GlobalIndex(i));
        Minimizer minimizer(false);
        minimizer.setQuiet(true); // the refits are part of this analysis: one line for all of it, not one per minimum
        minimizer.setModel(candidate);
        emit Stage(tr("Refitting the minima the walkers found: candidate %1 of up to %2, %3 distinct so far").arg(k + 1).arg(attempts).arg(polished.size()), false);
        minimizer.Minimize();
        emit IncrementProgress(int(std::max<qint64>(1, refitClock.restart())));
        QCoreApplication::processEvents();
        ModelLandscape landscape(candidate);
        Minimum found{ landscape.Start(), 0, QJsonObject() };
        found.sse = landscape(found.x);
        candidate->Calculate();
        found.model = candidate->ExportModel(false, false);
        // a refit that ends above the point it started from did not converge: nothing to report
        if (!std::isfinite(found.sse) || found.sse > m_merged.minima[k].sse * (1.0 + 1e-9))
            continue;
        const bool known = std::any_of(polished.begin(), polished.end(), [&](const Minimum& m) { return sameBasin(m.x, m.sse, found.x, found.sse); });
        if (!known)
            polished.push_back(found);
    }
    std::sort(polished.begin(), polished.end(), [](const Minimum& a, const Minimum& b) { return a.sse < b.sse; });

    QJsonArray minima;
    for (const Minimum& m : polished) {
        QJsonObject o;
        o["sse"] = m.sse;
        o["parameters"] = ToArray(m.x);
        o["within_level"] = m.sse <= m_level;
        o["relative_sse"] = m.sse / sseMin; // against the SSE of the fit this search started from
        o["is_fit"] = sameBasin(m.x, m.sse, x0, sseMin); // the fit lies in this minimum's basin
        o["model"] = m.model;
        minima << o;
    }
    out["minima"] = minima;

    m_results << out;
    emit AnalyseFinished();
    return true;
}

QString LandscapeSearch::Summary(const QJsonObject& r)
{
    if (r.isEmpty() || !r.contains("variables"))
        return QString();
    auto number = [](double x) { return QString::number(x, 'g', 5); };
    auto edgeNote = [](const QString& edge) {
        if (edge == QLatin1String("box"))
            return QObject::tr("reaches the border of the sampled range");
        if (edge == QLatin1String("unsampled"))
            return QObject::tr("not reached by the sampling");
        return QString();
    };

    if (r["mode"].toString() == QLatin1String("Minima")) {
        QString text = QObject::tr("<p><b>Minima search</b>: SSE of the fit %1, wall at %2 times that.</p>")
                           .arg(QString::number(r["sse_min"].toDouble(), 'g', 5), QString::number(r["wall"].toDouble() / r["sse_min"].toDouble(), 'g', 3));
        const QJsonArray found = r["minima"].toArray();
        if (found.isEmpty())
            text += QObject::tr("<p>No minimum was refitted.</p>");
        else {
            text += QStringLiteral("<table><tr><th>#</th><th>%1</th><th>%2</th><th>%3</th><th></th></tr>")
                        .arg(QObject::tr("SSE"), QObject::tr("SSE / SSE of the fit"), QObject::tr("Parameters"));
            for (int k = 0; k < found.size(); ++k) {
                const QJsonObject m = found[k].toObject();
                QStringList values;
                for (const QJsonValue& x : m["parameters"].toArray())
                    values << number(x.toDouble());
                QString note;
                if (m["is_fit"].toBool())
                    note = QObject::tr("contains the fit");
                else if (m["relative_sse"].toDouble() < 1.0 - 1e-3)
                    note = QObject::tr("deeper than the fit");
                else if (m["within_level"].toBool())
                    note = QObject::tr("inside the confidence region");
                text += QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                            .arg(k + 1).arg(number(m["sse"].toDouble()), number(m["relative_sse"].toDouble()), values.join(QStringLiteral(", ")), note);
            }
            text += QStringLiteral("</table>");
        }
        text += QObject::tr("<p>%1 walkers, %2 evaluations of the SSE, %3 s.</p>")
                    .arg(r["walkers"].toArray().size())
                    .arg(number(r["evaluations"].toDouble()), number(r["time_ms"].toDouble() / 1000.0));
        return text;
    }

    QString text;
    text += QObject::tr("<p><b>Landscape sampling</b>: %1 % confidence, SSE of the fit %2, limit of the confidence region %3.</p>")
                .arg(number(r["confidence"].toDouble()), number(r["sse_min"].toDouble()), number(r["level"].toDouble()));
    if (r["recentered"].toInt() > 0)
        text += QObject::tr("<p>The walkers found a deeper minimum than the fit (SSE %1 instead of %2); the intervals belong to it.</p>")
                    .arg(number(r["sse_min"].toDouble()), number(r["start_sse"].toDouble()));

    text += QStringLiteral("<table><tr><th>%1</th><th>%2</th><th>%3</th><th>%4</th><th></th></tr>")
                .arg(QObject::tr("Parameter"), QObject::tr("Fit"), QObject::tr("Lower"), QObject::tr("Upper"));
    for (const QJsonValue& value : r["variables"].toArray()) {
        const QJsonObject v = value.toObject();
        const QJsonArray segments = v["segments"].toArray();
        if (segments.isEmpty()) {
            text += QStringLiteral("<tr><td>%1</td><td>%2</td><td></td><td></td><td>%3</td></tr>")
                        .arg(v["name"].toString(), number(v["value"].toDouble()), QObject::tr("no interval below the limit"));
            continue;
        }
        for (const QJsonValue& s : segments) {
            const QJsonObject seg = s.toObject();
            const QStringList notes = { edgeNote(seg["lower_edge"].toString()), edgeNote(seg["upper_edge"].toString()) };
            QString note;
            if (!notes[0].isEmpty())
                note += QObject::tr("lower: %1").arg(notes[0]);
            if (!notes[1].isEmpty())
                note += (note.isEmpty() ? QString() : QStringLiteral("; ")) + QObject::tr("upper: %1").arg(notes[1]);
            text += QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                        .arg(v["name"].toString(), number(v["value"].toDouble()), number(seg["lower"].toDouble()), number(seg["upper"].toDouble()), note);
        }
    }
    text += QStringLiteral("</table>");

    const QJsonArray minima = r["minima"].toArray();
    if (minima.size() > 1) {
        text += QObject::tr("<p>Minima found (refitted):</p><ul>");
        for (const QJsonValue& m : minima) {
            QStringList values;
            for (const QJsonValue& x : m.toObject()["parameters"].toArray())
                values << number(x.toDouble());
            text += QStringLiteral("<li>SSE %1 at (%2)%3</li>")
                        .arg(number(m.toObject()["sse"].toDouble()), values.join(QStringLiteral(", ")),
                            m.toObject()["within_level"].toBool() ? QObject::tr(" - inside the confidence region") : QString());
        }
        text += QStringLiteral("</ul>");
    }
    text += QObject::tr("<p>%1 walkers, %2 evaluations of the SSE, %3 s.</p>")
                .arg(r["walkers"].toArray().size())
                .arg(number(r["evaluations"].toDouble()), number(r["time_ms"].toDouble() / 1000.0));
    return text;
}

