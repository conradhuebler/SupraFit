/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

class nmr_ItoI_Model : public AbstractNMRModel {
    Q_OBJECT

public:
    nmr_ItoI_Model(DataClass* data);
    nmr_ItoI_Model(AbstractNMRModel* data);

    virtual ~nmr_ItoI_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::nmr_ItoI; }

    virtual void CollectOptimizationParameters_Private() override;
    inline int GlobalParameterSize() const override { return 1; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;
    virtual bool SupportThreads() const override { return false; }

    // The NMR signal is linear in the chemical shifts, so the VarPro solver can project them out.
    // Applies only when all shifts are free — the default ("Fix Host Signal" = no); if the host shift
    // is fixed, fall back to the classic full-vector solver. Claude Generated.
    bool SupportsVarPro() const override { return getOption(Host) == QLatin1String("no"); }
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
        if (i == 0)
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
    /*! \brief Fill m_design with the two linear-response columns (free-host and complex mole fractions
     * of the observed host) at the current stability constant, so both the signal (m_design·shifts)
     * and the VarPro projection share one design matrix. Claude Generated. */
    void FillDesign();
    Eigen::MatrixXd m_design;
};
