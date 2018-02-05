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

#include <QtCore/QJsonObject>
#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QtMath>

#include "dataclass.h"

struct ModelOption
{
    QStringList values;
    QString value = "unset"; 
};

class AbstractModel : public DataClass
{
  Q_OBJECT  
  
public:
    AbstractModel(DataClass *data);
    AbstractModel(AbstractModel *other);
    
    virtual ~AbstractModel();
    
    /*! \brief set the OptimizationType to type and returns the Parameters
     * 
     */
    QVector<qreal > OptimizeParameters(OptimizationType type);
    
    /*! \brief Locks Parameter not to be optimised during Levenberg-Marquadt
     */
    inline void setLockedParameter(const QList<int> &lock){ m_locked_parameters = lock; }
    
    /*! \brief Clear the list of to be optimised Parameters
     */
    void clearOptParameter();
    
    /*! \brief returns the locked Parameters
     */
    inline QList<int> LockedParamters() const { return m_locked_parameters; }
    
    /*! \brief Set the last OptimizationType to runtype
     */
    inline void setLastOptimzationRun(OptimizationType last_optimization) { m_last_optimization = last_optimization; }
    
    /*! \brief Returns the last OptimizationType
     */
    inline OptimizationType LastOptimzationRun() const { return m_last_optimization; }
    
    /*
     * function to create a new instance of the model, this way was quite easier than
     * a copy constructor
     */
    virtual QSharedPointer<AbstractModel > Clone() = 0;
    
    /*
     * ! \brief Export model to json file
     * 
     */
    QJsonObject ExportModel(bool statistics = true, bool locked = false) const;
    
    /* ! \brief Import model from json
     * 
     */
    void ImportModel(const QJsonObject &topjson, bool override = true);
 
    /*! \brief Returns the name of the model
     */
    virtual QString Name() const = 0; 
    
        
    /*! \brief get the Name of the ith GlobalParameter
     */
    inline virtual QString GlobalParameterName(int i = 0) const {Q_UNUSED(i) return QString();}

    /*! \brief Prefix of the ith global parameter 
     */
    virtual inline QString GlobalParameterPrefix(int i = 0) const { Q_UNUSED(i) return QString(); }
    
    /*! \brief Suffix of the ith  global parameter 
     */
    virtual inline QString GlobalParameterSuffix(int i = 0) const {Q_UNUSED(i) return QString(); }
    
    /*! \brief Description of the ith global parameter 
     */   
    virtual inline QString GlobalParameterDescription(int i = 0) const { Q_UNUSED(i) return QString(); }
    
    /*! \brief get the Name of the ith LocalParameter
     */
    inline virtual QString LocalParameterName(int i = 0) const {Q_UNUSED(i) return QString();} 
    
    /*! \brief Prefix of theith  local parameter 
     */
    virtual inline QString LocalParameterPrefix(int i = 0) const {Q_UNUSED(i) return QString(); }
    
    /*! \brief Suffix of the ith local parameter 
     */
    virtual inline QString LocalParameterSuffix(int i = 0) const {Q_UNUSED(i) return QString(); }
    
    /*! \brief Description of the ith local parameter 
     */   
    virtual inline QString LocalParameterDescription(int i = 0) const { Q_UNUSED(i) return QString(); }
    
    /*! \brief Return a formated value as string of the global parameter with the value
     */
    virtual QString formatedGlobalParameter(qreal value, int globalParamater = 0) const { Q_UNUSED(globalParamater) return QString::number(value); }
    
    /*! \brief Return a formated value as string of the global parameter with the value
     */
    virtual QString formatedLocalParameter(qreal value, int localParamater = 0) const { Q_UNUSED(localParamater) return QString::number(value); }

    virtual inline int Color(int i) const { return i; }
        
    void SetSingleParameter(double value, int parameter);
    void setOptParamater(qreal & parameter);
    void setOptParamater(QList< qreal >& parameter);
    void addGlobalParameter(QList <qreal > &vector);
    void addGlobalParameter(int i);
    void addLocalParameter(int i);
   
    void UpdateStatistic(const QJsonObject &object);
    
    QJsonObject getMCStatisticResult(int i) const { return m_mc_statistics[i]; }
    QJsonObject getWGStatisticResult() const { return m_wg_statistics; }
    QJsonObject getMoCoStatisticResult() const { return m_moco_statistics; }
    QJsonObject getReduction() const { return m_reduction; }
    QJsonObject getFastConfidence() const { return m_fast_confidence; }
    
    int getMCStatisticResult() const { return m_mc_statistics.size(); }
        
    QList<qreal >  getCalculatedModel();
        
    /*! \brief returns a List of all Series, that are to be included in optimisation
     */
    inline QList<int> ActiveSignals() const { return m_active_signals; }

    /*! \brief return if the series i is active in optimisation
     */
    inline int ActiveSignals(int i) const 
    { 
        if(i < m_active_signals.size())
            return m_active_signals.at(i);
        else
            return 1;
    }
    
    /*! \brief set the series active / inactive according to the given list
     * 0 - inactive
     * 1 - active
     */
    inline void setActiveSignals(const QList<int > &active_signals) 
    { 
        m_active_signals = active_signals; 
        emit ActiveSignalsChanged(m_active_signals);
    }
    
    /*! \brief Set the current active parameter (which are internally reference through a pointer)
     * to the given vector
     */
    void setParameter(const QVector<qreal> &parameter);
    
    /*! \brief Returns if the current model parameter makes dont't sense
     * true - no sense
     * false - all fine
     */
    inline bool isCorrupt() const { return m_corrupt; }
    
    inline qreal SumofSquares() const { return m_sum_squares; }
    inline qreal SumofAbsolute() const { return m_sum_absolute; }
    inline int Points() const { return m_used_variables; }
    inline int Parameter() { return m_opt_para.size(); }
    inline qreal MeanError() const { return m_mean; }
    inline qreal Variance() const { return m_variance; }
    inline qreal StdDeviation() const { return qSqrt(m_variance); }
    inline qreal StdError() const { return m_stderror; }
    inline qreal SEy() const { return m_SEy; }
    inline qreal ChiSquared() const { return m_chisquared; }
    inline qreal CovFit() const { return m_covfit; }

    inline bool isConverged() const { return m_converged; }
    inline void setConverged(bool converged) { m_converged = converged; }
    /*! \brief Returns the f value for the given p value
     *  Degrees of freedom and number of parameters are taken in account
     */
    qreal finv(qreal p);
    qreal Error(qreal confidence, bool f = true);
    
    virtual void InitialGuess() = 0;
    
    /*! \brief Returns pointer to Model DataTable
     * overloaded function
     */
    inline DataTable * ModelTable() { return m_model_signal; }
    
    /*! \brief Returns pointer to Error DataTable
     * overloaded function
     */
    inline DataTable * ErrorTable() { return m_model_error; }
    
    /*! \brief Returns const pointer to Model DataTable
     * overloaded function
     */
    inline DataTable * ModelTable() const  { return m_model_signal; }
    
    /*! \brief Returns const pointer to Error DataTable
     * overloaded function
     */
    inline DataTable * ErrorTable() const { return m_model_error; }
    
    inline OptimizerConfig getOptimizerConfig() const { return m_opt_config; }
    inline void setOptimizerConfig(const OptimizerConfig &config) 
    { 
        m_opt_config = config;
    }
    
    /*
     * definies wheater this model can be calculate in parallel
     * should be useful when the model observables are calculated numerically
     */
    virtual bool SupportThreads() const = 0;
    
    qreal SumOfErrors(int i) const;
    
    qreal ModelError() const;
    
    /*! \brief set the values of the global parameter to const QList< qreal > &list
     */
    virtual void setGlobalParameter(const QList< qreal > &list);
    /*! \brief return the list of global parameter values
     */
    inline QList<qreal > GlobalParameter() const { return m_global_parameter; }
    /*! \brief return i global parameter
     */
    inline qreal GlobalParameter(int i) const { return m_global_parameter[i]; }
    /*! \brief returns size of global parameter
     */
    virtual int GlobalParameterSize() const = 0;
    /*! \brief return size of input parameter 
     */
    virtual int InputParameterSize() const = 0;
    
     /*! \brief returns size of local parameter
     */
    virtual int LocalParameterSize() const = 0;
    
    qreal LocalParameter(int parameter, int series) const;
    QVector<qreal> getLocalParameterColumn(int parameter) const;
    void setLocalParameter(qreal value, int parameter, int series);
    void setLocalParameterSeries(const QVector<qreal > &vector, int series);
    void setLocalParameterSeries(const Vector &vector, int series);
    void setLocalParameterColumn(const QVector<qreal> &vector, int parameter);
    void setLocalParameterColumn(const Vector &vector, int parameter);
    
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
    
    AbstractModel &operator=(const AbstractModel &other);
    AbstractModel *operator=(const AbstractModel *other);
    inline void setData(DataClass *data) { m_data = data;}
    
    inline void addOption(const QString &name, const QStringList &values)
    {
        if(m_model_options.contains(name))
            return;
        ModelOption option;
        option.values = values;
        option.value = values.first();
        m_model_options[name] = option;
    }
    
    void setOption(const QString &name, const QString &value);
    
    inline QString getOption(const QString &name) const
    {
        if(!m_model_options.contains(name))
            return QString("unset");
        ModelOption option = m_model_options[name];
        return option.value;
    }
    
    inline const QStringList getAllOptions() const { return m_model_options.keys(); }
    
    inline const QStringList getSingleOptionValues(const QString &name) const { if(m_model_options.contains(name)) return m_model_options[name].values; else return QStringList(); }
    
    inline virtual void DeclareOptions() { }
    
    inline virtual void EvaluateOptions() { }
    
    inline virtual QString ModelInfo() const { return QString(); }
    
    inline bool isLocked() const { return m_locked_model; }
public slots:
    /*! \brief Calculated the current model with all previously set and defined parameters
     */
     void Calculate();
     
private:

    
protected:
    /*
     * This function handles the optimization flags, which get passed by
     * @param OptimizationType type
     * and returns the selected current CalculateVariables
     * @return QVector<qreal>
     * 
     */
    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type) = 0;
    
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
    qreal CalculateVariance();

    /*! \brief Calculated the variance of the raw data
     */
    qreal CalculateCovarianceFit();
        
// #warning to do as well
    //FIXME more must be
    QVector<double * > m_opt_para;
    QList<qreal > m_global_parameter;
    QList< QJsonObject> m_mc_statistics;
    
    QJsonObject m_wg_statistics;
    QJsonObject m_moco_statistics;
    QJsonObject m_reduction;
    QJsonObject m_fast_confidence;
    
    qreal m_sum_absolute, m_sum_squares, m_variance, m_mean, m_stderror, m_SEy, m_chisquared, m_covfit;
    int m_used_variables;
    QList<int > m_active_signals;
    QList<int > m_locked_parameters, m_enabled_parameter;
    OptimizationType m_last_optimization;
    qreal m_last_p, m_f_value;
    int m_last_parameter, m_last_freedom;
    bool m_corrupt, m_converged, m_locked_model;
    OptimizerConfig m_opt_config;
    QPointer<DataTable > m_model_signal, m_model_error, m_local_parameter;
//     QString m_name;
        
    DataClass *m_data;
    QMap<QString, ModelOption > m_model_options;
    
signals:
    /*
     * Signal is emitted whenever void Calculate() is finished
     */
    void Recalculated();
    void Message(const QString &str, int priority = 3);
    void Warning(const QString &str, int priority = 1);
    void StatisticChanged();
};
