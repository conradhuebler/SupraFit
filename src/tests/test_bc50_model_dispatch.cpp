/*
 * SupraFit - each titration model's BC50System() must reproduce its old hardcoded BC50
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
 */

/* BC50 used to be reported by a per-model call that hand-picked the formula and global-parameter
 * indices for each stoichiometry. That was unified: every model now returns a BC50::ModelSystem
 * (stoichiometry + cumulative lg beta) and one shared dispatch computes the value. This test builds
 * each fixed model, sets known global parameters, and asserts that BC50::FromSpeciation(model's
 * descriptor) equals the value the old hardcoded formula produced from those same globals -
 * i.e. that each model's BC50System() encodes the right stoichiometry and constant conversion.
 *
 * The nmr family (ids 1-4) covers all four implemented stoichiometries; the fl / uv_vis / itc
 * models of each stoichiometry share the identical BC50System() body, so nmr is representative.
 * Claude Generated (2026). */

#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

#include <cmath>

#include "src/core/bc50.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"

class BC50ModelDispatchTest : public QObject {
    Q_OBJECT

private:
    static QSharedPointer<AbstractModel> build(int id)
    {
        DataTable* indep = new DataTable(20, 2, nullptr);
        DataTable* dep = new DataTable(20, 3, nullptr);
        for (int r = 0; r < 20; ++r) {
            indep->data(r, 0) = 1e-3;
            indep->data(r, 1) = 1e-3 * (r + 1) / 20.0;
            for (int c = 0; c < 3; ++c)
                dep->data(r, c) = 0.1 * r;
        }
        DataClass* project = new DataClass();
        project->setIndependentTable(indep);
        project->setDataType(DataClassPrivate::Table);
        project->setSimulateDependent(3);
        project->setDependentTable(dep);

        QSharedPointer<AbstractModel> model = CreateModel(id, QPointer<DataClass>(project));
        if (model)
            for (int i = 0; i < model->GlobalParameterSize(); ++i)
                model->setGlobalParameter(3.0 + 1.5 * i, i); // g0=3.0, g1=4.5, g2=6.0
        return model;
    }

    static void checkClose(double got, double expected, const QString& what)
    {
        QVERIFY2(std::abs(got - expected) / expected < 1e-12,
            qPrintable(QString("%1: dispatch %2 vs hardcoded %3")
                    .arg(what).arg(got, 0, 'g', 12).arg(expected, 0, 'g', 12)));
    }

private slots:

    void nmr_ItoI()
    {
        auto m = build(1);
        QVERIFY(m);
        QCOMPARE(BC50::Classify(m->BC50System().stoich), BC50::System::ItoI);
        checkClose(BC50::FromSpeciation(m->BC50System().stoich, m->BC50System().lgBeta),
            BC50::ItoI::BC50(3.0), "nmr 1:1");
    }

    void nmr_IItoI()
    {
        auto m = build(2);
        QVERIFY(m);
        QCOMPARE(BC50::Classify(m->BC50System().stoich), BC50::System::IItoI);
        // globals: g0 = lgK21 (stepwise), g1 = lgK11
        checkClose(BC50::FromSpeciation(m->BC50System().stoich, m->BC50System().lgBeta),
            BC50::IItoI::BC50(3.0, 4.5), "nmr 2:1/1:1");
    }

    void nmr_ItoII()
    {
        auto m = build(3);
        QVERIFY(m);
        QCOMPARE(BC50::Classify(m->BC50System().stoich), BC50::System::ItoII);
        // globals: g0 = lgK11, g1 = lgK12 (stepwise)
        checkClose(BC50::FromSpeciation(m->BC50System().stoich, m->BC50System().lgBeta),
            BC50::ItoII::BC50(3.0, 4.5), "nmr 1:1/1:2");
    }

    void nmr_IItoII()
    {
        auto m = build(4);
        QVERIFY(m);
        QCOMPARE(BC50::Classify(m->BC50System().stoich), BC50::System::IItoII);
        // globals: g0 = lgK21, g1 = lgK11, g2 = lgK12
        checkClose(BC50::FromSpeciation(m->BC50System().stoich, m->BC50System().lgBeta),
            BC50::IItoII::BC50_A0(3.0, 4.5, 6.0), "nmr 2:1/1:1/1:2");
    }

    /* The unified ModelInfo must still contain a BC50 line for a fixed model. */
    void modelInfoContainsBC50()
    {
        auto m = build(3);
        QVERIFY(m);
        QVERIFY2(m->ModelInfo().contains("BC50"), "fixed model ModelInfo lost its BC50 block");
    }
};

QTEST_MAIN(BC50ModelDispatchTest)

#include "test_bc50_model_dispatch.moc"
