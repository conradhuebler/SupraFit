/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/bc50.h"
#include "src/global.h"

#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtCore/QVector>

#include "src/core/AbstractTitrationModel.h"
#include "src/core/dataclass.h"

typedef Eigen::VectorXd Vector;

class IItoI_ItoI_ItoII_Solver;

class fl_IItoI_ItoI_ItoII_Model : public AbstractTitrationModel {
    Q_OBJECT

public:
    enum {
        Method = 1,
        Cooperativity2_1 = 2,
        Cooperativity1_2 = 2
    };

    fl_IItoI_ItoI_ItoII_Model(DataClass* data);
    fl_IItoI_ItoI_ItoII_Model(AbstractTitrationModel* data);

    virtual ~fl_IItoI_ItoI_ItoII_Model() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::fl_IItoI_ItoI_ItoII; }

    virtual void OptimizeParameters_Private() override;
    inline int GlobalParameterSize() const override { return 3; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone() override;
    virtual bool SupportThreads() const override { return true; }
    virtual MassResults MassBalance(qreal A, qreal B) override;
    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i == 0)
            return K21;
        else if (i == 1)
            return K11;
        else if (i == 2)
            return K12;
        else
            return QString();
    }

    virtual QString SpeciesName(int i) const override
    {
        if (i == 0)
            return tr("A2B");
        else if (i == 1)
            return tr("AB");
        else if (i == 2)
            return tr("AB2");
        else
            return QString();
    }

    virtual void DeclareOptions() override;
    virtual void EvaluateOptions() override;

    virtual inline int Color(int i) const override
    {
        if (i > 2)
            return i + 1;
        return i;
    }

    virtual int LocalParameterSize(int series = 0) const override { Q_UNUSED(series)
        return 5; }

    virtual QString AnalyseMonteCarlo(const QJsonObject& object, bool forceAll = false) const override;

    virtual QString AdditionalOutput() const override { return QString(); }

    virtual QString ParameterComment(int parameter) const override;

    virtual QString ModelInfo() const override;

private:
    QList<QPointer<IItoI_ItoI_ItoII_Solver>> m_solvers;
    QList<qreal> m_constants_pow;
    QThreadPool* m_threadpool;

protected:
    virtual void CalculateVariables() override;
};
