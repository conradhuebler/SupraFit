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
