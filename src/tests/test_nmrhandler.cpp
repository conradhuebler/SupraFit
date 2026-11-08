/*
 * Headless tests of the NMR import (NmrHandler, TraceIO)
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

// Claude Generated: WP3 of roadmap/input_data_editing.md.
//
// End to end: synthetic 1D spectra of a 1:1 titration in fast exchange (δ_obs = δ_free + x_bound
// (δ_complex - δ_free), x_bound from the closed 1:1 formula ItoI::HostConcentration) with Lorentz or
// pseudo-Voigt lines on a descending ppm axis, noise and an unrelated neighbour line -> NmrHandler ->
// shift table -> nmr_ItoI fit. The accuracy (position error in units of the line width, error of lg K
// and of the limiting shifts) is measured over a spread of widths, line shapes, noise levels, step
// counts and neighbour distances (FWHM 0.002-0.02 ppm on a 0.001 ppm grid = 2-20 points per line
// width, noise 0-5 % of the line height, 10/20 steps, neighbour 0.015-0.05 ppm); the WORST case is
// printed (qInfo) and pinned. NMR_DEBUG=1 prints every case.

#include <cmath>
#include <random>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtTest/QtTest>

#include "src/core/equil.h"
#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/nmrhandler.h"
#include "src/core/spectrahandler.h"
#include "src/core/trace/traceio.h"
#include "src/global.h"

namespace {
struct Line {
    double position, fwhm, amplitude, eta; ///< eta = Lorentz fraction of the pseudo-Voigt
};

double lineshape(double x, const Line& l)
{
    const double u = (x - l.position) / (0.5 * l.fwhm);
    return l.amplitude * (l.eta / (1 + u * u) + (1 - l.eta) * std::exp(-std::log(2.0) * u * u));
}

/// spectrum on a descending axis from 'from' to 'to' (from > to) with step dx
XYTrace spectrum(const QVector<Line>& lines, double from, double to, double dx, double noise, std::mt19937& rng, const QString& name)
{
    const int n = int(std::round((from - to) / dx)) + 1;
    Eigen::VectorXd x(n), y(n);
    std::normal_distribution<double> gauss(0, 1);
    for (int i = 0; i < n; ++i) {
        x(i) = from - i * dx;
        double v = 0;
        for (const Line& l : lines)
            v += lineshape(x(i), l);
        y(i) = v + noise * gauss(rng);
    }
    return XYTrace(x, y, name);
}

/// bound fraction of the host in a 1:1 system (closed formula of the project)
double boundFraction(double host0, double guest0, double lgK)
{
    return (host0 - ItoI::HostConcentration(host0, guest0, lgK)) / host0;
}

struct Case {
    double fwhm, eta, noise, neighbour;
    int steps;
};

struct Result {
    bool allOk = true;
    QString problem;
    double maxErrorFwhm = 0, maxErrorPpm = 0, dLgK = 0, maxLimitError = 0;
};

const double kLgK = 3.5, kHost = 2e-3, kFrom = 8.0, kTo = 6.5, kDx = 0.001;
// host signals: free -> complex
const double kFree[3] = { 7.60, 7.10, 6.95 };
const double kComplex[3] = { 7.30, 7.22, 6.80 };

bool writeTrace(const QString& file, const XYTrace& t, const QString& separator = "\t", bool decimalComma = false, const QString& header = QString(), bool crlf = false, bool bom = false)
{
    QFile f(file);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    QByteArray text;
    if (bom)
        text += "\xEF\xBB\xBF";
    const QString eol = crlf ? "\r\n" : "\n";
    if (!header.isEmpty())
        text += (header + eol).toUtf8();
    for (Eigen::Index i = 0; i < t.size(); ++i) {
        QString a = QString::number(t.x()(i), 'g', 17), b = QString::number(t.y()(i), 'g', 17);
        if (decimalComma) {
            a.replace('.', ',');
            b.replace('.', ',');
        }
        text += (a + separator + b + eol).toUtf8();
    }
    f.write(text);
    return true;
}
} // namespace

class TestNmrHandler : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;

    /// guest equivalents of step i of n, denser at the start where the shifts change fastest
    static double guest(int i, int n) { return kHost * 8.0 * std::pow(double(i) / double(n - 1), 2); }

    static QVector<XYTrace> titration(const Case& c, std::mt19937& rng, QVector<QVector<double>>* truth)
    {
        QVector<XYTrace> spectra;
        truth->clear();
        for (int i = 0; i < c.steps; ++i) {
            const double x = boundFraction(kHost, guest(i, c.steps), kLgK);
            QVector<Line> lines;
            QVector<double> row;
            for (int s = 0; s < 3; ++s) {
                const double delta = kFree[s] + x * (kComplex[s] - kFree[s]);
                row << delta;
                lines << Line{ delta, c.fwhm, 1.0 - 0.2 * s, c.eta };
            }
            // a line of the guest or an impurity below signal 2; it does not move
            lines << Line{ kFree[1] - c.neighbour, c.fwhm, 0.6, c.eta };
            truth->append(row);
            spectra << spectrum(lines, kFrom, kTo, kDx, c.noise, rng, QString("step %1").arg(i));
        }
        return spectra;
    }

    static TraceOpList pipeline(const Case& c)
    {
        TraceOpList ops;
        if (c.noise > 0) {
            int window = int(std::round(0.7 * c.fwhm / kDx)) | 1;
            ops.append(SmoothOp(std::max(window, 5), 2));
        }
        PickPeaksOp pick;
        pick.threshold = 0.05;
        pick.minDistance = 0.5 * c.fwhm;
        ops.append(pick);
        return ops;
    }

    /// handler -> table -> nmr_ItoI fit; errors against the truth
    static Result runCase(const Case& c, unsigned seed)
    {
        std::mt19937 rng(seed);
        QVector<QVector<double>> truth;
        NmrHandler handler;
        for (const XYTrace& t : titration(c, rng, &truth))
            handler.addSpectrum(t);
        handler.setOperations(pipeline(c));
        // positions as a user would click them: a little off the true peaks
        for (int s = 0; s < 3; ++s)
            handler.addSignal(kFree[s] + 0.002, 0.05, QString("H%1").arg(s + 1));
        handler.track();

        Result r;
        for (int k = 0; k < c.steps; ++k) {
            for (int s = 0; s < 3; ++s) {
                const NmrCell cell = handler.cell(k, s);
                if (cell.status != NmrCellStatus::Ok) {
                    r.allOk = false;
                    r.problem = QString("step %1 signal %2: %3").arg(k).arg(s).arg(NmrHandler::statusName(cell.status));
                    for (int kk = 0; kk < c.steps; ++kk) {
                        r.problem += QString("\n %1:").arg(kk);
                        for (int ss = 0; ss < 3; ++ss)
                            r.problem += QString(" %1/%2 (%3)").arg(handler.cell(kk, ss).shift, 0, 'f', 4).arg(truth[kk][ss], 0, 'f', 4).arg(NmrHandler::statusName(handler.cell(kk, ss).status));
                    }
                    return r;
                }
                const double e = std::abs(cell.shift - truth[k][s]);
                r.maxErrorPpm = std::max(r.maxErrorPpm, e);
                r.maxErrorFwhm = std::max(r.maxErrorFwhm, e / c.fwhm);
            }
        }
        if (qEnvironmentVariableIsSet("NMR_DEBUG") && r.maxErrorFwhm > 1) {
            for (int kk = 0; kk < c.steps; ++kk) {
                QString line = QString(" %1:").arg(kk);
                for (int ss = 0; ss < 3; ++ss)
                    line += QString(" %1/%2 h %3").arg(handler.cell(kk, ss).shift, 0, 'f', 4).arg(truth[kk][ss], 0, 'f', 4).arg(handler.cell(kk, ss).height, 0, 'f', 3);
                qInfo().noquote() << line;
            }
        }
        QString error;
        std::unique_ptr<DataTable> shifts(handler.shiftTable(&error));
        if (!shifts) {
            r.allOk = false;
            r.problem = error;
            return r;
        }
        Eigen::MatrixXd indep(c.steps, 2);
        for (int k = 0; k < c.steps; ++k) {
            indep(k, 0) = kHost;
            indep(k, 1) = guest(k, c.steps);
        }
        DataClass* data = new DataClass();
        data->setIndependentTable(new DataTable(indep));
        data->setDataType(DataClassPrivate::Table);
        data->setSimulateDependent(3);
        data->setDependentTable(new DataTable(shifts->Table()));
        data->setDataBegin(0);
        data->setDataEnd(c.steps);
        {
            QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_ItoI, data);
            model->InitialGuess();
            Minimizer minim(false);
            minim.setModel(model);
            minim.Minimize();
            r.dLgK = std::abs(model->GlobalParameter(0) - kLgK);
            for (int s = 0; s < 3; ++s) {
                r.maxLimitError = std::max(r.maxLimitError, std::abs(model->LocalParameter(0, s) - kFree[s]));
                r.maxLimitError = std::max(r.maxLimitError, std::abs(model->LocalParameter(1, s) - kComplex[s]));
            }
        }
        delete data;
        return r;
    }

    /// handler with a small titration and a few states set
    static void fillHandler(NmrHandler* handler, int steps = 16)
    {
        std::mt19937 rng(7);
        QVector<QVector<double>> truth;
        for (const XYTrace& t : titration(Case{ 0.01, 1.0, 0.0, 0.05, steps }, rng, &truth))
            handler->addSpectrum(t);
        handler->addSignal(kFree[0], 0.08, "H1");
        handler->addSignal(kFree[1], 0.08, "H2");
        handler->addSignal(kFree[2], 0.08, "H3");
        handler->track();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        qApp->setProperty("StoreRawData", true);
    }

    void numberLines()
    {
        QVector<double> v;
        QVERIFY(TraceIO::parseNumberLine("7,25", &v));
        QCOMPARE(v, QVector<double>({ 7.25 }));
        QVERIFY(TraceIO::parseNumberLine("1,2,3", &v));
        QCOMPARE(v, QVector<double>({ 1, 2, 3 }));
        QVERIFY(TraceIO::parseNumberLine("7.25, 3.0", &v));
        QCOMPARE(v, QVector<double>({ 7.25, 3.0 }));
        QVERIFY(TraceIO::parseNumberLine("7,25 3,5", &v));
        QCOMPARE(v, QVector<double>({ 7.25, 3.5 }));
        QVERIFY(TraceIO::parseNumberLine("1e-3\t2,5e2\t", &v));
        QCOMPARE(v, QVector<double>({ 1e-3, 250 }));
        QVERIFY(TraceIO::parseNumberLine("7,25;-1,5", &v));
        QCOMPARE(v, QVector<double>({ 7.25, -1.5 }));
        QVERIFY(TraceIO::parseNumberLine("\"7.25\",\"3\"", &v));
        QCOMPARE(v, QVector<double>({ 7.25, 3 }));
        QVERIFY(!TraceIO::parseNumberLine("# 7.25 3", &v));
        QVERIFY(!TraceIO::parseNumberLine("ppm intensity", &v));
        QVERIFY(!TraceIO::parseNumberLine("   ", &v));
        QVERIFY(!TraceIO::parseNumberLine("nan 1", &v));
    }

    /// files as NMR programs export them: every variant gives the same trace bit for bit
    void fileFormats()
    {
        std::mt19937 rng(1);
        const XYTrace t = spectrum({ Line{ 7.2, 0.01, 1, 1 } }, 7.4, 7.0, 0.002, 0.01, rng, "t");
        struct Variant {
            QString file, separator, header;
            bool comma, crlf, bom;
        };
        const QList<Variant> variants = {
            { "tab_comma_crlf_bom.txt", "\t", "# exported\r\n# ppm\tintensity", true, true, true },
            { "semicolon.csv", ";", "ppm;intensity", false, false, false },
            { "semicolon_comma.csv", ";", "\"ppm\";\"int\"", true, true, false },
            { "comma.csv", ",", "\"ppm\",\"int\"", false, false, false },
            { "blank_comma.txt", " ", "## title\n##\n", true, false, false },
            { "blank.xy", "  ", QString(), false, true, true },
        };
        for (const Variant& v : variants) {
            const QString file = m_dir.filePath(v.file);
            QVERIFY(writeTrace(file, t, v.separator, v.comma, v.header, v.crlf, v.bom));
            QVector<XYTrace> traces;
            QString error;
            QVERIFY2(TraceIO::readTraceFile(file, &traces, &error), qPrintable(v.file + ": " + error));
            QCOMPARE(traces.size(), 1);
            QCOMPARE(traces[0].size(), t.size());
            QCOMPARE(traces[0].orientation(), XYTrace::Orientation::Descending);
            QCOMPARE(traces[0].source(), file);
            for (Eigen::Index i = 0; i < t.size(); ++i) {
                QCOMPARE(traces[0].x()(i), t.x()(i));
                QCOMPARE(traces[0].y()(i), t.y()(i));
            }
        }
    }

    void matrixAndTopSpinAndErrors()
    {
        // matrix: x + three spectra
        const QString matrix = m_dir.filePath("matrix.csv");
        {
            QFile f(matrix);
            QVERIFY(f.open(QIODevice::WriteOnly));
            QTextStream s(&f);
            s << "ppm,s1,s2,s3\n";
            for (int i = 0; i < 50; ++i)
                s << 8.0 - 0.01 * i << "," << i << "," << 2 * i << "," << 3 * i << "\n";
        }
        QVector<XYTrace> traces;
        QString error;
        QVERIFY2(TraceIO::readTraceFile(matrix, &traces, &error), qPrintable(error));
        QCOMPARE(traces.size(), 3);
        QCOMPARE(traces[2].name(), QString("matrix.csv [3]"));
        QCOMPARE(traces[2].y()(10), 30.0);

        // TopSpin totxt: intensities only, the axis from LEFT/RIGHT
        const QString totxt = m_dir.filePath("totxt.txt");
        {
            QFile f(totxt);
            QVERIFY(f.open(QIODevice::WriteOnly));
            QTextStream s(&f);
            s << "# File created = Sunday, November 8, 2026\n# Spectral Region:\n# LEFT = 10.0 ppm. RIGHT = -1.0 ppm.\n#\n# SIZE = 12 ( = number of points)\n#\n";
            for (int i = 0; i < 12; ++i)
                s << i * 10 << "\n";
        }
        QVERIFY2(TraceIO::readTraceFile(totxt, &traces, &error), qPrintable(error));
        QCOMPARE(traces.size(), 1);
        QCOMPARE(traces[0].x()(0), 10.0);
        QCOMPARE(traces[0].x()(11), -1.0);
        QCOMPARE(traces[0].x()(1), 9.0);

        // errors with a message, no crash
        const QString text = m_dir.filePath("text.txt");
        {
            QFile f(text);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("no numbers here\nat all\n");
        }
        QVERIFY(!TraceIO::readTraceFile(text, &traces, &error));
        QVERIFY(error.contains("no rows of numbers"));
        const QString unordered = m_dir.filePath("unordered.txt");
        {
            QFile f(unordered);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("1 1\n3 2\n2 3\n");
        }
        QVERIFY(!TraceIO::readTraceFile(unordered, &traces, &error));
        QVERIFY(!error.isEmpty());
        const QString single = m_dir.filePath("single.txt");
        {
            QFile f(single);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("1\n2\n3\n");
        }
        QVERIFY(!TraceIO::readTraceFile(single, &traces, &error));
        QVERIFY(error.contains("LEFT"));
        QVERIFY(!TraceIO::readTraceFile(m_dir.filePath("does_not_exist.txt"), &traces, &error));
        NmrHandler handler;
        QVERIFY(!handler.addFile(text, &error));
        QCOMPARE(handler.spectrumCount(), 0);
        QVERIFY(handler.addFile(matrix, &error));
        QCOMPARE(handler.spectrumCount(), 3);
    }

    /// natural order "2" before "10"; the order can be changed afterwards
    void directoryOrder()
    {
        QDir().mkpath(m_dir.filePath("series"));
        std::mt19937 rng(2);
        for (int i : { 10, 2, 1 }) {
            const XYTrace t = spectrum({ Line{ 7.2 - 0.01 * i, 0.01, 1, 1 } }, 7.4, 7.0, 0.002, 0, rng, "t");
            QVERIFY(writeTrace(m_dir.filePath(QString("series/spec_%1.txt").arg(i)), t));
        }
        QVERIFY(writeTrace(m_dir.filePath("series/notes.dat"), spectrum({}, 7.4, 7.0, 0.1, 0, rng, "n")));
        NmrHandler handler;
        QString error;
        QVERIFY2(handler.addDirectory(m_dir.filePath("series"), ".txt", &error), qPrintable(error));
        QCOMPARE(handler.spectrumCount(), 3);
        QCOMPARE(handler.spectra().at(0).name(), QString("spec_1.txt"));
        QCOMPARE(handler.spectra().at(1).name(), QString("spec_2.txt"));
        QCOMPARE(handler.spectra().at(2).name(), QString("spec_10.txt"));
        handler.setReferenceIndex(2);
        QVERIFY(handler.moveSpectrum(2, 0));
        QCOMPARE(handler.spectra().at(0).name(), QString("spec_10.txt"));
        QCOMPARE(handler.referenceIndex(), 0); // the reference moves with its spectrum
        QVERIFY(handler.removeSpectrum(0));
        QCOMPARE(handler.spectrumCount(), 2);
        QVERIFY(!handler.addDirectory(m_dir.filePath("nothing"), ".txt", &error));
    }

    /// signals from the peaks of the reference spectrum
    void pickSignalsFromReference()
    {
        NmrHandler handler;
        fillHandler(&handler);
        QString error;
        const int n = handler.pickSignals(0.08, &error);
        QCOMPARE(n, 4); // three host signals and the neighbour line
        QCOMPARE(handler.signalList()[0].window, 0.08);
        // descending axis: the peaks come in axis order, highest ppm first
        QVERIFY(std::abs(handler.signalList()[0].position - kFree[0]) < 1e-3);
        handler.track();
        QVERIFY2(handler.problems().isEmpty(), qPrintable(handler.problems().join("\n")));
    }

    /// accuracy over a spread of conditions; worst case printed and pinned
    void endToEndSpread()
    {
        Result worst;
        QString worstPos, worstK, worstLimit;
        int cases = 0;
        unsigned seed = 11;
        for (double fwhm : { 0.002, 0.003, 0.008, 0.02 })
            for (double eta : { 1.0, 0.5 })
                for (double noise : { 0.0, 0.005, 0.02, 0.05 })
                    for (int steps : { 10, 20 })
                        for (double neighbour : { 0.05, 0.025, 0.015 }) {
                            const Case c{ fwhm, eta, noise, neighbour, steps };
                            const Result r = runCase(c, seed++);
                            const QString label = QString("FWHM %1 ppm, eta %2, noise %3, %4 steps, neighbour %5 ppm").arg(fwhm).arg(eta).arg(noise).arg(steps).arg(neighbour);
                            QVERIFY2(r.allOk, qPrintable(label + ": " + r.problem));
                            ++cases;
                            if (qEnvironmentVariableIsSet("NMR_DEBUG"))
                                qInfo().noquote() << QString("%1: pos %2 FWHM dlgK %3 lim %4").arg(label).arg(r.maxErrorFwhm, 0, 'g', 3).arg(r.dLgK, 0, 'g', 3).arg(r.maxLimitError, 0, 'g', 3);
                            if (r.maxErrorFwhm > worst.maxErrorFwhm) {
                                worst.maxErrorFwhm = r.maxErrorFwhm;
                                worstPos = label;
                            }
                            worst.maxErrorPpm = std::max(worst.maxErrorPpm, r.maxErrorPpm);
                            if (r.dLgK > worst.dLgK) {
                                worst.dLgK = r.dLgK;
                                worstK = label;
                            }
                            if (r.maxLimitError > worst.maxLimitError) {
                                worst.maxLimitError = r.maxLimitError;
                                worstLimit = label;
                            }
                        }
        qInfo().noquote() << QString("NMR tracking over %1 cases (lg K %2, 3 signals):").arg(cases).arg(kLgK);
        qInfo().noquote() << QString("  worst position error %1 FWHM (%2)").arg(worst.maxErrorFwhm, 0, 'g', 3).arg(worstPos);
        qInfo().noquote() << QString("  worst position error %1 ppm").arg(worst.maxErrorPpm, 0, 'g', 3);
        qInfo().noquote() << QString("  worst |lg K - true| %1 (%2)").arg(worst.dLgK, 0, 'g', 3).arg(worstK);
        qInfo().noquote() << QString("  worst limiting shift error %1 ppm (%2)").arg(worst.maxLimitError, 0, 'g', 3).arg(worstLimit);
        QVERIFY2(worst.maxErrorFwhm < kPinPositionFwhm, qPrintable(QString("position error %1 FWHM").arg(worst.maxErrorFwhm)));
        QVERIFY2(worst.dLgK < kPinLgK, qPrintable(QString("lg K error %1").arg(worst.dLgK)));
        QVERIFY2(worst.maxLimitError < kPinLimit, qPrintable(QString("limiting shift error %1 ppm").arg(worst.maxLimitError)));
    }

    /// a signal that vanishes is missing (NaN), never interpolated; the table is refused until the
    /// cells are set by hand or the rows/column are left out
    void missingSignal()
    {
        std::mt19937 rng(3);
        const int n = 10;
        NmrHandler handler;
        for (int k = 0; k < n; ++k) {
            const double x = boundFraction(kHost, guest(k, n), kLgK);
            QVector<Line> lines;
            for (int s = 0; s < 3; ++s)
                lines << Line{ kFree[s] + x * (kComplex[s] - kFree[s]), 0.01, (s == 2 && k >= 6) ? 0.0 : 1.0, 1.0 };
            handler.addSpectrum(spectrum(lines, kFrom, kTo, kDx, 0, rng, QString("step %1").arg(k)));
        }
        for (int s = 0; s < 3; ++s)
            handler.addSignal(kFree[s], 0.05, QString("H%1").arg(s + 1));
        handler.track();
        for (int k = 0; k < n; ++k) {
            const NmrCell cell = handler.cell(k, 2);
            QCOMPARE(cell.status, k >= 6 ? NmrCellStatus::Missing : NmrCellStatus::Ok);
            QCOMPARE(std::isnan(cell.shift), k >= 6);
        }
        QCOMPARE(handler.problems().size(), 4);
        QString error;
        QVERIFY(!handler.shiftTable(&error));
        QVERIFY2(error.contains("spectrum 7 (step 6), signal H3: missing"), qPrintable(error));

        // signal switched off
        NmrSignal off = handler.signalList()[2];
        off.enabled = false;
        QVERIFY(handler.setSignal(2, off));
        handler.track();
        std::unique_ptr<DataTable> table(handler.shiftTable(&error));
        QVERIFY2(table, qPrintable(error));
        QCOMPARE(table->columnCount(), 2);
        QCOMPARE(table->header(), QStringList({ "δ H1 /ppm", "δ H2 /ppm" }));
        off.enabled = true;
        handler.setSignal(2, off);
        handler.track();

        // rows left out
        for (int k = 6; k < n; ++k)
            handler.setSpectrumExcluded(k, true);
        handler.track();
        table.reset(handler.shiftTable(&error));
        QVERIFY2(table, qPrintable(error));
        QCOMPARE(table->rowCount(), 6);
        // concentrations of all spectra: the excluded rows are left out
        DataTable conc(n, 2, nullptr);
        for (int k = 0; k < n; ++k) {
            conc.data(k, 0) = kHost;
            conc.data(k, 1) = guest(k, n);
        }
        conc.setHeader({ "Host", "Guest" });
        std::unique_ptr<DataTable> import(handler.importTable(&conc, &error));
        QVERIFY2(import, qPrintable(error));
        QCOMPARE(import->rowCount(), 6);
        QCOMPARE(import->columnCount(), 5);
        QCOMPARE(import->data(5, 1), guest(5, n));
        QCOMPARE(import->header()[0], QString("Host"));
        DataTable wrong(3, 2, nullptr);
        QVERIFY(!handler.importTable(&wrong, &error));
        QVERIFY(error.contains("rows"));
        for (int k = 6; k < n; ++k)
            handler.setSpectrumExcluded(k, false);

        // set by hand
        for (int k = 6; k < n; ++k)
            QVERIFY(handler.setManualShift(k, 2, 6.81));
        QCOMPARE(handler.cell(7, 2).status, NmrCellStatus::Manual);
        QCOMPARE(handler.cell(7, 2).shift, 6.81);
        table.reset(handler.shiftTable(&error));
        QVERIFY2(table, qPrintable(error));
        QCOMPARE(table->rowCount(), n);
        QVERIFY(handler.clearManualShift(7, 2));
        QCOMPARE(handler.cell(7, 2).status, NmrCellStatus::Missing);
    }

    /// two signals that cross: where they merge into one peak both claim it (conflict)
    void crossingSignals()
    {
        std::mt19937 rng(4);
        NmrHandler handler;
        const int n = 11;
        for (int k = 0; k < n; ++k)
            handler.addSpectrum(spectrum({ Line{ 7.20 + 0.02 * k, 0.01, 1, 1 }, Line{ 7.35 - 0.01 * k, 0.01, 0.8, 1 } }, 7.6, 7.0, kDx, 0, rng, QString("s%1").arg(k)));
        handler.addSignal(7.20, 0.04, "up");
        handler.addSignal(7.35, 0.04, "down");
        handler.track();
        QCOMPARE(handler.cell(5, 0).status, NmrCellStatus::Conflict);
        QCOMPARE(handler.cell(5, 1).status, NmrCellStatus::Conflict);
        QCOMPARE(handler.cell(5, 0).shift, handler.cell(5, 1).shift);
        for (int k = 0; k < 4; ++k) {
            QCOMPARE(handler.cell(k, 0).status, NmrCellStatus::Ok);
            QVERIFY(std::abs(handler.cell(k, 0).shift - (7.20 + 0.02 * k)) < 1e-3);
        }
        QString error;
        QVERIFY(!handler.shiftTable(&error));
        QVERIFY(error.contains("conflict"));
    }

    /// referencing: a miscalibrated series, the offset brings it on the true scale; signals move along
    void referenceShift()
    {
        std::mt19937 rng(5);
        const double miscalibration = 0.123;
        NmrHandler handler;
        QVector<QVector<double>> truth;
        const Case c{ 0.01, 1.0, 0.0, 0.05, 10 };
        for (XYTrace t : titration(c, rng, &truth)) {
            Eigen::VectorXd x = t.x().array() + miscalibration;
            t.setData(x, t.y());
            handler.addSpectrum(t);
        }
        for (int s = 0; s < 3; ++s)
            handler.addSignal(kFree[s] + miscalibration, 0.05);
        handler.track();
        QVERIFY(std::abs(handler.cell(9, 0).shift - truth[9][0] - miscalibration) < 1e-3);
        handler.referenceTo(7.0 + miscalibration, 7.0);
        QVERIFY(std::abs(handler.referenceOffset() + miscalibration) < 1e-12);
        QVERIFY(std::abs(handler.signalList()[0].position - kFree[0]) < 1e-12);
        handler.track();
        for (int k = 0; k < 10; ++k)
            for (int s = 0; s < 3; ++s)
                QVERIFY2(std::abs(handler.cell(k, s).shift - truth[k][s]) < 1e-3, qPrintable(QString("%1 %2").arg(k).arg(s)));
        QVERIFY(std::abs(handler.referencedSpectra().at(0).xMax() - kFrom) < 1e-12);
    }

    /// reference spectrum in the middle: the series is followed in both directions
    void referenceInTheMiddle()
    {
        std::mt19937 rng(6);
        NmrHandler handler;
        QVector<QVector<double>> truth;
        const Case c{ 0.01, 0.5, 0.0, 0.05, 12 };
        for (const XYTrace& t : titration(c, rng, &truth))
            handler.addSpectrum(t);
        handler.setReferenceIndex(6);
        for (int s = 0; s < 3; ++s)
            handler.addSignal(truth[6][s] + 0.001, 0.05);
        handler.track();
        for (int k = 0; k < 12; ++k)
            for (int s = 0; s < 3; ++s) {
                QCOMPARE(handler.cell(k, s).status, NmrCellStatus::Ok);
                QVERIFY2(std::abs(handler.cell(k, s).shift - truth[k][s]) < 1e-3, qPrintable(QString("%1 %2").arg(k).arg(s)));
            }
    }

    /// every state survives toJson/fromJson; without raw data the files are read again
    void jsonRoundtrip()
    {
        NmrHandler handler;
        fillHandler(&handler);
        TraceOpList ops;
        ops.append(SmoothOp(5, 2));
        PickPeaksOp pick;
        pick.threshold = 0.05;
        pick.integrate = true;
        ops.append(pick);
        handler.setOperations(ops);
        handler.setReferenceOffset(0.01);
        handler.setReferenceIndex(2);
        handler.setDefaultWindow(0.03);
        NmrSignal off = handler.signalList()[1];
        off.enabled = false;
        handler.setSignal(1, off);
        handler.setSpectrumExcluded(10, true);
        handler.track();
        QVERIFY(handler.setManualShift(5, 2, 6.9));
        QVERIFY2(std::isfinite(handler.cell(4, 0).integral), qPrintable(NmrHandler::statusName(handler.cell(4, 0).status) + QString(" %1").arg(handler.cell(4, 0).shift)));
        const QJsonObject json = handler.toJson();
        QVERIFY(NmrHandler::isNmrBlock(json));
        QVERIFY(json.contains("Data"));

        NmrHandler back;
        QString error;
        QVERIFY2(back.fromJson(json, &error), qPrintable(error));
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(back.toJson(), json);
        QCOMPARE(back.referenceIndex(), 2);
        QCOMPARE(back.referenceOffset(), 0.01);
        QCOMPARE(back.signalList()[1].enabled, false);
        QVERIFY(back.isSpectrumExcluded(10));
        QCOMPARE(back.cell(5, 2).status, NmrCellStatus::Manual);
        for (int k = 0; k < handler.spectrumCount(); ++k)
            for (int s = 0; s < 3; ++s) {
                QCOMPARE(back.cell(k, s).status, handler.cell(k, s).status);
                if (std::isfinite(handler.cell(k, s).shift))
                    QCOMPARE(back.cell(k, s).shift, handler.cell(k, s).shift);
            }

        // through a DataClass of type Spectrum: the raw block comes back unchanged
        DataClass data;
        Eigen::MatrixXd indep = Eigen::MatrixXd::Ones(handler.spectrumCount(), 2), dep = Eigen::MatrixXd::Ones(handler.spectrumCount(), 2);
        data.setIndependentTable(new DataTable(indep));
        data.setDependentTable(new DataTable(dep));
        data.setDataType(DataClassPrivate::Spectrum);
        data.setRawData(json);
        DataClass copy;
        QVERIFY(copy.ImportData(data.ExportData()));
        QCOMPARE(copy.DataType(), DataClassPrivate::Spectrum);
        QCOMPARE(copy.RawData(), json);
    }

    void withoutRawDataTheFilesAreRead()
    {
        std::mt19937 rng(8);
        QVector<QVector<double>> truth;
        const QVector<XYTrace> spectra = titration(Case{ 0.01, 1.0, 0.0, 0.05, 4 }, rng, &truth);
        NmrHandler handler;
        // two single files and a matrix file of two spectra
        for (int i = 0; i < 2; ++i) {
            const QString file = m_dir.filePath(QString("raw_%1.txt").arg(i));
            QVERIFY(writeTrace(file, spectra[i]));
            QVERIFY(handler.addFile(file));
        }
        const QString matrix = m_dir.filePath("raw_matrix.csv");
        {
            QFile f(matrix);
            QVERIFY(f.open(QIODevice::WriteOnly));
            QTextStream s(&f);
            for (Eigen::Index i = 0; i < spectra[2].size(); ++i)
                s << QString::number(spectra[2].x()(i), 'g', 17) << ";" << QString::number(spectra[2].y()(i), 'g', 17) << ";" << QString::number(spectra[3].y()(i), 'g', 17) << "\n";
        }
        QVERIFY(handler.addFile(matrix));
        QCOMPARE(handler.spectrumCount(), 4);
        handler.moveSpectrum(0, 1); // the stored order is kept
        handler.addSignal(kFree[0], 0.05, "H1");
        handler.setManualShift(3, 0, 7.4);
        qApp->setProperty("StoreRawData", false);
        const QJsonObject json = handler.toJson();
        qApp->setProperty("StoreRawData", true);
        QVERIFY(!json.contains("Data"));
        NmrHandler back;
        QString error;
        QVERIFY2(back.fromJson(json, &error), qPrintable(error));
        QCOMPARE(back.spectrumCount(), 4);
        QCOMPARE(back.spectra().at(0).name(), QString("raw_1.txt"));
        QCOMPARE(back.spectra().at(3).name(), QString("raw_matrix.csv [2]"));
        for (int i = 0; i < 4; ++i)
            QCOMPARE(back.spectra().at(i).y(), handler.spectra().at(i).y());
        QCOMPARE(back.cell(3, 0).status, NmrCellStatus::Manual);

        // a file gone: named in the message, the positional states are not applied to the wrong spectra
        QVERIFY(QFile::remove(m_dir.filePath("raw_0.txt")));
        NmrHandler partial;
        QVERIFY(partial.fromJson(json, &error));
        QCOMPARE(partial.spectrumCount(), 3);
        QVERIFY2(error.contains("raw_0.txt") && error.contains("not restored"), qPrintable(error));
        QVERIFY(!partial.hasManualShift(2, 0));
    }

    /// older spectra projects: not an NMR block, read by SpectraHandler as before; an older SupraFit
    /// reads the spectra of an NMR block with its SpectraHandler
    void oldProjectsUnchanged()
    {
        SpectraHandler spectra;
        std::mt19937 rng(9);
        QVector<QVector<double>> truth;
        const QVector<XYTrace> traces = titration(Case{ 0.01, 1.0, 0.0, 0.05, 3 }, rng, &truth);
        for (int i = 0; i < 3; ++i) {
            const QString file = m_dir.filePath(QString("uv_%1.xy").arg(i));
            writeTrace(file, traces[i], " ");
            spectra.addSpectrum(file);
        }
        const QJsonObject old = spectra.getSpectraData();
        QVERIFY(!NmrHandler::isNmrBlock(old));
        QVERIFY(!old.contains("Profile"));

        NmrHandler handler;
        fillHandler(&handler, 5);
        SpectraHandler older;
        older.LoadData(handler.toJson());
        QCOMPARE(older.getOrder().size(), 5);
        QCOMPARE(older.Spectra().at(4).y(), handler.spectra().at(4).y());
    }

private:
    // pinned worst cases of endToEndSpread (measured values are printed by the test)
    // measured 2026-11-08 over 192 cases: 0.237 FWHM, 0.0085 lg K, 0.0022 ppm (all three: FWHM 0.02 ppm,
    // pseudo-Voigt eta 0.5, noise 5 % of the line height, 10 steps, neighbour line 0.015 ppm = 0.75 FWHM away)
    static constexpr double kPinPositionFwhm = 0.3;
    static constexpr double kPinLgK = 0.015;
    static constexpr double kPinLimit = 0.003;
};

QTEST_GUILESS_MAIN(TestNmrHandler)
#include "test_nmrhandler.moc"
