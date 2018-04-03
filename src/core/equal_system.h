/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/NonLinearOptimization>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QRunnable>

#ifndef EQUAL_H
#define EQUAL_H

class AbstractTitrationModel;

typedef Eigen::VectorXd Vector;

template <typename _Scalar, int NX = Eigen::Dynamic, int NY = Eigen::Dynamic>
struct ScriptedEqualSystem {
    typedef _Scalar Scalar;
    enum {
        InputsAtCompileTime = NX,
        ValuesAtCompileTime = NY
    };
    typedef Eigen::Matrix<Scalar, InputsAtCompileTime, 1> InputType;
    typedef Eigen::Matrix<Scalar, ValuesAtCompileTime, 1> ValueType;
    typedef Eigen::Matrix<Scalar, ValuesAtCompileTime, InputsAtCompileTime> JacobianType;

    int m_inputs, m_values;

    inline ScriptedEqualSystem(int inputs, int values)
        : m_inputs(inputs)
        , m_values(values)
    {
    }

    int inputs() const { return m_inputs; }
    int values() const { return m_values; }
};

struct MyScripteEqualSystem : ScriptedEqualSystem<double> {
    inline MyScripteEqualSystem(int inputs, int values, QPointer<AbstractTitrationModel> model)
        : ScriptedEqualSystem(inputs, values)
        , no_parameter(inputs)
        , no_points(values)
        , m_model(model)
    {
    }
    int operator()(const Eigen::VectorXd& parameter, Eigen::VectorXd& fvec) const;
    QPointer<AbstractTitrationModel> m_model;
    Eigen::VectorXd Concen_0;
    int no_parameter;
    int no_points;
    int inputs() const { return no_parameter; } // There are two parameters of the model
    int values() const { return no_points; } // The number of observations
};

struct MyScripteEqualSystemNumericalDiff : Eigen::NumericalDiff<MyScripteEqualSystem> {
};

class ConcentrationSolver : public QObject, public QRunnable {
    Q_OBJECT
public:
    ConcentrationSolver(QPointer<AbstractTitrationModel> model);
    ~ConcentrationSolver();
    virtual void run();
    void setInput(double A_0, double B_0);
    inline QList<double> Concentrations() const { return m_concentration; }

private:
    qreal m_A_0, m_B_0;
    //     qreal complex_21, complex_11, complex_12;
    QPointer<AbstractTitrationModel> m_model;
    QList<double> m_concentration;
    MyScripteEqualSystem* functor;
    int SolveEqualSystem(double A_0, double B_0);
};

#endif
