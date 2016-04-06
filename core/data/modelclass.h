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

#ifndef MODELCLASS_H
#define MODELCLASS_H

#include <QtCore/qobject.h>
#include <QVector>
#include <QtCharts/QLineSeries>
#include "dataclass.h"
class AbstractTitrationModel : public QObject , protected DataClass
{
    Q_OBJECT

public:
    AbstractTitrationModel(const DataClass *data, QObject *parent = 0);
    ~AbstractTitrationModel();
    int MaxVars() const { return (m_pure_signals.size()); }
    bool MinimizeConstants();
    bool MinimizeSignals();
    
    virtual QPair<qreal, qreal> Pair(int i, int j = 0) = 0;
    inline qreal PureSignal(int i) const 
        { 
            if(i >= MaxVars())
                return 0;
            return m_pure_signals[i]; 
        }
    virtual int ConstantSize() const = 0;
    QVector< QPointer< QtCharts::QLineSeries > > Signals(int c = 1) const;
    virtual void setPureSignals(QVector< qreal > list) = 0;
    virtual void setComplexSignals(QVector< qreal > list, int i) = 0;
    virtual void setConstants(QVector< qreal > list) = 0;
    virtual void CalculateSignal() = 0;
    
public slots:
    inline void SwitchConentrations() { m_concentrations = !m_concentrations; }
private:
    virtual qreal Minimize(QVector<int > vars) = 0;
    
    
    virtual qreal GuestConcentration(qreal host_0, qreal guest_0) = 0;
protected:
    //QVector< QVector<qreal > >m_pure_signals;
    QVector<qreal > m_pure_signals;
    QHash<const DataPoint *, QVector< qreal> > m_signals;
    bool m_concentrations;
};


class ItoIModel : public AbstractTitrationModel 
{
    Q_OBJECT
    
public:
    ItoIModel(const DataClass *data, QObject *parent = 0);
    ~ItoIModel();
    QPair<qreal, qreal> Pair(int i, int j = 0);
    inline int ConstantSize() const { return 1;}
    void setPureSignals(QVector< qreal > list);
    void setComplexSignals(QVector< qreal > list, int i);
    void setConstants(QVector< qreal > list);
    void CalculateSignal();
private:
    
    
    qreal Minimize(QVector<int > vars);
    
    qreal GuestConcentration(qreal host_0, qreal guest_0);
    qreal ComplexConcentration(qreal host_0, qreal guest_0);
    qreal m_K11;
    
//     QVector< QVector<qreal > >m_ItoI_signals;
    QVector<qreal > m_ItoI_signals;
};
/*
class IItoIModel : public ItoIModel
{
     Q_OBJECT
    
public:
    IItoIModel(const DataClass* data, QObject* parent = 0);
    
private:
    
    
};
*/
#endif // MODELCLASS_H
