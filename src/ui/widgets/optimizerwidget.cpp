/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include <QtWidgets/QLabel>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QCheckBox>
#include "optimizerwidget.h"

OptimizerWidget::OptimizerWidget(OptimizerConfig config, QWidget *parent) : m_config(config), QWidget(parent)
{
    
    setUi();
    
}

OptimizerWidget::~OptimizerWidget()
{
    
    
    
}
void OptimizerWidget::setUi()
{
    m_tabwidget = new QTabWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->addWidget(m_tabwidget);
    
    setGeneral();
    setLevMar();
}


void OptimizerWidget::setGeneral()
{
    QGridLayout *layout = new QGridLayout;
    
    m_maxiter = new QSpinBox;
    m_maxiter->setMaximum(999999);
    m_maxiter->setValue(m_config.MaxIter);
    
    m_sum_convergence = new QSpinBox;
    m_sum_convergence->setMaximum(3);
    m_sum_convergence->setMinimum(1);
    m_sum_convergence->setValue(m_config.Sum_Convergence);
    
    m_shift_convergence = new ScientificBox;
    m_shift_convergence->setRange(0, 1E-1);
    m_shift_convergence->setSingleStep(1E-4);
    m_shift_convergence->setValue(m_config.Shift_Convergence);
  
    m_constant_convergence = new ScientificBox;
    m_constant_convergence->setRange(0, 1E-1);
    m_constant_convergence->setSingleStep(1E-4);
    m_constant_convergence->setValue(m_config.Constant_Convergence);
    
    m_error_convergence = new ScientificBox;
    m_error_convergence->setRange(0, 1E-1);
    m_error_convergence->setSingleStep(1E-8);
    m_error_convergence->setValue(m_config.Error_Convergence);
    
    
    m_levmar_constants_periter = new QSpinBox;
    m_levmar_constants_periter->setMaximum(999999);
    m_levmar_constants_periter->setValue(m_config.LevMar_Constants_PerIter);
    
    m_levmar_shifts_periter = new QSpinBox;
    m_levmar_shifts_periter->setMaximum(999999);
    m_levmar_shifts_periter->setValue(m_config.LevMar_Shifts_PerIter);
    
    m_optimize_shifts = new QCheckBox(tr("Optimize Shifts"));
    m_optimize_shifts->setTristate(true);
    if(!m_config.OptimizeBorderShifts && !m_config.OptimizeIntermediateShifts)
        m_optimize_shifts->stateChanged(Qt::Unchecked);
    else if(m_config.OptimizeBorderShifts && !m_config.OptimizeIntermediateShifts)
        m_optimize_shifts->stateChanged(Qt::PartiallyChecked);
    else
        m_optimize_shifts->stateChanged(Qt::Checked);
    
    layout->addWidget(new QLabel(tr("Maximal No. of Iterations")), 0, 0);
    layout->addWidget(m_maxiter, 0, 1);
    
    layout->addWidget(new QLabel(tr("No. Constraints for Convergence")), 1, 0);
    layout->addWidget(m_sum_convergence, 1, 1);
  
    
    layout->addWidget(new QLabel(tr("Tolerance Constants Convergence")), 3, 0);
    layout->addWidget(m_constant_convergence, 3, 1);
    
    layout->addWidget(new QLabel(tr("Tolerance Error Convergence")), 4, 0);
    layout->addWidget(m_error_convergence, 4, 1);
    
    layout->addWidget(new QLabel(tr("Maximal Levenberg Marquadt Iterations for Constants")), 2, 0);
    layout->addWidget(m_levmar_constants_periter, 2, 1);
    
    layout->addWidget(new QLabel(tr("Optimize Shifts")), 5, 0);
    layout->addWidget(m_optimize_shifts, 5, 1);
    
    layout->addWidget(new QLabel(tr("Maximal Levenberg Marquadt Iterations for Shifts")), 6, 0);
    layout->addWidget(m_levmar_shifts_periter, 6, 1);    
    
    layout->addWidget(new QLabel(tr("Tolerance Shift Convergence")), 7, 0);
    layout->addWidget(m_shift_convergence, 7, 1);


    QWidget *generalwidget = new QWidget;
    generalwidget->setLayout(layout);
    m_tabwidget->addTab(generalwidget, tr("General Settings"));
}


void OptimizerWidget::setLevMar()
{
    QGridLayout *layout = new QGridLayout;
    
    m_levmarmu = new ScientificBox;
    m_levmarmu->setRange(0, 1E-1);
    m_levmarmu->setSingleStep(1E-8);
    m_levmarmu->setValue(m_config.LevMar_mu);
    m_levmarmu->setDecimals(2);

    m_levmar_eps1 = new ScientificBox;
    m_levmar_eps1->setRange(0, 1E-1);
    m_levmar_eps1->setSingleStep(1E-20);
    m_levmar_eps1->setDecimals(2);
    m_levmar_eps1->setValue(m_config.LevMar_Eps1);
    
    m_levmar_eps2 = new ScientificBox;
    m_levmar_eps2->setRange(0, 1E-1);
    m_levmar_eps2->setSingleStep(1E-20);
    m_levmar_eps2->setDecimals(2);
    m_levmar_eps2->setValue(m_config.LevMar_Eps2);
    
    m_levmar_eps3 = new ScientificBox;
    m_levmar_eps3->setRange(0, 1E-1);
    m_levmar_eps3->setSingleStep(1E-20);
    m_levmar_eps3->setDecimals(2);
    m_levmar_eps3->setValue(m_config.LevMar_Eps3);
    
    m_levmar_delta = new ScientificBox;
    m_levmar_delta->setRange(0, 1);
    m_levmar_delta->setSingleStep(1E-10);
    m_levmar_delta->setDecimals(2);
    m_levmar_delta->setValue(m_config.LevMar_Delta);
     
    layout->addWidget(new QLabel(tr("scale factor for initial \\mu {opts[0]}}")), 0, 0);
    layout->addWidget(m_levmarmu, 0, 1);
    layout->addWidget(new QLabel(tr("stopping thresholds for ||J^T e||_inf, \\mu = {opts[1]}")), 1, 0);
    layout->addWidget(m_levmar_eps1, 1, 1);
    layout->addWidget(new QLabel(tr("stopping thresholds for ||Dp||_2 = {opts[2]}")), 2, 0);
    layout->addWidget(m_levmar_eps2, 2, 1);
    layout->addWidget(new QLabel(tr("stopping thresholds for ||e||_2 = {opts[3]}")), 3, 0);
    layout->addWidget(m_levmar_eps3, 3, 1);
    layout->addWidget(new QLabel(tr("tep used in difference approximation to the Jacobian: = {opts[4]}")), 4, 0);
    layout->addWidget(m_levmar_delta, 4, 1);
    
    QWidget *generalwidget = new QWidget;
    generalwidget->setLayout(layout);
    m_tabwidget->addTab(generalwidget, tr("Levmar Settings"));
}

OptimizerConfig OptimizerWidget::Config() const
{
    OptimizerConfig config;
    
    config.MaxIter = m_maxiter->value();
    config.LevMar_Constants_PerIter = m_levmar_constants_periter->value();
    config.LevMar_Shifts_PerIter = m_levmar_shifts_periter->value();
    config.Sum_Convergence = m_sum_convergence->value();
    config.Shift_Convergence = m_shift_convergence->value();
    config.Constant_Convergence = m_constant_convergence->value();
    config.Error_Convergence = m_error_convergence->value();
    
    if(m_optimize_shifts->checkState() == Qt::Unchecked)
    {
        config.OptimizeBorderShifts = 0;
        config.OptimizeIntermediateShifts = 0;
    }else if(m_optimize_shifts->checkState() == Qt::PartiallyChecked)
    {
        config.OptimizeBorderShifts = 1;
        config.OptimizeIntermediateShifts = 0;       
    }else
    {
        config.OptimizeBorderShifts = 1;
        config.OptimizeIntermediateShifts = 1;
    }
    config.LevMar_mu = m_levmarmu->value();
    config.LevMar_Eps1 = m_levmar_eps1->value();
    config.LevMar_Eps2 = m_levmar_eps2->value();
    config.LevMar_Eps3 = m_levmar_eps3->value();
    config.LevMar_Delta = m_levmar_delta->value();
    
    return config;
}









#include "optimizerwidget.moc"
