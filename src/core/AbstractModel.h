/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <Eigen/Dense>

#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QtMath>

#include "dataclass.h"
struct ModelOption {
    QStringList values;
    QString value = "unset";
    QString name;
};

struct ModelSeries {
    QString name;

    QList<QPointF> m_values;
};

/* Many things can be plotted, therefore let several series form a chart, where more than one chart can be managed by a model */
struct ModelChart {
    QVector<ModelSeries> m_series;
    QString title;
    QString x_axis;
    QString y_axis;
};

/*! \brief AbstractModelPrivate provides the shared data to have thread-safe
 * data exchange for models */
class AbstractModelPrivate : public QSharedData {

public:
    AbstractModelPrivate() {}
    AbstractModelPrivate(const AbstractModelPrivate& other)
        : QSharedData(other)
        , m_model_options(other.m_model_options)
        , m_locked_parameters(other.m_locked_parameters)
        , m_enabled_global(other.m_enabled_global)
        , m_enabled_local(other.m_enabled_local)
    {
    }

    virtual ~AbstractModelPrivate()
    {


    }
    // TODO Maybe move this to dataclass sometimes - to have different plot types already without model more consistently defined
    /* Model Option such as Cooperativity can be defined */
    QMap<int, ModelOption> m_model_options;

    /* Lock parameters temporary and prevent them from being optimised without changed the model definition
     * This is used for Grid Search, where single parameters are locked temporary */
    QList<int> m_locked_parameters;

    QVector<int> m_enabled_local, m_enabled_global;
};

class AbstractModel : public DataClass {
    Q_OBJECT

public:
    enum { PlotMode = 1024 };

    /*! \brief Constructor with the DataClass as argument
     * Constructor, which should be used when a model is assigned to a dataclass
     * The parent dataclass "takes care" of this model
     *
     * The model tree in the main window collects all models, that are constructed via this way
     */
    AbstractModel(DataClass* data);

    /*! \brief Constructor with the class type as argument
     * Constructor, which is be used when a model should not be assigned to a dataclass
     * The parent dataclass doesn't know about this model
     *
     * If only the above constructor is implemented, the project and model tree view for example would not work correctly
     * When both are implemented, the correct constructor will be choosen (at least with gcc) */
    AbstractModel(AbstractModel* model);

    /*! \brief Destructor
     *  virtual destructor should be all around */
    virtual ~AbstractModel() override;

    /*! \brief Define the identification of that specific model
     *  Reimplement this function and provide the correspondig enum in global.h */
    virtual SupraFit::Model SFModel() const override = 0;

    /*! \brief set the OptimizationType to type and returns the Parameters
     * 
     */
    QVector<qreal> OptimizeParameters();

    /*! \brief returns the Parameters used in optimisation
     *
     */
    inline QVector<qreal> OptimizeParameters() const { return m_parameter; }

    /*! \brief returns a pair of int
     *  first - index of parameter
     *  second define if global or local
     */
    virtual inline QPair<int, int> IndexParameters(int i) const { return m_opt_index[i]; }

    /*! \brief Locks Parameter not to be optimised during Levenberg-Marquadt
     */
    inline void setLockedParameter(const QList<int>& lock) { private_d->m_locked_parameters = lock; }

    /*! \brief Clear the list of to be optimised Parameters
     */
    void clearOptParameter();

    /*! \brief returns the locked Parameters
     */
    inline QList<int> LockedParameters() const { return private_d->m_locked_parameters; }

    /*
     * function to create a new instance of the model, this way was quite easier than
     * a copy constructor
     */
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) = 0;

    /*! \brief Export model to json file
     * 
     */
    virtual QJsonObject ExportModel(bool statistics = true, bool locked = false);

    /*! \brief Import model from json
     * 
     */
    virtual bool ImportModel(const QJsonObject& topjson, bool override = true);

    /*! \brief Import model from legacy json
     *
     */
    virtual bool LegacyImportModel(const QJsonObject& topjson, bool override = true);

    /*! \brief Returns the name of the model
     */
    inline QString Name() const { return m_name; }

    /*! \brief Overrides the name of the model
     */
    inline void setName(const QString& name)
    {
        m_name = name;
        emit ModelNameChanged(name);
    }

    /*! \brief set the calculation style to bool fast
     * some useless loops will be omitted in AbstractModel::Calculation call
     */
    inline void setFast(bool fast = true) { m_fast = fast; }

    /*! \brief set the calculation style to bool fast
     * some useless loops will be omitted in AbstractModel::Calculation call
     * Variance, CovFit etc
     */
    inline void CalculateStatistics(bool statistics = true) { m_statistics = statistics; }

    /*! \brief get the Name of the ith GlobalParameter
     */
    inline virtual QString GlobalParameterName(int i = 0) const
    {
        Q_UNUSED(i)
        return QString();
    }

    /*! \brief Prefix of the ith global parameter 
     */
    virtual inline QString GlobalParameterPrefix(int i = 0) const
    {
        Q_UNUSED(i)
        return QString();
    }

    /*! \brief Suffix of the ith  global parameter 
     */
    virtual inline QString GlobalParameterSuffix(int i = 0) const
    {
        Q_UNUSED(i)
        return QString();
    }

    /*! \brief Description of the ith global parameter 
     */
    virtual inline QString GlobalParameterDescription(int i = 0) const
    {
        Q_UNUSED(i)
        return QString();
    }

    /*! \brief get the Name of the ith LocalParameter
     */
    inline virtual QString LocalParameterName(int i = 0) const
    {
        Q_UNUSED(i)
        return QString();
    }

    /*! \brief Prefix of theith  local parameter 
     */
    virtual inline QString LocalParameterPrefix(int i = 0) const
    {
        Q_UNUSED(i)
        return QString();
    }

    /*! \brief Suffix of the ith local parameter 
     */
    virtual inline QString LocalParameterSuffix(int i = 0) const
    {
        Q_UNUSED(i)
        return QString();
    }

    /*! \brief Description of the ith local parameter 
     */
    virtual inline QString LocalParameterDescription(int i = 0) const
    {
        Q_UNUSED(i)
        return QString();
    }

    /*! \brief Return a formated value as string of the global parameter with the value
     */
    virtual QString formatedGlobalParameter(qreal value, int globalParamater = 0) const
    {
        Q_UNUSED(globalParamater)
        return QString::number(value);
    }

    /*! \brief Return a formated value as string of the global parameter with the value
     */
    virtual QString formatedLocalParameter(qreal value, int localParamater = 0) const
    {
        Q_UNUSED(localParamater)
        return QString::number(value);
    }

    virtual inline int Color(int i) const { return i; }

    /*! \brief returns Akaike’s Information Criterion (AIC)
    */
    inline qreal AIC() const { return DataPoints() * log(SSE() / double(DataPoints())) + 2 * (Parameter() + 1); }

    /*! \brief returns a second-order (corrected) Akaike’s Information Criterion (AIC)
    */
    inline qreal AICc() const
    {
        int K = Parameter() + 1;
        return AIC() + (2 * K * (K + 1)) / double(DataPoints() - K - 1);
    }

    inline virtual void ReleaseLocks() {}

    virtual void SetSingleParameter(double value, int parameter);

    void setGlobalParameter(double value, int parameter);

    void addGlobalParameter();
    void addGlobalParameter(int i);
    void addLocalParameter(int i);

    void setOptions(const QJsonObject& options);

    int UpdateStatistic(const QJsonObject& object);

    int getReductionStatisticResults() const { return m_reduction.size(); }

    QJsonObject getFastConfidence() const { return m_fast_confidence; }

    int getMoCoStatisticResult() const { return m_moco_statistics.size(); }

    int getWGStatisticResult() const { return m_wg_statistics.size(); }

    int getMCStatisticResult() const { return m_mc_statistics.size(); }

    int getCVStatisticResult() const { return m_cv_statistics.size(); }

    /*! \brief Load statistic defined by type
     * If more than results can be stored, define index
     */
    QJsonObject getStatistic(SupraFit::Method type, int index = 0) const;

    /*! \brief Export the current model and data inclusive the defined statistic */
    QJsonObject ExportStatistic(SupraFit::Method type, int index = 0);

    /*! \brief Removes raw data from result */
    bool DropRawData(SupraFit::Method type, int index = 0);

    /*! \brief Remove result */
    bool RemoveStatistic(SupraFit::Method type, int index = 0);

    virtual QList<qreal> getCalculatedModel();

    /*! \brief returns a List of all Series, that are to be included in optimisation
     */
    inline QList<int> ActiveSignals() const { return m_active_signals; }

    /*! \brief return if the series i is active in optimisation
     */
    inline int ActiveSignals(int i) const
    {
        if (i < m_active_signals.size())
            return m_active_signals.at(i);
        else
            return 1;
    }

    /*! \brief set the series active / inactive according to the given list
     * 0 - inactive
     * 1 - active
     */
    inline void setActiveSignals(const QList<int>& active_signals)
    {
        m_active_signals = active_signals;
        emit ActiveSignalsChanged(m_active_signals);
    }

    /*! \brief Set the current active parameter (which are internally reference through a pointer)
     * to the given vector
     */
    void setParameter(const QVector<qreal>& parameter);

    /*! \brief Returns if the current model parameter makes dont't sense
     * true - no sense
     * false - all fine
     */
    inline bool isCorrupt() const { return m_corrupt; }

    inline qreal SSE() const { return m_sum_squares; }
    inline qreal SAE() const { return m_sum_absolute; }
    inline int Points() const { return m_used_variables; }
    inline int Parameter() const { return m_opt_para.size(); }
    inline qreal MeanError() const { return m_mean; }
    inline qreal Variance() const { return m_variance; }

    inline qreal StdDeviation() const { return qSqrt(m_variance); }
    inline qreal sigma() const { return qSqrt(m_variance); }

    inline qreal StdError() const { return m_stderror; }
    inline qreal SEy() const { return m_SEy; }
    inline qreal SEy(int i) const { return qSqrt(SumOfErrors(i) / (double(DataPoints() - GlobalParameterSize() - LocalParameterSize(i)))); }

    inline qreal ChiSquared() const { return m_chisquared; }
    inline qreal CovFit() const { return m_covfit; }

    inline QVector<qreal> StatisticVector() const { return QVector<qreal>() << SSE() << SEy() << ChiSquared() << sigma(); }

    inline bool isConverged() const { return m_converged; }
    virtual inline void setConverged(bool converged) { m_converged = converged; }
    /*! \brief Returns the f value for the given p value
     *  Degrees of freedom and number of parameters are taken in account
     */
    qreal finv(qreal p);
    qreal Error(qreal confidence, bool f = true);

    /*! \brief Demand initial guess
     * An initial guess will be demanded, if it fails, the guess will be automatically calculated
     * when all requirements are met
     */
    inline void InitialGuess()
    {
        if (Type() == DataClassPrivate::DataType::Simulation)
            return;

        m_demand_guess = true;
        InitialGuess_Private();
    }

    /*! \brief Here goes the model implementation for the initial guess
     */

    virtual void InitialGuess_Private() = 0;

    /*! \brief Returns pointer to Model DataTable
     * overloaded function
     */
    inline DataTable* ModelTable() { return m_model_signal; }

    /*! \brief Returns pointer to Error DataTable
     * overloaded function
     */
    inline DataTable* ErrorTable() { return m_model_error; }

    /*! \brief Returns const pointer to Model DataTable
     * overloaded function
     */
    inline DataTable* ModelTable() const { return m_model_signal; }

    /*! \brief Returns const pointer to Error DataTable
     * overloaded function
     */
    inline DataTable* ErrorTable() const { return m_model_error; }

    /*! \brief Returns pointer to Global DataTable
     * overloaded function
     */
    inline DataTable* GlobalTable()
    {
        return m_global_parameter;
    }

    /*! \brief Returns pointer to Global DataTable
     * overloaded function
     */
    inline DataTable* GlobalTable() const { return m_global_parameter; }

    /*! \brief Returns const pointer to Local DataTable
     * overloaded function
     */
    inline DataTable* LocalTable()
    {
        return m_local_parameter;
    }

    /*! \brief Returns const pointer to Local DataTable
     * overloaded function
     */
    inline DataTable* LocalTable() const { return m_local_parameter; }

    inline QJsonObject getOptimizerConfig() const { return m_opt_config; }
    inline void setOptimizerConfig(const QJsonObject& config)
    {
        m_opt_config = config;
    }

    /*
     * definies wheater this model can be calculate in parallel
     * should be useful when the model observables are calculated numerically
     */
    virtual bool SupportThreads() const = 0;

    virtual qreal SumOfErrors(int i) const;

    virtual qreal ModelError() const;

    /*! \brief set the values of the global parameter to const QPointer< DataTable > list
     */
    virtual void setGlobalParameter(const QPointer<DataTable> list);

    /*! \brief set the values of the global parameter to const QList< qreal > &list
     */
    virtual void setGlobalParameter(const QList<qreal>& list);

    /*! \brief set the values of the global parameter to const QList< qreal > &list
     */
    virtual void forceGlobalParameter(double value, int parameter);

    /*! \brief return the list of global parameter values, overloaded function
     */
    virtual inline QPointer<DataTable> GlobalParameter() const { return GlobalTable(); }
    /*! \brief return i global parameter
     */
    virtual inline qreal GlobalParameter(int i) const { return (*GlobalTable())[i]; }
    /*! \brief returns size of global parameter
     */
    virtual int GlobalParameterSize() const = 0;
    /*! \brief return size of input parameter 
     */
    virtual int InputParameterSize() const = 0;

    /*! \brief returns size of local parameter
     */
    virtual int LocalParameterSize(int series = 0) const = 0;

    virtual qreal LocalParameter(int parameter, int series) const;

    QVector<qreal> getLocalParameterColumn(int parameter) const;
    qreal LocalParameter(const QPair<int, int>& pair) const;

    void setLocalParameter(qreal value, int parameter, int series);
    void setLocalParameter(qreal value, const QPair<int, int>& pair);

    virtual void forceLocalParameter(qreal value, const QPair<int, int>& pair);
    virtual void forceLocalParameter(qreal value, int parameter, int series);

    void setLocalParameterSeries(const QVector<qreal>& vector, int series);
    void setLocalParameterSeries(const Vector& vector, int series);

    void setLocalParameterColumn(const QVector<qreal>& vector, int parameter);
    void setLocalParameterColumn(const Vector& vector, int parameter);

    /*! \brief return text of calculated model
     */
    QString Model2Text() const;

    QString Global2Text() const;

    QString Local2Text() const;

    /*! \brief reimplment, if more model specfic model information should be printed out
     */
    virtual QString Model2Text_Private() const { return QString(); }

    AbstractModel& operator=(const AbstractModel& other);
    AbstractModel* operator=(const AbstractModel* other);

    inline QVector<QPair<int, int>> GlobalIndex() const { return m_global_index; }

    inline QVector<QPair<int, int>> LocalIndex() const { return m_local_index; }

    inline void addOption(int index, const QString& name, const QStringList& values)
    {
        if (private_d->m_model_options.contains(index))
            return;
        ModelOption option;
        option.values = values;
        option.value = values.first();
        option.name = name;
        private_d->m_model_options[index] = option;
    }

    void setOption(int index, const QString& value);

    inline QString getOption(int index) const
    {
        if (!private_d->m_model_options.contains(index))
            return QString("unset");
        ModelOption option = private_d->m_model_options[index];
        return option.value;
    }

    inline QString getOptionName(int index) const
    {
        if (!private_d->m_model_options.contains(index))
            return QString("unset");
        ModelOption option = private_d->m_model_options[index];
        return option.name;
    }

    inline int getOptionIndex(int index) const
    {
        if (!private_d->m_model_options.contains(index))
            return -1;
        ModelOption option = private_d->m_model_options[index];
        QStringList values = option.values;
        return values.indexOf(option.value);
    }

    void DebugOptions() const;

    inline QList<int> getAllOptions() const { return private_d->m_model_options.keys(); }

    inline const QStringList getSingleOptionValues(int index) const
    {
        if (private_d->m_model_options.contains(index))
            return private_d->m_model_options[index].values;
        else
            return QStringList();
    }

    inline virtual void DeclareOptions() {}

    inline virtual void DeclareSystemParameter() {}

    inline virtual void EvaluateOptions() {}

    /*! \brief Implement ModelInfo() const for output, that is printed after each recalculation
     * (or optimisation) - If there are demanding task, they should NOT go here */
    inline virtual QString ModelInfo() const { return QString(); }

    /*! \brief Implement AdditionalOutput() const for more optional calculation done on top of
     * the current model parameter, demaninding task (if any) should go here.
     * The output will be printed out in an extra dialog in the gui */
    virtual QString AdditionalOutput() const { return QString(); }

    inline bool isLocked() const { return m_locked_model; }

    inline virtual bool LocalEnabled(int i) const { return private_d->m_enabled_local[i]; }

    inline virtual bool GlobalEnabled(int i) const { return private_d->m_enabled_global[i]; }

    virtual bool SupportSeries() const = 0;

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const override { return QString(); }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const override { return QString(); }

    virtual inline int MaxParameter() { return GlobalParameterSize() + LocalParameterSize(); }

    virtual QVector<qreal> AllParameter() const;

    inline QVector<int> LocalEnabled() const { return private_d->m_enabled_local; }

    inline void RemoveOption(int key) { private_d->m_model_options.remove(key); }

    inline void addSearchResult(const QJsonObject& search) { m_search_results << search; }

    inline int SearchSize() const { return m_search_results.size(); }

    inline QJsonObject Search(int i) { return m_search_results[i]; }

    QString RandomInput(double indep, double dep) const;

    virtual QString RandomInput(const QVector<double>& indep, const QVector<double>& dep) const;

    inline virtual QString RandomExportSuffix() const { return QString("*.dat (*.dat)"); }

    /*! \brief Calculate standard type statistics for stored statistic results */
    virtual QString AnalyseStatistic(bool forceAll = false) const;

    /*! \brief Calculate standard type statistics for stored statistic results */
    virtual QString AnalyseStatistic(const QJsonObject& object, bool forceAll = false) const;

    /*! \brief Calculate standard type of monte carlo statistics */
    virtual QString AnalyseMonteCarlo(const QJsonObject& object, bool forceAll = false) const;

    /*! \brief Calculate standard type of model comparison statistics */
    virtual QString AnalyseModelComparison(const QJsonObject& object, bool forceAll = false) const;

    /*! \brief Calculate standard type of grid search statistics */
    virtual QString AnalyseGridSearch(const QJsonObject& object, bool forceAll = false) const;

    /*! \brief Calculate standard type of grid search statistics */
    virtual QVector<QJsonObject> PostGridSearch(QList<QJsonObject>& models) const { Q_UNUSED(models)
        return QVector<QJsonObject>(); }

    virtual QVector<qreal> DeCompose(int datapoint, int series = 0) const { Q_UNUSED(datapoint)
        Q_UNUSED(series) return QVector<qreal>(); }

    inline virtual int ChildrenSize() const override { return 0; }

    inline QString ModelUUID() const { return m_model_uuid; }

    inline const ModelChart* Chart(const QString& str) const
    {
        if (m_model_charts.contains(str))
            return m_model_charts[str];
        else
            return NULL;
    }

    inline QStringList Charts() const { return m_model_charts.keys(); }

    qreal ErrorfTestThreshold(qreal pvalue);

    inline void setDescription(const QString& str) { m_desc = str; }

    inline QString Description() const { return m_desc; }

    /*! \brief Define the cut off for reduction analysis 
     * Cut offs are model specific parameters, that have the be determined seperatly */
    inline virtual double ReductionCutOff() const { return -1; }

    void clearStatistic();

public slots:
    /*! \brief Calculated the current model with all previously set and defined parameters
     */
    void Calculate();
    virtual inline void UpdateParameter() {}

    virtual inline void UpdateOption(int index, const QString& str) { Q_UNUSED(index)
        Q_UNUSED(str) }

private:
    QSharedDataPointer<AbstractModelPrivate> private_d;

    void ParseFastConfidence(const QJsonObject& object);

protected:
    /*
     * This function handles the optimization flags, which get passed by
     * @param OptimizationType type
     * and returns the selected current CalculateVariables
     * @return QVector<qreal>
     * 
     */
    virtual void OptimizeParameters_Private();

    /* 
     * @param int i, int j and qreal value
     * of the model value - DataTable 
     * returns true if value was used
     * returns false if value was not intended for usage (excluded by series, datatable etc)
     */
    bool SetValue(int i, int j, qreal value);

    /*! \brief This function defines how the model values are to be calculated
     */
    virtual void CalculateVariables() = 0;

    /*! \brief Calculated the variance of the estimated model variables
     */
    virtual qreal CalculateVariance();

    /*! \brief Calculated the variance of the raw data
     */
    virtual qreal CalculateCovarianceFit() const;

    void clearChart(const QString& hash);

    void addPoints(const QString& hash, qreal x, const Vector& y, const QStringList& names = QStringList());

    void addSeries(const QString& hash, const QString& name, const QList<QPointF>& points, const QString& x_label = "x", const QString& y_label = "y");

    void UpdateChart(const QString& hash, const QString& x_label, const QString& y_label);

    void PrepareParameter(int global, int local);

    // #warning to do as well
    //FIXME more must be
    QVector<double*> m_opt_para;
    QVector<QPair<int, int>> m_local_index, m_global_index, m_opt_index;
    QVector<qreal> m_parameter, m_variance_series, m_mean_series;
    QVector<int> m_used_series;

    QList<QJsonObject> m_mc_statistics;
    QList<QJsonObject> m_cv_statistics;
    QList<QJsonObject> m_wg_statistics;
    QList<QJsonObject> m_moco_statistics;
    QList<QJsonObject> m_search_results;
    QList<QJsonObject> m_reduction;

    QJsonObject m_fast_confidence;

    qreal m_sum_absolute, m_sum_squares, m_variance, m_mean, m_stderror, m_SEy, m_chisquared, m_covfit;
    int m_used_variables;
    QList<int> m_active_signals;
    qreal m_last_p, m_f_value;
    int m_last_parameter, m_last_freedom;
    bool m_corrupt, m_converged, m_locked_model, m_fast, m_statistics = true, m_guess_failed = true, m_demand_guess = false;
    QJsonObject m_opt_config;
    QPointer<DataTable> m_model_signal, m_model_error;
    QPointer<DataTable> m_local_parameter, m_global_parameter;

    QString m_more_info, m_name, m_name_cached, m_model_uuid, m_desc;

    /* Let models store additional charts, where anything can be plotted, e.g concentration curves or anything else
      BUT since abstractmodels should not depend on anything graphics related, they will be no series, but simple structs
      that can be worked with */
    QMap<QString, ModelChart*> m_model_charts;
signals:
    /*
     * Signal is emitted whenever void Calculate() is finished
     */
    void Recalculated();
    void StatisticChanged();
    void OptionChanged(int index, const QString& value);
    void ChartUpdated(const QString& chart);
    void ModelNameChanged(const QString& name);
};
