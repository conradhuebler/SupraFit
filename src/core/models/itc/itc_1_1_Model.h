/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/AbstractItcModel.h"
#include "src/core/models/dataclass.h"

class itc_ItoI_Model : public AbstractItcModel {
    Q_OBJECT

public:
    itc_ItoI_Model(DataClass* data);
    itc_ItoI_Model(AbstractItcModel* data);

    virtual ~itc_ItoI_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::itc_ItoI; }

    virtual void OptimizeParameters_Private() override;
    inline int GlobalParameterSize() const override { return 1; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;
    virtual bool SupportThreads() const override { return false; }

    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i == 0)
            return K11;
        else
            return QString();
    }

    virtual inline QString SpeciesName(int i) const override
    {
        if (i == 0)
            return AB;
        else
            return QString();
    }

    virtual inline QString LocalParameterName(int i = 0) const override
    {
        if (i == 0)
            return dHAB_;
        else if (i == 1)
            return msolv;
        else if (i == 2)
            return nsolv;
        else if (i == 3)
            return fx;
        else
            return QString();
    }

    virtual int LocalParameterSize(int i = 0) const override { Q_UNUSED(i)
        return 4; }
    virtual inline int InputParameterSize() const override { return 1; }

    virtual inline int Color(int i) const override
    {
        if (i == 0)
            return 1;
        else
            return i + 3;
    }

    virtual QString AdditionalOutput() const override;

    QString ParameterComment(int parameter) const override;

    QString ModelInfo() const override;

    QString AnalyseMonteCarlo(const QJsonObject& object, bool forceAll = false) const override;
    QString AnalyseGridSearch(const QJsonObject& object, bool forceAll = false) const override;

protected:
    virtual void CalculateVariables() override;
};
