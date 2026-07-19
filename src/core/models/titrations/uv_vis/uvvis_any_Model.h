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
#include "src/core/models/titrations/AbstractTitrationModel.h"

class uvvis_any_Model : public AbstractTitrationModel {
    Q_OBJECT

public:
    uvvis_any_Model(DataClass* data);
    uvvis_any_Model(AbstractTitrationModel* data);

    virtual ~uvvis_any_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::uvvis_any; }

    virtual void CollectOptimizationParameters_Private() override;
    inline int GlobalParameterSize() const override { return m_global_parametersize; }
    // Beer-Lambert: one extinction coefficient per free component plus one per species. Claude Generated.
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

    // The Beer-Lambert signal is linear in the extinction coefficients, so the locals can be projected
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

    //    virtual QVector<qreal> DeCompose(int datapoint, int series = 0) const override;

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
    /*! \brief Beer-Lambert design matrix (DataPoints x (nComp + nSpecies)): absolute concentrations
     * of every free component followed by every species; fitted against the extinction coefficients. */
    Eigen::MatrixXd m_concentrations;

protected:
    virtual void CalculateVariables() override;
};
