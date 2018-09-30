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
#include "src/core/AbstractModel.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

#include "systemparameterwidget.h"

SystemParameterWidget::SystemParameterWidget(const SystemParameter& parameter, bool readonly, QWidget* parent)
    : QGroupBox(parent)
    , m_parameter(parameter)
    , m_change(false)
    , m_readonly(readonly)

{
    m_textfield = new QLineEdit;
    // m_textfield->setReadOnly(m_readonly);
    m_boolbox = new QCheckBox;
    // m_boolbox->setDisabled(m_readonly);
    m_list = new QComboBox;
    m_list->setItemDelegate(new HTMLDelegate(this));
    // m_list->setDisabled(m_readonly);

    QLabel* label = new QLabel(parameter.Description());
    label->setFixedWidth(250);
    connect(m_textfield, SIGNAL(textChanged(QString)), this, SLOT(PrepareChanged()));
    connect(m_boolbox, SIGNAL(stateChanged(int)), this, SLOT(PrepareChanged()));
    connect(m_list, SIGNAL(currentIndexChanged(int)), this, SLOT(PrepareChanged()));

    setTitle(parameter.Name());

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(label);
    if (parameter.isScalar() || parameter.isString())
        layout->addWidget(m_textfield);
    else if (parameter.isBool())
        layout->addWidget(m_boolbox);

    else if (parameter.isList()) {
        layout->addWidget(m_list);
        m_list->addItems(parameter.getList());
    }

    if (parameter.isBool())
        m_boolbox->setChecked(parameter.Bool());
    else
        m_textfield->setText(parameter.value().toString());
    setLayout(layout);
}

SystemParameterWidget::~SystemParameterWidget()
{
}

SystemParameter SystemParameterWidget::Value()
{
    if (m_parameter.isScalar() || m_parameter.isString())
        m_parameter.setValue(m_textfield->text().replace(",", "."));
    else if (m_parameter.isBool())
        m_parameter.setValue(m_boolbox->isChecked());
    else if (m_parameter.isList())
        m_parameter.setValue(m_list->currentText());
    return m_parameter;
}

void SystemParameterWidget::setValue(const SystemParameter& parameter)
{
    m_change = true;
    QVariant variant = parameter.value();
    if (m_parameter.isScalar() || m_parameter.isString())
        m_textfield->setText(variant.toString());
    else if (m_parameter.isBool())
        m_boolbox->setChecked(variant.toBool());
    else if (m_parameter.isList())
        m_list->setCurrentText(variant.toString());
    m_change = false;
}

void SystemParameterWidget::PrepareChanged()
{
    if (!m_change)
        emit valueChanged();
}
