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

#include "src/capabilities/continuousvariation.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/core/models.h"

#include "src/ui/widgets/optimizerflagwidget.h"
#include "src/ui/widgets/modelwidget.h"

#include <QtCore/QMutexLocker>

#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QProgressBar>

#include "statisticdialog.h"
StatisticDialog::StatisticDialog(QSharedPointer<AbstractTitrationModel> model, QWidget *parent) : QDialog(parent), m_model(model)
{
    setUi();
//     Pending();
//     connect(this, SIGNAL(Interrupt()), this, SLOT(Pending()));
    connect(m_model.data(), SIGNAL(Recalculated()), this, SLOT(Update()));
}


StatisticDialog::~StatisticDialog()
{
    
    
    
}

void StatisticDialog::setUi()
{
    QVBoxLayout *layout = new QVBoxLayout;
    QTabWidget *widget = new QTabWidget;
    widget->addTab(ContinuousVariationWidget(), tr("Continuous Variation"));
    widget->addTab(MonteCarloWidget(), tr("Monte Carlo"));
    m_optim_flags = new OptimizerFlagWidget; 
    layout->addWidget(widget);
    m_time_info = new QLabel;
    m_progress = new QProgressBar;
    layout->addWidget(m_time_info);
    
    
    m_hide = new QPushButton(tr("Hide"));
    m_interrupt = new QPushButton(tr("Interrupt"));
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(m_progress);
    hLayout->addWidget(m_interrupt);
    layout->addLayout(hLayout);
    layout->addWidget(m_hide);
    connect(m_hide, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_interrupt, SIGNAL(clicked()), this, SIGNAL(Interrupt()));
    setLayout(layout);
}

QWidget *StatisticDialog::MonteCarloWidget()
{
    QWidget *mc_widget = new QWidget;
    QGridLayout *layout = new QGridLayout;
    
    m_mc_steps = new QSpinBox;
    m_mc_steps->setMinimum(1);
    m_mc_steps->setMaximum(1e9);
    m_mc_steps->setValue(1000);
    m_mc_steps->setSingleStep(1e2);
    layout->addWidget(new QLabel(tr("Number of MC Steps:")), 0, 0);
    layout->addWidget(m_mc_steps, 0, 1);
    
    m_varianz_box = new QDoubleSpinBox;
    m_varianz_box->setDecimals(5);
    m_varianz_box->setSingleStep(1e-2);
    m_varianz_box->setValue(m_model.data()->StdDeviation());
    layout->addWidget(new QLabel(tr("Varianz")), 1, 0);
    layout->addWidget(m_varianz_box, 1, 1);
    
    m_original = new QCheckBox;
    m_original->setText(tr("Use Original Data"));
    layout->addWidget(m_original, 2, 0, 1, 2);
    
    m_bootstrap = new QCheckBox;
    m_bootstrap->setText(tr("Bootstrap"));
    layout->addWidget(m_bootstrap, 3, 0, 1, 2);
    
    m_mc = new QPushButton(tr("Simulate"));
    layout->addWidget(m_mc, 4, 0, 1, 2);
    
    connect(m_mc, SIGNAL(clicked()), this, SIGNAL(MCStatistic()));
    mc_widget->setLayout(layout);
    return mc_widget;
}


QWidget * StatisticDialog::ContinuousVariationWidget()
{
    QWidget *cv_widget = new QWidget;
    QGridLayout *layout = new QGridLayout;   
    
    m_cv_increment = new QDoubleSpinBox;
    m_cv_increment->setSingleStep(1e-3);
    m_cv_increment->setValue(0.01);
    m_cv_increment->setDecimals(6);
    layout->addWidget(new QLabel(tr("Increment")), 0, 0);
    layout->addWidget(m_cv_increment, 0, 1);
    
    m_cv_steps = new QSpinBox;
    m_cv_steps->setValue(1000);
    m_cv_steps->setMaximum(1e7);
    m_cv_steps->setSingleStep(100);
    
    layout->addWidget(new QLabel(tr("Max. Steps")), 1, 0);
    layout->addWidget(m_cv_steps, 1, 1);
    
    m_cv = new QPushButton(tr("Calculate"));
    layout->addWidget(m_cv, 2, 0, 1, 2);
    
    connect(m_cv, SIGNAL(clicked()), this, SIGNAL(CVStatistic()));
    cv_widget->setLayout(layout);
    return cv_widget;
}

CVConfig StatisticDialog::getCVConfig()
{
    CVConfig config;
    config.increment = m_cv_increment->value();
    config.maxsteps = m_cv_steps->value();
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setMaximum(-1);
    m_progress->setValue(0);
//     Pending();
    return config;
}

MCConfig StatisticDialog::getMCConfig()
{
    MCConfig config;
    config.variance = m_varianz_box->value();
    config.maxsteps = m_mc_steps->value();
    config.original = m_original->isChecked();
    config.bootstrap = m_bootstrap->isChecked();
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setMaximum(m_mc_steps->value() + m_mc_steps->value()/100);
    m_progress->setValue(0);
//     Pending();
    return config;
}
void StatisticDialog::IncrementProgress(int time)
{
    QMutexLocker locker(&mutex);
    
    m_time += time;
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    int val = m_progress->value() + 1;
    qreal aver = double(m_time)/val;
    int remain = double(m_progress->maximum() - val)*aver/qAbs(QThreadPool::globalInstance()->maxThreadCount())/1000;
    int used = (t0 - m_time_0)/1000;
    
    if(m_progress->maximum() == -1)
    {
        m_time_info->setText(tr("Nobody knows when this will end.\nBut you hold the door for %2 sec. .").arg(used));
    }else{
        m_time_info->setText(tr("Remaining time approx: %1 sec., elapsed time: %2 sec. .").arg(remain).arg(used));
        m_progress->setValue(val);
    }
    
}

void StatisticDialog::Pending()
{
    bool ishidden = m_progress->isHidden();
    if(ishidden)
    {
        m_interrupt->show();
        m_progress->show();
    }else
    {
        m_interrupt->hide();
        m_progress->hide();
    }
}

void StatisticDialog::Update()
{
    m_varianz_box->setValue(m_model.data()->StdDeviation());
}

#include "statisticdialog.moc"
