/*
 * Inner filter and dynamic quenching of the fluorescence models - Claude Generated (2026), AI-implemented, not checked against the literature
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
 * The two corrections of fl_any (docs/fluorescence_models.md) are tested against data made by independent code in this file:
 *   inner filter:  F_obs = F * 10^(-(A_ex + A_em)/2),  A = d * sum_c eps_c * T_c   (totals T_c)
 *   quenching:     F_obs = F / (1 + K_SV [B]_free)
 * where F = sum over species c_i phi_i is the signal of the model without corrections. The fit with the correction must recover the constants,
 * the fit without it must be measurably biased (the test would pass trivially if the correction did nothing).
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
#include "src/core/models/titrations/fluorescence/fl_any_Model.h"
#include "src/global.h"

class TestFlCorrections : public QObject {
    Q_OBJECT

private:
    static constexpr int N = 21;
    static constexpr double Host = 1e-5;
    static constexpr double LgBeta = 4.0;

    struct Truth {
        DataClass* data = nullptr; ///< the data as they would be measured
        QVector<double> host, guest, freeGuest; ///< totals and the free guest of the model without corrections
    };

    static QSharedPointer<AbstractModel> flAny(DataClass* data)
    {
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::fl_any, data);
        QJsonObject def, r;
        r["value"] = QStringLiteral("A + B <=> AB");
        def["Reactions"] = r;
        model->DefineModel(def);
        return model;
    }

    // the signal of the model without corrections at lg beta = 4, phi = (host 2e8, guest 0.3e8, complex 6e8), two series (the second 0.8 times)
    static Truth makeTruth()
    {
        Truth t;
        Eigen::MatrixXd indep(N, 2);
        for (int i = 0; i < N; ++i) {
            indep(i, 0) = Host;
            indep(i, 1) = 5 * Host * i / (N - 1);
            t.host << indep(i, 0);
            t.guest << indep(i, 1);
        }
        DataClass* data = new DataClass();
        data->setIndependentTable(new DataTable(indep));
        data->setDataType(DataClassPrivate::Table);
        data->setSimulateDependent(2);
        data->setDependentTable(new DataTable(Eigen::MatrixXd::Zero(N, 2)));
        data->setDataBegin(0);
        data->setDataEnd(N);
        QSharedPointer<AbstractModel> model = flAny(data);
        model->InitialGuess();
        model->setGlobalParameter(LgBeta, 0);
        const double phi[3] = { 2e8, 0.3e8, 6e8 };
        for (int s = 0; s < 2; ++s)
            for (int p = 0; p < 3; ++p)
                model->setLocalParameter(phi[p] * (s ? 0.8 : 1.0), p, s);
        model->setFast(false);
        model->Calculate();
        Eigen::MatrixXd dep = model->ModelTable()->Table();
        data->setDependentTable(new DataTable(dep));
        data->setDataBegin(0);
        data->setDataEnd(N);
        AbstractTitrationModel* titration = qobject_cast<AbstractTitrationModel*>(model.data());
        for (int i = 0; i < N; ++i)
            t.freeGuest << titration->getConcentration(i)(2); // columns: point, free A, free B, AB
        t.data = data;
        return t;
    }

    static void scale(DataClass* data, const QVector<double>& factor)
    {
        Eigen::MatrixXd dep = data->DependentModel()->Table();
        for (int i = 0; i < N; ++i)
            dep.row(i) *= factor[i];
        data->setDependentTable(new DataTable(dep));
        data->setDataBegin(0);
        data->setDataEnd(N);
    }

    // fit lg beta (and lg K_SV) with the given options; returns lg beta, SSE relative to the signal energy
    static double fit(DataClass* data, bool innerFilter, const QString& quencher, double* lgSV, double* relSSE, const QString& solver = QStringLiteral("VarPro"))
    {
        QSharedPointer<AbstractModel> model = flAny(data);
        QJsonObject cfg = model->getOptimizerConfig();
        cfg["FitSolver"] = solver;
        model->setOptimizerConfig(cfg);
        model->setOption(fl_any_Model::InnerFilter, innerFilter ? "on" : "off");
        model->setOption(fl_any_Model::Quenching, quencher);
        if (innerFilter) {
            model->setSystemParameterValue(fl_any_Model::PathLength, 1.0);
            model->setSystemParameterValue(fl_any_Model::EpsilonExcitation + 0, 2e4);
            model->setSystemParameterValue(fl_any_Model::EpsilonExcitation + 1, 6e3);
            model->setSystemParameterValue(fl_any_Model::EpsilonEmission + 0, 1e3);
            model->setSystemParameterValue(fl_any_Model::EpsilonEmission + 1, 0.0);
        }
        model->InitialGuess();
        model->setGlobalParameter(3.5, 0);
        if (quencher != QLatin1String("none"))
            model->setGlobalParameter(4.0, 1);
        Minimizer m(false);
        m.setModel(model);
        m.Minimize();
        double energy = 0;
        for (int c = 0; c < 2; ++c)
            for (int r = 0; r < N; ++r)
                energy += std::pow(model->DependentModel()->data(r, c), 2);
        if (relSSE)
            *relSSE = model->SSE() / energy;
        if (lgSV)
            *lgSV = model->GlobalParameter(1);
        return model->GlobalParameter(0);
    }

private slots:
    // Inner filter: absorbances A_ex = d (eps_ex,A T_A + eps_ex,B T_B), A_em = d (eps_em,A T_A) from the totals; the data are attenuated by 10^(-(A_ex+A_em)/2).
    void innerFilterIsCorrected()
    {
        Truth t = makeTruth();
        QVector<double> factor;
        double maxAbsorbance = 0;
        for (int i = 0; i < N; ++i) {
            const double aEx = 1.0 * (2e4 * t.host[i] + 6e3 * t.guest[i]);
            const double aEm = 1.0 * (1e3 * t.host[i]);
            maxAbsorbance = std::max(maxAbsorbance, aEx + aEm);
            factor << std::pow(10.0, -(aEx + aEm) / 2);
        }
        scale(t.data, factor);
        qInfo().noquote() << QString("largest A_ex + A_em %1, signal attenuated to %2").arg(maxAbsorbance, 0, 'f', 3).arg(factor.last(), 0, 'f', 3);
        QVERIFY(factor.last() < 0.9); // the effect is there

        double rel = 0;
        const double corrected = fit(t.data, true, "none", nullptr, &rel);
        const double plain = fit(t.data, false, "none", nullptr, nullptr);
        qInfo().noquote() << QString("lg beta: truth 4.000, with the correction %1 (SSE/energy %2), without %3").arg(corrected, 0, 'f', 4).arg(rel, 0, 'g', 3).arg(plain, 0, 'f', 4);
        QVERIFY2(std::abs(corrected - LgBeta) < 1e-3, "the inner filter correction does not recover lg beta");
        QVERIFY2(rel < 1e-12, "the corrected model does not describe the attenuated data");
        QVERIFY2(std::abs(plain - LgBeta) > 0.02, "without the correction lg beta is not biased: the test cannot tell the correction from nothing");
        delete t.data;
    }

    // Dynamic quenching by the free guest: the data are divided by 1 + K_SV [B]; lg K_SV is fitted.
    void dynamicQuenchingIsFitted()
    {
        Truth t = makeTruth();
        const double lgSV = 5.0;
        QVector<double> factor;
        for (int i = 0; i < N; ++i)
            factor << 1.0 / (1.0 + std::pow(10.0, lgSV) * t.freeGuest[i]);
        scale(t.data, factor);
        qInfo().noquote() << QString("signal quenched to %1 at the end").arg(factor.last(), 0, 'f', 3);
        QVERIFY(factor.last() < 0.8);

        double rel = 0, fittedSV = 0;
        const double corrected = fit(t.data, false, "B", &fittedSV, &rel);
        const double plain = fit(t.data, false, "none", nullptr, nullptr);
        qInfo().noquote() << QString("lg beta: truth 4.000, with quenching %1 (lg K_SV %2, truth 5, SSE/energy %3), without %4").arg(corrected, 0, 'f', 4).arg(fittedSV, 0, 'f', 4).arg(rel, 0, 'g', 3).arg(plain, 0, 'f', 4);
        QVERIFY2(std::abs(corrected - LgBeta) < 5e-3, "the quenching term does not recover lg beta");
        QVERIFY2(std::abs(fittedSV - lgSV) < 5e-2, "the quenching term does not recover lg K_SV");
        QVERIFY2(rel < 1e-10, "the quenching model does not describe the quenched data");
        QVERIFY2(std::abs(plain - LgBeta) > 0.02, "without the quenching term lg beta is not biased: the test cannot tell the term from nothing");
        delete t.data;
    }

    // Both corrections at once, and the full-vector solver reaches the same optimum as VarPro (the corrections keep the signal linear in the coefficients).
    void bothCorrectionsAndBothSolvers()
    {
        Truth t = makeTruth();
        QVector<double> factor;
        for (int i = 0; i < N; ++i) {
            const double aEx = 2e4 * t.host[i] + 6e3 * t.guest[i], aEm = 1e3 * t.host[i];
            factor << std::pow(10.0, -(aEx + aEm) / 2) / (1.0 + std::pow(10.0, 5.0) * t.freeGuest[i]);
        }
        scale(t.data, factor);
        double sseVar = 0, sseLev = 0, svVar = 0, svLev = 0;
        const double var = fit(t.data, true, "B", &svVar, &sseVar, "VarPro");
        const double lev = fit(t.data, true, "B", &svLev, &sseLev, "LevMar");
        qInfo().noquote() << QString("VarPro lg beta %1 lg K_SV %2 SSE %3 | LevMar lg beta %4 lg K_SV %5 SSE %6").arg(var, 0, 'f', 4).arg(svVar, 0, 'f', 4).arg(sseVar, 0, 'g', 3).arg(lev, 0, 'f', 4).arg(svLev, 0, 'f', 4).arg(sseLev, 0, 'g', 3);
        QVERIFY(std::abs(var - LgBeta) < 5e-3 && std::abs(svVar - 5.0) < 5e-2);
        QVERIFY(sseVar < 1e-10 && sseLev < 1e-8);
        delete t.data;
    }

    // Without the options nothing changes: the signal factor is 1.
    void offIsTheOldModel()
    {
        Truth t = makeTruth();
        double rel = 0;
        const double lg = fit(t.data, false, "none", nullptr, &rel);
        QVERIFY(std::abs(lg - LgBeta) < 1e-3);
        QVERIFY(rel < 1e-12);
        delete t.data;
    }
};

QTEST_MAIN(TestFlCorrections)
#include "test_fl_corrections.moc"
