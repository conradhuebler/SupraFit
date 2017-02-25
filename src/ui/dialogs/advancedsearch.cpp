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

#include "src/capabilities/globalsearch.h"

#include "src/core/AbstractModel.h"
#include "src/core/minimizer.h"
#include "src/ui/widgets/optimizerflagwidget.h"
#include "src/ui/widgets/modelwidget.h"

#include <QApplication>

#include <QtCore/QJsonObject>
#include <QtCore/QThreadPool>
#include <QtCore/QMutexLocker>

#include <QtWidgets/QProgressBar>
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
    connect(m_min, SIGNAL(valueChanged(double)), this, SIGNAL(valueChanged()));
    connect(m_max, SIGNAL(valueChanged(double)), this, SIGNAL(valueChanged()));
    connect(m_step, SIGNAL(valueChanged(double)), this, SIGNAL(valueChanged()));
    
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



AdvancedSearch::AdvancedSearch(QWidget *parent ) : QDialog(parent)
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

    QVBoxLayout *layout = new QVBoxLayout;
    
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        QPointer<ParameterWidget > widget = new ParameterWidget(m_model->ConstantNames()[i], this);
        layout->addWidget(widget);
        connect(widget, SIGNAL(valueChanged()), this, SLOT(MaxSteps()));
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
    
    m_progress = new QProgressBar;
    m_max_steps = new QLabel;
    QGridLayout *mlayout = new QGridLayout;
    mlayout->addLayout(layout, 0, 0, 1, 2);
    mlayout->addWidget(m_max_steps, 1, 0, 1, 2);
    mlayout->addWidget(m_progress, 2, 0, 1, 2);
    mlayout->addWidget(m_optim, 3, 0);
    if(m_model->ConstantSize() == 1)
        mlayout->addWidget(m_1d_search,3, 1);
    mlayout->addWidget(m_scan, 3, 0);
    if(m_model->ConstantSize() == 2)
    {
        mlayout->addWidget(m_2d_search,3, 1);
    }
    
    m_progress->hide();
    
    setLayout(mlayout);
    MaxSteps();
}

void AdvancedSearch::MaxSteps()
{
    int max_count = 1;
    for(int i = 0; i < m_parameter_list.size(); ++i)
    {
        double min = 0, max = 0, step = 0;
        min = m_parameter_list[i]->Min();
        max = m_parameter_list[i]->Max();
        step = m_parameter_list[i]->Step();
        max_count *= (max+step-min)/step;
    }
    m_max_steps->setText(tr("No of calculations to be done: %1").arg(max_count));
}

void AdvancedSearch::GlobalSearch()
{
//     Waiter wait;
//     QVector< QVector<double > > full_list = ParamList();
//     m_models_list.clear();
//     QVector<double > error; 
//     m_type = m_optim_flags->getFlags();   
//     m_type |= OptimizationType::ComplexationConstants;
//     int t0 = QDateTime::currentMSecsSinceEpoch();
//     ConvertList(full_list, error);
//     int t1 = QDateTime::currentMSecsSinceEpoch();
//     std::cout << "time for scanning: " << t1-t0 << " msecs." << std::endl;
//     emit MultiScanFinished(1);
  
}

void AdvancedSearch::LocalSearch()
{
//     Waiter wait;
//     QVector< QVector<double > > full_list = ParamList();
//     
//     
//     Scan(full_list);
//     emit PlotFinished(2);
}

void AdvancedSearch::Create2DPlot()
{
//     Waiter wait;
//     QVector< QVector<double > > full_list = ParamList();
//     
//     QVector<double > error; 
//     m_type = m_optim_flags->getFlags();   
//     if(m_model->ConstantSize() == 2)
//     {
//         int t0 = QDateTime::currentMSecsSinceEpoch();
//         ConvertList(full_list, error);
//         int t1 = QDateTime::currentMSecsSinceEpoch();
//         std::cout << "time for scanning: " << t1-t0 << " msecs." << std::endl;
//         emit PlotFinished(1);
//     }
}



void AdvancedSearch::IncrementProgress(int time)
{
    QMutexLocker locker(&mutex);
    m_time += time;
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    int val = m_progress->value() + 1;
    qreal aver = double(m_time)/val;
    int remain = double(m_progress->maximum() - val)*aver/3000;
    int used = (t0 - m_time_0)/1000;
    m_max_steps->setText(tr("Remaining time approx: %1 sec., elapsed time: %2 sec. .").arg(remain).arg(used));
    m_progress->setValue(val);
}


#include "advancedsearch.moc"
