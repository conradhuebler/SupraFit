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

    QMap<int, ModelOption> m_model_options;
    QList<int> m_locked_parameters;
    QVector<int> m_enabled_local, m_enabled_global;
};

class AbstractModel : public DataClass {
    Q_OBJECT

public:
    enum { PlotMode = 1024 };

    AbstractModel(DataClass* data);

    AbstractModel(AbstractModel* model);

    virtual ~AbstractModel();

    virtual SupraFit::Model SFModel() const = 0;

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
    inline void setLockedParameter(const QList<int>& lock) { d->m_locked_parameters = lock; }

    /*! \brief Clear the list of to be optimised Parameters
     */
    void clearOptParameter();

    /*! \brief returns the locked Parameters
     */
    inline QList<int> LockedParameters() const { return d->m_locked_parameters; }

    /*
     * function to create a new instance of the model, this way was quite easier than
     * a copy constructor
     */
    virtual QSharedPointer<AbstractModel> Clone() = 0;

    /*
     * ! \brief Export model to json file
     * 
     */
    virtual QJsonObject ExportModel(bool statistics = true, bool locked = false);

    /* ! \brief Import model from json
     * 
     */
    virtual bool ImportModel(const QJsonObject& topjson, bool override = true);

    /*! \brief Returns the name of the model
     */
    inline QString Name() const { return m_name; }

    /*! \brief Overrides the name of the model
     */
    inline void setName(const QString name) { m_name = name; }

    /*! \brief set the calculation style to bool fast
     * some useless loops will be omitted in AbstractModel::Calculation call
     * Variance, CovFit etc
     */
    inline void setFast(bool fast = true) { m_fast = fast; }

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
    inline qreal AIC() const { return DataPoints() * log(SumofSquares() / double(DataPoints())) + 2 * (Parameter() + 1); }

    /*! \brief returns a second-order (corrected) Akaike’s Information Criterion (AIC)
    */
    inline qreal AICc() const
    {
        int K = Parameter() + 1;
        return AIC() + (2 * K * (K + 1)) / double(DataPoints() - K - 1);
    }

    inline virtual void ReleaseLocks() {}

    void SetSingleParameter(double value, int parameter);

    void setGlobalParameter(double value, int parameter);

    void addGlobalParameter();
    void addGlobalParameter(int i);
    void addLocalParameter(int i);

    int UpdateStatistic(const QJsonObject& object);

    QJsonObject getReduction() const { return m_reduction; }
    QJsonObject getFastConfidence() const { return m_fast_confidence; }

    int getMoCoStatisticResult() const { return m_moco_statistics.size(); }

    int getWGStatisticResult() const { return m_wg_statistics.size(); }

    int getMCStatisticResult() const { return m_mc_statistics.size(); }

    /*! \brief Load statistic defined by type
     * If more than results can be stored, define index
     */
    QJsonObject getStatistic(SupraFit::Statistic type, int index = 0) const;

    bool RemoveStatistic(SupraFit::Statistic type, int index = 0);

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

    inline qreal SumofSquares() const { return m_sum_squares; }
    inline qreal SumofAbsolute() const { return m_sum_absolute; }
    inline int Points() const { return m_used_variables; }
    inline int Parameter() const { return m_opt_para.size(); }
    inline qreal MeanError() const { return m_mean; }
    inline qreal Variance() const { return m_variance; }
    inline qreal StdDeviation() const { return qSqrt(m_variance); }
    inline qreal StdError() const { return m_stderror; }
    inline qreal SEy() const { return m_SEy; }
    inline qreal ChiSquared() const { return m_chisquared; }
    inline qreal CovFit() const { return m_covfit; }

    inline bool isConverged() const { return m_converged; }
    virtual inline void setConverged(bool converged) { m_converged = converged; }
    /*! \brief Returns the f value for the given p value
     *  Degrees of freedom and number of parameters are taken in account
     */
    qreal finv(qreal p);
    qreal Error(qreal confidence, bool f = true);

    /*! \brief Demand initial guess
     * An initial guess will be demanded, if it fails, the guess will be automatically calculated
     * when all requirement are met
     */
    inline void InitialGuess()
    {
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

    inline OptimizerConfig getOptimizerConfig() const { return m_opt_config; }
    inline void setOptimizerConfig(const OptimizerConfig& config)
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

    /*! \brief set the values of the global parameter to const QPointer<DataTable> list
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

    /*! \brief return text of stored data
     */
    QString Data2Text() const;

    /*! \brief reimplment, if more model specfic raw data information should be printed out
     */
    virtual QString Data2Text_Private() const { return QString(); }

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
        if (d->m_model_options.contains(index))
            return;
        ModelOption option;
        option.values = values;
        option.value = values.first();
        option.name = name;
        d->m_model_options[index] = option;
    }

    void setOption(int index, const QString& value);

    inline QString getOption(int index) const
    {
        if (!d->m_model_options.contains(index))
            return QString("unset");
        ModelOption option = d->m_model_options[index];
        return option.value;
    }

    inline QString getOptionName(int index) const
    {
        if (!d->m_model_options.contains(index))
            return QString("unset");
        ModelOption option = d->m_model_options[index];
        return option.name;
    }

    inline int getOptionIndex(int index) const
    {
        if (!d->m_model_options.contains(index))
            return -1;
        ModelOption option = d->m_model_options[index];
        QStringList values = option.values;
        return values.indexOf(option.value);
    }

    void DebugOptions() const;

    inline QList<int> getAllOptions() const { return d->m_model_options.keys(); }

    inline const QStringList getSingleOptionValues(int index) const
    {
        if (d->m_model_options.contains(index))
            return d->m_model_options[index].values;
        else
            return QStringList();
    }

    inline virtual void DeclareOptions() {}

    inline virtual void DeclareSystemParameter() {}

    inline virtual void EvaluateOptions() {}

    inline virtual QString ModelInfo() const { return QString(); }

    inline bool isLocked() const { return m_locked_model; }

    inline virtual bool LocalEnabled(int i) const { return d->m_enabled_local[i]; }

    inline virtual bool GlobalEnabled(int i) const { return d->m_enabled_global[i]; }

    virtual bool SupportSeries() const = 0;

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const { return QString(); }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const { return QString(); }

    virtual inline int MaxParameter() { return GlobalParameterSize() + LocalParameterSize(); }

    virtual QVector<qreal> AllParameter() const;

    inline QVector<int> LocalEnabled() const { return d->m_enabled_local; }

    inline void RemoveOption(int key) { d->m_model_options.remove(key); }

    inline void addSearchResult(const QJsonObject& search) { m_search_results << search; }

    inline int SearchSize() const { return m_search_results.size(); }

    inline QJsonObject Search(int i) { return m_search_results[i]; }

    QString RandomInput(double indep, double dep) const;

    virtual QString RandomInput(const QVector<double>& indep, const QVector<double>& dep) const;

    inline virtual QString RandomExportSuffix() const { return QString("*.dat (*.dat)"); }

    virtual QString AdditionalOutput() const { return QString(); }

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

public slots:
    /*! \brief Calculated the current model with all previously set and defined parameters
     */
    void Calculate();
    virtual inline void UpdateParameter() {}

    virtual inline void UpdateOption(int index, const QString& str) { Q_UNUSED(index)
        Q_UNUSED(str) }

private:
    QSharedDataPointer<AbstractModelPrivate> d;

protected:
    /*
     * This function handles the optimization flags, which get passed by
     * @param OptimizationType type
     * and returns the selected current CalculateVariables
     * @return QVector<qreal>
     * 
     */
    virtual QVector<qreal> OptimizeParameters_Private() = 0;

    /* 
     * @param int i, in j and qreal value
     * of the model value - DataTable 
     */
    void SetValue(int i, int j, qreal value);

    /*! \brief This function defines how the model values are to be calculated
     */
    virtual void CalculateVariables() = 0;

    /*! \brief Calculated the variance of the estimated model variables
     */
    virtual qreal CalculateVariance();

    /*! \brief Calculated the variance of the raw data
     */
    virtual qreal CalculateCovarianceFit();

    void PrepareParameter(int global, int local);

    // #warning to do as well
    //FIXME more must be
    QVector<double*> m_opt_para;
    QVector<QPair<int, int>> m_local_index, m_global_index, m_opt_index;
    QVector<qreal> m_parameter;

    QList<QJsonObject> m_mc_statistics;
    QList<QJsonObject> m_wg_statistics;
    QList<QJsonObject> m_moco_statistics;
    QList<QJsonObject> m_search_results;

    QJsonObject m_reduction;
    QJsonObject m_fast_confidence;

    qreal m_sum_absolute, m_sum_squares, m_variance, m_mean, m_stderror, m_SEy, m_chisquared, m_covfit;
    int m_used_variables;
    QList<int> m_active_signals;
    qreal m_last_p, m_f_value;
    int m_last_parameter, m_last_freedom;
    bool m_corrupt, m_converged, m_locked_model, m_fast, m_guess_failed = true, m_demand_guess = false;
    OptimizerConfig m_opt_config;
    QPointer<DataTable> m_model_signal, m_model_error;
    QPointer<DataTable> m_local_parameter, m_global_parameter;

    QString m_more_info, m_name, m_name_cached;
signals:
    /*
     * Signal is emitted whenever void Calculate() is finished
     */
    void Recalculated();
    void Message(const QString& str, int priority = 3);
    void Warning(const QString& str, int priority = 1);
    void StatisticChanged();
    void OptionChanged(int index, const QString& value);
};
