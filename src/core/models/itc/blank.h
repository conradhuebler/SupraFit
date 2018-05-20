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

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/AbstractItcModel.h"
#include "src/core/dataclass.h"

class Blank : public AbstractItcModel {
    Q_OBJECT

public:
    Blank(DataClass* data);
    Blank(AbstractItcModel* data);

    ~Blank();

    virtual inline SupraFit::Model SFModel() const { return SupraFit::itc_blank; }

    virtual QVector<qreal> OptimizeParameters_Private() override;
    inline int GlobalParameterSize() const override { return 0; }
    virtual void InitialGuess() override;
    virtual QSharedPointer<AbstractModel> Clone() override;
    virtual bool SupportThreads() const override { return false; }

    virtual inline QString LocalParameterName(int i = 0) const override
    {
        if (i == 0)
            return tr("m (solv H)");
        else if (i == 1)
            return tr("n (solv H)");
        else
            return QString();
    }

    virtual int LocalParameterSize() const override { return 2; }
    virtual inline int InputParameterSize() const override { return 1; }

    virtual inline int Color(int i) const override
    {
        if (i == 0)
            return 1;
        else
            return i + 3;
    }

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const override { return "c<sub>0</sub>"; }

    virtual qreal PrintOutIndependent(int i, int format) const override;

protected:
    virtual void CalculateVariables() override;
};
