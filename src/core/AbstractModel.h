/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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


#ifndef ABSTRACTMODEL_H
#define ABSTRACTMODEL_H

#include "src/global.h"

#include <Eigen/Dense>

#include <QtCore/QJsonObject>
#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QtMath>

#include "dataclass.h"

typedef Eigen::VectorXd Vector;

struct ConfidenceBar
{
    qreal upper_2_5 = 0;
    qreal upper_5 = 0;
    qreal lower_2_5 = 0;
    qreal lower_5 = 0;
    qreal lower = 0;
    qreal upper = 0;
};

struct MassResults
{
    Vector MassBalance;
    Vector Components;
};

class AbstractTitrationModel : public DataClass
{
    Q_OBJECT

public:
    AbstractTitrationModel(const DataClass *data);
    virtual ~AbstractTitrationModel();
    QVector<qreal > OptimizeParameters(OptimizationType type);
    inline void setLockedParameter(const QList<int> &lock){ m_locked_parameters = lock; }
    inline QList<int> LockedParamters() const { return m_locked_parameters; }
    
    inline void setLastOptimzationRun(OptimizationType last_optimization) { m_last_optimization = last_optimization; }
    inline OptimizationType LastOptimzationRun() const { return m_last_optimization; }
    
    void SetSingleParameter(double value, int parameter);
    void setOptParamater(qreal & parameter);
    void setOptParamater(QList< qreal >& parameter);
    void addOptParameter(QList <qreal > &vector);
    void addOptParameterList_fromPure(int i);
    void addOptParameterList_fromConstant(int i);
    void clearOptParameter();
    inline int MaxVars() const { return (m_pure_signals_parameter.rows()); }
    qreal SumOfErrors(int i) const;
    virtual QPair<qreal, qreal> Pair(int i, int j = 0) const;
    inline qreal PureSignal(int i) const 
        { 
            if(i >= MaxVars())
                return 0;
            return m_pure_signals_parameter(i,0); 
        }
    /*
     * function to create a new instance of the model, this way was quite easier than
     * a copy constructor
     */
    virtual QSharedPointer<AbstractTitrationModel > Clone() const = 0;
        
    virtual int ConstantSize() const = 0;
    virtual void setPureSignals(const QList< qreal > &list);
    virtual void setComplexSignals(const QList< qreal > &list, int i);
    virtual void setConstants(const QList< qreal > &list);
    /*
     * defines the initial guess for the model
     */
    virtual void InitialGuess() = 0;
    QList<qreal >  getCalculatedSignals();

    inline QString Name() const { return m_name; }
    void setParamter(const QVector<qreal> &parameter);
    inline int Size() const { return DataClass::Size(); }
    
    inline DataTable * ModelTable() { return m_model_signal; }
    inline DataTable * ErrorTable() { return m_model_error; }
    inline DataTable * ModelTable() const  { return m_model_signal; }
    inline DataTable * ErrorTable() const { return m_model_error; }
    inline OptimizerConfig getOptimizerConfig() const { return m_opt_config; }
    inline void setOptimizerConfig(const OptimizerConfig &config) 
    { 
        m_opt_config = config;
    }
    inline bool isCorrupt() const { return m_corrupt; }
    void adress() const;
    /*
     * ! \brief Export model to json file
     * 
     */
    QJsonObject ExportModel(bool statistics = true) const;
    /* ! \brief Import model from json
     * 
     */
    void ImportModel(const QJsonObject &topjson, bool override = true);
    inline QList<int> ActiveSignals() const { return m_active_signals; }
    inline int ActiveSignals(int i) const 
    { 
        if(i < m_active_signals.size())
            return m_active_signals.at(i);
        else
            return 0;
    }
    inline void setActiveSignals(const QList<int > &active_signals) 
    { 
        m_active_signals = active_signals; 
        emit ActiveSignalsChanged(m_active_signals);
    }
    void MiniShifts();
    inline QVector<qreal *> getOptConstants() const { return m_opt_para; }
    qreal ModelError() const;
    inline QStringList ConstantNames() const { return m_constant_names; }
    void setCVStatistic(const QJsonObject &result, int i);
    void setMCStatistic(const QJsonObject &result, int i);
    void setMoCoStatistic(const QJsonObject &result, int i);
    /*
     * definies wheater this model can calculate equilibrium concentrations in parallel
     * should be useful when the equilibrium concentrations are calculated numerically
     */
    virtual bool SupportThreads() const = 0;
    QJsonObject getMCStatisticResult(int i) const { return m_mc_statistics[i]; }
    QJsonObject getCVStatisticResult(int i) const { return m_cv_statistics[i]; }
    QJsonObject getMoCoStatisticResult(int i) const { return m_moco_statistics[i]; }
    int getMCStatisticResult() const { return m_mc_statistics.size(); }
    int getCVStatisticResult() const { return m_cv_statistics.size(); }
    int getMoCoStatisticResult() const { return m_moco_statistics.size(); }
    inline QList<qreal > Constants() const { return m_complex_constants; }
    inline qreal Constant(int i) const { return m_complex_constants[i]; }
    virtual qreal BC50();
    virtual MassResults MassBalance(qreal A, qreal B);
    inline QPointer<DataTable > getConcentrations() { return m_concentrations; }
    inline qreal SumofSquares() const { return m_sum_squares; }
    inline qreal SumofAbsolute() const { return m_sum_absolute; }
    inline int Points() const { return m_used_variables; }
    inline int Parameter() { return m_opt_para.size(); }
    inline qreal MeanError() const { return m_mean; }
    inline qreal Variance() const { return m_variance; }
    inline qreal StdDeviation() const { return qSqrt(m_variance); }
    inline qreal StdError() const { return m_stderror; }
    inline Eigen::MatrixXd PureParameter() const { return m_pure_signals_parameter; }
    inline Eigen::MatrixXd ComplexParameter() const { return m_complex_signal_parameter; }
    /*! \brief Returns the f value for the given p value
     *  Degrees of freedom and number of parameters are taken in account
     */
    qreal finv(qreal p);
    qreal Error(qreal confidence, bool f = true);
    
public slots:
     inline  void Calculate() { Calculate(Constants());}
     void Calculate(const QList<qreal > &constants);
     
private:
    QList<int > m_active_signals;
    QList<int > m_locked_parameters;
    OptimizationType m_last_optimization;
    qreal CalculateVariance();
    qreal m_last_p, m_f_value;
    int m_last_parameter, m_last_freedom;
protected:
    /* 
     * @param int i, in j and qreal value
     * of the model signal - DataTable 
     */
    void SetSignal(int i, int j, qreal value);
    
    /*
     * set the concentration of the @param int i datapoint to
     * @param const Vector& equilibrium, 
     * the vector holds the concentration of
     * each species in that model
     */
    void SetConcentration(int i, const Vector &equlibrium);
    /*
     * Sets the name of the model, to be identifyed by 
     * the user
     */
    inline void setName(const QString &str) { m_name = str; }
    /*
     * This function handles the optimization flags, which get passed by
     * @param OptimizationType type
     * and returns the selected current CalculateVariables
     * @return QVector<qreal>
     * 
     */
    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type) = 0;
    /*
     * This function defines the model 
     * 
     */
    virtual void CalculateVariables(const QList<qreal > &constants) = 0;
    QString m_name;
    QList<qreal > m_complex_constants;
    QVector< QVector < qreal > > m_difference; 
    QVector<double * > m_opt_para;
    QVector<QVector<qreal * > >m_lim_para;
    QPointer<DataTable > m_model_signal, m_model_error, m_concentrations;
    Eigen::MatrixXd m_complex_signal_parameter, m_pure_signals_parameter;
    const DataClass *m_data;
    OptimizerConfig m_opt_config;
    bool m_corrupt;
    QStringList m_constant_names;
    QList< QJsonObject> m_mc_statistics;
    QList< QJsonObject> m_cv_statistics;
    QList< QJsonObject> m_moco_statistics;
    qreal m_sum_absolute, m_sum_squares, m_variance, m_mean, m_stderror;
    int m_used_variables;
        
signals:
    /*
     * Signal is emitted whenever void Calculate() is finished
     */
    void Recalculated();
    void Message(const QString &str, int priority = 3);
    void Warning(const QString &str, int priority = 1);
    void StatisticChanged();
};

#endif // ABSTRACTMODEL_H
