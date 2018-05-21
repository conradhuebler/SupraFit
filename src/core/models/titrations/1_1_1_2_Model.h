/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/AbstractTitrationModel.h"
#include "src/core/dataclass.h"

class ItoI_ItoII_Model : public AbstractTitrationModel {
    Q_OBJECT

public:
    enum {
        Cooperativity = 3
    };

    ItoI_ItoII_Model(DataClass* data);
    ~ItoI_ItoII_Model();

    virtual inline SupraFit::Model SFModel() const { return SupraFit::ItoI_ItoII; }

    virtual QVector<qreal> OptimizeParameters_Private() override;
    inline int GlobalParameterSize() const override { return 2; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone() override;
    virtual bool SupportThreads() const override { return false; }

    virtual qreal BC50() const override { return BC50::ItoI_ItoII_BC50(GlobalParameter(0), GlobalParameter(1)); }
    virtual qreal BC50SF() const override { return BC50::ItoI_ItoII_BC50_SF(GlobalParameter(0), GlobalParameter(1)); }

    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i == 0)
            return tr("K<sub>11</sub>");
        else if (i == 1)
            return tr("K<sub>12</sub>");
        else
            return QString();
    }

    virtual inline QString SpeciesName(int i) const override
    {
        if (i == 0)
            return tr("AB");
        else if (i == 1)
            return tr("AB2");
        else
            return QString();
    }

    virtual void DeclareOptions() override;

    virtual void EvaluateOptions() override;
    //   virtual inline QString Name() const override { return tr("1:1/1:2-Model"); }

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
