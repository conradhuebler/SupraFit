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

#include "src/core/AbstractModel.h"
#include "src/global.h"

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/dataclass.h"

class AbstractItcModel : public AbstractModel {
    Q_OBJECT

public:
    enum {
        CellVolume = 1,
        CellConcentration = 2,
        SyringeConcentration = 3,
        Temperature = 4
    };

    enum {
        Binding = 1,
        Dilution = 2,
        Reservoir = 3
    };

    AbstractItcModel(DataClass* data);
    AbstractItcModel(AbstractItcModel* data);

    ~AbstractItcModel();

    virtual qreal BC50() const { return 0; }
    virtual inline qreal BC50SF() const { return BC50(); }

    virtual qreal PrintOutIndependent(int i, int format) const override;

    inline qreal InitialHostConcentration(int i) const { return m_c0->data(1, i); }
    inline qreal InitialGuestConcentration(int i) const { return m_c0->data(2, i); }
    QString Model2Text_Private() const;

    virtual QString SpeciesName(int i) const { Q_UNUSED(i)
        return QString(); }
    virtual inline QString GlobalParameterPrefix(int i = 0) const override
    {
        Q_UNUSED(i)
        return QString("10^");
    }
    inline void setConcentrations(const QPointer<DataTable> table)
    {
        m_c0 = new DataTable(table);
        m_lock_concentrations = true;
    }
    inline QPointer<DataTable> ConcentrationTable() const { return m_c0; }
    inline double getV() const { return getSystemParameter(CellVolume).Double(); }
    inline double getCellConcentration() const { return m_cell_concentration; }
    inline double getSyringeConcentration() const { return m_syringe_concentration; }
    inline double getT() const { return m_T; }
    virtual inline bool SupportSeries() const override { return false; }

    /*! \brief Return a formated value as string of the global parameter with the value
     */
    virtual QString formatedGlobalParameter(qreal value, int globalParamater = 0) const
    {
        Q_UNUSED(globalParamater)
        return QString::number(qPow(10, value));
    }

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const override { return "G<sub>0</sub>/H<sub>0</sub>"; }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const override { return "q"; }

    virtual QString ModelInfo() const override;

    virtual void UpdateOption(int index, const QString& str) override;

public slots:
    virtual void UpdateParameter();

private:
    void virtual DeclareSystemParameter() override;
    void virtual DeclareOptions() override;
    bool m_lock_concentrations;
    QMutex m_lock;

private slots:
    void CalculateConcentrations();

protected:
    void SetConcentration(int i, const Vector& equilibrium);
    QPointer<DataTable> m_c0, m_concentrations;
    void Concentration() { CalculateConcentrations(); }
    double m_V, m_cell_concentration, m_syringe_concentration, m_T;
};
