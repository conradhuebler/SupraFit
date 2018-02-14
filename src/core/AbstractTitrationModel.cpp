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

AbstractTitrationModel::AbstractTitrationModel(DataClass *data) : AbstractModel(data)
{
    m_last_optimization = static_cast<OptimizationType>(OptimizationType::ComplexationConstants | OptimizationType::OptimizeShifts);
}

AbstractTitrationModel::AbstractTitrationModel(AbstractTitrationModel *other) : AbstractModel(other)
{
    m_last_optimization = static_cast<OptimizationType>(OptimizationType::ComplexationConstants | OptimizationType::OptimizeShifts);
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


qreal AbstractTitrationModel::BC50() const
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

QString AbstractTitrationModel::formatedGlobalParameter(qreal value, int globalParameter) const
{
    Q_UNUSED(globalParameter)
    QString string;
    string = QString::number(qPow(10,value));
    return string;
}

QString AbstractTitrationModel::Model2Text_Private() const
{
    QString text;

    text += "Equilibrium concentration calculated with complexation constants:\n";
    for(int i = 0; i < getConcentrations()->columnCount(); ++i)
    {
        text += getConcentrations()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    }
    text += "\n";
    text += getConcentrations()->ExportAsString();
    text += "\n\n";
    text += "Equilibrium Model Signal calculated with complexation constants:\n";
    for(int i = 0; i < DependentModel()->columnCount(); ++i)
        text += DependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    return text;
}

qreal AbstractTitrationModel::PrintOutIndependent(int i, int format) const
{
    switch(format){
            case PlotMode::G:
                    return InitialGuestConcentration(i); 
                break;
                
            case PlotMode::H:   
                    return InitialHostConcentration(i);
                break;
                
            case PlotMode::HG:
                    return InitialHostConcentration(i)/InitialGuestConcentration(i);                
                break;    
                
            case PlotMode::GH:
            default:
                    return InitialGuestConcentration(i)/InitialHostConcentration(i);                   
                break;    
        };
}

QString AbstractTitrationModel::ModelInfo() const 
{
    qreal bc50 = BC50()*1E6;
    QString format_text;
    if(bc50 > 0)
    {
        format_text = tr("BC50<sub>0</sub>: %1").arg(bc50);
        QChar mu = QChar(956);
        format_text += QString(" [") + mu + 


QString("M]");
        return format_text;
    }else
        return QString();
}

qreal AbstractTitrationModel::Guess_1_1() const
{
    qreal K11 = 0;
    QVector<qreal> x;
    QVector< QVector<qreal> > y(SeriesCount());
    for(int i = 1; i < DataPoints(); ++i)
    {
        if(!(InitialHostConcentration(i)*InitialGuestConcentration(i)))
            continue;
        x << (1/InitialHostConcentration(i)/InitialGuestConcentration(i));
        for(int j = 0; j < SeriesCount(); ++j)
        {
            y[j] << 1/(DependentModel()->data(j,i)-DependentModel()->data(j,0));
        }
    }
    for(int i = 0; i < SeriesCount(); ++i)
    {
        LinearRegression regress = LeastSquares(x, y[i]);
        K11 += qLn(qAbs(1/regress.m))/2.3;
    }
    K11 /= double(SeriesCount());
    return K11;
}


#include "AbstractTitrationModel.moc"
