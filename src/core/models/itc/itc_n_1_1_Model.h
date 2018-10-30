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

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/AbstractItcModel.h"
#include "src/core/dataclass.h"

class itc_n_ItoI_Model : public AbstractItcModel {
    Q_OBJECT

public:
    itc_n_ItoI_Model(DataClass* data);
    itc_n_ItoI_Model(AbstractItcModel* data);

    virtual ~itc_n_ItoI_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::itc_n_ItoI; }

    virtual void OptimizeParameters_Private() override;
    inline int GlobalParameterSize() const override { return 1; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone() override;
    virtual bool SupportThreads() const override { return false; }

    /*
    virtual inline qreal BC50() const override { return 0; }
    virtual inline qreal BC50SF() const override { return BC50(); }
    */
    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i == 0)
            return tr("K<sub>11</sub>");
        else
            return QString();
    }

    virtual inline QString SpeciesName(int i) const override
    {
        if (i == 1)
            return tr("AB");
        else
            return QString();
    }

    virtual inline QString LocalParameterName(int i = 0) const override
    {
        if (i == 0)
            return tr("dH (AB)");
        else if (i == 1)
            return tr("m (solv H)");
        else if (i == 2)
            return tr("n (solv H)");
        else if (i == 3)
            return tr("fx");
        else
            return QString();
    }

    virtual int LocalParameterSize(int i = 0) const override { Q_UNUSED(i)
        return 4; }
    virtual inline int InputParameterSize() const override { return 1; }
    //  virtual inline QString Name() const override { return tr("ITC 1:1-Model"); }

    virtual inline int Color(int i) const override
    {
        if (i == 0)
            return 1;
        else
            return i + 3;
    }

    virtual QString ParameterComment(int parameter) const override { Q_UNUSED(parameter)
        return QString("Reaction: A + B &#8652; AB"); }

protected:
    virtual void CalculateVariables() override;
};
