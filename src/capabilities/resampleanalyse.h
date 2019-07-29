/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/capabilities/abstractsearchclass.h"
#include "src/core/AbstractModel.h"

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

class AbstractModel;
class MonteCarloThread;
class Minimizer;

class ResampleAnalyse : public AbstractSearchClass {
    Q_OBJECT

public:
    ResampleAnalyse();
    virtual ~ResampleAnalyse() override;
    virtual bool Run() override;

    void CrossValidation();
    void PlainReduction();
    QJsonObject ModelData() const { return m_model_data; }

    /* Since we change the checked rows of the model, we have to detach the data table from the global model */
    virtual inline void setModel(const QSharedPointer<AbstractModel> model) override
    {
        m_model = model->Clone();
        m_model->detach();
    }

public slots:
    void Interrupt() override;

private:
    void addThread(QPointer<MonteCarloThread> thread);
    bool Pending() const;
    QHash<int, QVector<int>> m_job;
    QVector<QPointer<MonteCarloThread>> m_threads;
    QJsonObject m_model_data;

signals:
    void InterruptAll();
};
