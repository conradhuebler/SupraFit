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
    qreal SumOfErrors(int i) const;
    virtual QPair<qreal, qreal> Pair(int i, int j = 0) = 0;
    inline qreal PureSignal(int i) const 
        { 
            if(i >= MaxVars())
                return 0;
            return m_pure_signals[i]; 
        }
    virtual int ConstantSize() const = 0;
    QVector< QPointer< QtCharts::QScatterSeries > > Signals(int c = 3) const;
    QVector< QPointer< QtCharts::QLineSeries > > Model(int c = 1) const;
    QVector< QPointer< QtCharts::QLineSeries > > ErrorBars(int c = 1) const;
    virtual void setPureSignals(QVector< qreal > list) = 0;
    virtual void setComplexSignals(QVector< qreal > list, int i) = 0;
    virtual void setConstants(QVector< qreal > list) = 0;
    virtual void CalculateSignal() = 0;
    virtual qreal Minimize(QVector<int > vars) = 0;
    virtual QVector<qreal > Constants() const = 0;
    inline QString Name() const { return m_name; } 
private:
    virtual qreal GuestConcentration(qreal host_0, qreal guest_0) = 0;
    
protected:
    inline void setName(const QString &str) { m_name = str; }
    QString m_name;
    //QVector< QVector<qreal > >m_pure_signals;
    QVector<qreal > m_pure_signals;
    QHash<const DataPoint *, QVector< qreal> > m_signals;
    QVector< QVector < qreal > > m_difference; 
    bool *ptr_concentrations;
    bool m_repaint;
signals:
    void Recalculated();
};


class ItoI_Model : public AbstractTitrationModel 
{
    Q_OBJECT
    
public:
    ItoI_Model(const DataClass *data, QObject *parent = 0);
    ~ItoI_Model();
    QPair<qreal, qreal> Pair(int i, int j = 0);
    inline int ConstantSize() const { return 1;}
    void setPureSignals(QVector< qreal > list);
    void setComplexSignals(QVector< qreal > list, int i);
    void setConstants(QVector< qreal > list);
    void CalculateSignal();
    qreal Minimize(QVector<int > vars);
    QVector<qreal > Constants() const { return QVector<qreal>() << m_K11; }
private:
    qreal GuestConcentration(qreal host_0, qreal guest_0);
    qreal ComplexConcentration(qreal host_0, qreal guest_0);
    qreal m_K11;
    
//     QVector< QVector<qreal > >m_ItoI_signals;
    QVector<qreal > m_ItoI_signals;
};

class IItoI_ItoI_Model : public AbstractTitrationModel
{
     Q_OBJECT
    
public:
    IItoI_ItoI_Model(const DataClass* data, QObject* parent = 0);
    ~IItoI_ItoI_Model();
    
    QPair<qreal, qreal> Pair(int i, int j = 0);
    inline int ConstantSize() const { return 2;}
    void setPureSignals(QVector< qreal > list);
    void setComplexSignals(QVector< qreal > list, int i);
    void setConstants(QVector< qreal > list);
    void CalculateSignal();
    qreal Minimize(QVector<int > vars);
    QVector<qreal > Constants() const { return QVector<qreal>() << m_K21 << m_K11; }
private:
    qreal GuestConcentration(qreal host_0, qreal guest_0);
    qreal HostConcentration(qreal host_0, qreal guest_0);
    
    
    qreal m_K21, m_K11;
    QVector<qreal > m_ItoI_signals, m_IItoI_signals;
    
};

#endif // MODELCLASS_H
