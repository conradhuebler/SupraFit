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

#include "src/core/models/AbstractModel.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>

#include <algorithm>
#include <cmath>
#include <random>

namespace {
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

    void setStart(const std::vector<double>& x) { m_start = x; }
    void setRandomStart(double cut) { m_randomStartCut = cut; }

    void run() override
    {
        ModelLandscape landscape(m_model);
        LandscapeSampler sampler([&landscape](const std::vector<double>& x) { return landscape(x); }, m_lower, m_upper);
        sampler.setSettings(m_settings);
        sampler.setInterrupt([this] { return m_stop->load(); });

        std::vector<double> start = m_start;
        if (m_randomStartCut > 0) { // somewhere below the wall, else the fitted point
            std::mt19937_64 rng(m_settings.seed);
            std::uniform_real_distribution<double> u(0.0, 1.0);
            for (int attempt = 0; attempt < 200; ++attempt) {
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
    m_merged = LandscapeResult();
    m_intervals.clear();
    m_results.clear();

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
    const int points = m_model->Points(), parameters = m_model->Parameter();
    if (!(sseMin > 1e-300) || points <= parameters) {
        emit Message(tr("The landscape search needs a fit with residuals (SSE %1, %2 points, %3 parameters).").arg(sseMin).arg(points).arg(parameters));
        return false;
    }

    const double confidence = m_controller.value("Confidence").toDouble(95.0);
    m_level = LandscapeConfidenceLevel(sseMin, points, parameters, m_model->finv(confidence / 100.0));
    const double cutFactor = m_controller.value("CutFactor").toDouble(3.0);
    const double cut = sseMin + cutFactor * (m_level - sseMin);

    // box: fixed half width, or a multiple of the extent of the level along each variable alone
    const double range = m_controller.value("Range").toDouble(0.0);
    const double boxFactor = m_controller.value("BoxFactor").toDouble(4.0);
    std::vector<double> half(d, range);
    for (int i = 0; i < d; ++i)
        if (!(range > 0)) {
            const double ext = std::max(Extent(base, x0, i, +1.0, m_level, ExtentLimit), Extent(base, x0, i, -1.0, m_level, ExtentLimit));
            half[i] = boxFactor * std::max(ext, 1e-3);
        }
    m_lower.assign(d, 0.0);
    m_upper.assign(d, 0.0);

    LandscapeSettings settings;
    const QString sampler = m_controller.value("Sampler").toString("Metropolis");
    settings.sampler = sampler == QLatin1String("Langevin") ? LandscapeSettings::Sampler::Langevin : LandscapeSettings::Sampler::Metropolis;
    settings.temperature = m_controller.value("TemperatureFactor").toDouble(1.0) * LandscapeSampler::NaturalTemperature(sseMin, points, parameters);
    settings.stepSize = settings.sampler == LandscapeSettings::Sampler::Langevin ? 0.04 : 0.05;
    settings.hillHeight = 0.25 * settings.temperature;
    settings.biasTemperature = 0.5 * (cut - sseMin); // takes down barriers up to about 2.3 times this
    settings.sseCut = cut;
    settings.maxSteps = m_controller.value("MaxSteps").toInt(50000);
    settings.bins = m_controller.value("Bins").toInt(60);
    settings.seed = uint64_t(m_controller.value("Seed").toInt(1));

    int walkers = m_controller.value("Walkers").toInt(qApp->instance()->property("threads").toInt());
    walkers = std::max(1, walkers);
    emit setMaximumSteps(walkers);

    // The extent along one variable alone understates the profile extent of correlated constants: if a
    // stretch reaches the box, that variable's box is doubled and the walk repeated (fixed `Range`: never).
    const int maxExpansions = m_controller.value("MaxExpansions").toInt(4);
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
        pool.waitForDone();

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
                    if (seg.lowerEdge == LandscapeSegment::Edge::Box || seg.upperEdge == LandscapeSegment::Edge::Box) {
                        half[i] *= 2;
                        expand = true;
                        break;
                    }
        if (!expand)
            break;
    }

    QJsonObject out;
    out["confidence"] = confidence;
    out["sse_min"] = sseMin;
    out["level"] = m_level;
    out["wall"] = cut;
    out["datapoints"] = points;
    out["parameters"] = parameters;
    out["time_ms"] = double(QDateTime::currentMSecsSinceEpoch() - t0);
    out["evaluations"] = evaluations;
    out["rounds"] = rounds + 1;
    out["walkers"] = walkerInfo;
    out["best"] = ToArray(m_merged.best);
    out["best_sse"] = m_merged.bestSSE;

    QJsonObject set;
    set["sampler"] = sampler;
    set["temperature"] = settings.temperature;
    set["hill_height"] = settings.hillHeight;
    set["bias_temperature"] = settings.biasTemperature;
    set["max_steps"] = settings.maxSteps;
    set["bins"] = settings.bins;
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
    out["variables"] = variables;

    QJsonArray minima;
    for (size_t k = 0; k < m_merged.minima.size() && k < 10; ++k) {
        QJsonObject m;
        m["sse"] = m_merged.minima[k].sse;
        m["parameters"] = ToArray(m_merged.minima[k].x);
        minima << m;
    }
    out["minima"] = minima;

    m_results << out;
    emit AnalyseFinished();
    return true;
}
