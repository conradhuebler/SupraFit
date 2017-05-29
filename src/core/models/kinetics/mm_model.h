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

#ifndef MM_Model_H
#define MM_Model_H

#include "src/global.h"
#include "src/core/AbstractModel.h"


#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/dataclass.h"

class Michaelis_Menten_Model : public AbstractModel 
{
    Q_OBJECT
    
public:
    Michaelis_Menten_Model(const DataClass *data);
    Michaelis_Menten_Model(const AbstractModel *model);
    ~Michaelis_Menten_Model();
    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type) override;
    inline int GlobalParameterSize() const override { return 2;} 
    virtual void InitialGuess() override;
    virtual QSharedPointer<AbstractModel > Clone() const override;
    virtual bool SupportThreads() const override { return false; }

    /*! \brief we have only the time as input parameter
     */
    virtual inline int InputParameterSize() const override { return 1; } 
    virtual inline QString GlobalParameterName(int i = 0) const override 
    { 
        if( i== 0)
            return tr("v<sub>max</sub>");
        else if( i == 1)
            return tr("K<sub>m</sub>");
        else 
            return QString();
    }
    
    virtual int LocalParameterSize() const override {return 0; }
[[deprecated]]
    inline virtual void MiniShifts() override { return; }
    
private:
    
    
protected:
    virtual void CalculateVariables() override;
    qreal m_vmax, m_Km;
};

#endif // MM_Model
