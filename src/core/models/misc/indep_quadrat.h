/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

class Indep_Quadrat : public AbstractModel {
    Q_OBJECT

public:
    Indep_Quadrat(DataClass* data);
    Indep_Quadrat(AbstractModel* data);

    enum {
        Order = 1
    };

    virtual ~Indep_Quadrat() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::Indep_Quadrat; }

    inline int GlobalParameterSize() const override { return 2; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;
    virtual bool SupportThreads() const override { return false; }

    /*! \brief we have only the time as input parameter
     */
    virtual inline int InputParameterSize() const override { return 1; }
    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i == 0)
            return QString("%1%2").arg(Unicode_theta).arg(Unicode_Sub_1);
        else if (i == 1)
            return QString("%1%2").arg(Unicode_theta).arg(Unicode_Sub_2);
        else
            return QString();
    }

    virtual void DeclareOptions() override;

    virtual int LocalParameterSize(int i = 0) const override
    {
        Q_UNUSED(i)
        return 0;
    }

    virtual qreal PrintOutIndependent(int i) const override
    {
        return IndependentModel()->data(i);
    }

    virtual inline bool SupportSeries() const override { return false; }

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const override { return QString("x"); }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const override { return QString("y"); }

private:
protected:
    virtual void CalculateVariables() override;
    qreal m_a, m_b;
};
