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




#include "libmath.h"
#include "src/core/dataclass.h"
#include "src/core/toolset.h"

#include <QtMath>

#include <QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QDateTime>
#include <QtCore/QCollator>
#include <cmath>
#include <cfloat>
#include <iostream>
#include "AbstractTitrationModel.h"

AbstractTitrationModel::AbstractTitrationModel(const DataClass *data) : AbstractModel(data)
{
    m_last_optimization = static_cast<OptimizationType>(OptimizationType::ComplexationConstants | OptimizationType::OptimizeShifts | OptimizationType::UnconstrainedShifts);
//     m_constant_names << tr("no constants");   
}

AbstractTitrationModel::~AbstractTitrationModel()
{

}

void AbstractTitrationModel::adress() const
{
    std::cout << "We are at " << this;
    std::cout << "\t" << m_data;
    for(int i = 0; i < m_opt_para.size(); ++i)
        std::cout << m_opt_para[i] << " ";
    std::cout << std::endl;
}

void AbstractTitrationModel::SetConcentration(int i, const Vector& equilibrium)
{
    if(!m_concentrations)
    {
        m_concentrations = new DataTable( equilibrium.rows(), DataPoints(), this);
    }
    m_concentrations->setRow(equilibrium, i);
}


void AbstractTitrationModel::MiniShifts()
{
    double cut_error = 1;
    for(int j = 0; j < m_lim_para.size(); ++j)
    {
        for(int i = 0; i < SeriesCount(); ++i)    
        {
            if(ActiveSignals(i) == 1)
            {
                if(m_model_error->data(i, 0) < cut_error && j == 0)
                    *m_lim_para[j][i] -= m_model_error->data(i,0);
                if(m_model_error->data(i, m_model_error->rowCount() -1 ) < cut_error && j == 1)
                    *m_lim_para[j][i] -= m_model_error->data(i, m_model_error->rowCount() -1 );   
            }
        }
    }
}

qreal AbstractTitrationModel::BC50()
{
    return 0;
}


MassResults AbstractTitrationModel::MassBalance(qreal A, qreal B)
{
    MassResults result;
    Vector values(1) ;
    values(0) = 0;
    result.MassBalance = values;
    return result;
}

QString AbstractTitrationModel::formatedGlobalParameter(qreal value) const
{
    QString string;
    string = QString::number(qPow(10,value));
    return string;
}



#include "AbstractTitrationModel.moc"
