/*
 * <one line to give the library's name and an idea of what it does.>
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

#include "src/core/AbstractModel.h"
#include "src/core/minimizer.h"

#include <QtCore/QObject>
#include <QtCore/QWeakPointer>

#include "statistic.h"


Statistic::Statistic(QObject *parent) : QObject(parent), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater))
{
    
    
    
}

Statistic::~Statistic()
{
    
    
    
}

void Statistic::ConfidenceAssesment()
{
    if(!m_model)
        return;
    for(int i = 0; i < m_series.size(); ++i)
        m_series[i].clear();
    m_series.clear();
    
    m_minimizer->setModel(m_model);
    QJsonObject optimized = m_model->ExportJSON();
    QVector<double > parameter = m_model.data()->OptimizeParameters(m_type);
    
    m_model.data()->CalculateSignal();
    qreal error = m_model.data()->ModelError();
    for(int i = 0; i < parameter.size(); ++i)
    {
        QList<QPointF> series;

        QVector<int> locked(parameter.size(), 1);
        locked[i] = 0;
        
        m_model->setLockedParameter(locked);
        QVector<double> vars = parameter;
        double increment = vars[i]/200;
        for(int m = 0; m < 100; ++m)
        {
            double x  = m_model->IncrementParameter(increment, i);
            QJsonObject json = m_model->ExportJSON();
            m_minimizer->setParameter(json);
            m_minimizer->Minimize(m_type);
            {
            QJsonObject json = m_minimizer->Parameter();
            m_model->ImportJSON(json);
            m_model->CalculateSignal();
            }
            qreal new_error = m_model->ModelError();
            
            if(new_error/error > double(4))
                break;
            series.append(QPointF(x,new_error));
        }
        increment *= -1;
        qDebug() << "switching";
        m_model->ImportJSON(optimized);
        for(int m = 0; m < 100; ++m)
        {
            double x  = m_model->IncrementParameter(increment, i);
            QJsonObject json = m_model->ExportJSON();
            m_minimizer->setParameter(json);
            m_minimizer->Minimize(m_type);
            {
            QJsonObject json = m_minimizer->Parameter();
            m_model->ImportJSON(json);
            m_model->CalculateSignal();
            }
            qreal new_error = m_model->ModelError();
            
            if(new_error/error > double(4))
                break;
            series.prepend(QPointF(x,new_error));
        }
        m_series << series;
    }
}


void Statistic::setParameter(const QJsonObject& json)
{
    m_model->ImportJSON(json);
}

#include "statistic.moc"
