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

#include "src/global.h"

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/AbstractModel.h"
#include "src/core/dataclass.h"

class MetaModel : public AbstractModel {
    Q_OBJECT

public:
    MetaModel();
    ~MetaModel();

    virtual inline SupraFit::Model SFModel() const { return SupraFit::MetaModel; }

    virtual QVector<qreal> OptimizeParameters_Private() override;
    virtual void InitialGuess_Private() override;

    virtual QSharedPointer<AbstractModel> Clone() override;
    virtual bool SupportThreads() const override { return false; }

    virtual int GlobalParameterSize() const { return m_glob_param; }

    virtual int InputParameterSize() const { return m_inp_param; }

    virtual int LocalParameterSize() const { return m_loc_param; }

    inline void addModel(QSharedPointer<AbstractModel> model)
    {
        m_models << model;
        m_glob_param += model->GlobalParameterSize();
        m_inp_param += model->InputParameterSize();
        m_loc_param += model->LocalParameterSize();
    }

    inline QVector<QWeakPointer<AbstractModel>> Models() const { return m_models; }

    virtual bool SupportSeries() const { return true; }

private:
    QVector<QWeakPointer<AbstractModel>> m_models;
    int m_glob_param = 0, m_inp_param = 0, m_loc_param = 0;

protected:
    virtual void CalculateVariables() override;
};
