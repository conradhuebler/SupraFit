/*
 * <Yes another least-squares solver in use.>
 * Copyright (C) 2021 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global_config.h"

#ifdef experimental

#include "src/core/models/models.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtCore/QtGlobal>
#include <QtCore/QtMath>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <external/least-squares-cpp/include/lsqcpp.h>

#include <iostream>

#include "src/core/libmath.h"

typedef QList<qreal> Variables;

// Implement an objective functor.
struct ParabolicError {
    void operator()(const Eigen::VectorXd& xval,
        Eigen::VectorXd& fval,
        Eigen::MatrixXd&) const
    {
        // omit calculation of jacobian, so finite differences will be used
        // to estimate jacobian numerically
        fval.resize(ModelSignals.size());
        //   for(lsq::Index i = 0; i < fval.size(); ++i)
        //       fval(i) = xval(i*2) * xval(i*2) + xval(i*2+1) * xval(i*2+1);

        //std::cout << xval << std::endl;
        QVector<qreal> param(inputs());
        for (int i = 0; i < inputs(); ++i)
            param[i] = xval(i);
        // qDebug() << param;
        model.data()->setParameter(param);
        model.data()->Calculate();
        Variables CalculatedSignals = model.data()->getCalculatedModel();
        for (int i = 0; i < ModelSignals.size(); ++i)
            fval(i) = CalculatedSignals[i] - ModelSignals[i];
    }
    int no_parameter;
    int no_points;
    int m_potenz;
    Variables ModelSignals;
    QSharedPointer<AbstractModel> model;
    int inputs() const { return no_parameter; }
    int values() const { return no_points; }
};

int LeastSquaresRookfighter(QWeakPointer<AbstractModel> model, QVector<qreal>& param)
{

#ifndef extended_f_test
    model.toStrongRef()->CalculateStatistics(false);
    model.toStrongRef()->setFast(true);
#endif

    QList<int> locked = model.toStrongRef()->LockedParameters();

    QJsonObject config = model.toStrongRef()->getOptimizerConfig();

    int MaxIter = config["MaxLevMarInter"].toInt();
    double ErrorConvergence = config["ErrorConvergence"].toDouble();
    double DeltaParameter = config["DeltaParameter"].toDouble();

    Variables ModelSignals = model.toStrongRef()->getSignals(model.toStrongRef()->ActiveSignals());
    model.toStrongRef()->setFast();
    if (ModelSignals.size() == 0 || ModelSignals.size() < param.size())
        return -1;
    Eigen::VectorXd parameter(param.size());
    for (int i = 0; i < param.size(); ++i)
        parameter(i) = param[i];

    /*
    QString message = QString();
    message += "Starting Levenberg-Marquardt for " + QString::number(parameter.size()) + " parameters:\n";
    message += "Old vector : ";
    for (double d : param) {
        message += QString::number(d) + " ";
    }
    message += "\n";
    emit model.data()->Info()->Message(message, 5);
    */
    ParabolicError error;
    error.model = model;
    error.ModelSignals = ModelSignals;
    error.no_parameter = param.size();
    error.no_points = ModelSignals.size();
    // Create GradienDescent optimizer with Barzilai Borwein method
    lsq::LevenbergMarquardt<double, ParabolicError> optimizer;
    optimizer.setErrorFunction(error);

    // Set number of iterations for levenberg-marquardt.
    optimizer.setMaxIterationsLM(100);

    // Set number of iterations as stop criterion.
    optimizer.setMaxIterations(100);

    // Set the minimum length of the gradient.
    optimizer.setMinGradientLength(1e-6);

    // Set the minimum length of the step.
    optimizer.setMinStepLength(1e-6);

    // Set the minimum least squares error.
    optimizer.setMinError(0);

    // Turn verbosity on, so the optimizer prints status updates after each
    // iteration.
    optimizer.setVerbosity(0);

    // Set initial guess.

    /*
    MyFunctor functor(param.size(), ModelSignals.size());

    Eigen::NumericalDiff<MyFunctor> numDiff(functor);
    Eigen::LevenbergMarquardt<Eigen::NumericalDiff<MyFunctor>> lm(numDiff);
    int iter = 0;

    lm.parameters.factor = config["LevMar_Factor"].toInt(); //step bound for the diagonal shift, is this related to damping parameter, lambda?
    lm.parameters.maxfev = config["LevMar_MaxFEv"].toDouble(); //max number of function evaluations
    lm.parameters.xtol = config["LevMar_Xtol"].toDouble(); //tolerance for the norm of the solution vector
    lm.parameters.ftol = config["LevMar_Ftol"].toDouble(); //tolerance for the norm of the vector function
    lm.parameters.gtol = config["LevMar_Gtol"].toDouble(); // tolerance for the norm of the gradient of the error vector
    lm.parameters.epsfcn = config["LevMar_epsfcn"].toDouble(); //error precision

    Eigen::LevenbergMarquardtSpace::Status status = lm.minimizeInit(parameter);
    qreal error_0 = 0;
    qreal error_2 = 1;
    qreal norm = 1;
    */
    /*
    QVector<qreal> globalConstants;
    for (; iter < MaxIter && ((qAbs(error_0 - error_2) > ErrorConvergence) || norm > DeltaParameter); ++iter) {
        globalConstants.clear();
        globalConstants = model.toStrongRef()->OptimizeParameters();
        error_0 = model.toStrongRef()->SSE();
#pragma message("this used to be not here before restructuring")
        model.toStrongRef()->setLockedParameter(locked);
        status = lm.minimizeOneStep(parameter);
        error_2 = model.toStrongRef()->SSE();

        auto constants = model.toStrongRef()->OptimizeParameters();
        norm = 0;
        for (int i = 0; i < globalConstants.size(); ++i)
            norm += qAbs(globalConstants[i] - constants[i]);
    }
    */

    // Start the optimization.
    auto result = optimizer.minimize(parameter);

    //std::cout << "Done! Converged: " << (result.converged ? "true" : "false")
    //    << " Iterations: " << result.iterations << std::endl;

    // do something with final function value
    // std::cout << "Final fval: " << result.fval.transpose() << std::endl;

    // do something with final x-value
    // std::cout << "Final xval: " << result.xval.transpose() << std::endl;

    /*
    QString result;
    result += "Levenberg-Marquardt returned in  " + QString::number(iter) + " iter, sumsq " + QString::number(model.data()->ModelError()) + "\n";
    result += "Last Sum of Changes in complexation constants was " + QString::number(norm) + "\n";
    result += "New vector:";
    for (int i = 0; i < param.size(); ++i) {
        result += QString::number(param[i]) + " ";
    }
    result += "\n";

    emit model.data()->Info()->Message(result, 4);
    */
    for (int i = 0; i < result.xval.size(); ++i)
        param[i] = result.xval(i);
    model.toStrongRef()->setConverged(result.iterations < MaxIter);
    return result.iterations;
}
#endif
