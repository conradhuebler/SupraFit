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

#ifndef MM_Model_H
#define MM_Model_H

#include "src/global.h"
#include "src/core/AbstractModel.h"


#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/dataclass.h"

class Michaelis_Menten_Model : public AbstractTitrationModel 
{
    Q_OBJECT
    
public:
    Michaelis_Menten_Model(const DataClass *data);
    Michaelis_Menten_Model(const AbstractTitrationModel *model);
    ~Michaelis_Menten_Model();
    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type);
    inline int ConstantSize() const { return 2;}
    virtual void InitialGuess();
    virtual QSharedPointer<AbstractModel > Clone() const;
    virtual bool SupportThreads() const { return false; }
    virtual QPair<qreal, qreal> Pair(int i, int j = 0) const;
private:
    
    
protected:
    virtual void CalculateVariables(const QList<qreal > &constants);
    
    QList<qreal > m_ItoI_signals;
    qreal m_vmax, m_Km;
};

#endif // MM_Model
