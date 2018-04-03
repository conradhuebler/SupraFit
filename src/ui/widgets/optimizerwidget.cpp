/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global_config.h"

#include "src/core/AbstractModel.h"

#include "src/ui/widgets/buttons/scientificbox.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "optimizerwidget.h"

OptimizerWidget::OptimizerWidget(OptimizerConfig config, QWidget* parent)
    : QWidget(parent)
    , m_config(config)
{

    setUi();
}

OptimizerWidget::~OptimizerWidget()
{
}
void OptimizerWidget::setUi()
{
    m_tabwidget = new QTabWidget;
    QVBoxLayout* layout = new QVBoxLayout;
    setLayout(layout);
    layout->addWidget(m_tabwidget);

    m_tabwidget->addTab(GeneralWidget(), tr("General Settings"));
    m_tabwidget->addTab(LevmarWidget(), tr("Levmar Settings"));
    m_tabwidget->addTab(AdvancedWidget(), tr("Advanced Settings"));
}

OptimizerConfig OptimizerWidget::Config() const
{
    OptimizerConfig config;

    config.MaxIter = m_maxiter->value();
    config.Constant_Convergence = m_constant_convergence->value();
    config.Error_Convergence = m_error_convergence->value();
    config.single_iter = m_single_iter->value();
    config.skip_not_converged_concentrations = m_skip_corrupt_concentrations->isChecked();
    config.concen_convergency = m_concen_convergency->value();

    config.LevMar_Factor = m_levmar_factor->value();
    config.LevMar_Xtol = m_levmar_eps1->value();
    config.LevMar_Ftol = m_levmar_eps2->value();
    config.LevMar_Gtol = m_levmar_eps3->value();
    config.LevMar_epsfcn = m_levmar_delta->value();

    return config;
}

QWidget* OptimizerWidget::GeneralWidget()
{
    QWidget* widget = new QWidget;
    QGridLayout* layout = new QGridLayout;

    m_maxiter = new QSpinBox;
    m_maxiter->setMaximum(9999999);
    m_maxiter->setValue(m_config.MaxIter);

    m_constant_convergence = new ScientificBox;
    m_constant_convergence->setRange(0, 1);
    m_constant_convergence->setSingleStep(1E-3);
    m_constant_convergence->setDecimals(3);
    m_constant_convergence->setValue(m_config.Constant_Convergence);

    m_error_convergence = new ScientificBox;
    m_error_convergence->setRange(0, 1E-1);
    m_error_convergence->setSingleStep(1E-8);
    m_error_convergence->setDecimals(8);
    m_error_convergence->setValue(m_config.Error_Convergence);

    layout->addWidget(new QLabel(tr("Maximal No. of Iterations")), 0, 0);
    layout->addWidget(m_maxiter, 0, 1);

    layout->addWidget(new QLabel(tr("Tolerance Constants Convergence")), 1, 0);
    layout->addWidget(m_constant_convergence, 1, 1);

    layout->addWidget(new QLabel(tr("Tolerance Error Convergence")), 2, 0);
    layout->addWidget(m_error_convergence, 2, 1);

    widget->setLayout(layout);
    return widget;
}

QWidget* OptimizerWidget::LevmarWidget()
{
    QWidget* widget = new QWidget;
    QGridLayout* layout = new QGridLayout;

    m_levmar_factor = new QSpinBox;
    m_levmar_factor->setRange(0, 1000);
    m_levmar_factor->setSingleStep(10);
    m_levmar_factor->setValue(m_config.LevMar_Factor);

    m_levmar_eps1 = new ScientificBox;
    m_levmar_eps1->setRange(0, 1E-1);
    m_levmar_eps1->setSingleStep(1E-20);
    m_levmar_eps1->setDecimals(20);
    m_levmar_eps1->setValue(m_config.LevMar_Xtol);

    m_levmar_eps2 = new ScientificBox;
    m_levmar_eps2->setRange(0, 1E-1);
    m_levmar_eps2->setSingleStep(1E-20);
    m_levmar_eps2->setDecimals(20);
    m_levmar_eps2->setValue(m_config.LevMar_Ftol);

    m_levmar_eps3 = new ScientificBox;
    m_levmar_eps3->setRange(0, 1E-1);
    m_levmar_eps3->setSingleStep(1E-20);
    m_levmar_eps3->setDecimals(20);
    m_levmar_eps3->setValue(m_config.LevMar_Gtol);

    m_levmar_delta = new ScientificBox;
    m_levmar_delta->setRange(0, 1);
    m_levmar_delta->setSingleStep(1E-10);
    m_levmar_delta->setDecimals(20);
    m_levmar_delta->setValue(m_config.LevMar_epsfcn);

    layout->addWidget(new QLabel(tr("Minipack Factor")), 0, 0);
    layout->addWidget(m_levmar_factor, 0, 1);
    layout->addWidget(new QLabel(tr("Minipack XTol")), 1, 0);
    layout->addWidget(m_levmar_eps1, 1, 1);
    layout->addWidget(new QLabel(tr("Minipack Ftol")), 2, 0);
    layout->addWidget(m_levmar_eps2, 2, 1);
    layout->addWidget(new QLabel(tr("Minipack GTol")), 3, 0);
    layout->addWidget(m_levmar_eps3, 3, 1);
    layout->addWidget(new QLabel(tr("Minipack epsfcn")), 4, 0);
    layout->addWidget(m_levmar_delta, 4, 1);

    widget->setLayout(layout);
    return widget;
}

QWidget* OptimizerWidget::AdvancedWidget()
{
    QWidget* widget = new QWidget;
    QGridLayout* layout = new QGridLayout;

    m_single_iter = new QSpinBox();
    m_single_iter->setMaximum(1e9);
    m_single_iter->setMinimum(30);
    m_single_iter->setValue(m_config.single_iter);

    m_skip_corrupt_concentrations = new QCheckBox(tr("Skip invalid points"));
    m_skip_corrupt_concentrations->setChecked(m_config.skip_not_converged_concentrations);

    m_concen_convergency = new ScientificBox;
    m_concen_convergency->setRange(1E-18, 1E-6);
    m_concen_convergency->setDecimals(13);
    m_concen_convergency->setSingleStep(1E-12);
    m_concen_convergency->setValue(m_config.concen_convergency);

    layout->addWidget(new QLabel(tr("Number of internal iterations")), 1, 0);
    layout->addWidget(m_single_iter, 1, 1);

    layout->addWidget(m_skip_corrupt_concentrations, 2, 0, 1, 2);
    layout->addWidget(new QLabel("Concentration Convergency Threshold"), 3, 0);
    layout->addWidget(m_concen_convergency, 3, 1);

    widget->setLayout(layout);
    return widget;
}

#include "optimizerwidget.moc"
