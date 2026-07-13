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

    // Fit a fresh nmr_any with the given solver; return SSE and fitted global betas.
    static double fit(const QString& solver, const QString& reactions, DataClass* data, QVector<double>& betas)
    {
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
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
        QTest::addColumn<QString>("reactions");
        QTest::addColumn<QList<double>>("trueBetas"); // in species order
        QTest::addColumn<int>("series");

        QTest::newRow("1:1") << QStringLiteral("A + B <=> AB") << QList<double>{ 4.0 } << 3;
        QTest::newRow("1:1/1:2") << QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2") << QList<double>{ 3.8, 5.9 } << 2;
        QTest::newRow("2:1/1:1") << QStringLiteral("A + B <=> AB\n2 A + B <=> A2B") << QList<double>{ 4.2, 6.6 } << 2;
    }

    void equivalence()
    {
        QFETCH(QString, reactions);
        QFETCH(QList<double>, trueBetas);
        QFETCH(int, series);

        // (1) synthesise a noise-free signal with a truth nmr_any at the true constants + shifts.
        DataClass* data = makeData(series);
        {
            QSharedPointer<AbstractModel> truth = CreateModel(SupraFit::nmr_any, data);
            QJsonObject def;
            def["Reactions"] = strOption(reactions);
            truth->DefineModel(def);
            truth->InitialGuess();
            for (int k = 0; k < trueBetas.size(); ++k)
                truth->setGlobalParameter(trueBetas[k], k);
            // distinct per-series shifts so the linear projection is genuinely exercised
            for (int s = 0; s < series; ++s)
                for (int p = 0; p < truth->LocalParameterSize(); ++p)
                    truth->setLocalParameter(9.0 - p - 0.7 * s, p, s);
            truth->Calculate();
            data->setDependentTable(new DataTable(truth->ModelTable()->Table()));
        }

        // (2) fit the same data with both solvers.
        QVector<double> betaLev, betaVar;
        const double sseLev = fit(QStringLiteral("LevMar"), reactions, data, betaLev);
        const double sseVar = fit(QStringLiteral("VarPro"), reactions, data, betaVar);

        qInfo().noquote() << QString("[%1] SSE LevMar=%2 VarPro=%3")
                                 .arg(QTest::currentDataTag()).arg(sseLev, 0, 'g', 4).arg(sseVar, 0, 'g', 4);

        // Both must essentially annihilate the noise-free residual.
        QVERIFY2(std::isfinite(sseVar) && sseVar < 1e-8,
            qPrintable(QString("VarPro SSE %1 not ~0 (did not reach the true minimum)").arg(sseVar)));
        QVERIFY2(std::isfinite(sseLev) && sseLev < 1e-8,
            qPrintable(QString("LevMar SSE %1 not ~0").arg(sseLev)));

        // Same recovered constants (both vs each other and vs the truth), for every global.
        QCOMPARE(betaVar.size(), trueBetas.size());
        for (int k = 0; k < trueBetas.size(); ++k) {
            qInfo().noquote() << QString("  [%1] lg beta[%2] true=%3 LevMar=%4 VarPro=%5")
                                     .arg(QTest::currentDataTag()).arg(k).arg(trueBetas[k])
                                     .arg(betaLev[k], 0, 'g', 6).arg(betaVar[k], 0, 'g', 6);
            QVERIFY2(std::abs(betaVar[k] - betaLev[k]) < 1e-3,
                qPrintable(QString("global %1: VarPro %2 != LevMar %3").arg(k).arg(betaVar[k]).arg(betaLev[k])));
            QVERIFY2(std::abs(betaVar[k] - trueBetas[k]) < 1e-2,
                qPrintable(QString("global %1: VarPro %2 did not recover truth %3").arg(k).arg(betaVar[k]).arg(trueBetas[k])));
        }
        delete data;
    }
};

QTEST_MAIN(TestVarPro)
#include "test_varpro.moc"
