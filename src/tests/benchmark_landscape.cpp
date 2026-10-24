/*
 * SupraFit - benchmark: landscape sampling vs profile likelihood vs Monte Carlo vs multistart
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
 */

/*
 * Manual measurement tool (not a ctest), WP-E of roadmap/landscape_sampling.md. Claude Generated.
 *
 * Part 1 - confidence intervals: for several synthetic titrations (1-3 constants, several noise realisations) the
 * intervals of LandscapeSearch are compared with the profile likelihood by constrained refit (bracketing and
 * bisection on one constant, Nelder-Mead over the others, warm started - the cheap alternative) in SSE evaluations,
 * wall time and the position of the interval ends; Monte Carlo (percentile interval, a different statistic) is
 * timed on the same fits.
 * Part 2 - global search: fits whose blind start ends in a wrong basin are given to LandscapeSearch and to
 * random multistart with the ordinary minimiser: found the better minimum, evaluations/time.
 *
 * Numbers are quoted as the worst case over all data sets and seeds (CLAUDE.md), not the best.
 * Build target: benchmark_landscape.
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <random>

#include <Eigen/Dense>

#include "src/capabilities/jobmanager.h"
#include "src/capabilities/landscapesearch.h"
#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/toolset.h"
#include "src/global.h"

namespace {
using V = std::vector<double>;

QJsonObject strOpt(const QString& v)
{
    QJsonObject o;
    o["value"] = v;
    return o;
}

struct Config {
    QString name;
    QString reactions;
    QList<double> betas;
    int series, points;
    double ratio, noise;
    V shifts;
};

DataClass* makeData(int series, double ratio, int points)
{
    Eigen::MatrixXd indep(points, 2), dep = Eigen::MatrixXd::Zero(points, series);
    for (int i = 0; i < points; ++i) {
        indep(i, 0) = 1e-3;
        indep(i, 1) = ratio * 1e-3 * i / (points - 1);
    }
    DataClass* data = new DataClass();
    data->setIndependentTable(new DataTable(indep));
    data->setDataType(DataClassPrivate::Table);
    data->setSimulateDependent(series);
    data->setDependentTable(new DataTable(dep));
    data->setDataBegin(0);
    data->setDataEnd(points);
    return data;
}

QSharedPointer<AbstractModel> newModel(DataClass* data, const QString& reactions)
{
    QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
    QJsonObject def;
    def["Reactions"] = strOpt(reactions);
    model->DefineModel(def);
    QJsonObject cfg = model->getOptimizerConfig();
    cfg["FitSolver"] = QStringLiteral("VarPro");
    model->setOptimizerConfig(cfg);
    return model;
}

/*! noisy synthetic data of a configuration (noise realisation `seed`), a fitted model from the blind start */
QSharedPointer<AbstractModel> synthesise(const Config& c, uint64_t seed, DataClass*& data, bool globalMinimum)
{
    data = makeData(c.series, c.ratio, c.points);
    {
        QSharedPointer<AbstractModel> truth = newModel(data, c.reactions);
        truth->InitialGuess();
        for (int k = 0; k < c.betas.size(); ++k)
            truth->setGlobalParameter(c.betas[k], k);
        for (int s = 0; s < c.series; ++s)
            for (int p = 0; p < truth->LocalParameterSize(); ++p)
                truth->setLocalParameter(c.shifts[std::min<size_t>(p, c.shifts.size() - 1)] * (1.0 - 0.07 * s), p, s);
        truth->Calculate();
        Eigen::MatrixXd signal = truth->ModelTable()->Table();
        std::mt19937_64 rng(seed);
        std::normal_distribution<double> g(0.0, c.noise);
        for (int r = 0; r < signal.rows(); ++r)
            for (int col = 0; col < signal.cols(); ++col)
                signal(r, col) += g(rng);
        data->setDependentTable(new DataTable(signal));
    }
    QSharedPointer<AbstractModel> model = newModel(data, c.reactions);
    model->InitialGuess();
    Minimizer m(false);
    m.setModel(model);
    m.Minimize();
    model->Calculate();
    if (!globalMinimum)
        return model;

    // part 1 compares interval methods, not fits: start both from the better of the blind fit and a fit from the truth
    QSharedPointer<AbstractModel> fromTruth = newModel(data, c.reactions);
    fromTruth->InitialGuess();
    for (int k = 0; k < c.betas.size(); ++k)
        fromTruth->setGlobalParameter(c.betas[k], k);
    Minimizer mt(false);
    mt.setModel(fromTruth);
    mt.Minimize();
    fromTruth->Calculate();
    ModelLandscape a(model->Clone(false)), b(fromTruth->Clone(false));
    return b(b.Start()) < a(a.Start()) ? fromTruth : model;
}

/*! Nelder-Mead; returns the minimum, `calls` counts evaluations of f */
double nelderMead(const std::function<double(const V&)>& f, V& x, double step, int maxIter, double ftol)
{
    const size_t n = x.size();
    std::vector<V> s(n + 1, x);
    std::vector<double> fv(n + 1);
    for (size_t i = 0; i < n; ++i)
        s[i + 1][i] += step;
    for (size_t i = 0; i <= n; ++i)
        fv[i] = f(s[i]);
    for (int it = 0; it < maxIter; ++it) {
        std::vector<size_t> o(n + 1);
        for (size_t i = 0; i <= n; ++i)
            o[i] = i;
        std::sort(o.begin(), o.end(), [&](size_t a, size_t b) { return fv[a] < fv[b]; });
        std::vector<V> s2;
        std::vector<double> f2;
        for (size_t i : o) {
            s2.push_back(s[i]);
            f2.push_back(fv[i]);
        }
        s = s2;
        fv = f2;
        if (fv[n] - fv[0] < ftol * (std::abs(fv[0]) + 1e-300))
            break;
        V c(n, 0.0);
        for (size_t i = 0; i < n; ++i)
            for (size_t j = 0; j < n; ++j)
                c[j] += s[i][j] / double(n);
        auto along = [&](double t) {
            V p(n);
            for (size_t j = 0; j < n; ++j)
                p[j] = c[j] + t * (s[n][j] - c[j]);
            return p;
        };
        V r = along(-1.0);
        const double fr = f(r);
        if (fr < fv[0]) {
            V e = along(-2.0);
            const double fe = f(e);
            if (fe < fr) {
                s[n] = e;
                fv[n] = fe;
            } else {
                s[n] = r;
                fv[n] = fr;
            }
        } else if (fr < fv[n - 1]) {
            s[n] = r;
            fv[n] = fr;
        } else {
            V k = along(fr < fv[n] ? -0.5 : 0.5);
            const double fk = f(k);
            if (fk < std::min(fr, fv[n])) {
                s[n] = k;
                fv[n] = fk;
            } else {
                for (size_t i = 1; i <= n; ++i) {
                    for (size_t j = 0; j < n; ++j)
                        s[i][j] = s[0][j] + 0.5 * (s[i][j] - s[0][j]);
                    fv[i] = f(s[i]);
                }
            }
        }
    }
    size_t best = 0;
    for (size_t i = 1; i <= n; ++i)
        if (fv[i] < fv[best])
            best = i;
    x = s[best];
    return fv[best];
}

struct Edge {
    double value = 0;
    bool open = false; ///< level not reached within the limit
};

/*! Profile likelihood by constrained refit: the cheap alternative. */
struct ProfileReference {
    ModelLandscape landscape;
    long calls = 0;
    V x0;
    double level;
    double limit = 10.0; ///< log10 units

    ProfileReference(QSharedPointer<AbstractModel> m, double lvl)
        : landscape(m)
        , x0(landscape.Start())
        , level(lvl)
    {
    }

    double f(const V& x)
    {
        ++calls;
        return landscape(x);
    }

    /*! min over the other variables at x_i = value, warm started from `others` (updated) */
    double profile(int i, double value, V& others)
    {
        const int d = int(x0.size());
        if (d == 1)
            return f({ value });
        auto full = [&](const V& o) {
            V x(d);
            for (int j = 0, k = 0; j < d; ++j)
                x[j] = j == i ? value : o[k++];
            return f(x);
        };
        double best = nelderMead(full, others, 0.1, 400, 1e-10);
        best = nelderMead(full, others, 0.02, 400, 1e-12); // restart: NM stalls in thin valleys
        return best;
    }

    Edge edge(int i, double sign)
    {
        V others;
        for (int j = 0; j < int(x0.size()); ++j)
            if (j != i)
                others.push_back(x0[j]);
        double inside = 0, outside = 0.05;
        while (outside < limit) {
            V warm = others;
            if (profile(i, x0[i] + sign * outside, warm) > level)
                break;
            others = warm;
            inside = outside;
            outside *= 2;
        }
        if (outside >= limit)
            return { x0[i] + sign * limit, true };
        for (int it = 0; it < 22; ++it) {
            const double mid = 0.5 * (inside + outside);
            V warm = others;
            if (profile(i, x0[i] + sign * mid, warm) <= level) {
                inside = mid;
                others = warm;
            } else
                outside = mid;
        }
        return { x0[i] + sign * 0.5 * (inside + outside), false };
    }
};

double percentile(std::vector<double> v, double p)
{
    std::sort(v.begin(), v.end());
    return v.empty() ? 0.0 : v[std::min(v.size() - 1, size_t(p * v.size()))];
}

struct Row {
    QString name;
    uint64_t seed;
    int dims;
    long evalSampler = 0, evalProfile = 0;
    double msSampler = 0, msProfile = 0, msMC = 0;
    double worstEdge = 0; ///< |sampler - profile| / width of the profile interval, crossings only
    int kindMismatch = 0; ///< an end open/box in one method and a crossing in the other
    int unsampled = 0;
};
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    qApp->setProperty("threads", 4);
    bool quick = false, verbose = false, skipBasins = false;
    QString only;
    for (int a = 1; a < argc; ++a) {
        const QString arg = argv[a];
        if (arg == "--quick")
            quick = true;
        else if (arg == "--verbose")
            verbose = true;
        else if (arg == "--no-basins")
            skipBasins = true;
        else if (arg.startsWith("--only="))
            only = arg.mid(7);
    }

    const std::vector<Config> configs = {
        { "1 const, 3 series, noise 0.05", "A + B <=> AB", { 3.0 }, 3, 16, 3.0, 0.05, { 9.0, 8.1 } },
        { "1 const, 3 series, noise 0.5", "A + B <=> AB", { 3.0 }, 3, 16, 3.0, 0.5, { 9.0, 8.1 } },
        { "2 const 1:1/1:2, 4 series, noise 0.02", "A + B <=> AB\nA + 2 B <=> AB2", { 3.0, 5.4 }, 4, 30, 4.0, 0.02, { 9.0, 5.0, 2.0 } },
        { "3 const 1:1/1:2/2:1, 4 series, noise 0.02", "A + B <=> AB\nA + 2 B <=> AB2\n2 A + B <=> A2B", { 3.0, 5.4, 5.6 }, 4, 30, 4.0, 0.02, { 9.0, 5.0, 2.0, 7.0 } },
    };
    const int seeds = quick ? 2 : 5;

    std::printf("\n# Part 1 - confidence intervals (95 %%), %d noise realisations per configuration\n\n", seeds);
    std::printf("| configuration | seed | sampler evals | profile evals | sampler/profile evals | sampler ms (4 thr) | profile ms (1 thr) | MC 500 steps ms (4 thr) | worst end deviation (of interval width) | kind mismatches | unsampled ends |\n");
    std::printf("|---|---|---|---|---|---|---|---|---|---|---|\n");
    std::vector<Row> rows;
    for (const Config& c : configs) {
        if (quick && c.betas.size() > 2)
            continue;
        if (!only.isEmpty() && !c.name.contains(only))
            continue;
        for (uint64_t seed = 1; seed <= uint64_t(seeds); ++seed) {
            DataClass* data = nullptr;
            QSharedPointer<AbstractModel> model = synthesise(c, seed, data, true);
            Row row;
            row.name = c.name;
            row.seed = seed;

            // sampler
            QElapsedTimer timer;
            LandscapeSearch search;
            search.setModel(model);
            QJsonObject ctl;
            ctl["Polish"] = 0;
            search.setController(ctl);
            timer.start();
            const bool ok = search.Run();
            row.msSampler = timer.elapsed();
            if (!ok) {
                std::printf("| %s | %llu | run failed |\n", qPrintable(c.name), (unsigned long long)seed);
                delete data;
                continue;
            }
            row.dims = int(search.Intervals().size());
            row.evalSampler = long(search.Result()["0"].toObject()["evaluations"].toDouble());

            // profile likelihood by constrained refit, at the same level
            ProfileReference ref(model->Clone(false), search.Level());
            timer.restart();
            std::vector<Edge> lo(row.dims), hi(row.dims);
            for (int i = 0; i < row.dims; ++i) {
                lo[i] = ref.edge(i, -1.0);
                hi[i] = ref.edge(i, +1.0);
            }
            row.msProfile = timer.elapsed();
            row.evalProfile = ref.calls;

            for (int i = 0; i < row.dims; ++i) {
                const auto& segs = search.Intervals()[i];
                const LandscapeSegment* seg = nullptr;
                for (const auto& s : segs)
                    if (s.lower <= ref.x0[i] && ref.x0[i] <= s.upper)
                        seg = &s;
                if (!seg) {
                    ++row.kindMismatch;
                    continue;
                }
                const double width = std::max(1e-9, std::min(hi[i].value, ref.x0[i] + 10) - std::max(lo[i].value, ref.x0[i] - 10));
                auto compare = [&](double sampler, LandscapeSegment::Edge kind, const Edge& r) {
                    if (kind == LandscapeSegment::Edge::Unsampled)
                        ++row.unsampled;
                    const bool samplerCrossing = kind == LandscapeSegment::Edge::Crossing;
                    if (samplerCrossing != !r.open)
                        ++row.kindMismatch;
                    else if (samplerCrossing)
                        row.worstEdge = std::max(row.worstEdge, std::abs(sampler - r.value) / width);
                };
                compare(seg->lower, seg->lowerEdge, lo[i]);
                compare(seg->upper, seg->upperEdge, hi[i]);
                if (verbose)
                    std::printf("  var %d x0 %.3f | profile [%.3f%s, %.3f%s] | sampler [%.3f kind %d, %.3f kind %d] box [%.2f, %.2f] segments %zu\n", i, ref.x0[i],
                        lo[i].value, lo[i].open ? " open" : "", hi[i].value, hi[i].open ? " open" : "", seg->lower, int(seg->lowerEdge), seg->upper, int(seg->upperEdge),
                        search.Lower()[i], search.Upper()[i], segs.size());
            }

            // Monte Carlo on the same fit
            {
                JobManager manager;
                manager.setModel(model->Clone(false));
                QJsonObject job = MonteCarloConfigBlock;
                job["MaxSteps"] = 500;
                manager.AddSingleJob(job);
                timer.restart();
                manager.RunJobs();
                row.msMC = timer.elapsed();
            }

            std::printf("| %s | %llu | %ld | %ld | %.1f | %.0f | %.0f | %.0f | %.3f | %d | %d |\n", qPrintable(c.name), (unsigned long long)seed,
                row.evalSampler, row.evalProfile, double(row.evalSampler) / double(std::max(1L, row.evalProfile)), row.msSampler, row.msProfile, row.msMC,
                row.worstEdge, row.kindMismatch, row.unsampled);
            std::fflush(stdout);
            rows.push_back(row);
            delete data;
        }
    }

    std::printf("\nWorst case per configuration over the seeds:\n\n| configuration | max sampler/profile evals | max sampler ms | max profile ms | max end deviation | kind mismatches (sum) | unsampled ends (sum) |\n|---|---|---|---|---|---|---|\n");
    for (const Config& c : configs) {
        double ratio = 0, ms = 0, mp = 0, dev = 0;
        int mism = 0, uns = 0;
        bool any = false;
        for (const Row& r : rows)
            if (r.name == c.name) {
                any = true;
                ratio = std::max(ratio, double(r.evalSampler) / double(std::max(1L, r.evalProfile)));
                ms = std::max(ms, r.msSampler);
                mp = std::max(mp, r.msProfile);
                dev = std::max(dev, r.worstEdge);
                mism += r.kindMismatch;
                uns += r.unsampled;
            }
        if (any)
            std::printf("| %s | %.1f | %.0f | %.0f | %.3f | %d | %d |\n", qPrintable(c.name), ratio, ms, mp, dev, mism, uns);
    }

    // ---- Part 2: global search -------------------------------------------------------------------------
    if (skipBasins)
        return 0;
    std::printf("\n# Part 2 - global search from fits that ended in a wrong basin\n\n");
    std::printf("| data set | blind fit SSE | reference SSE | landscape found it | landscape evals | landscape ms | multistart success (of 20) | multistart ms per start |\n|---|---|---|---|---|---|---|---|\n");
    struct Basin {
        QString name;
        QString reactions;
        QList<double> betas;
        int series, points;
        double ratio;
        V shifts;
    };
    const std::vector<Basin> basins = {
        { "nmr 1:1/1:2 (3.0, 5.4)", "A + B <=> AB\nA + 2 B <=> AB2", { 3.0, 5.4 }, 4, 30, 4.0, { 9.0, 5.0, 2.0 } },
        { "nmr 1:1/2:1 (3.0, 4.5)", "A + B <=> AB\n2 A + B <=> A2B", { 3.0, 4.5 }, 3, 20, 3.0, { 9.0, 6.0, 3.0 } },
        { "nmr 1:1/1:2 (2.4, 4.2)", "A + B <=> AB\nA + 2 B <=> AB2", { 2.4, 4.2 }, 3, 20, 3.0, { 9.0, 6.0, 3.0 } },
        { "nmr 2:1/1:1 (4.2, 6.6)", "A + B <=> AB\n2 A + B <=> A2B", { 4.2, 6.6 }, 3, 20, 3.0, { 9.0, 6.0, 3.0 } },
    };
    int tried = 0, wrong = 0, found = 0;
    for (const Basin& b : basins) {
        for (uint64_t seed = 1; seed <= uint64_t(quick ? 3 : 8); ++seed) {
            ++tried;
            Config c{ b.name, b.reactions, b.betas, b.series, b.points, b.ratio, 0.02, b.shifts };
            DataClass* data = nullptr;
            QSharedPointer<AbstractModel> model = synthesise(c, seed, data, false);

            // reference: the fit started at the true constants
            QSharedPointer<AbstractModel> refModel = newModel(data, b.reactions);
            refModel->InitialGuess();
            for (int k = 0; k < b.betas.size(); ++k)
                refModel->setGlobalParameter(b.betas[k], k);
            Minimizer rm(false);
            rm.setModel(refModel);
            rm.Minimize();
            ModelLandscape refL(refModel->Clone(false)), blindL(model->Clone(false));
            const double refSSE = refL(refL.Start()), blindSSE = blindL(blindL.Start());
            if (blindSSE <= 1.02 * refSSE) {
                delete data;
                continue; // the blind fit found the right basin: nothing to search for
            }
            ++wrong;

            LandscapeSearch search;
            search.setModel(model);
            QJsonObject ctl;
            ctl["Range"] = 6.0;
            ctl["Polish"] = 5;
            search.setController(ctl);
            QElapsedTimer timer;
            timer.start();
            const bool ok = search.Run();
            const double ms = timer.elapsed();
            double bestPolished = INFINITY;
            long evals = 0;
            int recentered = 0;
            if (ok) {
                const QJsonObject r = search.Result()["0"].toObject();
                evals = long(r["evaluations"].toDouble());
                bestPolished = r["sse_min"].toDouble(); // the minimum the walk ended up centred on
                for (const auto m : r["minima"].toArray())
                    bestPolished = std::min(bestPolished, m.toObject()["sse"].toDouble());
                recentered = r["recentered"].toInt();
            }
            const bool hit = bestPolished <= 1.02 * refSSE;
            found += hit;

            // random multistart with the ordinary minimiser
            std::mt19937_64 rng(seed * 7919);
            std::uniform_real_distribution<double> u(0.0, 8.0);
            int success = 0;
            timer.restart();
            const int starts = 20;
            for (int s = 0; s < starts; ++s) {
                QSharedPointer<AbstractModel> m = newModel(data, b.reactions);
                m->InitialGuess();
                for (int k = 0; k < b.betas.size(); ++k)
                    m->setGlobalParameter(u(rng), k);
                Minimizer mm(false);
                mm.setModel(m);
                mm.Minimize();
                ModelLandscape L(m->Clone(false));
                if (L(L.Start()) <= 1.02 * refSSE)
                    ++success;
            }
            const double msStart = double(timer.elapsed()) / starts;
            std::printf("| %s seed %llu | %.4g | %.4g | %s (recentred %d) | %ld | %.0f | %d | %.0f |\n", qPrintable(b.name), (unsigned long long)seed, blindSSE, refSSE, hit ? "yes" : "NO",
                recentered, evals, ms, success, msStart);
            std::fflush(stdout);
            delete data;
        }
    }
    std::printf("\n%d data sets tried, %d with a wrong blind basin, landscape search found the better minimum in %d of them.\n", tried, wrong, found);
    return 0;
}
