/*
 * Headless test for the ITC thermogram import pipeline (ItcProcessor)
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

// Proves that the full ITC thermogram → (volume, net heat) pipeline works WITHOUT a GUI, driven
// only by the core ItcProcessor + ThermogramHandler on the sample thermograms in data/samples/itc.

#include <cmath>

#include <QtCore/QJsonObject>
#include <QtTest/QSignalSpy>
#include <QtTest/QtTest>

#include <Eigen/Dense>

#include "libpeakpick/peakpick.h"

#include "src/core/itcprocessor.h"
#include "src/core/models/datatable.h"
#include "src/core/thermogramhandler.h"
#include "src/core/toolset.h"

#include "src/global.h"

#ifndef SAMPLE_DIR
#define SAMPLE_DIR "."
#endif

class TestItcThermogram : public QObject {
    Q_OBJECT

    static PeakPick::spectrum loadSample(const QString& name)
    {
        QPair<Vector, Vector> xy = ToolSet::LoadXYFile(QString(SAMPLE_DIR) + "/" + name);
        return PeakPick::spectrum(xy.first, xy.second);
    }

    // Configure one handler from a thermogram with a single repeating peak rule that yields
    // roughly peakCount peaks across the trace (exact count is irrelevant to the invariants below).
    static void configure(ThermogramHandler* handler, const PeakPick::spectrum& spectrum, int peakCount)
    {
        const qreal begin = spectrum.XMin();
        const qreal end = spectrum.XMax();
        const qreal duration = (end - begin) / (peakCount + 1);

        QVector<QPointF> rules;
        rules << QPointF(begin + duration, duration); // start after a lead-in; one repeating rule

        handler->setThermogram(spectrum);
        handler->setThermogramBegin(begin);
        handler->setThermogramEnd(end);
        handler->setPeakRules(rules);
    }

private slots:

    /*! \brief Pins the ABSOLUTE peak integrals to their measured values. Claude Generated
     *
     * The other tests here are relative invariants: testScalingLinearity compares a ratio and
     * testDilutionSubtractsToZero subtracts a trace from itself. Both stay green under a uniform
     * bias in the integration, so neither would notice if the numerical integration changed. This
     * test is the one that would: it fixes what the pipeline actually computes on a real thermogram.
     *
     * Any change to the integration - in SupraFit or in libpeakpick underneath it - either fixes a
     * defect or introduces one, and both must be visible. If this test fails, do not adjust the
     * numbers to make it pass: establish which of the two happened first, then replace them in a
     * commit that says why they moved.
     *
     * The values were MEASURED against data/samples/itc/reaction.dat (2026-07-15), not derived, so
     * they pin behaviour rather than correctness. Tolerance is relative 1e-9: repeated runs differ
     * in the last one or two bits (~1e-16 relative) because IntegrateNumerical sums under an OpenMP
     * reduction and floating-point addition is not associative, so the thread scheduling reorders
     * it. 1e-9 sits far above that noise and far below any change worth catching. */
    void testAbsoluteIntegralsPinned()
    {
        // Unscaled peak integrals: the fingerprint of the numerical integration itself.
        static const QVector<qreal> expected_raw = {
            0.0030043654955833332, 0.00031275316342521583, -0.0011679504443039279, 0.0031370380013509488,
            0.006761693551501826, 0.0065824939150939355, 0.0063003840125368527, 0.0063711045214132527,
            0.0050663794604497506, 0.0013295367971036776, 0.00069578919918931461, 0.00044051125459215875,
            0.00050554440436924135, 0.0004927183499344645, 0.00045780110185959615, 0.00027712997505625517,
            0.00022831850569511629, -9.9031318189881149e-06, -0.015729648987413823, 0.031507749825159762
        };

        ItcProcessor proc;
        configure(proc.experiment(), loadSample("reaction.dat"), 20);
        proc.setScalingFactor(1.0);
        proc.process();

        const QVector<qreal> raw = proc.experiment()->Integrals();
        QCOMPARE(raw.size(), expected_raw.size());
        for (int i = 0; i < expected_raw.size(); ++i) {
            QVERIFY2(qAbs(raw[i] - expected_raw[i]) <= 1e-9 * qAbs(expected_raw[i]),
                qPrintable(QString("peak %1 integral moved: expected %2, got %3")
                               .arg(i)
                               .arg(expected_raw[i], 0, 'g', 17)
                               .arg(raw[i], 0, 'g', 17)));
        }

        // Same run through the scaled path, so a regression in the cal->J application is separable
        // from one in the integration above.
        proc.setScalingFactor(cal2joule);
        const QVector<qreal> net = proc.netHeat();
        QCOMPARE(net.size(), expected_raw.size());
        for (int i = 0; i < expected_raw.size(); ++i) {
            const qreal want = expected_raw[i] * cal2joule;
            QVERIFY2(qAbs(net[i] - want) <= 1e-9 * qAbs(want),
                qPrintable(QString("peak %1 net heat moved: expected %2, got %3")
                               .arg(i)
                               .arg(want, 0, 'g', 17)
                               .arg(net[i], 0, 'g', 17)));
        }
    }

    // The pipeline runs headless and produces a (volume, net heat) table.
    void testHeadlessRunProducesResult()
    {
        ItcProcessor proc;
        configure(proc.experiment(), loadSample("reaction.dat"), 20);
        proc.setScalingFactor(1.0);
        proc.process();

        const QVector<qreal> net = proc.netHeat();
        QVERIFY2(net.size() > 0, "no injections integrated headless");
        for (qreal h : net)
            QVERIFY(std::isfinite(h));

        // Per-injection volumes flow through to the result table's first column.
        QVector<qreal> volumes;
        for (int i = 0; i < net.size(); ++i)
            volumes << (i + 1) * 0.5;
        proc.setInjectionVolumes(volumes);

        DataTable* table = proc.resultTable();
        QCOMPARE(table->rowCount(), net.size());
        QCOMPARE(table->columnCount(), 2);
        for (int i = 0; i < net.size(); ++i) {
            QCOMPARE(table->data(i, 0), volumes[i]);
            QCOMPARE(table->data(i, 1), net[i]);
        }
        delete table;
    }

    // The net heat scales linearly with the scaling factor (cal->J), calibration ratio unchanged.
    void testScalingLinearity()
    {
        const PeakPick::spectrum exp = loadSample("reaction.dat");

        ItcProcessor proc;
        configure(proc.experiment(), exp, 20);
        proc.setScalingFactor(1.0);
        proc.process();
        const QVector<qreal> net1 = proc.netHeat();

        // Change only the scaling factor: it re-scales in place without re-integrating, so the peak
        // set is identical and the change isolates the scaling. Changing cal->J must never force a
        // re-calibration.
        proc.setScalingFactor(cal2joule);
        const QVector<qreal> net2 = proc.netHeat();

        QCOMPARE(net2.size(), net1.size());
        int compared = 0;
        for (int i = 0; i < net1.size(); ++i) {
            if (qAbs(net1[i]) < 1e-12)
                continue;
            QVERIFY2(qAbs(net2[i] / net1[i] - cal2joule) < 1e-6, "scaling is not linear in the factor");
            ++compared;
        }
        QVERIFY2(compared > 0, "no non-zero injections to compare scaling");
    }

    // Subtracting a dilution equal to the experiment leaves ~zero net heat everywhere.
    void testDilutionSubtractsToZero()
    {
        const PeakPick::spectrum spectrum = loadSample("reaction.dat");

        ItcProcessor proc;
        configure(proc.experiment(), spectrum, 20);
        configure(proc.dilution(), spectrum, 20);
        proc.setDilutionEnabled(true);
        proc.setScalingFactor(cal2joule);
        proc.process();

        const QVector<qreal> net = proc.netHeat();
        QVERIFY(net.size() > 0);
        for (qreal h : net)
            QVERIFY2(qAbs(h) < 1e-9, "experiment minus identical dilution should be ~0");
    }

    // One volume for every injection, sized to the peak list, and it reaches the result table.
    void testUniformInjectionVolume()
    {
        ItcProcessor proc;
        configure(proc.experiment(), loadSample("reaction.dat"), 20);
        proc.setScalingFactor(1.0);
        proc.process();

        proc.setUniformInjectionVolume(1.5);
        QCOMPARE(proc.injectionVolumes().size(), proc.injectionCount());
        QVERIFY(proc.injectionCount() > 0);
        for (qreal v : proc.injectionVolumes())
            QCOMPARE(v, 1.5);

        QScopedPointer<DataTable> table(proc.resultTable());
        for (int i = 0; i < table->rowCount(); ++i)
            QCOMPARE(table->data(i, 0), 1.5);
    }

    // Padding fills the gap without touching - or truncating - what is already known.
    void testPadInjectionVolumesKeepsKnownEntries()
    {
        ItcProcessor proc;
        configure(proc.experiment(), loadSample("reaction.dat"), 20);
        proc.setScalingFactor(1.0);
        proc.process();
        QVERIFY2(proc.injectionCount() > 3, "sample must give more peaks than the seeded volumes");

        proc.setInjectionVolumes(QVector<qreal>() << 0.5 << 0.5 << 0.5);
        proc.padInjectionVolumes(2.0);

        QCOMPARE(proc.injectionVolumes().size(), proc.injectionCount());
        for (int i = 0; i < 3; ++i)
            QCOMPARE(proc.injectionVolumes()[i], 0.5); // known volumes survive
        for (int i = 3; i < proc.injectionCount(); ++i)
            QCOMPARE(proc.injectionVolumes()[i], 2.0);

        // Already full: padding again must not append or overwrite.
        proc.padInjectionVolumes(9.0);
        QCOMPARE(proc.injectionVolumes().size(), proc.injectionCount());
        QCOMPARE(proc.injectionVolumes()[3], 2.0);

        // A vector longer than the peak list is never shortened - a temporary drop in the peak
        // count must not destroy volumes the user typed.
        QVector<qreal> longer(proc.injectionCount() + 5, 0.25);
        proc.setInjectionVolumes(longer);
        proc.padInjectionVolumes(1.0);
        QCOMPARE(proc.injectionVolumes().size(), longer.size());
    }

    // A single volume can be set by index, and the vector grows with zeros to reach it.
    void testSingleInjectionVolume()
    {
        ItcProcessor proc;
        proc.setInjectionVolume(2, 0.75);
        QCOMPARE(proc.injectionVolumes().size(), 3);
        QCOMPARE(proc.injectionVolumes()[0], 0.0);
        QCOMPARE(proc.injectionVolumes()[1], 0.0);
        QCOMPARE(proc.injectionVolumes()[2], 0.75);

        proc.setInjectionVolume(0, 0.25); // overwrite in place, no growth
        QCOMPARE(proc.injectionVolumes().size(), 3);
        QCOMPARE(proc.injectionVolumes()[0], 0.25);

        proc.setInjectionVolume(-1, 5.0); // nonsense index is ignored, not crashed on
        QCOMPARE(proc.injectionVolumes().size(), 3);
    }

    // setScalingFactor drives BOTH handlers and re-scales them, without the caller chaining
    // ApplyScaling() itself; re-setting the same factor is a no-op.
    void testCoupledScaling()
    {
        const PeakPick::spectrum spectrum = loadSample("reaction.dat");

        ItcProcessor proc;
        configure(proc.experiment(), spectrum, 20);
        configure(proc.dilution(), spectrum, 20);
        proc.setDilutionEnabled(true);
        proc.setScalingFactor(1.0);
        proc.process();

        const QVector<qreal> exp1 = proc.experiment()->IntegralsScaled();
        const QVector<qreal> dil1 = proc.dilution()->IntegralsScaled();
        QVERIFY(exp1.size() > 0 && dil1.size() > 0);

        QSignalSpy spy(&proc, &ItcProcessor::resultChanged);
        proc.setScalingFactor(2.0); // deliberately no ApplyScaling() call here

        QCOMPARE(proc.scalingFactor(), 2.0);
        QCOMPARE(proc.experiment()->ScalingFactor(), 2.0);
        QCOMPARE(proc.dilution()->ScalingFactor(), 2.0);
        QVERIFY2(spy.count() >= 1, "re-scaling did not announce a new result");

        const QVector<qreal> exp2 = proc.experiment()->IntegralsScaled();
        const QVector<qreal> dil2 = proc.dilution()->IntegralsScaled();
        for (int i = 0; i < exp1.size(); ++i)
            QVERIFY2(qAbs(exp2[i] - 2.0 * exp1[i]) < 1e-9, "experiment was not re-scaled");
        for (int i = 0; i < dil1.size(); ++i)
            QVERIFY2(qAbs(dil2[i] - 2.0 * dil1[i]) < 1e-9, "dilution was not re-scaled");

        spy.clear();
        proc.setScalingFactor(2.0);
        QCOMPARE(spy.count(), 0); // idempotent: this is what stops two bound UI controls looping
    }

    /*! \brief Flipping the dilution gate re-joins an already integrated dilution. Claude Generated
     *
     * The regression guard for "the subtraction silently stops happening". The gate is an input to
     * the join, so a caller that flips it must not also have to know it needs to re-process. Note
     * the gate selects whether an integrated dilution participates - it does not integrate one; the
     * dilution is enabled before process() here for exactly that reason. */
    void testDilutionGatingRecomputes()
    {
        const PeakPick::spectrum spectrum = loadSample("reaction.dat");

        ItcProcessor proc;
        configure(proc.experiment(), spectrum, 20);
        configure(proc.dilution(), spectrum, 20);
        proc.setDilutionEnabled(true);
        proc.setScalingFactor(cal2joule);
        proc.process(); // integrates both handlers

        for (qreal h : proc.netHeat())
            QVERIFY2(qAbs(h) < 1e-9, "identical dilution should cancel the experiment out");

        QSignalSpy spy(&proc, &ItcProcessor::resultChanged);

        proc.setDilutionEnabled(false); // the join must stop, without re-processing
        QCOMPARE(spy.count(), 1);
        const QVector<qreal> without = proc.netHeat();
        QCOMPARE(without.size(), proc.experiment()->IntegralsScaled().size());
        bool any_nonzero = false;
        for (int i = 0; i < without.size(); ++i) {
            QVERIFY2(qAbs(without[i] - proc.experiment()->IntegralsScaled()[i]) < 1e-12,
                "disabling the dilution did not fall back to the experiment alone");
            any_nonzero = any_nonzero || qAbs(without[i]) > 1e-9;
        }
        QVERIFY2(any_nonzero, "experiment-only net heat should not be all zero");

        proc.setDilutionEnabled(true); // and start again
        QCOMPARE(spy.count(), 2);
        for (qreal h : proc.netHeat())
            QVERIFY2(qAbs(h) < 1e-9, "re-enabling the dilution did not restore the subtraction");

        spy.clear();
        proc.setDilutionEnabled(true);
        QCOMPARE(spy.count(), 0); // no change, no signal
    }

    // Legacy projects stored one "injectvolume" scalar; it spreads over the stored peak count.
    void testJsonLegacyScalarFallback()
    {
        QJsonObject fit;
        fit["PeakCount"] = 5;
        QJsonObject experiment;
        experiment["fit"] = fit;

        QJsonObject legacy;
        legacy["experiment"] = experiment;
        legacy["injectvolume"] = "0.5";

        ItcProcessor proc;
        proc.fromJson(legacy);
        QCOMPARE(proc.injectionVolumes().size(), 5);
        for (qreal v : proc.injectionVolumes())
            QCOMPARE(v, 0.5);

        // Comma decimals, as a German locale would have written them.
        legacy["injectvolume"] = "0,5";
        ItcProcessor comma;
        comma.fromJson(legacy);
        QCOMPARE(comma.injectionVolumes().size(), 5);
        for (qreal v : comma.injectionVolumes())
            QCOMPARE(v, 0.5);

        // No peak count to spread over, and unparsable text: empty, not a crash.
        QJsonObject broken;
        broken["experiment"] = QJsonObject();
        broken["injectvolume"] = "0.5";
        ItcProcessor no_count;
        no_count.fromJson(broken);
        QVERIFY(no_count.injectionVolumes().isEmpty());

        legacy["injectvolume"] = "not a number";
        ItcProcessor garbage;
        garbage.fromJson(legacy);
        QVERIFY(garbage.injectionVolumes().isEmpty());
    }

    // An old project re-saved by the current code carries both keys; the real vector must win.
    void testJsonNewVectorWins()
    {
        QJsonObject fit;
        fit["PeakCount"] = 3;
        QJsonObject experiment;
        experiment["fit"] = fit;

        QJsonObject both;
        both["experiment"] = experiment;
        both["injectvolume"] = "0.5"; // legacy scalar
        both["InjectVolume"] = "1 2 3"; // per-injection vector

        ItcProcessor proc;
        proc.fromJson(both);
        QCOMPARE(proc.injectionVolumes().size(), 3);
        QCOMPARE(proc.injectionVolumes()[0], 1.0);
        QCOMPARE(proc.injectionVolumes()[1], 2.0);
        QCOMPARE(proc.injectionVolumes()[2], 3.0);
    }

    // toJson -> fromJson reproduces the injection volumes and the scaling factor.
    void testJsonRoundTrip()
    {
        const PeakPick::spectrum spectrum = loadSample("reaction.dat");

        ItcProcessor proc;
        configure(proc.experiment(), spectrum, 20);
        configure(proc.dilution(), spectrum, 20);
        proc.setDilutionEnabled(true);
        proc.setScalingFactor(cal2joule);
        proc.process();

        QVector<qreal> volumes;
        for (int i = 0; i < proc.netHeat().size(); ++i)
            volumes << 1.0 + i; // deliberately all different: a scalar could not carry this
        proc.setInjectionVolumes(volumes);

        const QJsonObject json = proc.toJson();

        ItcProcessor restored;
        restored.fromJson(json);

        QCOMPARE(restored.injectionVolumes().size(), volumes.size());
        for (int i = 0; i < volumes.size(); ++i)
            QVERIFY(qAbs(restored.injectionVolumes()[i] - volumes[i]) < 1e-9);

        QVERIFY2(qAbs(restored.experiment()->ScalingFactor() - cal2joule) < 1e-9,
            "scaling factor did not round-trip through JSON");

        QVERIFY2(restored.dilutionEnabled(), "the dilution gate did not round-trip");
        QVERIFY2(qAbs(restored.dilution()->ScalingFactor() - cal2joule) < 1e-9,
            "the dilution's scaling factor did not round-trip");
    }
};

QTEST_MAIN(TestItcThermogram)
#include "test_itc_thermogram.moc"
