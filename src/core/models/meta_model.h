/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <optional>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

/* The structure of the MetaModelParameters MMParameter is as follows
 * A pair of a double/qreal with a QVector of QVector of four integers - indicies
 * (double) - (<0,0,0,0>; <1,0,1,0>) ....
 * (double) represents the value (theta) of the parameter
 * <a,b,c,d> represents  the identifikation of the submodel and the specific parameter within that model
 * a represents the index of the model
 * b can be either 0 (global parameter) or 1 (local parameter)
 * c,d by the time of writing, couple of years after implementation, I forget them, but I will remember ...
 * c would most probably be the index of the parameter within the original model
 */

typedef QPair<qreal, QVector<QVector<int>>> MMParameter;

class MetaModel : public AbstractModel {
    Q_OBJECT

public:
    MetaModel(DataClass* data);

    virtual ~MetaModel() override;

    enum ConnectType {
        None = 0,
        All = 1,
        Custom = 2
    };

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::MetaModel; }

    virtual inline void setConnectType(ConnectType type)
    {
        m_connect_type = type;
        ApplyConnectType();
        CollectParameter();
        emit Recalculated();
    }

    virtual void CollectOptimizationParameters_Private() override;

    QVector<qreal> CollectParameter();

    virtual void InitialGuess_Private() override;

    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;

    virtual qreal GlobalParameter(int i) const override;

    virtual qreal LocalParameter(int parameter, int series) const override;

    virtual bool SupportThreads() const override { return false; }

    virtual int GlobalParameterSize() const override { return m_global_par.size(); }

    virtual int InputParameterSize() const override { return m_inp_param; }

    virtual int LocalParameterSize(int i = 0) const override;

    /*! \brief Returns pointer to Model DataTable
     * overloaded function
     */
    virtual inline DataTable* ModelTable() override
    {
        UpdateCalculated();
        return m_model_signal;
    }

    /*! \brief Returns const pointer to Model DataTable
     * overloaded function
     */
    virtual inline DataTable* ModelTable() const override { return m_model_signal; }

    virtual inline int Size() const override { return m_size; }
    virtual inline int IndependentVariableSize() const override { return m_indep_var; }
    virtual inline int DataPoints() const override { return m_dep_var; }
    virtual inline int SeriesCount() const override { return m_series_count; }

    virtual void forceGlobalParameter(double value, int parameter) override;

    virtual void forceLocalParameter(qreal value, int parameter, int series) override;

    virtual inline void setConverged(bool converged) override
    {
        for (int i = 0; i < m_models.size(); ++i)
            m_models[i]->setConverged(converged);
        m_converged = converged;
    }

    virtual QString GlobalParameterName(int i = 0) const override;
    virtual QString LocalParameterName(int i = 0) const override;

    /*! \brief Adds a copy of \p model and prepares the meta model again (parameters, sliced table, model table); emits ModelAdded. */
    void addModel(const QPointer<AbstractModel> model);

    /*! \brief Adds copies of all \p models and prepares the meta model once, as n addModel calls would leave it.
     *  ModelAdded is emitted for every copy after the preparation. Used when a meta model is rebuilt from a file. Claude Generated. */
    void addModels(const QVector<QPointer<AbstractModel>>& models);

    /*! \brief Takes the \p models themselves instead of copies, otherwise as addModels. The models must belong to no project
     *  (detached with DataClass::RemoveChild) and to nothing else: the meta model owns them from now on. Used when a meta model
     *  is rebuilt from a file, the models read for it are not cloned again. Claude Generated. */
    void adoptModels(const QVector<QSharedPointer<AbstractModel>>& models);

    inline const QVector<QSharedPointer<AbstractModel>> Models() const { return m_models; }

    inline int ModelSize() const { return m_models.size(); }

    virtual bool SupportSeries() const override { return true; }

    virtual QJsonObject ExportModel(bool statistics = true, bool locked = false) override;

    virtual bool ImportModel(const QJsonObject& topjson, bool override = true) override;

    /*! \brief ImportModel for a meta model whose models already hold their blocks of \p topjson: \p imported[i] tells that
     *  model i was read from block "raw"/i with AbstractModel::ImportModel, so it is not read again, only its stored name is
     *  taken back (the one the import would set). Everything of the meta model itself (connect type, linked parameters,
     *  parameter tables, statistics) is read as by ImportModel. An empty \p imported reads every model again. Claude Generated. */
    bool ImportAdoptedModels(const QJsonObject& topjson, const QVector<bool>& imported, bool override = true);

    virtual QList<double> getSignals(QList<int> active_signal) override;

    virtual QList<double> getCalculatedModel() override;
    virtual QList<qreal> getCalculatedSquaredErrors() override;
    virtual QList<qreal> getCalculatedAbsoluteErrors() override;

    virtual qreal ModelError() const override;

    virtual qreal SumOfErrors(int i) const override;

    virtual qreal CalculateCovarianceFit() const override;

    virtual qreal CalculateVariance() override;

    QVector<qreal> ErrorVector() const override;

    void PrepareTables();
    virtual void IndependentModelOverride() override;

    virtual void DependentModelOverride() override;

    virtual void CheckedModelOverride() override { DependentModelOverride(); }

    void DebugParameter() const;

    inline QVector<MMParameter> CombinedParameter() const { return m_mmparameter; }

    inline const MMParameter* CombinedParameter(int i) const { return &m_mmparameter[i]; }

    void MoveParameterList(int source, int destination);

    void MoveSingleParameter(int parameter_index_1, int parameter_index_2 = -1, int destination = -1);

    inline bool LocalEnabled(int i) const override { Q_UNUSED(i)
        return true; }

    inline bool GlobalEnabled(int i) const override { Q_UNUSED(i)
        return true; }

    virtual QVector<qreal> AllParameter() const override;

    ConnectType ConnectionType() const { return m_connect_type; }

    void RemoveModel(const AbstractModel* model);

    inline virtual int ChildrenSize() const override { return m_models.size(); }

    virtual inline QPointer<DataClass> Children(int i) override { return m_models[i].data(); }

    DataTable* IndependentModel() override;
    DataTable* DependentModel() override;

    void OverrideInDependentTable(DataTable* table) override;
    void ResortParameter();

private slots:
    void UpdateCalculated();

private:
    void ApplyConnectType();

    /*! \brief Appends a copy of \p model (tables, counters, names, connections) without preparing the meta model.
     *  \p modelTable also appends its model table (rebuilding the one of the models before). With \p adopted (== \p model)
     *  the model itself is taken instead of a copy. Claude Generated. */
    QSharedPointer<AbstractModel> appendModel(const QPointer<AbstractModel>& model, bool modelTable, const QSharedPointer<AbstractModel>& adopted = {});

    /*! \brief Prepares the meta model once after addModels/adoptModels appended \p added and emits ModelAdded for each. Claude Generated. */
    void finishAdding(const QVector<QSharedPointer<AbstractModel>>& added);

    /*! \brief Prepares the meta model after models were appended: optimisation parameters (connect type) and sliced table. Claude Generated. */
    void prepareModels();

    QVector<QSharedPointer<AbstractModel>> m_models;
    int m_glob_param = 0, m_inp_param = 0, m_loc_param = 0, m_size = 0, m_indep_var = 0, m_dep_var = 0, m_series_count = 0, m_unique_global = 0, m_unique_local = 0, m_unique_series = 0;
    int m_max_indep_var = 0;
    QVector<QVector<QPair<int, int>>> m_global_index, m_local_index;
    QStringList m_global_names, m_local_names, m_original_global, m_original_local;

    QVector<MMParameter> m_mmparameter;

    /* Claude Generated: the parameters each model was calculated with when it was read (ImportAdoptedModels); the next
     * CalculateVariables does not calculate a model again whose parameters it leaves at these values. Empty otherwise. */
    QVector<std::optional<QVector<qreal>>> m_calculated_parameter;

    QVector<QVector<int>> m_indicies;

    QVector<int> m_global_par, m_local_par;

    SupraFit::Model m_model_type;
    ConnectType m_connect_type = ConnectType::None;

    QPointer<DataTable> m_sliced_table;

    QVector<QPointer<DataTable>> m_garbage_table;

    void UpdateSlicedTable();

protected:
    virtual void CalculateVariables() override;

signals:
    void ModelAdded(QSharedPointer<AbstractModel> model);
    void ModelRemoved();
    void ParameterMoved();
    void ParameterSorted();
    void DemandResortParameter();
};
