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

#include "src/core/minimizer.h"
#include "src/core/models.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QThreadPool>
#include <QtCore/QVector>
#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QPointF>
#include <QtCore/QList>

#include <iostream>


#include "globalsearch.h"

GlobalSearch::GlobalSearch(QObject *parent) : QObject(parent), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater)), m_initial_guess(true), m_optimize(true)
{
    
    
}

GlobalSearch::~GlobalSearch()
{
    
    
}
void GlobalSearch::Interrupt()
{
    m_allow_break = true; 
    QThreadPool::globalInstance()->clear();
}

QVector<QVector<double> > GlobalSearch::ParamList() 
{
    int max_count = 1;
    
    m_time = 0;
    QVector< QVector<double > > full_list;
    for(int i = 0; i < m_parameter.size(); ++i)
    {
        QVector<double > list;
        double min = 0, max = 0, step = 0;
        min = m_parameter[i][0];
        max = m_parameter[i][1];
        step = m_parameter[i][2];
        max_count *= (max+step-min)/step;
        for(double s = min; s <= max; s += step)
            list << s;
        full_list << list;
    }
    emit setMaximumSteps(max_count);
    return full_list;
}

QList<QJsonObject > GlobalSearch::SearchGlobal()
{
    QVector< QVector<double > > full_list = ParamList();
    m_models_list.clear();
    QVector<double > error; 
    m_type |= OptimizationType::ComplexationConstants;
    int t0 = QDateTime::currentMSecsSinceEpoch();
    ConvertList(full_list, error);
    int t1 = QDateTime::currentMSecsSinceEpoch();
    std::cout << "time for scanning: " << t1-t0 << " msecs." << std::endl;
    return m_models_list;
}

QVector<VisualData> GlobalSearch::Create2DPLot()
{
    QVector< QVector<double > > full_list = ParamList();
    m_models_list.clear();
    QVector<double > error; 

    int t0 = QDateTime::currentMSecsSinceEpoch();
    ConvertList(full_list, error);
    int t1 = QDateTime::currentMSecsSinceEpoch();
    std::cout << "time for scanning: " << t1-t0 << " msecs." << std::endl;
    return m_3d_data;
}


void GlobalSearch::ConvertList(const QVector<QVector<double> >& full_list, QVector<double > &error)
{  
    m_full_list.clear();
    QVector<int > position(full_list.size(), 0);
    int maxthreads =qApp->instance()->property("threads").toInt();
    QVector<QVector<QPointer<NonLinearFitThread> > > threads;
    QVector<QPointer<NonLinearFitThread> > thread_rows;
    QThreadPool::globalInstance()->setMaxThreadCount(maxthreads -1 );
    m_allow_break = false;
    if(m_initial_guess)
        m_model->InitialGuess();
    while(!m_allow_break)
    {
        QCoreApplication::processEvents();
        QList<double > parameter; 
        for(int j = 0; j < position.size(); ++j)
            parameter << full_list[j][position[j]];
        
        bool temporary = true;
        for(int i = 0; i < position.size(); ++i)
        {
            temporary = temporary && (position[i] == full_list[i].size() - 1);
        }
        if(temporary)
            m_allow_break = true;
        m_model->setConstants(parameter);
        m_full_list.append( parameter );
        QPointer< NonLinearFitThread > thread = m_minimizer.data()->addJob(m_model, m_type, m_optimize);
        thread_rows << thread;
        connect(thread, SIGNAL(finished(int)), this, SIGNAL(SingeStepFinished(int)), Qt::DirectConnection);
        
        if(m_model->SupportThreads())
            QThreadPool::globalInstance()->waitForDone();
        
        for(int k = position.size() - 1; k >= 0; --k)
        {
            if(position[k] == ( full_list[k].size() - 1) )
            {
                position[k] = 0;
                if(position[k] <= full_list[k].size() - 1)
                {
                    threads << thread_rows;
                    thread_rows.clear();
                }
                
            }
            else
            {
                position[k]++;
                if(position[k] == full_list[k].size())
                {
                    position[k] = 0;
                    if(k > 0)
                        position[k - 1]++;
                }
                break;
            }
        }
    }
    if(m_optimize)
    {
        if(!m_model->SupportThreads())
        {
            QThreadPool::globalInstance()->setMaxThreadCount(maxthreads);
            while(QThreadPool::globalInstance()->activeThreadCount())
            {
                QCoreApplication::processEvents();
            }
        }
    }
    for(int i = 0; i < threads.size(); ++i)
    {
        VisualData dataRow1;
        for(int j = 0; j < threads[i].size(); ++j)
        {
            QCoreApplication::processEvents();
            QList< qreal > parameter = threads[i][j]->Model()->Constants();
            
            QJsonObject json = threads[i][j]->ConvergedParameter();
            m_model->ImportJSON(json);
            m_model->Calculate();
            double current_error = m_model->ModelError();
            error << current_error; 
            if(error_max < current_error)
                error_max = current_error;
            last_result.m_error = error;
            last_result.m_input = full_list;
            if(m_type & OptimizationType::ComplexationConstants)
                m_models_list << json;
            else
                dataRow1.data.append(QVector<qreal >() << parameter[0] << m_model->SumofSquares() << parameter[1]);
            delete threads[i][j];
        }
        if(!(m_type & OptimizationType::ComplexationConstants))
            m_3d_data << dataRow1;
    }
    
    return;
}

QList<QList<QPointF> > GlobalSearch::LocalSearch()
{
    QVector< QVector<double > > full_list = ParamList();
    Scan(full_list);
    return m_series;
}


void GlobalSearch::Scan(const QVector< QVector<double > >& list)
{
    for(int i = 0; i < m_series.size(); ++i)
        m_series[i].clear();
    m_series.clear();
    QVector<double > error;
    for(int j = 0; j < list.size(); ++j)
    {
        QList<QPointF> series;
        for(int i = 0; i < list[j].size(); ++i)
        {
            m_model->setConstants(QList<qreal> () << list[j][i]);
            m_model->Calculate();
            error << m_model->ModelError();
            series.append(QPointF(list[j][i],m_model->ModelError( )));
        }
        m_series << series;
    }
}


#include "globalsearch.moc"
