/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#pragma once

#include "src/global.h"
#include "src/core/AbstractModel.h"

#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QVector>
#include <QtCore/QThreadPool>
#include "src/core/dataclass.h"

typedef Eigen::VectorXd Vector;

struct MassResults;


class ConSolver : public QObject,  public QRunnable
{

public:
    ConSolver(QPointer<AbstractTitrationModel> model);
    ~ConSolver();
    virtual void run();
    void setInput(double A_0, double B_0);
    inline QPair<double, double> Concentrations() const { return  m_concentration; }

private:
    qreal m_A_0, m_B_0;
    QPair<double, double> HostConcentration(double a0, double b0);
//     qreal complex_21, complex_11, complex_12;
    QPointer<AbstractTitrationModel> m_model;
    QPair<double, double> m_concentration;
    int SolveEqualSystem(double A_0, double B_0);
};

class IItoI_ItoI_ItoII_Model : public AbstractTitrationModel
{
     Q_OBJECT
    
public:
    IItoI_ItoI_ItoII_Model(DataClass* data);
    ~IItoI_ItoI_ItoII_Model();
    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type) override;
    inline int GlobalParameterSize() const override { return 3;}
    virtual void InitialGuess() override;
    virtual QSharedPointer<AbstractModel > Clone() override;
    virtual bool SupportThreads() const override { return true; }
    virtual MassResults MassBalance(qreal A, qreal B) override;
    virtual inline QString GlobalParameterName(int i = 0) const override 
    { 
        if(i == 0)
            return tr("K<sub>21</sub>");
        else if( i == 1)
            return  tr("K<sub>11</sub>");
        else if( i == 2 )
            return  tr("K<sub>12</sub>");
        else 
            return QString();
    }
    virtual void DeclareOptions() override;
    virtual inline QString Name() const override { return tr("2:1/1:1/1:2-Model"); }

    virtual inline int Color(int i) const override {  if(i > 2) return i + 1; return i; }



private:
    qreal m_K21, m_K11, m_K12;
    QList<qreal > m_IItoI_signals, m_ItoI_signals, m_ItoII_signals;
    QList<QPointer<ConSolver > > m_solvers;
    QList<qreal> m_constants_pow;
    QThreadPool *m_threadpool;
    
protected:
    virtual void CalculateVariables() override;
};
