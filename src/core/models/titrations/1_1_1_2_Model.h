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

#ifndef ItoI_ItoII_Model_H
#define ItoI_ItoII_Model_H

#include "src/global.h"
#include "src/core/AbstractModel.h"

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/dataclass.h"

class ItoI_ItoII_Model : public AbstractTitrationModel
{
     Q_OBJECT
    
public:
    ItoI_ItoII_Model(const DataClass* data);
    ~ItoI_ItoII_Model();
    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type) override;
    inline int GlobalParameterSize() const override { return 2;}
    virtual void InitialGuess() override;
    virtual QSharedPointer<AbstractModel > Clone() const override;
    virtual bool SupportThreads() const override { return false; }
    virtual qreal BC50() override;
    
private:
    inline qreal HostConcentration(qreal host_0, qreal guest_0) 
    {
        return HostConcentration(host_0, guest_0, GlobalParameter());
    }
    qreal HostConcentration(qreal host_0, qreal guest_0, const QList<qreal > &constants);
    qreal GuestConcentration(qreal host_0, qreal guest_0, const QList<qreal > &constants);
    
    qreal m_K11, m_K12;
    QList<qreal > m_ItoI_signals, m_ItoII_signals;
    static qreal Y(qreal x, const QVector<qreal > & parameter);
protected:
    virtual void CalculateVariables(const QList<qreal > &constants) override;
};

#endif // 2_1_1_1_MODEL_H
