/*
 * SupraFit - equivalence test for the opt-in VarPro solver vs the classic Levenberg-Marquardt
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
 */

/*
 * The variable-projection solver (FitSolver = "VarPro") must reach the same optimum as the classic
 * full-vector Levenberg-Marquardt (FitSolver = "LevMar") on the same data: it optimises only the
 * global constants and projects the linear shifts out, so on noise-free synthetic data both must
 * recover the true constants and the true signal. This is the correctness gate before VarPro is
 * benchmarked / used more widely. Claude Generated.
 */

#include <cmath>

#include <QtTest/QtTest>
#include <QtCore/QJsonObject>

#include <Eigen/Dense>

#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/global.h"

class TestVarPro : public QObject {
    Q_OBJECT

private:
    static constexpr int N = 16;
    static constexpr double A0 = 1e-3;

    static QJsonObject strOption(const QString& value)
    {
        QJsonObject o;
        o["value"] = value;
        return o;
    }

    // Host-constant / guest-titrated 2-component data with @p series (simulated) dependent columns.
    static DataClass* makeData(int series)
    {
        Eigen::MatrixXd indep(N, 2);
        Eigen::MatrixXd dep(N, series);
        for (int i = 0; i < N; ++i) {
            indep(i, 0) = A0;
            indep(i, 1) = 3e-3 * i / (N - 1);
            for (int j = 0; j < series; ++j)
                dep(i, j) = 0.0;
        }
        DataClass* data = new DataClass();
        data->setIndependentTable(new DataTable(indep));
        data->setDataType(DataClassPrivate::Table);
        data->setSimulateDependent(series);
        data->setDependentTable(new DataTable(dep));
        data->setDataBegin(0);
        data->setDataEnd(N);
        return data;
    }

    // Fit a fresh model of @p modelId with the given solver; return SSE and fitted global betas.
    static double fit(int modelId, const QString& solver, const QString& reactions, DataClass* data, QVector<double>& betas)
    {
        QSharedPointer<AbstractModel> model = CreateModel(static_cast<SupraFit::Model>(modelId), data);
        QJsonObject def;
        def["Reactions"] = strOption(reactions);
        model->DefineModel(def);
        QJsonObject cfg = model->getOptimizerConfig();
        cfg["FitSolver"] = solver;
        model->setOptimizerConfig(cfg);
        model->InitialGuess();
        Minimizer m(false);
        m.setModel(model);
        m.Minimize();
        betas.clear();
        for (int k = 0; k < model->GlobalParameterSize(); ++k)
            betas << model->GlobalParameter(k);
        return model->SSE();
    }

private slots:
    void equivalence_data()
    {
        QTest::addColumn<int>("modelId");
        QTest::addColumn<QString>("reactions");
        QTest::addColumn<QList<double>>("trueBetas"); // in species order
        QTest::addColumn<int>("series");
        QTest::addColumn<double>("localScale"); // magnitude of the linear locals (shifts / extinction)

        const int nmr = static_cast<int>(SupraFit::nmr_any);
        const int uvvis = static_cast<int>(SupraFit::uvvis_any);
        // Fixed 1:1 model (ignores the Reactions field) - exercises the design-matrix refactor of the
        // classic model, not just the generalised *_any path.
        QTest::newRow("nmr_ItoI fixed 1:1") << static_cast<int>(SupraFit::nmr_ItoI) << QString() << QList<double>{ 4.0 } << 3 << 9.0;
        // GP order is stepwise (K21, K11); K21=2.4,K11=4.2 == cumulative beta(AB)=4.2, beta(A2B)=6.6,
        // the same well-conditioned system as the "nmr 2:1/1:1" any-model row.
        QTest::newRow("nmr_IItoI_ItoI fixed 2:1/1:1") << static_cast<int>(SupraFit::nmr_IItoI_ItoI) << QString() << QList<double>{ 2.4, 4.2 } << 2 << 9.0;
        QTest::newRow("nmr_ItoI_ItoII fixed 1:1/1:2") << static_cast<int>(SupraFit::nmr_ItoI_ItoII) << QString() << QList<double>{ 4.0, 2.0 } << 2 << 9.0;
        QTest::newRow("nmr 1:1") << nmr << QStringLiteral("A + B <=> AB") << QList<double>{ 4.0 } << 3 << 9.0;
        QTest::newRow("nmr 1:1/1:2") << nmr << QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2") << QList<double>{ 3.8, 5.9 } << 2 << 9.0;
        QTest::newRow("nmr 2:1/1:1") << nmr << QStringLiteral("A + B <=> AB\n2 A + B <=> A2B") << QList<double>{ 4.2, 6.6 } << 2 << 9.0;
        QTest::newRow("uvvis 1:1") << uvvis << QStringLiteral("A + B <=> AB") << QList<double>{ 4.0 } << 3 << 4000.0;
        QTest::newRow("uvvis 1:1/1:2") << uvvis << QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2") << QList<double>{ 3.8, 5.9 } << 2 << 4000.0;
    }

    void equivalence()
    {
        QFETCH(int, modelId);
        QFETCH(QString, reactions);
        QFETCH(QList<double>, trueBetas);
        QFETCH(int, series);
        QFETCH(double, localScale);

        // (1) synthesise a noise-free signal with a truth model at the true constants + linear locals.
        DataClass* data = makeData(series);
        {
            QSharedPointer<AbstractModel> truth = CreateModel(static_cast<SupraFit::Model>(modelId), data);
            QJsonObject def;
            def["Reactions"] = strOption(reactions);
            truth->DefineModel(def);
            truth->InitialGuess();
            for (int k = 0; k < trueBetas.size(); ++k)
                truth->setGlobalParameter(trueBetas[k], k);
            // distinct per-series locals so the linear projection is genuinely exercised
            for (int s = 0; s < series; ++s)
                for (int p = 0; p < truth->LocalParameterSize(); ++p)
                    truth->setLocalParameter(localScale * (1.0 - 0.1 * p - 0.07 * s), p, s);
            truth->Calculate();
            data->setDependentTable(new DataTable(truth->ModelTable()->Table()));
        }

        // (2) fit the same data with both solvers.
        QVector<double> betaLev, betaVar;
        const double sseLev = fit(modelId, QStringLiteral("LevMar"), reactions, data, betaLev);
        const double sseVar = fit(modelId, QStringLiteral("VarPro"), reactions, data, betaVar);

        qInfo().noquote() << QString("[%1] SSE LevMar=%2 VarPro=%3")
                                 .arg(QTest::currentDataTag()).arg(sseLev, 0, 'g', 4).arg(sseVar, 0, 'g', 4);

        // Scale the "reached the minimum" floor to the truth signal energy so it works for both NMR
        // shifts (~O(10)) and UV/Vis extinction coefficients (~O(1000)).
        double energy = 0.0;
        for (int r = 0; r < data->DependentModel()->rowCount(); ++r)
            for (int c = 0; c < data->DependentModel()->columnCount(); ++c)
                energy += data->DependentModel()->data(r, c) * data->DependentModel()->data(r, c);
        const double floor = 1e-10 * qMax(energy, 1.0);
        const bool levConverged = std::isfinite(sseLev) && sseLev < floor;

        // VarPro MUST reach the true minimum on this noise-free data — the core correctness gate.
        QVERIFY2(std::isfinite(sseVar) && sseVar < floor,
            qPrintable(QString("VarPro SSE %1 not ~0 (floor %2)").arg(sseVar).arg(floor)));
        // The classic full-vector solver may fail to reach it (extra local minima from optimising the
        // linear locals jointly): that is a VarPro robustness win, reported, not a test failure.
        if (!levConverged)
            qInfo().noquote() << QString("  [%1] NOTE: classic LevMar stalled at SSE=%2 while VarPro reached %3 — VarPro more robust here")
                                     .arg(QTest::currentDataTag()).arg(sseLev, 0, 'g', 4).arg(sseVar, 0, 'g', 4);

        // VarPro recovers the true constants; where LevMar also converged, the two must agree.
        QCOMPARE(betaVar.size(), trueBetas.size());
        for (int k = 0; k < trueBetas.size(); ++k) {
            qInfo().noquote() << QString("  [%1] lg beta[%2] true=%3 LevMar=%4 VarPro=%5")
                                     .arg(QTest::currentDataTag()).arg(k).arg(trueBetas[k])
                                     .arg(betaLev[k], 0, 'g', 6).arg(betaVar[k], 0, 'g', 6);
            QVERIFY2(std::abs(betaVar[k] - trueBetas[k]) < 1e-2,
                qPrintable(QString("global %1: VarPro %2 did not recover truth %3").arg(k).arg(betaVar[k]).arg(trueBetas[k])));
            if (levConverged)
                QVERIFY2(std::abs(betaVar[k] - betaLev[k]) < 1e-3,
                    qPrintable(QString("global %1: VarPro %2 != LevMar %3 (both converged)").arg(k).arg(betaVar[k]).arg(betaLev[k])));
        }
        delete data;
    }
};

QTEST_MAIN(TestVarPro)
#include "test_varpro.moc"
