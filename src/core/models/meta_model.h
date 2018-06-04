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
#include <QtCore/QPair>
#include <QtCore/QVector>

#include "src/core/AbstractModel.h"
#include "src/core/dataclass.h"

typedef QPair<int, QPair<int, int>> CombinedParameter;

class MetaModel : public AbstractModel {
    Q_OBJECT

public:
    MetaModel();
    ~MetaModel();

    enum ConnectType {
        None = 0,
        All = 1,
        Dilution = 2,
        Custom = 3
    };

    virtual inline SupraFit::Model SFModel() const { return SupraFit::MetaModel; }

    virtual inline void setConnectType(ConnectType type) { m_connect_type = type; }

    virtual QVector<qreal> OptimizeParameters_Private() override;

    virtual void InitialGuess_Private() override;

    virtual QSharedPointer<AbstractModel> Clone() override;

    virtual qreal GlobalParameter(int i) const override;

    virtual bool SupportThreads() const override { return false; }

    virtual int GlobalParameterSize() const { return m_global_par.size(); }

    virtual int InputParameterSize() const { return m_inp_param; }

    virtual int LocalParameterSize() const { return m_local_par.size(); }

    virtual inline int Size() const override { return m_size; }
    virtual inline int IndependentVariableSize() const override { return m_indep_var; }
    virtual inline int DataPoints() const override { return m_dep_var; }
    virtual inline int SeriesCount() const override { return m_series_count; }

    virtual inline void setConverged(bool converged)
    {
        for (int i = 0; i < m_models.size(); ++i)
            m_models[i]->setConverged(converged);
        m_converged = converged;
    }

    void addModel(const QPointer<AbstractModel> model);

    inline QVector<QSharedPointer<AbstractModel>> Models() const { return m_models; }

    inline int ModelSize() const { return m_models.size(); }

    virtual bool SupportSeries() const { return true; }

    virtual QJsonObject ExportModel(bool statistics = true, bool locked = false);

    virtual bool ImportModel(const QJsonObject& topjson, bool override = true);

    virtual QList<double> getSignals(QList<int> active_signal) override;

    virtual QList<double> getCalculatedModel() override;

    virtual qreal ModelError() const override;

    virtual qreal SumOfErrors(int i) const override;

    virtual qreal CalculateCovarianceFit() override;

    virtual qreal CalculateVariance() override;

    void PrepareTables();

private:
    void ApplyConnectType();

    QVector<QSharedPointer<AbstractModel>> m_models;
    int m_glob_param = 0, m_inp_param = 0, m_loc_param = 0, m_size = 0, m_indep_var = 0, m_dep_var = 0, m_series_count = 0, m_unique_global = 0, m_unique_local = 0, m_unique_series = 0;
    QVector<QVector<QPair<int, int>>> m_global_index, m_local_index;
    QStringList m_global_names, m_local_names;
    QVector<QPair<qreal, QVector<CombinedParameter>>> m_combined_local, m_combined_global;
    QVector<QPair<int, int>> m_global_par, m_local_par;

    SupraFit::Model m_model_type;
    ConnectType m_connect_type = ConnectType::None;

protected:
    virtual void CalculateVariables() override;

signals:
    void ModelAdded(QSharedPointer<AbstractModel> model);
};
