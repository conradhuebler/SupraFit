/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/dataclass.h"

class Michaelis_Menten_Model : public AbstractModel {
    Q_OBJECT

public:
    Michaelis_Menten_Model(DataClass* data);
    ~Michaelis_Menten_Model();

    virtual inline SupraFit::Model SFModel() const { return SupraFit::Michaelis_Menten; }

    virtual QVector<qreal> OptimizeParameters_Private(OptimizationType type) override;
    inline int GlobalParameterSize() const override { return 2; }
    virtual void InitialGuess() override;
    virtual QSharedPointer<AbstractModel> Clone() override;
    virtual bool SupportThreads() const override { return false; }

    /*! \brief we have only the time as input parameter
     */
    virtual inline int InputParameterSize() const override { return 1; }
    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i == 0)
            return tr("v<sub>max</sub>");
        else if (i == 1)
            return tr("K<sub>m</sub>");
        else
            return QString();
    }

    virtual int LocalParameterSize() const override { return 0; }

    virtual qreal PrintOutIndependent(int i, int format = 0) const override
    {
        Q_UNUSED(format)
        return IndependentModel()->data(0, i);
    }

    //  virtual inline QString Name() const override { return tr("Michaelis Menten"); }
    virtual inline bool SupportSeries() const override { return false; }

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const override { return "S<sub>0</sub>"; }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const override { return "v"; }

private:
protected:
    virtual void CalculateVariables() override;
    qreal m_vmax, m_Km;
};
