/*
 * SupraFit - model-level test for nmr_any with self-aggregation (BFGS speciation)
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * nmr_any (model 34) is not covered by the reference-project regression, so this test exercises
 * it directly: the classic A_aB_b grid path (cross-checked against the analytic 1:1 root) and the
 * new self-aggregation path A2 <=> 2A / A + B <=> AB (mass-balance closure). Claude Generated.
 */

#include <cmath>

#include <QtTest/QtTest>
#include <QtCore/QJsonObject>

#include <Eigen/Dense>

#include "src/core/equil.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/models/titrations/AbstractNMRModel.h"
#include "src/global.h"

class TestNmrSelfAggregation : public QObject {
    Q_OBJECT

private:
    static constexpr int N = 12;
    static constexpr double A0 = 1e-3; // host total, held constant along the titration

    // Build a DataClass with a host-constant / guest-titrated independent table and one
    // (simulated) dependent series. Column 0 = host (A), column 1 = guest (B).
    static DataClass* makeData()
    {
        Eigen::MatrixXd indep(N, 2);
        Eigen::MatrixXd dep(N, 1);
        for (int i = 0; i < N; ++i) {
            indep(i, 0) = A0;
            indep(i, 1) = 2e-3 * i / (N - 1);
            dep(i, 0) = 0.0;
        }
        DataClass* data = new DataClass();
        data->setIndependentTable(new DataTable(indep));
        data->setDataType(DataClassPrivate::Table);
        data->setSimulateDependent(1);
        data->setDependentTable(new DataTable(dep));
        // the plain setDependentTable() does not initialise the active point range, so do it here
        data->setDataBegin(0);
        data->setDataEnd(N);
        return data;
    }

    static QJsonObject strOption(const QString& value)
    {
        QJsonObject o;
        o["value"] = value;
        return o;
    }

    static double relError(double got, double expected)
    {
        return std::abs(got - expected) / std::max(std::abs(expected), 1e-30);
    }

private slots:
    /** 1:1 system {AB} (A + B <=> AB): free host must match the analytic 1:1 quadratic root. */
    void testGridMatchesAnalytic()
    {
        DataClass* data = makeData();
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
        QVERIFY(!model.isNull());

        QJsonObject def;
        def["Reactions"] = strOption("A + B <=> AB");
        QVERIFY(model->DefineModel(def));

        QCOMPARE(model->GlobalParameterSize(), 1); // only AB

        model->InitialGuess(); // allocate/size the local (shift) tables consistently
        model->setFast(false); // store per-datapoint species concentrations (getConcentration)
        const double logK = 4.0;
        model->setGlobalParameter(logK, 0);
        model->Calculate();

        AbstractNMRModel* tm = qobject_cast<AbstractNMRModel*>(model.data());
        QVERIFY(tm);
        for (int i = 0; i < N; ++i) {
            Vector c = tm->getConcentration(i);
            QVERIFY(c.size() >= 4);
            const double freeA = c(1);
            const double freeB = c(2);
            const double B0 = 2e-3 * i / (N - 1);

            const double freeA_ref = ItoI::HostConcentration(A0, B0, logK);
            QVERIFY2(relError(freeA, freeA_ref) < 1e-5,
                qPrintable(QString("point %1: free A %2 vs analytic %3").arg(i).arg(freeA).arg(freeA_ref)));

            // mass balance A0 = freeA + [AB]; c(3) holds a*[AB] = [AB] (a = 1)
            QVERIFY(relError(freeA + c(3), A0) < 1e-8);
            // mass balance B0 = freeB + [AB]
            if (B0 > 0)
                QVERIFY(relError(freeB + c(3), B0) < 1e-6);
        }
        delete data;
    }

    /** Self-aggregation {AB, A2} (preceding dimerisation): mass balance closes and A2 forms. */
    void testSelfAggregationMassBalance()
    {
        DataClass* data = makeData();
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
        QVERIFY(!model.isNull());

        QJsonObject def;
        def["Reactions"] = strOption("A + B <=> AB\n2 A <=> A2");
        QVERIFY(model->DefineModel(def));

        QCOMPARE(model->GlobalParameterSize(), 2); // AB (index 0) and A2 (index 1)

        model->InitialGuess(); // allocate/size the local (shift) tables consistently
        model->setFast(false); // store per-datapoint species concentrations (getConcentration)
        model->setGlobalParameter(4.0, 0); // lg beta(AB)
        model->setGlobalParameter(5.0, 1); // lg beta(A2)  -> strong dimerisation
        model->Calculate();

        AbstractNMRModel* tm = qobject_cast<AbstractNMRModel*>(model.data());
        QVERIFY(tm);

        double maxDimerFraction = 0.0;
        for (int i = 0; i < N; ++i) {
            Vector c = tm->getConcentration(i);
            QVERIFY(c.size() >= 5);
            const double freeA = c(1);
            const double boundA_AB = c(3); // 1 * [AB]
            const double boundA_A2 = c(4); // 2 * [A2]

            // host mass balance: free A + A in AB + A in A2 = A0 (constant)
            QVERIFY2(relError(freeA + boundA_AB + boundA_A2, A0) < 1e-7,
                qPrintable(QString("point %1: A balance %2 vs %3")
                        .arg(i).arg(freeA + boundA_AB + boundA_A2).arg(A0)));

            maxDimerFraction = std::max(maxDimerFraction, boundA_A2 / A0);
        }
        // with lg beta(A2) = 5 the dimer must carry a large share of the host
        QVERIFY2(maxDimerFraction > 0.5,
            qPrintable(QString("max dimer fraction only %1").arg(maxDimerFraction)));
        delete data;
    }

    /** Explicit species list "2,0|1,1" defines exactly {A2, AB} with correct names and balance. */
    void testExplicitSpeciesList()
    {
        DataClass* data = makeData();
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
        QVERIFY(!model.isNull());

        QJsonObject def;
        def["Reactions"] = strOption("2 A <=> A2\nA + B <=> AB"); // {A2, AB} - order as written
        QVERIFY(model->DefineModel(def));

        QCOMPARE(model->GlobalParameterSize(), 2);

        AbstractNMRModel* tm = qobject_cast<AbstractNMRModel*>(model.data());
        QVERIFY(tm);
        QCOMPARE(tm->SpeciesName(0), QString::fromUtf8("A₂")); // A2 (index 0)
        QCOMPARE(tm->SpeciesName(1), QString("AB")); // AB (index 1)

        model->InitialGuess();
        model->setFast(false);
        model->setGlobalParameter(5.0, 0); // lg beta(A2)
        model->setGlobalParameter(4.0, 1); // lg beta(AB)
        model->Calculate();

        for (int i = 0; i < N; ++i) {
            Vector c = tm->getConcentration(i);
            QVERIFY(c.size() >= 5);
            const double freeA = c(1);
            const double boundA_A2 = c(3); // 2 * [A2]  (species index 0 -> column 3)
            const double boundA_AB = c(4); // 1 * [AB]  (species index 1 -> column 4)
            QVERIFY2(relError(freeA + boundA_A2 + boundA_AB, A0) < 1e-7,
                qPrintable(QString("point %1: A balance %2 vs %3")
                        .arg(i).arg(freeA + boundA_A2 + boundA_AB).arg(A0)));
        }
        delete data;
    }
};

QTEST_MAIN(TestNmrSelfAggregation)
#include "test_nmr_selfaggregation.moc"
