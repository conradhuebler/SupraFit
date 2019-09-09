/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"
#include "src/core/models.h"
#include "src/core/toolset.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "meta_model.h"

MetaModel::MetaModel(DataClass* data)
    : AbstractModel(data)
{
    PrepareParameter(0, 0);
    //connect(this, &AbstractModel::Recalculated, this, &MetaModel::UpdateCalculated);
    connect(this, &DataClass::ProjectTitleChanged, this, [this](const QString& str) {
        m_name = str;
    });
    //connect(this, &MetaModel::ParameterMoved, this, &MetaModel::ResortParameter);
    m_name = ProjectTitle();
}

MetaModel::~MetaModel()
{
    for (int i = 0; i < m_models.size(); ++i)
        m_models[i].clear();

    /*    for (int i = 0; i < m_garbage_table.size(); ++i)
        if (m_garbage_table[i])
            delete m_garbage_table[i];
        */
}

void MetaModel::InitialGuess_Private()
{
    for (int i = 0; i < m_models.size(); ++i)
        m_models[i].data()->InitialGuess();

    Calculate();
}

qreal MetaModel::GlobalParameter(int i) const
{
    int index = m_global_par[i];

    return m_mmparameter[index].first;
}

QString MetaModel::GlobalParameterName(int j) const
{
    return m_global_names[j];
}

QString MetaModel::LocalParameterName(int j) const
{
    return m_local_names[j];
}

qreal MetaModel::LocalParameter(int parameter, int series) const
{
    int count = 0;

    for (int i = 0; i < m_local_par.size(); ++i) {

        int param_index = m_local_par[i];
        int index = 0;
        qreal value = 0;

        index = m_mmparameter[param_index].second.first()[0];
        value = m_mmparameter[param_index].first;

        if (index == series) {
            if (count == parameter)
                return value;
            count++;
        }
    }

    return 0;
}

void MetaModel::ApplyConnectType()
{
    if (m_connect_type == ConnectType::Custom)
        return;

    m_mmparameter.clear();

    if (m_connect_type == ConnectType::All) {

        m_mmparameter.resize(m_local_names.size() + m_global_names.size());

        for (int i = 0; i < m_models.size(); ++i) {
            for (int j = 0; j < m_global_index[i].size(); ++j) {

                QVector<int> param(4);
                param[0] = i;
                param[1] = 0;
                param[2] = m_global_index[i][j].first;
                param[3] = m_global_index[i][j].second;

                QString name = m_models[i]->GlobalParameterName(j);

                m_mmparameter[m_original_global.indexOf(name)].second << param;
                m_mmparameter[m_original_global.indexOf(name)].first += m_models[i]->GlobalParameter(j);
            }
        }

        for (int i = 0; i < m_models.size(); ++i) {
            for (int j = 0; j < m_local_index[i].size(); ++j) {

                QVector<int> param(4);
                param[0] = i;
                param[1] = 1;
                param[2] = m_local_index[i][j].first;
                param[3] = m_local_index[i][j].second;

                QString name = m_models[i]->LocalParameterName(j);

                m_mmparameter[m_original_global.size() + m_original_local.indexOf(name)].second << param;
                m_mmparameter[m_original_global.size() + m_original_local.indexOf(name)].first += m_models[i]->LocalParameter(m_local_index[i][j]);
            }
        }

        m_series_count = 1;
    } else if (m_connect_type == ConnectType::None) {
        for (int i = 0; i < m_models.size(); ++i) {

            for (int j = 0; j < m_global_index[i].size(); ++j) {

                QVector<int> para(4);
                para[0] = i;
                para[1] = 0;
                para[2] = m_global_index[i][j].first;
                para[3] = m_global_index[i][j].second;

                MMParameter p(m_models[i]->GlobalParameter(j), { para });
                m_mmparameter << p;
            }
        }
        for (int i = 0; i < m_models.size(); ++i) {

            for (int j = 0; j < m_local_index[i].size(); ++j) {

                QVector<int> para(4);
                para[0] = i;
                para[1] = 1;
                para[2] = m_local_index[i][j].first;
                para[3] = m_local_index[i][j].second;

                MMParameter p(m_models[i]->LocalParameter(m_local_index[i][j]), { para });
                m_mmparameter << p;
            }
        }
        m_series_count = m_models.size();
    }

    for (int i = m_mmparameter.size() - 1; i >= 0; --i) {
        if (std::isnan(m_mmparameter[i].first) || m_mmparameter[i].second.size() == 0)
            m_mmparameter.remove(i);
    }
    m_series_count = 1;
    ResortParameter();

    for (int i = 0; i < m_mmparameter.size(); ++i) {
        m_mmparameter[i].first /= double(m_mmparameter[i].second.size());
    }
}

void MetaModel::ResortParameter()
{
    // return;

    QMutexLocker mutex(&m_mutex);
    QMultiMap<qreal, int> indicies;

    for (int i = 0; i < m_mmparameter.size(); ++i) {
        qreal index = m_mmparameter[i].second.size();
        // qreal value = m_mmparameter[i].first;
        // index += value / pow(10, ceil(log10(abs(value)))) * (-1 * value < 0);
        indicies.insert(-1 * index, i);
    }
    QVector<MMParameter> mmparameter;

    for (const auto& i : indicies)
        mmparameter << m_mmparameter[i];

    m_mmparameter = mmparameter;
    emit ParameterSorted();
}

void MetaModel::OptimizeParameters_Private()
{
    m_local_index.clear();
    m_global_index.clear();
    m_indicies.clear();

    for (int i = 0; i < m_models.size(); ++i) {
        m_global_index << m_models[i].data()->GlobalIndex();
        m_local_index << m_models[i].data()->LocalIndex();
    }

    ApplyConnectType();

    CollectParameter();
}

QVector<qreal> MetaModel::CollectParameter()
{
    m_opt_para.clear();
    m_global_par.clear();
    m_local_par.clear();
    m_opt_index.clear();

    int local_count = 0;
    int global_count = 0;

    m_global_names.clear();
    m_local_names.clear();

    for (int i = 0; i < m_mmparameter.size(); ++i) {
        MMParameter parameter = m_mmparameter[i];
        bool global = (parameter.second.size() > 1) || m_models.size() == 1;
        m_opt_para << &m_mmparameter[i].first;
        m_parameter << parameter.first;

        if (global) {
            m_opt_index << QPair<int, int>(global_count, 0);
            m_global_par << i;
            global_count++;
        } else {
            m_opt_index << QPair<int, int>(local_count, 1);
            m_local_par << i;
            local_count++;
        }
        QStringList names;
        for (int j = 0; j < parameter.second.size(); ++j) {
            QSharedPointer<AbstractModel> model = Models()[parameter.second[j][0]];

            if (parameter.second[j][1] == 0)
                names << model->GlobalParameterName(parameter.second[j][2]);
            else
                names << model->LocalParameterName(parameter.second[j][2]);
        }
        names.removeDuplicates();
        if (global)
            m_global_names << names.join(";");
        else
            m_local_names << names.join(";");
    }

    m_local_parameter->clear(m_local_names.size(), 1);
    m_global_parameter->clear(m_global_names.size(), 1);

    LocalTable()->setHeader(m_local_names);
    GlobalTable()->setHeader(m_global_names);

    return m_parameter;
}

QVector<qreal> MetaModel::AllParameter() const
{
    QVector<qreal> parameter;
    for (int i = 0; i < m_mmparameter.size(); ++i)
        parameter << m_mmparameter[i].first;
    return parameter;
}

void MetaModel::forceGlobalParameter(double value, int parameter)
{
    m_mmparameter[parameter].first = value;
}

void MetaModel::forceLocalParameter(qreal value, int parameter, int series)
{
    Q_UNUSED(series)
    m_mmparameter[parameter].first = value;
}

QList<double> MetaModel::getSignals(QList<int> active_signal)
{
    QList<double> x;

    if (active_signal.size() < m_models.size())
        active_signal = QVector<int>(m_models.size(), 1).toList();

    for (int index = 0; index < m_models.size(); ++index) {
        if (active_signal[index] == 0)
            continue;

        QList<int> local_signals = m_models[index]->ActiveSignals();

        if (local_signals.size() < m_models[index]->SeriesCount())
            local_signals = QVector<int>(m_models[index]->SeriesCount(), 1).toList();

        for (int j = 0; j < m_models[index]->SeriesCount(); ++j) {
            if (local_signals[j] != 1)
                continue;

            for (int i = 0; i < m_models[index]->DataPoints(); ++i) {
                if (m_models[index]->DependentModel()->isChecked(j, i))
                    x.append(m_models[index]->DependentModel()->data(j, i));
            }
        }
    }
    return x;
}

qreal MetaModel::ModelError() const
{
    qreal error = 0;
    for (int index = 0; index < m_models.size(); ++index) {
        for (int z = 0; z < m_models[index]->SeriesCount(); ++z) {
            if (!m_models[index]->ActiveSignals(z))
                continue;
            error += m_models[index]->SumOfErrors(z);
        }
    }
    return error;
}

qreal MetaModel::CalculateVariance()
{
    qreal v = 0;
    int count = 0;
    for (int index = 0; index < m_models.size(); ++index) {
        for (int i = 0; i < m_models[index]->DataPoints(); ++i) {
            for (int j = 0; j < m_models[index]->SeriesCount(); ++j) {
                if (!m_models[index]->ActiveSignals(j))
                    continue;
                if (m_models[index]->DependentModel()->isChecked(j, i)) {
                    v += qPow(m_models[index]->ErrorTable()->data(j, i) - m_mean, 2);
                    count++;
                }
            }
        }
    }
    return v / (count - 1);
}

qreal MetaModel::CalculateCovarianceFit() const
{
    qreal model = 0, data = 0;
    int count = 0;
    qreal cov_data = 0, cov_model = 0;
    for (int index = 0; index < m_models.size(); ++index) {
        for (int i = 0; i < m_models[index]->DataPoints(); ++i) {
            for (int j = 0; j < m_models[index]->SeriesCount(); ++j) {
                if (!m_models[index]->ActiveSignals(j))
                    continue;

                if (m_models[index]->DependentModel()->isChecked(j, i)) {
                    model += m_model_signal->data(j, i);
                    data += m_models[index]->DependentModel()->data(j, i);
                    count++;
                }
            }
        }
    }
    double mean_model = model / double(count);
    double mean_data = data / double(count);
    for (int index = 0; index < m_models.size(); ++index) {

        for (int i = 0; i < m_models[index]->DataPoints(); ++i) {
            for (int j = 0; j < m_models[index]->SeriesCount(); ++j) {
                if (!m_models[index]->ActiveSignals(j))
                    continue;
                if (m_models[index]->DependentModel()->isChecked(j, i)) {
                    cov_data += qPow(m_models[index]->ModelTable()->data(j, i) - mean_model, 2);
                    cov_model += qPow(m_models[index]->DependentModel()->data(j, i) - mean_data, 2);
                }
            }
        }
    }
    return cov_model / cov_data;
}

qreal MetaModel::SumOfErrors(int i) const
{
    qreal sum = 0;
    for (int index = 0; index < m_models.size(); ++index) {

        if (!m_models[index]->ActiveSignals(i) || i >= m_models[index]->Size())
            return sum;

        for (int j = 0; j < m_models[index]->DataPoints(); ++j) {
            sum += qPow(m_models[index]->ErrorTable()->data(i, j), 2);
        }
    }
    return sum;
}

QVector<qreal> MetaModel::ErrorVector() const
{
    QVector<qreal> error;

    for (const QSharedPointer<AbstractModel> model : m_models)
        error << model->ErrorVector();

    return error;
}
QList<double> MetaModel::getCalculatedModel()
{
    QList<double> x;
    for (int index = 0; index < m_models.size(); ++index) {
        for (int j = 0; j < m_models[index]->SeriesCount(); ++j) {
            if (!m_models[index]->ActiveSignals(j))
                continue;
            for (int i = 0; i < m_models[index]->DataPoints(); ++i)
                if (m_models[index]->DependentModel()->isChecked(j, i))
                    x.append(m_models[index]->ModelTable()->data(j, i));
        }
    }
    return x;
}

void MetaModel::addModel(const QPointer<AbstractModel> model)
{
    /* We will create a copy of that model
     */
    QSharedPointer<AbstractModel> t = model->Clone();

    for (int i = 0; i < model->GlobalParameterSize(); ++i)
        m_original_global << model->GlobalParameterName(i);

    for (int i = 0; i < model->LocalParameterSize(); ++i)
        m_original_local << model->LocalParameterName(i);

    t->ReleaseLocks();

    m_original_global.removeDuplicates();
    m_local_names.removeDuplicates();

    /* We shall not use the derived functions but the base functions of DataClass since at the moment, we dont have real data yet */
    DataClass::IndependentModel()->append(model->IndependentModel());
    DataClass::DependentModel()->append(model->DependentModel());
    ModelTable()->append(model->ModelTable());

    m_models << t;
    connect(model, &DataClass::ProjectTitleChanged, t.data(), &DataClass::setProjectTitle);
    m_glob_param += model->GlobalParameterSize();
    m_inp_param += model->InputParameterSize();
    m_loc_param += model->LocalParameterSize();
    m_size += model->Size();
    m_indep_var += model->IndependentVariableSize();
    m_dep_var += model->DataPoints();
    m_max_indep_var = qMax(m_max_indep_var, model->DataPoints());
    m_series_count += model->SeriesCount();
    OptimizeParameters_Private();
    UpdateSlicedTable();
    //DataClass::setProjectTitle("MetaModel (" + QString::number(m_models.size()) + ")");
    connect(this, &DataClass::Message, model, &DataClass::Message);
    connect(this, &DataClass::Warning, model, &DataClass::Warning);

    emit ModelAdded(t);
}

void MetaModel::RemoveModel(const AbstractModel* model)
{
    QVector<QSharedPointer<AbstractModel>> models = m_models;
    m_models.clear();

    IndependentModel()->clear();
    DependentModel()->clear();
    AbstractModel::ModelTable()->clear();

    m_glob_param = 0;
    m_inp_param = 0;
    m_loc_param = 0;
    m_size = 0;
    m_indep_var = 0;
    m_max_indep_var = 0;

    m_dep_var = 0;
    m_series_count = 0;

    m_original_global.clear();
    m_original_local.clear();

    m_mmparameter.clear();

    for (int l = 0; l < models.size(); ++l) {
        if (models[l].data() == model)
            continue;

        for (int i = 0; i < models[l].data()->GlobalParameterSize(); ++i)
            m_original_global << models[l].data()->GlobalParameterName(i);

        for (int i = 0; i < models[l].data()->LocalParameterSize(); ++i)
            m_original_local << models[l].data()->LocalParameterName(i);

        IndependentModel()->append(models[l].data()->IndependentModel());
        DependentModel()->append(models[l].data()->DependentModel());
        ModelTable()->append(models[l].data()->ModelTable());

        m_models << models[l];

        m_glob_param += models[l].data()->GlobalParameterSize();
        m_inp_param += models[l].data()->InputParameterSize();
        m_loc_param += models[l].data()->LocalParameterSize();
        m_size += models[l].data()->Size();
        m_indep_var += models[l].data()->IndependentVariableSize();
        m_max_indep_var = qMax(m_max_indep_var, model->DataPoints());

        m_dep_var += models[l].data()->DataPoints();
        m_series_count += models[l].data()->SeriesCount();
    }

    m_original_global.removeDuplicates();
    m_local_names.removeDuplicates();

    OptimizeParameters_Private();
    UpdateSlicedTable();

    //DataClass::setProjectTitle("MetaModel");
    emit ModelRemoved();
}

void MetaModel::UpdateCalculated()
{
    QMutexLocker mutex(&m_mutex);

    if (m_model_signal)
        delete m_model_signal;
    m_model_signal = new DataTable(0, 0, this);
    for (int i = 0; i < m_models.size(); ++i)
        AbstractModel::ModelTable()->append(m_models[i]->ModelTable());
}

void MetaModel::IndependentModelOverride()
{
    int pred = 0;
    for (int i = 0; i < m_models.size(); ++i) {
        m_models[i].data()->OverrideInDependentTable(IndependentModel()->Block(pred, 0, m_models[i]->IndependentModel()->rowCount(), 1));
        pred += m_models[i]->IndependentModel()->rowCount();
    }
}

void MetaModel::DependentModelOverride()
{
    int pred = 0;
    for (int i = 0; i < m_models.size(); ++i) {
        DataTable* table = DependentModel()->Block(pred, 0, m_models[i]->DependentModel()->rowCount(), 1);
        m_models[i].data()->OverrideCheckedTable(table);
        m_models[i].data()->OverrideDependentTable(table);
        pred += m_models[i]->DependentModel()->rowCount();
    }
}

void MetaModel::CalculateVariables()
{
    QMutexLocker mutex(&m_mutex);
    for (int i = 0; i < m_mmparameter.size(); ++i) {
        MMParameter parameter = m_mmparameter[i];
        qreal value = parameter.first;
        for (int j = 0; j < parameter.second.size(); ++j) {
            QVector<int> vector = parameter.second[j];
#pragma message("FIXME strange")
            if (vector.isEmpty()) {
                qDebug() << "what happend here?";
                continue;
            }
            if (vector[1] == 0)
                m_models[vector[0]].data()->setGlobalParameter(value, vector[2]);
            else if (vector[1] == 1)
                m_models[vector[0]].data()->setLocalParameter(value, vector[2], vector[3]);
        }
    }

    for (int i = 0; i < m_models.size(); ++i) {
        m_models[i].data()->Calculate();
        m_sum_squares += m_models[i].data()->SSE();
        m_used_variables += m_models[i].data()->Points();
        m_sum_absolute += m_models[i].data()->SAE();
        m_mean += m_models[i].data()->MeanError();
    }
    if (!m_fast)
        PrepareTables();
}

QSharedPointer<AbstractModel> MetaModel::Clone(bool statistics)
{
    QSharedPointer<MetaModel> model = QSharedPointer<MetaModel>(new MetaModel(new DataClass()), &QObject::deleteLater);

    for (const QSharedPointer<AbstractModel>& m : Models())
        model.data()->addModel(m->Clone(statistics).data());
    model.data()->setConnectType(m_connect_type);
    model.data()->m_mmparameter = m_mmparameter;
    model.data()->OptimizeParameters_Private();

    return model;
}

void MetaModel::PrepareTables()
{
    QVector<qreal> parameters = AllParameter();

    m_local_parameter->clear(m_local_names.size(), 1);
    for (int i = 0; i < m_local_names.size(); ++i)
        m_local_parameter->data(i, 0) = parameters[m_global_names.size() + i];
    m_local_parameter->setHeader(m_local_names);

    m_global_parameter->clear(m_global_names.size(), 1);
    for (int i = 0; i < m_global_names.size(); ++i)
        m_global_parameter->data(i, 0) = parameters[i];
    m_global_parameter->setHeader(m_global_names);
}

bool MetaModel::ImportModel(const QJsonObject& topjson, bool override)
{
    if (topjson["model"].toInt() != SFModel())
        return false;

    AbstractModel::ImportModel(topjson, override);
    int size = topjson["size"].toInt();
    if (size != m_models.size()) {
        emit Info()->Message(QString("Stored models in json file (%1) and available models in memory (%2) dont fit").arg(size).arg(m_models.size()));
        emit Info()->Message(QString("Unique Identification for this model is %1").arg(UUID()));
    }
    QJsonObject raw = topjson["raw"].toObject();
    bool result = true;
    for (int i = 0; i < size && i < m_models.size(); ++i) {
        bool import = m_models[i]->ImportModel(raw[QString::number(i)].toObject(), override);
        result = result && import;
    }
    m_connect_type = static_cast<ConnectType>(topjson["connecttype"].toInt());
    if (m_connect_type == ConnectType::Custom) {
        m_mmparameter.clear();
        QJsonObject parameter = topjson["parameter"].toObject();
        int size = parameter["size"].toInt();
        for (int i = 0; i < size; ++i) {

            QStringList seq = parameter[QString::number(i)].toString().split(":");
            double value = seq.first().toDouble();
            QVector<QVector<int>> parm = ToolSet::String2Int2DVec(seq.last());
            m_mmparameter << MMParameter(value, parm);
        }
    }
    ApplyConnectType();
    CollectParameter();
    OptimizeParameters_Private();
    Calculate();
    // emit ParameterMoved();
    DataClass::setProjectTitle(topjson["name"].toString());

    return result;
}

QJsonObject MetaModel::ExportModel(bool statistics, bool locked)
{
    PrepareTables();
    QJsonObject model = AbstractModel::ExportModel(statistics, locked);
    model["size"] = m_models.size();
    QJsonObject raw;
    for (int i = 0; i < m_models.size(); ++i) {
        raw[QString::number(i)] = m_models[i]->ExportModel(statistics, locked);
    }
    QJsonObject uuid;
    for (int i = 0; i < m_models.size(); ++i) {
        uuid[QString::number(i)] = m_models[i]->UUID();
    }
    model["raw"] = raw;
    model["uuids"] = uuid;
    model["connecttype"] = m_connect_type;
    if (m_connect_type == ConnectType::Custom) {
        QJsonObject parameter;
        parameter["size"] = m_mmparameter.size();
        for (int i = 0; i < m_mmparameter.size(); ++i)
            parameter[QString::number(i)] = QString::number(m_mmparameter[i].first) + ":" + ToolSet::Int2DVec2String(m_mmparameter[i].second);
        model["parameter"] = parameter;
    }
    return model;
}

void MetaModel::DebugParameter() const
{
    qDebug() << m_mmparameter << this;
}

void MetaModel::MoveParameterList(int source, int destination)
{
    m_connect_type = ConnectType::Custom;

    if (source == destination)
        return;
    if (source < m_mmparameter.size() && destination < m_mmparameter.size()) {
        MMParameter parameter = m_mmparameter[source];
        for (int i = 0; i < parameter.second.size(); ++i)
            m_mmparameter[destination].second << parameter.second[i];
        m_mmparameter.takeAt(source);
    }
    ResortParameter();
    CollectParameter();
    emit ParameterMoved();
}

void MetaModel::MoveSingleParameter(int parameter_index_1, int parameter_index_2, int destination)
{
    m_connect_type = ConnectType::Custom;

    if (parameter_index_1 == destination)
        return;
    if (parameter_index_2 == -1 && destination == -1 && parameter_index_1 < m_mmparameter.size()) {
        MMParameter parameter = m_mmparameter.takeAt(parameter_index_1);
        for (int i = 0; i < parameter.second.size(); ++i) {
            MMParameter p(parameter.first, { parameter.second[i] });
            m_mmparameter << p;
        }
    } else if (parameter_index_2 != -1 && destination == -1) {
        if (m_mmparameter[parameter_index_1].second.size() == 1)
            return;

        MMParameter parameter = m_mmparameter[parameter_index_1];
        m_mmparameter[parameter_index_1].second.takeAt(parameter_index_2);
        parameter.second = QVector<QVector<int>>() << parameter.second[parameter_index_2];
        m_mmparameter << parameter;

    } else if (parameter_index_1 < m_mmparameter.size() && destination < m_mmparameter.size()) {
        MMParameter parameter = m_mmparameter[parameter_index_1];
        m_mmparameter[destination].second << parameter.second[parameter_index_2];
        m_mmparameter[parameter_index_1].second.takeAt(parameter_index_2);
        if (m_mmparameter[parameter_index_1].second.size() == 0)
            m_mmparameter.takeAt(parameter_index_1);
    } else
        return;

    ResortParameter();
    CollectParameter();
    emit ParameterMoved();
}

int MetaModel::LocalParameterSize(int i) const
{
    if (m_connect_type == ConnectType::None && i < m_models.size())
        return m_models[i]->LocalParameterSize();
    else if (m_connect_type == ConnectType::All)
        return m_local_par.size();
    else if (m_connect_type == ConnectType::Custom) {
        return m_local_names.size();
    } else
        return 0;
}

void MetaModel::UpdateSlicedTable()
{
    if (m_sliced_table)
        delete m_sliced_table;

    m_sliced_table = new DataTable(m_indep_var, m_max_indep_var, this);
    QStringList header;
    for (int i = 0; i < ModelSize(); ++i) {
        for (const QString& name : m_models[i]->IndependentModel()->header())
            header << QString("%1 | %2").arg(m_models[i]->ProjectTitle()).arg(name);

        for (int j = 0; j < m_models[i]->IndependentVariableSize(); ++j) {
            for (int k = 0; k < m_models[i]->DataPoints(); ++k) {
                m_sliced_table->operator()(i + j, k) = m_models[i]->IndependentModel()->operator()(j, k);
                m_sliced_table->setChecked(j, k, m_models[i]->IndependentModel()->isChecked(j, k));
            }
        }
    }
    m_sliced_table->setHeader(header);
}

DataTable* MetaModel::IndependentModel()
{
    return m_sliced_table;
}

DataTable* MetaModel::DependentModel()
{
    return DataClass::DependentModel();
    /*
    QPointer<DataTable > table = new DataTable;
    
     for (int l = 0; l < m_models.size(); ++l) {
        table->append(m_models[l].data()->DependentModel());
    //    m_models[l].data()->DependentModel()->Debug("Moving tables");
    }
    
    //table->Debug("All tables moved around");
    
    m_garbage_table << table;
    
    return table;*/
}

void MetaModel::OverrideInDependentTable(DataTable* table)
{
    for (int i = 0; i < ModelSize(); ++i) {
        m_models[i]->detach();
        for (int j = 0; j < m_models[i]->IndependentVariableSize(); ++j) {
            for (int k = 0; k < m_models[i]->DataPoints(); ++k) {
                m_models[i]->IndependentModel()->operator()(j, k) = table->operator()(i + j, k);
                m_models[i]->IndependentModel()->setChecked(j, k, table->isChecked(j, k));
            }
        }
    }
}

#include "meta_model.moc"
