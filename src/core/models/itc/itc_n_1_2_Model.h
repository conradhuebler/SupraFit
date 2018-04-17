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

#include "src/core/bc50.h"
#include "src/global.h"

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/AbstractItcModel.h"
#include "src/core/dataclass.h"

class itc_n_ItoII_Model : public AbstractItcModel {
    Q_OBJECT

public:
    itc_n_ItoII_Model(DataClass* data);
    itc_n_ItoII_Model(AbstractItcModel* data);

    ~itc_n_ItoII_Model();

    virtual inline SupraFit::Model SFModel() const { return SupraFit::itc_n_ItoII; }

    virtual QVector<qreal> OptimizeParameters_Private(OptimizationType type) override;
    inline int GlobalParameterSize() const override { return 2; }
    virtual void InitialGuess() override;
    virtual QSharedPointer<AbstractModel> Clone() override;
    virtual bool SupportThreads() const override { return false; }

    virtual inline qreal BC50() const override { return BC50::ItoI_BC50(GlobalParameter(0)); }
    virtual inline qreal BC50SF() const override { return BC50(); }

    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i == 0)
            return tr("K<sub>1</sub>");
        else if (i == 1)
            return tr("K<sub>2</sub>");
        else
            return QString();
    }

    virtual inline QString SpeciesName(int i) const override
    {
        if (i == 1)
            return tr("AB 1");
        else if (i == 2)
            return tr("AB 2");
        else
            return QString();
    }

    virtual inline QString LocalParameterName(int i = 0) const override
    {
        if (i == 0)
            return tr("dH1");
        else if (i == 1)
            return tr("n1");
        else if (i == 2)
            return tr("dH2");
        else if (i == 3)
            return tr("n2");
        else if (i == 4)
            return tr("m (solv H)");
        else if (i == 5)
            return tr("n (solv H)");
        else
            return QString();
    }

    virtual int LocalParameterSize() const override { return 6; }
    virtual inline int InputParameterSize() const override { return 1; }

    virtual inline int Color(int i) const override
    {
        if (i == 0)
            return 1;
        else if (i == 1)
            return 2;
        else
            return i + 2;
    }

protected:
    virtual void CalculateVariables() override;
};
