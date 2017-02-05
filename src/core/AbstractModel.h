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

#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCharts/QLineSeries>
#include <QtCharts/QVXYModelMapper>

#include "dataclass.h"

class QStandardItemModel;


struct OptimizerConfig
{
    int MaxIter = 1000;
    int Sum_Convergence = 2;
    qreal Shift_Convergence = 1E-3;
    qreal Constant_Convergence = 1E-3;
    qreal Error_Convergence = 5E-7;
    
    bool OptimizeBorderShifts = true;
    bool OptimizeIntermediateShifts = true;
    
    int LevMar_Constants_PerIter = 1;
    int LevMar_Shifts_PerIter = 1;
    
    qreal LevMar_mu = 1E-03;
    qreal LevMar_Eps1 = 1E-15;
    qreal LevMar_Eps2 = 1E-15;
    qreal LevMar_Eps3 = 1E-20;
    qreal LevMar_Delta = 1E-06;
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
    virtual QVector<qreal > OptimizeParameters_Private(OptimizationType type) = 0;
    double IncrementParameter(double increment, int parameter);
    void setOptParamater(qreal & parameter);
    void addOptParameter(qreal &vector);
    void setOptParamater(QVector< qreal >& parameter);
    void addOptParameter(QVector <qreal > &vector);
    void clearOptParameter();
    inline int MaxVars() const { return (m_pure_signals.size()); }
    qreal SumOfErrors(int i) const;
    virtual QPair<qreal, qreal> Pair(int i, int j = 0) const = 0;
    inline qreal PureSignal(int i) const 
        { 
            if(i >= MaxVars())
                return 0;
            return m_pure_signals[i]; 
        }
        
    virtual QSharedPointer<AbstractTitrationModel > Clone() const = 0;
        
    virtual int ConstantSize() const = 0;
    virtual void setPureSignals(const QVector< qreal > &list) = 0;
    virtual void setComplexSignals(QVector< qreal > list, int i) = 0;
    virtual void setConstants(QVector< qreal > list) = 0;
    virtual void CalculateSignal(QVector<qreal > constants) = 0;
    virtual void InitialGuess() = 0;
    QVector<qreal >  getCalculatedSignals(QList<int > active_signal = QList<int >() << 0);

    virtual QVector<qreal > Constants() const = 0;
    inline QString Name() const { return m_name; }
    QVector<double > Parameter() const;
    void setParamter(const QVector<qreal> &parameter);
    inline QPointer<QtCharts::QVXYModelMapper> ModelMapper(const int i) { return m_model_mapper[i]; }
    inline QPointer<QtCharts::QVXYModelMapper> ErrorMapper(const int i) { return m_error_mapper[i]; }
        
    inline int Size() const { return DataClass::Size(); }
    
    inline DataTable * ModelTable() { return m_model_signal; }
    inline DataTable * ErrorTable() { return m_model_error; }
    inline DataTable * ModelTable() const  { return m_model_signal; }
    inline DataTable * ErrorTable() const { return m_model_error; }
    void UpdatePlotModels();
    inline OptimizerConfig getOptimizerConfig() const { return m_opt_config; }
    inline void setOptimizerConfig(const OptimizerConfig &config) 
    { 
        m_opt_config = config;
        m_inform_config_changed = true;
    }
    inline bool isCorrupt() const { return m_corrupt; }
    void adress() const;
    QJsonObject ExportJSON() const;
    void ImportJSON(const QJsonObject &topjson);
    
    inline QList<int > ActiveSignals() { return m_active_signals; }
    inline QList<int > ActiveSignals() const { return m_active_signals; }
    inline void setActiveSignals(QList<int > active_signals) 
    { 
        m_active_signals = active_signals; 
        emit ActiveSignalsChanged(m_active_signals);
    }
    void MiniShifts();
    inline QVector<qreal *> getOptConstants() const { return m_opt_para; }
    qreal ModelError() const;
    inline QStringList ConstantNames() const { return m_constant_names; }
public slots:
     inline  void CalculateSignal() { CalculateSignal(Constants());}
     
private:
//     virtual qreal HostConcentration(qreal host_0, qreal guest_0, QVector<qreal > constants) = 0;
    QList<QPointer<QtCharts::QVXYModelMapper> >m_model_mapper, m_error_mapper;
    
    bool m_debug;

    QList<int > m_active_signals;
    bool m_pending;
    QList<int > m_locked_parameters;
protected:
    
    void SetSignal(int i, int j, qreal value);
    inline void setName(const QString &str) { m_name = str; }
    void ClearDataSeries() ;
    QString m_name;
    QVector<qreal > m_pure_signals, m_complex_constants;
    QVector< QVector < qreal > > m_difference; 
    bool *ptr_concentrations;
    QVector<double * > m_opt_para;
    QVector<QVector<qreal * > >m_lim_para;
    QStandardItemModel *m_plot_model, *m_plot_error;
    DataTable *m_model_signal, *m_model_error;
    const DataClass *m_data;
    OptimizerConfig m_opt_config;
    bool m_inform_config_changed, m_corrupt;
    QStringList m_constant_names;
signals:
    void Recalculated();
    void Message(const QString &str, int priority = 3);
    void Warning(const QString &str, int priority = 1);
    
};

#endif // ABSTRACTMODEL_H
