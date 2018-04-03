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

#include "src/global.h"
#include <QPropertyAnimation>

#include <QtCore/QTimer>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <QDebug>

#include "optimizerflagwidget.h"

OptimizerFlagWidget::OptimizerFlagWidget(QWidget* parent)
    : QWidget(parent)
    , m_type(OptimizationType::GlobalParameter | OptimizationType::LocalParameter)
    , m_hidden(true)
{
    setUi();
    setFlags(m_type);
}

OptimizerFlagWidget::OptimizerFlagWidget(OptimizationType type, QWidget* parent)
    : QWidget(parent)
    , m_hidden(true)
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
    m_globalparameter = new QCheckBox(tr("Global Parameter"));
    m_localparameter = new QCheckBox(tr("Local Parameter"));

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(new QLabel(tr("Define Parameter to be optimised:")));
    layout->addWidget(m_globalparameter);
    layout->addWidget(m_localparameter);
    m_main_layout->addLayout(layout);
    setLayout(m_main_layout);
}

void OptimizerFlagWidget::setFlags(OptimizationType type)
{
    m_type = type;
    m_globalparameter->setChecked((m_type & OptimizationType::GlobalParameter) == OptimizationType::GlobalParameter);
    m_localparameter->setChecked(((m_type & OptimizationType::LocalParameter) == OptimizationType::LocalParameter));
}

void OptimizerFlagWidget::DisableOptions(OptimizationType type)
{
    if (type & OptimizationType::GlobalParameter) {
        m_globalparameter->setChecked(false);
        m_globalparameter->setEnabled(false);
    }
}

OptimizationType OptimizerFlagWidget::getFlags() const
{
    OptimizationType type = static_cast<OptimizationType>(OptimizationType::GlobalParameter | OptimizationType::LocalParameter);

    if (m_globalparameter->isChecked() && m_globalparameter->isEnabled())
        type = type | OptimizationType::GlobalParameter;
    else
        type &= ~OptimizationType::GlobalParameter;

    if (m_localparameter->isChecked())
        type = type | OptimizationType::LocalParameter;
    else
        type &= ~OptimizationType::LocalParameter;

    return type;
}

#include "optimizerflagwidget.moc"
