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
#include "src/ui/widgets/optimizerflagwidget.h"
#include "src/ui/widgets/modelwidget.h"

#include <QApplication>

#include <QtCore/QJsonObject>
#include <QtCore/QThreadPool>

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>

#include <QDebug>
#include <iostream>

#include "advancedsearch.h"

ParameterWidget::ParameterWidget(const QString &name, QWidget* parent) : QGroupBox(QString(tr("Constant: %1").arg(name)),  parent)
{
    m_min = new QDoubleSpinBox;
    m_min->setValue(1);
    m_max = new QDoubleSpinBox;
    m_max->setValue(7);
    m_step = new QDoubleSpinBox;
    m_step->setValue(0.25);
    
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel(tr("Min")), 0, 0);
    layout->addWidget(m_min, 0, 1);
    layout->addWidget(new QLabel(tr("Step")), 1, 0);
    layout->addWidget(m_step, 1, 1);
    layout->addWidget(new QLabel(tr("Max")), 2, 0);
    layout->addWidget(m_max, 2, 1);
    setLayout(layout);
}

double ParameterWidget::Max() const
{
    return m_max->value();
}

double ParameterWidget::Min() const
{
    return m_min->value();
}

double ParameterWidget::Step() const
{
    return m_step->value();
}



AdvancedSearch::AdvancedSearch(QWidget *parent ) : QDialog(parent), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater))
{
    setModal(false);
    error_max = 0;
}


AdvancedSearch::~AdvancedSearch()
{
}

double AdvancedSearch::MaxX() const
{
    if(m_model->ConstantSize() >= 1)
        return m_parameter_list[0]->Max();
    return 0;
}

double AdvancedSearch::MinX() const
{
    if(m_model->ConstantSize() >= 1)
        return m_parameter_list[0]->Min();
    return 0;
}

double AdvancedSearch::MaxY() const
{
    if(m_model->ConstantSize() >= 2)
        return m_parameter_list[1]->Max();
    return 0;
}

double AdvancedSearch::MinY() const
{
    if(m_model->ConstantSize() >= 2)
        return m_parameter_list[1]->Min();
    return 0;
}

void AdvancedSearch::SetUi()
{
    if(m_model.isNull())
        return;
    m_minimizer.data()->setModel(m_model);
    QVBoxLayout *layout = new QVBoxLayout;
    
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        QPointer<ParameterWidget > widget = new ParameterWidget(m_model->ConstantNames()[i], this);
        layout->addWidget(widget);
        m_parameter_list << widget;
    }
    m_optim_flags = new OptimizerFlagWidget;
    OptimizationType type = static_cast<OptimizationType>(0);
    type = OptimizationType::ComplexationConstants;
    m_optim_flags->DisableOptions(type);
    layout->addWidget(m_optim_flags);
    m_1d_search = new QPushButton(tr("1D Search"));
    m_2d_search = new QPushButton(tr("Create 2D Plot"));
    m_scan = new QPushButton(tr("Scan"));
    connect(m_scan, SIGNAL(clicked()), this, SLOT(GlobalSearch()));
    connect(m_2d_search, SIGNAL(clicked()), this, SLOT(Create2DPlot()));
    connect(m_1d_search, SIGNAL(clicked()), this, SLOT(LocalSearch()));
    
    QGridLayout *mlayout = new QGridLayout;
    mlayout->addLayout(layout, 0, 0, 1, 2);
    mlayout->addWidget(m_optim, 3, 0);
    if(m_model->ConstantSize() == 1)
        mlayout->addWidget(m_1d_search,3, 1);
    mlayout->addWidget(m_scan, 3, 0);
    if(m_model->ConstantSize() == 2)
    {
        mlayout->addWidget(m_2d_search,3, 1);
    }
    setLayout(mlayout);
}

QVector<QVector<double> > AdvancedSearch::ParamList() const
{
    QVector< QVector<double > > full_list;
    for(int i = 0; i < m_parameter_list.size(); ++i)
    {
        QVector<double > list;
        double min = 0, max = 0, step = 0;
        min = m_parameter_list[i]->Min();
        max = m_parameter_list[i]->Max();
        step = m_parameter_list[i]->Step();
        for(double s = min; s <= max; s += step)
            list << s;
        full_list << list;
    }
    return full_list;
}


void AdvancedSearch::LocalSearch()
{
    Waiter wait;
    QVector< QVector<double > > full_list = ParamList();
    
    
    Scan(full_list);
    emit PlotFinished(2);
}


void AdvancedSearch::GlobalSearch()
{
    Waiter wait;
    QVector< QVector<double > > full_list = ParamList();
    m_models_list.clear();
    QVector<double > error; 
    m_type = m_optim_flags->getFlags();   
    m_type |= OptimizationType::ComplexationConstants;
    //      if(m_model->ConstantSize() == 2)
    //      {
    int t0 = QDateTime::currentMSecsSinceEpoch();
    ConvertList(full_list, error);
    qDebug() << m_models_list;
    int t1 = QDateTime::currentMSecsSinceEpoch();
    std::cout << "time for scanning: " << t1-t0 << " msecs." << std::endl;
    emit MultiScanFinished(1);
    //      }  
}

void AdvancedSearch::Create2DPlot()
{
    Waiter wait;
    QVector< QVector<double > > full_list = ParamList();
    
    QVector<double > error; 
    m_type = m_optim_flags->getFlags();   
    if(m_model->ConstantSize() == 2)
    {
        int t0 = QDateTime::currentMSecsSinceEpoch();
        ConvertList(full_list, error);
        int t1 = QDateTime::currentMSecsSinceEpoch();
        std::cout << "time for scanning: " << t1-t0 << " msecs." << std::endl;
        emit PlotFinished(1);
    }
}

void AdvancedSearch::ConvertList(const QVector<QVector<double> >& full_list, QVector<double > &error)
{
    QVector<int > position(full_list.size(), 0);
    QVector< QVector<double > > input;
    int maxthreads =qApp->instance()->property("threads").toInt();
    QVector<QVector<QPointer<NonLinearFitThread> > > threads;
    QVector<QPointer<NonLinearFitThread> > thread_rows;
    QThreadPool::globalInstance()->setMaxThreadCount(maxthreads -1 );
    bool allow_break = false;
    while(!allow_break)
    {
        
        QList<double > parameter; //(full_list.size(), 0);
        for(int j = 0; j < position.size(); ++j)
            parameter << full_list[j][position[j]];
        
        bool temporary = true;
        for(int i = 0; i < position.size(); ++i)
        {
            temporary = temporary && (position[i] == full_list[i].size() - 1);
        }
        if(temporary)
            allow_break = true;
        m_model->setConstants(parameter);
        
        thread_rows << m_minimizer.data()->addJob(m_model, m_type);
        if(m_model->SupportThreads())
            QThreadPool::globalInstance()->waitForDone();
        
        for(int k = position.size() - 1; k >= 0; --k)
        {
            if(position[k] == ( full_list[k].size() - 1) )
            {
                position[k] = 0;
                //                 k--;
                //                                  if(k >= 0)
                //                                  {
                //                     position[k]++;
                if(position[k] <= full_list[k].size() - 1)
                {
                    threads << thread_rows;
                    thread_rows.clear();
                    //                         position[k + 1] = 0;
                }
                //                     break;
                //                                 }
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
    if(!m_model->SupportThreads())
    {
        QThreadPool::globalInstance()->setMaxThreadCount(maxthreads);
        QThreadPool::globalInstance()->waitForDone(-1);
    }
    for(int i = 0; i < threads.size(); ++i)
    {
        QtDataVisualization::QSurfaceDataRow *dataRow1 = new QtDataVisualization::QSurfaceDataRow;
        for(int j = 0; j < threads[i].size(); ++j)
        {
            QList< qreal > parameter = threads[i][j]->Model()->Constants();
            
            QJsonObject json = threads[i][j]->ConvergedParameter();
            m_model->ImportJSON(json);
            m_model->CalculateSignal();
            qDebug() << json;
            double current_error = m_model->ModelError();
            error << current_error; 
            if(error_max < current_error)
                error_max = current_error;
            last_result.m_error = error;
            last_result.m_input = full_list;
            if(m_type & OptimizationType::ComplexationConstants)
                m_models_list << json;
            else
                *dataRow1 << QVector3D(parameter[0], m_model->ModelError(), parameter[1]);
            delete threads[i][j];
        }
        if(!(m_type & OptimizationType::ComplexationConstants))
            m_3d_data << dataRow1;
    }
    return;
    
}


void AdvancedSearch::Scan(const QVector< QVector<double > >& list)
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
            m_model->CalculateSignal();
            error << m_model->ModelError();
            series.append(QPointF(list[j][i],m_model->ModelError( )));
        }
        m_series << series;
    }
}

#include "advancedsearch.moc"
