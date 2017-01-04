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

#include "src/core/AbstractModel.h"


#include <QDebug>
#include <QtCore/qobject.h>
#include <QVector>
#include <QtCharts/QLineSeries>
#include <QtCharts/QVXYModelMapper>

#include "src/core/dataclass.h"

class QStandardItemModel;

class ItoI_Model : public AbstractTitrationModel 
{
    Q_OBJECT
    
public:
    ItoI_Model(const DataClass *data);
    ~ItoI_Model();
    QPair<qreal, qreal> Pair(int i, int j = 0);
    inline int ConstantSize() const { return 1;}
    void setPureSignals(const QVector< qreal > &list);
    void setComplexSignals(QVector< qreal > list, int i);
    void setConstants(QVector< qreal > list);
    void CalculateSignal(QVector<qreal > constants = QVector<qreal>());
    QVector<qreal > Constants() const { return QVector<qreal>() << m_K11; }
    virtual QVector< QVector< qreal > > AllShifts();
    virtual void InitialGuess();
private:
    void MiniShifts();
    inline qreal HostConcentration(qreal host_0, qreal guest_0) {return HostConcentration(host_0, guest_0, Constants());}
    qreal HostConcentration(qreal host_0, qreal guest_0, QVector<qreal > constants);
    qreal m_K11;
    
    QVector<qreal > m_ItoI_signals;
};

#endif // 1_1_Model
