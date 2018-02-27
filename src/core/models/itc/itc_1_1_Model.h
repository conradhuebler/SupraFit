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

#include "src/global.h"
#include "src/core/AbstractItcModel.h"

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/dataclass.h"

class itc_ItoI_Model : public AbstractItcModel
{
    Q_OBJECT
    
public:
    itc_ItoI_Model(DataClass *data);
    ~itc_ItoI_Model();

    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type) override;
    inline int GlobalParameterSize() const override { return 2;}
    virtual void InitialGuess() override;
    virtual QSharedPointer<AbstractModel > Clone() override;
    virtual bool SupportThreads() const override { return false; }
    virtual qreal BC50() const override;
    virtual inline qreal BC50SF() const override { return BC50(); }
    virtual inline QString GlobalParameterName(int i = 0) const override 
    { 
        if( i == 0)
            return tr("K<sub>11</sub>");
        else if( i == 1)
            return tr("dH");
        else
            return QString();
    }

    virtual int LocalParameterSize() const override {return 3; }
    virtual inline int InputParameterSize() const override { return 1; }

    virtual inline QString Name() const override { return tr("itc_1:1-Model"); }

    virtual inline QString GlobalParameterPrefix(int i = 0) const override
    {
        if(i == 0)
            return QString("10^");
        else
            return QString();
    }

protected:
    virtual void CalculateVariables() override;
};

