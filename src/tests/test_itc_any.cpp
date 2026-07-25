/*
 * SupraFit - model-level test for itc_any (BFGS speciation, arbitrary species)
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
 * ITC derives its totals from the cell/syringe protocol, so this test focuses on the generalized
 * model plumbing under the shared SpeciationEngine: parameter/species counts for the classic 1:1
 * grid and for a self-aggregation reaction system, correct species names, and that Calculate() runs
 * without crashing and yields finite heats. Claude Generated.
 */

#include <cmath>

#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

#include <Eigen/Dense>

#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/models/titrations/AbstractItcModel.h"
#include "src/global.h"

class TestItcAny : public QObject {
    Q_OBJECT

private:
    static constexpr int N = 12;

    static DataClass* makeData()
    {
        Eigen::MatrixXd indep(N, 1); // injection volumes
        Eigen::MatrixXd dep(N, 1);
        for (int i = 0; i < N; ++i) {
            indep(i, 0) = 10.0; // µL per injection
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

    /*! \brief Give a built model a real cell/syringe protocol and let it re-read it.
     *
     * Has to happen on the model, not on the DataClass beforehand: building a model runs
     * DeclareSystemParameter() and then LoadSystemParameter(), which would drop values planted
     * earlier. Without a protocol m_c0 stays zero and every concentration-derived guess silently
     * falls back to its default - which is what the two structural tests above do, and why they
     * cannot see a seeding change. Claude Generated */
    static void setProtocol(const QSharedPointer<AbstractModel>& model)
    {
        model->setSystemParameterValue(AbstractItcModel::CellVolume, 1400.0); // µL
        model->setSystemParameterValue(AbstractItcModel::CellConcentration, 1.0); // mmol/L host
        model->setSystemParameterValue(AbstractItcModel::SyringeConcentration, 50.0); // mmol/L guest
        // 50 mmol/L drives the guest to a few times the host over the 12 injections. AB2 needs guest
        // excess to form at all; a guest-poor titration carries no information about it.
        model->setSystemParameterValue(AbstractItcModel::Temperature, 298.15);
        model->setSystemParameterValue(AbstractItcModel::Reservoir, false);
        model->UpdateParameter();
    }

    static QJsonObject strOption(const QString& value)
    {
        QJsonObject o;
        o["value"] = value;
        return o;
    }

    static bool allFinite(AbstractModel* model)
    {
        for (int i = 0; i < model->DataPoints(); ++i)
            for (int j = 0; j < model->SeriesCount(); ++j)
                if (!std::isfinite(model->ModelTable()->data(i, j)))
                    return false;
        return true;
    }

private slots:
    /** Classic 1:1 (A + B <=> AB): one species AB, four local parameters (dH, m, n, fx). */
    void testGridCounts()
    {
        DataClass* data = makeData();
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::itc_any, data);
        QVERIFY(!model.isNull());

        QJsonObject def;
        def["Reactions"] = strOption("A + B <=> AB");
        QVERIFY(model->DefineModel(def));

        QCOMPARE(model->InputParameterSize(), 1); // injection volume only
        QCOMPARE(model->GlobalParameterSize(), 1); // AB
        QCOMPARE(model->LocalParameterSize(), 4); // dH(AB), m, n, fx
        AbstractItcModel* im = qobject_cast<AbstractItcModel*>(model.data());
        QVERIFY(im);
        QCOMPARE(im->SpeciesName(0), QString("AB"));

        model->InitialGuess();
        model->Calculate();
        QVERIFY(allFinite(model.data()));
        delete data;
    }

    /** Reaction editor with self-aggregation: {AB, A2}, five local parameters, correct names. */
    void testSelfAggregationSpecies()
    {
        DataClass* data = makeData();
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::itc_any, data);
        QVERIFY(!model.isNull());

        QJsonObject def;
        def["Reactions"] = strOption("A + B <=> AB\n2 A <=> A2");
        model->DefineModel(def);

        QCOMPARE(model->GlobalParameterSize(), 2); // AB, A2
        QCOMPARE(model->LocalParameterSize(), 5); // dH(AB), dH(A2), m, n, fx
        AbstractItcModel* im = qobject_cast<AbstractItcModel*>(model.data());
        QVERIFY(im);
        QCOMPARE(im->SpeciesName(0), QString("AB"));
        QCOMPARE(im->SpeciesName(1), QString::fromUtf8("A₂"));

        model->InitialGuess();
        model->Calculate();
        QVERIFY(allFinite(model.data()));
        delete data;
    }

    /** The seed of a higher species must scale with its stoichiometric order.
     *
     * Regression: InitialGuess_Private() used to set every species past the first to K + K - the same
     * value for all of them, whatever their stoichiometry. Three species make that visible: AB2 and
     * AB3 came out equal, and now must not. Claude Generated */
    void testSeedScalesWithOrder()
    {
        DataClass* data = makeData();
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::itc_any, data);
        QVERIFY(!model.isNull());

        QJsonObject def;
        def["Reactions"] = strOption("A + B <=> AB\nA + 2 B <=> AB2\nA + 3 B <=> AB3");
        QVERIFY(model->DefineModel(def));
        QCOMPARE(model->GlobalParameterSize(), 3);
        setProtocol(model);

        model->InitialGuess();
        const double ab2 = model->GlobalParameter(1); // order 3 -> 2 * (-lg c_ref)
        const double ab3 = model->GlobalParameter(2); // order 4 -> 3 * (-lg c_ref)

        QVERIFY2(ab3 > ab2, qPrintable(QString("AB3 %1 must be seeded above AB2 %2 (old code gave both K + K)").arg(ab3).arg(ab2)));
        QVERIFY2(std::abs(ab3 / ab2 - 1.5) < 1e-9,
            qPrintable(QString("seed ratio %1, expected 3/2 from the orders").arg(ab3 / ab2)));
        delete data;
    }

    /** Set to the true constants, itc_any 1:1/1:2 reproduces the synthetic heats exactly.
     *
     * The model side of the fit question: the truth is an exact minimum (SSE ~1e-21), so any failure
     * to reach it from a sane start is the optimiser's, not the model's. Deliberately does not fit -
     * see the recovery note in SESSION_HANDOFF.md: from this very data itc_any's minimiser runs away
     * even from a correctly scaled seed, which is a separate open defect. Claude Generated */
    void testTruthIsExactMinimum()
    {
        DataClass* data = makeData();

        QJsonObject def;
        def["Reactions"] = strOption("A + B <=> AB\nA + 2 B <=> AB2");
        const double trueAB = 5.0, trueAB2 = 8.0; // cumulative lg beta

        QSharedPointer<AbstractModel> truth = CreateModel(SupraFit::itc_any, data);
        QVERIFY(truth->DefineModel(def));
        QCOMPARE(truth->GlobalParameterSize(), 2);
        setProtocol(truth);
        truth->InitialGuess();
        truth->setGlobalParameter(trueAB, 0);
        truth->setGlobalParameter(trueAB2, 1);
        // Local layout for two species: dH(AB), dH(AB2), m, n, fx. InitialGuess() derives fx from the
        // dependent data, which is still zero here, so pin the three protocol terms to neutral values
        // and let the two enthalpies carry the signal.
        truth->setLocalParameter(-40.0, 0, 0); // dH(AB)
        truth->setLocalParameter(-25.0, 1, 0); // dH(AB2)
        truth->setLocalParameter(0.0, 2, 0); // m (solv H)
        truth->setLocalParameter(1.0, 3, 0); // n (solv H)
        truth->setLocalParameter(1.0, 4, 0); // fx
        truth->Calculate();

        // A guest-rich regime, or AB2 never forms and carries no signal.
        AbstractItcModel* im = qobject_cast<AbstractItcModel*>(truth.data());
        QVERIFY(im);
        QVERIFY2(im->InitialGuestConcentration(N - 1) > 2.0 * im->InitialHostConcentration(N - 1),
            "protocol is guest-poor: AB2 would not form");
        QVERIFY(allFinite(truth.data()));

        data->setDependentTable(new DataTable(truth->ModelTable()->Table()));

        QSharedPointer<AbstractModel> ref = CreateModel(SupraFit::itc_any, data);
        QVERIFY(ref->DefineModel(def));
        setProtocol(ref);
        ref->InitialGuess();
        ref->setGlobalParameter(trueAB, 0);
        ref->setGlobalParameter(trueAB2, 1);
        ref->setLocalParameter(-40.0, 0, 0);
        ref->setLocalParameter(-25.0, 1, 0);
        ref->setLocalParameter(0.0, 2, 0);
        ref->setLocalParameter(1.0, 3, 0);
        ref->setLocalParameter(1.0, 4, 0);
        ref->Calculate();

        QVERIFY2(ref->SSE() < 1e-12, qPrintable(QString("SSE at the true constants is %1, expected ~0").arg(ref->SSE())));
        delete data;
    }
};

QTEST_MAIN(TestItcAny)
#include "test_itc_any.moc"
