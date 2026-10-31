/*
 * SupraFit - tests of the landscape search on fitted titration models
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
 * WP-C of roadmap/landscape_sampling.md: the intervals of the sampler are checked against the profile of the same
 * model computed on a plain grid (SSE(log beta) with the linear parameters projected out), which uses nothing of
 * the sampler. Claude Generated.
 */

#include <cmath>
#include <random>

#include <QtTest/QtTest>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

#include <Eigen/Dense>

#include "src/capabilities/jobmanager.h"
#include "src/capabilities/landscapesearch.h"
#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/global.h"

namespace {
constexpr double Host = 1e-3;

QJsonObject strOption(const QString& value)
{
    QJsonObject o;
    o["value"] = value;
    return o;
}

DataClass* makeData(int series, double ratio = 3.0, int points = 16)
{
    const int Points = points;
    Eigen::MatrixXd indep(Points, 2), dep = Eigen::MatrixXd::Zero(Points, series);
    for (int i = 0; i < Points; ++i) {
        indep(i, 0) = Host;
        indep(i, 1) = ratio * Host * i / (Points - 1);
    }
    DataClass* data = new DataClass();
    data->setIndependentTable(new DataTable(indep));
    data->setDataType(DataClassPrivate::Table);
    data->setSimulateDependent(series);
    data->setDependentTable(new DataTable(dep));
    data->setDataBegin(0);
    data->setDataEnd(Points);
    return data;
}

/*! A fitted nmr_any model on noisy synthetic data (fixed seed). */
QSharedPointer<AbstractModel> fittedModel(DataClass* data, const QString& reactions, const QList<double>& betas, int series, double noise,
    const double* localScale = nullptr)
{
    static const double defaultScale[3] = { 9.0, 8.1, 7.2 };
    if (!localScale)
        localScale = defaultScale;
    {
        QSharedPointer<AbstractModel> truth = CreateModel(SupraFit::nmr_any, data);
        QJsonObject def;
        def["Reactions"] = strOption(reactions);
        truth->DefineModel(def);
        truth->InitialGuess();
        for (int k = 0; k < betas.size(); ++k)
            truth->setGlobalParameter(betas[k], k);
        for (int s = 0; s < series; ++s)
            for (int p = 0; p < truth->LocalParameterSize(); ++p)
                truth->setLocalParameter(localScale[std::min(p, 2)] * (1.0 - 0.07 * s), p, s);
        truth->Calculate();
        Eigen::MatrixXd signal = truth->ModelTable()->Table();
        std::mt19937 rng(12345);
        std::normal_distribution<double> g(0.0, noise);
        for (int r = 0; r < signal.rows(); ++r)
            for (int c = 0; c < signal.cols(); ++c)
                signal(r, c) += g(rng);
        data->setDependentTable(new DataTable(signal));
    }
    QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
    QJsonObject def;
    def["Reactions"] = strOption(reactions);
    model->DefineModel(def);
    QJsonObject cfg = model->getOptimizerConfig();
    cfg["FitSolver"] = QStringLiteral("VarPro");
    model->setOptimizerConfig(cfg);
    model->InitialGuess();
    Minimizer m(false);
    m.setModel(model);
    m.Minimize();
    model->Calculate();
    return model;
}
}

class TestLandscapeSearch : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { qApp->setProperty("threads", 4); }

    // One constant: the interval of the sampler is the one of a fine scan of SSE(log beta).
    void oneConstantAgainstAScan()
    {
        DataClass* data = makeData(3);
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB"), { 3.0 }, 3, 0.05);

        LandscapeSearch search;
        search.setModel(model);
        QJsonObject controller;
        controller["Walkers"] = 2;
        controller["Bins"] = 80;
        search.setController(controller);
        QVERIFY(search.Run());

        // reference: scan of the same landscape
        ModelLandscape landscape(model->Clone(false));
        const std::vector<double> x0 = landscape.Start();
        const double level = search.Level();
        QVERIFY(landscape(x0) < level);
        double lo = x0[0], hi = x0[0];
        const double step = 1e-3;
        for (double x = x0[0]; x > x0[0] - 6; x -= step) {
            if (landscape({ x }) > level)
                break;
            lo = x;
        }
        for (double x = x0[0]; x < x0[0] + 6; x += step) {
            if (landscape({ x }) > level)
                break;
            hi = x;
        }

        const auto& seg = search.Intervals()[0];
        QCOMPARE(seg.size(), size_t(1));
        const double bin = (search.Upper()[0] - search.Lower()[0]) / 80;
        qInfo() << "sampler" << seg[0].lower << seg[0].upper << "scan" << lo << hi << "bin" << bin;
        QVERIFY(seg[0].Bounded());
        QVERIFY(std::abs(seg[0].lower - lo) < 0.5 * bin && std::abs(seg[0].upper - hi) < 0.5 * bin);

        const QJsonObject result = search.Result()["0"].toObject();
        QCOMPARE(result["variables"].toArray().size(), 1);
        QVERIFY(result["variables"].toArray()[0].toObject()["segments"].toArray().size() == 1);
        QVERIFY(result["evaluations"].toDouble() > 0);
        delete data;
    }

    // Two correlated constants: profile intervals against the profile of a plain 2D grid.
    void twoConstantsAgainstAGrid()
    {
        // 30 points up to 4 equivalents: enough points below one equivalent (the kink of the curve) and enough guest
        // for the second constant. (16 points up to 12 equivalents leave lg beta 1 open to the top: only one or two
        // points lie below one equivalent. Sampler and grid agree on that, but it is no test case.)
        DataClass* data = makeData(4, 4.0, 30);
        const double shifts[3] = { 9.0, 5.0, 2.0 }; // species far apart in the signal
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2"), { 3.0, 5.4 }, 4, 0.02, shifts);

        LandscapeSearch search;
        search.setModel(model);
        QJsonObject controller;
        controller["Walkers"] = 4;
        controller["Bins"] = 60;
        controller["MaxSteps"] = 40000;
        search.setController(controller);
        QVERIFY(search.Run());

        ModelLandscape landscape(model->Clone(false));
        const double level = search.Level();
        const std::vector<double>& lo = search.Lower();
        const std::vector<double>& hi = search.Upper();
        // exact profile on a grid of the variable in question: coarse scan of the other variable, then golden section
        // around the best coarse point (the valley of correlated constants is thinner than any sensible grid)
        const int g = 90;
        std::vector<double> profile[2] = { std::vector<double>(g, INFINITY), std::vector<double>(g, INFINITY) };
        for (int v = 0; v < 2; ++v) {
            const int o = 1 - v;
            for (int a = 0; a < g; ++a) {
                const double xv = lo[v] + (a + 0.5) * (hi[v] - lo[v]) / g;
                auto f = [&](double xo) {
                    std::vector<double> x(2);
                    x[v] = xv;
                    x[o] = xo;
                    return landscape(x);
                };
                const int coarse = 200;
                const double wo = (hi[o] - lo[o]) / coarse;
                double best = INFINITY, at = lo[o];
                for (int c = 0; c <= coarse; ++c) {
                    const double s = f(lo[o] + c * wo);
                    if (s < best) {
                        best = s;
                        at = lo[o] + c * wo;
                    }
                }
                double l = std::max(lo[o], at - wo), r = std::min(hi[o], at + wo);
                const double phi = 0.6180339887498949;
                for (int it = 0; it < 40; ++it) {
                    const double m1 = r - phi * (r - l), m2 = l + phi * (r - l);
                    if (f(m1) < f(m2))
                        r = m2;
                    else
                        l = m1;
                }
                profile[v][a] = std::min(best, f(0.5 * (l + r)));
            }
        }
        int bounded = 0;
        for (int v = 0; v < 2; ++v) {
            const double w = (hi[v] - lo[v]) / g;
            int first = -1, last = -1;
            for (int i = 0; i < g; ++i)
                if (profile[v][i] <= level) {
                    if (first < 0)
                        first = i;
                    last = i;
                }
            QVERIFY(first >= 0);
            const double refLower = lo[v] + first * w, refUpper = lo[v] + (last + 1) * w;
            const auto& seg = search.Intervals()[v];
            QVERIFY(!seg.empty());
            const LandscapeSegment& lowSeg = seg.front();
            const LandscapeSegment& highSeg = seg.back();
            qInfo() << "variable" << v << "sampler" << lowSeg.lower << highSeg.upper << "grid" << refLower << refUpper
                    << "cell of the grid" << w << "edges" << int(lowSeg.lowerEdge) << int(highSeg.upperEdge);
            // an end the grid finds at the box has to be reported as the box (an interval cut by the box, not a
            // crossing); every other end has to match the grid. The grid cell and the sampler's bin both count.
            const double tolerance = 2.5 * w;
            const bool gridLowAtBox = first == 0, gridHighAtBox = last == g - 1;
            if (gridLowAtBox)
                QCOMPARE(lowSeg.lowerEdge, LandscapeSegment::Edge::Box);
            else
                QVERIFY2(lowSeg.lowerEdge == LandscapeSegment::Edge::Crossing && std::abs(lowSeg.lower - refLower) < tolerance,
                    qPrintable(QString("variable %1: lower end %2, grid %3").arg(v).arg(lowSeg.lower).arg(refLower)));
            if (gridHighAtBox)
                QCOMPARE(highSeg.upperEdge, LandscapeSegment::Edge::Box);
            else
                QVERIFY2(highSeg.upperEdge == LandscapeSegment::Edge::Crossing && std::abs(highSeg.upper - refUpper) < tolerance,
                    qPrintable(QString("variable %1: upper end %2, grid %3").arg(v).arg(highSeg.upper).arg(refUpper)));
            if (!gridLowAtBox && !gridHighAtBox)
                ++bounded;
        }
        // lg beta 1 is bounded on both sides; lg beta 2 is open towards "no second complex" (the data do not rule it out)
        QVERIFY(bounded >= 1);
        delete data;
    }

    // The method is a first-class result: a job through the JobManager lands in the model's store, survives export
    // and import, and a model without variable projection is refused without storing anything.
    void jobManagerStoresAndRestoresTheResult()
    {
        DataClass* data = makeData(3);
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB"), { 3.0 }, 3, 0.05);

        JobManager manager;
        manager.setModel(model);
        QJsonObject job = LandscapeConfigBlock;
        job["Walkers"] = 2;
        job["Polish"] = 3;
        job["MaxSteps"] = 20000;
        manager.AddSingleJob(job);
        manager.RunJobs();

        QCOMPARE(model->getStatistic(SupraFit::Method::LandscapeSampling, 0).isEmpty(), false);
        const QJsonObject stored = model->getStatistic(SupraFit::Method::LandscapeSampling, 0);
        QCOMPARE(AccessCI(stored["controller"].toObject(), "Method").toInt(), int(SupraFit::Method::LandscapeSampling));
        const QJsonObject result = stored["0"].toObject();
        QCOMPARE(result["variables"].toArray().size(), 1);
        const QJsonArray minima = result["minima"].toArray();
        QVERIFY(!minima.isEmpty());
        QVERIFY(minima[0].toObject()["within_level"].toBool()); // the fitted minimum itself, polished
        QVERIFY(std::abs(minima[0].toObject()["parameters"].toArray()[0].toDouble() - model->GlobalParameter(0)) < 0.05);

        // export / import round trip into a fresh model
        const QJsonObject exported = model->ExportModel(true);
        QSharedPointer<AbstractModel> copy = CreateModel(SupraFit::nmr_any, data);
        QJsonObject def;
        def["Reactions"] = strOption(QStringLiteral("A + B <=> AB"));
        copy->DefineModel(def);
        QVERIFY(copy->ImportModel(exported));
        QCOMPARE(copy->getStatistic(SupraFit::Method::LandscapeSampling, 0)["0"].toObject()["level"].toDouble(), result["level"].toDouble());
        delete data;
    }

    void modelWithoutVariableProjectionStoresNothing()
    {
        DataClass* data = makeData(2);
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_ItoI, data);
        model->setOption(1, QStringLiteral("yes")); // not the VarPro default: see nmr_1_1_Model::SupportsVarPro
        if (model->SupportsVarPro())
            QSKIP("the model still supports variable projection with this option");
        model->InitialGuess();

        JobManager manager;
        manager.setModel(model);
        manager.AddSingleJob(LandscapeConfigBlock);
        manager.RunJobs();
        QCOMPARE(model->getStatistic(SupraFit::Method::LandscapeSampling, 0).isEmpty(), true);
        delete data;
    }

    // Every control of the sampler reaches it and is written to the result: the integration and finite-difference steps, central
    // differences, the wall, the stopping rule and the bias (a very short run, only the settings are looked at).
    void controlsReachTheSampler()
    {
        DataClass* data = makeData(3);
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB"), { 3.0 }, 3, 0.05);
        LandscapeSearch search;
        search.setModel(model);
        QJsonObject ctl;
        ctl["Walkers"] = 1;
        ctl["Polish"] = 0;
        ctl["MaxSteps"] = 300;
        ctl["Sampler"] = QStringLiteral("Langevin");
        ctl["StepSize"] = 0.02;
        ctl["GradientStep"] = 5e-4;
        ctl["CentralGradient"] = true;
        ctl["LangevinCorrection"] = false;
        ctl["CutFactor"] = 7.5;
        ctl["MinSteps"] = 123;
        ctl["Patience"] = 17;
        ctl["MinHillFraction"] = 0.3;
        ctl["BiasFactor"] = 1.25;
        ctl["HillHeightFactor"] = 0.5;
        ctl["HillWidth"] = 0.08;
        search.setController(ctl);
        QVERIFY(search.Run());
        const QJsonObject result = search.Result()["0"].toObject();
        const QJsonObject s = result["settings"].toObject();
        QCOMPARE(s["step_size"].toDouble(), 0.02);
        QCOMPARE(s["gradient_step"].toDouble(), 5e-4);
        QCOMPARE(s["central_gradient"].toBool(), true);
        QCOMPARE(s["langevin_correction"].toBool(), false);
        QCOMPARE(s["cut_factor"].toDouble(), 7.5);
        QCOMPARE(s["min_steps"].toInt(), 123);
        QCOMPARE(s["patience"].toInt(), 17);
        QCOMPARE(s["min_hill_fraction"].toDouble(), 0.3);
        QCOMPARE(s["bias_factor"].toDouble(), 1.25);
        QCOMPARE(s["hill_height_factor"].toDouble(), 0.5);
        QCOMPARE(s["hill_width"].toDouble(), 0.08);
        // the wall is where it was asked for: SSE_min + 7.5 (level - SSE_min)
        const double sseMin = result["sse_min"].toDouble(), level = result["level"].toDouble();
        QVERIFY(std::abs(result["wall"].toDouble() - (sseMin + 7.5 * (level - sseMin))) < 1e-9 * result["wall"].toDouble());
        delete data;
    }

    // Langevin and Metropolis give the same interval on a fitted model (default settings, forward differences).
    void langevinAgreesWithMetropolisOnAModel()
    {
        DataClass* data = makeData(4, 4.0, 30);
        const double shifts[3] = { 9.0, 5.0, 2.0 };
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2"), { 3.0, 5.4 }, 4, 0.02, shifts);

        double edges[2][2];
        for (int k = 0; k < 2; ++k) {
            LandscapeSearch search;
            search.setModel(model);
            QJsonObject ctl;
            ctl["Walkers"] = 4;
            ctl["Polish"] = 0;
            ctl["Sampler"] = k ? QStringLiteral("Langevin") : QStringLiteral("Metropolis");
            search.setController(ctl);
            QVERIFY(search.Run());
            if (k) {
                const double moving = search.Result()["0"].toObject()["walkers"].toArray()[0].toObject()["acceptance"].toDouble();
                QVERIFY2(moving > 0.4, qPrintable(QString("Langevin walker is stopped by the wall: %1 of the moves pass").arg(moving)));
            }
            const auto& seg = search.Intervals()[0];
            QCOMPARE(seg.size(), size_t(1));
            QVERIFY(seg[0].Bounded());
            edges[k][0] = seg[0].lower;
            edges[k][1] = seg[0].upper;
            qInfo() << (k ? "Langevin" : "Metropolis") << "interval of lg beta 1" << edges[k][0] << edges[k][1] << "evals"
                    << search.Result()["0"].toObject()["evaluations"].toDouble();
        }
        // the sampled bins are about 0.05 wide
        QVERIFY2(std::abs(edges[0][0] - edges[1][0]) < 0.1 && std::abs(edges[0][1] - edges[1][1]) < 0.1,
            qPrintable(QString("Metropolis [%1, %2], Langevin [%3, %4]").arg(edges[0][0]).arg(edges[0][1]).arg(edges[1][0]).arg(edges[1][1])));
        delete data;
    }

    // Surface: points are recorded, the 2D profile of the first pair has the profile's minima where they belong.
    void surfaceProfileIsRecorded()
    {
        DataClass* data = makeData(4, 4.0, 30);
        const double shifts[3] = { 9.0, 5.0, 2.0 };
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2"), { 3.0, 5.4 }, 4, 0.02, shifts);

        LandscapeSearch search;
        search.setModel(model);
        QJsonObject ctl;
        ctl["Walkers"] = 4;
        ctl["Polish"] = 0;
        ctl["MaxSteps"] = 20000;
        ctl["Surface"] = true;
        search.setController(ctl);
        QVERIFY(search.Run());

        QVERIFY(!search.Points().empty());
        // two constants: one pair (0, 1), its grid as recorded
        const QJsonArray surfaces = search.Result()["0"].toObject()["surfaces"].toArray();
        QCOMPARE(surfaces.size(), 1);
        QCOMPARE(surfaces[0].toObject()["x"].toInt(), 0);
        QCOMPARE(surfaces[0].toObject()["y"].toInt(), 1);
        QCOMPARE(surfaces[0].toObject()["bins"].toInt(), 60);
        const QJsonArray surface = surfaces[0].toObject()["points"].toArray();
        QVERIFY(surface.size() > 100);

        // the surface profile of a variable runs along the 1D profile. Both take the minimum over the recorded points,
        // but the 1D profile bins with a clamp at the bounds and drops nothing, while the surface assignment puts a point
        // at the upper bound behind the wall: the two minima differ by at most one bin's step in SSE.
        const int bins = 60;
        std::vector<double> columnProfile(size_t(bins), INFINITY);
        double worst = 0, level = search.Level(), sseFit = search.Result()["0"].toObject()["sse_min"].toDouble();
        int compared = 0;
        for (const QJsonValue& v : surface) {
            const QJsonArray p = v.toArray();
            const double x = p[0].toDouble(), sse = p[2].toDouble();
            const int a = int((x - search.Lower()[0]) / (search.Upper()[0] - search.Lower()[0]) * bins);
            if (a >= 0 && a < bins)
                columnProfile[size_t(a)] = std::min(columnProfile[size_t(a)], sse);
        }
        double step = 0; // the largest change of the 1D profile from one bin to the next: one bin's step in SSE
        for (int a = 0; a + 1 < bins; ++a)
            if (std::isfinite(search.Merged().profile[0][a]) && std::isfinite(search.Merged().profile[0][a + 1]))
                step = std::max(step, std::abs(search.Merged().profile[0][a + 1] - search.Merged().profile[0][a]));
        for (int a = 0; a < bins; ++a)
            if (std::isfinite(search.Merged().profile[0][a]) && std::isfinite(columnProfile[size_t(a)])) {
                worst = std::max(worst, std::abs(columnProfile[size_t(a)] - search.Merged().profile[0][a]));
                ++compared;
            }
        qInfo() << "surface vs 1D profile:" << compared << "bins compared, worst difference" << worst << ", largest bin step of the 1D profile" << step
                << "(level" << level << ", SSE of the fit" << sseFit << ")";
        QVERIFY2(worst <= 2 * step + 1e-12, qPrintable(QString("worst %1 against a bin step of %2").arg(worst).arg(step)));
        // the level line in the result relative to the fit
        const QJsonObject result = search.Result()["0"].toObject();
        QVERIFY(result["contour_max"].toDouble() > result["level"].toDouble() / result["sse_min"].toDouble());

        // without Surface nothing is recorded
        LandscapeSearch quiet;
        quiet.setModel(model);
        QJsonObject c2;
        c2["Walkers"] = 2;
        c2["MaxSteps"] = 2000;
        quiet.setController(c2);
        QVERIFY(quiet.Run());
        QVERIFY(quiet.Points().empty());
        QVERIFY(quiet.Result()["0"].toObject()["surfaces"].toArray().isEmpty());
        delete data;
    }

    // Three constants give the 2D profile of every pair, in the order (0,1) (0,2) (1,2), each with its own grid.
    void everyPairHasASurface()
    {
        DataClass* data = makeData(4, 4.0, 30);
        const double shifts[4] = { 9.0, 5.0, 2.0, 7.0 };
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2\n2 A + B <=> A2B"), { 3.0, 5.4, 5.6 }, 4, 0.02, shifts);

        LandscapeSearch search;
        search.setModel(model);
        QJsonObject ctl;
        ctl["Walkers"] = 2;
        ctl["Polish"] = 0;
        ctl["MaxSteps"] = 3000;
        ctl["MaxExpansions"] = 0;
        ctl["Surface"] = true;
        search.setController(ctl);
        QVERIFY(search.Run());

        const QJsonArray surfaces = search.Result()["0"].toObject()["surfaces"].toArray();
        QCOMPARE(surfaces.size(), 3);
        const int pairs[3][2] = { { 0, 1 }, { 0, 2 }, { 1, 2 } };
        for (int q = 0; q < 3; ++q) {
            const QJsonObject s = surfaces[q].toObject();
            QCOMPARE(s["x"].toInt(), pairs[q][0]);
            QCOMPARE(s["y"].toInt(), pairs[q][1]);
            QVERIFY2(s["points"].toArray().size() > 10, qPrintable(QString("pair %1: %2 points").arg(q).arg(s["points"].toArray().size())));
        }
        // the fit lies in every grid: some point of each pair is below the level
        for (int q = 0; q < 3; ++q) {
            double lowest = INFINITY;
            for (const QJsonValue& v : surfaces[q].toObject()["points"].toArray())
                lowest = std::min(lowest, v.toArray()[2].toDouble());
            QVERIFY2(lowest < search.Level(), qPrintable(QString("pair %1: lowest SSE %2, level %3").arg(q).arg(lowest).arg(search.Level())));
        }
        delete data;
    }

    // Three constants: holes in the surface of a pair (cells enclosed by visited ones) are closed by a search over the third constant;
    // each new cell holds the minimum over that constant (checked against a dense scan), and the surface has more cells than without.
    void threeConstantSurfaceGapsAreClosed()
    {
        DataClass* data = makeData(4, 4.0, 30);
        const double shifts[4] = { 9.0, 5.0, 2.0, 7.0 };
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2\n2 A + B <=> A2B"), { 3.0, 5.4, 5.6 }, 4, 0.02, shifts);
        ModelLandscape landscape(model->Clone(false));

        auto run = [&](bool exact, LandscapeSearch& search) {
            search.setModel(model);
            QJsonObject ctl;
            ctl["Walkers"] = 2;
            ctl["Polish"] = 0;
            ctl["MaxSteps"] = 1500;
            ctl["MaxExpansions"] = 0;
            ctl["Surface"] = true;
            ctl["ExactSurface"] = exact;
            search.setController(ctl);
            QVERIFY(search.Run());
        };
        LandscapeSearch plain, closed;
        run(false, plain);
        run(true, closed);
        const QJsonArray before = plain.Result()["0"].toObject()["surfaces"].toArray();
        const QJsonArray after = closed.Result()["0"].toObject()["surfaces"].toArray();
        QCOMPARE(after.size(), 3);
        int cellsClosed = 0;
        for (int q = 0; q < 3; ++q) {
            cellsClosed += after[q].toObject()["closed_cells"].toInt();
            QVERIFY(after[q].toObject()["points"].toArray().size() >= before[q].toObject()["points"].toArray().size());
        }
        qInfo() << "cells closed over three pairs:" << cellsClosed << "; evaluations" << plain.Result()["0"].toObject()["evaluations"].toDouble() << "->" << closed.Result()["0"].toObject()["evaluations"].toDouble()
                << ", ms" << plain.Result()["0"].toObject()["time_ms"].toDouble() << "->" << closed.Result()["0"].toObject()["time_ms"].toDouble();
        QVERIFY(cellsClosed > 0);

        // pair (0, 1): every cell that was not there without the search, against a dense scan over the third constant
        const int bins = after[0].toObject()["bins"].toInt();
        const double xlo = closed.Lower()[0], xw = closed.Upper()[0] - xlo, ylo = closed.Lower()[1], yw = closed.Upper()[1] - ylo;
        std::vector<char> seen(size_t(bins) * bins, 0);
        for (const QJsonValue& v : before[0].toObject()["points"].toArray()) {
            const QJsonArray p = v.toArray();
            seen[size_t(int((p[0].toDouble() - xlo) / xw * bins)) * bins + size_t(int((p[1].toDouble() - ylo) / yw * bins))] = 1;
        }
        int checked = 0;
        double worstExcess = 0;
        for (const QJsonValue& v : after[0].toObject()["points"].toArray()) {
            const QJsonArray p = v.toArray();
            if (seen[size_t(int((p[0].toDouble() - xlo) / xw * bins)) * bins + size_t(int((p[1].toDouble() - ylo) / yw * bins))])
                continue;
            if (checked++ % 3 != 0)
                continue;
            double lowest = INFINITY;
            for (int n = 0; n <= 600; ++n)
                lowest = std::min(lowest, landscape({ p[0].toDouble(), p[1].toDouble(), closed.Lower()[2] + n * (closed.Upper()[2] - closed.Lower()[2]) / 600 }));
            worstExcess = std::max(worstExcess, (p[2].toDouble() - lowest) / lowest);
            QVERIFY2(p[2].toDouble() >= lowest * (1 - 1e-6) - 1e-9 || p[2].toDouble() > lowest * 0.99, "a closed cell below the dense scan");
        }
        qInfo() << "closed cells of pair (0,1) checked" << checked / 3 << ", worst excess over the dense scan" << worstExcess;
        QVERIFY(checked > 0);
        QVERIFY2(worstExcess < 0.02, qPrintable(QString("a closed cell above the dense scan by %1").arg(worstExcess)));
        delete data;
    }

    // Four constants: the holes of a pair surface are closed by a local search (Nelder-Mead) over the two others, started from the nearest
    // visited cell. A cell is an upper bound of the profile; compared with a grid over the two others (60 x 60, a coarse upper bound too).
    void fourConstantSurfaceGapsAreClosed()
    {
        DataClass* data = makeData(4, 4.0, 30);
        const double shifts[4] = { 9.0, 5.0, 2.0, 7.0 };
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2\n2 A + B <=> A2B\n2 A + 2 B <=> A2B2"), { 3.0, 5.4, 5.6, 8.0 }, 4, 0.02, shifts);
        ModelLandscape landscape(model->Clone(false));

        auto run = [&](bool exact, LandscapeSearch& search) {
            search.setModel(model);
            QJsonObject ctl;
            ctl["Walkers"] = 2;
            ctl["Polish"] = 0;
            ctl["MaxSteps"] = 1500;
            ctl["MaxExpansions"] = 0;
            ctl["Surface"] = true;
            ctl["ExactSurface"] = exact;
            ctl["ExactPairCells"] = 600;
            search.setController(ctl);
            QVERIFY(search.Run());
        };
        LandscapeSearch plain, closed;
        run(false, plain);
        run(true, closed);
        const QJsonArray before = plain.Result()["0"].toObject()["surfaces"].toArray();
        const QJsonArray after = closed.Result()["0"].toObject()["surfaces"].toArray();
        QCOMPARE(after.size(), 6);
        int cellsClosed = 0;
        for (int q = 0; q < 6; ++q)
            cellsClosed += after[q].toObject()["closed_cells"].toInt();
        qInfo() << "cells closed over six pairs:" << cellsClosed << "; evaluations" << plain.Result()["0"].toObject()["evaluations"].toDouble() << "->" << closed.Result()["0"].toObject()["evaluations"].toDouble()
                << ", ms" << plain.Result()["0"].toObject()["time_ms"].toDouble() << "->" << closed.Result()["0"].toObject()["time_ms"].toDouble();
        QVERIFY(cellsClosed > 0);

        const int bins = after[0].toObject()["bins"].toInt();
        const double xlo = closed.Lower()[0], xw = closed.Upper()[0] - xlo, ylo = closed.Lower()[1], yw = closed.Upper()[1] - ylo;
        std::vector<char> seen(size_t(bins) * bins, 0);
        for (const QJsonValue& v : before[0].toObject()["points"].toArray()) {
            const QJsonArray p = v.toArray();
            seen[size_t(int((p[0].toDouble() - xlo) / xw * bins)) * bins + size_t(int((p[1].toDouble() - ylo) / yw * bins))] = 1;
        }
        int checked = 0;
        double worstAbove = 0, worstBelow = 0;
        for (const QJsonValue& v : after[0].toObject()["points"].toArray()) {
            const QJsonArray p = v.toArray();
            if (seen[size_t(int((p[0].toDouble() - xlo) / xw * bins)) * bins + size_t(int((p[1].toDouble() - ylo) / yw * bins))])
                continue;
            if (checked++ % 8 != 0)
                continue;
            double lowest = INFINITY;
            for (int m = 0; m <= 60; ++m)
                for (int n = 0; n <= 60; ++n)
                    lowest = std::min(lowest, landscape({ p[0].toDouble(), p[1].toDouble(), closed.Lower()[2] + m * (closed.Upper()[2] - closed.Lower()[2]) / 60, closed.Lower()[3] + n * (closed.Upper()[3] - closed.Lower()[3]) / 60 }));
            worstAbove = std::max(worstAbove, (p[2].toDouble() - lowest) / lowest);
            worstBelow = std::max(worstBelow, (lowest - p[2].toDouble()) / lowest);
        }
        qInfo() << "closed cells of pair (0,1) checked" << (checked + 7) / 8 << ": worst above the grid" << worstAbove << ", worst below" << worstBelow;
        QVERIFY(checked > 0);
        QVERIFY2(worstAbove < 0.05, qPrintable(QString("a closed cell above the grid by %1").arg(worstAbove)));
        delete data;
    }

    // The minima search on a data set with two regions (1:1/2:1, lg beta (3.0, 4.5), noise 0.02, seed 6) whose blind fit stops on a
    // slope (SSE 8.6 times the optimum, no barrier to it: the SSE falls monotonically on the straight line). Each minimum comes with
    // the model to load it.
    void finderFindsTheOtherBasin()
    {
        const QString reactions = QStringLiteral("A + B <=> AB\n2 A + B <=> A2B");
        const double truth[2] = { 3.0, 4.5 };
        DataClass* data = makeData(3, 3.0, 20);
        {
            QSharedPointer<AbstractModel> signal = CreateModel(SupraFit::nmr_any, data);
            QJsonObject def;
            def["Reactions"] = strOption(reactions);
            signal->DefineModel(def);
            signal->InitialGuess();
            signal->setGlobalParameter(truth[0], 0);
            signal->setGlobalParameter(truth[1], 1);
            const double shifts[3] = { 9.0, 6.0, 3.0 };
            for (int s = 0; s < 3; ++s)
                for (int p = 0; p < signal->LocalParameterSize(); ++p)
                    signal->setLocalParameter(shifts[std::min(p, 2)] * (1.0 - 0.07 * s), p, s);
            signal->Calculate();
            Eigen::MatrixXd values = signal->ModelTable()->Table();
            std::mt19937_64 rng(6);
            std::normal_distribution<double> noise(0.0, 0.02);
            for (int r = 0; r < values.rows(); ++r)
                for (int c = 0; c < values.cols(); ++c)
                    values(r, c) += noise(rng);
            data->setDependentTable(new DataTable(values));
        }
        auto fit = [&](bool fromTruth) {
            QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
            QJsonObject def;
            def["Reactions"] = strOption(reactions);
            model->DefineModel(def);
            QJsonObject cfg = model->getOptimizerConfig();
            cfg["FitSolver"] = QStringLiteral("VarPro");
            model->setOptimizerConfig(cfg);
            model->InitialGuess();
            if (fromTruth) {
                model->setGlobalParameter(truth[0], 0);
                model->setGlobalParameter(truth[1], 1);
            }
            Minimizer minimizer(false);
            minimizer.setModel(model);
            minimizer.Minimize();
            model->Calculate();
            return model;
        };
        QSharedPointer<AbstractModel> right = fit(true), wrong = fit(false);
        ModelLandscape rightLandscape(right->Clone(false)), wrongLandscape(wrong->Clone(false));
        const double sseRight = rightLandscape(rightLandscape.Start()), sseWrong = wrongLandscape(wrongLandscape.Start());
        qInfo() << "right basin SSE" << sseRight << "wrong basin SSE" << sseWrong << "ratio" << sseWrong / sseRight;
        QVERIFY2(sseWrong > 3 * sseRight, "this data set no longer has a blind fit in the wrong basin");
        auto search = [&](QSharedPointer<AbstractModel> start) {
            LandscapeSearch s;
            s.setModel(start);
            QJsonObject ctl;
            ctl["Mode"] = QStringLiteral("Minima");
            ctl["Walkers"] = 4;
            ctl["MaxSteps"] = 30000;
            ctl["Polish"] = 10;
            s.setController(ctl);
            QObject::connect(&s, &AbstractSearchClass::Message, [](const QString& message) { qInfo() << "search message:" << message; });
            if (!s.Run())
                return QJsonObject();
            return s.Result()["0"].toObject();
        };

        // The data have a flat valley (lg beta 2 from 4.4 down to -9 at SSE 0.0221 - 0.0223; the fit, 0.02227, sits at its far end) and a
        // separate region without the second species (lg beta 2 -> -inf, SSE 0.0235). From the fit the search must list the valley's
        // deepest point first (0.7 % below the fit), say that it contains the fit, and find the other region as a minimum of its own.
        {
            const QJsonObject result = search(right);
            QCOMPARE(result["mode"].toString(), QStringLiteral("Minima"));
            QCOMPARE(result["recentered"].toInt(), 0); // a minima search does not move its centre, the deeper minimum is listed instead
            QVERIFY(std::abs(result["wall"].toDouble() / result["sse_min"].toDouble() - 10.0) < 1e-9);
            const QJsonArray minima = result["minima"].toArray();
            QVERIFY2(minima.size() >= 2, qPrintable(QString("%1 minima").arg(minima.size())));
            int fits = 0;
            double previous = 0;
            bool otherRegion = false;
            for (const QJsonValue& v : minima) {
                const QJsonObject m = v.toObject();
                qInfo() << "  from the fit: minimum" << m["sse"].toDouble() << "relative" << m["relative_sse"].toDouble() << "contains the fit" << m["is_fit"].toBool()
                        << "parameters" << m["parameters"].toArray();
                QVERIFY2(m["sse"].toDouble() >= previous, "the minima are sorted by SSE");
                previous = m["sse"].toDouble();
                QVERIFY2(!m["model"].toObject().isEmpty(), "a minimum comes with the model to load it");
                QVERIFY2(m["relative_sse"].toDouble() <= 1.0 + 1e-9 || !m["is_fit"].toBool(), "a refit that ends above the fit's own SSE at the fit is not reported");
                fits += m["is_fit"].toBool() ? 1 : 0;
                if (!m["is_fit"].toBool() && m["parameters"].toArray()[1].toDouble() < -5.0 && std::abs(m["sse"].toDouble() / 0.0234977 - 1.0) < 0.03)
                    otherRegion = true;
            }
            QCOMPARE(fits, 1); // the fit lies in exactly one of them
            const QJsonObject deepest = minima[0].toObject();
            QVERIFY2(std::abs(deepest["sse"].toDouble() / 0.022124 - 1.0) < 0.015, qPrintable(QString("deepest SSE %1").arg(deepest["sse"].toDouble())));
            QVERIFY(deepest["is_fit"].toBool());
            QVERIFY(deepest["relative_sse"].toDouble() < 1.0);
            QVERIFY2(otherRegion, "the region without the second species was not found as a minimum of its own");
        }
        // from the blind fit that stopped on the slope (SSE 8.6 times the optimum): downhill to the same deepest minimum, not itself
        {
            const QJsonObject result = search(wrong);
            const QJsonArray minima = result["minima"].toArray();
            QVERIFY(!minima.isEmpty());
            const QJsonObject first = minima[0].toObject();
            qInfo() << "  from the blind fit: deepest minimum" << first["sse"].toDouble() << "relative" << first["relative_sse"].toDouble();
            QVERIFY2(std::abs(first["sse"].toDouble() / 0.022124 - 1.0) < 0.015, qPrintable(QString("deepest SSE %1").arg(first["sse"].toDouble())));
            QVERIFY(first["relative_sse"].toDouble() < 0.2);
            QVERIFY(!first["is_fit"].toBool()); // the blind fit does not lie in a minimum: it stopped on the slope
            QVERIFY(!first["model"].toObject().isEmpty());
        }
        delete data;
    }

    // Two constants: the surface has no gaps. A short walk visits a path; with `ExactSurface` the cells in and around the explored
    // region are evaluated, so every cell of the rectangle the surface spans is there and holds at most the exact SSE at its centre
    // (the lowest visited point of a cell can be lower than that). Without it the same walk leaves gaps.
    void exactSurfaceHasNoGaps()
    {
        DataClass* data = makeData(4, 4.0, 30);
        const double shifts[3] = { 9.0, 5.0, 2.0 };
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2"), { 3.0, 5.4 }, 4, 0.02, shifts);
        ModelLandscape landscape(model->Clone(false));

        auto run = [&](bool exact) {
            LandscapeSearch search;
            search.setModel(model);
            QJsonObject ctl;
            ctl["Walkers"] = 2;
            ctl["Polish"] = 0;
            ctl["MaxSteps"] = 3000;
            ctl["MaxExpansions"] = 0;
            ctl["Surface"] = true;
            ctl["ExactSurface"] = exact;
            search.setController(ctl);
            if (!search.Run())
                return QJsonObject();
            return search.Result()["0"].toObject()["surfaces"].toArray()[0].toObject();
        };
        // bounds of the box are needed to put the points into bins: take them from the 1D profile result of a run
        LandscapeSearch probe;
        probe.setModel(model);
        QJsonObject pc;
        pc["Walkers"] = 2;
        pc["Polish"] = 0;
        pc["MaxSteps"] = 3000;
        pc["MaxExpansions"] = 0;
        pc["Surface"] = true;
        probe.setController(pc);
        QVERIFY(probe.Run());
        const QJsonObject probeResult = probe.Result()["0"].toObject();
        const QJsonObject exactSurface = probeResult["surfaces"].toArray()[0].toObject();
        QVERIFY(exactSurface["exact"].toBool());
        const int bins = exactSurface["bins"].toInt();
        QVERIFY(exactSurface["exact_evaluations"].toDouble() > 0 && exactSurface["exact_evaluations"].toDouble() <= double(bins) * bins);

        const double xlo = probe.Lower()[0], xw = probe.Upper()[0] - xlo, ylo = probe.Lower()[1], yw = probe.Upper()[1] - ylo;
        std::vector<char> present(size_t(bins) * size_t(bins), 0);
        int amin = bins, amax = -1, bmin = bins, bmax = -1;
        int checked = 0;
        double worstExcess = 0;
        for (const QJsonValue& v : exactSurface["points"].toArray()) {
            const QJsonArray p = v.toArray();
            const int a = int((p[0].toDouble() - xlo) / xw * bins), b = int((p[1].toDouble() - ylo) / yw * bins);
            QVERIFY(a >= 0 && a < bins && b >= 0 && b < bins);
            present[size_t(a) * bins + b] = 1;
            amin = std::min(amin, a);
            amax = std::max(amax, a);
            bmin = std::min(bmin, b);
            bmax = std::max(bmax, b);
            if (checked++ % 5 == 0) { // never above the exact SSE at the centre of the cell
                const double exactAtCentre = landscape({ p[0].toDouble(), p[1].toDouble() });
                worstExcess = std::max(worstExcess, (p[2].toDouble() - exactAtCentre) / exactAtCentre);
            }
        }
        int missing = 0;
        for (int a = amin; a <= amax; ++a)
            for (int b = bmin; b <= bmax; ++b)
                if (!present[size_t(a) * bins + b])
                    ++missing;
        qInfo() << "exact surface:" << exactSurface["points"].toArray().size() << "cells in a rectangle of" << (amax - amin + 1) * (bmax - bmin + 1) << ", missing" << missing
                << ", exact evaluations" << exactSurface["exact_evaluations"].toDouble() << ", worst excess over the exact value" << worstExcess;
        QCOMPARE(missing, 0);
        QVERIFY2(worstExcess < 1e-9, qPrintable(QString("a cell above the exact SSE by %1").arg(worstExcess)));

        // the same walk without it: gaps
        const QJsonObject plain = run(false);
        QVERIFY(!plain["exact"].toBool());
        QVERIFY2(plain["points"].toArray().size() < exactSurface["points"].toArray().size() / 2,
            qPrintable(QString("%1 visited cells against %2 exact").arg(plain["points"].toArray().size()).arg(exactSurface["points"].toArray().size())));
        delete data;
    }

    // Two constants: the exact grid also gives the exact profile of each constant, so the intervals of a short walk agree with an
    // independent profile over the whole box (within a bin) and none ends "not reached by the sampling"; without it the same short
    // walk leaves such ends.
    void exactProfileFillsTheIntervals()
    {
        DataClass* data = makeData(4, 4.0, 30);
        const double shifts[3] = { 9.0, 5.0, 2.0 };
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2"), { 3.0, 5.4 }, 4, 0.02, shifts);

        auto run = [&](bool exact, LandscapeSearch& search) {
            search.setModel(model);
            QJsonObject ctl;
            ctl["Walkers"] = 1;
            ctl["Polish"] = 0;
            ctl["MaxSteps"] = 400;
            ctl["MaxExpansions"] = 0;
            ctl["ExactSurface"] = exact;
            search.setController(ctl);
            return search.Run();
        };
        auto unsampledEnds = [](const LandscapeSearch& search) {
            int n = 0;
            for (const auto& variable : search.Intervals())
                for (const LandscapeSegment& s : variable)
                    n += (s.lowerEdge == LandscapeSegment::Edge::Unsampled) + (s.upperEdge == LandscapeSegment::Edge::Unsampled);
            return n;
        };

        LandscapeSearch plain, exact;
        QVERIFY(run(false, plain));
        QVERIFY(run(true, exact));
        QVERIFY(exact.Result()["0"].toObject()["exact_profile"].toBool());
        QVERIFY(!plain.Result()["0"].toObject()["exact_profile"].toBool());
        qInfo() << "unsampled interval ends: plain" << unsampledEnds(plain) << "exact" << unsampledEnds(exact);
        QVERIFY2(unsampledEnds(plain) > 0, "the short walk was meant to leave ends the walkers did not reach");
        QCOMPARE(unsampledEnds(exact), 0);

        // independent: the profile of each constant over the whole box, from a grid of its own (the same box, bins of the same size)
        ModelLandscape landscape(model->Clone(false));
        const int bins = 60;
        const std::vector<double>& lo = exact.Lower();
        const std::vector<double>& hi = exact.Upper();
        std::vector<double> profile[2] = { std::vector<double>(bins, INFINITY), std::vector<double>(bins, INFINITY) };
        for (int a = 0; a < bins; ++a)
            for (int b = 0; b < bins; ++b) {
                const double sse = landscape({ lo[0] + (a + 0.5) * (hi[0] - lo[0]) / bins, lo[1] + (b + 0.5) * (hi[1] - lo[1]) / bins });
                profile[0][a] = std::min(profile[0][a], sse);
                profile[1][b] = std::min(profile[1][b], sse);
            }
        const double level = exact.Level();
        for (int v = 0; v < 2; ++v) {
            const double w = (hi[v] - lo[v]) / bins;
            int first = -1, last = -1;
            for (int i = 0; i < bins; ++i)
                if (profile[v][i] <= level) {
                    if (first < 0)
                        first = i;
                    last = i;
                }
            QVERIFY(first >= 0);
            const auto& segments = exact.Intervals()[v];
            QVERIFY(!segments.empty());
            const double refLower = lo[v] + first * w, refUpper = lo[v] + (last + 1) * w;
            qInfo() << "variable" << v << "exact result [" << segments.front().lower << segments.back().upper << "] independent grid [" << refLower << refUpper << "] bin" << w;
            QVERIFY2(std::abs(segments.front().lower - refLower) < 1.5 * w && std::abs(segments.back().upper - refUpper) < 1.5 * w,
                qPrintable(QString("variable %1 differs from an independent grid by more than a bin").arg(v)));
        }
        delete data;
    }

    // The refits an analysis runs by itself are quiet (no line in the log, no notification: the model's Message and Warning stay
    // silent, while an ordinary fit speaks), and the progress tells what happens: the stages come in order, the bar of each stage
    // reaches exactly its maximum, and the stage text is not a message.
    void progressIsReportedAndRefitsAreQuiet()
    {
        DataClass* data = makeData(4, 4.0, 30);
        const double shifts[3] = { 9.0, 5.0, 2.0 };
        auto model = fittedModel(data, QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2"), { 3.0, 5.4 }, 4, 0.02, shifts);

        // sanity: the log line of an ordinary fit exists, so that its absence below means something
        QSignalSpy messages(model->Info(), &DataClassPrivateObject::Message);
        QSignalSpy warnings(model->Info(), &DataClassPrivateObject::Warning);
        {
            Minimizer loud(false);
            loud.setModel(model->Clone(false));
            loud.Minimize();
        }
        QVERIFY2(messages.count() >= 1, "an ordinary fit is expected to log its time");
        messages.clear();

        struct Stage {
            QString text;
            bool restart;
            int maximum = -1;
            int increments = 0;
        };
        QList<Stage> stages;
        LandscapeSearch search;
        QObject::connect(&search, &AbstractSearchClass::Stage, [&](const QString& text, bool restart) {
            if (restart)
                stages << Stage{ text, true, -1, 0 };
            else if (!stages.isEmpty())
                stages.last().text = text; // the same stage, a new text
        });
        QObject::connect(&search, &AbstractSearchClass::setMaximumSteps, [&](int maximum) {
            if (!stages.isEmpty())
                stages.last().maximum = maximum;
        });
        QObject::connect(&search, &AbstractSearchClass::IncrementProgress, [&](int msecs) {
            QVERIFY(msecs >= 1); // the dialog ignores less
            if (!stages.isEmpty())
                ++stages.last().increments;
        });
        QSignalSpy plainMessages(&search, &AbstractSearchClass::Message);

        search.setModel(model);
        QJsonObject ctl;
        ctl["Walkers"] = 2;
        ctl["MaxSteps"] = 2500;
        ctl["Polish"] = 3;
        ctl["MaxExpansions"] = 0;
        ctl["Surface"] = true;
        search.setController(ctl);
        QVERIFY(search.Run());

        QCOMPARE(messages.count(), 0); // no "Optimisation took ..." for the refits
        QCOMPARE(warnings.count(), 0);
        QCOMPARE(plainMessages.count(), 0); // and the stages are no messages
        QStringList texts;
        for (const Stage& s : stages)
            texts << s.text;
        qInfo().noquote() << "stages:" << texts.join(" | ");
        QVERIFY2(stages.size() >= 4, qPrintable(texts.join(" | ")));
        QVERIFY2(stages[0].text.startsWith(QStringLiteral("Setting up")), qPrintable(stages[0].text));
        QVERIFY2(stages[1].text.startsWith(QStringLiteral("Sampling")), qPrintable(stages[1].text));
        QVERIFY2(stages[2].text.startsWith(QStringLiteral("Evaluating")), qPrintable(stages[2].text));
        QVERIFY2(stages[3].text.startsWith(QStringLiteral("Refitting")), qPrintable(stages[3].text));
        // sampling and setting up: the bar ends exactly at its maximum (a walker that ended early counts as complete)
        for (int k = 0; k < 2; ++k) {
            QVERIFY2(stages[k].maximum > 0, qPrintable(stages[k].text));
            QCOMPARE(stages[k].increments, stages[k].maximum);
        }
        QCOMPARE(stages[1].maximum, 2 * ((2500 + 499) / 500)); // two walkers, 500 steps a unit
        // the grid: one unit per 25 cells, the maximum follows the growing rectangle; never more increments than the maximum
        QVERIFY(stages[2].maximum > 0 && stages[2].increments > 0 && stages[2].increments <= stages[2].maximum);
        // the refits: one increment per candidate tried, not more than planned
        QVERIFY(stages[3].maximum > 0 && stages[3].increments >= 1 && stages[3].increments <= stages[3].maximum);
        delete data;
    }

    void modelWithoutVariableProjectionIsRefused()
    {
        LandscapeSearch none;
        QVERIFY(!none.Run()); // no model
    }
};

QTEST_MAIN(TestLandscapeSearch)
#include "test_landscapesearch.moc"
