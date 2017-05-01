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

struct ConfidenceBar
{
    qreal upper_2_5 = 0;
    qreal upper_5 = 0;
    qreal lower_2_5 = 0;
    qreal lower_5 = 0;
    qreal lower = 0;
    qreal upper = 0;
};


class AbstractModel : public DataClass
{
  Q_OBJECT  
  
public:
    AbstractModel(const DataClass *data);
    ~AbstractModel();
    
    /*
     * function to create a new instance of the model, this way was quite easier than
     * a copy constructor
     */
    virtual QSharedPointer<AbstractModel > Clone() const = 0;
    
    /*
     * ! \brief Export model to json file
     * 
     */
    virtual QJsonObject ExportModel(bool statistics = true) const = 0;
    /* ! \brief Import model from json
     * 
     */
    virtual void ImportModel(const QJsonObject &topjson, bool override = true) = 0;
    
#warning fixme here
    virtual void setParamter(const QVector<qreal> &parameter) = 0; //FIXME
    
#warning fixme here
    virtual QVector<qreal > OptimizeParameters(OptimizationType type) = 0; //FIXME
     
     inline QStringList ConstantNames() const { return m_constant_names; }
    
    inline qreal SumofSquares() const { return m_sum_squares; }
    inline qreal SumofAbsolute() const { return m_sum_absolute; }
    inline int Points() const { return m_used_variables; }
    inline int Parameter() { return m_opt_para.size(); }
    inline qreal MeanError() const { return m_mean; }
    inline qreal Variance() const { return m_variance; }
    inline qreal StdDeviation() const { return qSqrt(m_variance); }
    inline qreal StdError() const { return m_stderror; }
#warning must be changed
    inline Eigen::MatrixXd PureParameter() const { return m_pure_signals_parameter; }
#warning must go away maybe
    inline Eigen::MatrixXd ComplexParameter() const { return m_complex_signal_parameter; }
    /*! \brief Returns the f value for the given p value
     *  Degrees of freedom and number of parameters are taken in account
     */
    qreal finv(qreal p);
    qreal Error(qreal confidence, bool f = true);
    
    virtual void InitialGuess() = 0;
public slots:
//      inline  void Calculate() { Calculate(Constants());}
     void Calculate(const QList<qreal > &constants);
     
private:

    
protected:
#warning to do as well
    QStringList m_constant_names; //FIXME more must be
    QVector<double * > m_opt_para;
    QList< QJsonObject> m_mc_statistics;
    QList< QJsonObject> m_cv_statistics;
    QList< QJsonObject> m_moco_statistics;
    qreal m_sum_absolute, m_sum_squares, m_variance, m_mean, m_stderror;
    int m_used_variables;
    Eigen::MatrixXd m_complex_signal_parameter, m_pure_signals_parameter;
    QList<int > m_active_signals;
    QList<int > m_locked_parameters;
    OptimizationType m_last_optimization;
    qreal CalculateVariance();
    qreal m_last_p, m_f_value;
    int m_last_parameter, m_last_freedom;
};


#endif
