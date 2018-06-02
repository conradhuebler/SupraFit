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
/* if (i >= m_glob_param)
        return 0;

    int model = 0;
    int index = 0;

    if (i) {
        model = m_glob_param / i - 1;
        index = m_glob_param % i;
    }
    return m_models[model]->GlobalParameter(index);*/
#warning this is a hack ...

    return m_parameter[i][i];
}

void MetaModel::setConnectType(ConnectType type)
{
    if (m_global_index.isEmpty() && m_local_index.isEmpty())
        return;
    OptimizeParameters_Private();

    m_combined_global.clear();
    m_combined_local.clear();

    if (type == ConnectType::All) {

        m_combined_local = QVector<QVector<CombinedParameter>>(m_local_names.size());
        m_combined_global = QVector<QVector<CombinedParameter>>(m_global_names.size());

        for (int i = 0; i < m_models.size(); ++i) {
            for (int j = 0; j < m_global_index[i].size(); ++j) {
                CombinedParameter parameter;
                parameter.model = i;
                parameter.global = 0;
                parameter.index = QPair<int, int>(j, 0);
                parameter.name = m_models[i]->GlobalParameterName(j);
                m_combined_global[m_global_names.indexOf(parameter.name)] << parameter;
            }
        }
        for (int i = 0; i < m_models.size(); ++i) {
            for (int j = 0; j < m_local_index[i].size(); ++j) {
                CombinedParameter parameter;
                parameter.model = i;
                parameter.global = 1;
                parameter.index = QPair<int, int>(j, 0);
                parameter.name = m_models[i]->LocalParameterName(j);
                m_combined_local[m_local_names.indexOf(parameter.name)] << parameter;
            }
        }
    } else if (type == ConnectType::None) {
        for (int i = 0; i < m_models.size(); ++i) {
            QVector<CombinedParameter> param;

            for (int j = 0; j < m_global_index[i].size(); ++j) {
                CombinedParameter parameter;
                parameter.model = i;
                parameter.global = 0;
                parameter.index = QPair<int, int>(j, 0);
                parameter.name = m_models[i]->GlobalParameterName(j);
                param << parameter;
            }
            m_combined_global << param;
        }
        for (int i = 0; i < m_models.size(); ++i) {
            QVector<CombinedParameter> param;
            for (int j = 0; j < m_local_index[i].size(); ++j) {
                CombinedParameter parameter;
                parameter.model = i;
                parameter.global = 1;
                parameter.index = QPair<int, int>(j, 0);
                parameter.name = m_models[i]->LocalParameterName(j);
                param << parameter;
            }
            m_combined_local << param;
        }
    }
}

QVector<qreal> MetaModel::OptimizeParameters_Private()
{
    m_local_index.clear();
    m_global_index.clear();

    QVector<QVector<qreal>> parameter;
    for (int i = 0; i < m_models.size(); ++i) {
        parameter << m_models[i].data()->OptimizeParameters();
        m_global_index << m_models[i].data()->GlobalIndex();
        m_local_index << m_models[i].data()->LocalIndex();
    }

    m_parameter = parameter;

    m_opt_para.clear();
    QVector<qreal> param;
    for (int i = 0; i < parameter.size(); ++i)
        for (int j = 0; j < parameter[i].size(); ++j) {
            m_opt_para << &m_parameter[i][j];
            param << m_parameter[i][j];
        }

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
    if (m_models.size() == 0 && model->SFModel() != SupraFit::itc_blank) {
        m_model_type = model->SFModel();
        m_unique_global = model->GlobalParameterSize();
        m_unique_local = model->LocalParameterSize();
        m_unique_series = model->SeriesCount();
    } else {
        /* We support dilution only for ITCModels
         */
        if (qobject_cast<AbstractItcModel*>(model)) {
            if (m_model_type != model->SFModel() && model->SFModel() != SupraFit::itc_blank) {
                m_model_identic = false;
                m_unique_global = 0;
                m_unique_local = 0;
                m_unique_series = 0;
            } else if (m_model_type != model->SFModel() && model->SFModel() == SupraFit::itc_blank)
                m_contains_dilution = true;
        }
    }
    for (int i = 0; i < model->GlobalParameterSize(); ++i)
        m_global_names << model->GlobalParameterName(i);

    for (int i = 0; i < model->LocalParameterSize(); ++i)
        m_local_names << model->LocalParameterName(i);

    m_global_names.removeDuplicates();
    m_local_names.removeDuplicates();

    m_models << t;
    connect(model, &DataClass::NameChanged, t.data(), &DataClass::setProjectTitle);
    m_glob_param += model->GlobalParameterSize();
    m_inp_param += model->InputParameterSize();
    m_loc_param += model->LocalParameterSize();
    m_size += model->Size();
    m_indep_var += model->IndependentVariableSize();
    m_dep_var += model->DataPoints();
    m_series_count += model->SeriesCount();
    emit ModelAdded(t);
}

void MetaModel::CombineParameter()
{
    if (m_parameter.isEmpty())
        return;

    for (int i = 0; i < m_combined_global.size(); ++i) {
        qreal value;
        for (int j = 0; j < m_combined_global[i].size(); ++j) {

            if (j == 0)
                value = m_parameter[m_combined_global[i][j].model][m_combined_global[i][j].index.first];
            else
                m_parameter[m_combined_global[i][j].model][m_combined_global[i][j].index.first] = value;
        }
    }

    for (int i = 0; i < m_combined_local.size(); ++i) {
        qreal value;
        for (int j = 0; j < m_combined_local[i].size(); ++j) {
            {
                if (j == 0)
                    value = m_parameter[m_combined_local[i][j].model][m_combined_local[i][j].index.first + m_models[m_combined_local[i][j].model].data()->GlobalParameterSize()];
                else
                    m_parameter[m_combined_local[i][j].model][m_combined_local[i][j].index.first + m_models[m_combined_local[i][j].model].data()->GlobalParameterSize()] = value;
            }
        }
    }
}

void MetaModel::CalculateVariables()
{
    CombineParameter();

    for (int i = 0; i < m_global_index.size(); ++i) {
        for (int j = 0; j < m_global_index[i].size(); ++j) {
            QPair<int, int> pair = m_global_index[i][j];
            m_models[i].data()->setGlobalParameter(m_parameter[i][j], pair.first);
        }
    }

    for (int i = 0; i < m_local_index.size(); ++i) {
        for (int j = 0; j < m_local_index[i].size(); ++j) {
            QPair<int, int> pair = m_local_index[i][j];
            m_models[i].data()->setLocalParameter(m_parameter[i][j + m_models[i].data()->GlobalParameterSize()], pair.first, pair.second);
        }
    }

    for (int i = 0; i < m_models.size(); ++i) {
        m_models[i].data()->Calculate();
    }
}

QSharedPointer<AbstractModel> MetaModel::Clone()
{
    QSharedPointer<MetaModel> model = QSharedPointer<MetaModel>(new MetaModel, &QObject::deleteLater);

    for (const QSharedPointer<AbstractModel> m : Models())
        model.data()->addModel(m->Clone().data());

    return model;
}

bool MetaModel::ImportModel(const QJsonObject& topjson, bool override)
{
    if (topjson["model"].toInt() != SFModel())
        return false;

    int size = topjson["size"].toInt();

    QJsonObject raw = topjson["raw"].toObject();
    bool result = true;
    for (int i = 0; i < size; ++i) {
        bool import = m_models[i]->ImportModel(raw[QString::number(i)].toObject(), override);
        result = result && import;
    }
    return result;
}

QJsonObject MetaModel::ExportModel(bool statistics, bool locked) const
{
    QJsonObject model;
    model["size"] = m_models.size();
    model["model"] = SFModel();
    QJsonObject raw;
    for (int i = 0; i < m_models.size(); ++i) {
        raw[QString::number(i)] = m_models[i]->ExportModel(statistics, locked);
    }
    model["raw"] = raw;
    return model;
}

#include "meta_model.moc"
