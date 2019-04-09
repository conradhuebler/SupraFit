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

class ReductionAnalyse : public AbstractSearchClass {
    Q_OBJECT

public:
    ReductionAnalyse();
    virtual ~ReductionAnalyse() override;

    enum CVType {
        LeaveOneOut = 1,
        LeaveTwoOut = 2,
        LeaveManyOut = 3
    };

    void CrossValidation();
    void PlainReduction();
    QJsonObject ModelData() const { return m_model_data; }
    /* Since we change the checked rows of the model, we have to detach the data table from the global model */
    virtual inline void setModel(const QSharedPointer<AbstractModel> model) override
    {
        m_model = model->Clone();
        m_model->detach();
    }

    // void setController(const QJsonObject& controller) { m_controller = controller; }
    void clear();
public slots:
    void Interrupt() override;

private:
    void addThread(QPointer<MonteCarloThread> thread);
    bool Pending() const;
    inline virtual QJsonObject Controller() const override { return m_controller; }

    QVector<QPointer<MonteCarloThread>> m_threads;
    QJsonObject m_model_data;
    QJsonObject m_controller;

signals:
    void MaximumSteps(int steps);
    void InterruptAll();
};
