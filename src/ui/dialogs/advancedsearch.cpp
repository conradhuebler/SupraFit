/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/toolset.h"

#include "src/ui/widgets/optimizerflagwidget.h"
#include "src/ui/guitools/waiter.h"

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
#include <QtCore/QPointer>

#include <QDebug>
#include <iostream>

#include "advancedsearch.h"

ParameterWidget::ParameterWidget(const QString &name,  qreal value, QWidget* parent) : QGroupBox(parent)
{
    qreal div = value/5.0;
    qreal prod = value*5;
    m_min = new QDoubleSpinBox;
    m_min->setMinimum(-1e8);
    m_min->setMaximum(1e8);
    m_min->setValue(ToolSet::floor(qMin(div,prod)));

    m_max = new QDoubleSpinBox;
    m_max->setMinimum(-1e8);
    m_max->setMaximum(1e8);
    m_max->setValue(ToolSet::ceil(qMax(div, prod)));

    m_step = new QDoubleSpinBox;
    m_step->setMinimum(0.01);
    m_step->setMaximum(1e8);
    m_step->setValue(ToolSet::ceil((m_max->value()-m_min->value())/10));

    connect(m_min, SIGNAL(valueChanged(double)), this, SIGNAL(valueChanged()));
    connect(m_max, SIGNAL(valueChanged(double)), this, SIGNAL(valueChanged()));
    connect(m_step, SIGNAL(valueChanged(double)), this, SIGNAL(valueChanged()));
    
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel(tr("<h4>Parameter: %1</h4>").arg(name)), 0, 0, 1, 6);

    layout->addWidget(new QLabel(tr("Start:")), 1, 0);
    layout->addWidget(m_min, 1, 1);
    layout->addWidget(new QLabel(tr("Step")), 1, 3);
    layout->addWidget(m_step, 1, 4);
    layout->addWidget(new QLabel(tr("End:")), 1, 5);
    layout->addWidget(m_max, 1, 6);
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
    m_error_max = 0;
    setWindowTitle("Global Search Dialog"); 
}


AdvancedSearch::~AdvancedSearch()
{
}

double AdvancedSearch::MaxX() const
{
    if(m_model->GlobalParameterSize() >= 1)
        return m_parameter_list[0]->Max();
    return 0;
}

double AdvancedSearch::MinX() const
{
    if(m_model->GlobalParameterSize() >= 1)
        return m_parameter_list[0]->Min();
    return 0;
}

double AdvancedSearch::MaxY() const
{
    if(m_model->GlobalParameterSize() >= 2)
        return m_parameter_list[1]->Max();
    return 0;
}

double AdvancedSearch::MinY() const
{
    if(m_model->GlobalParameterSize() >= 2)
        return m_parameter_list[1]->Min();
    return 0;
}

void AdvancedSearch::SetUi()
{
    if(m_model.isNull())
        return;
    
    m_search = new GlobalSearch(this);
    m_search->setModel(m_model);
    
    QVBoxLayout *layout = new QVBoxLayout;
    
    for(int i = 0; i < m_model->GlobalParameterSize(); ++i)
    {
        QPointer<ParameterWidget > widget = new ParameterWidget(m_model->GlobalParameterName(i), m_model->GlobalParameter(i), this);
        layout->addWidget(widget);
        connect(widget, SIGNAL(valueChanged()), this, SLOT(MaxSteps()));
        m_parameter_list << widget;
    } 

    if(!m_model->SupportSeries())
    {
        for(int i = 0; i < m_model->LocalParameterSize(); ++i)
        {
            QPointer<ParameterWidget > widget = new ParameterWidget(m_model->LocalParameterName(i), m_model->LocalParameter(i, 0), this);
            layout->addWidget(widget);
            connect(widget, SIGNAL(valueChanged()), this, SLOT(MaxSteps()));
            m_parameter_list << widget;
        }
    }

    m_initial_guess = new QCheckBox(tr("Apply initial Guess"));
    m_initial_guess->setChecked(true);
    connect(m_initial_guess, SIGNAL(stateChanged(int)), this, SLOT(setOptions()));
    m_optim = new QCheckBox(tr("Optimise"));
    m_optim->setChecked(true);
    connect(m_optim, SIGNAL(stateChanged(int)), this, SLOT(setOptions()));
    QHBoxLayout *options = new QHBoxLayout;
    
    options->addWidget(m_optim);
    options->addWidget(m_initial_guess);
    layout->addLayout(options);
    
    m_optim_flags = new OptimizerFlagWidget;
    OptimizationType type = static_cast<OptimizationType>(0);
    type = OptimizationType::GlobalParameter;
    m_optim_flags->DisableOptions(type);
    layout->addWidget(m_optim_flags);
    

    
    m_1d_search = new QPushButton(tr("1D Search"));
    m_2d_search = new QPushButton(tr("Create 2D Plot"));
    m_scan = new QPushButton(tr("Scan"));
    m_interrupt = new QPushButton(tr("Interrupt"));
    
    connect(m_scan, SIGNAL(clicked()), this, SLOT(SearchGlobal()));
    connect(m_2d_search, SIGNAL(clicked()), this, SLOT(Create2DPlot()));
    connect(m_1d_search, SIGNAL(clicked()), this, SLOT(LocalSearch()));
    connect(m_interrupt, SIGNAL(clicked()), m_search, SLOT(Interrupt()), Qt::DirectConnection);
    m_progress = new QProgressBar;
    m_max_steps = new QLabel;
    QGridLayout *mlayout = new QGridLayout;
    mlayout->addLayout(layout, 0, 0, 1, 2);
    mlayout->addWidget(m_max_steps, 1, 0, 1, 2);
    mlayout->addWidget(m_progress, 2, 0, 1, 2);
    mlayout->addWidget(m_optim, 3, 0);
    if(m_model->GlobalParameterSize() == 1)
        mlayout->addWidget(m_1d_search,3, 1);
    mlayout->addWidget(m_scan, 3, 0);
    mlayout->addWidget(m_interrupt, 3, 0);
    if(m_model->GlobalParameterSize() == 2)
    {
        mlayout->addWidget(m_2d_search,3, 1);
    }
    
//     m_progress->hide();
    m_interrupt->hide();
    connect(m_search, SIGNAL(IncrementProgress(int)), this, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(m_search, SIGNAL(setMaximumSteps(int)), m_progress, SLOT(setMaximum(int)));
    connect(this, SIGNAL(setValue(int)), m_progress, SLOT(setValue(int)));
    
    setLayout(mlayout);
    MaxSteps();
    setOptions();
}

void AdvancedSearch::setOptions()
{
    m_optim_flags->setEnabled(m_optim->isChecked());
}

void AdvancedSearch::MaxSteps()
{
    m_parameter.clear();
    int max_count = 1;
    for(int i = 0; i < m_parameter_list.size(); ++i)
    {
        double min = 0, max = 0, step = 0;
        min = m_parameter_list[i]->Min();
        max = m_parameter_list[i]->Max();
        step = m_parameter_list[i]->Step();
        m_parameter.append( QVector<qreal>() << min << max << step );
        max_count *= (max+step-min)/step;
    }
    m_max_steps->setText(tr("No of calculations to be done: %1").arg(max_count));
}

void AdvancedSearch::PrepareProgress()
{
    m_scan->hide();
    m_interrupt->show();
    m_time_0 =  QDateTime::currentMSecsSinceEpoch();
    m_time = 0;
    m_progress->setValue(0);
//     m_progress->show();   
}

void AdvancedSearch::Finished()
{
//     m_progress->hide();   
     m_scan->show();
     m_interrupt->hide();
}

void AdvancedSearch::SearchGlobal()
{
     Waiter wait;
     PrepareProgress();
     m_search->setConfig(Config());
     m_models_list.clear();
     QVector<double > error; 
     m_models_list = m_search->SearchGlobal();
//      m_full_list = m_search->Inputlist();
     Finished();
     emit MultiScanFinished();
}

void AdvancedSearch::LocalSearch()
{
     Waiter wait;
     PrepareProgress();
     m_search->setConfig(Config());
     m_series = m_search->LocalSearch();
     Finished();
     emit PlotFinished(2);
}

void AdvancedSearch::Create2DPlot()
{
     Waiter wait;
     PrepareProgress();
     m_search->setConfig(Config());
     QVector<VisualData> data_list = m_search->Create2DPLot();
     m_error_max = 0;
     for(const VisualData &data : data_list)
     {
         QtDataVisualization::QSurfaceDataRow *dataRow1 = new QtDataVisualization::QSurfaceDataRow;
         for(int i = 0; i < data.data.size(); ++i)
         {
             *dataRow1 << QVector3D(data.data[i][0],data.data[i][1],data.data[i][2]);
             m_error_max = qMax(m_error_max, data.data[i][1]);
         }
        m_3d_data << dataRow1;
     }
     Finished();
     emit PlotFinished(1);
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
    emit setValue(val);
}


GSConfig AdvancedSearch::Config() const
{
    GSConfig config;
    config.parameter = m_parameter;
    config.runtype = m_optim_flags->getFlags();
    config.initial_guess = m_initial_guess->isChecked();
    config.optimize = m_optim->isChecked();
    return config;
}


#include "advancedsearch.moc"
