/*
 * SupraFit - multiplet signals of the NMR import (NmrHandler)
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

// Claude Generated: multiplet signals of NmrHandler (class description, step 3b).
//
// Synthetic 1:1 titrations in fast exchange whose host signal is a multiplet (d, t, dd; J 3-15 Hz at 500 MHz) with a
// known centre that moves per step by 0.2-3x the closest line spacing; Lorentz lines of 2-20 points per FWHM, noise
// 0-5 % of the highest line, with or without a neighbour multiplet (moving in parallel, its centre 1.25-5x the mean span
// away, optionally with a roof effect = intensities bent towards each other). Per centre estimator (Centroid,
// OuterMidpoint, LineMean) with peak picking and with the lineshape fit: error of the centre in FWHM and ppm, cells not
// tracked, |Δ lg K| of the nmr_ItoI fit; the old tracking of one signal per line for comparison (lost and swapped
// cells). The WORST case of the spread is printed (qInfo) and pinned. Further: strongly coupled AB pairs (bias of the
// estimators), false multiplets in empty windows, conflict/missing/line-count warning, automatic grouping, JSON.
// MULTIPLET_DEBUG=1 prints every case (and the cells of the first estimator), MULTIPLET_ONLY=<text> runs the cases whose
// description contains the text, MULTIPLET_LIMIT=n every n-th case (neither is pinned), MULTIPLET_FIT_ALL=1 the lineshape
// fit on every case.

#include <cmath>
#include <random>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonArray>
#include <QtTest/QtTest>

#include "src/core/equil.h"
#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/nmrhandler.h"
#include "src/global.h"

namespace {
const double kMHz = 500.0, kDx = 0.0002, kLgK = 3.5, kHost = 2e-3, kStart = 7.5;
const int kSteps = 10;

struct Line {
    double x, height, fwhm;
};

/// Lorentz lines on a descending ppm axis from 'from' down to 'to', Gaussian noise of sigma noise
XYTrace spectrum(const QVector<Line>& lines, double from, double to, double noise, std::mt19937& rng, const QString& name)
{
    const int n = int(std::round((from - to) / kDx)) + 1;
    Eigen::VectorXd x(n), y(n);
    std::normal_distribution<double> gauss(0, 1);
    for (int i = 0; i < n; ++i) {
        x(i) = from - i * kDx;
        double v = 0;
        for (const Line& l : lines) {
            const double u = (x(i) - l.x) / (0.5 * l.fwhm);
            v += l.height / (1 + u * u);
        }
        y(i) = v + noise * gauss(rng);
    }
    return XYTrace(x, y, name);
}

double guest(int i, int n) { return kHost * 8.0 * std::pow(double(i) / double(n - 1), 2); }
double boundFraction(double host0, double guest0, double lgK) { return (host0 - ItoI::HostConcentration(host0, guest0, lgK)) / host0; }

/// first-order multiplet: line offsets from the centre [ppm] and relative intensities
struct Pattern {
    QString name;
    QVector<double> offsets, weights;
    double spacing = 0, span = 0; ///< closest spacing and outer distance [ppm]
};

Pattern makePattern(const QString& name, double jHz)
{
    const double J = jHz / kMHz, J2 = 0.45 * J;
    Pattern p;
    p.name = name;
    if (name == "s") {
        p.offsets = { 0 };
        p.weights = { 1 };
    } else if (name == "d") {
        p.offsets = { -J / 2, J / 2 };
        p.weights = { 1, 1 };
    } else if (name == "t") {
        p.offsets = { -J, 0, J };
        p.weights = { 0.5, 1, 0.5 };
    } else { // dd, the second coupling 0.45 J
        p.offsets = { -J / 2 - J2 / 2, -J / 2 + J2 / 2, J / 2 - J2 / 2, J / 2 + J2 / 2 };
        p.weights = { 1, 1, 1, 1 };
    }
    p.spacing = p.offsets.size() > 1 ? 1e300 : J;
    for (int i = 1; i < p.offsets.size(); ++i)
        p.spacing = std::min(p.spacing, p.offsets[i] - p.offsets[i - 1]);
    p.span = p.offsets.last() - p.offsets.first();
    return p;
}

/// lines of a pattern at centre; roof > 0 bends the intensities towards side (+1: higher ppm)
QVector<Line> lines(const Pattern& p, double centre, double fwhm, double roof = 0, int side = 1)
{
    QVector<Line> result;
    for (int i = 0; i < p.offsets.size(); ++i) {
        const double bend = p.span > 0 ? 1 + roof * side * p.offsets[i] / (0.5 * p.span) : 1;
        result << Line{ centre + p.offsets[i], p.weights[i] * bend, fwhm };
    }
    return result;
}

/// half span of a multiplet as a user sets it: the lines and a margin of a quarter of their spread, less than half the gap
/// to the next line of a neighbour (gap <= 0: none), at least two points
double halfSpanOf(const Pattern& p, double gap) { return 0.5 * p.span + std::max(gap > 0 ? std::min(0.25 * p.span, 0.4 * gap) : 0.25 * p.span, 2 * kDx); }

/// lg K of nmr_ItoI fitted to the shift columns (rows = steps)
qint64 g_lgKNs = 0; ///< time in fitLgK (timing of the spread)

double fitLgK(const Eigen::MatrixXd& shifts)
{
    QElapsedTimer timer;
    timer.start();
    Eigen::MatrixXd indep(shifts.rows(), 2);
    for (int k = 0; k < shifts.rows(); ++k) {
        indep(k, 0) = kHost;
        indep(k, 1) = guest(k, int(shifts.rows()));
    }
    DataClass* data = new DataClass();
    data->setIndependentTable(new DataTable(indep));
    data->setDataType(DataClassPrivate::Table);
    data->setSimulateDependent(int(shifts.cols()));
    data->setDependentTable(new DataTable(shifts));
    data->setDataBegin(0);
    data->setDataEnd(int(shifts.rows()));
    double lgK = std::numeric_limits<double>::quiet_NaN();
    {
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_ItoI, data);
        model->InitialGuess();
        Minimizer minim(false);
        minim.setModel(model);
        minim.Minimize();
        lgK = model->GlobalParameter(0);
    }
    delete data;
    g_lgKNs += timer.nsecsElapsed();
    return lgK;
}

struct MCase {
    QString pattern;
    double jHz, motion; ///< motion: largest step of the centre in units of the closest line spacing
    int fwhmPoints;
    double noise, neighbour, roof; ///< neighbour: centre distance in mean spans (0: none)
    QString text() const
    {
        return QString("%1 J %2 Hz, step %3 x spacing, FWHM %4 pt, noise %5 %, neighbour %6, roof %7").arg(pattern).arg(jHz).arg(motion).arg(fwhmPoints).arg(noise * 100).arg(neighbour).arg(roof);
    }
};

/// a synthetic series: spectra, true centres per step [signal] and the signals (multiplets and one per line)
struct Series {
    QVector<XYTrace> spectra;
    QVector<QVector<double>> centre; ///< [step][signal]
    QVector<QVector<QVector<double>>> lineTruth; ///< [step][signal][line]
    QVector<Pattern> patterns;
    double fwhm = 0, window = 0;
    double gap = 0; ///< between the outer lines of the two multiplets (0: no neighbour)
};

Series makeSeries(const MCase& c, unsigned seed)
{
    std::mt19937 rng(seed);
    Series s;
    s.fwhm = c.fwhmPoints * kDx;
    s.patterns << makePattern(c.pattern, c.jHz);
    if (c.neighbour > 0)
        s.patterns << makePattern("d", 7.0);
    QVector<double> x(kSteps);
    double maxStep = 0;
    for (int i = 0; i < kSteps; ++i) {
        x[i] = boundFraction(kHost, guest(i, kSteps), kLgK);
        if (i > 0)
            maxStep = std::max(maxStep, std::abs(x[i] - x[i - 1]));
    }
    const double delta = -c.motion * s.patterns[0].spacing / maxStep; // the shift moves upfield
    // a neighbour closer than the steps would be ambiguous for any tracking: at least 2.5 x the largest step away
    const double distance = c.neighbour > 0 ? std::max(c.neighbour * 0.5 * (s.patterns[0].span + s.patterns[1].span), 2.5 * c.motion * s.patterns[0].spacing) : 0;
    s.window = std::max(0.01, 1.5 * c.motion * s.patterns[0].spacing);
    if (c.neighbour > 0)
        s.gap = distance - 0.5 * (s.patterns[0].span + s.patterns[1].span);
    double hi = -1e300, lo = 1e300;
    QVector<QVector<Line>> all(kSteps);
    for (int i = 0; i < kSteps; ++i) {
        const double a = kStart + delta * x[i];
        QVector<double> centres{ a };
        all[i] << lines(s.patterns[0], a, s.fwhm, c.roof, +1);
        if (c.neighbour > 0) {
            centres << a + distance;
            all[i] << lines(s.patterns[1], a + distance, s.fwhm, c.roof, -1);
        }
        QVector<QVector<double>> truth;
        for (int p = 0; p < s.patterns.size(); ++p) {
            QVector<double> l;
            for (double o : s.patterns[p].offsets)
                l << centres[p] + o;
            truth << l;
        }
        s.centre << centres;
        s.lineTruth << truth;
        for (const Line& l : all[i]) {
            hi = std::max(hi, l.x);
            lo = std::min(lo, l.x);
        }
    }
    for (int i = 0; i < kSteps; ++i)
        s.spectra << spectrum(all[i], hi + 0.05, lo - 0.05, c.noise, rng, QString("step %1").arg(i));
    return s;
}

TraceOpList operationsFor(const MCase& c)
{
    TraceOpList ops;
    if (c.noise > 0)
        ops.append(SmoothOp(std::max(int(std::round(0.7 * c.fwhmPoints)) | 1, 5), 2));
    return ops;
}

struct Outcome {
    int cells = 0, open = 0;
    double worstPpm = 0, worstFwhm = 0, sumFwhm = 0, dLgK = std::numeric_limits<double>::quiet_NaN();
    double worstChangeFwhm = 0; ///< error of δ_k - δ_0 (a constant bias cancels)
    int n = 0;
};

/// errors of the tracked centres against the truth, and lg K if complete
Outcome evaluate(const NmrHandler& handler, const Series& s, bool lgK)
{
    Outcome o;
    const int columns = s.patterns.size();
    Eigen::MatrixXd shifts(kSteps, columns);
    for (int k = 0; k < kSteps; ++k)
        for (int c = 0; c < columns; ++c) {
            const NmrCell cell = handler.cell(k, c);
            ++o.cells;
            shifts(k, c) = cell.shift;
            if (cell.status != NmrCellStatus::Ok) {
                ++o.open;
                continue;
            }
            const double e = std::abs(cell.shift - s.centre[k][c]);
            const NmrCell first = handler.cell(0, c);
            if (first.status == NmrCellStatus::Ok)
                o.worstChangeFwhm = std::max(o.worstChangeFwhm, std::abs((cell.shift - first.shift) - (s.centre[k][c] - s.centre[0][c])) / s.fwhm);
            o.worstPpm = std::max(o.worstPpm, e);
            o.worstFwhm = std::max(o.worstFwhm, e / s.fwhm);
            o.sumFwhm += e / s.fwhm;
            ++o.n;
        }
    // lg K only where the shift moves by at least 3 FWHM over the series (else it is not defined by the data)
    if (lgK && o.open == 0 && std::abs(s.centre.last()[0] - s.centre.first()[0]) >= 3 * s.fwhm)
        o.dLgK = std::abs(fitLgK(shifts) - kLgK);
    return o;
}

/// worst case of a method over the spread
struct Summary {
    QString name;
    int cases = 0, complete = 0, cells = 0, open = 0, n = 0;
    double worstPpm = 0, worstFwhm = 0, sumFwhm = 0, worstLgK = 0, worstChange = 0;
    QString worstCase, worstLgKCase, openCase, worstChangeCase;
    void add(const Outcome& o, const MCase& c)
    {
        ++cases;
        complete += o.open == 0;
        cells += o.cells;
        open += o.open;
        n += o.n;
        sumFwhm += o.sumFwhm;
        if (o.open > 0 && openCase.isEmpty())
            openCase = c.text();
        if (o.worstFwhm > worstFwhm) {
            worstFwhm = o.worstFwhm;
            worstCase = c.text();
        }
        worstPpm = std::max(worstPpm, o.worstPpm);
        if (o.worstChangeFwhm > worstChange) {
            worstChange = o.worstChangeFwhm;
            worstChangeCase = c.text();
        }
        if (std::isfinite(o.dLgK) && o.dLgK > worstLgK) {
            worstLgK = o.dLgK;
            worstLgKCase = c.text();
        }
    }
    void print() const
    {
        qInfo("%-28s %d/%d cases complete, %d of %d cells open; centre worst %.3f FWHM (%.2e ppm), mean %.3f FWHM; change from step 1 worst %.3f FWHM; |Δ lg K| worst %.4f",
            qPrintable(name), complete, cases, open, cells, worstFwhm, worstPpm, n > 0 ? sumFwhm / n : 0.0, worstChange, worstLgK);
        qInfo("    worst centre: %s", qPrintable(worstCase));
        qInfo("    worst change: %s", qPrintable(worstChangeCase));
        if (!worstLgKCase.isEmpty())
            qInfo("    worst lg K:   %s", qPrintable(worstLgKCase));
        if (!openCase.isEmpty())
            qInfo("    first open:   %s", qPrintable(openCase));
    }
};

NmrHandler handlerFor(const Series& s, const MCase& c)
{
    NmrHandler handler;
    for (const XYTrace& t : s.spectra)
        handler.addSpectrum(t);
    handler.setOperations(operationsFor(c));
    handler.setThresholdScope(NmrThresholdScope::Window);
    return handler;
}

/// the multiplets as a user defines them: centre a little off, half span as the automatic grouping sets it
QVector<NmrSignal> multipletSignals(const Series& s)
{
    QVector<NmrSignal> list;
    for (int p = 0; p < s.patterns.size(); ++p) {
        NmrSignal signal;
        signal.name = p == 0 ? "A" : "N";
        signal.type = NmrSignalType::Multiplet;
        signal.halfSpan = halfSpanOf(s.patterns[p], s.gap);
        // a little off the centre, within the margin of the half span (as a range drawn around the lines)
        signal.position = s.centre[0][p] + 0.3 * (signal.halfSpan - 0.5 * s.patterns[p].span);
        signal.window = s.window;
        signal.pattern = s.patterns[p].name;
        list << signal;
    }
    return list;
}
} // namespace

class TestNmrMultiplet : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qApp->setProperty("StoreRawData", true);
    }

    /// the spread of the file header for one pattern (one slot each: the debug build needs about 70 s per pattern)
    void syntheticSpreadDoublet() { runSpread("d"); }
    void syntheticSpreadTriplet() { runSpread("t"); }
    void syntheticSpreadDoubletOfDoublets() { runSpread("dd"); }
    /*! \brief Worst cases over the three patterns, printed and pinned. */
    void syntheticSpreadSummary()
    {
        if (m_summaries.isEmpty())
            QSKIP("the spread slots did not run");
        printSpread();
        if (m_summaries[1][0].cases != 912)
            return; // a part of the spread (MULTIPLET_LIMIT / MULTIPLET_ONLY): printed, not pinned
        // the default estimator (outer midpoint) with peak picking, measured 2026-11-21 (docs/nmr_multiplets_2026-11-21.md)
        const Summary& separated = m_summaries[1][0];
        const Summary& close = m_summaries[1][1];
        const Summary& roof = m_summaries[1][2];
        const Summary& fit = m_summaries[3][0];
        QCOMPARE(separated.open, 0);
        QVERIFY2(separated.worstFwhm < kPinSeparatedFwhm, qPrintable(QString::number(separated.worstFwhm)));
        QVERIFY2(separated.sumFwhm / separated.n < kPinSeparatedMeanFwhm, qPrintable(QString::number(separated.sumFwhm / separated.n)));
        QVERIFY2(separated.worstLgK < kPinSeparatedLgK, qPrintable(QString::number(separated.worstLgK)));
        QVERIFY2(close.open <= kPinCloseOpen, qPrintable(QString::number(close.open)));
        QVERIFY2(close.worstFwhm < kPinCloseFwhm, qPrintable(QString::number(close.worstFwhm)));
        QVERIFY2(roof.open <= kPinRoofOpen, qPrintable(QString::number(roof.open)));
        QVERIFY2(roof.worstLgK < kPinRoofLgK, qPrintable(QString::number(roof.worstLgK)));
        QVERIFY2(fit.open <= kPinFitOpen, qPrintable(QString::number(fit.open)));
        // the old tracking of one signal per line loses or swaps a large part of the same cells
        QVERIFY(m_old_lost[0] + m_old_swapped[0] > m_old_cells[0] / 10);
    }

    /*! \brief Strongly coupled AB pair (Δν/J 1.5-10, J 7 Hz): bias of each estimator against the true shift of A,
     * compared with the formulas of the class description ((b) outward by C - Δν/2, (a)/(c) inward); and a titration in
     * which A moves away from B (Δν/J from 2 to 10): |Δ lg K| per estimator. */
    void strongCoupling()
    {
        const double J = 7.0 / kMHz, fwhm = 6 * kDx;
        auto abLines = [&](double a, double b) {
            const double nu0 = 0.5 * (a + b), dnu = a - b, C = 0.5 * std::sqrt(dnu * dnu + J * J), s2 = J / (2 * C);
            return QVector<Line>{ { nu0 + C + J / 2, 1 - s2, fwhm }, { nu0 + C - J / 2, 1 + s2, fwhm }, { nu0 - C + J / 2, 1 + s2, fwhm }, { nu0 - C - J / 2, 1 - s2, fwhm } };
        };
        const NmrMultipletCentre estimators[3] = { NmrMultipletCentre::Centroid, NmrMultipletCentre::OuterMidpoint, NmrMultipletCentre::LineMean };
        double worstFormula = 0;
        for (double ratio : { 1.5, 2.0, 3.0, 5.0, 10.0 }) {
            const double b = kStart, a = kStart + ratio * J;
            std::mt19937 rng(3);
            NmrHandler handler;
            handler.addSpectrum(spectrum(abLines(a, b), a + 0.05, b - 0.05, 0, rng, "AB"));
            handler.setThresholdScope(NmrThresholdScope::Window);
            const double C = 0.5 * std::sqrt(std::pow(a - b, 2) + J * J);
            NmrSignal signal;
            signal.name = "A";
            signal.type = NmrSignalType::Multiplet;
            signal.position = 0.5 * (b + a) + C; // midpoint of the A lines
            signal.halfSpan = 0.5 * J + 2 * kDx;
            signal.window = 0.01;
            handler.setSignals({ signal });
            QString text;
            for (int e = 0; e < 3; ++e) {
                handler.setMultipletCentre(estimators[e]);
                handler.track();
                const NmrCell cell = handler.cell(0, 0);
                QCOMPARE(cell.status, NmrCellStatus::Ok);
                text += QString(" %1 %2 Hz").arg(NmrHandler::multipletCentreName(estimators[e])).arg((cell.shift - a) * kMHz, 0, 'f', 3);
                if (estimators[e] == NmrMultipletCentre::OuterMidpoint)
                    worstFormula = std::max(worstFormula, std::abs((cell.shift - a) - (C - 0.5 * (a - b))) * kMHz);
                if (estimators[e] == NmrMultipletCentre::LineMean)
                    worstFormula = std::max(worstFormula, std::abs((cell.shift - a) - (0.5 * (a - b) * (a - b) / (2 * C) - 0.5 * (a - b))) * kMHz);
            }
            qInfo("AB, Δν/J %.1f: bias of the centre of A%s (formula: midpoint %+.3f Hz, intensity-weighted %+.3f Hz)", ratio, qPrintable(text), (C - 0.5 * (a - b)) * kMHz,
                (0.5 * (a - b) * (a - b) / (2 * C) - 0.5 * (a - b)) * kMHz);
        }
        qInfo("AB: worst deviation of (b)/(c) from the formulas %.3f Hz", worstFormula);
        QVERIFY2(worstFormula < 0.1, qPrintable(QString::number(worstFormula)));
        // titration: A moves from Δν/J 2 to 10 away from the static B (the bias changes along the series)
        QVector<XYTrace> spectra;
        QVector<double> truth;
        for (int i = 0; i < kSteps; ++i) {
            const double x = boundFraction(kHost, guest(i, kSteps), kLgK);
            const double a = kStart + (2 + 8 * x) * J;
            truth << a;
            std::mt19937 rng(10 + i);
            spectra << spectrum(abLines(a, kStart), kStart + 12 * J, kStart - 0.05, 0, rng, QString("step %1").arg(i));
        }
        QString text;
        for (int e = 0; e < 3; ++e) {
            NmrHandler handler;
            for (const XYTrace& t : spectra)
                handler.addSpectrum(t);
            handler.setThresholdScope(NmrThresholdScope::Window);
            const double a0 = truth[0], C0 = 0.5 * std::sqrt(std::pow(a0 - kStart, 2) + J * J);
            NmrSignal signal;
            signal.name = "A";
            signal.type = NmrSignalType::Multiplet;
            signal.position = 0.5 * (a0 + kStart) + C0;
            signal.halfSpan = 0.5 * J + 2 * kDx;
            signal.window = 0.02;
            handler.setSignals({ signal });
            handler.setMultipletCentre(estimators[e]);
            handler.track();
            Eigen::MatrixXd shifts(kSteps, 1);
            double worst = 0;
            for (int k = 0; k < kSteps; ++k) {
                QCOMPARE(handler.cell(k, 0).status, NmrCellStatus::Ok);
                shifts(k, 0) = handler.cell(k, 0).shift;
                worst = std::max(worst, std::abs(shifts(k, 0) - truth[k]));
            }
            text += QString(" %1: worst %2 Hz, |Δ lg K| %3;").arg(NmrHandler::multipletCentreName(estimators[e])).arg(worst * kMHz, 0, 'f', 3).arg(std::abs(fitLgK(shifts) - kLgK), 0, 'f', 4);
        }
        qInfo("AB titration (A from Δν/J 2 to 10):%s", qPrintable(text));
    }

    /*! \brief No multiplet where there is none: multiplets placed on pure noise (and beside the wing of a 200x line) over
     * seeds, noise levels, widths and half spans are never Ok; pickMultiplets finds none in noise; on a spectrum with a d,
     * a t and a dd under 3 % sinusoidal ripple it finds exactly those three with their line counts (the one-signal-per-peak
     * pick is printed for comparison). */
    void noFalseMultiplets()
    {
        int windows = 0, falseCells = 0, falsePicks = 0, picks = 0;
        for (unsigned seed = 1; seed <= 10; ++seed)
            for (double noise : { 0.01, 0.05 })
                for (bool wing : { false, true })
                    for (int width : { 2, 20 })
                        for (double h : { 0.005, 0.02 }) {
                            std::mt19937 rng(seed);
                            NmrHandler handler;
                            for (int k = 0; k < kSteps; ++k) {
                                QVector<Line> lines;
                                if (wing)
                                    lines << Line{ kStart + 0.06, 200.0, width * kDx };
                                handler.addSpectrum(spectrum(lines, kStart + 0.1, kStart - 0.1, noise, rng, QString("noise %1").arg(k)));
                            }
                            TraceOpList ops;
                            ops.append(SmoothOp(std::max(int(std::round(0.7 * width)) | 1, 5), 2));
                            handler.setOperations(ops);
                            handler.setThresholdScope(NmrThresholdScope::Window);
                            NmrSignal signal;
                            signal.type = NmrSignalType::Multiplet;
                            signal.position = kStart;
                            signal.halfSpan = h;
                            signal.window = 0.02;
                            handler.setSignals({ signal });
                            handler.track();
                            for (int k = 0; k < kSteps; ++k) {
                                ++windows;
                                falseCells += handler.cell(k, 0).status == NmrCellStatus::Ok;
                            }
                            if (!wing) {
                                ++picks;
                                QString error;
                                falsePicks += handler.pickMultiplets(0.02, 20.0 / kMHz, 0.05, &error, kStart - 0.05, kStart + 0.05);
                            }
                        }
        qInfo("empty windows: %d of %d multiplet cells Ok (noise 1/5 %%, flat or beside a 200x line, FWHM 2/20 points, half span 0.005/0.02 ppm, 10 seeds); pick in noise: %d multiplets in %d spectra", falseCells, windows, falsePicks, picks);
        QCOMPARE(falseCells, 0);
        QCOMPARE(falsePicks, 0);

        // ripple: d (J 7), t (J 6), dd (J 8 / 3.6) with 3 % sinusoidal ripple of the highest line
        const double fwhm = 4 * kDx;
        QVector<Line> lines;
        lines << ::lines(makePattern("d", 7), 7.40, fwhm) << ::lines(makePattern("t", 6), 7.55, fwhm) << ::lines(makePattern("dd", 8), 7.70, fwhm);
        std::mt19937 rng(1);
        const XYTrace clean = spectrum(lines, 7.8, 7.3, 0, rng, "ripple");
        Eigen::VectorXd y = clean.y();
        for (Eigen::Index i = 0; i < y.size(); ++i)
            y(i) += 0.03 * std::sin(37.0 * i);
        NmrHandler handler;
        handler.addSpectrum(XYTrace(clean.x(), y, "ripple"));
        handler.setThresholdScope(NmrThresholdScope::Window);
        QString error;
        const int single = handler.pickSignals(0.02, &error);
        const int multiplets = handler.pickMultiplets(0.02, 20.0 / kMHz, 0.05, &error);
        handler.track();
        QString text;
        for (int s = 0; s < handler.signalList().size(); ++s)
            text += QString(" %1 (%2 lines)").arg(handler.signalList()[s].position, 0, 'f', 4).arg(handler.cell(0, s).lines.size());
        qInfo("3 %% ripple: one signal per peak %d signals; grouped %d multiplets:%s", single, multiplets, qPrintable(text));
        QCOMPARE(multiplets, 3);
        const int expected[3] = { 2, 3, 4 };
        const double centres[3] = { 7.40, 7.55, 7.70 };
        for (int s = 0; s < 3; ++s) {
            QCOMPARE(int(handler.cell(0, s).lines.size()), expected[s]);
            QVERIFY(std::abs(handler.cell(0, s).shift - centres[s]) < 0.25 * fwhm);
        }
    }

    /*! \brief Status rules: two multiplets whose spans interleave are a conflict, a vanishing multiplet is missing, a
     * peak signal on a line of a multiplet conflicts with it, the line count is compared with the pattern (warning only),
     * a manual centre is kept. */
    void multipletRules()
    {
        QCOMPARE(NmrHandler::expectedLineCount("s"), 1);
        QCOMPARE(NmrHandler::expectedLineCount("d"), 2);
        QCOMPARE(NmrHandler::expectedLineCount("t"), 3);
        QCOMPARE(NmrHandler::expectedLineCount("dd"), 4);
        QCOMPARE(NmrHandler::expectedLineCount("dt"), 6);
        QCOMPARE(NmrHandler::expectedLineCount("ddd"), 8);
        QCOMPARE(NmrHandler::expectedLineCount("quint"), 5);
        QCOMPARE(NmrHandler::expectedLineCount("sept"), 7);
        QCOMPARE(NmrHandler::expectedLineCount("m"), -1);
        QCOMPARE(NmrHandler::expectedLineCount(""), -1);
        QCOMPARE(NmrHandler::expectedLineCount("x"), -1);
        const double fwhm = 4 * kDx;
        const Pattern t = makePattern("t", 7), d = makePattern("d", 6);
        // A (t) moves onto B (d) from step 4 on; C (d) vanishes from step 5 on
        QVector<XYTrace> spectra;
        for (int k = 0; k < kSteps; ++k) {
            std::mt19937 rng(k + 1);
            QVector<Line> l = lines(t, 7.50 - 0.008 * std::min(k, 4), fwhm);
            l << lines(d, 7.45, fwhm);
            if (k < 5)
                l << lines(d, 7.30, fwhm);
            spectra << spectrum(l, 7.6, 7.2, 0.002, rng, QString("step %1").arg(k));
        }
        NmrHandler handler;
        for (const XYTrace& s : spectra)
            handler.addSpectrum(s);
        handler.setThresholdScope(NmrThresholdScope::Window);
        auto multiplet = [](const QString& name, double position, double half, const QString& pattern) {
            NmrSignal s;
            s.name = name;
            s.type = NmrSignalType::Multiplet;
            s.position = position;
            s.halfSpan = half;
            s.window = 0.01;
            s.pattern = pattern;
            return s;
        };
        handler.setSignals({ multiplet("A", 7.50, 0.017, "t"), multiplet("B", 7.45, 0.009, "dd"), multiplet("C", 7.30, 0.009, "d") });
        handler.track();
        QCOMPARE(handler.cell(0, 0).status, NmrCellStatus::Ok);
        QCOMPARE(handler.cell(0, 1).status, NmrCellStatus::Ok);
        QVERIFY(handler.lineCountWarning(0, 0).isEmpty());
        QVERIFY(!handler.lineCountWarning(0, 1).isEmpty()); // a d defined as dd
        qInfo("line count warning: %s", qPrintable(handler.lineCountWarning(0, 1)));
        QVERIFY(handler.lineCountWarning(0, 2).isEmpty());
        // A at 7.468 (step 4): its lines 7.454..7.482 interleave with those of B (7.444, 7.456)
        int conflicts = 0;
        for (int k = 0; k < kSteps; ++k) {
            QString row = QString("  %1:").arg(k);
            for (int s = 0; s < 3; ++s) {
                row += QString(" %1 %2 (").arg(handler.cell(k, s).shift, 0, 'f', 4).arg(NmrHandler::statusName(handler.cell(k, s).status));
                for (double l : handler.cell(k, s).lines)
                    row += QString(" %1").arg(l, 0, 'f', 4);
                row += ")";
            }
            if (qEnvironmentVariableIsSet("MULTIPLET_DEBUG"))
                qInfo("%s", qPrintable(row));
        }
        for (int k = 4; k < kSteps; ++k)
            conflicts += handler.cell(k, 0).status == NmrCellStatus::Conflict && handler.cell(k, 1).status == NmrCellStatus::Conflict;
        QCOMPARE(conflicts, kSteps - 4);
        for (int k = 0; k < kSteps; ++k)
            QCOMPARE(handler.cell(k, 2).status, k < 5 ? NmrCellStatus::Ok : NmrCellStatus::Missing);
        QVERIFY(!handler.problems().isEmpty());
        // a single peak on a line of C conflicts with it
        handler.addSignal(7.305, 0.003, "P"); // the upper line of C lies at 7.306
        handler.track();
        QCOMPARE(handler.cell(0, 3).status, NmrCellStatus::Conflict);
        QCOMPARE(handler.cell(0, 2).status, NmrCellStatus::Conflict);
        handler.removeSignal(3);
        // manual centre
        QVERIFY(handler.setManualShift(6, 2, 7.301));
        QCOMPARE(handler.cell(6, 2).status, NmrCellStatus::Manual);
        QCOMPARE(handler.cell(6, 2).shift, 7.301);
    }

    /*! \brief addMultiplet takes the centre of the lines of the range and half the range; pickMultiplets groups by the
     * coupling and gives every group its centre; a multiplet turned back into a peak follows one line. */
    void addAndPickMultiplets()
    {
        const double fwhm = 4 * kDx;
        std::mt19937 rng(5);
        QVector<Line> l = lines(makePattern("dd", 8), 7.50, fwhm);
        l << lines(makePattern("t", 7), 7.40, fwhm);
        NmrHandler handler;
        handler.addSpectrum(spectrum(l, 7.6, 7.3, 0.002, rng, "one"));
        handler.setThresholdScope(NmrThresholdScope::Window);
        bool found = false;
        const int index = handler.addMultiplet(7.48, 7.52, 0.01, &found);
        QVERIFY(found);
        QCOMPARE(index, 0);
        const NmrSignal& added = handler.signalList()[0];
        QVERIFY(added.isMultiplet());
        QVERIFY(std::abs(added.halfSpan - 0.02) < 1e-12);
        QVERIFY2(std::abs(added.position - 7.50) < 0.1 * fwhm, qPrintable(QString::number(added.position, 'f', 6)));
        QCOMPARE(int(handler.cell(0, 0).lines.size()), 4);
        // an empty range: the middle, not found
        found = true;
        handler.addMultiplet(7.32, 7.34, 0.01, &found);
        QVERIFY(!found);
        QCOMPARE(handler.signalList().last().position, 7.33);
        QCOMPARE(handler.addMultiplet(7.4, 7.4, 0.01), -1);
        // grouping: J <= 20 Hz gives the dd and the t; J <= 2 Hz (below every spacing) splits them into their lines
        QString error;
        QCOMPARE(handler.pickMultiplets(0.01, 20.0 / kMHz, 0.05, &error), 2);
        handler.track();
        QCOMPARE(int(handler.cell(0, 0).lines.size()), 3);
        QCOMPARE(int(handler.cell(0, 1).lines.size()), 4);
        QVERIFY(std::abs(handler.signalList()[0].position - 7.40) < 0.1 * fwhm);
        QVERIFY(std::abs(handler.signalList()[1].position - 7.50) < 0.1 * fwhm);
        QCOMPARE(handler.signalList()[0].pattern, QString());
        QVERIFY(handler.pickMultiplets(0.01, 2.0 / kMHz, 0.05, &error) > 2);
        // a multiplet turned into a peak follows the nearest line again
        handler.pickMultiplets(0.01, 20.0 / kMHz, 0.05, &error);
        NmrSignal back = handler.signalList()[0];
        back.type = NmrSignalType::Peak;
        QVERIFY(handler.setSignal(0, back));
        handler.track();
        QVERIFY(handler.cell(0, 0).lines.isEmpty());
        QCOMPARE(handler.cell(0, 0).status, NmrCellStatus::Ok);
        QVERIFY(std::abs(handler.cell(0, 0).shift - 7.40) < 0.25 * fwhm); // the middle line of the t
        NmrSignal invalid = handler.signalList()[1];
        invalid.halfSpan = -1;
        QVERIFY(!handler.setSignal(1, invalid));
    }

    /*! \brief JSON: multiplet keys only with a multiplet; a block without them is read and written unchanged; the
     * lineshape fit of multiplets is recomputed identically after loading. */
    void multipletJson()
    {
        const double fwhm = 4 * kDx;
        NmrHandler handler;
        for (int k = 0; k < 4; ++k) {
            std::mt19937 rng(k);
            QVector<Line> l = lines(makePattern("dd", 8), 7.50 - 0.004 * k, fwhm);
            l << Line{ 7.30, 1.0, fwhm };
            handler.addSpectrum(spectrum(l, 7.6, 7.2, 0.002, rng, QString("s%1").arg(k)));
        }
        handler.setThresholdScope(NmrThresholdScope::Window);
        handler.addSignal(7.30, 0.01, "P");
        handler.track();
        const QJsonObject plain = handler.toJson();
        QVERIFY(!plain.contains("MultipletCentre"));
        QVERIFY(!plain.contains("MultipletRipple"));
        QVERIFY(!plain.value("Signals").toArray().at(0).toObject().contains("Type"));
        NmrHandler old;
        QString error;
        QVERIFY(old.fromJson(plain, &error));
        QCOMPARE(old.toJson(), plain);

        handler.addMultiplet(7.48, 7.52, 0.01);
        NmrSignal m = handler.signalList()[1];
        m.pattern = "dd";
        handler.setSignal(1, m);
        handler.setMultipletCentre(NmrMultipletCentre::LineMean);
        handler.setMultipletRipple(0.08);
        NmrFitSettings fit;
        fit.method = NmrShiftMethod::LineshapeFit;
        fit.range = 0.004;
        handler.setFitSettings(fit);
        handler.track();
        const QJsonObject json = handler.toJson();
        const QJsonObject stored = json.value("Signals").toArray().at(1).toObject();
        QCOMPARE(stored.value("Type").toString(), QString("Multiplet"));
        QCOMPARE(stored.value("HalfSpan").toDouble(), 0.02);
        QCOMPARE(stored.value("Pattern").toString(), QString("dd"));
        QCOMPARE(json.value("MultipletCentre").toString(), QString("LineMean"));
        QCOMPARE(json.value("MultipletRipple").toDouble(), 0.08);
        NmrHandler back;
        QVERIFY2(back.fromJson(json, &error), qPrintable(error));
        QCOMPARE(back.toJson(), json);
        QCOMPARE(back.multipletCentre(), NmrMultipletCentre::LineMean);
        for (int k = 0; k < 4; ++k) {
            QCOMPARE(back.cell(k, 1).status, handler.cell(k, 1).status);
            QCOMPARE(back.cell(k, 1).shift, handler.cell(k, 1).shift);
            QCOMPARE(back.cell(k, 1).lines, handler.cell(k, 1).lines);
            QCOMPARE(handler.cell(k, 1).fitState, NmrFitState::Fitted);
            QVERIFY(std::abs(handler.cell(k, 1).shift - (7.50 - 0.004 * k)) < 0.1 * fwhm);
        }
        // fromJson resets the options of a handler used before
        QVERIFY(back.fromJson(plain, &error));
        QCOMPARE(back.multipletCentre(), NmrMultipletCentre::OuterMidpoint);
        QCOMPARE(back.multipletRipple(), 0.05);
    }

private:
    // pins of the spread (outer midpoint, peak picking), measured 2026-11-21 in the debug build: separated 912/912 cases,
    // worst 2.10 FWHM, mean 0.027 FWHM, |Δ lg K| 0.257 (noise 5 %, FWHM 20 points, total shift 6 FWHM); close neighbour
    // 20 of 1200 cells open, worst 2.03 FWHM; roof 0.3: 112 of 9720 open, |Δ lg K| 0.471; lineshape fit separated 39 of
    // 3080 open
    static constexpr double kPinSeparatedFwhm = 2.6;
    static constexpr double kPinSeparatedMeanFwhm = 0.04;
    static constexpr double kPinSeparatedLgK = 0.32;
    static constexpr int kPinCloseOpen = 25;
    static constexpr double kPinCloseFwhm = 2.6;
    static constexpr int kPinRoofOpen = 140;
    static constexpr double kPinRoofLgK = 0.6;
    static constexpr int kPinFitOpen = 50;
    QVector<QVector<Summary>> m_summaries; ///< [method][class] over the spread slots run so far
    QVector<int> m_old_cells, m_old_lost, m_old_swapped, m_old_cases, m_old_complete;
    QStringList m_classes, m_methods;

    void printSpread()
    {
        for (const QVector<Summary>& row : m_summaries)
            for (const Summary& s : row)
                if (s.cases > 0)
                    s.print();
        for (int k = 0; k < 3; ++k)
            qInfo("one signal per line (old) / %s: %d/%d cases without a lost or swapped cell; %d of %d line cells lost (%.1f %%), %d swapped (%.1f %%)", qPrintable(m_classes[k]), m_old_complete[k], m_old_cases[k], m_old_lost[k], m_old_cells[k],
                100.0 * m_old_lost[k] / std::max(1, m_old_cells[k]), m_old_swapped[k], 100.0 * m_old_swapped[k] / std::max(1, m_old_cells[k]));
    }

    /*! \brief The spread of the file header for one pattern: every estimator in peak picking and the lineshape fit,
     * against the old tracking of one signal per line; added to the totals of the summary slot. */
    void runSpread(const QString& onlyPattern)
    {
        QVector<MCase> cases;
        const QStringList patterns = { onlyPattern };
        const QList<double> couplings = { 3.0, 8.0, 15.0 }, motions = { 0.2, 1.0, 3.0 }, noises = { 0.0, 0.02, 0.05 };
        const QList<int> widths = { 2, 6, 20 };
        const QList<QPair<double, double>> neighbours = { { 0, 0 }, { 1.25, 0 }, { 2.5, 0 }, { 5, 0 }, { 1.25, 0.3 }, { 2.5, 0.3 } };
        for (const QString& p : patterns)
            for (double j : couplings)
                for (double m : motions)
                    for (int w : widths)
                        for (double noise : noises)
                            for (const auto& [n, roof] : neighbours)
                                cases << MCase{ p, j, m, w, noise, n, roof };
        if (qEnvironmentVariableIsSet("MULTIPLET_LIMIT")) { // a part of the spread (every n-th case) while exploring
            const int every = qEnvironmentVariable("MULTIPLET_LIMIT").toInt();
            QVector<MCase> part;
            for (int i = 0; i < cases.size(); i += std::max(1, every))
                part << cases[i];
            cases = part;
        }
        if (qEnvironmentVariableIsSet("MULTIPLET_ONLY")) { // the cases whose description contains the text
            QVector<MCase> part;
            for (const MCase& c : cases)
                if (c.text().contains(qEnvironmentVariable("MULTIPLET_ONLY")))
                    part << c;
            cases = part;
        }
        const bool debug = qEnvironmentVariableIsSet("MULTIPLET_DEBUG");
        const bool fitAll = qEnvironmentVariableIsSet("MULTIPLET_FIT_ALL");
        // [method][class]: class 0 = no neighbour or its lines >= 3 FWHM away, 1 = neighbour closer (tails overlap or
        // lines merge), 2 = roof effect (intensities bent towards the neighbour)
        const QStringList methods = { "peak picking, centroid", "peak picking, outer midpoint", "peak picking, line mean", "lineshape fit" };
        const QStringList classes = { "separated", "close neighbour", "roof 0.3" };
        QVector<QVector<Summary>> summaries(4, QVector<Summary>(3));
        for (int m = 0; m < 4; ++m)
            for (int k = 0; k < 3; ++k)
                summaries[m][k].name = methods[m] + " / " + classes[k];
        if (m_summaries.isEmpty()) {
            m_summaries = summaries;
            m_methods = methods;
            m_classes = classes;
            m_old_cells = m_old_lost = m_old_swapped = m_old_cases = m_old_complete = QVector<int>(3, 0);
        }
        qint64 pickNs = 0, fitNs = 0, oldNs = 0;
        QVector<int> oldCells(3, 0), oldLost(3, 0), oldSwapped(3, 0), oldCases(3, 0), oldComplete(3, 0);
        QElapsedTimer timer;
        timer.start();
        unsigned seed = 1;
        for (const MCase& c : cases) {
            const Series s = makeSeries(c, seed++);
            NmrHandler handler = handlerFor(s, c);
            handler.setSignals(multipletSignals(s));
            const int cls = c.roof > 0 ? 2 : (c.neighbour > 0 && s.gap < 3 * s.fwhm ? 1 : 0);
            const NmrMultipletCentre estimators[3] = { NmrMultipletCentre::Centroid, NmrMultipletCentre::OuterMidpoint, NmrMultipletCentre::LineMean };
            QElapsedTimer part;
            part.start();
            for (int e = 0; e < 3; ++e) {
                handler.setMultipletCentre(estimators[e]);
                handler.track();
                const Outcome o = evaluate(handler, s, true);
                summaries[e][cls].add(o, c);
                m_summaries[e][cls].add(o, c);
                if (debug && e == 0)
                    for (int k = 0; k < kSteps; ++k) {
                        QString row = QString("  %1:").arg(k);
                        for (int p = 0; p < s.patterns.size(); ++p) {
                            const NmrCell cell = handler.cell(k, p);
                            row += QString(" %1 (true %2, %3, %4 lines").arg(cell.shift, 0, 'f', 5).arg(s.centre[k][p], 0, 'f', 5).arg(NmrHandler::statusName(cell.status)).arg(cell.lines.size());
                            for (double l : cell.lines)
                                row += QString(" %1").arg(l, 0, 'f', 4);
                            row += ")";
                        }
                        qInfo("%s", qPrintable(row));
                    }
                if (debug)
                    qInfo("%s | %s: open %d, worst %.3f FWHM, dlgK %.4f", qPrintable(c.text()), qPrintable(methods[e]), o.open, o.worstFwhm, o.dLgK);
            }
            pickNs += part.nsecsElapsed();
            part.restart();
            // lineshape fit (free widths): the corners of the spread (J 3/15 Hz, step 0.2/3, FWHM 2/20 points, noise 0/5 %),
            // every case with MULTIPLET_FIT_ALL (debug build: 0.3 s per case)
            if (fitAll || (c.jHz != 8.0 && c.motion != 1.0 && c.fwhmPoints != 6 && c.noise != 0.02)) {
                handler.setMultipletCentre(NmrMultipletCentre::OuterMidpoint);
                NmrFitSettings fit;
                fit.method = NmrShiftMethod::LineshapeFit;
                fit.range = 3 * s.fwhm;
                handler.setFitSettings(fit);
                handler.track();
                const Outcome o = evaluate(handler, s, true);
                summaries[3][cls].add(o, c);
                m_summaries[3][cls].add(o, c);
                if (debug)
                    qInfo("%s | fit: open %d, worst %.3f FWHM, dlgK %.4f", qPrintable(c.text()), o.open, o.worstFwhm, o.dLgK);
            }
            fitNs += part.nsecsElapsed();
            part.restart();
            // the old way: one signal per line, window 0.9 x the closest spacing of its multiplet
            NmrHandler single = handlerFor(s, c);
            QVector<NmrSignal> list;
            QVector<QPair<int, int>> owner; // (multiplet, line)
            for (int p = 0; p < s.patterns.size(); ++p)
                for (int l = 0; l < s.patterns[p].offsets.size(); ++l) {
                    list << NmrSignal{ QString("L%1_%2").arg(p).arg(l), s.lineTruth[0][p][l], 0.9 * s.patterns[p].spacing, true };
                    owner << qMakePair(p, l);
                }
            single.setSignals(list);
            single.track();
            int lost = 0, swapped = 0;
            for (int k = 0; k < kSteps; ++k)
                for (int i = 0; i < list.size(); ++i) {
                    const NmrCell cell = single.cell(k, i);
                    if (cell.status != NmrCellStatus::Ok) {
                        ++lost;
                        continue;
                    }
                    // the nearest true line of all multiplets
                    double best = 1e300;
                    QPair<int, int> nearest;
                    for (int p = 0; p < s.patterns.size(); ++p)
                        for (int l = 0; l < s.lineTruth[k][p].size(); ++l)
                            if (std::abs(s.lineTruth[k][p][l] - cell.shift) < best) {
                                best = std::abs(s.lineTruth[k][p][l] - cell.shift);
                                nearest = qMakePair(p, l);
                            }
                    swapped += nearest != owner[i];
                }
            oldNs += part.nsecsElapsed();
            oldCells[cls] += list.size() * kSteps;
            oldLost[cls] += lost;
            oldSwapped[cls] += swapped;
            ++oldCases[cls];
            oldComplete[cls] += lost == 0 && swapped == 0;
            m_old_cells[cls] += list.size() * kSteps;
            m_old_lost[cls] += lost;
            m_old_swapped[cls] += swapped;
            ++m_old_cases[cls];
            m_old_complete[cls] += lost == 0 && swapped == 0;
        }
        qInfo("%s: %d cases (%s), %lld ms (lg K fits %lld ms)", qPrintable(onlyPattern), int(cases.size()), "J 3/8/15 Hz, step 0.2/1/3 x spacing, FWHM 2/6/20 points, noise 0/2/5 %, neighbour none/1.25/2.5/5 spans, roof 0.3 at 1.25/2.5", timer.elapsed(), g_lgKNs / 1000000);
        qInfo("time: peak picking %lld ms, lineshape fit %lld ms, one signal per line %lld ms", pickNs / 1000000, fitNs / 1000000, oldNs / 1000000);
        for (const QVector<Summary>& row : summaries)
            for (const Summary& s : row)
                if (s.cases > 0)
                    s.print();
        for (int k = 0; k < 3; ++k)
            qInfo("one signal per line (old) / %s: %d/%d cases without a lost or swapped cell; %d of %d line cells lost (%.1f %%), %d swapped (%.1f %%)", qPrintable(classes[k]), oldComplete[k], oldCases[k], oldLost[k], oldCells[k],
                100.0 * oldLost[k] / std::max(1, oldCells[k]), oldSwapped[k], 100.0 * oldSwapped[k] / std::max(1, oldCells[k]));
    }
};

QTEST_GUILESS_MAIN(TestNmrMultiplet)
#include "test_nmrmultiplet.moc"
