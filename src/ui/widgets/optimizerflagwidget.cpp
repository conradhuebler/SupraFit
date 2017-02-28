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

#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QCheckBox>

#include <QDebug>

#include "optimizerflagwidget.h"

OptimizerFlagWidget::OptimizerFlagWidget() :m_type(OptimizationType::ComplexationConstants)
{
    setUi();
}

OptimizerFlagWidget::OptimizerFlagWidget(OptimizationType type) : m_type(type)
{
    setUi();
}



OptimizerFlagWidget::~OptimizerFlagWidget()
{
}

void OptimizerFlagWidget::setUi()
{
    m_main_layout = new QVBoxLayout;
    m_main_layout->setAlignment(Qt::AlignTop);
    m_ComplexationConstants = new QCheckBox(tr("Optimize Complexation Constants"));
    m_ComplexationConstants->setChecked((m_type & OptimizationType::ComplexationConstants) == OptimizationType::ComplexationConstants);
    m_OptimizeShifts = new QCheckBox(tr("Optimize Shifts"));
    m_OptimizeShifts->setChecked(!((m_type & OptimizationType::OptimizeShifts) == OptimizationType::OptimizeShifts));
    m_ConstrainedShifts = new QCheckBox(tr("Optimize Shift\nParameters Constrained"));
    m_ConstrainedShifts->setChecked((m_type & OptimizationType::UnconstrainedShifts) != OptimizationType::UnconstrainedShifts);
    m_IntermediateShifts = new QCheckBox(tr("Optimize Nonzero-Concentration and\nNon-saturated Shifts"));
    m_IntermediateShifts->setChecked((m_type & OptimizationType::IntermediateShifts) == OptimizationType::IntermediateShifts);
    m_IgnoreZeroConcentrations = new QCheckBox(tr("Dont Optimize Zero\nConcentration Shifts"));
    m_IgnoreZeroConcentrations->setChecked((m_type & OptimizationType::IgnoreZeroConcentrations) == OptimizationType::IgnoreZeroConcentrations);
    
    m_more = new QPushButton(tr("<"));
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
    EnableShiftSelection();
    ConstrainedChanged();
    m_more->setFlat(true);
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
    
    
}

void OptimizerFlagWidget::ConstrainedChanged()
{
    m_IntermediateShifts->setEnabled(m_ConstrainedShifts->isChecked());
    m_IntermediateShifts->setChecked(m_ConstrainedShifts->isChecked());
    
    m_IgnoreZeroConcentrations->setEnabled(!m_ConstrainedShifts->isChecked());
    m_IgnoreZeroConcentrations->setChecked(!m_ConstrainedShifts->isChecked());
}


void OptimizerFlagWidget::ShowFirst()
{
    bool showing = m_more->isFlat();
    if(!showing)
    {
        QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry");
        QRect geom = geometry();
        geom.setHeight(m_first_row->geometry().height());
        animation->setDuration(200);
        animation->setStartValue(geom);
        geom.setHeight(2*m_first_row->geometry().height());
        animation->setEndValue(geom);
        m_first_row->show();
        animation->start();
        m_more->setText(tr("V"));
        m_more->setFlat(true);
    }else{
        QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry");
        QRect geom = geometry();
        geom.setHeight(2*m_first_row->geometry().height());
        animation->setDuration(200);
        animation->setStartValue(geom);
        geom.setHeight(m_first_row->geometry().height());
        animation->setEndValue(geom);
        animation->start();
        m_first_row->hide();
        m_more->setText(tr("<"));
        m_more->setFlat(false);    
    }
}

#include "optimizerflagwidget.moc"
