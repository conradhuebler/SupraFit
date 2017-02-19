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

#ifndef ItoI_Model_H
#define ItoI_Model_H

#include "src/global.h"
#include "src/core/AbstractModel.h"


#include <QDebug>
#include <QtCore/QObject>
#include <QVector>

#include <chaiscript/chaiscript.hpp>


#include "src/core/dataclass.h"

class ItoI_Model : public AbstractTitrationModel 
{
    Q_OBJECT
    
public:
    ItoI_Model(const DataClass *data);
    ItoI_Model(const AbstractTitrationModel *model);
    ~ItoI_Model();
    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type);
//     QPair<qreal, qreal> Pair(int i, int j = 0) const;
    inline int ConstantSize() const { return 1;}
//     void setPureSignals(const QList< qreal > &list);
//     void setComplexSignals(const QList< qreal > &list, int i);
    virtual void CalculateSignal(const QList<qreal > &constants);
    virtual void InitialGuess();
    virtual QSharedPointer<AbstractTitrationModel > Clone() const;
    virtual bool SupportThreads() const { return false; }
    virtual qreal BC50();
    
private:
    inline qreal HostConcentration(qreal host_0, qreal guest_0) {return HostConcentration(host_0, guest_0, Constants());}
    qreal HostConcentration(qreal host_0, qreal guest_0, const QList<qreal > &constants);
    
    
protected:
    QList<qreal > m_ItoI_signals;
    qreal m_K11;
};

/*
class ItoI_Model_Script : public ItoI_Model
{
  Q_OBJECT
  
public:
    ItoI_Model_Script(const DataClass *data);
    virtual void CalculateSignal(const QList<qreal > &constants);

private:
    std::string m_content, m_content_2;
    chaiscript::ChaiScript *chai;
};*/

#endif // 1_1_Model
