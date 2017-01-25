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
    m_ComplexationConstants = new QCheckBox(tr("Optimize\nComplexation\nConstants"));
    m_ComplexationConstants->setChecked(m_type & OptimizationType::ComplexationConstants);
    m_IgnoreAllShifts = new QCheckBox(tr("Ignore\nShifts"));
    m_IgnoreAllShifts->setChecked(m_type & OptimizationType::IgnoreAllShifts);
    m_ConstrainedShifts = new QCheckBox(tr("Optimize Shift\nParameters\nConstrained"));
    m_ConstrainedShifts->setChecked(m_type & OptimizationType::ConstrainedShifts);
    m_IntermediateShifts = new QCheckBox(tr("Optimize Nonzero-Concentration\nand\nNon-saturated Shifts"));
    m_IntermediateShifts->setChecked(m_type & OptimizationType::IntermediateShifts);
    m_IgnoreZeroConcentrations = new QCheckBox(tr("Dont Optimize Zero\nConcentration Shifts"));
    m_IgnoreZeroConcentrations->setChecked(m_type & OptimizationType::IgnoreZeroConcentrations);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_ComplexationConstants);
    layout->addWidget(m_IgnoreAllShifts);
    layout->addWidget(m_ConstrainedShifts);
    layout->addWidget(m_IntermediateShifts);
    layout->addWidget(m_IgnoreZeroConcentrations);
    setLayout(layout);
    connect(m_IgnoreAllShifts, SIGNAL(stateChanged(int)), this, SLOT(EnableShiftSelection()));
    connect(m_ConstrainedShifts, SIGNAL(stateChanged(int)), this, SLOT(ConstrainedChanged()));
    EnableShiftSelection();
    ConstrainedChanged();
}

OptimizationType OptimizerFlagWidget::getFlags() const
{
    OptimizationType type = static_cast<OptimizationType>(0);
    
    if(m_ComplexationConstants->isChecked())
        type = OptimizationType::ComplexationConstants;

    if(!m_IgnoreAllShifts->isChecked())
    {
        if(m_ConstrainedShifts->isChecked())
            type = type | OptimizationType::ConstrainedShifts;   
        else
            type = type | OptimizationType::UnconstrainedShifts;  
        
        if(m_IntermediateShifts->isChecked())
            type = type | OptimizationType::IntermediateShifts;   
        
        if(m_IgnoreZeroConcentrations->isChecked())
            type = type | OptimizationType::IgnoreZeroConcentrations;   
    }
    
    return type;
}

void OptimizerFlagWidget::EnableShiftSelection()
{
    m_ConstrainedShifts->setEnabled(!m_IgnoreAllShifts->isChecked());
    m_IntermediateShifts->setEnabled(!m_IgnoreAllShifts->isChecked());
    m_IgnoreZeroConcentrations->setEnabled(!m_IgnoreAllShifts->isChecked());
}

void OptimizerFlagWidget::ConstrainedChanged()
{
    m_IgnoreZeroConcentrations->setEnabled(m_ConstrainedShifts->isChecked());
}


#include "optimizerflagwidget.moc"
