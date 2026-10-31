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
 * The fixed models are presets of fl_any since 2026-11 (AI-implemented rebuild); before, the first point had a coefficient of its own and fl_ItoI
 * scaled every row with the model signal of row 0 (defects 1 and 2 of the review, pinned here). Their constants are the cumulative lg beta.
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
        QJsonObject cfg = model->getOptimizerConfig();
        cfg["FitSolver"] = QStringLiteral("VarPro"); // the locals are projected out (flat directions at low guest coverage make the full-vector solver drift)
        model->setOptimizerConfig(cfg);
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
    // Error 2: leaving out the first point (the first row unchecked) must not make the fit worse or break the model. (The earlier fl_ItoI scaled every
    // row with the model signal of row 0; a fresh model with row 0 left out gave a sum of squares of 3e8.)
    void leavingOutTheFirstPointKeepsTheFit()
    {
        for (const SupraFit::Model id : { SupraFit::fl_ItoI, SupraFit::fl_ItoI_ItoII }) {
            const bool one = id == SupraFit::fl_ItoI;
            DataClass* data = one ? makeSignal(2, "A + B <=> AB", { 4.0 }, { 2e8, 0.0, 6e8 }) : makeSignal(2, "A + B <=> AB\nA + 2 B <=> AB2", { 4.0, 7.0 }, { 2e8, 0.0, 6e8, 9e8 });
            QSharedPointer<AbstractModel> model = CreateModel(id, data);
            model->InitialGuess();
            model->setGlobalParameter(4.0, 0);
            if (!one)
                model->setGlobalParameter(7.0, 1);
            Minimizer m(false);
            m.setModel(model);
            m.Minimize();
            const double before = model->SSE();
            double spread = 0;
            for (int c = 0; c < model->DependentModel()->columnCount(); ++c)
                for (int r = 0; r < N; ++r)
                    spread += std::pow(model->DependentModel()->data(r, c), 2);
            data->setDataBegin(1);
            QSharedPointer<AbstractModel> fresh = model->Clone(); // a model that has never calculated row 0
            fresh->Calculate();
            qInfo().noquote() << QString("%1: SSE %2 with all points, %3 without the first (fresh model)").arg(model->Name()).arg(before, 0, 'g', 3).arg(fresh->SSE(), 0, 'g', 3);
            QVERIFY2(std::isfinite(fresh->SSE()) && fresh->SSE() <= before * (1 + 1e-6) + 1e-9 * spread, "the fit got worse when the first point was left out");
            delete data;
        }
    }

    // Error 1: row 0 has its own free coefficient ("A0") in the fixed models, so the first point never constrains the fit: a wrong first
    // point is fitted exactly. In a model of the host fluorescence ("A") the first point (no guest) measures the same coefficient as the others.
    void firstPointConstrainsTheFit()
    {
        DataClass* data = makeSignal(2, "A + B <=> AB\nA + 2 B <=> AB2", { 4.0, 7.0 }, { 2e8, 0.0, 6e8, 9e8 });
        Eigen::MatrixXd dep = data->DependentModel()->Table();
        dep.row(0) *= 1.5; // the first point is 50 % too high in every series
        data->setDependentTable(new DataTable(dep));
        data->setDataBegin(0);
        data->setDataEnd(N);
        for (const SupraFit::Model id : { SupraFit::fl_any, SupraFit::fl_ItoI_ItoII }) {
            QSharedPointer<AbstractModel> model = CreateModel(id, data);
            if (id == SupraFit::fl_any) {
                QJsonObject def, r;
                r["value"] = QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2");
                def["Reactions"] = r;
                model->DefineModel(def);
            }
            model->InitialGuess();
            model->setGlobalParameter(4.0, 0);
            model->setGlobalParameter(7.0, 1);
            Minimizer m(false);
            m.setModel(model);
            m.Minimize();
            double spread = 0;
            for (int c = 0; c < model->DependentModel()->columnCount(); ++c)
                for (int r = 0; r < N; ++r)
                    spread += std::pow(model->DependentModel()->data(r, c), 2);
            const double rel = model->SSE() / spread;
            qInfo().noquote() << QString("%1: a first point 50 % too high leaves SSE/signal^2 = %2").arg(model->Name()).arg(rel, 0, 'g', 3);
            QVERIFY2(rel > 1e-4, "a 50 % wrong first point did not show in the sum of squares");
        }
        delete data;
    }

    void fixedModelsOnTextbookData()
    {
        // 1:1, truth lg K = 4; molar fluorescence per series: host 2e8, guest 0 (silent), complex 6e8
        {
            DataClass* data = makeSignal(2, "A + B <=> AB", { 4.0 }, { 2e8, 0.0, 6e8 });
            QVector<double> k;
            const double rel = fitFixed(SupraFit::fl_ItoI, data, k);
            qInfo().noquote() << QString("fl 1:1      : truth lgK 4.00 -> lgK %1, SSE/spread %2").arg(k.value(0), 0, 'f', 3).arg(rel, 0, 'g', 3);
            QVERIFY2(std::abs(k.value(0) - 4.0) < 1e-2, "fl_ItoI does not recover lg K of textbook data");
            delete data;
        }
        // 1:1 and 1:2: truth lg beta(AB) = 4, lg beta(AB2) = 7 (lg K11 = 4, lg K12 = 3)
        {
            DataClass* data = makeSignal(2, "A + B <=> AB\nA + 2 B <=> AB2", { 4.0, 7.0 }, { 2e8, 0.0, 6e8, 9e8 });
            QVector<double> k;
            const double atTruth = fitFixed(SupraFit::fl_ItoI_ItoII, data, k, { 4.0, 7.0 });
            qInfo().noquote() << QString("fl 1:1/1:2  : started at the truth (lg beta 4, 7)  -> %1 %2, SSE/spread %3").arg(k.value(0), 0, 'f', 3).arg(k.value(1), 0, 'f', 3).arg(atTruth, 0, 'g', 3);
            QVERIFY(std::abs(k.value(0) - 4.0) < 1e-2 && std::abs(k.value(1) - 7.0) < 1e-2);
            const double cold = fitFixed(SupraFit::fl_ItoI_ItoII, data, k);
            qInfo().noquote() << QString("fl 1:1/1:2  : cold start                          -> %1 %2, SSE/spread %3").arg(k.value(0), 0, 'f', 3).arg(k.value(1), 0, 'f', 3).arg(cold, 0, 'g', 3);
            // noise-free data of up to 5 equivalents of guest leave a flat valley for lg beta(AB2): the fit is as good as the truth (SSE/spread ~1e-10), the
            // constants are within half a unit of it (the earlier model ended at 4.49 / 3.69 for lg K11 / lg K12 = 4 / 3, a wrong pair)
            QVERIFY2(cold < 1e-8 && std::abs(k.value(0) - 4.0) < 0.5 && std::abs(k.value(1) - 7.0) < 0.5, "the cold start of fl_ItoI_ItoII ends far from the constants");
            delete data;
        }
        // 2:1 and 1:1, truth lg beta(AB) = 4, lg beta(A2B) = 7
        {
            DataClass* data = makeSignal(2, "A + B <=> AB\n2 A + B <=> A2B", { 4.0, 7.0 }, { 2e8, 0.0, 6e8, 9e8 });
            QVector<double> k;
            const double rel = fitFixed(SupraFit::fl_IItoI_ItoI, data, k);
            qInfo().noquote() << QString("fl 2:1/1:1  : truth lg beta(AB) 4, lg beta(A2B) 7 -> %1 %2, SSE/spread %3").arg(k.value(0), 0, 'f', 3).arg(k.value(1), 0, 'f', 3).arg(rel, 0, 'g', 3);
            const double rel2 = fitFixed(SupraFit::fl_IItoI_ItoI, data, k, { 4.0, 7.0 });
            qInfo().noquote() << QString("   started at the truth                         -> %1 %2, SSE/spread %3").arg(k.value(0), 0, 'f', 3).arg(k.value(1), 0, 'f', 3).arg(rel2, 0, 'g', 3);
            delete data;
        }
    }
};

QTEST_MAIN(TestFlFixedVsAny)
#include "test_fl_fixed_vs_any.moc"
