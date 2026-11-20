/*
 * SupraFit - JEOL Delta reader and FID processing
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
 * Claude Generated (JEOL import of roadmap/input_data_editing.md, findings in docs/jeol_import_2026-11-20.md).
 * A: JeolReader on synthetic files of both byte orders, truncated/corrupted files, absurd offsets.
 * B: radix-2 FFT against a direct DFT, line positions and automatic phases of synthetic FIDs (worst case over
 *    a spread, printed with qInfo and pinned), FidSettings JSON, NmrHandler with a synthetic .jdf series.
 * C: the real titration series in SUPRAFIT_JEOL_DIR (default: the operator's directory; SKIP if missing),
 *    compared with the shifts of the operator's own evaluation in the .suprafit file of that directory.
 *    The real files are only read. JEOL_EXPLORE=1 runs the real series over LB 0.3-6 Hz (and prints the
 *    auto-phase cases above 2°), JEOL_CELLS=1 prints every cell against the reference.
 */

#include <algorithm>
#include <cmath>
#include <complex>
#include <random>

#include <QtCore/QCollator>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/nmrhandler.h"
#include "src/core/trace/fidprocessing.h"
#include "src/core/trace/jeolreader.h"
#include "src/core/trace/traceops.h"

#include "jdf_test_writer.h"

namespace {
const double kPi = 3.14159265358979323846;
const double kFrequency = 500.159915; // MHz
const QString kReferenceFile = QStringLiteral("SB18_T1_T5_ObDGlc_CDCl3_20C.suprafit");

QString realDirectory()
{
    const QString dir = qEnvironmentVariable("SUPRAFIT_JEOL_DIR", QStringLiteral("/home/conrad/TUBAF/SB18_Titrationen"));
    return QDir(dir).exists() && !QDir(dir).entryList({ "*.jdf" }, QDir::Files).isEmpty() ? dir : QString();
}

/// the highest peak of the trace within [lo, hi] ppm (parabola-refined); index -1 if none
TracePeak highestPeak(const XYTrace& trace, double lo, double hi)
{
    QVector<TracePeak> peaks;
    TraceOps::pickPeaks(trace.range(lo, hi), 0.5, true, 0, false, &peaks);
    TracePeak best;
    best.height = -1e300;
    for (const TracePeak& p : peaks)
        if (p.height > best.height)
            best = p;
    return best;
}

double wrap(double degrees)
{
    double a = std::fmod(degrees, 360.0);
    if (a <= -180)
        a += 360;
    if (a > 180)
        a -= 360;
    return a;
}

/// direct DFT S_k = Σ f_j exp(-2πi jk/n) (reference of the FFT test)
Eigen::VectorXcd dft(const Eigen::VectorXcd& f)
{
    const Eigen::Index n = f.size();
    Eigen::VectorXcd s = Eigen::VectorXcd::Zero(n);
    for (Eigen::Index k = 0; k < n; ++k)
        for (Eigen::Index j = 0; j < n; ++j)
            s(k) += f(j) * std::polar(1.0, -2.0 * kPi * double((j * k) % n) / double(n));
    return s;
}

/// files of one series (prefix) in natural order
QStringList seriesFiles(const QString& dir, const QString& prefix)
{
    QStringList files = QDir(dir).entryList({ prefix + "*.jdf" }, QDir::Files);
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(files.begin(), files.end(), [&collator](const QString& a, const QString& b) { return collator.compare(a, b) < 0; });
    for (QString& f : files)
        f = QDir(dir).filePath(f);
    return files;
}

/// the operator's evaluation of one series: shift table (rows x signals), concentrations, lg K of model_0
struct Reference {
    QVector<QVector<double>> shifts;
    Eigen::MatrixXd concentrations;
    double lgK = std::numeric_limits<double>::quiet_NaN();
};

Reference readReference(const QString& dir, const QString& key)
{
    Reference ref;
    QFile file(QDir(dir).filePath(kReferenceFile));
    if (!file.open(QIODevice::ReadOnly))
        return ref;
    const QJsonObject root = QJsonDocument::fromJson(qUncompress(file.readAll())).object();
    auto rowsOf = [](const QJsonObject& table) {
        QVector<QVector<double>> rows;
        for (int r = 0; r < table.value("rows").toInt(); ++r) {
            QVector<double> row;
            for (const QString& v : table.value("data").toObject().value(QString::number(r)).toString().split(' ', Qt::SkipEmptyParts))
                row << v.toDouble();
            rows << row;
        }
        return rows;
    };
    for (const QString& p : root.keys()) {
        const QJsonObject project = root.value(p).toObject();
        const QJsonObject data = project.value("data").toObject();
        if (!data.value("title").toString().contains(key))
            continue;
        ref.shifts = rowsOf(data.value("dependent").toObject());
        const QVector<QVector<double>> conc = rowsOf(data.value("independent").toObject());
        ref.concentrations.resize(conc.size(), 2);
        for (int r = 0; r < conc.size(); ++r)
            for (int c = 0; c < 2; ++c)
                ref.concentrations(r, c) = conc[r].value(c);
        ref.lgK = project.value("model_0").toObject().value("data").toObject().value("globalParameter").toObject().value("data").toObject().value("0").toString().toDouble();
    }
    return ref;
}

/// lg K of a 1:1 NMR fit (nmr_ItoI) of these shifts
double fitLgK(const Eigen::MatrixXd& concentrations, const Eigen::MatrixXd& shifts)
{
    DataClass* data = new DataClass();
    data->setIndependentTable(new DataTable(concentrations));
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
    return lgK;
}
} // namespace

class TestJeol : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;

    /// Claude Generated: the series read with LB lb (settings first: every FID is transformed once, while it is read);
    /// kept for the next slot of the same run (the debug build needs about 35 s per series)
    static bool loadSeries(const QStringList& files, double lb, NmrHandler* handler, qint64* loadMs, QString* error)
    {
        static QHash<QString, QPair<NmrHandler, qint64>> cache;
        const QString key = files.value(0) + QString::number(files.size()) + "|" + QString::number(lb);
        if (!cache.contains(key)) {
            QElapsedTimer timer;
            timer.start();
            NmrHandler loaded;
            FidSettings settings = loaded.fidSettings();
            settings.lineBroadening = lb;
            if (!loaded.setFidSettings(settings, error))
                return false;
            for (const QString& file : files)
                if (!loaded.addFile(file, error))
                    return false;
            cache.insert(key, { loaded, timer.elapsed() });
        }
        *handler = cache.value(key).first;
        *loadMs = cache.value(key).second;
        return true;
    }

    /// a small FID file: three lines, digital filter delay of 19.65625 points
    static JdfTest::Spec smallSpec(bool littleEndian, int dataType = 0, int points = 256)
    {
        JdfTest::Spec s;
        s.littleEndian = littleEndian;
        s.dataType = dataType;
        s.params = JdfTest::fidParams(kFrequency, 5000, 5.0, points);
        s.values = JdfTest::fidPoints({ { 7.26, 1.0, 0.2 }, { 5.0, 0.5, 0.1 } }, kFrequency, 5000, 5.0, points, 19.65625, 30, 0, 0.01).conjugate();
        return s;
    }

    /// expected failure with a message (never a crash or a partial result)
    static void expectFailure(const QByteArray& bytes, const QString& part)
    {
        JeolReader::Data data;
        data.title = "untouched";
        QString error;
        QVERIFY(!JeolReader::readBytes(bytes, &data, &error, "x.jdf"));
        QVERIFY2(error.contains(part, Qt::CaseInsensitive), qPrintable(error + " / expected: " + part));
        QCOMPARE(data.title, QString("untouched"));
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
    }

    // ---------------------------------------------------------------------------------------------
    // A: reader

    void readBothByteOrders()
    {
        const JdfTest::Spec le = smallSpec(true), be = smallSpec(false);
        JeolReader::Data a, b;
        QString error;
        QVERIFY2(JeolReader::readBytes(JdfTest::write(le), &a, &error), qPrintable(error));
        QVERIFY2(JeolReader::readBytes(JdfTest::write(be), &b, &error), qPrintable(error));
        QVERIFY(a.littleEndian);
        QVERIFY(!b.littleEndian);
        for (const JeolReader::Data* d : { &a, &b }) {
            QCOMPARE(d->title, QString("synthetic"));
            QVERIFY(d->isTimeDomain());
            QCOMPARE(d->axisType, 3);
            QCOMPARE(d->points, qint64(256));
            QCOMPARE(d->values.size(), Eigen::Index(256));
            QCOMPARE(d->values, le.values); // float64: bit for bit
            QCOMPARE(d->frequency, kFrequency);
            QCOMPARE(d->sweep, 5000.0);
            QCOMPARE(d->offset, 5.0);
            QCOMPARE(d->nucleus, QString("Proton"));
            QCOMPARE(d->solvent, QString("CHLOROFORM-D"));
            QCOMPARE(d->temperature, 20.5);
            QCOMPARE(d->scans, 16);
            QVERIFY(d->digitalFilter);
            QCOMPARE(d->filterOrders, QVector<int>({ 2, 54, 73 }));
            QCOMPARE(d->groupDelay, 0.5 * 53 / 16.0 + 0.5 * 72 / 2.0);
            const JeolReader::Parameter* x90 = d->parameter("x90");
            QVERIFY(x90);
            QCOMPARE(x90->siValue(), 6e-6);
            QCOMPARE(x90->display(), QString("6 µs"));
            QCOMPARE(d->parameter("X_SWEEP")->display(), QString("5000 Hz"));
            QVERIFY(!d->parameter("NOT_THERE"));
        }
        // 32-bit floats
        JeolReader::Data f;
        QVERIFY2(JeolReader::readBytes(JdfTest::write(smallSpec(true, 1)), &f, &error), qPrintable(error));
        QVERIFY((f.values - le.values).cwiseAbs().maxCoeff() < 1e-6 * le.values.cwiseAbs().maxCoeff());
        // the FID convention: conjugated, delay kept
        Fid fid;
        QVERIFY2(FidProcessing::fromJeol(a, &fid, &error), qPrintable(error));
        QCOMPARE(fid.points, le.values.conjugate());
        QCOMPARE(fid.groupDelay, a.groupDelay);
        QVERIFY(fid.description.contains("CHLOROFORM-D"));
        // a file on disk, recognised by its content
        const QString path = m_dir.filePath("small.jdf");
        QVERIFY(JdfTest::save(path, JdfTest::write(le)));
        QVERIFY(JeolReader::isJeolFile(path));
        QVERIFY2(JeolReader::readFile(path, &a, &error), qPrintable(error));
        QCOMPARE(a.source, path);
    }

    void brokenFiles()
    {
        const QByteArray good = JdfTest::write(smallSpec(true));
        expectFailure(QByteArray(), "not a JEOL");
        expectFailure(QByteArray("JEOL.NM"), "not a JEOL");
        QByteArray wrong = good;
        wrong[0] = 'X';
        expectFailure(wrong, "not a JEOL");
        // truncated anywhere: always a message
        for (int size = 8; size < good.size(); size += 97)
            expectFailure(good.left(size), size < JeolReader::HeaderSize ? "header" : "truncated");

        auto patched = [&good](int offset, quint64 value, int width) {
            QByteArray b = good;
            if (width == 4)
                JdfTest::put32(b, offset, quint32(value), false);
            else
                JdfTest::put64(b, offset, value, false);
            return b;
        };
        expectFailure(patched(1212, 0xFFFFFF00u, 4), "parameter section"); // start beyond the file
        expectFailure(patched(1216, 0xFFFFFFF0u, 4), "parameter section"); // length beyond the file
        expectFailure(patched(1212, 100, 4), "parameter section"); // inside the header
        expectFailure(patched(1284, 0xFFFFFFF0u, 4), "data section");
        expectFailure(patched(1288, 0xFFFFFFFFFFFFFFF0ull, 8), "data section"); // absurd length, no allocation
        expectFailure(patched(176, 0xFFFFFFFFu, 4), "fewer than"); // 4e9 points: more than the data section holds
        {
            QByteArray b = patched(176, 0x10000000u, 4); // 2^28 points: more than the data section holds
            JdfTest::put32(b, 240, 0x0FFFFFFFu, false);
            expectFailure(b, "fewer than");
        }
        expectFailure(patched(208, 300, 4), "valid points");
        expectFailure(patched(1320, quint64(good.size()) + 1, 8), "truncated");
        {
            QByteArray b = good;
            b[8] = 7;
            expectFailure(b, "byte order");
        }
        {
            // parameter header in the file's byte order
            QByteArray b = good;
            JdfTest::put32(b, 1360, 32, true);
            expectFailure(b, "not supported");
            b = good;
            JdfTest::put32(b, 1360 + 8, 0x7FFFFFFF, true);
            expectFailure(b, "parameter index");
        }
        // other layouts: refused as "not supported"
        JdfTest::Spec twoD = smallSpec(true);
        twoD.extraPoints2 = 4;
        expectFailure(JdfTest::write(twoD), "not supported");
        JdfTest::Spec dims = smallSpec(true);
        dims.dimensions = 2;
        expectFailure(JdfTest::write(dims), "not supported");
        {
            QByteArray b = JdfTest::write(smallSpec(true));
            b[14] = char((2 << 6) | 1);
            expectFailure(b, "not supported");
            b = good;
            b[24] = 4;
            expectFailure(b, "not supported");
        }
        // a spectrum (ppm axis) is read but is no FID
        JdfTest::Spec spectrum = smallSpec(true);
        spectrum.axisUnit = 26;
        JeolReader::Data data;
        QString error;
        QVERIFY2(JeolReader::readBytes(JdfTest::write(spectrum), &data, &error), qPrintable(error));
        Fid fid;
        QVERIFY(!FidProcessing::fromJeol(data, &fid, &error));
        QVERIFY(error.contains("not supported"));
        // random corruption of the header and the parameter section: a result or a message, no crash
        std::mt19937 rng(11);
        std::uniform_int_distribution<int> where(8, 1360 + 16 + 64 * 13), value(0, 255);
        int refused = 0;
        for (int trial = 0; trial < 3000; ++trial) {
            QByteArray b = good;
            for (int k = 0; k < 4; ++k)
                b[where(rng)] = char(value(rng));
            JeolReader::Data d;
            QString reason;
            if (!JeolReader::readBytes(b, &d, &reason)) {
                ++refused;
                QVERIFY(!reason.isEmpty());
            }
        }
        qInfo("random corruption: %d of 3000 refused with a message, the others read", refused);
    }

    // ---------------------------------------------------------------------------------------------
    // B: FFT and FID processing

    void fftAgainstDft()
    {
        std::mt19937 rng(3);
        std::normal_distribution<double> gauss(0, 1);
        double worst = 0, worstRound = 0;
        int worstN = 0;
        for (int n = 1; n <= 4096; n *= 2) {
            Eigen::VectorXcd f(n);
            for (int j = 0; j < n; ++j)
                f(j) = std::complex<double>(gauss(rng), gauss(rng));
            Eigen::VectorXcd s = f;
            QVERIFY(FidProcessing::fft(s));
            const double scale = f.cwiseAbs().sum();
            const double error = (s - dft(f)).cwiseAbs().maxCoeff() / scale;
            if (error > worst) {
                worst = error;
                worstN = n;
            }
            QVERIFY(FidProcessing::fft(s, true));
            worstRound = std::max(worstRound, (s - f).cwiseAbs().maxCoeff() / f.cwiseAbs().maxCoeff());
        }
        qInfo("FFT vs DFT, n = 1..4096: worst max|FFT - DFT| / Σ|f| = %.3g (n = %d); round trip %.3g", worst, worstN, worstRound);
        QVERIFY(worst < 1e-14);
        QVERIFY(worstRound < 1e-13);
        Eigen::VectorXcd odd(12);
        odd.setOnes();
        QVERIFY(!FidProcessing::fft(odd));
        QCOMPARE(odd, Eigen::VectorXcd(Eigen::VectorXcd::Ones(12)));
        QCOMPARE(FidProcessing::paddedSize(1000, 1), Eigen::Index(1024));
        QCOMPARE(FidProcessing::paddedSize(1024, 2), Eigen::Index(2048));
        QCOMPARE(FidProcessing::paddedSize(FidProcessing::MaxSize, 2), Eigen::Index(0));
    }

    /// line positions over a spread of frequency, T2, noise, points, delay, ph0/ph1 (phase given = the true one)
    void linePositionsSpread()
    {
        const double sweep = 6250, offset = 4.5;
        // worst case per zero filling factor (index 1, 2)
        double worstFwhm[3] = { 0, 0, 0 }, worstHz[3] = { 0, 0, 0 };
        QString worstCase[3];
        int cases = 0;
        for (int points : { 8192, 32768 })
            for (double t2 : { 0.1, 0.3, 1.0 })
                for (double noise : { 0.0, 0.01, 0.05 })
                    for (double delay : { 0.0, 19.65625 })
                        for (double ph : { 0.0, 1.0 })
                            for (int zf : { 1, 2 }) {
                                const double ph0 = 73.0 * ph, ph1 = -41.0 * ph;
                                // lines on and between grid points, near the edges and the carrier
                                const QVector<JdfTest::Line> lines = { { 9.137, 1.0, t2 }, { 5.0011, 0.7, t2 }, { 2.4567, 0.5, t2 }, { -1.2345, 0.8, t2 } };
                                Fid fid;
                                fid.points = JdfTest::fidPoints(lines, kFrequency, sweep, offset, points, delay, ph0, ph1, noise, quint32(cases + 1));
                                fid.sweep = sweep;
                                fid.frequency = kFrequency;
                                fid.offset = offset;
                                fid.groupDelay = delay;
                                FidSettings settings;
                                settings.lineBroadening = 0.3;
                                settings.zeroFill = zf;
                                settings.autoPhase = false;
                                settings.ph0 = -ph0;
                                settings.ph1 = -ph1;
                                XYTrace spectrum;
                                QString error;
                                QVERIFY2(FidProcessing::process(fid, settings, &spectrum, nullptr, &error), qPrintable(error));
                                QCOMPARE(spectrum.orientation(), XYTrace::Orientation::Descending);
                                const double fwhm = 1.0 / (kPi * t2) + settings.lineBroadening; // Hz
                                for (const JdfTest::Line& line : lines) {
                                    const TracePeak p = highestPeak(spectrum, line.ppm - 0.05, line.ppm + 0.05);
                                    const double hz = std::abs(p.x - line.ppm) * kFrequency;
                                    ++cases;
                                    if (hz / fwhm > worstFwhm[zf]) {
                                        worstFwhm[zf] = hz / fwhm;
                                        worstHz[zf] = hz;
                                        worstCase[zf] = QString("N %1, T2 %2 s, noise %3, delay %4, phase %5, line %6 ppm").arg(points).arg(t2).arg(noise).arg(delay).arg(ph).arg(line.ppm);
                                    }
                                }
                            }
        for (int zf : { 1, 2 })
            qInfo("line position (parabola of the maximum, LB 0.3 Hz) over %d lines, zero fill x%d: worst %.3g FWHM = %.3g Hz (%s)", cases / 2, zf, worstFwhm[zf], worstHz[zf], qPrintable(worstCase[zf]));
        QVERIFY2(worstFwhm[1] < kPinPositionFwhm1, qPrintable(QString::number(worstFwhm[1])));
        QVERIFY2(worstFwhm[2] < kPinPositionFwhm2, qPrintable(QString::number(worstFwhm[2])));
    }

    /// the digital filter delay is removed: an on-resonance line of a delayed FID is absorptive, ph1 free
    void groupDelayCorrection()
    {
        const double sweep = 6250, offset = 4.5;
        const QVector<JdfTest::Line> lines = { { 8.5, 1.0, 0.3 }, { 0.5, 1.0, 0.3 } };
        Fid fid;
        fid.points = JdfTest::fidPoints(lines, kFrequency, sweep, offset, 16384, 19.6875);
        fid.sweep = sweep;
        fid.frequency = kFrequency;
        fid.offset = offset;
        fid.groupDelay = 19.6875;
        FidSettings settings;
        settings.autoPhase = false;
        XYTrace on, off;
        QVERIFY(FidProcessing::process(fid, settings, &on));
        settings.groupDelayCorrection = false;
        QVERIFY(FidProcessing::process(fid, settings, &off));
        for (const JdfTest::Line& line : lines) {
            const TracePeak p = highestPeak(on, line.ppm - 0.02, line.ppm + 0.02);
            // absorptive: the smallest value around the line is tiny compared with its height
            const XYTrace around = on.range(line.ppm - 0.05, line.ppm + 0.05);
            QVERIFY2(around.y().minCoeff() > -0.01 * p.height, qPrintable(QString::number(around.y().minCoeff() / p.height)));
            const XYTrace wrong = off.range(line.ppm - 0.05, line.ppm + 0.05);
            QVERIFY(wrong.y().minCoeff() < -0.2 * p.height);
        }
    }

    /// automatic phase over random ph0/ph1, noise and line sets
    void autoPhaseSpread()
    {
        const double sweep = 6250, offset = 4.5;
        std::mt19937 rng(5);
        std::uniform_real_distribution<double> phase0(-180, 180), phase1(-90, 90), position(-0.5, 9.5), amplitude(0.2, 1.0);
        double worst0 = 0, worst1 = 0, worstReal = 0;
        QString worstCase;
        int cases = 0, evaluations = 0;
        QElapsedTimer timer;
        timer.start();
        for (double noise : { 0.0, 0.005, 0.02 })
            for (int count : { 1, 3, 8 })
                for (int trial = 0; trial < 6; ++trial) {
                    QVector<JdfTest::Line> lines;
                    for (int l = 0; l < count; ++l)
                        lines << JdfTest::Line{ position(rng), amplitude(rng), 0.3 };
                    const double ph0 = phase0(rng), ph1 = count == 1 ? 0.0 : phase1(rng);
                    Fid fid;
                    fid.points = JdfTest::fidPoints(lines, kFrequency, sweep, offset, 16384, 19.6875, ph0, ph1, noise, quint32(1000 + cases));
                    fid.sweep = sweep;
                    fid.frequency = kFrequency;
                    fid.offset = offset;
                    fid.groupDelay = 19.6875;
                    FidSettings settings;
                    settings.autoPhase = true;
                    settings.autoFirstOrder = count > 1;
                    XYTrace spectrum;
                    FidPhase used;
                    QVERIFY(FidProcessing::process(fid, settings, &spectrum, &used));
                    evaluations += used.evaluations;
                    const double e0 = std::abs(wrap(used.ph0 + ph0)), e1 = std::abs(used.ph1 + ph1);
                    // phase error at the lines (what the spectrum shows): ph0 + ph1·(ν - ν_pivot)/sweep
                    double eLine = 0;
                    for (const JdfTest::Line& l : lines)
                        eLine = std::max(eLine, std::abs(wrap(used.ph0 + ph0 + (used.ph1 + ph1) * (l.ppm - offset) * kFrequency / sweep)));
                    ++cases;
                    if (eLine > 2 && qEnvironmentVariableIsSet("JEOL_EXPLORE"))
                        qInfo("  %d lines, noise %g, ph0 %.1f ph1 %.1f: found %.2f / %.2f, error at a line %.2f°", count, noise, ph0, ph1, used.ph0, used.ph1, eLine);
                    if (eLine > worstReal) {
                        worstReal = eLine;
                        worstCase = QString("%1 lines, noise %2, ph0 %3, ph1 %4: found %5 / %6").arg(count).arg(noise).arg(ph0, 0, 'f', 1).arg(ph1, 0, 'f', 1).arg(used.ph0, 0, 'f', 2).arg(used.ph1, 0, 'f', 2);
                    }
                    worst0 = std::max(worst0, e0);
                    worst1 = std::max(worst1, e1);
                }
        qInfo("auto phase over %d spectra (1-8 lines, noise 0-2 %%, ph0 ±180°, ph1 ±90°): worst phase error at a line %.3g° (%s); worst |Δph0| %.3g°, |Δph1| %.3g°; %d evaluations per spectrum, %lld ms each",
            cases, worstReal, qPrintable(worstCase), worst0, worst1, evaluations / cases, timer.elapsed() / cases);
        QVERIFY2(worstReal < kPinPhaseDegree, qPrintable(QString::number(worstReal)));
    }

    void settingsJson()
    {
        FidSettings s;
        s.lineBroadening = 1.25;
        s.gaussBroadening = 0.5;
        s.zeroFill = 4;
        s.ph0 = -12.5;
        s.ph1 = 33;
        s.pivot = 7.26;
        s.autoPhase = true;
        s.autoFirstOrder = false;
        s.groupDelayCorrection = false;
        FidSettings back;
        QString error;
        QVERIFY2(FidSettings::fromJson(s.toJson(), &back, &error), qPrintable(error));
        QVERIFY(back == s);
        QCOMPARE(s.toJson().value("Version").toInt(), FidSettings::Version);
        QVERIFY(FidSettings::fromJson(QJsonObject(), &back));
        QVERIFY(back == FidSettings());
        QJsonObject newer = s.toJson();
        newer["Version"] = FidSettings::Version + 1;
        QVERIFY(!FidSettings::fromJson(newer, &back, &error));
        QVERIFY(error.contains("version"));
        QJsonObject bad = s.toJson();
        bad["ZeroFill"] = 0;
        QVERIFY(!FidSettings::fromJson(bad, &back, &error));
        bad = s.toJson();
        bad["LineBroadening"] = -1;
        QVERIFY(!FidSettings::fromJson(bad, &back, &error));
        // a FID in JSON is lossless
        Fid fid;
        fid.points = JdfTest::fidPoints({ { 7.0, 1.0, 0.2 } }, kFrequency, 5000, 5.0, 64, 3.25, 10, 0, 0.1);
        fid.sweep = 5000;
        fid.frequency = kFrequency;
        fid.offset = 5;
        fid.groupDelay = 3.25;
        fid.name = "a.jdf";
        Fid again;
        QVERIFY2(Fid::fromJson(fid.toJson(), &again, &error), qPrintable(error));
        QCOMPARE(again.points, fid.points);
        QCOMPARE(again.groupDelay, 3.25);
    }

    /// NmrHandler with a synthetic .jdf series: directory, tracking, settings, persistence
    void nmrHandlerSeries()
    {
        const int steps = 8;
        const QString dir = m_dir.filePath("series");
        QDir().mkpath(dir);
        std::mt19937 rng(9);
        std::uniform_real_distribution<double> phase0(-180, 180);
        QVector<double> truth;
        for (int k = 0; k < steps; ++k) {
            const double f = 1.0 - std::exp(-1.5 * k / double(steps - 1));
            truth << 7.60 - 0.05 * f;
            // a receiver phase per experiment, as in the real series
            const QByteArray bytes = JdfTest::fidFile({ { truth.last(), 1.0, 0.3 }, { 7.26, 3.0, 0.3 }, { 6.90 + 0.02 * f, 0.6, 0.3 } }, kFrequency, 6250, 4.5, 8192, 19.6875, phase0(rng), -20, 0.002, k % 2 == 0);
            QVERIFY(JdfTest::save(QDir(dir).filePath(QString("titration_%1_single_pulse-1-1.jdf").arg(k + 1)), bytes));
        }
        QVERIFY(JdfTest::save(QDir(dir).filePath("notes.txt"), "not a spectrum"));
        NmrHandler handler;
        QString error;
        QVERIFY2(handler.addDirectory(dir, ".jdf", &error), qPrintable(error));
        QCOMPARE(handler.spectrumCount(), steps);
        QVERIFY(handler.hasFids());
        QVERIFY(handler.isFid(0));
        QVERIFY(handler.spectra().at(1).name().startsWith("titration_2_"));
        QVERIFY(handler.spectra().at(steps - 1).name().startsWith("titration_8_"));
        QCOMPARE(handler.spectra().at(0).orientation(), XYTrace::Orientation::Descending);
        QVERIFY(handler.fidDescription(0).contains("CHLOROFORM-D"));
        // default: every spectrum phased on its own
        QVERIFY(handler.fidSettings().autoPhase);
        handler.addSignal(7.60, 0.03, "H1");
        handler.addSignal(6.90, 0.03, "H2");
        handler.track();
        for (int k = 0; k < steps; ++k) {
            QCOMPARE(handler.cell(k, 0).status, NmrCellStatus::Ok);
            QVERIFY2(std::abs(handler.cell(k, 0).shift - truth[k]) < 2e-4, qPrintable(QString::number(handler.cell(k, 0).shift - truth[k])));
            QVERIFY(handler.spectra().at(k).range(7.5, 7.7).y().minCoeff() > -0.05 * handler.cell(k, 0).height);
        }
        // a common phase from the reference spectrum: the other spectra are mis-phased then
        QVERIFY2(handler.autoPhaseFromReference(&error), qPrintable(error));
        QVERIFY(!handler.fidSettings().autoPhase);
        QCOMPARE(handler.fidPhase(3).ph0, handler.fidSettings().ph0);
        // the settings change the spectra, ids and signals stay
        const QString id = handler.spectra().idAt(2);
        FidSettings settings = handler.fidSettings();
        settings.autoPhase = true;
        settings.zeroFill = 1;
        settings.lineBroadening = 1.0;
        QVERIFY2(handler.setFidSettings(settings, &error), qPrintable(error));
        QCOMPARE(handler.spectra().idAt(2), id);
        QCOMPARE(handler.spectra().at(2).size(), Eigen::Index(8192));
        settings.zeroFill = 99;
        QVERIFY(!handler.setFidSettings(settings, &error));
        QCOMPARE(handler.fidSettings().zeroFill, 1);
        handler.setManualShift(4, 1, 6.95);
        handler.track();

        for (bool raw : { true, false }) {
            qApp->setProperty("StoreRawData", raw);
            const QJsonObject json = handler.toJson();
            QVERIFY(json.contains("FidProcessing"));
            QCOMPARE(json.contains("Fid"), raw);
            NmrHandler back;
            QVERIFY2(back.fromJson(json, &error), qPrintable(error));
            QVERIFY2(error.isEmpty(), qPrintable(error));
            QVERIFY(back.fidSettings() == handler.fidSettings());
            QCOMPARE(back.spectrumCount(), steps);
            QVERIFY(back.isFid(steps - 1));
            for (int k = 0; k < steps; ++k) {
                QCOMPARE(back.spectra().at(k).y(), handler.spectra().at(k).y());
                for (int s = 0; s < 2; ++s) {
                    QCOMPARE(back.cell(k, s).status, handler.cell(k, s).status);
                    QCOMPARE(back.cell(k, s).shift, handler.cell(k, s).shift);
                }
            }
        }
        qApp->setProperty("StoreRawData", false);
        // without FIDs nothing new is written
        NmrHandler text;
        text.addSpectrum(handler.spectra().at(0));
        QVERIFY(!text.toJson().contains("FidProcessing"));
        QVERIFY(!text.toJson().contains("Fid"));
        // a broken .jdf is named, the others are read
        QByteArray broken = JdfTest::write(smallSpec(true));
        broken.truncate(3000);
        QVERIFY(JdfTest::save(QDir(dir).filePath("titration_9_single_pulse-1-1.jdf"), broken));
        NmrHandler partial;
        QVERIFY(partial.addDirectory(dir, ".jdf", &error));
        QCOMPARE(partial.spectrumCount(), steps);
        QVERIFY2(error.contains("titration_9") && error.contains("truncated"), qPrintable(error));
    }

    // ---------------------------------------------------------------------------------------------
    // C: real series

    void realFilesReadable()
    {
        const QString dir = realDirectory();
        if (dir.isEmpty())
            QSKIP("no JEOL data (SUPRAFIT_JEOL_DIR)");
        for (const QString& prefix : { QString("sb-SB18_T1_b_gluco_"), QString("sb-SB18_T5_CDCl3_ObDGlc_") }) {
            const QStringList files = seriesFiles(dir, prefix);
            QCOMPARE(files.size(), 15);
            qint64 points = -1;
            for (const QString& file : files) {
                JeolReader::Data data;
                QString error;
                QVERIFY2(JeolReader::readFile(file, &data, &error), qPrintable(error));
                QVERIFY(data.littleEndian);
                QVERIFY(data.isTimeDomain());
                QCOMPARE(data.axisType, 3);
                QCOMPARE(data.nucleus, QString("Proton"));
                QCOMPARE(data.solvent, QString("CHLOROFORM-D"));
                QVERIFY(std::abs(data.frequency - 500.1599) < 1e-3);
                QVERIFY(data.sweep > 5000 && data.sweep < 10000);
                QVERIFY(data.offset > 4 && data.offset < 5.5);
                QVERIFY(data.temperature > 18 && data.temperature < 22);
                QVERIFY(data.digitalFilter);
                QVERIFY(data.groupDelay > 19 && data.groupDelay < 20);
                QVERIFY(std::abs(data.acquisitionTime - data.points / data.sweep) < 1e-6);
                if (points < 0)
                    points = data.points;
                QCOMPARE(data.points, points);
                QCOMPARE(data.values.size(), Eigen::Index(points));
            }
            qInfo("%s: 15 files, %lld points each", qPrintable(prefix), points);
        }
    }

    /// the series through NmrHandler, compared with the operator's MestReNova shifts (multiplet centres)
    void realSeriesAgainstReference()
    {
        const QString dir = realDirectory();
        if (dir.isEmpty() || !QFile::exists(QDir(dir).filePath(kReferenceFile)))
            QSKIP("no JEOL data with the reference evaluation (SUPRAFIT_JEOL_DIR)");
        const QList<double> broadenings = qEnvironmentVariableIsSet("JEOL_EXPLORE") ? QList<double>{ 0.3, 1, 2, 3, 4, 6 } : QList<double>{ kRealLineBroadening };
        for (const QString& key : { QString("T1"), QString("T5") }) {
            const QStringList files = seriesFiles(dir, key == "T1" ? "sb-SB18_T1_b_gluco_" : "sb-SB18_T5_CDCl3_ObDGlc_");
            const Reference ref = readReference(dir, key);
            QCOMPARE(files.size(), ref.shifts.size());
            const int columns = ref.shifts.first().size();
            Eigen::MatrixXd refTable(files.size(), columns);
            for (int k = 0; k < files.size(); ++k)
                for (int c = 0; c < columns; ++c)
                    refTable(k, c) = ref.shifts[k][c];
            const double lgKRef = fitLgK(ref.concentrations, refTable);
            for (double lb : broadenings) {
                QElapsedTimer timer;
                qint64 loadMs = 0;
                QString error;
                NmrHandler handler;
                QVERIFY2(loadSeries(files, lb, &handler, &loadMs, &error), qPrintable(error));
                timer.start();
                // referencing: CHCl3 of the first spectrum at 7.26 ppm
                handler.setReferenceOffset(0);
                handler.referenceTo(highestPeak(handler.spectra().at(0), 7.2, 7.3).x, 7.26);
                double spread = 0; // CHCl3 of the other spectra on this scale
                for (int k = 0; k < handler.spectrumCount(); ++k)
                    spread = std::max(spread, std::abs(highestPeak(handler.referencedSpectra().at(k), 7.2, 7.3).x - 7.26));
                // intensities differ between the experiments (scans, gain): normalised to the CHCl3 line, which
                // has the same concentration in every sample; the tracking compares heights from step to step
                TraceOpList operations;
                NormaliseOp normalise(TraceOps::NormaliseMode::Max);
                normalise.useReference = true;
                normalise.refStart = 7.2;
                normalise.refEnd = 7.3;
                operations.append(normalise);
                operations.append(RangeOp(5.25, 9.05));
                PickPeaksOp pick;
                pick.threshold = 2e-5;
                pick.relative = false;
                operations.append(pick);
                handler.setOperations(operations);
                // one signal per resolved line of each multiplet of the first spectrum (the reference reports
                // the multiplet centre = the mean of its lines); window 0.4 x the closest spacing, at most 0.004 ppm
                const XYTrace first = handler.processedSpectra().at(0);
                QVector<NmrSignal> signalList;
                QVector<QVector<int>> multiplets(columns);
                QString lineCount; // lines per multiplet and their offsets from the reference centre [10^-3 ppm]
                for (int c = 0; c < columns; ++c) {
                    const double centre = ref.shifts[0][c];
                    QVector<TracePeak> peaks;
                    TraceOps::pickPeaks(first.range(centre - 0.012, centre + 0.012), 0.0, true, 0, false, &peaks);
                    double highest = 0;
                    for (const TracePeak& p : peaks)
                        highest = std::max(highest, p.height);
                    QVector<double> lines;
                    for (const TracePeak& p : peaks)
                        if (p.height >= 0.42 * highest)
                            lines << p.x;
                    std::sort(lines.begin(), lines.end());
                    double window = 0.004;
                    for (int i = 1; i < lines.size(); ++i)
                        window = std::min(window, kWindowFactor * (lines[i] - lines[i - 1]));
                    lineCount += QString(" [%1]").arg(lines.size());
                    for (int i = 0; i < lines.size(); ++i)
                        lineCount += QString(" %1").arg((lines[i] - centre) * 1000, 0, 'f', 1);
                    for (double x : lines) {
                        multiplets[c] << signalList.size();
                        signalList << NmrSignal{ QString("S%1_%2").arg(c + 1).arg(multiplets[c].size()), x, window, true };
                    }
                }
                handler.setSignals(signalList);
                for (NmrShiftMethod method : { NmrShiftMethod::PeakPicking, NmrShiftMethod::LineshapeFit }) {
                    NmrFitSettings fit;
                    fit.method = method;
                    fit.range = 0.006 + 0.0015 * lb;
                    handler.setFitSettings(fit);
                    timer.restart();
                    handler.track();
                    const qint64 trackMs = timer.elapsed();
                    const QStringList problems = handler.problems();
                    // centre per step vs the reference; changes relative to step 1 as well
                    Eigen::MatrixXd shifts(files.size(), columns);
                    double worst = 0, sum = 0, worstChange = 0, fwhmSum = 0;
                    int n = 0, nFwhm = 0;
                    QString worstCell;
                    for (int k = 0; k < files.size(); ++k)
                        for (int c = 0; c < columns; ++c) {
                            double centre = 0;
                            for (int sIndex : multiplets[c]) {
                                const NmrCell cell = handler.cell(k, sIndex);
                                centre += cell.shift / multiplets[c].size();
                                if (std::isfinite(cell.fwhm)) {
                                    fwhmSum += cell.fwhm;
                                    ++nFwhm;
                                }
                            }
                            shifts(k, c) = centre;
                            if (!std::isfinite(centre))
                                continue;
                            const double d = centre - ref.shifts[k][c];
                            const double dChange = (centre - shifts(0, c)) - (ref.shifts[k][c] - ref.shifts[0][c]);
                            sum += std::abs(d);
                            ++n;
                            if (std::abs(d) > worst) {
                                worst = std::abs(d);
                                worstCell = QString("step %1, signal %2").arg(k + 1).arg(c + 1);
                            }
                            worstChange = std::max(worstChange, std::abs(dChange));
                        }
                    const double fwhm = nFwhm > 0 ? fwhmSum / nFwhm : std::numeric_limits<double>::quiet_NaN();
                    const double lgK = problems.isEmpty() ? fitLgK(ref.concentrations, shifts) : std::numeric_limits<double>::quiet_NaN();
                    qInfo("%s LB %.1f Hz lines %s %s: %d problems; %d cells |δ - reference| mean %.2e ppm, worst %.2e ppm (%s); change from step 1 worst %.2e ppm; mean fitted FWHM %.2e ppm; "
                          "lg K %.4f (reference table refitted %.4f, stored %.4f); CHCl3 spread %.1e ppm; load %lld ms, track %lld ms",
                        qPrintable(key), lb, qPrintable(lineCount), method == NmrShiftMethod::PeakPicking ? "peak picking" : "lineshape fit", int(problems.size()), n, sum / std::max(1, n), worst, qPrintable(worstCell), worstChange, fwhm,
                        lgK, lgKRef, ref.lgK, spread, loadMs, trackMs);
                    if (qEnvironmentVariableIsSet("JEOL_CELLS") && method == NmrShiftMethod::PeakPicking) {
                        qInfo("%s", qPrintable(problems.join("; ")));
                        for (int k = 0; k < files.size(); ++k) {
                            QString row = QString("  %1:").arg(k + 1);
                            for (int c = 0; c < columns; ++c)
                                row += QString(" %1 (%2)").arg(shifts(k, c), 0, 'f', 5).arg((shifts(k, c) - ref.shifts[k][c]) * 1000, 0, 'f', 2);
                            qInfo("%s", qPrintable(row));
                        }
                    }
                    if (lb != kRealLineBroadening || qEnvironmentVariableIsSet("JEOL_EXPLORE"))
                        continue;
                    QVERIFY(handler.fidSettings().autoPhase);
                    QVERIFY2(problems.isEmpty(), qPrintable(problems.join("\n")));
                    QVERIFY2(worst < kPinRealShift, qPrintable(QString::number(worst)));
                    QVERIFY2(std::abs(lgK - lgKRef) < kPinRealLgK, qPrintable(QString::number(lgK - lgKRef)));
                }
            }
        }
    }

    /// Claude Generated: the real series without the helpers realSeriesAgainstReference needs (no Normalise to CHCl3,
    /// no Range, no PickPeaks step: the default picking of the handler), threshold of the whole spectrum vs threshold
    /// of the search window; the same signals (one per resolved line of each multiplet of spectrum 1). Printed per
    /// series: cells tracked, |δ - reference| of the complete multiplet centres, lg K; the window threshold is pinned
    /// (MinHeight 0.3: open cells at most kPinRealWindowOpen; MinHeight 0.1: every cell).
    void realSeriesWindowThreshold()
    {
        const QString dir = realDirectory();
        if (dir.isEmpty() || !QFile::exists(QDir(dir).filePath(kReferenceFile)))
            QSKIP("no JEOL data with the reference evaluation (SUPRAFIT_JEOL_DIR)");
        const QList<double> broadenings = qEnvironmentVariableIsSet("JEOL_EXPLORE") ? QList<double>{ 0.3, 1, 3 } : QList<double>{ kRealLineBroadening };
        for (const QString& key : { QString("T1"), QString("T5") }) {
            const QStringList files = seriesFiles(dir, key == "T1" ? "sb-SB18_T1_b_gluco_" : "sb-SB18_T5_CDCl3_ObDGlc_");
            const Reference ref = readReference(dir, key);
            QCOMPARE(files.size(), ref.shifts.size());
            const int columns = ref.shifts.first().size();
            Eigen::MatrixXd refTable(files.size(), columns);
            for (int k = 0; k < files.size(); ++k)
                for (int c = 0; c < columns; ++c)
                    refTable(k, c) = ref.shifts[k][c];
            const double lgKRef = fitLgK(ref.concentrations, refTable);
            for (double lb : broadenings) {
                NmrHandler handler;
                qint64 loadMs = 0;
                QString error;
                QVERIFY2(loadSeries(files, lb, &handler, &loadMs, &error), qPrintable(error));
                handler.setReferenceOffset(0);
                handler.referenceTo(highestPeak(handler.spectra().at(0), 7.2, 7.3).x, 7.26);
                handler.setOperations(TraceOpList());
                // the same signals as realSeriesAgainstReference, from the referenced spectrum 1 without steps
                const XYTrace first = handler.processedSpectra().at(0);
                QVector<NmrSignal> signalList;
                QVector<QVector<int>> multiplets(columns);
                for (int c = 0; c < columns; ++c) {
                    const double centre = ref.shifts[0][c];
                    QVector<TracePeak> peaks;
                    TraceOps::pickPeaks(first.range(centre - 0.012, centre + 0.012), 0.0, true, 0, false, &peaks);
                    double highest = 0;
                    for (const TracePeak& p : peaks)
                        highest = std::max(highest, p.height);
                    QVector<double> lines;
                    for (const TracePeak& p : peaks)
                        if (p.height >= 0.42 * highest)
                            lines << p.x;
                    std::sort(lines.begin(), lines.end());
                    double window = 0.004;
                    for (int i = 1; i < lines.size(); ++i)
                        window = std::min(window, kWindowFactor * (lines[i] - lines[i - 1]));
                    for (double x : lines) {
                        multiplets[c] << signalList.size();
                        signalList << NmrSignal{ QString("S%1_%2").arg(c + 1).arg(multiplets[c].size()), x, window, true };
                    }
                }
                handler.setSignals(signalList);
                // MinHeight 0.1 as well: without the normalisation the intensity of a line jumps by up to 8x between
                // experiments, more than the default 0.3 of the last height allows
                const QList<QPair<NmrThresholdScope, double>> configurations = { { NmrThresholdScope::Spectrum, 0.3 }, { NmrThresholdScope::Window, 0.3 }, { NmrThresholdScope::Window, 0.1 } };
                for (const auto& [scope, minHeight] : configurations) {
                    handler.setThresholdScope(scope);
                    handler.setMinHeight(minHeight);
                    QCOMPARE(handler.usesWindowThreshold(), scope == NmrThresholdScope::Window);
                    for (NmrShiftMethod method : { NmrShiftMethod::PeakPicking, NmrShiftMethod::LineshapeFit }) {
                        NmrFitSettings fit;
                        fit.method = method;
                        fit.range = 0.006 + 0.0015 * lb;
                        handler.setFitSettings(fit);
                        QElapsedTimer timer;
                        timer.start();
                        handler.track();
                        const qint64 trackMs = timer.elapsed();
                        int okCells = 0, lineCells = 0, complete = 0, nFwhm = 0;
                        double worst = 0, sum = 0, fwhmSum = 0, worstFwhm = 0;
                        QString worstCell;
                        Eigen::MatrixXd shifts(files.size(), columns);
                        for (int k = 0; k < files.size(); ++k)
                            for (int c = 0; c < columns; ++c) {
                                double centre = 0;
                                for (int sIndex : multiplets[c]) {
                                    const NmrCell cell = handler.cell(k, sIndex);
                                    ++lineCells;
                                    okCells += cell.status == NmrCellStatus::Ok;
                                    centre += (cell.status == NmrCellStatus::Ok ? cell.shift : std::numeric_limits<double>::quiet_NaN()) / multiplets[c].size();
                                    if (std::isfinite(cell.fwhm)) {
                                        fwhmSum += cell.fwhm;
                                        ++nFwhm;
                                    }
                                }
                                shifts(k, c) = centre;
                                if (!std::isfinite(centre))
                                    continue;
                                ++complete;
                                const double d = std::abs(centre - ref.shifts[k][c]);
                                sum += d;
                                if (d > worst) {
                                    worst = d;
                                    worstCell = QString("step %1, signal %2").arg(k + 1).arg(c + 1);
                                }
                            }
                        const double fwhm = nFwhm > 0 ? fwhmSum / nFwhm : std::numeric_limits<double>::quiet_NaN();
                        worstFwhm = std::isfinite(fwhm) ? worst / fwhm : std::numeric_limits<double>::quiet_NaN();
                        const bool all = complete == files.size() * columns;
                        const double lgK = all ? fitLgK(ref.concentrations, shifts) : std::numeric_limits<double>::quiet_NaN();
                        const bool window = scope == NmrThresholdScope::Window;
                        qInfo("%s LB %.1f Hz, no steps, %s threshold, MinHeight %.1f, %s: %d of %d line cells tracked, %d of %d multiplet cells complete; |δ - reference| mean %.2e ppm, worst %.2e ppm (%s)%s; lg K %.4f (reference refitted %.4f); track %lld ms",
                            qPrintable(key), lb, window ? "search-window" : "whole-spectrum", minHeight, method == NmrShiftMethod::PeakPicking ? "peak picking" : "lineshape fit", okCells, lineCells, complete,
                            int(files.size() * columns), sum / std::max(1, complete), worst, qPrintable(worstCell),
                            std::isfinite(fwhm) ? qPrintable(QString(" = %1 FWHM (mean fitted FWHM %2 ppm)").arg(worstFwhm, 0, 'f', 2).arg(fwhm, 0, 'e', 2)) : "", lgK, lgKRef, trackMs);
                        if (qEnvironmentVariableIsSet("JEOL_CELLS") && window) {
                            qInfo("%s", qPrintable(handler.problems().mid(0, 20).join("; ")));
                            for (int k = 0; k < files.size(); ++k) {
                                QString row = QString("  %1:").arg(k + 1);
                                for (int c = 0; c < columns; ++c)
                                    row += QString(" %1 (%2)").arg(shifts(k, c), 0, 'f', 5).arg((shifts(k, c) - ref.shifts[k][c]) * 1000, 0, 'f', 2);
                                qInfo("%s", qPrintable(row));
                            }
                        }
                        if (!window || lb != kRealLineBroadening || qEnvironmentVariableIsSet("JEOL_EXPLORE"))
                            continue;
                        const int open = int(files.size() * columns) - complete;
                        QVERIFY2(open <= (minHeight < 0.3 ? 0 : kPinRealWindowOpen), qPrintable(QString("%1 open multiplet cells").arg(open)));
                        QVERIFY2(worst < kPinRealShift, qPrintable(QString::number(worst)));
                        if (all)
                            QVERIFY2(std::abs(lgK - lgKRef) < kPinRealLgK, qPrintable(QString::number(lgK - lgKRef)));
                    }
                }
            }
        }
    }

private:
    // pinned worst cases (measured values are printed by the tests; see docs/jeol_import_2026-11-20.md)
    static constexpr double kPinPositionFwhm1 = 0.4;
    static constexpr double kPinPositionFwhm2 = 0.15;
    static constexpr double kPinPhaseDegree = 7.0;
    static constexpr double kPinRealShift = 0.0025;
    static constexpr double kPinRealLgK = 0.03;
    static constexpr double kRealLineBroadening = 3.0;
    static constexpr double kWindowFactor = 0.9;
    // realSeriesWindowThreshold (Claude Generated), measured 2026-11-20 at LB 3 Hz without steps, window threshold: MinHeight 0.3
    // T1 75/75 multiplet cells (worst 1.7e-3 ppm), T5 44/60 (16 open: intensity jumps between the experiments); MinHeight
    // 0.1 both complete (worst 1.7e-3 / 5.9e-4 ppm, lg K 2.669 / 2.892); the worst shift and lg K use the pins above
    static constexpr int kPinRealWindowOpen = 16;
};

QTEST_GUILESS_MAIN(TestJeol)
#include "test_jeol.moc"
