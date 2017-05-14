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


#ifndef ABSTRACTTITRATIONMODEL_H
#define ABSTRACTTITRATIONMODEL_H

#include "src/global.h"

#include <Eigen/Dense>

#include <QtCore/QJsonObject>
#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QtMath>

#include "AbstractModel.h"

typedef Eigen::VectorXd Vector;


struct MassResults
{
    Vector MassBalance;
    Vector Components;
};

class AbstractTitrationModel : public AbstractModel
{
    Q_OBJECT

public:
    AbstractTitrationModel(const DataClass *data);
    virtual ~AbstractTitrationModel();
    
    void addOptParameterList_fromPure(int i);
    void addOptParameterList_fromConstant(int i);
    
    inline int MaxVars() const { return (m_pure_signals_parameter.rows()); }

    virtual QPair<qreal, qreal> Pair(int i, int j = 0) const;
    inline qreal PureSignal(int i) const 
        { 
            if(i >= MaxVars())
                return 0;
            return m_pure_signals_parameter(i,0); 
        }
        
    
    virtual void setPureSignals(const QList< qreal > &list);
    virtual void setComplexSignals(const QList< qreal > &list, int i);
    
    /*
     * defines the initial guess for the model
     */
    


    inline QString Name() const { return m_name; }
//     void setParamter(const QVector<qreal> &parameter);
    inline int Size() const { return DataClass::Size(); }

    void adress() const;
    /*
     * ! \brief Export model to json file
     * 
     */
    QJsonObject ExportModel(bool statistics = true) const override;
    /* ! \brief Import model from json
     * 
     */
    void ImportModel(const QJsonObject &topjson, bool override = true) override;

    void MiniShifts() override;
    inline QVector<qreal *> getOptConstants() const { return m_opt_para; }
    
    virtual inline QString GlobalParameterPrefix() const override { return QString("10^"); }
    virtual QString formatedGlobalParameter(qreal value) const override;
    
    virtual qreal BC50();
    virtual MassResults MassBalance(qreal A, qreal B);
    inline QPointer<DataTable > getConcentrations() { return m_concentrations; }
    
    /*! \brief we have two concentrations for all titration models, host and guest
     */
    virtual inline int InputParameterSize() const override { return 2; }
    virtual int LocalParameterSize() const override {return GlobalParameterSize(); }
   /*  
private:
    QList<int > m_active_signals;
    QList<int > m_locked_parameters;
    OptimizationType m_last_optimization;
    qreal CalculateVariance();
    qreal m_last_p, m_f_value;
    int m_last_parameter, m_last_freedom;*/
   
protected:    
    /*
     * set the concentration of the @param int i datapoint to
     * @param const Vector& equilibrium, 
     * the vector holds the concentration of
     * each species in that model
     */
    void SetConcentration(int i, const Vector &equlibrium);

    QString m_name;
    QVector< QVector < qreal > > m_difference; 
    
    QVector<QVector<qreal * > >m_lim_para;
    QPointer<DataTable > m_concentrations;
    
    
};

#endif // ABSTRACTMODEL_H
