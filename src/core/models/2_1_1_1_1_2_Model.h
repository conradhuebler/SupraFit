/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifndef IItoI_ItoI_ItoII_Model_H
#define IItoI_ItoI_ItoII_Model_H

#include "src/global.h"
#include "src/core/AbstractModel.h"

#include <QDebug>
#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QVector>

#include "src/core/dataclass.h"

typedef Eigen::VectorXd Vector;

class ConcentrationSolver;

class IItoI_ItoI_ItoII_Model : public AbstractTitrationModel
{
     Q_OBJECT
    
public:
    IItoI_ItoI_ItoII_Model(const DataClass* data);
    ~IItoI_ItoI_ItoII_Model();
    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type);
    inline int ConstantSize() const { return 3;}
    void CalculateSignal(const QList<qreal > &constants); 
    virtual void InitialGuess();
    virtual QSharedPointer<AbstractTitrationModel > Clone() const;
    virtual bool SupportThreads() const { return true; }
//     virtual Vector MassBalance(qreal A, qreal B);
private:
    qreal m_K21, m_K11, m_K12;
    QList<qreal > m_IItoI_signals, m_ItoI_signals, m_ItoII_signals;
    QList<QPointer<ConcentrationSolver > > m_solvers;
    QList<qreal> m_constants_pow;
};

#endif // 2_1_1_1_MODEL_H
