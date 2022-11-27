/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "dataclass.h"

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
#include <QtCore/QPointF>
#include <QtCore/QRandomGenerator>
#include <QtCore/QUuid>

#include <iostream>

#include "AbstractModel.h"

AbstractModel::AbstractModel(DataClass* data)
    : DataClass(data)
    , m_last_p(1)
    , m_f_value(1)
    , m_last_parameter(0)
    , m_last_freedom(0)
    , m_corrupt(false)
    , m_converged(false)
    , m_locked_model(false)
    , m_fast(true)
{
    m_opt_config = OptimConfigBlock;
    connect(this, &DataClass::SystemParameterChanged, this, &AbstractModel::UpdateParameter);

    connect(this, &AbstractModel::OptionChanged, this, &AbstractModel::UpdateOption);
    private_d = new AbstractModelPrivate;
    setActiveSignals(QVector<int>(SeriesCount(), 1).toList());

    m_model_signal = new DataTable(DataPoints(), SeriesCount(), this);
    m_model_error = new DataTable(DataPoints(), SeriesCount(), this);

    connect(this, &DataClass::Update, this, [this]() {
        m_model_signal->clear(SeriesCount(), DataPoints());
        m_model_error->clear(SeriesCount(), DataPoints());
    });

    connect(this, &DataClass::SystemParameterChanged, this, &AbstractModel::Calculate);
    connect(this, &DataClass::Update, this, &AbstractModel::Calculate);

    // connect(this->Info(), &DataClassPrivateObject::Update, this, &AbstractModel::Calculate);

    /* This function call as to be only in this constructor */
    AddChildren(this);

    QUuid uuid;
    m_model_uuid = uuid.createUuid().toString();
    setPlotMode(true);
}

AbstractModel::AbstractModel(DataClass* data, const QJsonObject& model)
    : DataClass(data)
    , m_last_p(1)
    , m_f_value(1)
    , m_last_parameter(0)
    , m_last_freedom(0)
    , m_corrupt(false)
    , m_converged(false)
    , m_locked_model(false)
    , m_fast(true)
{
    m_model_definition = model;

    DefineModel(model);

    m_opt_config = OptimConfigBlock;
    connect(this, &DataClass::SystemParameterChanged, this, &AbstractModel::UpdateParameter);

    connect(this, &AbstractModel::OptionChanged, this, &AbstractModel::UpdateOption);
    private_d = new AbstractModelPrivate;
    setActiveSignals(QVector<int>(SeriesCount(), 1).toList());

    m_model_signal = new DataTable(DataPoints(), SeriesCount(), this);
    m_model_error = new DataTable(DataPoints(), SeriesCount(), this);

    connect(this, &DataClass::Update, this, [this]() {
        m_model_signal->clear(SeriesCount(), DataPoints());
        m_model_error->clear(SeriesCount(), DataPoints());
    });

    connect(this, &DataClass::SystemParameterChanged, this, &AbstractModel::Calculate);
    connect(this, &DataClass::Update, this, &AbstractModel::Calculate);

    // connect(this->Info(), &DataClassPrivateObject::Update, this, &AbstractModel::Calculate);

    /* This function call as to be only in this constructor */
    AddChildren(this);

    QUuid uuid;
    m_model_uuid = uuid.createUuid().toString();
    setPlotMode(true);
}

AbstractModel::AbstractModel(AbstractModel* model)
    : DataClass(model)
    , private_d(new AbstractModelPrivate(*model->private_d))
    , m_last_p(model->m_last_p)
    , m_f_value(model->m_f_value)
    , m_last_parameter(model->m_last_parameter)
    , m_last_freedom(model->m_last_freedom)
    , m_corrupt(model->m_corrupt)
    , m_converged(model->m_converged)
    , m_locked_model(model->m_locked_model)
    , m_fast(true)
    , m_defined_model(model->m_defined_model)
    , m_name_cached(model->Name()) // m_model_definition(model->m_model_definition)
{
    m_opt_config = OptimConfigBlock;
    connect(this, &DataClass::SystemParameterChanged, this, &AbstractModel::UpdateParameter);
    connect(this, &AbstractModel::OptionChanged, this, &AbstractModel::UpdateOption);

    setActiveSignals(QVector<int>(SeriesCount(), 1).toList());

    m_model_signal = new DataTable(DataPoints(), SeriesCount(), this);
    m_model_error = new DataTable(DataPoints(), SeriesCount(), this);

    connect(this, &DataClass::Update, this, [this]() {
        m_model_signal->clear(SeriesCount(), DataPoints());
        m_model_error->clear(SeriesCount(), DataPoints());
    });

    connect(this, &DataClass::SystemParameterChanged, this, &AbstractModel::Calculate);
    connect(this, &DataClass::Update, this, &AbstractModel::Calculate);

    // connect(this->Info(), &DataClassPrivateObject::Update, this, &AbstractModel::Calculate);

    QUuid uuid;
    m_model_uuid = uuid.createUuid().toString();
    setPlotMode(true);
}
/*
void AbstractModel::DefineModel(const QJsonObject &model)
{
    QJsonObject parse = model;
    if (parse.contains("ScriptModel"))
        parse = parse["ScriptModel"].toObject();
    if (model.contains("GlobalParameterSize"))
        m_global_parameter_size = parse["GlobalParameterSize"].toInt();
    else
        return;

    if (model.contains("GlobalParameterNames"))
        m_global_parameter_names = parse["GlobalParameterNames"].toString().split("|");

    if (model.contains("LocalParameterSize"))
        m_local_parameter_size = parse["LocalParameterSize"].toInt();
    else
        return;

    if (model.contains("LocalParameterNames"))
        m_local_parameter_names = parse["LocalParameterNames"].toString().split("|");

    if (model.contains("InputSize"))
        m_input_size = parse["InputSize"].toInt();
    else
        return;

    if (model.contains("ChaiScript")) {
      // m_execute_chai.clear();
      QStringList strings;
      QJsonObject exec = parse["ChaiScript"].toObject();
      for (const QString &key : exec.keys())
        // for (int i = 0; i < exec.size(); ++i)
        strings << exec[key].toString();
      m_execute = strings.join("\n");
      //m_python = false;
      //m_chai = true;
    }

    m_name_cached = model["Name"].toString();
    m_name = model["Name"].toString();

    m_input_names = model["InputNames"].toString().split("|");
    m_depmodel_names = model["DepModelNames"].toString().split("|");

    m_model_definition = model;
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());

    for (int i = 0; i < m_input_names.size(); ++i)
        IndependentModel()->setHeaderData(i, Qt::Horizontal, m_input_names[i], Qt::DisplayRole);

    for (int i = 0; i < m_depmodel_names.size(); ++i)
        DependentModel()->setHeaderData(i, Qt::Horizontal, m_depmodel_names[i], Qt::DisplayRole);
}
*/

void AbstractModel::PrepareParameter(int global, int local)
{
    if (SFModel() != SupraFit::MetaModel) {
        if (InputParameterSize() != IndependentModel()->columnCount())
            throw - 2;
    }

    m_global_boundaries = QList<ParameterBoundary>(global);
    QList<ParameterBoundary> local_boundaries(SeriesCount());
    m_local_boundaries = QList<QList<ParameterBoundary>>(local, local_boundaries);
    QStringList header;

    if (!LocalTable())
        m_local_parameter = new DataTable(SeriesCount(), local, this);
    for (int i = 0; i < LocalParameterSize(); ++i)
        header << LocalParameterName(i);
    LocalTable()->setHeader(header);

    header = QStringList();

    if (!GlobalTable())
        m_global_parameter = new DataTable(1, global, this);
    for (int i = 0; i < GlobalParameterSize(); ++i)
        header << GlobalParameterName(i);
    GlobalTable()->setHeader(header);

    private_d->m_enabled_global = QVector<int>(global, 0);
    private_d->m_enabled_local = QVector<int>(local, 0);

    while (m_random_global.size() < GlobalParameterSize())
        m_random_global << QPair<qreal, qreal>(0, 10);

    while (m_random_local.size() < SeriesCount()) {
        QVector<QPair<qreal, qreal>> random;
        while (random.size() < LocalParameterSize())
            random << QPair<qreal, qreal>(0, 10);
        m_random_local << random;
    }

    addGlobalParameter();
    DeclareSystemParameter();
    DeclareOptions();
    LoadSystemParameter();
    UpdateParameter();
    if (m_name_cached.isEmpty())
        m_name = Model2Name(SFModel());
    else
        m_name = m_name_cached;
    m_complete = true;
}

AbstractModel::~AbstractModel()
{
    emit Deleted();

    if (m_model_signal)
        delete m_model_signal;

    if (m_model_error)
        delete m_model_error;

    qDeleteAll(m_model_charts);

#ifdef _DEBUG
    std::cout << "Model to be deleted" << std::endl;
#endif
}

void AbstractModel::InitialiseRandom()
{
    if (!qApp->instance()->property("InitialiseRandom").toBool() || SFModel() == SupraFit::MetaModel)
        return;

    for (int i = 0; i < m_random_global.size(); ++i) {
        (*GlobalTable())[i] = QRandomGenerator::global()->generateDouble() * (m_random_global[i].second - m_random_global[i].first) + m_random_global[i].first;
    }
    for (int series = 0; series < SeriesCount(); ++series) {
        for (int i = 0; i < LocalParameterSize(); ++i) {
            LocalTable()->data(series, i) = QRandomGenerator::global()->generateDouble() * (m_random_local[series][i].second - m_random_local[series][i].first) + m_random_local[series][i].first;
        }
    }
    Calculate();
    emit Recalculated();
}

QVector<qreal> AbstractModel::OptimizeParameters()
{
    clearOptParameter();

    OptimizeParameters_Private();

    QVector<qreal> variables;
    for (int i = 0; i < m_opt_para.size(); ++i)
        variables << *m_opt_para[i];

    /* I really hope, this is old und already obsolete stuff, can remember right now */
    /*qDebug() << m_opt_para;
    for (int j = m_opt_para.size() - 1; j >= 0; --j) {
        if (variables[j] == 0) {
            variables.removeAt(j);
            m_opt_para.removeAt(j);
            m_opt_index.removeAt(j);
        }
    }*/
    private_d->m_locked_parameters.clear();
    for (int i = 0; i < variables.size(); ++i)
        private_d->m_locked_parameters << 1;
    m_parameter = variables;
    return variables;
}

void AbstractModel::OptimizeParameters_Private()
{
    for (int i = 0; i < GlobalParameterSize(); ++i)
        addGlobalParameter(i);

    for (int i = 0; i < LocalParameterSize(); ++i)
        addLocalParameter(i);
}

void AbstractModel::clearOptParameter()
{
    m_opt_para.clear();
    m_global_index.clear();
    m_local_index.clear();
    m_opt_index.clear();
    for (int i = 0; i < private_d->m_enabled_local.size(); ++i)
        private_d->m_enabled_local[i] = 0;

    for (int i = 0; i < private_d->m_enabled_global.size(); ++i)
        private_d->m_enabled_global[i] = 0;
}

void AbstractModel::setGlobalParameter(double value, int parameter)
{
    if (GlobalParameter()->isChecked(0, parameter))
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
        if (GlobalParameter()->isChecked(0, i))
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

bool AbstractModel::SetValue(int i, int j, qreal value)
{
    if (!ActiveSignals(j) || !DependentModel()->isChecked(i, j)) {
        m_model_error->data(i, j) = 0;
        m_model_signal->data(i, j) = DependentModel()->data(i, j);
        return false;
    }
    bool return_value = true;
    if (std::isnan(value) || std::isinf(value)) {
        value = 0;
        m_corrupt = true;
        return_value = false;
    }
    QVector<qreal> mean_series(SeriesCount(), 0);
    QVector<int> used_series(SeriesCount(), 0);
    //if (Type() != 3) {
    if (!m_locked_model)
        m_model_signal->data(i, j) = value;
    m_model_error->data(i, j) = m_model_signal->data(i, j) - DependentModel()->data(i, j);
    m_sum_absolute += qAbs(m_model_signal->data(i, j) - DependentModel()->data(i, j));
    m_sum_squares += qPow(m_model_signal->data(i, j) - DependentModel()->data(i, j), 2);
    // m_mean += qAbs(m_model_signal->data(i, j) - DependentModel()->data(i, j));
    m_mean += m_model_signal->data(i, j) - DependentModel()->data(i, j);

    mean_series[j] += qAbs(m_model_signal->data(i, j) - DependentModel()->data(i, j));
    used_series[j]++;
    m_used_variables++;
    //}
    m_used_series = used_series;
    m_mean_series = mean_series;
    return return_value;
}

void AbstractModel::Calculate()
{
    if (!LocalTable() || !m_complete || (DataBegin() == DataEnd() && SFModel() != SupraFit::MetaModel))
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
    m_squared = 0;

    for (const QString& str : Charts())
        clearChart(str);

    //    DependentModel()->Debug("AbstractModel::Calculate");

    EvaluateOptions();
    CalculateVariables();

    if (!m_complete) {
        return;
    }

    if (isCorrupt()) {
        // qDebug() << "Something went wrong during model calculation, most probably some numeric stuff";
    }

    m_mean /= qreal(m_used_variables);

    if (!m_statistics)
        return;

    for (int i = 0; i < m_used_series.size(); ++i)
        m_mean_series[i] /= double(m_used_series[i]);

    double degree_freedom = m_used_variables - Parameter();
    m_variance = CalculateVariance();
    m_stderror = qSqrt(m_variance) / qSqrt(m_used_variables);
    m_SEy = qSqrt(m_sum_squares / degree_freedom);
    m_chisquared = qSqrt(m_sum_squares / (degree_freedom - 1));

    if (SFModel() != SupraFit::MetaModel) {
        QVector<qreal> x, y;
        for (int i = 0; i < DataPoints(); ++i)
            for (int j = 0; j < SeriesCount(); ++j) {
                if (DependentModel()->isChecked(i, j)) {
                    x << DependentModel()->data(i, j);
                    y << ModelTable()->data(i, j);
                }
            }
        PeakPick::LinearRegression regression = PeakPick::LeastSquares(ToolSet::QVector2DoubleEigVec(x), ToolSet::QVector2DoubleEigVec(y));
        m_squared = regression.R;
    }
    //FIXME sometimes ...
    m_covfit = 0; //CalculateCovarianceFit();

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
    QVector<qreal> variance(SeriesCount(), 0);
    for (int i = 0; i < DataPoints(); ++i) {
        for (int j = 0; j < SeriesCount(); ++j) {
            if (!ActiveSignals(j))
                continue;
            if (DependentModel()->isChecked(i, j)) {
                v += qPow(m_model_error->data(i, j) - m_mean, 2);
                variance[j] += qPow(m_model_error->data(i, j) - m_mean_series[j], 2);
                count++;
            }
        }
    }
    for (int i = 0; i < variance.size(); ++i)
        variance[i] /= qSqrt(double(m_used_series[i] - 1));
    m_variance_series = variance;
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

qreal AbstractModel::CalculateCovarianceFit() const
{
    qreal model = 0, data = 0;
    int count = 0;
    qreal cov_data = 0, cov_model = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        for (int j = 0; j < SeriesCount(); ++j) {
            if (!ActiveSignals(j))
                continue;

            if (DependentModel()->isChecked(i, j)) {
                model += m_model_signal->data(i, j);
                data += DependentModel()->data(i, j);
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
            if (DependentModel()->isChecked(i, j)) {
                cov_data += qPow(m_model_signal->data(i, j) - mean_model, 2);
                cov_model += qPow(DependentModel()->data(i, j) - mean_data, 2);
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
            if (DependentModel()->isChecked(i, j))
                x.append(ModelTable()->data(i, j));
    }
    return x;
}

qreal AbstractModel::SumOfErrors(int i) const
{
    qreal sum = 0;

    if (!ActiveSignals(i) || i >= Size())
        return sum;

    for (int j = 0; j < DataPoints(); ++j) {
        sum += qPow(m_model_error->data(j, i), 2);
    }
    return sum;
}

qreal AbstractModel::finv(qreal p)
{
    /*
     * Lets cache the f-value, that if nothing changes, no integration is needed
     */
    if (!(qFuzzyCompare(p, m_last_p) && m_last_parameter == Parameter() && m_last_freedom == Points() - Parameter())) {
        m_f_value = ToolSet::finv(p, Parameter(), Points() - Parameter());
        m_last_p = p;
        m_last_parameter = Parameter();
        m_last_freedom = Points() - Parameter();
    }
    return m_f_value;
}

qreal AbstractModel::Error(qreal confidence)
{
    qreal f_value = finv(confidence / 100);
    return SSE() * (f_value * Parameter() / (Points() - Parameter()) + 1);
}

void AbstractModel::SetSingleParameter(double value, int parameter)
{
    if (parameter < m_opt_para.size()) {
        *m_opt_para[parameter] = value;
    }
    // qDebug() << this <<  m_opt_para;
}

void AbstractModel::setParameter(const QVector<qreal>& parameter)
{
    if (parameter.size() != m_opt_para.size())
        return;
    for (int i = 0; i < parameter.size(); ++i)
        if (private_d->m_locked_parameters[i])
            *m_opt_para[i] = parameter[i];
}

qreal AbstractModel::LocalParameter(int parameter, int series) const
{
    if (series >= LocalTable()->rowCount() || parameter >= LocalTable()->columnCount())
        return 0;
    else
        return LocalTable()->data(series, parameter);
}

QVector<qreal> AbstractModel::getLocalParameterColumn(int parameter) const
{
    QVector<qreal> column;
    if (parameter >= LocalTable()->columnCount())
        return column;
    else {
        for (int i = 0; i < LocalTable()->rowCount(); ++i)
            column << LocalTable()->data(i, parameter);
    }
    return column;
}

void AbstractModel::setLocalParameter(qreal value, const QPair<int, int>& pair)
{
    if (LocalTable()->isChecked(pair.second, pair.first))
        LocalTable()->data(pair.second, pair.first) = value;
}

void AbstractModel::setLocalParameter(qreal value, int parameter, int series)
{
    if (LocalTable()->isChecked(series, parameter))
        LocalTable()->data(series, parameter) = value;
}

void AbstractModel::forceLocalParameter(qreal value, const QPair<int, int>& pair)
{
    LocalTable()->data(pair.second, pair.first) = value;
}

void AbstractModel::forceLocalParameter(qreal value, int parameter, int series)
{
    LocalTable()->data(series, parameter) = value;
}

qreal AbstractModel::LocalParameter(const QPair<int, int>& pair) const
{
    return LocalTable()->data(pair.second, pair.first);
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
        private_d->m_enabled_global[i] = 1;
        /* we allow to break, if this is unchecked for optimisation*/
        if (!GlobalTable()->isChecked(0, i))
            continue;
        m_opt_para << &(*GlobalTable())[i];
        m_global_index << QPair<int, int>(i, 0);
        m_opt_index << QPair<int, int>(i, 0);
    }
}

void AbstractModel::addGlobalParameter(int i)
{
    if (i < GlobalTable()->columnCount()) {
        private_d->m_enabled_global[i] = 1;
        /* see above comment */
        if (!GlobalTable()->isChecked(0, i))
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
        if (!LocalTable()->isChecked(j, i))
            continue;
        m_opt_para << &LocalTable()->data(j, i);
        m_local_index << QPair<int, int>(i, j);
        m_opt_index << QPair<int, int>(i, 1);
    }
    private_d->m_enabled_local[i] = 1;
}

int AbstractModel::UpdateStatistic(const QJsonObject& object)
{
    int index;
    int match = 0;
    bool duplicate = false;

    QJsonObject controller = object["controller"].toObject();
    switch (AccessCI(controller, "Method").toInt()) {
    case SupraFit::Method::WeakenedGridSearch:

        duplicate = false;
        for (int i = 0; i < m_wg_statistics.size(); ++i) {
            QJsonObject control = m_wg_statistics[i]["controller"].toObject();
            if (qFuzzyCompare(controller["timestamp"].toDouble(), control["timestamp"].toDouble())) {
                duplicate = true;
                m_wg_statistics[i] = object;
            }
        }
        if (!duplicate)
            m_wg_statistics << object;
        index = m_wg_statistics.lastIndexOf(object);
        break;

    case SupraFit::Method::ModelComparison:

        duplicate = false;
        for (int i = 0; i < m_moco_statistics.size(); ++i) {
            QJsonObject control = m_moco_statistics[i]["controller"].toObject();
            if (qFuzzyCompare(controller["timestamp"].toDouble(), control["timestamp"].toDouble())) {
                duplicate = true;
                m_moco_statistics[i] = object;
            }
        }
        if (!duplicate)
            m_moco_statistics << object;
        index = m_moco_statistics.lastIndexOf(object);

        break;

    case SupraFit::Method::FastConfidence:
        m_fast_confidence = object;
        ParseFastConfidence(object);
        index = 0;
        break;

    case SupraFit::Method::Reduction:
        index = match;
        for (int i = 0; i < m_reduction.size(); ++i) {
            int RunType = m_reduction[i]["controller"].toObject()["ReductionRuntype"].toInt();
            if (RunType == object["controller"].toObject()["ReductionRuntype"].toInt()) {
                m_reduction[i] = object;
                index = i;
                match++;
            }
        }

        if (match == 0)
            m_reduction << object;

        break;

    case SupraFit::Method::GlobalSearch:

        duplicate = false;
        for (int i = 0; i < m_search_results.size(); ++i) {
            QJsonObject control = m_search_results[i]["controller"].toObject();
            if (qFuzzyCompare(controller["timestamp"].toDouble(), control["timestamp"].toDouble())) {
                duplicate = true;
                m_search_results[i] = object;
            }
        }
        if (!duplicate)
            m_search_results << object;
        index = m_search_results.lastIndexOf(object);

        break;

    case SupraFit::Method::MonteCarlo:

        duplicate = false;
        for (int i = 0; i < m_mc_statistics.size(); ++i) {
            QJsonObject control = m_mc_statistics[i]["controller"].toObject();
            if (qFuzzyCompare(controller["timestamp"].toDouble(), control["timestamp"].toDouble())) {
                duplicate = true;
                m_mc_statistics[i] = object;
            }
        }
        if (!duplicate)
            m_mc_statistics << object;
        index = m_mc_statistics.lastIndexOf(object);
        break;

    case SupraFit::Method::CrossValidation:
        duplicate = false;
        for (int i = 0; i < m_cv_statistics.size(); ++i) {
            QJsonObject control = m_cv_statistics[i]["controller"].toObject();

            if (qFuzzyCompare(controller["timestamp"].toDouble(), control["timestamp"].toDouble())) {
                duplicate = true;
                m_cv_statistics[i] = object;
            }
        }
        if (!duplicate)
            m_cv_statistics << object;
        index = m_cv_statistics.lastIndexOf(object);
        break;
    }
    emit StatisticChanged();
    return index;
}

QJsonObject AbstractModel::getStatistic(SupraFit::Method type, int index) const
{
    switch (type) {
    case SupraFit::Method::WeakenedGridSearch:
        if (index < m_wg_statistics.size())
            return m_wg_statistics[index];
        break;

    case SupraFit::Method::ModelComparison:
        if (index < m_moco_statistics.size())
            return m_moco_statistics[index];
        break;

    case SupraFit::Method::FastConfidence:
        return m_fast_confidence;
        break;

    case SupraFit::Method::Reduction:
        if (index < m_reduction.size())
            return m_reduction[index];
        break;

    case SupraFit::Method::MonteCarlo:
        if (index < m_mc_statistics.size())
            return m_mc_statistics[index];
        break;

    case SupraFit::Method::CrossValidation:
        if (index < m_cv_statistics.size())
            return m_cv_statistics[index];
        break;

    case SupraFit::Method::GlobalSearch:
        if (index < m_search_results.size())
            return m_search_results[index];
        break;
    }
    return QJsonObject();
}

bool AbstractModel::RemoveStatistic(SupraFit::Method type, int index)
{
    switch (type) {
    case SupraFit::Method::WeakenedGridSearch:
        if (index < m_wg_statistics.size())
            m_wg_statistics.takeAt(index);
        else
            return false;
        break;

    case SupraFit::Method::ModelComparison:
        if (index < m_moco_statistics.size())
            m_moco_statistics.takeAt(index);
        else
            return false;
        break;

    case SupraFit::Method::FastConfidence:
        m_fast_confidence = QJsonObject();
        break;

    case SupraFit::Method::Reduction:
        if (index < m_reduction.size())
            m_reduction.takeAt(index);
        break;

    case SupraFit::Method::MonteCarlo:
        if (index < m_mc_statistics.size())
            m_mc_statistics.takeAt(index);
        else
            return false;
        break;
    case SupraFit::Method::CrossValidation:
        if (index < m_cv_statistics.size())
            m_cv_statistics.takeAt(index);
        else
            return false;
        break;

    case SupraFit::Method::GlobalSearch:
        if (index < m_search_results.size())
            m_search_results.takeAt(index);
        else
            return false;
        break;
    }
    return true;
}

void AbstractModel::ParseFastConfidence(const QJsonObject& data)
{
    const QString str = "Simplified Model Comparison";
    if (m_model_charts.keys().contains(str))
        m_model_charts[str]->m_series.clear();
    for (int i = 0; i < data.keys().size() - 1; ++i) {
        QJsonObject block = data[QString::number(i)].toObject();
        QString name = block["name"].toString();

        QList<QPointF> points = ToolSet::String2PointsList(block["points"].toString());
        addSeries(str, name, points, "parameter value", "Sum of Squares (SSE)");
    }
    emit ChartUpdated(str);
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
    text += "Errors obtained from currrent calculation:\n";
    for (int i = 0; i < DependentModel()->columnCount(); ++i)
        text += " " + DependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += ErrorTable()->ExportAsString();
    text += "\n\n";
    text += AdditionalOutput();
    text += "###############################################################################################\n\n";
    text += AnalyseStatistic();
    text += "\n\n###############################################################################################";
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
    text += AdditionalOutput() + "\n";
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

    json["globalParameter"] = GlobalTable()->ExportTable(true, private_d->m_enabled_global);
    QJsonObject statisticObject;
    QString help = "Please consider to \n(1) - Save the entry to a seperate file (via right click) and \n(2 - a) Remove the corresponding entry from the Results List or\n(2 - b) Drop the raw data for this result!";
    if (statistics) {
        int counter = 0;

        QJsonValueRef ref = statisticObject[QString::number(counter)] = m_fast_confidence;
        if (ref.isUndefined()) {
            qWarning() << "Critical warning, statistic data are to large to be stored in file";
            emit Info()->Warning(QString("Critical warning, statistic data are to large to be stored in file. Attempted to write %1 in model %2 from data %3. %4").arg(SupraFit::Method::FastConfidence).arg(Name()).arg(ProjectTitle()).arg(help));
            statisticObject.remove(QString::number(counter));
        }

        counter++;

        auto SaveStatistic = [this, help](QJsonObject& statisticObject, const QList<QJsonObject>& data, int& counter) {
            for (int i = 0; i < data.size(); ++i) {
                QJsonValueRef ref = statisticObject[QString::number(counter)] = data[i];
                if (ref.isNull()) {
                    emit Info()->Warning(QString("Critical warning, statistic data are to large to be stored in file. Attempted to write %2 # %1 in %3 from %4. %5").arg(i + 1).arg(SupraFit::Method2Name(AccessCI(data[i]["controller"].toObject(), "Method").toInt())).arg(Name()).arg(ProjectTitle()).arg(help));
                    statisticObject.remove(QString::number(counter));
                }
                counter++;
            }
        };

        SaveStatistic(statisticObject, m_mc_statistics, counter);
        SaveStatistic(statisticObject, m_cv_statistics, counter);
        SaveStatistic(statisticObject, m_moco_statistics, counter);
        SaveStatistic(statisticObject, m_wg_statistics, counter);
        SaveStatistic(statisticObject, m_wg_statistics, counter);
        SaveStatistic(statisticObject, m_search_results, counter);
        SaveStatistic(statisticObject, m_reduction, counter);

        json["methods"] = statisticObject;
    }

    QJsonObject globalBoundaries;
    for (int i = 0; i < m_global_boundaries.size(); ++i) {
        globalBoundaries[QString::number(i)] = ToolSet::DoubleVec2String(Boundary2Vector((m_global_boundaries[i])));
    }
    json["globalBoundaries"] = globalBoundaries;

    json["localParameter"] = LocalTable()->ExportTable(true, private_d->m_enabled_local);

    QJsonObject localBoundaries;
    for (int j = 0; j < m_local_parameter->columnCount() && j < m_local_boundaries.size(); ++j) {
        for (int i = 0; i < m_local_parameter->rowCount() && i < m_local_boundaries[j].size(); ++i) {
            localBoundaries[QString::number(i) + "+" + QString::number(j)] = ToolSet::DoubleVec2String(Boundary2Vector(m_local_boundaries[j][i]));
        }
    }
    json["localBoundaries"] = localBoundaries;

    json["locked"] = ToolSet::IntVec2String(private_d->m_locked_parameters.toVector());
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
    toplevel["SSE"] = m_sum_squares;
    toplevel["SAE"] = m_sum_absolute;
    toplevel["mean_error"] = m_mean;
    toplevel["variance"] = m_variance;
    toplevel["standard_error"] = m_stderror;
    toplevel["converged"] = m_converged;
    toplevel["valid"] = !isCorrupt();
    toplevel["name"] = m_name;
    QJsonObject definiton;
    for (const QString& key : m_defined_model.keys()) {
        definiton[key] = m_defined_model[key];
    }

    toplevel["ModelDefinition"] = definiton;
    if (m_locked_model || locked) {
#ifdef _DEBUG
//         qDebug() << "Writing calculated data to json file";
#endif
        toplevel["locked_model"] = true;
        toplevel["result"] = resultObject;
    }

    return toplevel;
}

QJsonObject AbstractModel::ExportStatistic(SupraFit::Method type, int index)
{
    QJsonObject blob = getStatistic(type, index);

    if (blob.isEmpty())
        return QJsonObject();
    QJsonObject toplevel;
    toplevel["data"] = ExportData();
    QJsonObject model = ExportModel(false, false);
    QJsonObject datablob = model["data"].toObject();
    QJsonObject statisticObject;
    statisticObject[QString::number(type) + ":0"] = blob;
    datablob["methods"] = statisticObject;
    model["data"] = datablob;
    toplevel["model_0"] = model;

    return toplevel;
}

bool AbstractModel::DropRawData(SupraFit::Method type, int index)
{
    QJsonObject blob = getStatistic(type, index);
    QJsonObject controller = blob["controller"].toObject();
    controller.remove("raw");
    blob["controller"] = controller;
    UpdateStatistic(blob);
    return true;
}

void AbstractModel::DebugOptions() const
{
    for (const int i : getAllOptions()) {
        qDebug() << getOptionName(i) << getOption(i);
    }
}

QVector<qreal> AbstractModel::AllParameter() const
{
    QVector<qreal> parameter = GlobalTable()->toVector();

    for (int row = 0; row < LocalTable()->rowCount(); ++row) {
        for (int column = 0; column < LocalTable()->columnCount(); ++column) {
            parameter << LocalTable()->data(row, column);
        }
    }
    return parameter;
}

void AbstractModel::setOptions(const QJsonObject& options)
{
    for (int index : getAllOptions())
        setOption(index, options[QString::number(index)].toString());
}

bool AbstractModel::ImportModel(const QJsonObject& topjson, bool override)
{
#ifdef _DEBUG
// quint64 t0 = QDateTime::currentMSecsSinceEpoch();
#endif

    QJsonObject json = topjson["data"].toObject();

    if (topjson.contains("ModelDefinition")) {
        QJsonObject model = topjson["ModelDefinition"].toObject();
        for (const QString& key : model.keys()) {
            m_defined_model.insert(key, model[key].toObject());
        }
        DefineModel(m_model_definition);
    }

    int copy_model = topjson["model"].toInt();
    bool unsafe_copy = qApp->instance()->property("UnsafeCopy").toBool();

    if (copy_model != SFModel()) {
        if (!unsafe_copy) {
            emit Info()->Message(QString("The two models don't fit. Unsafe copy is disabled on purpose. If you really want to do that, change the configuration in the Settings Dialog."));
            return false;
        } else {
            emit Info()->Message(QString("The two models don't fit. Unsafe copy is enabled. Good Luck. - Statistical data will be ignored."));
            override = false;
        }
    }

    QList<int> active_signals;

    GlobalTable()->ImportTable(json["globalParameter"].toObject());

    setOptions(topjson["options"].toObject());

    QStringList keys;
    QJsonObject statisticObject;

    keys = json["methods"].toObject().keys();
    statisticObject = json["methods"].toObject();

    if (override) {
        UpdateStatistic(statisticObject[QString::number(SupraFit::Method::FastConfidence)].toObject());

        if (!m_fast_confidence.isEmpty())
            ParseFastConfidence(m_fast_confidence);

        for (const QString& str : qAsConst(keys)) {
            QJsonObject object = statisticObject[str].toObject();
            QJsonObject controller = object["controller"].toObject();
            if (controller.isEmpty())
                continue;

            UpdateStatistic(object);
        }
    }
    private_d->m_locked_parameters = ToolSet::String2IntVec(json["locked"].toString()).toList();

    active_signals = ToolSet::String2IntVec(json["active_series"].toString()).toList();
    LocalTable()->ImportTable(json["localParameter"].toObject());

    if (json.contains("globalBoundaries")) {
        QJsonObject globalBoundaries = json["globalBoundaries"].toObject();
        for (int i = 0; i < m_global_boundaries.size(); ++i) {
            m_global_boundaries[i] = Vector2Boundary(ToolSet::String2DoubleVec(globalBoundaries[QString::number(i)].toString()));
        }
    }
    if (json.contains("localBoundaries")) {
        QJsonObject localBoundaries = json["localBoundaries"].toObject();
        for (int j = 0; j < m_local_parameter->columnCount() && j < m_local_boundaries.size(); ++j) {
            for (int i = 0; i < m_local_parameter->rowCount() && i < m_local_boundaries[j].size(); ++i) {
                m_local_boundaries[j][i] = Vector2Boundary(ToolSet::String2DoubleVec(localBoundaries[QString::number(i) + "+" + QString::number(j)].toString()));
            }
        }
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
            if (ModelTable()->columnCount() == concentrationsVector.size())
                ModelTable()->setRow(concentrationsVector, row);
        }
    }
#ifdef _DEBUG
    //  quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    //  qDebug() << "model imported within" << t1 - t0 << " msecs";
#endif

    m_sum_squares = topjson["SSE"].toInt();
    m_sum_absolute = topjson["SAE"].toInt();
    m_mean = topjson["mean_error"].toInt();
    m_variance = topjson["variance"].toInt();
    m_stderror = topjson["standard_error"].toInt();
    m_converged = topjson["converged"].toBool();
    // private_d->m_locked_parameters = ToolSet::String2IntVec(topjson["locked"].toString()).toList();
    if (topjson.contains("name") && !unsafe_copy)
        m_name = topjson["name"].toString();

    if (SFModel() != SupraFit::MetaModel)
        Calculate();

#ifdef _DEBUG
        // quint64 t2 = QDateTime::currentMSecsSinceEpoch();
        // qDebug() << "calculation took " << t2 - t1 << " msecs";
#endif
    return true;
}

bool AbstractModel::LegacyImportModel(const QJsonObject& topjson, bool override)
{
#ifdef _DEBUG
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
#endif
    if (topjson[Name()].isNull()) {
        emit Info()->Warning("Sorry, this file doesn't contain any " + Name() + " model.");
        qWarning() << "file doesn't contain any " + Name();
        return false;
    }
    int fileversion = topjson["SupraFit"].toInt();

    if (fileversion >= 2002 && fileversion <= qint_version)
      return ImportModel(topjson, override);

    if (static_cast<SupraFit::Model>(topjson["model"].toInt()) != SFModel()) {
        if (fileversion >= qint_version) {
            emit Info()->Warning("Sorry, I suppose I do not support this data. " + Name());
            qWarning() << "No old data, but models dont fit, sorry";
            return false;
        }
        emit Info()->Message("Models don't fit! But that seems to be ok, because it is an old SupraFit file.");
    }

    if (fileversion > qint_version) {
        emit Info()->Warning(QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2").arg(qint_version).arg(fileversion));
        qWarning() << QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2. Get the most recent version from <a href='https://github.com/conradhuebler/SupraFit'>https://github.com/conradhuebler/SupraFit</a>").arg(qint_version).arg(fileversion);
        return false;
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

    setOptions(topjson["options"].toObject());

    QStringList keys;
    QJsonObject statisticObject;

    if (fileversion < 1607) {
        keys = json["statistics"].toObject().keys();
        statisticObject = json["statistics"].toObject();
    } else {
        keys = json["methods"].toObject().keys();
        statisticObject = json["methods"].toObject();
    }
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

        if (fileversion < 1600) {
            UpdateStatistic(statisticObject[QString::number(SupraFit::Method::ModelComparison)].toObject());
            UpdateStatistic(statisticObject[QString::number(SupraFit::Method::WeakenedGridSearch)].toObject());
            // m_moco_statistics << statisticObject[QString::number(SupraFit::Method::ModelComparison)].toObject();
            m_wg_statistics << statisticObject[QString::number(SupraFit::Method::WeakenedGridSearch)].toObject();
        }
        if (fileversion < 1608) {
            UpdateStatistic(statisticObject[QString::number(SupraFit::Method::Reduction)].toObject());
            //m_reduction << statisticObject[QString::number(SupraFit::Method::Reduction)].toObject();
        }
        UpdateStatistic(statisticObject[QString::number(SupraFit::Method::FastConfidence)].toObject());
        //m_fast_confidence = statisticObject[QString::number(SupraFit::Method::FastConfidence)].toObject();

        if (!m_fast_confidence.isEmpty())
            ParseFastConfidence(m_fast_confidence);

        if (fileversion >= 1608) {
            for (const QString& str : qAsConst(keys)) {
                QJsonObject object = statisticObject[str].toObject();
                QJsonObject controller = object["controller"].toObject();
                if (controller.isEmpty())
                    continue;

                UpdateStatistic(object);
            }
        }
    }

    if (fileversion >= 1601) {
        private_d->m_locked_parameters = ToolSet::String2IntVec(json["locked"].toString()).toList();
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
            if (ModelTable()->columnCount() == concentrationsVector.size())
                ModelTable()->setRow(concentrationsVector, row);
        }
    }
#ifdef _DEBUG
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "model imported within" << t1 - t0 << " msecs";
#endif

    m_sum_squares = topjson["SSE"].toInt();
    m_sum_absolute = topjson["SAE"].toInt();
    m_mean = topjson["mean_error"].toInt();
    m_variance = topjson["variance"].toInt();
    m_stderror = topjson["standard_error"].toInt();
    m_converged = topjson["converged"].toBool();
    // private_d->m_locked_parameters = ToolSet::String2IntVec(topjson["locked"].toString()).toList();
    if (topjson.contains("name"))
        m_name = topjson["name"].toString();
    /*
        if (d->m_independent_model->columnCount() != d->m_scaling.size())
            for (int i = 0; i < d->m_independent_model->columnCount(); ++i)
                d->m_scaling << 1;
    */
    if (SFModel() != SupraFit::MetaModel)
        Calculate();

#ifdef _DEBUG
    quint64 t2 = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "calculation took " << t2 - t1 << " msecs";
#endif
    return true;
}

void AbstractModel::clearStatistic()
{
    m_mc_statistics.clear();
    m_cv_statistics.clear();
    m_wg_statistics.clear();
    m_moco_statistics.clear();
    m_search_results.clear();
    m_reduction.clear();

    m_fast_confidence = QJsonObject();
}

void AbstractModel::setOption(int index, const QString& value)
{
    if (!private_d->m_model_options.contains(index) || value.isEmpty() || value.isNull())
        return;
    private_d->m_model_options[index].value = value;
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
    QPointer<DataTable> dep_model = ModelTable()->PrepareMC(dep, rng);

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
    private_d = other.private_d;

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
    m_squared = other.m_squared;

    return *this;
}

AbstractModel* AbstractModel::operator=(const AbstractModel* other)
{
    setOptimizerConfig(other->getOptimizerConfig());

    m_model_signal = other->m_model_signal;
    m_model_error = other->m_model_error;

    m_active_signals = other->m_active_signals;
    private_d = other->private_d;

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
    m_squared = other->m_squared;

    return this;
}

QString AbstractModel::AnalyseStatistic(bool forceAll) const
{
    QString result = QString("<table>");

    for (int i = 0; i < getMCStatisticResult(); ++i) {
        result += "<tr><th>Monte Carlo</th></tr>";
        result += AnalyseMonteCarlo(getStatistic(SupraFit::MonteCarlo, i), forceAll);
    }

    for (int i = 0; i < getMoCoStatisticResult(); ++i) {
        result += "<tr><th>Model Comparison</th></tr>";
        result += AnalyseModelComparison(getStatistic(SupraFit::ModelComparison, i), forceAll);
    }

    for (int i = 0; i < getWGStatisticResult(); ++i) {
        result += "<tr><th>Grid Search</th></tr>";
        result += AnalyseGridSearch(getStatistic(SupraFit::WeakenedGridSearch, i), forceAll);
    }

    result += "</table";

    return result;
}

QString AbstractModel::AnalyseStatistic(const QJsonObject& object, bool forceAll) const
{
    QJsonObject controller = object["controller"].toObject();

    QString text;

    switch (AccessCI(controller, "Method").toInt()) {
    case SupraFit::Method::WeakenedGridSearch:
        return AnalyseGridSearch(object, forceAll);
        break;

    case SupraFit::Method::ModelComparison:
        return AnalyseModelComparison(object, forceAll);
        break;

    case SupraFit::Method::FastConfidence:
        return AnalyseModelComparison(object, forceAll);
        break;

    case SupraFit::Method::Reduction:

        for (int i = 0; i < object.count() - 1; ++i) {
            QJsonObject data = object.value(QString::number(i)).toObject();
            if (data.isEmpty())
                continue;
            text += Print::TextFromConfidence(data, object["controller"].toObject());
        }
        break;

    case SupraFit::Method::MonteCarlo:
    case SupraFit::Method::CrossValidation:

        return AnalyseMonteCarlo(object, forceAll);
        break;
    }
    return text;
}

QString AbstractModel::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{
    Q_UNUSED(forceAll)
    QString result;
    QJsonObject controller = object["controller"].toObject();
    QStringList keys = object.keys();

    for (int i = 0; i < keys.size() - 1; ++i) {
        result += Print::TextFromConfidence(object[QString::number(i)].toObject(), controller);
    }

    if (forceAll) {
        result += "<p>Ordered List of all obtained values for each parameter.</p>\n";
        for (int i = 0; i < keys.size() - 1; ++i) {
            QJsonObject data = object[QString::number(i)].toObject();
            QString const_name = data["name"].toString();
            if (data["type"].toString() == "Local Parameter") {
                if (data.contains("index")) {
                    int index = data["index"].toString().split("|")[1].toInt();
                    const_name += QString(" - Series %1").arg(index + 1);
                }
            }
            QString vector = data["data"].toObject()["raw"].toString();
            result += "-------------------------------------------------\n" + const_name + "\n" + vector + "\n-------------------------------------------------\n\n";
        }
    }
    return result;
}

QString AbstractModel::AnalyseModelComparison(const QJsonObject& object, bool forceAll) const
{
    Q_UNUSED(forceAll)

    QString result;
    QJsonObject controller = object["controller"].toObject();
    QStringList keys = object.keys();

    for (int i = 0; i < keys.size() - 1; ++i) {
        result += Print::TextFromConfidence(object[QString::number(i)].toObject(), controller);
    }

    return result;
}

QString AbstractModel::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{
    Q_UNUSED(forceAll)

    QString result;
    QJsonObject controller = object["controller"].toObject();
    QStringList keys = object.keys();

    for (int i = 0; i < keys.size() - 1; ++i) {
        result += Print::TextFromConfidence(object[QString::number(i)].toObject(), controller);
    }

    return result;
}

void AbstractModel::clearChart(const QString& str)
{
    if (m_model_charts.contains(str)) {
        for (int i = 0; i < m_model_charts[str]->m_series.size(); ++i)
            m_model_charts[str]->m_series[i].m_values.clear();
    }
}

void AbstractModel::addPoints(const QString& str, qreal x, const Vector& vector, const QStringList& names)
{
    if (!m_model_charts.contains(str)) {
        ModelChart* chart = new ModelChart;
        chart->title = QString(str);
        for (int i = 0; i < vector.size(); ++i) {
            ModelSeries series;

            if (i < names.size())
                series.name = names[i];
            chart->m_series << series;
        }
        m_model_charts[str] = chart;
    }
    if (vector.size() == m_model_charts[str]->m_series.size()) {
        for (int i = 0; i < m_model_charts[str]->m_series.size(); ++i) {
            m_model_charts[str]->m_series[i].m_values << QPointF(x, vector(i));
        }
    }
}

void AbstractModel::UpdateChart(const QString& str, const QString& x_label, const QString& y_label)
{
    if (!m_model_charts.contains(str)) {
        return;
    }

    m_model_charts[str]->x_axis = x_label;
    m_model_charts[str]->y_axis = y_label;
}

void AbstractModel::addSeries(const QString& str, const QString& name, const QList<QPointF>& points, const QString& x_label, const QString& y_label)
{
    if (!m_model_charts.contains(str)) {
        ModelChart* chart = new ModelChart;
        chart->title = QString(str);
        m_model_charts[str] = chart;
    }

    ModelSeries series;
    series.m_values = points;
    series.name = name;

    m_model_charts[str]->m_series << series;
    m_model_charts[str]->x_axis = x_label;
    m_model_charts[str]->y_axis = y_label;
}

qreal AbstractModel::ErrorfTestThreshold(qreal pvalue)
{
    qreal f_value = finv(pvalue);
    qreal error = SSE();
    return error * (f_value * Parameter() / (Points() - Parameter()) + 1);
}

void AbstractModel::UpdateModelDefiniton(const QHash<QString, QJsonObject>& model)
{
    for (const QString& key : model.keys()) {
        QJsonObject o = m_defined_model[key];
        o["value"] = model[key]["value"];
        m_defined_model[key] = o;
    }
    DefineModel(QJsonObject());
    Calculate();
}

QList<double> AbstractModel::getPenalty() const
{
    double penalty = 0.0;
    /*
    for(int i = 0; i < m_global_boundaries.size(); ++i)
    {
        ParameterBoundary global = m_global_boundaries[i];
        double value = global.lower_barrier_wall * log( 1+ exp(-global.lower_barrier_beta*(global.lower_barrier - GlobalParameter(i))));
        if(global.limit_lower && !std::isnan(value))
           penalty += value;

        value = global.upper_barrier_wall * log( 1+ exp(-global.upper_barrier_beta*(global.upper_barrier - GlobalParameter(i))));
        if(global.limit_upper && !std::isnan(value))
            penalty += value;
    }
    for(int j = 0; j < m_local_parameter->columnCount() ; ++j)
    {
        for(int i = 0; i < m_local_parameter->rowCount(); ++i)
        {
        ParameterBoundary local = m_local_boundaries[j][i];
        double value = local.lower_barrier_wall * log10( 1+ exp(-local.lower_barrier_beta*(local.lower_barrier - LocalTable()->data(i,j))));
        if(local.limit_lower && !std::isnan(value))
            penalty += value;

        value = local.upper_barrier_wall * log10( 1+ exp(-local.upper_barrier_beta*(local.upper_barrier - LocalTable()->data(i,j))));
        if(local.limit_upper && !std::isnan(value))
            penalty += value;
        }
    }
    */
    QList<double> result(m_used_variables, penalty);
    return result;
}
#include "AbstractModel.moc"
