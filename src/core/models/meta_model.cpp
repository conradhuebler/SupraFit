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

#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"
#include "src/core/models.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "meta_model.h"

MetaModel::MetaModel()
    : AbstractModel(new DataClass())
{
    PrepareParameter(0, 0);
    connect(this, &AbstractModel::Recalculated, this, &MetaModel::UpdateCalculated);
}

MetaModel::~MetaModel()
{
    for (int i = 0; i < m_models.size(); ++i)
        m_models[i].clear();
}

void MetaModel::InitialGuess_Private()
{
    for (int i = 0; i < m_models.size(); ++i)
        m_models[i].data()->InitialGuess();

    Calculate();
}

qreal MetaModel::GlobalParameter(int i) const
{
    QPair<int, int> pair = m_global_par[i];

    if (pair.first == 0)
        return m_combined_global[pair.second].first;
    else
        return m_combined_local[pair.second].first;
}

qreal MetaModel::LocalParameter(int parameter, int series) const
{
    int count = 0;
    for (int i = 0; i < m_local_par.size(); ++i) {
        QPair<int, int> pair = m_local_par[i];
        int index = 0;
        qreal value = 0;
        if (pair.first == 0) {
            index = m_combined_global[pair.second].second.first().first;
            value = m_combined_global[pair.second].first;
        } else {
            index = m_combined_local[pair.second].second.first().first;
            value = m_combined_local[pair.second].first;
        }
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

    m_combined_global.clear();
    m_combined_local.clear();

    if (m_connect_type == ConnectType::All) {

        m_combined_local = QVector<QPair<qreal, QVector<CombinedParameter>>>(m_local_names.size());
        m_combined_global = QVector<QPair<qreal, QVector<CombinedParameter>>>(m_global_names.size());

        for (int i = 0; i < m_models.size(); ++i) {
            for (int j = 0; j < m_global_index[i].size(); ++j) {
                CombinedParameter parameter;
                parameter.first = i;
                parameter.second = m_global_index[i][j];
                QString name = m_models[i]->GlobalParameterName(j);
                m_combined_global[m_global_names.indexOf(name)].second << parameter;
                m_combined_global[m_global_names.indexOf(name)].first += m_models[i]->GlobalParameter(j);
            }
        }
        for (int i = 0; i < m_combined_global.size(); ++i)
            m_combined_global[i].first /= double(m_combined_global[i].second.size());

        for (int i = 0; i < m_models.size(); ++i) {
            for (int j = 0; j < m_local_index[i].size(); ++j) {
                CombinedParameter parameter;
                parameter.first = i;
                parameter.second = m_local_index[i][j];
                QString name = m_models[i]->LocalParameterName(j);
                m_combined_local[m_local_names.indexOf(name)].second << parameter;
                m_combined_local[m_local_names.indexOf(name)].first += m_models[i]->LocalParameter(m_local_index[i][j]);
            }
        }

        for (int i = 0; i < m_combined_local.size(); ++i)
            m_combined_local[i].first /= double(m_combined_local[i].second.size());
        m_series_count = 1;
    } else if (m_connect_type == ConnectType::None) {
        for (int i = 0; i < m_models.size(); ++i) {

            for (int j = 0; j < m_global_index[i].size(); ++j) {
                QVector<CombinedParameter> param;
                CombinedParameter parameter;
                parameter.first = i;
                parameter.second = m_global_index[i][j];
                param << parameter;
                m_combined_global << QPair<qreal, QVector<CombinedParameter>>(m_models[i]->GlobalParameter(j), param);
            }
        }
        for (int i = 0; i < m_models.size(); ++i) {

            for (int j = 0; j < m_local_index[i].size(); ++j) {
                QVector<CombinedParameter> param;
                CombinedParameter parameter;
                parameter.first = i;
                parameter.second = m_local_index[i][j];
                param << parameter;
                m_combined_local << QPair<qreal, QVector<CombinedParameter>>(m_models[i]->LocalParameter(m_local_index[i][j]), param);
            }
        }
        m_series_count = m_models.size();
    }
}

QVector<qreal> MetaModel::OptimizeParameters_Private()
{
    m_local_index.clear();
    m_global_index.clear();

    for (int i = 0; i < m_models.size(); ++i) {
        m_global_index << m_models[i].data()->GlobalIndex();
        m_local_index << m_models[i].data()->LocalIndex();
    }

    ApplyConnectType();

    m_opt_para.clear();
    m_global_par.clear();
    m_local_par.clear();

    QVector<qreal> param;
    for (int i = 0; i < m_combined_global.size(); ++i) {

        if (std::isnan(m_combined_global[i].first))
            continue;

        if (m_combined_global[i].second.size() > 1 && m_models.size() > 1) {
            m_global_par << QPair<int, int>(0, i);
            m_opt_index << QPair<int, int>(param.size(), 0);

        } else if (m_combined_global[i].second.size() == 1 && m_models.size() > 1) {
            m_local_par << QPair<int, int>(0, i);
            m_opt_index << QPair<int, int>(param.size(), 1);

        } else {
            m_global_par << QPair<int, int>(0, i);
            m_opt_index << QPair<int, int>(param.size(), 0);
        }

        m_opt_para << &m_combined_global[i].first;
        param << m_combined_global[i].first;
    }

    for (int i = 0; i < m_combined_local.size(); ++i) {

        if (std::isnan(m_combined_local[i].first))
            continue;
        if (m_combined_local[i].second.size() > 1 && m_models.size() > 1) {
            m_global_par << QPair<int, int>(1, i);
            m_opt_index << QPair<int, int>(param.size(), 0);
        } else if (m_combined_local[i].second.size() == 1 && m_models.size() > 1) {
            m_local_par << QPair<int, int>(1, i);
            m_opt_index << QPair<int, int>(param.size(), 1);
        } else {
            m_global_par << QPair<int, int>(1, i);
            m_opt_index << QPair<int, int>(param.size(), 0);
        }

        m_opt_para << &m_combined_local[i].first;
        param << m_combined_local[i].first;
    }
    m_parameter = param;
    return param;
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

qreal MetaModel::CalculateCovarianceFit()
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
        m_global_names << model->GlobalParameterName(i);

    for (int i = 0; i < model->LocalParameterSize(); ++i)
        m_local_names << model->LocalParameterName(i);

    m_global_names.removeDuplicates();
    m_local_names.removeDuplicates();

    IndependentModel()->append(model->IndependentModel());
    DependentModel()->append(model->DependentModel());
    ModelTable()->append(model->ModelTable());

    m_models << t;
    connect(model, &DataClass::NameChanged, t.data(), &DataClass::setProjectTitle);
    m_glob_param += model->GlobalParameterSize();
    m_inp_param += model->InputParameterSize();
    m_loc_param += model->LocalParameterSize();
    m_size += model->Size();
    m_indep_var += model->IndependentVariableSize();
    m_dep_var += model->DataPoints();
    m_series_count += model->SeriesCount();
    OptimizeParameters_Private();
    emit ModelAdded(t);
}

void MetaModel::UpdateCalculated()
{
    if (m_model_signal)
        delete m_model_signal;
    m_model_signal = new DataTable(0, 0, this);
    for (int i = 0; i < m_models.size(); ++i)
        ModelTable()->append(m_models[i]->ModelTable());
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
    for (int i = 0; i < m_combined_global.size(); ++i) {
        for (int j = 0; j < m_combined_global[i].second.size(); ++j) {
            const CombinedParameter& para = m_combined_global[i].second[j];
            m_models[para.first].data()->setGlobalParameter(m_combined_global[i].first, para.second.first);
        }
    }

    for (int i = 0; i < m_combined_local.size(); ++i) {
        for (int j = 0; j < m_combined_local[i].second.size(); ++j) {
            const CombinedParameter& para = m_combined_local[i].second[j];
            m_models[para.first].data()->setLocalParameter(m_combined_local[i].first, para.second);
        }
    }

    for (int i = 0; i < m_models.size(); ++i) {
        m_models[i].data()->Calculate();
        m_sum_squares += m_models[i].data()->SumofSquares();
        m_used_variables += m_models[i].data()->Points();
        m_sum_absolute += m_models[i].data()->SumofAbsolute();
        m_mean += m_models[i].data()->MeanError();
    }
    if (!m_fast)
        PrepareTables();
}

QSharedPointer<AbstractModel> MetaModel::Clone()
{
    QSharedPointer<MetaModel> model = QSharedPointer<MetaModel>(new MetaModel, &QObject::deleteLater);

    for (const QSharedPointer<AbstractModel> m : Models())
        model.data()->addModel(m->Clone().data());
    model.data()->setConnectType(m_connect_type);
    model.data()->OptimizeParameters_Private();

    return model;
}

void MetaModel::PrepareTables()
{
    QStringList header;

    m_local_parameter->clear();
    QVector<QVector<qreal>> parameters(SeriesCount());
    for (int i = 0; i < m_local_par.size(); ++i) {
        QPair<int, int> pair = m_local_par[i];
        qreal val;
        int index = 0;
        if (pair.first == 0) {
            val = m_combined_global[pair.second].first;
            index = m_combined_global[pair.second].second.first().first;
        } else {
            val = m_combined_local[pair.second].first;
            index = m_combined_local[pair.second].second.first().first;
        }
        parameters[index] << val;
    }
    for (int i = 0; i < parameters.size(); ++i)
        m_local_parameter->insertRow(parameters[i]);

    header = QStringList();

    int size = 0;
    if (GlobalParameterSize())
        size = 1;

    m_global_parameter->clear(GlobalParameterSize(), size);

    m_global_parameter->setCheckedAll(false);

    for (int i = 0; i < GlobalParameterSize(); ++i) {
        (*GlobalTable())[i] = GlobalParameter(i);
        GlobalTable()->setChecked(i, 0, true);
    }
}

bool MetaModel::ImportModel(const QJsonObject& topjson, bool override)
{
    if (topjson["model"].toInt() != SFModel())
        return false;

    AbstractModel::ImportModel(topjson, override);

    int size = topjson["size"].toInt();

    QJsonObject raw = topjson["raw"].toObject();
    bool result = true;
    for (int i = 0; i < size; ++i) {
        bool import = m_models[i]->ImportModel(raw[QString::number(i)].toObject(), override);
        result = result && import;
    }
    m_connect_type = (ConnectType)topjson["connecttype"].toInt();
    OptimizeParameters_Private();
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
    model["raw"] = raw;
    model["connecttype"] = m_connect_type;

    return model;
}

void MetaModel::DebugParameter() const
{
    //qDebug() << m_global_names;
    //qDebug() << m_combined_global;
    for (int i = 0; i < m_combined_global.size(); ++i) {
        qDebug() << m_combined_global[i].first;
        for (int j = 0; j < m_combined_global[i].second.size(); ++j) {
            int model = m_combined_global[i].second[j].first;
            qDebug() << m_combined_global[i].second[j].second;
            //int index = m_combined_global[i].second[j].second;
            //m_models[model]->GlobalParameterName(index);
        }
    }
}
#include "meta_model.moc"
