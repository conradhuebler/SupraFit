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

#ifndef MODELCLASS_H
#define MODELCLASS_H

#include <QDebug>
#include <QtCore/qobject.h>
#include <QVector>
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
    qreal Error_Convergence = 5E-8;
    
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
    void setOptParamater(qreal & parameter);
    void addOptParameter(qreal &vector);
    void setOptParamater(QVector< qreal >& parameter);
    void addOptParameter(QVector <qreal > &vector);
    void clearOptParameter();
    int MaxVars() const { return (m_pure_signals.size()); }
    qreal SumOfErrors(int i) const;
    virtual QPair<qreal, qreal> Pair(int i, int j = 0) = 0;
    inline qreal PureSignal(int i) const 
        { 
            if(i >= MaxVars())
                return 0;
            return m_pure_signals[i]; 
        }
    virtual int ConstantSize() const = 0;
    virtual void setPureSignals(const QVector< qreal > &list) = 0;
    virtual void setComplexSignals(QVector< qreal > list, int i) = 0;
    virtual void setConstants(QVector< qreal > list) = 0;
    virtual void CalculateSignal(QVector<qreal > constants) = 0;
    virtual void InitialGuess() = 0;
    QVector<qreal >  getCalculatedSignals(QVector<int > active_signal = QVector<int >(1,0));
    virtual QVector<QVector< qreal > > AllShifts() = 0;
    virtual QVector<qreal> Minimize();
    virtual QVector<qreal > Constants() const = 0;
    inline QString Name() const { return m_name; }
    QVector<double > Parameter() const;
    void setParamter(const QVector<qreal> &parameter);
    inline QPointer<QtCharts::QVXYModelMapper> ModelMapper(const int i) { return m_model_mapper[i]; }
    inline QPointer<QtCharts::QVXYModelMapper> ErrorMapper(const int i) { return m_error_mapper[i]; }
        
    inline int Size() const { return DataClass::Size(); }
    
    inline DataTable * ModelSignal() { return m_model_signal; }
    inline DataTable * ModelError() { return m_model_error; }
    void UpdatePlotModels();
    inline OptimizerConfig getOptimizerConfig() const { return m_opt_config; }
    void setOptimizerConfig(const OptimizerConfig &config) 
    { 
        m_opt_config = config;
        m_inform_config_changed = true;
    }
    bool isCorrupt() const { return m_corrupt; }
    void adress() const;
public slots:
     inline  void CalculateSignal() { CalculateSignal(Constants());}
     
private:
    virtual qreal HostConcentration(qreal host_0, qreal guest_0, QVector<qreal > constants) = 0;
    QVector<QPointer<QtCharts::QVXYModelMapper> >m_model_mapper, m_error_mapper;
    
    bool m_debug;
    QString OptPara2String() const;
    inline QString bool2YesNo(bool var) const
    {
        if(var)
            return QString("yes");
        else
            return QString("No");
    }
protected:
    virtual void MiniShifts() = 0;
    void SetSignal(int i, int j, qreal value);
    inline void setName(const QString &str) { m_name = str; }
    void ClearDataSeries() ;
    QString m_name;
    //QVector< QVector<qreal > >m_pure_signals;
    QVector<qreal > m_pure_signals, m_complex_constants;
//     QHash<const DataPoint *, QVector< qreal> > m_signals;
    QVector< QVector < qreal > > m_difference; 
    bool *ptr_concentrations;
    bool m_repaint;
    QVector<double * > m_opt_para;
    QVector<QVector<qreal * > >m_opt_vec;
    QVector<QVector<qreal * > >m_lim_para;
    QStandardItemModel *m_plot_model, *m_plot_error;
    DataTable *m_model_signal, *m_model_error;
    const DataClass *m_data;
    OptimizerConfig m_opt_config;
    bool m_inform_config_changed, m_corrupt;
signals:
    void Recalculated();
    void Message(const QString &str, int priority = 3);
    void Warning(const QString &str, int priority = 1);
    
};


class ItoI_Model : public AbstractTitrationModel 
{
    Q_OBJECT
    
public:
    ItoI_Model(const DataClass *data);
    ~ItoI_Model();
    QPair<qreal, qreal> Pair(int i, int j = 0);
    inline int ConstantSize() const { return 1;}
    void setPureSignals(const QVector< qreal > &list);
    void setComplexSignals(QVector< qreal > list, int i);
    void setConstants(QVector< qreal > list);
    void CalculateSignal(QVector<qreal > constants = QVector<qreal>());
    QVector<qreal > Constants() const { return QVector<qreal>() << m_K11; }
    virtual QVector< QVector< qreal > > AllShifts();
    virtual void InitialGuess();
private:
    void MiniShifts();
    inline qreal HostConcentration(qreal host_0, qreal guest_0) {return HostConcentration(host_0, guest_0, Constants());}
    qreal HostConcentration(qreal host_0, qreal guest_0, QVector<qreal > constants);
    qreal m_K11;
    
    QVector<qreal > m_ItoI_signals;
};

class IItoI_ItoI_Model : public AbstractTitrationModel
{
     Q_OBJECT
    
public:
    IItoI_ItoI_Model(const DataClass* data);
    ~IItoI_ItoI_Model();
    
    QPair<qreal, qreal> Pair(int i, int j = 0);
    inline int ConstantSize() const { return 2;}
    void setPureSignals(const QVector< qreal > &list);
    void setComplexSignals(QVector< qreal > list, int i);
    void setConstants(QVector< qreal > list);
    virtual void CalculateSignal(QVector<qreal > constants= QVector<qreal>());
    QVector<qreal > Constants() const { return m_complex_constants; }
    virtual QVector< QVector< qreal > > AllShifts();
    void MiniShifts();
    virtual void InitialGuess();
private:
    inline qreal HostConcentration(qreal host_0, qreal guest_0) {return HostConcentration(host_0, guest_0, Constants());}
    qreal HostConcentration(qreal host_0, qreal guest_0, QVector<qreal > constants);
protected:
    qreal m_K21, m_K11;
    QVector<qreal > m_ItoI_signals, m_IItoI_signals;
};

// class test_II_ItoI_Model : public IItoI_ItoI_Model
// {
//     Q_OBJECT
//     
// public:
//     test_II_ItoI_Model(const DataClass* data) : IItoI_ItoI_Model(data) { }
//     
//     virtual void CalculateSignal(QVector<qreal > constants= QVector<qreal>());
// };

class ItoI_ItoII_Model : public AbstractTitrationModel
{
     Q_OBJECT
    
public:
    ItoI_ItoII_Model(const DataClass* data);
    ~ItoI_ItoII_Model();
    
    QPair<qreal, qreal> Pair(int i, int j = 0);
    inline int ConstantSize() const { return 2;}
    void setPureSignals(const QVector< qreal > &list);
    void setComplexSignals(QVector< qreal > list, int i);
    void setConstants(QVector< qreal > list);
    void CalculateSignal(QVector<qreal > constants = QVector<qreal>());
    QVector<qreal > Constants() const { return m_complex_constants; }
    virtual QVector< QVector< qreal > > AllShifts();
    void MiniShifts();
    virtual void InitialGuess();
    
private:
    inline qreal HostConcentration(qreal host_0, qreal guest_0) 
    {
        return HostConcentration(host_0, guest_0, Constants());
    }
    qreal HostConcentration(qreal host_0, qreal guest_0, QVector<qreal > constants);
    qreal GuestConcentration(qreal host_0, qreal guest_0, QVector<qreal > constants);
    
    qreal m_K11, m_K12;
    QVector<qreal > m_ItoI_signals, m_ItoII_signals;
};

#endif // MODELCLASS_H
