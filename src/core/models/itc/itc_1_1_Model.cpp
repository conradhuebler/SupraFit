/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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



#include "src/core/AbstractTitrationModel.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"

#include <QtMath>

#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QDebug>
#include <QtCore/QDateTime>
#include <cmath>
#include <cfloat>
#include <iostream>

#include "itc_1_1_Model.h"

itc_ItoI_Model::itc_ItoI_Model(DataClass *data) : AbstractTitrationModel(data)
{
   PrepareParameter(GlobalParameterSize(), LocalParameterSize());
   DeclearSystemParameter();
}

itc_ItoI_Model::~itc_ItoI_Model() 
{
    
}

void itc_ItoI_Model::DeclearSystemParameter()
{
    m_data->addSystemParameter("Cell Volume", "Volume of the cell", SystemParameter::Scalar);
    m_data->addSystemParameter("Inject Volume", "Inject Volume per step", SystemParameter::Scalar);
    m_data->addSystemParameter("Temperature", "Temperature", SystemParameter::Scalar);
    m_data->addSystemParameter("Cell concentration", "Concentration in cell", SystemParameter::Scalar);
    m_data->addSystemParameter("Syringe concentration", "Concentration in syringe", SystemParameter::Scalar);
    m_data->LoadSystemParameter();
}


void itc_ItoI_Model::InitialGuess()
{
    m_K11 = 7;
    m_global_parameter = QList<qreal>() << m_K11 << -40000;
    
    m_local_parameter->data(0, 0) = -1000;
    m_local_parameter->data(1, 0) = 1;
    m_local_parameter->data(2, 0) = 1;

    setOptParamater(m_global_parameter);
    
    AbstractTitrationModel::Calculate();
}

QVector<qreal> itc_ItoI_Model::OptimizeParameters_Private(OptimizationType type)
{    
    if((OptimizationType::ComplexationConstants & type) == OptimizationType::ComplexationConstants)
        setOptParamater(m_global_parameter);

    addLocalParameter(0);
    addLocalParameter(1);
    addLocalParameter(2);
    QVector<qreal >parameter;
    for(int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}


qreal itc_ItoI_Model::HostConcentration(qreal host_0, qreal guest_0, const QList< qreal > &constants)
{
    if(constants.size() == 0)
        return host_0;
    qreal K11 = qPow(10, constants.first());
    qreal a, b, c;
    qreal complex;
    a = K11;
    b = -1*(K11*host_0+K11*guest_0+1);
    c = K11*guest_0*host_0;
    complex = MinQuadraticRoot(a,b,c);
    return host_0 - complex;
}

void itc_ItoI_Model::CalculateVariables()
{  
    m_corrupt = false;
    
    m_sum_absolute = 0;
    m_sum_squares = 0;

    qreal V = m_data->getSystemParameter("Cell Volume").Double();
    qreal initial_cell = m_data->getSystemParameter("Cell concentration").Double();
    qreal initial_syringe = m_data->getSystemParameter("Syringe concentration").Double();

#ifdef _DEBUG
    qDebug() << "Concentration in cell" << n_cell;
#endif
    qreal V_cell = V;
    qreal cum_shot = 0;
    qreal emp_exp = 1e-3;
    qreal cell = initial_cell*emp_exp;
    qreal gun = initial_syringe*emp_exp;
    qreal dil_heat = m_local_parameter->data(0, 0);
    qreal dil_inter = m_local_parameter->data(1, 0);
    qreal fx = m_local_parameter->data(2, 0);
    qreal dH = GlobalParameter()[1];
    qreal complex_prev = 0;
    qreal prod = 1;
    for(int i = 0; i < DataPoints(); ++i)
    {
        qreal shot_vol = IndependentModel()->data(0,i);
        cum_shot += shot_vol;
        V_cell += shot_vol;
        prod *= (1-shot_vol/V);
        cell *= (1-shot_vol/V);
        qreal host_0 = cell*fx; //n_cell/V_cell*emp_exp; ///fx;
        qreal guest_0 = gun*(1-prod);

#ifdef _DEBUG
        qDebug() << "Cell/Host concentration" << host_0;
     //   qDebug() << "Guest concentration " << guest_0;
#endif
        qreal dilution = (guest_0*dil_heat+dil_inter);
        qreal host = HostConcentration(host_0, guest_0, GlobalParameter());
        qreal complex = (host_0 - host);
        Vector vector(4);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest_0 - complex;
        vector(3) = complex;
#ifdef _DEBUG
        qDebug() << host/host_0;
#endif
        SetConcentration(i, vector);
        qreal value = V*(complex-complex_prev)*dH;
        SetValue(i, 0, value+dilution);
        complex_prev = complex;
    }
    emit Recalculated();
}


QSharedPointer<AbstractModel > itc_ItoI_Model::Clone()
{
    QSharedPointer<AbstractModel > model = QSharedPointer<itc_ItoI_Model>(new itc_ItoI_Model(this), &QObject::deleteLater);
    model.data()->setData( m_data );
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParamters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

qreal itc_ItoI_Model::BC50() const
{
    return 1/qPow(10,GlobalParameter()[0]); 
}

#include "itc_1_1_Model.moc"
