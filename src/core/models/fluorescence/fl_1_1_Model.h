/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/AbstractTitrationModel.h"
#include "src/core/dataclass.h"

class fl_ItoI_Model : public AbstractTitrationModel {
    Q_OBJECT

public:
    fl_ItoI_Model(DataClass* data);
    fl_ItoI_Model(AbstractTitrationModel* data);

    virtual ~fl_ItoI_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::fl_ItoI; }

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

    virtual void DeclareOptions() override;
    virtual void EvaluateOptions() override;
    virtual int LocalParameterSize(int series = 0) const override { Q_UNUSED(series)
        return 3; }

    virtual QString AdditionalOutput() const override { return QString(); }

    virtual QString ParameterComment(int parameter) const override;

    virtual QString ModelInfo() const override;

    QString AnalyseMonteCarlo(const QJsonObject& object, bool forceAll = false) const override;

protected:
    virtual void CalculateVariables() override;
};
