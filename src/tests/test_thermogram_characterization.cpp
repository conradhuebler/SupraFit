/*
 * Characterisation test of the ITC thermogram handler (WP5 of roadmap/input_data_editing.md)
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

/* Claude Generated (WP5). Freezes what ThermogramHandler/ItcProcessor computed BEFORE the handler moved
 * onto XYTrace: integrals (raw and scaled), peak indices, baseline coefficients, the chart series
 * (as fingerprints), the stored fit parameters and the net heat, for the samples in data/samples/itc
 * and several integration schemes. The golden file data/samples/itc/thermogram_characterization.json
 * was written by the unchanged implementation (SUPRAFIT_THERMO_GOLDEN_WRITE=1 rewrites it - only do
 * that for a deliberate, explained change of numbers, never to make the test pass).
 *
 * Tolerance: relative 1e-9 (as testAbsoluteIntegralsPinned), since IntegrateNumerical sums under an
 * OpenMP reduction and the last bits scatter between runs.
 *
 * Also checks the raw-data persistence: old project files carry the thermogram with 6 significant
 * digits (QString::number); they must load with the same numbers as before, while files written now
 * carry the raw curve lossless. */

#include <cmath>

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QSignalSpy>
#include <QtTest/QtTest>

#include <Eigen/Dense>

#include "libpeakpick/peakpick.h"

#include "src/core/itcprocessor.h"
#include "src/core/thermogramhandler.h"
#include "src/core/toolset.h"
#include "src/core/trace/tracejson.h"

#include "src/global.h"

#ifndef SAMPLE_DIR
#define SAMPLE_DIR "."
#endif

namespace {

PeakPick::spectrum loadSample(const QString& name)
{
    QPair<Vector, Vector> xy = ToolSet::LoadXYFile(QString(SAMPLE_DIR) + "/" + name);
    return PeakPick::spectrum(xy.first, xy.second);
}

/// one repeating rule over the trace, as in test_itc_thermogram
void configure(ThermogramHandler* handler, const PeakPick::spectrum& spectrum, int peakCount)
{
    const qreal begin = spectrum.XMin();
    const qreal end = spectrum.XMax();
    const qreal duration = (end - begin) / (peakCount + 1);
    QVector<QPointF> rules;
    rules << QPointF(begin + duration, duration);
    handler->setThermogram(spectrum);
    handler->setThermogramBegin(begin);
    handler->setThermogramEnd(end);
    handler->setPeakRules(rules);
}

QJsonArray toArray(const QVector<qreal>& values)
{
    QJsonArray array;
    for (qreal v : values)
        array << v;
    return array;
}

QJsonArray toArray(const Eigen::VectorXd& values)
{
    QJsonArray array;
    for (Eigen::Index i = 0; i < values.size(); ++i)
        array << values(i);
    return array;
}

/// count, sums, sums of squares, first and last three points of a chart series
QJsonObject fingerprint(const QList<QPointF>& points)
{
    double sx = 0, sy = 0, sxx = 0, syy = 0;
    for (const QPointF& p : points) {
        sx += p.x();
        sy += p.y();
        sxx += p.x() * p.x();
        syy += p.y() * p.y();
    }
    QJsonArray head, tail;
    for (int i = 0; i < qMin(3, int(points.size())); ++i)
        head << QJsonArray{ points[i].x(), points[i].y() };
    for (int i = qMax(0, int(points.size()) - 3); i < int(points.size()); ++i)
        tail << QJsonArray{ points[i].x(), points[i].y() };
    return QJsonObject{ { "count", int(points.size()) }, { "sx", sx }, { "sy", sy }, { "sxx", sxx }, { "syy", syy }, { "head", head }, { "tail", tail } };
}

QJsonObject handlerState(const ThermogramHandler* handler)
{
    QJsonObject state;
    state["Integrals"] = toArray(handler->Integrals());
    state["IntegralsScaled"] = toArray(handler->IntegralsScaled());
    state["CalibrationRatio"] = handler->CalibrationRatio();
    state["Calibration"] = handler->Calibration();
    state["ThermogramBegin"] = handler->ThermogramBegin();
    state["ThermogramEnd"] = handler->ThermogramEnd();
    state["IntegrationRangeThreshold"] = handler->IntegrationRangeThreshold();
    state["LastIterations"] = handler->LastIterations();

    QJsonArray peaks;
    for (const PeakPick::Peak& peak : *handler->Peaks())
        peaks << QJsonArray{ int(peak.start), int(peak.max), int(peak.min), int(peak.end), int(peak.int_start), int(peak.int_end), peak.integ_num };
    state["Peaks"] = peaks;

    QJsonArray baselines, grids;
    for (const Vector& b : handler->BaseLine().baselines)
        baselines << toArray(b);
    for (std::size_t i = 0; i < handler->BaseLine().x_grid_points.size(); ++i)
    {
        QList<QPointF> grid;
        const Vector& x = handler->BaseLine().x_grid_points[i];
        const Vector& y = handler->BaseLine().y_grid_points[i];
        for (Eigen::Index j = 0; j < std::min(x.size(), y.size()); ++j)
            grid << QPointF(x(j), y(j));
        grids << fingerprint(grid);
    }
    state["Baselines"] = baselines;
    state["BaselineGridPoints"] = grids;

    state["ThermogramSeries"] = fingerprint(handler->ThermogramSeries());
    state["BaselineSeries"] = fingerprint(handler->BaselineSeries());
    state["BaselineGrid"] = fingerprint(handler->BaselineGrid());
    state["BaselineIgnored"] = fingerprint(handler->BaselineIgnored());

    const QVariant store = qApp->property("StoreRawData");
    qApp->setProperty("StoreRawData", false);
    QJsonObject parameter = handler->getThermogramParameter();
    qApp->setProperty("StoreRawData", store);
    parameter.remove("SupraFit"); // the program version, not a result
    state["Parameter"] = parameter;
    return state;
}

QJsonObject processorState(const ItcProcessor& processor)
{
    QJsonObject state;
    state["experiment"] = handlerState(processor.experiment());
    if (processor.dilutionEnabled())
        state["dilution"] = handlerState(processor.dilution());
    state["NetHeat"] = toArray(processor.netHeat());
    return state;
}

/// the fit block as SupraFit wrote it before WP5: raw curve with QString::number (6 significant digits)
QJsonObject oldFormatBlock(const ThermogramHandler* handler)
{
    const QVariant store = qApp->property("StoreRawData");
    qApp->setProperty("StoreRawData", false);
    QJsonObject fit = handler->getThermogramParameter();
    qApp->setProperty("StoreRawData", store);
    QJsonObject thermo;
    thermo["x"] = ToolSet::DoubleList2String(handler->Spectrum()->x());
    thermo["y"] = ToolSet::DoubleList2String(handler->Spectrum()->y());
    fit["Thermogram"] = thermo;
    return fit;
}

/// the same block with the raw curve lossless (TraceJson)
QJsonObject losslessBlock(const ThermogramHandler* handler)
{
    QJsonObject fit = oldFormatBlock(handler);
    QJsonObject thermo;
    thermo["x"] = TraceJson::vectorToString(handler->Spectrum()->x());
    thermo["y"] = TraceJson::vectorToString(handler->Spectrum()->y());
    fit["Thermogram"] = thermo;
    return fit;
}

QJsonObject reloadState(const QJsonObject& fit)
{
    QJsonObject experiment;
    experiment["fit"] = fit;
    ItcProcessor processor;
    processor.fromJson(QJsonObject{ { "experiment", experiment } });
    processor.process();
    return processorState(processor);
}

/// all cases, computed by the current implementation
QJsonObject computeAll()
{
    QJsonObject cases;
    QElapsedTimer timer;
    timer.start();
    auto mark = [&timer](const char* name) { qInfo("%s: %lld ms", name, timer.restart()); };
    const PeakPick::spectrum reaction = loadSample("reaction.dat");
    const PeakPick::spectrum dilution = loadSample("dilution.dat");

    {
        ItcProcessor p;
        configure(p.experiment(), reaction, 20);
        p.setScalingFactor(1.0);
        p.process();
        cases["reaction_custom"] = processorState(p);
        mark("reaction_custom");
        cases["reload_old_format"] = reloadState(oldFormatBlock(p.experiment()));
        mark("reload_old_format");
        cases["reload_lossless"] = reloadState(losslessBlock(p.experiment()));
        mark("reload_lossless");
    }
    {
        ItcProcessor p;
        configure(p.experiment(), dilution, 20);
        p.setScalingFactor(1.0);
        p.process();
        cases["dilution_custom"] = processorState(p);
        mark("dilution_custom");
    }
    for (bool averaged : { true, false }) {
        ItcProcessor p;
        configure(p.experiment(), reaction, 20);
        p.experiment()->setCurrentCutOption("Zero");
        p.experiment()->setAveraged(averaged);
        p.setScalingFactor(1.0);
        p.process();
        cases[averaged ? "reaction_zero_averaged" : "reaction_zero"] = processorState(p);
        mark(averaged ? "reaction_zero_averaged" : "reaction_zero");
    }
    {
        ItcProcessor p;
        configure(p.experiment(), reaction, 20);
        p.experiment()->setCurrentCutOption("Threshold");
        p.experiment()->setMaximalIterations(15);
        p.experiment()->setOvershotCounter(1);
        p.setScalingFactor(1.0);
        p.process();
        cases["reaction_threshold"] = processorState(p);
        mark("reaction_threshold");
    }
    {
        ItcProcessor p;
        configure(p.experiment(), reaction, 20);
        p.experiment()->setCalibrationHeat(0.01);
        p.experiment()->setCalibrationStart(300);
        p.experiment()->setConstantOffset(1e-5);
        p.setScalingFactor(cal2joule);
        p.process();
        cases["reaction_calibration"] = processorState(p);
        mark("reaction_calibration");
    }
    {
        ItcProcessor p;
        configure(p.experiment(), reaction, 20);
        configure(p.dilution(), dilution, 20);
        p.setDilutionEnabled(true);
        p.setScalingFactor(cal2joule);
        p.process();
        cases["reaction_minus_dilution"] = processorState(p);
        mark("reaction_minus_dilution");
    }
    {
        QString file = QString(SAMPLE_DIR) + "/synthetic.itc";
        std::vector<PeakPick::Peak> peaks;
        QVector<qreal> inject;
        qreal offset = 0, freq = 0;
        QPair<PeakPick::spectrum, QJsonObject> pair = ToolSet::LoadITCFile(file, &peaks, offset, freq, inject);
        ItcProcessor p;
        p.experiment()->setThermogram(pair.first);
        p.experiment()->setPeakList(peaks);
        p.setInjectionVolumes(inject);
        p.setScalingFactor(1.0);
        // the sequence of the import dialog; process() would run UpdatePeaks(), whose XtoIndex() never
        // returns for a rule within the first four samples of this 12-point trace (Track B2)
        p.experiment()->Initialise();
        p.experiment()->IntegrateThermogram();
        cases["synthetic_itc"] = processorState(p);
        mark("synthetic_itc");
    }
    return cases;
}

/// recursive comparison; numbers with relative tolerance
void compare(const QJsonValue& want, const QJsonValue& got, const QString& path, QStringList* errors)
{
    if (errors->size() > 20)
        return;
    if (want.type() != got.type()) {
        *errors << QString("%1: type %2 vs %3").arg(path).arg(want.type()).arg(got.type());
        return;
    }
    if (want.isDouble()) {
        const double a = want.toDouble(), b = got.toDouble();
        if (!(std::abs(a - b) <= 1e-9 * std::max(std::abs(a), std::abs(b)) + 1e-300))
            *errors << QString("%1: %2 vs %3").arg(path).arg(a, 0, 'g', 17).arg(b, 0, 'g', 17);
    } else if (want.isArray()) {
        const QJsonArray wa = want.toArray(), ga = got.toArray();
        if (wa.size() != ga.size()) {
            *errors << QString("%1: length %2 vs %3").arg(path).arg(wa.size()).arg(ga.size());
            return;
        }
        for (int i = 0; i < wa.size(); ++i)
            compare(wa[i], ga[i], QString("%1[%2]").arg(path).arg(i), errors);
    } else if (want.isObject()) {
        const QJsonObject wo = want.toObject(), go = got.toObject();
        if (wo.keys() != go.keys())
            *errors << QString("%1: keys %2 vs %3").arg(path, wo.keys().join(","), go.keys().join(","));
        for (const QString& key : wo.keys())
            if (go.contains(key))
                compare(wo[key], go[key], path + "." + key, errors);
    } else if (want != got) {
        *errors << QString("%1: %2 vs %3").arg(path, want.toVariant().toString(), got.toVariant().toString());
    }
}

QString goldenPath() { return QString(SAMPLE_DIR) + "/thermogram_characterization.json"; }

} // namespace

class TestThermogramCharacterization : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qApp->setProperty("StoreRawData", true);
    }

    /*! \brief Every case reproduces the numbers of the implementation before WP5. */
    void testAgainstGolden()
    {
        const QJsonObject now = computeAll();
        if (qEnvironmentVariableIsSet("SUPRAFIT_THERMO_GOLDEN_WRITE")) {
            QFile file(goldenPath());
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
            file.write(QJsonDocument(now).toJson(QJsonDocument::Compact));
            QSKIP("golden file written");
        }
        QFile file(goldenPath());
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(goldenPath()));
        const QJsonObject golden = QJsonDocument::fromJson(file.readAll()).object();
        QVERIFY(!golden.isEmpty());

        QStringList errors;
        compare(golden, now, "cases", &errors);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join("\n")));
    }

    /*! \brief A block written now carries the raw curve bit for bit, and loading it gives the numbers
     * the lossless reload gave before WP5 (golden "reload_lossless"). Old 6-digit blocks are covered by
     * the golden case "reload_old_format". */
    void testWrittenRawDataIsLossless()
    {
        ItcProcessor p;
        configure(p.experiment(), loadSample("reaction.dat"), 20);
        p.setScalingFactor(1.0);
        p.process();

        const QJsonObject fit = p.experiment()->getThermogramParameter();
        QVERIFY(fit.contains("Thermogram"));
        Eigen::VectorXd x, y;
        QVERIFY(TraceJson::vectorFromJson(fit["Thermogram"].toObject()["x"], &x));
        QVERIFY(TraceJson::vectorFromJson(fit["Thermogram"].toObject()["y"], &y));
        QVERIFY(x == p.experiment()->Trace().x());
        QVERIFY(y == p.experiment()->Trace().y());
        // older SupraFit reads the string with ToolSet::String2DoubleEigVec - same doubles
        QVERIFY(ToolSet::String2DoubleEigVec(fit["Thermogram"].toObject()["y"].toString()) == p.experiment()->Trace().y());

        QFile file(goldenPath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonObject golden = QJsonDocument::fromJson(file.readAll()).object();
        QStringList errors;
        compare(golden["reload_lossless"], reloadState(fit), "reload_written", &errors);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join("\n")));
    }

    /*! \brief setStoreRawData() overrides the application property; unset, the property decides. */
    void testStoreRawDataSwitch()
    {
        ThermogramHandler handler;
        handler.setThermogram(XYTrace(Eigen::VectorXd::LinSpaced(10, 0, 9), Eigen::VectorXd::Ones(10)));
        qApp->setProperty("StoreRawData", false);
        QVERIFY(!handler.getThermogramParameter().contains("Thermogram"));
        qApp->setProperty("StoreRawData", true);
        QVERIFY(handler.getThermogramParameter().contains("Thermogram"));
        handler.setStoreRawData(false);
        QVERIFY(!handler.getThermogramParameter().contains("Thermogram"));
        qApp->setProperty("StoreRawData", false);
        handler.setStoreRawData(true);
        QVERIFY(handler.getThermogramParameter().contains("Thermogram"));
        qApp->setProperty("StoreRawData", true);
    }

    /*! \brief x and y of different lengths are refused with a message, not an exception. */
    void testMismatchedTraceRefused()
    {
        ThermogramHandler handler;
        QSignalSpy spy(&handler, &ThermogramHandler::Message);
        handler.setThermogram(XYTrace(Eigen::VectorXd::LinSpaced(5, 0, 4), Eigen::VectorXd::Zero(4)));
        QCOMPARE(spy.count(), 1);
        QVERIFY(handler.Trace().isEmpty());
        QCOMPARE(int(handler.Spectrum()->size()), 0);
    }
};

QTEST_MAIN(TestThermogramCharacterization)
#include "test_thermogram_characterization.moc"
