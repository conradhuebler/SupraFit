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
        // from one in the integration above. ApplyScaling() is explicit because setScalingFactor()
        // only assigns; the re-scale is the caller's job.
        proc.setScalingFactor(cal2joule);
        proc.experiment()->ApplyScaling();
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

        // Change only the scaling factor and re-scale in place (no re-integration), so the peak set
        // is identical and the change isolates the scaling. This mirrors the intended behaviour that
        // changing the cal->J factor must not require re-calibrating/re-integrating.
        proc.setScalingFactor(cal2joule);
        proc.experiment()->ApplyScaling();
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

    // toJson -> fromJson reproduces the injection volumes and the scaling factor.
    void testJsonRoundTrip()
    {
        ItcProcessor proc;
        configure(proc.experiment(), loadSample("reaction.dat"), 20);
        proc.setScalingFactor(cal2joule);
        proc.process();

        QVector<qreal> volumes;
        for (int i = 0; i < proc.netHeat().size(); ++i)
            volumes << 1.0 + i;
        proc.setInjectionVolumes(volumes);

        const QJsonObject json = proc.toJson();

        ItcProcessor restored;
        restored.fromJson(json);

        QCOMPARE(restored.injectionVolumes().size(), volumes.size());
        for (int i = 0; i < volumes.size(); ++i)
            QVERIFY(qAbs(restored.injectionVolumes()[i] - volumes[i]) < 1e-9);

        QVERIFY2(qAbs(restored.experiment()->ScalingFactor() - cal2joule) < 1e-9,
            "scaling factor did not round-trip through JSON");
    }
};

QTEST_MAIN(TestItcThermogram)
#include "test_itc_thermogram.moc"
