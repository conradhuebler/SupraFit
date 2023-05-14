/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/AbstractModel.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QScrollArea>

#include "systemparameterwidget.h"

SystemParameterWidget::SystemParameterWidget(const SystemParameter& parameter, bool readonly, QWidget* parent)
    : QGroupBox(parent)
    , m_parameter(parameter)
    , m_change(false)
    , m_readonly(readonly)

{
    m_textfield = new QLineEdit;
    m_boolbox = new QCheckBox;
    m_list = new QComboBox;

    setTitle(parameter.Name());
    QLabel* label = new QLabel(parameter.Description());
    label->setMinimumWidth(150);
    label->setWordWrap(true);
    label->setMaximumWidth(200);
    setToolTip(parameter.Description());

    connect(m_textfield, SIGNAL(textChanged(QString)), this, SLOT(PrepareChanged()));
    connect(m_boolbox, SIGNAL(stateChanged(int)), this, SLOT(PrepareChanged()));
    connect(m_list, SIGNAL(currentIndexChanged(int)), this, SLOT(PrepareChanged()));

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(label, Qt::AlignLeft);
    if (parameter.isScalar() || parameter.isString()) {
        layout->addWidget(m_textfield, Qt::AlignRight);
        m_textfield->setMaximumWidth(150);
        m_textfield->setMinimumWidth(100);
    } else if (parameter.isBool())
        layout->addWidget(m_boolbox, Qt::AlignRight);

    else if (parameter.isList()) {
        layout->addWidget(m_list, Qt::AlignRight);
        m_list->addItems(parameter.getList());
    }

    if (parameter.isBool())
        m_boolbox->setChecked(parameter.Bool());
    else
        m_textfield->setText(parameter.value().toString());
    setLayout(layout);
    setMaximumWidth(400);
    setMinimumWidth(350);
    setMinimumHeight(80);
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
    QVariant variant = parameter.value();
    if (variant == m_textfield->text() && !m_parameter.isBool())
        return;

    m_change = true;
    if (m_parameter.isScalar() || m_parameter.isString() && variant != m_textfield->text())
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

SPOverview::SPOverview(DataClass* data, bool readonly)
    : m_data(data)
    , m_readonly(readonly)
{
    QGridLayout* mainlayout = new QGridLayout;
    QScrollArea* scrollArea = new QScrollArea;

    QWidget* flowidget = new QWidget;
    mainlayout->addWidget(scrollArea, 1, 0);
    FlowLayout* layout = new FlowLayout;
    for (int index : m_data->getSystemParameterList()) {
        QPointer<SystemParameterWidget> widget = new SystemParameterWidget(m_data->getSystemParameter(index), m_readonly, this);
        connect(widget, &SystemParameterWidget::valueChanged,
            [widget, this]() {
                if (widget) {
                    m_data->setSystemParameter(widget->Value());
                    m_data->WriteSystemParameter();
                }
            });

        connect(m_data, &DataClass::SystemParameterChanged,
            [index, widget, this]() {
                if (widget) {
                    widget->setValue(m_data->getSystemParameter(index));
                }
            });
        layout->addWidget(widget);
    }
    flowidget->setLayout(layout);
    scrollArea->setWidget(flowidget);
    setLayout(mainlayout);
}
