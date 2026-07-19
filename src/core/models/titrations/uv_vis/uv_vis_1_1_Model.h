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
#include "src/core/models/titrations/AbstractTitrationModel.h"

class uv_vis_ItoI_Model : public AbstractTitrationModel {
    Q_OBJECT

public:
    uv_vis_ItoI_Model(DataClass* data);
    uv_vis_ItoI_Model(AbstractTitrationModel* data);

    virtual ~uv_vis_ItoI_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::uv_vis_ItoI; }

    virtual void CollectOptimizationParameters_Private() override;
    inline int GlobalParameterSize() const override { return 1; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;
    virtual bool SupportThreads() const override { return false; }

    // Beer-Lambert absorbance is linear in the extinction coefficients, so VarPro can project them.
    // A silent host/guest just zeroes its design column (guest*0), so projecting it yields eps=0
    // harmlessly - no gating needed here (user-locked locals remain a TODO). Claude Generated.
    bool SupportsVarPro() const override { return true; }
    void ProjectLinearParameters() override;

    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i == 0)
            return K11;
        else
            return QString();
    }

    virtual inline QString SpeciesName(int i) const override
    {
        if (i == 1)
            return AB;
        else
            return QString();
    }

    virtual inline int Color(int i) const override
    {
        if (i == 0)
            return 1;
        else
            return i + 3;
    }
    virtual QString ParameterComment(int parameter) const override;

    virtual QString ModelInfo() const override;

    virtual QString AdditionalOutput() const override;

    QString AnalyseMonteCarlo(const QJsonObject& object, bool forceAll = false) const override;
    virtual QString AnalyseGridSearch(const QJsonObject& object, bool forceAll = false) const override;

    virtual QVector<qreal> DeCompose(int datapoint, int series = 0) const override;

    inline double ReductionCutOff() const override { return 1; }

protected:
    virtual void CalculateVariables() override;

private:
    /*! \brief Fill m_design with the free-host, free-guest and complex extinction columns (silent
     * components carry a 0 multiplier), shared by the signal and the VarPro projection. CG. */
    void FillDesign();
    Eigen::MatrixXd m_design;
};
