/*
 * Fixed fluorescence models against the reaction-driven fl_any - Claude Generated (2026)
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
 */

/*
 * Data from fl_any (signal = sum of concentration x fluorescence coefficient, the textbook form) is fitted with the fixed fluorescence models
 * (fl_ItoI, fl_ItoI_ItoII, fl_IItoI_ItoI, fl_IItoI_ItoI_ItoII): do they reach the same optimum, and do they recover the constants?
 */

#include <cmath>

#include <QtCore/QJsonObject>
#include <QtCore/QVector>
#include <QtTest/QtTest>

#include <Eigen/Dense>

#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/global.h"

class TestFlFixedVsAny : public QObject {
    Q_OBJECT

private:
    static constexpr int N = 21;
    static constexpr double A0 = 1e-5;

    static DataClass* makeSignal(int series, const QString& reactions, const QVector<double>& betas, const QVector<double>& phi)
    {
        Eigen::MatrixXd indep(N, 2);
        Eigen::MatrixXd dep = Eigen::MatrixXd::Zero(N, series);
        for (int i = 0; i < N; ++i) {
            indep(i, 0) = A0;
            indep(i, 1) = 5 * A0 * i / (N - 1);
        }
        DataClass* data = new DataClass();
        data->setIndependentTable(new DataTable(indep));
        data->setDataType(DataClassPrivate::Table);
        data->setSimulateDependent(series);
        data->setDependentTable(new DataTable(dep));
        data->setDataBegin(0);
        data->setDataEnd(N);
        QSharedPointer<AbstractModel> truth = CreateModel(SupraFit::fl_any, data);
        QJsonObject def, r;
        r["value"] = reactions;
        def["Reactions"] = r;
        truth->DefineModel(def);
        truth->InitialGuess();
        for (int k = 0; k < betas.size(); ++k)
            truth->setGlobalParameter(betas[k], k);
        for (int s = 0; s < series; ++s)
            for (int p = 0; p < truth->LocalParameterSize(); ++p)
                truth->setLocalParameter(phi[p] * (1.0 - 0.05 * s), p, s);
        truth->Calculate();
        data->setDependentTable(new DataTable(truth->ModelTable()->Table()));
        data->setDataBegin(0);
        data->setDataEnd(N);
        return data;
    }

    // fit the fixed model from its own initial guess; returns SSE relative to the spread of the data, and the constants
    static double fitFixed(SupraFit::Model id, DataClass* data, QVector<double>& constants, const QVector<double>& start = QVector<double>())
    {
        QSharedPointer<AbstractModel> model = CreateModel(id, data);
        model->InitialGuess();
        for (int k = 0; k < start.size(); ++k)
            model->setGlobalParameter(start[k], k);
        Minimizer m(false);
        m.setModel(model);
        m.Minimize();
        double spread = 0;
        const DataTable* dep = model->DependentModel();
        for (int c = 0; c < dep->columnCount(); ++c) {
            double mean = 0;
            for (int r = 0; r < dep->rowCount(); ++r)
                mean += dep->data(r, c) / dep->rowCount();
            for (int r = 0; r < dep->rowCount(); ++r)
                spread += std::pow(dep->data(r, c) - mean, 2);
        }
        constants.clear();
        for (int k = 0; k < model->GlobalParameterSize(); ++k)
            constants << model->GlobalParameter(k);
        return model->SSE() / spread;
    }

private slots:
    // Leaving out the first point (the first row unchecked) must not change a model that already fits: the fixed models treat row 0 as a
    // special case (a free coefficient "A0" only for that row, and fl_ItoI scales every other row with the signal of row 0).
    void leavingOutTheFirstPointKeepsTheFit()
    {
        for (const SupraFit::Model id : { SupraFit::fl_ItoI, SupraFit::fl_ItoI_ItoII }) {
            const bool one = id == SupraFit::fl_ItoI;
            DataClass* data = one ? makeSignal(2, "A + B <=> AB", { 4.0 }, { 2e8, 0.0, 6e8 }) : makeSignal(2, "A + B <=> AB\nA + 2 B <=> AB2", { 4.0, 7.0 }, { 2e8, 0.0, 6e8, 9e8 });
            QVector<double> k;
            fitFixed(id, data, k, one ? QVector<double>{ 4.0 } : QVector<double>{ 4.0, 3.0 });
            QSharedPointer<AbstractModel> model = CreateModel(id, data);
            model->InitialGuess();
            for (int j = 0; j < k.size(); ++j)
                model->setGlobalParameter(k[j], j);
            Minimizer m(false);
            m.setModel(model);
            m.Minimize();
            const double before = model->SSE();
            data->setDataBegin(1);
            QSharedPointer<AbstractModel> fresh = model->Clone(); // a model that has never calculated row 0 (the stale signal of the old one hides the problem)
            fresh->Calculate();
            qInfo().noquote() << QString("%1: SSE %2 with all points, %3 without the first (fresh model)").arg(model->Name()).arg(before, 0, 'g', 3).arg(fresh->SSE(), 0, 'g', 3);
            if (one)
                QEXPECT_FAIL("", "fl_ItoI multiplies every row with the model signal of row 0, which is not calculated when row 0 is left out", Continue);
            QVERIFY2(fresh->SSE() < 1e-6 * (before + 1e-3), "the fit changed when the first point was left out");
            delete data;
        }
    }

    void fixedModelsOnTextbookData()
    {
        // 1:1, truth lg K = 4; molar fluorescence per series: host 2e8, guest 0 (silent), complex 6e8
        {
            DataClass* data = makeSignal(2, "A + B <=> AB", { 4.0 }, { 2e8, 0.0, 6e8 });
            QVector<double> k;
            const double rel = fitFixed(SupraFit::fl_ItoI, data, k);
            qInfo().noquote() << QString("fl 1:1      : truth lgK 4.00 -> lgK %1, SSE/spread %2").arg(k.value(0), 0, 'f', 3).arg(rel, 0, 'g', 3);
            QVERIFY2(std::abs(k.value(0) - 4.0) < 1e-3, "fl_ItoI does not recover lg K of textbook data");
            delete data;
        }
        // 1:1 and 1:2, truth lg beta 4, 7 (lg K11 = 4, lg K12 = 3)
        {
            DataClass* data = makeSignal(2, "A + B <=> AB\nA + 2 B <=> AB2", { 4.0, 7.0 }, { 2e8, 0.0, 6e8, 9e8 });
            QVector<double> k;
            const double rel = fitFixed(SupraFit::fl_ItoI_ItoII, data, k);
            qInfo().noquote() << QString("fl 1:1/1:2  : truth lgK11 4.00 lgK12 3.00 -> %1 %2, SSE/spread %3").arg(k.value(0), 0, 'f', 3).arg(k.value(1), 0, 'f', 3).arg(rel, 0, 'g', 3);
            const double rel2 = fitFixed(SupraFit::fl_ItoI_ItoII, data, k, { 4.0, 3.0 });
            qInfo().noquote() << QString("   started at the truth                         -> %1 %2, SSE/spread %3").arg(k.value(0), 0, 'f', 3).arg(k.value(1), 0, 'f', 3).arg(rel2, 0, 'g', 3);
            // the model can describe the data (the truth is a minimum) ...
            QVERIFY(std::abs(k.value(0) - 4.0) < 1e-3 && std::abs(k.value(1) - 3.0) < 1e-3);
            // ... but the fit from the model's own start does not find it
            const QVector<double> cold = k;
            Q_UNUSED(cold);
            fitFixed(SupraFit::fl_ItoI_ItoII, data, k);
            QEXPECT_FAIL("", "InitialGuess of fl_ItoI_ItoII ends in a wrong minimum (lg K 4.49 / 3.69)", Continue);
            QVERIFY(std::abs(k.value(0) - 4.0) < 1e-2 && std::abs(k.value(1) - 3.0) < 1e-2);
            delete data;
        }
        // 2:1 and 1:1, truth lg beta(AB) = 4, lg beta(A2B) = 7
        {
            DataClass* data = makeSignal(2, "A + B <=> AB\n2 A + B <=> A2B", { 4.0, 7.0 }, { 2e8, 0.0, 6e8, 9e8 });
            QVector<double> k;
            const double rel = fitFixed(SupraFit::fl_IItoI_ItoI, data, k);
            qInfo().noquote() << QString("fl 2:1/1:1  : truth lgK21 3.00 lgK11 4.00 -> %1 %2, SSE/spread %3").arg(k.value(0), 0, 'f', 3).arg(k.value(1), 0, 'f', 3).arg(rel, 0, 'g', 3);
            const double rel2 = fitFixed(SupraFit::fl_IItoI_ItoI, data, k, { 3.0, 4.0 });
            qInfo().noquote() << QString("   started at the truth                         -> %1 %2, SSE/spread %3").arg(k.value(0), 0, 'f', 3).arg(k.value(1), 0, 'f', 3).arg(rel2, 0, 'g', 3);
            delete data;
        }
    }
};

QTEST_MAIN(TestFlFixedVsAny)
#include "test_fl_fixed_vs_any.moc"
