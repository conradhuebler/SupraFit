/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

class DecayRates : public AbstractModel {
    Q_OBJECT

public:
    DecayRates(DataClass* data);
    DecayRates(AbstractModel* data);

    virtual ~DecayRates() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::DecayRates; }

    inline int GlobalParameterSize() const override { return 4; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;
    virtual bool SupportThreads() const override { return false; }

    /*! \brief we have only the time as input parameter
     */
    virtual inline int InputParameterSize() const override { return 1; }
    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        return QString("tau%1").arg(1 + i);
    }
    virtual void DeclareOptions() override;
    virtual void EvaluateOptions() override;
    virtual void OptimizeParameters_Private() override;
    virtual inline QString LocalParameterName(int i = 0) const override
    {
        return QString("B%1").arg(1 + i);
    }

    virtual int LocalParameterSize(int i = 0) const override
    {
        Q_UNUSED(i)
        return 4;
    }

    virtual qreal PrintOutIndependent(int i) const override
    {
        return IndependentModel()->data(i);
    }

    virtual inline bool SupportSeries() const override { return false; }

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const override { return S0; }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const override { return "v"; }

private:
protected:
    virtual void CalculateVariables() override;
    int m_z = 0, m_d = 0, m_v = 0;
};
