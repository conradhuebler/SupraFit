/*
 * SupraFit - flexible fluorescence titration model (arbitrary N-component equilibrium via BFGS speciation)
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

/* fl_any is the fluorescence counterpart of uvvis_any: the speciation is delegated to the shared
 * SpeciationEngine on AbstractTitrationModel and the equilibrium system is defined solely through the
 * free-text Reactions field. The signal is modelled as a linear combination of per-species fluorescence
 * coefficients (Σ c_species · φ_species, plus the free-component coefficients) — the same linear form the
 * fixed fl_1_1_1_2 / fl_2_1_1_1 models use, generalised to N components. (fl_1_1 keeps its non-linear tau
 * form and is not touched.) Because the signal is linear in the locals, VarPro can project them out.
 * Claude Generated. */

class fl_any_Model : public AbstractTitrationModel {
    Q_OBJECT

public:
    fl_any_Model(DataClass* data);
    fl_any_Model(AbstractTitrationModel* data);

    virtual ~fl_any_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::fl_any; }

    virtual void CollectOptimizationParameters_Private() override;
    inline int GlobalParameterSize() const override { return m_global_parametersize; }
    // One fluorescence coefficient per free component plus one per species. Claude Generated.
    inline int LocalParameterSize(int series = 0) const override
    {
        Q_UNUSED(series)
        return GlobalParameterSize() + m_component_count;
    }
    QString LocalParameterName(int i = 0) const override;
    QString LocalParameterDescription(int i = 0) const override { return LocalParameterName(i); }
    inline bool UseDynamicParameterWidget() const override { return true; }

    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;
    virtual bool SupportThreads() const override { return false; }

    // The fluorescence signal is linear in the per-species coefficients, so the locals can be projected
    // out by the VarPro solver (reusing the m_concentrations design matrix). Claude Generated.
    bool SupportsVarPro() const override { return true; }
    void ProjectLinearParameters() override;

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

    inline double ReductionCutOff() const override { return 1; }

    inline virtual bool DemandInput() const { return true; }

    void UpdateShifts();
    void UpdateLinear() override
    {
        UpdateShifts();
    }

private:
    int m_global_parametersize = 0;
    QStringList m_global_names, m_species_names;
    /*! \brief Fluorescence design matrix (DataPoints x (nComp + nSpecies)): absolute concentrations
     * of every free component followed by every species; fitted against the fluorescence coefficients. */
    Eigen::MatrixXd m_concentrations;

protected:
    virtual void CalculateVariables() override;
};