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

#ifndef II_ItoI_ItoI_Model_H
#define II_ItoI_ItoI_Model_H

#include "src/global.h"
#include "src/core/AbstractModel.h"

#include <QtCore/QObject>
#include <QtCore/QVector>


#include "src/core/dataclass.h"


class IItoI_ItoI_Model : public AbstractTitrationModel
{
     Q_OBJECT
    
public:
    IItoI_ItoI_Model(const DataClass* data);
    ~IItoI_ItoI_Model();
    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type) override;
    inline int GlobalParameterSize() const override { return 2;}
    
    virtual void InitialGuess() override;
    virtual QSharedPointer<AbstractModel > Clone() const override;
    virtual bool SupportThreads() const override { return false; }
    virtual qreal BC50() override;
    virtual inline QStringList GlobalParameterNames() const override { return QStringList() << tr("K<sub>21</sub>") << tr("K<sub>11</sub>"); }
private:
    inline qreal HostConcentration(qreal host_0, qreal guest_0) {return HostConcentration(host_0, guest_0, GlobalParameter());}
    qreal HostConcentration(qreal host_0, qreal guest_0, const QList<qreal > &constants);
    
protected:
    virtual void CalculateVariables(const QList<qreal > &constants) override;
    
    qreal m_K21, m_K11;
    QList<qreal > m_ItoI_signals, m_IItoI_signals;
};


#endif // 1-1_1-2-MODEL_H