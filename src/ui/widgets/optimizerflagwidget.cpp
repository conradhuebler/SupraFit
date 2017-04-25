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

#include "src/global.h"
#include <QPropertyAnimation>

#include <QtCore/QTimer>

#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QCheckBox>

#include <QDebug>

#include "optimizerflagwidget.h"

OptimizerFlagWidget::OptimizerFlagWidget(QWidget *parent) :QWidget(parent), m_type(OptimizationType::ComplexationConstants | OptimizationType::OptimizeShifts | OptimizationType::UnconstrainedShifts), m_hidden(true)
{
     setUi();
     setFlags(m_type);

}

OptimizerFlagWidget::OptimizerFlagWidget(OptimizationType type, QWidget *parent) :QWidget(parent), m_hidden(true)
{
     setUi();
     setFlags(type);
}



OptimizerFlagWidget::~OptimizerFlagWidget()
{
}

void OptimizerFlagWidget::setUi()
{
    m_main_layout = new QVBoxLayout;
    m_main_layout->setAlignment(Qt::AlignTop);
    m_ComplexationConstants = new QCheckBox(tr("Complexation Constants"));
    
    m_OptimizeShifts = new QCheckBox(tr("Shifts"));
    m_ConstrainedShifts = new QCheckBox(tr("Optimise Shift Constrained"));
    m_IntermediateShifts = new QCheckBox(tr("Fit remaining Shifts"));
    m_IgnoreZeroConcentrations = new QCheckBox(tr("Skip Host Shift"));
    
    m_more = new QPushButton(tr("..more.."));
    m_more->setMaximumSize(50,30);
    connect(m_more, SIGNAL(clicked()), this, SLOT(ShowFirst()));
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_ComplexationConstants);
    layout->addWidget(m_OptimizeShifts);
    layout->addStretch(width()/2);
    layout->addWidget(m_more);
    m_main_layout->addLayout(layout);
    layout = new QHBoxLayout;
    layout->addWidget(m_ConstrainedShifts);
    layout->addWidget(m_IntermediateShifts);
    layout->addWidget(m_IgnoreZeroConcentrations);
    m_first_row = new QWidget;
    m_first_row->setLayout(layout);
    m_main_layout->addWidget(m_first_row);
    setLayout(m_main_layout);
    connect(m_OptimizeShifts, SIGNAL(stateChanged(int)), this, SLOT(EnableShiftSelection()));
    connect(m_ConstrainedShifts, SIGNAL(stateChanged(int)), this, SLOT(ConstrainedChanged()));

    m_more->setFlat(true);
    m_first_row->setMaximumHeight(0);
}

void OptimizerFlagWidget::setFlags(OptimizationType type)
{
    m_type = type;
    m_ComplexationConstants->setChecked((m_type & OptimizationType::ComplexationConstants) == OptimizationType::ComplexationConstants);
    m_OptimizeShifts->setChecked(((m_type & OptimizationType::OptimizeShifts) == OptimizationType::OptimizeShifts));
    m_ConstrainedShifts->setChecked((m_type & OptimizationType::UnconstrainedShifts) != OptimizationType::UnconstrainedShifts);
    m_IntermediateShifts->setChecked((m_type & OptimizationType::IntermediateShifts) == OptimizationType::IntermediateShifts);
    m_IgnoreZeroConcentrations->setChecked((m_type & OptimizationType::IgnoreZeroConcentrations) == OptimizationType::IgnoreZeroConcentrations);
    EnableShiftSelection();
}


void OptimizerFlagWidget::DisableOptions(OptimizationType type)
{
    if(type & OptimizationType::ComplexationConstants)
    {
        m_ComplexationConstants->setChecked(false);
        m_ComplexationConstants->setEnabled(false);
    }
}


OptimizationType OptimizerFlagWidget::getFlags() const
{
    OptimizationType type = static_cast<OptimizationType>(OptimizationType::ComplexationConstants | OptimizationType::UnconstrainedShifts | OptimizationType::OptimizeShifts);
    
    if(m_ComplexationConstants->isChecked() && m_ComplexationConstants->isEnabled())
        type = type | OptimizationType::ComplexationConstants;
    else 
        type &= ~OptimizationType::ComplexationConstants;
    if(m_OptimizeShifts->isChecked())
    {
         if(!m_ConstrainedShifts->isChecked())
         {
            type = type | OptimizationType::UnconstrainedShifts;  
            type = type & ~OptimizationType::ConstrainedShifts;  
             if(m_IgnoreZeroConcentrations->isChecked())
                 type = type | OptimizationType::IgnoreZeroConcentrations;
         }
         else
         {
             if(m_IntermediateShifts->isChecked())
                 type = type | OptimizationType::IntermediateShifts; 
            type = type &  ~OptimizationType::UnconstrainedShifts;  
            type = type | OptimizationType::ConstrainedShifts;
         }
    }else
    {
        type &= ~OptimizationType::OptimizeShifts;  
    }
    return type;
}

void OptimizerFlagWidget::EnableShiftSelection()
{
    m_ConstrainedShifts->setEnabled(m_OptimizeShifts->isChecked());
    m_IntermediateShifts->setEnabled(m_OptimizeShifts->isChecked());
    m_IgnoreZeroConcentrations->setEnabled(m_OptimizeShifts->isChecked());
    ConstrainedChanged();
}

void OptimizerFlagWidget::ConstrainedChanged()
{
    m_IntermediateShifts->setEnabled(m_ConstrainedShifts->isChecked());
    m_IntermediateShifts->setChecked(m_ConstrainedShifts->isChecked());
    
    m_IgnoreZeroConcentrations->setEnabled(!m_ConstrainedShifts->isChecked() && m_OptimizeShifts->isChecked());
    m_IgnoreZeroConcentrations->setChecked(!m_ConstrainedShifts->isChecked() && ((m_type & OptimizationType::IgnoreZeroConcentrations) == OptimizationType::IgnoreZeroConcentrations));
}


void OptimizerFlagWidget::ShowFirst()
{
    
    if(!m_hidden)
    {
        QPropertyAnimation *animation = new QPropertyAnimation(m_first_row, "maximumHeight");
        animation->setEasingCurve(QEasingCurve::InOutCubic);
        animation->setDuration(200);
        animation->setStartValue(100);
        animation->setEndValue(0);
        animation->start();
        m_more->setText(tr("..more.."));
        m_hidden = true;
    }else{
        QPropertyAnimation *animation = new QPropertyAnimation(m_first_row, "maximumHeight");
        
        animation->setEasingCurve(QEasingCurve::InOutCubic);
        animation->setDuration(200);         
        animation->setStartValue(0);
        animation->setEndValue(100);
        animation->start();
        m_more->setText(tr("..less.."));
        m_hidden = false;
    }
}

#include "optimizerflagwidget.moc"
