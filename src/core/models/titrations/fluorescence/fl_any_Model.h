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
 *
 * Two optional corrections that are not part of the first version of the model (AI-implemented by Claude, 2026; the science is written down in
 * docs/fluorescence_models.md and has not been checked against the literature by the operator yet):
 *   - inner filter: the signal is attenuated by 10^(-(A_ex + A_em)/2), the absorbances A computed from the total concentrations, the path length
 *     and the molar absorptivities of the components (system parameters);
 *   - dynamic quenching: the signal is divided by 1 + K_SV [Q], [Q] the free concentration of a component, K_SV a fitted constant (lg K_SV,
 *     the last global parameter).
 * Both leave the signal linear in the fluorescence coefficients, so VarPro stays applicable.
 * Claude Generated. */

class fl_any_Model : public AbstractTitrationModel {
    Q_OBJECT

public:
    fl_any_Model(DataClass* data);
    fl_any_Model(AbstractTitrationModel* data);

    virtual ~fl_any_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::fl_any; }

    virtual void CollectOptimizationParameters_Private() override;
    /*! \brief One lg beta per species, then lg K_SV of the dynamic quenching (not fitted unless the option asks for it). */
    inline int GlobalParameterSize() const override { return m_global_parametersize; }
    // One fluorescence coefficient per free component plus one per species. Claude Generated.
    inline int LocalParameterSize(int series = 0) const override
    {
        Q_UNUSED(series)
        return m_species_count + m_component_count;
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
        return i >= 0 && i < m_species_names.size() ? m_species_names[i] : QString(); // lg K_SV has no species
    }

    virtual inline int Color(int i) const override
    {
        if (i == 0)
            return 1;
        else
            return i + 3;
    }
    virtual QString ParameterComment(int parameter) const override;

    BC50::ModelSystem BC50System() const override;

    virtual QString AdditionalOutput() const override;

    inline double ReductionCutOff() const override { return 1; }

    /*! \brief The reactions are asked for, unless the model is a preset of fixed reactions (`DefinePreset`). */
    inline virtual bool DemandInput() const override { return !m_preset; }

    void UpdateShifts();
    void UpdateLinear() override
    {
        UpdateShifts();
    }

    /*! \brief Options of the corrections (indices clear of the species options) and the system parameters they read. */
    enum {
        InnerFilter = 100,
        Quenching = 101
    };
    enum {
        PathLength = 2000, ///< cm
        EpsilonExcitation = 2100, ///< + component: L mol^-1 cm^-1 at the excitation wavelength
        EpsilonEmission = 2200 ///< + component: L mol^-1 cm^-1 at the emission wavelength
    };

    void DeclareOptions() override;
    void DeclareSystemParameter() override;
    QString ModelInfo() const override;
    /*! \brief Parameters of another layout (files of the fixed fluorescence models before the rebuild) cannot be reproduced: they are reset and said. */
    void AdaptStoredModel(const QJsonObject& block) override;

    bool InnerFilterOn() const { return getOption(InnerFilter) == QLatin1String("on"); }
    /*! \brief Index of the component whose free concentration quenches, -1 without quenching. */
    int Quencher() const;
    /*! \brief lg K_SV, 0 if the table is not that large (a file of an older layout). */
    double QuenchingLg() const { return GlobalTable() && GlobalTable()->columnCount() > m_species_count ? (*GlobalTable())[m_species_count] : 0.0; }
    /*! \brief Factor of the signal at point \p i from inner filter and quenching (1 without them): the rows of the design matrix. */
    double SignalFactor(int i) const { return i < m_signal_factor.size() ? m_signal_factor[i] : 1.0; }

protected:
    /*! \brief For the fixed models: defines the reactions at once (called in the constructor of the derived class, where `SFModel()` is its own). */
    void DefinePreset(const QString& reactions);

private:
    int m_global_parametersize = 0, m_species_count = 0;
    bool m_preset = false;
    QStringList m_global_names, m_species_names;
    Eigen::VectorXd m_signal_factor;
    Eigen::MatrixXd m_design;
    /*! \brief Fluorescence design matrix (DataPoints x (nComp + nSpecies)): absolute concentrations
     * of every free component followed by every species; fitted against the fluorescence coefficients. */
    Eigen::MatrixXd m_concentrations;

protected:
    virtual void CalculateVariables() override;
};