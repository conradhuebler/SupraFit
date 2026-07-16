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

#include "src/core/models/dataclass.h"
#include "src/core/models/titrations/AbstractNMRModel.h"

class nmr_any_Model : public AbstractNMRModel {
    Q_OBJECT

public:
    nmr_any_Model(DataClass* data);
    nmr_any_Model(AbstractNMRModel* data);

    virtual ~nmr_any_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::nmr_any; }

    virtual void CollectOptimizationParameters_Private() override;
    inline int GlobalParameterSize() const override { return m_global_parametersize; }
    // AbstractNMRModel hard-codes 2; a reaction system raises the component count. Claude Generated.
    inline int InputParameterSize() const override { return m_component_count; }
    inline bool UseDynamicParameterWidget() const override { return true; }

    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;
    virtual bool SupportThreads() const override { return false; }

    // The NMR signal is linear in the per-species chemical shifts, so the locals can be projected out
    // by the VarPro solver (reusing the m_molar_ratios design matrix). Claude Generated.
    bool SupportsVarPro() const override { return true; }
    void ProjectLinearParameters() override;
    bool AnalyticVarProJacobian(const std::vector<int>& globalIndices, Eigen::MatrixXd& jacobian) override;

    bool DefineModel() override;

    void CalculateConcentrations();

    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        return m_global_names[i];
    }

    virtual inline QString SpeciesName(int i) const override
    {
        return m_species_names[i];
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

    //    virtual QVector<qreal> DeCompose(int datapoint, int series = 0) const override;

    inline double ReductionCutOff() const override { return 1; }

    inline virtual bool DemandInput() const override { return true; }

    void UpdateShifts();
    void UpdateLinear() override
    {
        UpdateShifts();
    }

private:
    int m_global_parametersize = 0;
    int m_observed = 0; ///< index of the observed component (NMR nucleus), default = component 0 (A)
    QStringList m_global_names, m_species_names;
    Eigen::MatrixXd m_concentrations, m_molar_ratios;

protected:
    virtual void CalculateVariables() override;
};
