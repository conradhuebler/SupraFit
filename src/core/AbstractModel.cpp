/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/dataclass.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"
#include "src/global.h"
#include "src/version.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <iostream>

#include "AbstractModel.h"

AbstractModel::AbstractModel(DataClass* data)
    : DataClass(data)
    , m_corrupt(false)
    , m_last_p(1)
    , m_f_value(1)
    , m_last_parameter(0)
    , m_last_freedom(0)
    , m_converged(false)
    , m_locked_model(false)
    , m_fast(true)
{
    connect(this, &DataClass::SystemParameterChanged, this, &AbstractModel::UpdateParameter);

    connect(this, &AbstractModel::OptionChanged, this, &AbstractModel::UpdateOption);
    d = new AbstractModelPrivate;
    setActiveSignals(QVector<int>(SeriesCount(), 1).toList());

    m_model_signal = new DataTable(SeriesCount(), DataPoints(), this);
    m_model_error = new DataTable(SeriesCount(), DataPoints(), this);

    connect(this, &DataClass::SystemParameterChanged, this, &AbstractModel::Calculate);
}

AbstractModel::AbstractModel(AbstractModel* model)
    : DataClass(model)
    , m_corrupt(model->m_corrupt)
    , m_last_p(model->m_last_p)
    , m_f_value(model->m_f_value)
    , m_last_parameter(model->m_last_parameter)
    , m_last_freedom(model->m_last_freedom)
    , m_converged(model->m_converged)
    , m_locked_model(model->m_locked_model)
    , m_fast(true)
    , d(new AbstractModelPrivate(*model->d))
{
    connect(this, &DataClass::SystemParameterChanged, this, &AbstractModel::UpdateParameter);
    connect(this, &AbstractModel::OptionChanged, this, &AbstractModel::UpdateOption);

    setActiveSignals(QVector<int>(SeriesCount(), 1).toList());

    m_model_signal = new DataTable(SeriesCount(), DataPoints(), this);
    m_model_error = new DataTable(SeriesCount(), DataPoints(), this);

    connect(this, &DataClass::SystemParameterChanged, this, &AbstractModel::Calculate);
}

void AbstractModel::PrepareParameter(int global, int local)
{
    QStringList header;

    if (!LocalTable())
        m_local_parameter = new DataTable(local, SeriesCount(), this);
    for (int i = 0; i < LocalParameterSize(); ++i)
        header << LocalParameterName(i);
    LocalTable()->setHeader(header);

    header = QStringList();
    if (!GlobalTable())
        m_global_parameter = new DataTable(global, 1, this);
    for (int i = 0; i < GlobalParameterSize(); ++i)
        header << GlobalParameterName(i);
    GlobalTable()->setHeader(header);

    d->m_enabled_global = QVector<int>(global, 0);
    d->m_enabled_local = QVector<int>(local, 0);

    addGlobalParameter();
    DeclareSystemParameter();
    DeclareOptions();
    LoadSystemParameter();
    UpdateParameter();
}

AbstractModel::~AbstractModel()
{
    if (m_model_signal)
        delete m_model_signal;

    if (m_model_error)
        delete m_model_error;

}

QVector<qreal> AbstractModel::OptimizeParameters()
{
    clearOptParameter();
    QVector<qreal> variables = OptimizeParameters_Private();
    /* I really hope, this is old und already obsolete stuff, can remember right now */
    /*qDebug() << m_opt_para;
    for (int j = m_opt_para.size() - 1; j >= 0; --j) {
        if (variables[j] == 0) {
            variables.removeAt(j);
            m_opt_para.removeAt(j);
            m_opt_index.removeAt(j);
        }
    }*/
    d->m_locked_parameters.clear();
    for (int i = 0; i < variables.size(); ++i)
        d->m_locked_parameters << 1;
    m_parameter = variables;
    return variables;
}

void AbstractModel::clearOptParameter()
{
    m_opt_para.clear();
    m_global_index.clear();
    m_local_index.clear();
    m_opt_index.clear();
    for (int i = 0; i < d->m_enabled_local.size(); ++i)
        d->m_enabled_local[i] = 0;

    for (int i = 0; i < d->m_enabled_global.size(); ++i)
        d->m_enabled_global[i] = 0;
}

void AbstractModel::setGlobalParameter(double value, int parameter)
{
    if (GlobalParameter()->isChecked(parameter, 0))
        (*GlobalTable())[parameter] = value;
}

void AbstractModel::forceGlobalParameter(double value, int parameter)
{
    (*GlobalTable())[parameter] = value;
}

void AbstractModel::setGlobalParameter(const QPointer<DataTable> list)
{
    if (list->columnCount() != GlobalTable()->columnCount())
        return;
    for (int i = 0; i < list->columnCount(); ++i) {
        if (GlobalParameter()->isChecked(i, 0))
            (*GlobalTable())[i] = (*list)[i];
    }
}

void AbstractModel::setGlobalParameter(const QList<qreal>& list)
{
    if (list.size() != GlobalTable()->columnCount())
        return;
    for (int i = 0; i < list.size(); ++i)
        (*GlobalTable())[i] = list[i];
}

void AbstractModel::SetValue(int i, int j, qreal value)
{
    if (!ActiveSignals(j) || !DependentModel()->isChecked(j, i)) {
        m_model_error->data(j, i) = 0;
        m_model_signal->data(j, i) = DependentModel()->data(j, i);
        return;
    }
    if (std::isnan(value) || std::isinf(value)) {
        value = 0;
        m_corrupt = true;
    }
    if (Type() != 3) {
        if (!m_locked_model)
            m_model_signal->data(j, i) = value;
        m_model_error->data(j, i) = m_model_signal->data(j, i) - DependentModel()->data(j, i);
        m_sum_absolute += qAbs(m_model_signal->data(j, i) - DependentModel()->data(j, i));
        m_sum_squares += qPow(m_model_signal->data(j, i) - DependentModel()->data(j, i), 2);
        m_mean += m_model_signal->data(j, i) - DependentModel()->data(j, i);
        m_used_variables++;
    }
}

void AbstractModel::Calculate()
{
    if (!LocalTable())
        return; // make sure, that PrepareParameter() has been called from subclass
    m_corrupt = false;
    m_mean = 0;
    m_variance = 0;
    m_used_variables = 0;
    m_stderror = 0;
    m_SEy = 0;
    m_chisquared = 0;
    m_covfit = 0;
    m_sum_squares = 0;
    m_sum_absolute = 0;

    EvaluateOptions();
    CalculateVariables();

    if (isCorrupt()) {
        // qDebug() << "Something went wrong during model calculation, most probably some numeric stuff";
    }

    m_mean /= qreal(m_used_variables);

    if (m_fast)
        return;

    m_variance = CalculateVariance();
    m_stderror = qSqrt(m_variance) / qSqrt(m_used_variables);
    m_SEy = qSqrt(m_sum_squares / (m_used_variables - LocalParameterSize() - GlobalParameterSize()));
    m_chisquared = qSqrt(m_sum_squares / (m_used_variables - LocalParameterSize() - GlobalParameterSize() - 1));
    m_covfit = CalculateCovarianceFit();

    emit Recalculated();
}

qreal AbstractModel::ModelError() const
{
    qreal error = 0;
    for (int z = 0; z < SeriesCount(); ++z) {
        if (!ActiveSignals(z))
            continue;
        error += SumOfErrors(z);
    }
    return error;
}

qreal AbstractModel::CalculateVariance()
{
    qreal v = 0;
    int count = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        for (int j = 0; j < SeriesCount(); ++j) {
            if (!ActiveSignals(j))
                continue;
            if (DependentModel()->isChecked(j, i)) {
                v += qPow(m_model_error->data(j, i) - m_mean, 2);
                count++;
            }
        }
    }
    return v / (count - 1);
}
/*
qreal AbstractModel::CalculateVarianceData()
{
    qreal v = 0;
    int count = 0;
    for(int i = 0; i < DataPoints(); ++i)
    {
        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(DependentModel()->isChecked(j,i))
            {
                v += m_model_signal->data(j,i);
                count++;
            }
        }
    }
    double mean = v/double(v);
    for(int i = 0; i < DataPoints(); ++i)
    {
        for(int j = 0; j < SeriesCount(); ++j)
        {
            if(DependentModel()->isChecked(j,i))
            {
                v += qPow(m_model_signal->data(j,i) - mean, 2);
                count++;
            }
        }
    }
    return v/(count -1 );
}*/

qreal AbstractModel::CalculateCovarianceFit()
{
    qreal model = 0, data = 0;
    int count = 0;
    qreal cov_data = 0, cov_model = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        for (int j = 0; j < SeriesCount(); ++j) {
            if (!ActiveSignals(j))
                continue;

            if (DependentModel()->isChecked(j, i)) {
                model += m_model_signal->data(j, i);
                data += DependentModel()->data(j, i);
                count++;
            }
        }
    }
    double mean_model = model / double(count);
    double mean_data = data / double(count);
    for (int i = 0; i < DataPoints(); ++i) {
        for (int j = 0; j < SeriesCount(); ++j) {
            if (!ActiveSignals(j))
                continue;
            if (DependentModel()->isChecked(j, i)) {
                cov_data += qPow(m_model_signal->data(j, i) - mean_model, 2);
                cov_model += qPow(DependentModel()->data(j, i) - mean_data, 2);
            }
        }
    }
    return cov_model / cov_data;
}

QList<double> AbstractModel::getCalculatedModel()
{
    QList<double> x;
    for (int j = 0; j < SeriesCount(); ++j) {
        if (!ActiveSignals(j))
            continue;
        for (int i = 0; i < DataPoints(); ++i)
            if (DependentModel()->isChecked(j, i))
                x.append(ModelTable()->data(j, i));
    }
    return x;
}

qreal AbstractModel::SumOfErrors(int i) const
{
    qreal sum = 0;

    if (!ActiveSignals(i) || i >= Size())
        return sum;

    for (int j = 0; j < DataPoints(); ++j) {
        sum += qPow(m_model_error->data(i, j), 2);
    }
    return sum;
}

qreal AbstractModel::finv(qreal p)
{
    /*
     * Lets cache the f-value, that if nothing changes, no integration is needed
     */
    if (!(p == m_last_p && m_last_parameter == Parameter() && m_last_freedom == Points() - Parameter())) {
        m_f_value = ToolSet::finv(p, Parameter(), Points() - Parameter());
        m_last_p = p;
        m_last_parameter = Parameter();
        m_last_freedom = Points() - Parameter();
    }
    return m_f_value;
}

qreal AbstractModel::Error(qreal confidence, bool f)
{
    if (f) {
        qreal f_value = finv(confidence / 100);
        return SumofSquares() * (f_value * Parameter() / (Points() - Parameter()) + 1);
    } else {
        return SumofSquares() + SumofSquares() * confidence / double(100);
    }
}

void AbstractModel::SetSingleParameter(double value, int parameter)
{
    if (parameter < m_opt_para.size()) {
        *m_opt_para[parameter] = value;
    }
}

void AbstractModel::setParameter(const QVector<qreal>& parameter)
{
    if (parameter.size() != m_opt_para.size())
        return;
    for (int i = 0; i < parameter.size(); ++i)
        if (d->m_locked_parameters[i])
            *m_opt_para[i] = parameter[i];
}

qreal AbstractModel::LocalParameter(int parameter, int series) const
{
    if (series >= LocalTable()->rowCount() || parameter >= LocalTable()->columnCount())
        return 0;
    else
        return LocalTable()->data(parameter, series);
}

QVector<qreal> AbstractModel::getLocalParameterColumn(int parameter) const
{
    QVector<qreal> column;
    if (parameter >= LocalTable()->columnCount())
        return column;
    else {
        for (int i = 0; i < LocalTable()->rowCount(); ++i)
            column << LocalTable()->data(parameter, i);
    }
    return column;
}

void AbstractModel::setLocalParameter(qreal value, const QPair<int, int>& pair)
{
    if (LocalTable()->isChecked(pair.first, pair.second))
        LocalTable()->data(pair.first, pair.second) = value;
}

void AbstractModel::setLocalParameter(qreal value, int parameter, int series)
{
    if (LocalTable()->isChecked(parameter, series))
        LocalTable()->data(parameter, series) = value;
}

void AbstractModel::forceLocalParameter(qreal value, const QPair<int, int>& pair)
{
    LocalTable()->data(pair.first, pair.second) = value;
}

void AbstractModel::forceLocalParameter(qreal value, int parameter, int series)
{
    LocalTable()->data(parameter, series) = value;
}

qreal AbstractModel::LocalParameter(const QPair<int, int>& pair) const
{
    return LocalTable()->data(pair.first, pair.second);
}

void AbstractModel::setLocalParameterColumn(const QVector<qreal>& vector, int parameter)
{
    if (parameter < LocalTable()->columnCount())
        LocalTable()->setColumn(vector, parameter);
}

void AbstractModel::setLocalParameterColumn(const Vector& vector, int parameter)
{
    if (parameter < LocalTable()->columnCount())
        LocalTable()->setColumn(vector, parameter);
}

void AbstractModel::setLocalParameterSeries(const QVector<qreal>& vector, int series)
{
    if (series < LocalTable()->rowCount())
        LocalTable()->setRow(vector, series);
}

void AbstractModel::setLocalParameterSeries(const Vector& vector, int series)
{
    if (series < LocalTable()->rowCount())
        LocalTable()->setRow(vector, series);
}

void AbstractModel::addGlobalParameter()
{
    for (int i = 0; i < GlobalTable()->columnCount(); ++i) {
        /* We enable this parameter, since it is used in model calculation, but */
        d->m_enabled_global[i] = 1;
        /* we allow to break, if this is unchecked for optimisation*/
        if (!GlobalTable()->isChecked(i, 0))
            continue;
        m_opt_para << &(*GlobalTable())[i];
        m_global_index << QPair<int, int>(i, 0);
        m_opt_index << QPair<int, int>(i, 0);
    }
}

void AbstractModel::addGlobalParameter(int i)
{
    if (i < GlobalTable()->columnCount()) {
        d->m_enabled_global[i] = 1;
        /* see above comment */
        if (!GlobalTable()->isChecked(i, 0))
            return;
        m_opt_para << &(*GlobalTable())[i];
        m_global_index << QPair<int, int>(i, 0);
        m_opt_index << QPair<int, int>(i, 0);
    }
}

void AbstractModel::addLocalParameter(int i)
{
    for (int j = 0; j < LocalTable()->rowCount(); ++j) {
        if (!ActiveSignals(j))
            continue;
        if (!LocalTable()->isChecked(i, j))
            continue;
        m_opt_para << &LocalTable()->data(i, j);
        m_local_index << QPair<int, int>(i, j);
        m_opt_index << QPair<int, int>(i, 1);
    }
    d->m_enabled_local[i] = 1;
}

int AbstractModel::UpdateStatistic(const QJsonObject& object)
{
    int index;
    QJsonObject controller = object["controller"].toObject();
    switch (controller["method"].toInt()) {
    case SupraFit::Statistic::WeakenedGridSearch:
        m_wg_statistics << object;
        index = m_wg_statistics.lastIndexOf(object);
        break;

    case SupraFit::Statistic::ModelComparison:
        m_moco_statistics << object;
        index = m_moco_statistics.lastIndexOf(object);
        break;

    case SupraFit::Statistic::FastConfidence:
        m_fast_confidence = object;
        index = 0;
        break;

    case SupraFit::Statistic::Reduction:
        m_reduction = object;
        index = 0;
        break;

    case SupraFit::Statistic::GlobalSearch:
        m_search_results << object;
        index = m_search_results.lastIndexOf(object);
        break;

    case SupraFit::Statistic::MonteCarlo:
    case SupraFit::Statistic::CrossValidation:
        bool duplicate = false;
        for (int i = 0; i < m_mc_statistics.size(); ++i) {
            QJsonObject control = m_mc_statistics[i]["controller"].toObject();
            if (controller == control) {
                duplicate = true;
                m_mc_statistics[i] = object;
            }
        }
        if (!duplicate)
            m_mc_statistics << object;
        index = m_mc_statistics.lastIndexOf(object);
        break;
    }
    emit StatisticChanged();
    return index;
}

QJsonObject AbstractModel::getStatistic(SupraFit::Statistic type, int index)
{
    switch (type) {
    case SupraFit::Statistic::WeakenedGridSearch:
        if (index < m_wg_statistics.size())
            return m_wg_statistics[index];
        break;

    case SupraFit::Statistic::ModelComparison:
        if (index < m_moco_statistics.size())
            return m_moco_statistics[index];
        break;

    case SupraFit::Statistic::FastConfidence:
        return m_fast_confidence;
        break;

    case SupraFit::Statistic::Reduction:
        return m_reduction;
        break;

    case SupraFit::Statistic::MonteCarlo:
    case SupraFit::Statistic::CrossValidation:
        if (index < m_mc_statistics.size())
            return m_mc_statistics[index];
        break;

    case SupraFit::Statistic::GlobalSearch:
        if (index < m_search_results.size())
            return m_search_results[index];
        break;
    }
    return QJsonObject();
}

bool AbstractModel::RemoveStatistic(SupraFit::Statistic type, int index)
{
    switch (type) {
    case SupraFit::Statistic::WeakenedGridSearch:
        if (index < m_wg_statistics.size())
            m_wg_statistics.takeAt(index);
        else
            return false;
        break;

    case SupraFit::Statistic::ModelComparison:
        if (index < m_moco_statistics.size())
            m_moco_statistics.takeAt(index);
        else
            return false;
        break;

    case SupraFit::Statistic::FastConfidence:
        m_fast_confidence = QJsonObject();
        break;

    case SupraFit::Statistic::Reduction:
        m_reduction = QJsonObject();
        break;

    case SupraFit::Statistic::MonteCarlo:
    case SupraFit::Statistic::CrossValidation:
        if (index < m_mc_statistics.size())
            m_mc_statistics.takeAt(index);
        else
            return false;
        break;

    case SupraFit::Statistic::GlobalSearch:
        if (index < m_search_results.size())
            m_search_results.takeAt(index);
        else
            return false;
        break;

    default:
        return false;
    }
    return true;
}

QString AbstractModel::Data2Text() const
{
    QString text;
    text += "#### Begin of Data Description ####\n";
    text += "Concentrations :   " + QString::number(DataPoints()) + "\n";
    for (int i = 0; i < IndependentModel()->columnCount(); ++i)
        text += " " + IndependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += IndependentModel()->ExportAsString();
    text += "\n";
    text += "Signals :          " + QString::number(SeriesCount()) + "\n";
    for (int i = 0; i < DependentModel()->columnCount(); ++i)
        text += " " + DependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += DependentModel()->ExportAsString();
    text += "\n";
    text += Data2Text_Private();
    text += "#### End of Data Description #####\n";
    text += "******************************************************************************************************\n";
    return text;
}

QString AbstractModel::Model2Text() const
{
    QString text;
    text += "\n";
    text += "******************************************************************************************************\n";
    text += "#### Current Model Results #####\n";
    text += "Global parameter for model:\n";
    for (int i = 0; i < GlobalParameterSize(); ++i)
        text += GlobalParameterName(i) + "\t\t: " + formatedGlobalParameter(GlobalParameter(i), i) + "\n";
    if (SupportSeries()) {
        text += "Local parameter for model\n";
        text += LocalTable()->ExportAsString();
    } else {
        for (int i = 0; i < LocalParameterSize(); ++i)
            text += LocalParameterName(i) + "\t\t: " + QString::number(LocalParameter(i, 0)) + "\n";
    }
    text += "\n";
    text += Model2Text_Private();
    text += "\n";
    text += ModelTable()->ExportAsString();
    text += "\n";
    text += "Errors obtained from currrent calculcation:\n";
    for (int i = 0; i < DependentModel()->columnCount(); ++i)
        text += " " + DependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += ErrorTable()->ExportAsString();
    text += "\n";
    text += "## Current Model Results Done ####\n";
    return text;
}

QString AbstractModel::Global2Text() const
{
    QString text;
    text += "\n";
    text += "******************************************************************************************************\n";
    text += "#### Current Model Results #####\n";
    text += "Equilibrium Model Calculation with complexation constants:\n";
    for (int i = 0; i < GlobalParameterSize(); ++i)
        text += " " + GlobalParameterName(i) + ":\t" + QString::number(GlobalParameter(i)) + "\n";
    for (int i = 0; i < IndependentModel()->columnCount(); ++i)
        text += " " + IndependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";

    text += "## Current Model Results Done ####\n";
    return text;
}

QString AbstractModel::Local2Text() const
{
    QString text;
    text += "\n";
    text += "******************************************************************************************************\n";
    text += "#### Current Model Results #####\n";
    if (SupportSeries()) {
        text += "Local parameter for model\n";
        text += LocalTable()->ExportAsString();
    } else {
        for (int i = 0; i < LocalParameterSize(); ++i)
            text += LocalParameterName(i) + "\t\t: " + QString::number(LocalParameter(i, 0)) + "\n";
    }
    return text;
}

QJsonObject AbstractModel::ExportModel(bool statistics, bool locked)
{
    QJsonObject json, toplevel;
    QJsonObject optionObject;

    json["globalParameter"] = GlobalTable()->ExportTable(true, d->m_enabled_global);

    if (statistics) {
        QJsonObject statisticObject;

        for (int i = 0; i < m_mc_statistics.size(); ++i) {
            statisticObject[QString::number(SupraFit::Statistic::MonteCarlo) + ":" + QString::number(i)] = m_mc_statistics[i];
        }

        for (int i = 0; i < m_moco_statistics.size(); ++i) {
            statisticObject[QString::number(SupraFit::Statistic::ModelComparison) + ":" + QString::number(i)] = m_moco_statistics[i];
        }

        for (int i = 0; i < m_wg_statistics.size(); ++i) {
            statisticObject[QString::number(SupraFit::Statistic::WeakenedGridSearch) + ":" + QString::number(i)] = m_wg_statistics[i];
        }

        for (int i = 0; i < m_search_results.size(); ++i) {
            statisticObject[QString::number(SupraFit::Statistic::GlobalSearch) + ":" + QString::number(i)] = m_search_results[i];
        }

        statisticObject[QString::number(SupraFit::Statistic::Reduction)] = m_reduction;
        statisticObject[QString::number(SupraFit::Statistic::FastConfidence)] = m_fast_confidence;
        json["statistics"] = statisticObject;
    }

    json["localParameter"] = LocalTable()->ExportTable(true, d->m_enabled_local);

    json["locked"] = ToolSet::IntVec2String(d->m_locked_parameters.toVector());
    for (int index : getAllOptions())
        optionObject[QString::number(index)] = getOption(index);

    QJsonObject resultObject;
    if (m_locked_model || locked) {
        for (int i = 0; i < DataPoints(); ++i) {
            resultObject[QString::number(i)] = ToolSet::DoubleList2String(ModelTable()->Row(i));
        }
    }
    json["active_series"] = ToolSet::IntVec2String(m_active_signals.toVector());

    toplevel["data"] = json;
    toplevel["options"] = optionObject;
    toplevel["model"] = SFModel();
    toplevel["SupraFit"] = qint_version;
    toplevel["sum_of_squares"] = m_sum_squares;
    toplevel["sum_of_absolute"] = m_sum_absolute;
    toplevel["mean_error"] = m_mean;
    toplevel["variance"] = m_variance;
    toplevel["standard_error"] = m_stderror;
    toplevel["converged"] = m_converged;
    if (m_locked_model || locked) {
#ifdef _DEBUG
//         qDebug() << "Writing calculated data to json file";
#endif
        toplevel["locked_model"] = true;
        toplevel["result"] = resultObject;
    }
    return toplevel;
}

void AbstractModel::DebugOptions() const
{
    for (const int i : getAllOptions()) {
        qDebug() << getOptionName(i) << getOption(i);
    }
}

QVector<qreal> AbstractModel::AllParameter() const
{
    QVector<qreal> parameter = GlobalTable()->toList();

    for (int r = 0; r < LocalTable()->rowCount(); ++r) {
        for (int c = 0; c < LocalTable()->columnCount(); ++c) {
            parameter << LocalTable()->data(c, r);
        }
    }
    return parameter;
}

bool AbstractModel::ImportModel(const QJsonObject& topjson, bool override)
{
#ifdef _DEBUG
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
#endif
    if (topjson[Name()].isNull()) {
        qWarning() << "file doesn't contain any " + Name();
        return false;
    }
    int fileversion = topjson["SupraFit"].toInt();
    if ((SupraFit::Model)topjson["model"].toInt() != SFModel()) {
        if (fileversion >= qint_version) {
            qWarning() << "No old data, but models dont fit, sorry";
            return false;
        }
        qWarning() << "Models don't fit! But that seems to be ok, because it is an old SupraFit file.";
    }
    QJsonObject json = topjson["data"].toObject();

    QList<int> active_signals;
    QList<qreal> constants;
    QJsonObject globalParameter, optionObject;

    if (fileversion < 1602) {
        if (json.contains("globalParameter"))
            globalParameter = json["globalParameter"].toObject();
        else if (json.contains("constants"))
            globalParameter = json["constants"].toObject();
        else {
            qWarning() << "No global parameter found!";
        }
        for (int i = 0; i < GlobalParameterSize(); ++i) {
            (*GlobalTable())[i] = globalParameter[QString::number(i)].toString().toDouble();
        }
    } else
        GlobalTable()->ImportTable(json["globalParameter"].toObject());

    optionObject = topjson["options"].toObject();
    for (int index : getAllOptions())
        setOption(index, topjson["options"].toObject()[QString::number(index)].toString());

    QStringList keys = json["statistics"].toObject().keys();
    QJsonObject statisticObject = json["statistics"].toObject();
    if (keys.size() > 9) {
        QCollator collator;
        collator.setNumericMode(true);
        std::sort(
            keys.begin(),
            keys.end(),
            [&collator](const QString& key1, const QString& key2) {
                return collator.compare(key1, key2) < 0;
            });
    }

    if (override) {
        m_mc_statistics.clear();
        if (fileversion >= 1600) {
            m_moco_statistics.clear();
            m_wg_statistics.clear();
        }
    }

    if (fileversion < 1600) {
        m_moco_statistics << statisticObject[QString::number(SupraFit::Statistic::ModelComparison)].toObject();
        m_wg_statistics << statisticObject[QString::number(SupraFit::Statistic::WeakenedGridSearch)].toObject();
    }

    m_reduction = statisticObject[QString::number(SupraFit::Statistic::Reduction)].toObject();
    m_fast_confidence = statisticObject[QString::number(SupraFit::Statistic::FastConfidence)].toObject();

    for (const QString& str : qAsConst(keys)) {
        if (str.contains(QString::number(SupraFit::Statistic::MonteCarlo) + ":"))
            m_mc_statistics << statisticObject[str].toObject();
        else if (str.contains(QString::number(SupraFit::Statistic::ModelComparison) + ":"))
            m_moco_statistics << statisticObject[str].toObject();
        else if (str.contains(QString::number(SupraFit::Statistic::WeakenedGridSearch) + ":"))
            m_wg_statistics << statisticObject[str].toObject();
        else if (str.contains(QString::number(SupraFit::Statistic::GlobalSearch) + ":"))
            m_search_results << statisticObject[str].toObject();
    }

    if (fileversion >= 1601) {
        d->m_locked_parameters = ToolSet::String2IntVec(json["locked"].toString()).toList();
    }
    if (fileversion < 1601) {
        if (json.contains("localParameter")) {
            /*
         * Here goes (legacy) SupraFit 2 data handling
        */
            if (LocalParameterSize()) {
                QJsonObject localParameter = json["localParameter"].toObject();
                for (int i = 0; i < SeriesCount(); ++i) {
                    QVector<qreal> localVector;
                    if (!localParameter[QString::number(i)].isNull()) {
                        localVector = ToolSet::String2DoubleVec(localParameter[QString::number(i)].toString());
                        active_signals << 1;
                    } else {
                        localVector = QVector<qreal>(LocalParameterSize(), 0);
                        active_signals << 0;
                    }
                    LocalTable()->setRow(localVector, i);
                }
            }
        } else if (json.contains("pureShift")) {
            /*
             * This is SupraFit 1 legacy data handling
             */
            for (int i = 0; i < SeriesCount(); ++i) {
                QVector<qreal> localSeries;
                QJsonObject pureShiftObject = json["pureShift"].toObject();

                localSeries << pureShiftObject[QString::number(i)].toString().toDouble();

                if (!pureShiftObject[QString::number(i)].isNull())
                    active_signals << 1;
                else
                    active_signals << 0;

                for (int j = 0; j < GlobalParameterSize(); ++j) {
                    QJsonObject object = json["shift_" + QString::number(j)].toObject();
                    localSeries << object[QString::number(i)].toString().toDouble();
                }
                LocalTable()->setRow(localSeries, i);
            }
        }

    } else {
        active_signals = ToolSet::String2IntVec(json["active_series"].toString()).toList();
        LocalTable()->ImportTable(json["localParameter"].toObject());
    }
    setActiveSignals(active_signals);

    if (topjson.contains("locked_model")) {
#ifdef _DEBUG
//         qDebug() << "Loaded calculated data from json file";
#endif
        m_locked_model = true;
        QJsonObject resultObject = topjson["result"].toObject();
        QStringList keys = resultObject.keys();

        QCollator collator;
        collator.setNumericMode(true);
        std::sort(
            keys.begin(),
            keys.end(),
            [&collator](const QString& key1, const QString& key2) {
                return collator.compare(key1, key2) < 0;
            });
        for (const QString& str : qAsConst(keys)) {
            QVector<qreal> concentrationsVector, signalVector;
            concentrationsVector = ToolSet::String2DoubleVec(resultObject[str].toString());
            int row = str.toInt();
            ModelTable()->setRow(concentrationsVector, row);
        }
    }
#ifdef _DEBUG
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "model importet within" << t1 - t0 << " msecs";
#endif

    m_sum_squares = topjson["sum_of_squares"].toInt();
    m_sum_absolute = topjson["sum_of_absolute"].toInt();
    m_mean = topjson["mean_error"].toInt();
    m_variance = topjson["variance"].toInt();
    m_stderror = topjson["standard_error"].toInt();
    m_converged = topjson["converged"].toBool();

    if (SFModel() != SupraFit::MetaModel)
        Calculate();

#ifdef _DEBUG
    quint64 t2 = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "calculation took " << t2 - t1 << " msecs";
#endif
    return true;
}

void AbstractModel::setOption(int index, const QString& value)
{
    if (!d->m_model_options.contains(index) || value.isEmpty() || value.isNull())
        return;
    d->m_model_options[index].value = value;
    OptimizeParameters();
    emit OptionChanged(index, value);
}

QString AbstractModel::RandomInput(double indep, double dep) const
{
    QVector<double> vec_indep(InputParameterSize(), indep), vec_dep(SeriesCount(), dep);
    return RandomInput(vec_indep, vec_dep);
}

QString AbstractModel::RandomInput(const QVector<double>& indep, const QVector<double>& dep) const
{
    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng(seed);

    QString input;
    QPointer<DataTable> indep_model = IndependentModel()->PrepareMC(indep, rng);
    QPointer<DataTable> dep_model = DependentModel()->PrepareMC(dep, rng);

    QStringList x = indep_model->ExportAsStringList();
    QStringList y = dep_model->ExportAsStringList();

    if (x.size() == y.size()) {
        for (int i = 0; i < x.size(); ++i)
            input += x[i].replace(",", ".") + "\t" + y[i].replace(",", ".") + "\n";
    }

    delete indep_model;
    delete dep_model;
    return input;
}

AbstractModel& AbstractModel::operator=(const AbstractModel& other)
{
    setOptimizerConfig(other.getOptimizerConfig());

    m_model_signal = other.m_model_signal;
    m_model_error = other.m_model_error;

    m_active_signals = other.m_active_signals;
    d = other.d;

    m_mc_statistics = other.m_mc_statistics;
    m_wg_statistics = other.m_wg_statistics;
    m_moco_statistics = other.m_moco_statistics;
    m_reduction = other.m_reduction;
    m_fast_confidence = other.m_fast_confidence;

    m_sum_absolute = other.m_sum_absolute;
    m_sum_squares = other.m_sum_squares;
    m_variance = other.m_variance;
    m_mean = other.m_mean;
    m_stderror = other.m_stderror;
    m_SEy = other.m_SEy;
    m_chisquared = other.m_chisquared;
    m_covfit = other.m_covfit;
    m_used_variables = other.m_used_variables;

    return *this;
}

AbstractModel* AbstractModel::operator=(const AbstractModel* other)
{
    setOptimizerConfig(other->getOptimizerConfig());

    m_model_signal = other->m_model_signal;
    m_model_error = other->m_model_error;

    m_active_signals = other->m_active_signals;
    d = other->d;

    m_mc_statistics = other->m_mc_statistics;
    m_wg_statistics = other->m_wg_statistics;
    m_moco_statistics = other->m_moco_statistics;
    m_reduction = other->m_reduction;
    m_fast_confidence = other->m_fast_confidence;

    m_sum_absolute = other->m_sum_absolute;
    m_sum_squares = other->m_sum_squares;
    m_variance = other->m_variance;
    m_mean = other->m_mean;
    m_stderror = other->m_stderror;
    m_SEy = other->m_SEy;
    m_chisquared = other->m_chisquared;
    m_covfit = other->m_covfit;
    m_used_variables = other->m_used_variables;

    return this;
}
#include "AbstractModel.moc"
