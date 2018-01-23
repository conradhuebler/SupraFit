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
    
    enum PlotMode { 
        H = 1, 
        G = 2, 
        HG = 3, 
        GH = 4
    };
    
    AbstractTitrationModel(DataClass *data);
    AbstractTitrationModel(AbstractTitrationModel *other);
    virtual ~AbstractTitrationModel();

    inline QString Name() const { return m_name; }
    inline int Size() const { return DataClass::Size(); }

    void adress() const;


    inline QVector<qreal *> getOptConstants() const { return m_opt_para; }
    
    virtual inline QString GlobalParameterPrefix(int i = 0) const override {Q_UNUSED(i)  return QString("10^"); }
    virtual inline QString LocalParameterSuffix(int i = 0) const override {Q_UNUSED(i) return QString(" ppm"); }
    virtual inline QString LocalParameterDescription(int i = 0) const override
    {
        if(i == 0)
            return "Shift of the pure - non silent substrat";
        else
            return tr("Shift of the pure %1 complex").arg(GlobalParameterName(i-1));
    }
    inline virtual QString LocalParameterName(int i = 0) const override
    {
        if(i == 0)
            return "Pure Component Shift";
        else
            return tr("%1 Complex Shift").arg(GlobalParameterName(i-1));
    }
    
    virtual QString formatedGlobalParameter(qreal value, int globalParameter) const override;
    
    virtual qreal BC50() const;
    virtual MassResults MassBalance(qreal A, qreal B);
    inline QPointer<DataTable > getConcentrations() const { return m_concentrations; }
    inline QPointer<DataTable > getConcentrations() { return m_concentrations; }
    /*! \brief we have two concentrations for all titration models, host and guest
     */
    virtual inline int InputParameterSize() const override { return 2; }
    virtual int LocalParameterSize() const override {return GlobalParameterSize() + 1; }
    
    /*! \brief reimplmented from AbstractModel
     */
    virtual QString Model2Text_Private() const override;
   
    /*! \brief reimplementantion model dependented printout of the independant parameter
     */
    virtual qreal PrintOutIndependent(int i, int format = 4) const override;
    
    virtual QString ModelInfo() const override;
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
