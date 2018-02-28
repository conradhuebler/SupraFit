/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/dataclass.h"

class AbstractItcModel : public AbstractModel
{
    Q_OBJECT
    
public:
    enum {
        CellVolume = 1,
        CellConcentration = 2,
        SyringeConcentration = 3,
        Temperature = 4
    };

    enum Option {
        Binding = 1,
        Dilution = 2
    };

    AbstractItcModel(DataClass *data);
    ~AbstractItcModel();

    virtual qreal BC50() const = 0;
    virtual inline qreal BC50SF() const { return BC50(); }

    virtual qreal PrintOutIndependent(int i, int format) const override;

    inline qreal InitialHostConcentration(int i) const { return m_c0->data(0,i);  }
    inline qreal InitialGuestConcentration(int i) const  { return m_c0->data(1,i);  }
    QString Model2Text_Private() const;

    virtual QString SpeciesName(int i) const = 0;
    virtual inline QString GlobalParameterPrefix(int i = 0) const override  { Q_UNUSED(i) return QString("10^");  }
    inline void setConcentrations(const QPointer<DataTable> table) { m_c0 = new DataTable(table); m_lock_concentrations = true; }
    inline QPointer<DataTable> ConcentrationTable() const { return m_c0; }

private:
    void virtual DeclareSystemParameter() override;
    void virtual DeclareOptions() override;
    bool m_dirty, m_lock_concentrations;
    QMutex m_lock;
private slots:
    void CalculateConcentrations();

protected:
    void SetConcentration(int i, const Vector& equilibrium);
    QPointer<DataTable > m_c0, m_concentrations;
    void Concentration() { CalculateConcentrations(); }
};

