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
};

QTEST_MAIN(TestItcAny)
#include "test_itc_any.moc"
