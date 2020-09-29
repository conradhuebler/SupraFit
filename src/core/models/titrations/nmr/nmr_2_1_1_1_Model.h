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

#include "src/global.h"

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/models/dataclass.h"
#include "src/core/models/titrations/AbstractNMRModel.h"

class nmr_IItoI_ItoI_Model : public AbstractNMRModel {
    Q_OBJECT

public:
    enum {
        Cooperativity = 3
    };

    nmr_IItoI_ItoI_Model(DataClass* data);
    nmr_IItoI_ItoI_Model(AbstractNMRModel* data);

    virtual ~nmr_IItoI_ItoI_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::nmr_IItoI_ItoI; }

    virtual void OptimizeParameters_Private() override;
    inline int GlobalParameterSize() const override { return 2; }

    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;
    virtual bool SupportThreads() const override { return false; }

    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i == 0)
            return K21;
        else if (i == 1)
            return K11;
        else
            return QString();
    }

    virtual inline QString SpeciesName(int i) const override
    {
        if (i == 0)
            return A2B;
        else if (i == 1)
            return AB;
        else
            return QString();
    }

    void DeclareOptions() override;
    void EvaluateOptions() override;

    virtual inline int Color(int i) const override
    {
        if (i > 1)
            return i + 2;
        return i;
    }

    /*! \brief Calculate standard type of monte carlo statistics */
    virtual QString AnalyseMonteCarlo(const QJsonObject& object, bool forceAll = false) const override;

    virtual QString ParameterComment(int parameter) const override;

    virtual QString ModelInfo() const override;

    virtual QString AnalyseGridSearch(const QJsonObject& object, bool forceAll = false) const override;

    virtual QVector<qreal> DeCompose(int datapoint, int series = 0) const override;

    virtual QString AdditionalOutput() const override;

    inline double ReductionCutOff() const override { return 1; }

protected:
    virtual void CalculateVariables() override;
};
