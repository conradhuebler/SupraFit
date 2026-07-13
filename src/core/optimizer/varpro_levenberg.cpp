/*
 * SupraFit - opt-in variable-projection (VarPro) Levenberg-Marquardt fit solver
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
 * Variable projection (separable non-linear least squares, Kaufman variant). The observable of the
 * supported titration models is LINEAR in the local parameters (chemical shifts / extinction
 * coefficients) once the global stability constants are fixed. So we optimise only the small
 * non-linear set — the enabled global parameters — with a self-contained damped Levenberg-Marquardt,
 * and at every residual evaluation project the linear locals out by masked least-squares
 * (AbstractModel::ProjectLinearParameters()). This shrinks the numerically-differentiated Jacobian
 * from (globals + all series×locals) columns to just the globals, removing the wasted speciation
 * re-solves the classic full-vector solver pays for every local parameter.
 *
 * Deliberately self-contained (no unsupported-Eigen module): the non-linear dimension is tiny (1-6
 * constants), which makes a hand-written LM both tractable and robust. Selected via the "FitSolver"
 * optimizer-config key; the classic solver stays the default/oracle. Claude Generated.
 *
 * CAVEAT — resampling on ill-determined directions. For the fit and for Cross-Validation, VarPro is
 * numerically identical to the classic solver (see test_varpro / test_varpro_cv: CV boxplots match
 * exactly). But the linear projection REGULARISES flat (ill-determined) directions: under Reduction
 * Analysis the classic joint optimiser lets a barely-populated higher complex drift (large scatter —
 * RA exposing the direction), whereas VarPro keeps the reduced-data fits nearer the optimum. Both
 * still flag the direction, but the RA/MC statistic there is solver-dependent by design — VarPro
 * reports a tighter distribution. Choose the solver accordingly: VarPro for speed, LevMar when the
 * uncertainty-exposing behaviour of RA/MC on flat directions must match the established reference.
 */

#include "src/global_config.h"

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QVector>

#include <Eigen/Dense>

#include <algorithm>
#include <cmath>
#include <vector>

#include "src/core/models/AbstractModel.h"

#include "src/core/libmath.h"

int VarProFit(QWeakPointer<AbstractModel> weak, QVector<double>& sse_history, QVector<QVector<double>>& parameter_history)
{
    QSharedPointer<AbstractModel> model = weak.toStrongRef();
    if (!model)
        return -1;

    model->CalculateStatistics(false);
    model->setFast(true);

    // The non-linear parameters are the enabled global parameters (species constants).
    std::vector<int> gidx;
    for (int k = 0; k < model->GlobalParameterSize(); ++k)
        if (model->GlobalParameter()->isChecked(0, k))
            gidx.push_back(k);
    const int n = static_cast<int>(gidx.size());
    if (n == 0)
        return 0;

    const QJsonObject config = model->getOptimizerConfig();
    const int MaxIter = config["MaxLevMarInter"].toInt();
    const double ErrorConvergence = config["ErrorConvergence"].toDouble();
    const double DeltaParameter = config["DeltaParameter"].toDouble();

    // Set the globals, project the linear locals (masked), recompute, and return the residual vector
    // (per active point) plus the global-only penalty — exactly the residual the classic functor forms.
    auto residualVector = [&](const Eigen::VectorXd& beta) -> Eigen::VectorXd {
        for (int i = 0; i < n; ++i)
            model->setGlobalParameter(beta(i), gidx[i]);
        model->ProjectLinearParameters();
        model->Calculate();
        const QList<double> err = model->getCalculatedAbsoluteErrors();
        const QList<double> pen = model->getPenalty();
        Eigen::VectorXd r(err.size());
        for (int i = 0; i < err.size(); ++i)
            r(i) = err[i] + (i < pen.size() ? pen[i] : 0.0);
        return r;
    };

    Eigen::VectorXd beta(n);
    for (int i = 0; i < n; ++i)
        beta(i) = model->GlobalParameter(gidx[i]);

    Eigen::VectorXd r = residualVector(beta);
    double sse = r.squaredNorm();

    double lambda = 1e-3;
    const double eps = 1e-6; // forward-difference step for the Jacobian
    bool converged = false;
    int iter = 0;
    for (; iter < MaxIter; ++iter) {
        // History (globals only) recorded at the start of the step, mirroring the classic solver.
        QVector<double> row;
        row.reserve(n);
        for (int i = 0; i < n; ++i)
            row << beta(i);
        parameter_history << row;
        sse_history << sse;

        // Forward-difference Jacobian of the projected residual w.r.t. the globals (m × n).
        const int m = static_cast<int>(r.size());
        Eigen::MatrixXd J(m, n);
        for (int i = 0; i < n; ++i) {
            Eigen::VectorXd bp = beta;
            const double h = eps * std::max(1.0, std::abs(beta(i)));
            bp(i) += h;
            J.col(i) = (residualVector(bp) - r) / h;
        }
        const Eigen::MatrixXd JtJ = J.transpose() * J;
        const Eigen::VectorXd Jtr = J.transpose() * r;
        const Eigen::VectorXd scale = JtJ.diagonal().cwiseMax(1e-12); // Marquardt diagonal scaling

        // Damped step with lambda backtracking: grow lambda until the step reduces the SSE.
        Eigen::VectorXd trialBeta = beta, trialR = r;
        double trialSse = sse;
        bool improved = false;
        for (int tries = 0; tries < 15; ++tries) {
            Eigen::MatrixXd A = JtJ;
            A.diagonal() += lambda * scale;
            const Eigen::VectorXd delta = A.ldlt().solve(-Jtr);
            const Eigen::VectorXd cand = beta + delta;
            const Eigen::VectorXd candR = residualVector(cand);
            const double candSse = candR.squaredNorm();
            if (std::isfinite(candSse) && candSse < sse) {
                trialBeta = cand;
                trialR = candR;
                trialSse = candSse;
                improved = true;
                lambda = std::max(lambda * 0.5, 1e-12);
                break;
            }
            lambda = std::min(lambda * 3.0, 1e12);
        }

        if (!improved)
            break; // sitting in a (local) minimum: no downhill step found

        const double sseChange = std::abs(sse - trialSse);
        const double stepNorm = (trialBeta - beta).cwiseAbs().sum();
        beta = trialBeta;
        r = trialR;
        sse = trialSse;

        if (sseChange <= ErrorConvergence && stepNorm <= DeltaParameter) {
            converged = true;
            ++iter;
            break;
        }
    }

    // Leave the model at the final optimum with the projected locals and a full (statistics) calculate.
    for (int i = 0; i < n; ++i)
        model->setGlobalParameter(beta(i), gidx[i]);
    model->ProjectLinearParameters();
    model->CalculateStatistics(true);
    model->setFast(false);
    model->Calculate();
    model->setConverged(converged);
    return iter;
}
