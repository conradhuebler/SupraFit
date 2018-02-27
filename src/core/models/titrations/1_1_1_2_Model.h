/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/AbstractModel.h"

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/dataclass.h"

class ItoI_ItoII_Model : public AbstractTitrationModel
{
     Q_OBJECT
    
public:
    ItoI_ItoII_Model(DataClass* data);
    ~ItoI_ItoII_Model();
    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type) override;
    inline int GlobalParameterSize() const override { return 2;}
    virtual void InitialGuess() override;
    virtual QSharedPointer<AbstractModel > Clone() override;
    virtual bool SupportThreads() const override { return false; }
    virtual qreal BC50() const override;
    virtual qreal BC50SF() const override;

    virtual inline QString GlobalParameterName(int i = 0) const override 
    { 
        if(i == 0)
            return  tr("K<sub>11</sub>");
        else if( i == 1)
            return tr("K<sub>12</sub>"); 
        else
            return QString();
    }

    virtual QString SpeciesName(int i) const override
    {
        if(i == 0)
            return tr("AB");
        else if(i == 1)
            return tr("AB2");
        else
            return QString();
    }

    virtual void DeclareOptions() override;
        
    virtual void EvaluateOptions() override;
    virtual inline QString Name() const override { return tr("1:1/1:2-Model"); }

    virtual inline int Color(int i) const override { if(i == 0) return 1; else if(i == 1) return 2; else return i + 2; }

private:
    static qreal Y(qreal x, const QVector<qreal > & parameter);
    static qreal Y_0(qreal x, const QVector<qreal > & parameter);

protected:
    virtual void CalculateVariables() override;
};
