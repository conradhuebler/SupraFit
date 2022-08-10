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
#include "src/core/models/titrations/AbstractItcModel.h"
//#include "src/core/concentrationalpolynomial.h"

class ConcentrationalPolynomial;

class itc_any_Model : public AbstractItcModel {
    Q_OBJECT

public:
    itc_any_Model(DataClass* data);
    itc_any_Model(AbstractItcModel* data);

    virtual ~itc_any_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::itc_any; }

    virtual void OptimizeParameters_Private() override;
    inline int GlobalParameterSize() const override { return m_global_parametersize; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;
    virtual bool SupportThreads() const override { return false; }

    bool DefineModel() override;

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

    virtual inline QString LocalParameterName(int i = 0) const override
    {
        return m_local_names[i];
    }

    virtual int LocalParameterSize(int i = 0) const override
    {
        return m_global_parametersize + 3;
    }

    virtual inline int InputParameterSize() const override { return 1; }

    virtual QString AdditionalOutput() const override;

    QString ParameterComment(int parameter) const override;

    QString ModelInfo() const override;

    QString AnalyseMonteCarlo(const QJsonObject& object, bool forceAll = false) const override;
    QString AnalyseGridSearch(const QJsonObject& object, bool forceAll = false) const override;
    inline virtual bool DemandInput() const { return true; }

    static QSharedPointer<AbstractModel> CreateModel(QPointer<DataClass> data)
    {
        return QSharedPointer<itc_any_Model>(new itc_any_Model(data.data()), &QObject::deleteLater);
    }

private:
    inline int Index(int a, int b) const { return (a - 1) * m_maxB + (b - 1); }

    int m_global_parametersize = 0;
    int m_maxA = 0, m_maxB = 0;
    QStringList m_global_names, m_species_names, m_local_names;
    QVector<ConcentrationalPolynomial*> m_solvers;

protected:
    virtual void CalculateVariables() override;
};
