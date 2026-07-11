/*
 * SupraFit - model-level test for nmr_any with an N-component reaction system
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
 * Exercises the reaction-editor path of nmr_any: a three-component competitive system
 * A + B <=> AB, A + C <=> AC parsed from the "Reactions" field, driven by the BFGS speciation
 * engine over a 3-column independent table. Verifies component/parameter counts and that the
 * mass balance of all three components closes at every data point. Claude Generated.
 */

#include <cmath>

#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

#include <Eigen/Dense>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/models/titrations/AbstractNMRModel.h"
#include "src/global.h"

class TestNmrNComponent : public QObject {
    Q_OBJECT

private:
    static constexpr int N = 10;
    static constexpr double A0 = 1e-3;

    // Independent table columns 0/1/2 = totals of components A / B / C (first-appearance order).
    static DataClass* makeData()
    {
        Eigen::MatrixXd indep(N, 3);
        Eigen::MatrixXd dep(N, 1);
        for (int i = 0; i < N; ++i) {
            indep(i, 0) = A0; // host A held constant
            indep(i, 1) = 2e-3 * i / (N - 1); // guest B titrated
            indep(i, 2) = 1e-3 * i / (N - 1); // competitor C titrated
            dep(i, 0) = 0.0;
        }
        DataClass* data = new DataClass();
        data->setIndependentTable(new DataTable(indep));
        data->setDataType(DataClassPrivate::Table);
        data->setSimulateDependent(1);
        data->setDependentTable(new DataTable(dep));
        data->setDataBegin(0);
        data->setDataEnd(N);
        return data;
    }

    // Two-component host/guest data (for stepwise A + B / A + AB systems).
    static DataClass* makeData2()
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
    /** Three-component competitive binding: counts, and mass balance of A, B and C. */
    void testCompetitiveMassBalance()
    {
        DataClass* data = makeData();
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
        QVERIFY(!model.isNull());

        QJsonObject def;
        def["Reactions"] = strOption("A + B <=> AB\nA + C <=> AC");
        model->DefineModel(def);

        QCOMPARE(model->InputParameterSize(), 3); // three components A, B, C
        QCOMPARE(model->GlobalParameterSize(), 2); // two species AB, AC

        AbstractNMRModel* tm = qobject_cast<AbstractNMRModel*>(model.data());
        QVERIFY(tm);
        QCOMPARE(tm->SpeciesName(0), QString("AB"));
        QCOMPARE(tm->SpeciesName(1), QString("AC"));

        model->InitialGuess();
        model->setFast(false);
        model->setGlobalParameter(4.0, 0); // lg beta(AB)
        model->setGlobalParameter(3.5, 1); // lg beta(AC)
        model->Calculate();

        // runtime citation: a BFGS-driven model must ask to cite Musketeer alongside SupraFit
        QVERIFY(model->CitationKeys().contains("musketeer"));
        const QString cite = model->CitationBlock();
        QVERIFY(cite.contains("Musketeer"));
        QVERIFY(cite.contains("SupraFit"));

        for (int i = 1; i < N; ++i) {
            const Vector c = tm->getConcentration(i);
            QVERIFY(c.size() >= 6); // idx, freeA, freeB, freeC, [AB], [AC]
            const double freeA = c(1);
            const double freeB = c(2);
            const double freeC = c(3);
            const double AB = c(4); // obs coeff of A in AB is 1
            const double AC = c(5); // obs coeff of A in AC is 1

            const double B0 = 2e-3 * i / (N - 1);
            const double C0 = 1e-3 * i / (N - 1);

            QVERIFY2(relError(freeA + AB + AC, A0) < 1e-7,
                qPrintable(QString("point %1: A balance %2 vs %3").arg(i).arg(freeA + AB + AC).arg(A0)));
            QVERIFY2(relError(freeB + AB, B0) < 1e-6,
                qPrintable(QString("point %1: B balance %2 vs %3").arg(i).arg(freeB + AB).arg(B0)));
            QVERIFY2(relError(freeC + AC, C0) < 1e-6,
                qPrintable(QString("point %1: C balance %2 vs %3").arg(i).arg(freeC + AC).arg(C0)));
        }
        delete data;
    }

    /** Stepwise notation "A + B <=> AB", "A + AB <=> A2B" on 2-column data: resolves to a
     * 2-component {AB, A2B} system (no 3rd "AB" component, no crash) and the host balance closes. */
    void testStepwiseTwoComponent()
    {
        DataClass* data = makeData2();
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
        QVERIFY(!model.isNull());

        QJsonObject def;
        def["Reactions"] = strOption("A + B <=> AB\nA + AB <=> A2B");
        model->DefineModel(def);

        QCOMPARE(model->InputParameterSize(), 2); // stepwise resolves to two components A, B
        QCOMPARE(model->GlobalParameterSize(), 2); // species AB and A2B

        AbstractNMRModel* tm = qobject_cast<AbstractNMRModel*>(model.data());
        QVERIFY(tm);
        QCOMPARE(tm->SpeciesName(0), QString("AB"));
        QCOMPARE(tm->SpeciesName(1), QString::fromUtf8("A₂B"));

        model->InitialGuess();
        model->setFast(false);
        model->setGlobalParameter(4.0, 0); // lg beta(AB)
        model->setGlobalParameter(7.0, 1); // lg beta(A2B)
        model->Calculate();

        for (int i = 1; i < N; ++i) {
            const Vector c = tm->getConcentration(i);
            QVERIFY(c.size() >= 5); // idx, freeA, freeB, hostBound(AB)=1*[AB], hostBound(A2B)=2*[A2B]
            const double freeA = c(1);
            const double boundAB = c(3);
            const double boundA2B = c(4);
            // host mass balance: free A + A in AB + A in A2B = A0
            QVERIFY2(relError(freeA + boundAB + boundA2B, A0) < 1e-7,
                qPrintable(QString("point %1: A balance %2 vs %3").arg(i).arg(freeA + boundAB + boundA2B).arg(A0)));
        }
        delete data;
    }

    /** A reaction system needing more components than data columns flags a mismatch and falls back
     * to the grid (no crash) — this flag drives the GUI's "Reaction / data mismatch" message. */
    void testComponentMismatchFlag()
    {
        DataClass* data = makeData2(); // only two concentration columns
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
        QVERIFY(!model.isNull());

        QJsonObject def;
        def["Reactions"] = strOption("A + B <=> AB\nA + C <=> AC"); // three components
        model->DefineModel(def);

        AbstractTitrationModel* tm = qobject_cast<AbstractTitrationModel*>(model.data());
        QVERIFY(tm);
        QCOMPARE(tm->ReactionComponentMismatch(), 3); // reactions asked for 3 components
        QCOMPARE(model->InputParameterSize(), 2); // fell back to the 2-component grid, no crash
        delete data;
    }
};

QTEST_MAIN(TestNmrNComponent)
#include "test_nmr_ncomponent.moc"
