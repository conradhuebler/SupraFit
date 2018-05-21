/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/AbstractTitrationModel.h"
#include "src/core/dataclass.h"

class fl_IItoI_ItoI_Model : public AbstractTitrationModel {
    Q_OBJECT

public:
    enum {
        Method = 1,
        Cooperativity = 2
    };

    fl_IItoI_ItoI_Model(DataClass* data);
    ~fl_IItoI_ItoI_Model();

    virtual inline SupraFit::Model SFModel() const { return SupraFit::fl_IItoI_ItoI; }

    virtual QVector<qreal> OptimizeParameters_Private() override;
    inline int GlobalParameterSize() const override { return 2; }

    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone() override;
    virtual bool SupportThreads() const override { return false; }
    virtual qreal BC50() const override { return BC50::IItoI_ItoI_BC50(GlobalParameter(0), GlobalParameter(1)); }
    virtual inline qreal BC50SF() const override { return BC50(); }

    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i == 0)
            return tr("K<sub>21</sub>");
        else if (i == 1)
            return tr("K<sub>11</sub>");
        else
            return QString();
    }
    void virtual DeclareOptions() override;

    virtual int LocalParameterSize() const override { return 4; }

    void virtual EvaluateOptions() override;
    //   virtual inline QString Name() const override { return tr("&Phi; 2:1/1:1-Model"); }

protected:
    virtual void CalculateVariables() override;
};
