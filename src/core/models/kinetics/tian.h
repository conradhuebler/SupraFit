/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2023 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

class TIANModel : public AbstractModel {
    Q_OBJECT

public:
    enum {
        Temperature = 1,
        C0 = 2,
        C1 = 3,
        C2 = 4,
        C3 = 5,
        C4 = 6
    };

    TIANModel(DataClass* data);
    TIANModel(AbstractModel* data);

    virtual ~TIANModel() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::TianModel; }

    virtual void OptimizeParameters_Private() override;

    inline int GlobalParameterSize() const override { return 2; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;
    virtual bool SupportThreads() const override { return false; }

    /*! \brief we have only the time as input parameter
     */
    virtual inline int InputParameterSize() const override { return 1; }
    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i == 0)
            return tr("A");
        else if (i == 1)
            return tr("k [1e-5]");
        else
            return QString("");
    }

    virtual int LocalParameterSize(int i = 0) const override
    {
        Q_UNUSED(i)
        return 0;
    }
    virtual inline bool SupportSeries() const override { return false; }
    virtual void DeclareOptions() override;

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const override { return "time (t)"; }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const override { return "heat flow signal ()"; }

public slots:
    virtual void UpdateParameter() override;

private:
    void virtual DeclareSystemParameter() override;

protected:
    virtual void CalculateVariables() override;
    qreal m_A0, m_B0;
};
