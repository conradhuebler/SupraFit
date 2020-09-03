/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/models.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtCore/QtGlobal>
#include <QtCore/QtMath>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/NonLinearOptimization>

#include <iostream>

#include "src/core/libmath.h"
typedef QList<qreal> Variables;

template <typename _Scalar, int NX = Eigen::Dynamic, int NY = Eigen::Dynamic>

struct Functor {
    typedef _Scalar Scalar;
    enum {
        InputsAtCompileTime = NX,
        ValuesAtCompileTime = NY
    };
    typedef Eigen::Matrix<Scalar, InputsAtCompileTime, 1> InputType;
    typedef Eigen::Matrix<Scalar, ValuesAtCompileTime, 1> ValueType;
    typedef Eigen::Matrix<Scalar, ValuesAtCompileTime, InputsAtCompileTime> JacobianType;

    int m_inputs, m_values;

    inline Functor(int inputs, int values)
        : m_inputs(inputs)
        , m_values(values)
    {
    }

    int inputs() const { return m_inputs; }
    int values() const { return m_values; }
};

struct MyFunctor : Functor<double> {
    inline MyFunctor(int inputs, int values)
        : Functor(inputs, values)
        , no_parameter(inputs)
        , no_points(values)
    {
    }
    inline ~MyFunctor() {}
    inline int operator()(const Eigen::VectorXd& parameter, Eigen::VectorXd& fvec) const
    {
        QVector<qreal> param(inputs());
        for (int i = 0; i < inputs(); ++i)
            param[i] = parameter(i);
        // qDebug() << param;
        model.data()->setParameter(param);
        model.data()->Calculate();
        Variables CalculatedSignals = model.data()->getCalculatedModel();
        for (int i = 0; i < ModelSignals.size(); ++i)
            fvec(i) = CalculatedSignals[i] - ModelSignals[i];

        return 0;
    }
    int no_parameter;
    int no_points;
    int m_potenz;
    Variables ModelSignals;
    QSharedPointer<AbstractModel> model;
    int inputs() const { return no_parameter; }
    int values() const { return no_points; }
};

struct MyFunctorNumericalDiff : Eigen::NumericalDiff<MyFunctor> {
};

int NonlinearFit(QWeakPointer<AbstractModel> model, QVector<qreal>& param)
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
    MyFunctor functor(param.size(), ModelSignals.size());
    functor.model = model;
    functor.ModelSignals = ModelSignals;
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
    for (int i = 0; i < functor.inputs(); ++i)
        param[i] = parameter(i);
    model.toStrongRef()->setConverged(iter < MaxIter);
    return iter;
}
