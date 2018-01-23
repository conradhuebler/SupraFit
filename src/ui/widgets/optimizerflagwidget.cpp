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

OptimizerFlagWidget::OptimizerFlagWidget(QWidget *parent) :QWidget(parent), m_type(OptimizationType::ComplexationConstants | OptimizationType::OptimizeShifts), m_hidden(true)
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
    m_IgnoreZeroConcentrations = new QCheckBox(tr("Skip Host Shift"));
    
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_ComplexationConstants);
    layout->addWidget(m_OptimizeShifts);
    layout->addWidget(m_IgnoreZeroConcentrations);
    m_main_layout->addLayout(layout);
    setLayout(m_main_layout);

}

void OptimizerFlagWidget::setFlags(OptimizationType type)
{
    m_type = type;
    m_ComplexationConstants->setChecked((m_type & OptimizationType::ComplexationConstants) == OptimizationType::ComplexationConstants);
    m_OptimizeShifts->setChecked(((m_type & OptimizationType::OptimizeShifts) == OptimizationType::OptimizeShifts));
    m_IgnoreZeroConcentrations->setChecked((m_type & OptimizationType::IgnoreZeroConcentrations) == OptimizationType::IgnoreZeroConcentrations);
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
    OptimizationType type = static_cast<OptimizationType>(OptimizationType::ComplexationConstants | OptimizationType::OptimizeShifts);
    
    if(m_ComplexationConstants->isChecked() && m_ComplexationConstants->isEnabled())
        type = type | OptimizationType::ComplexationConstants;
    else 
        type &= ~OptimizationType::ComplexationConstants;
    
    if(m_OptimizeShifts->isChecked())
        type = type | OptimizationType::OptimizeShifts;
    else
        type &= ~OptimizationType::OptimizeShifts;

    if(m_IgnoreZeroConcentrations->isChecked())
        type = type | OptimizationType::IgnoreZeroConcentrations;
    else
        type &= ~OptimizationType::IgnoreZeroConcentrations;
        
    return type;
}

#include "optimizerflagwidget.moc"
