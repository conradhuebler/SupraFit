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
#include "src/core/AbstractModel.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

#include "systemparameterwidget.h"

SystemParameterWidget::SystemParameterWidget(const SystemParameter &parameter, QWidget *parent) : m_parameter(parameter), QGroupBox(parent)
{
    m_textfield = new QLineEdit;
    m_boolbox = new QCheckBox;
    
    connect(m_textfield, SIGNAL(textChanged(QString)), this, SIGNAL(valueChanged()));
    connect(m_boolbox, SIGNAL(stateChanged(int)), this, SIGNAL(valueChanged()));
    
    setTitle(parameter.Name());
    
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(new QLabel(parameter.Description()));
    if(parameter.isScalar() || parameter.isString())
        layout->addWidget(m_textfield);
    else if(parameter.isBool())
        layout->addWidget(m_boolbox);
    
    setLayout(layout);
}

SystemParameterWidget::~SystemParameterWidget()
{
    
}

SystemParameter SystemParameterWidget::Value()
{
    if(m_parameter.isScalar() || m_parameter.isString())
         m_parameter.setValue(m_textfield->text() );
    else if(m_parameter.isBool())
         m_parameter.setValue(m_boolbox->isChecked() );
    
    return m_parameter;
}
