/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include <QtCore/QVector>

#include "src/core/AbstractItcModel.h"
#include "src/core/dataclass.h"

class IItoI_ItoI_ItoII_Solver;
class QThreadPool;

class itc_IItoII_Model : public AbstractItcModel {
    Q_OBJECT

public:
    enum {
        Cooperativity2_1 = 3,
        Cooperativity1_2 = 4
    };

    itc_IItoII_Model(DataClass* data);
    itc_IItoII_Model(AbstractItcModel* data);
    ~itc_IItoII_Model();

    virtual inline SupraFit::Model SFModel() const { return SupraFit::itc_IItoII; }
    virtual void DeclareOptions() override;
    virtual void EvaluateOptions() override;

    virtual QVector<qreal> OptimizeParameters_Private() override;
    inline int GlobalParameterSize() const override { return 3; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone() override;
    virtual bool SupportThreads() const override { return true; }

    virtual qreal BC50() const override { return BC50::IItoI_ItoI_ItoII_BC50(GlobalParameter(0), GlobalParameter(1), GlobalParameter(2)); }
    virtual qreal BC50SF() const override { return BC50::IItoI_ItoI_ItoII_BC50_SF(GlobalParameter(0), GlobalParameter(1), GlobalParameter(2)); }

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

    virtual inline QString LocalParameterName(int i = 0) const override
    {
        if (i == 0)
            return dHA2B_;
        else if (i == 1)
            return dHAB_;
        else if (i == 2)
            return dHAB2_;
        else if (i == 3)
            return msolv;
        else if (i == 4)
            return nsolv;
        else if (i == 5)
            return fx;
        else
            return QString();
    }

    virtual inline QString SpeciesName(int i) const override
    {
        if (i == 0)
            return A2B;
        else if (i == 1)
            return AB;
        else if (i == 2)
            return AB2;
        else
            return QString();
    }

    virtual int LocalParameterSize(int i = 0) const override { Q_UNUSED(i)
        return 6; }
    virtual inline int InputParameterSize() const override { return 1; }

    virtual inline int Color(int i) const override
    {
        if (i > 2)
            return i + 1;
        return i;
    }

    virtual QString AdditionalOutput() const override;

private:
    QList<QPointer<IItoI_ItoI_ItoII_Solver>> m_solvers;
    QPointer<QThreadPool> m_threadpool;

protected:
    virtual void CalculateVariables() override;
};
