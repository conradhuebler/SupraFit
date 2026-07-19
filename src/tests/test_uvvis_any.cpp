/*
 * SupraFit - model-level test for uvvis_any (BFGS speciation + Beer-Lambert observable)
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
 * Verifies uvvis_any on the classic 1:1 grid: the free/complex concentrations mass-balance and the
 * model signal equals the Beer-Lambert sum (free A + free B + complex) weighted by the molar
 * extinction coefficients. Claude Generated.
 */

#include <cmath>

#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

#include <Eigen/Dense>

#include "src/core/equil.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/models/titrations/AbstractTitrationModel.h"
#include "src/global.h"

class TestUvVisAny : public QObject {
    Q_OBJECT

private:
    static constexpr int N = 12;
    static constexpr double A0 = 1e-3;

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
    void testBeerLambert()
    {
        DataClass* data = makeData();
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::uvvis_any, data);
        QVERIFY(!model.isNull());

        QJsonObject def;
        def["Reactions"] = strOption("A + B <=> AB");
        QVERIFY(model->DefineModel(def));

        QCOMPARE(model->InputParameterSize(), 2);
        QCOMPARE(model->GlobalParameterSize(), 1); // AB
        QCOMPARE(model->LocalParameterSize(), 3); // eps A, eps B, eps AB

        model->InitialGuess();
        model->setFast(false);

        const double logK = 4.0;
        const double epsA = 100.0, epsB = 250.0, epsAB = 5000.0;
        model->setGlobalParameter(logK, 0);
        model->setLocalParameter(epsA, 0, 0);
        model->setLocalParameter(epsB, 1, 0);
        model->setLocalParameter(epsAB, 2, 0);
        model->Calculate();

        AbstractTitrationModel* tm = qobject_cast<AbstractTitrationModel*>(model.data());
        QVERIFY(tm);

        for (int i = 0; i < N; ++i) {
            const Vector c = tm->getConcentration(i);
            QVERIFY(c.size() >= 4);
            const double freeA = c(1);
            const double freeB = c(2);
            const double AB = c(3);
            const double B0 = 2e-3 * i / (N - 1);

            const double freeA_ref = ItoI::HostConcentration(A0, B0, logK);
            QVERIFY2(relError(freeA, freeA_ref) < 1e-5,
                qPrintable(QString("point %1: free A %2 vs analytic %3").arg(i).arg(freeA).arg(freeA_ref)));
            QVERIFY(relError(freeA + AB, A0) < 1e-8);
            if (B0 > 0)
                QVERIFY(relError(freeB + AB, B0) < 1e-6);

            // Beer-Lambert: modelled absorbance = eps . concentrations
            const double expected = epsA * freeA + epsB * freeB + epsAB * AB;
            const double got = model->ModelTable()->data(i, 0);
            QVERIFY2(relError(got, expected) < 1e-6,
                qPrintable(QString("point %1: A(model) %2 vs Beer-Lambert %3").arg(i).arg(got).arg(expected)));
        }
        delete data;
    }
};

QTEST_MAIN(TestUvVisAny)
#include "test_uvvis_any.moc"
