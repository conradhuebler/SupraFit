/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QTimer>

#include <QtGui/QDoubleValidator>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>

#include "parameterdialog.h"

ParameterDialog::ParameterDialog(const ParameterBoundary& boundary, double value, QWidget* parent)
    : QDialog(parent)
    , m_boundary(boundary)
    , m_value(value)
{
    setUi();
}

void ParameterDialog::setUi()
{
    QGridLayout* layout = new QGridLayout;
    setLayout(layout);

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_value_edit = new QDoubleSpinBox;
    m_value_edit->setDecimals(7);
    m_value_edit->setMinimum(-1e27);
    m_value_edit->setMaximum(1e27);
    m_value_edit->setValue((m_value));

    m_minimum_edit = new QDoubleSpinBox;
    m_minimum_edit->setDecimals(7);
    m_minimum_edit->setMinimum(-1e27);
    m_minimum_edit->setMaximum(1e27);
    m_minimum_edit->setValue((m_boundary.lower_barrier));

    m_maximum_edit = new QDoubleSpinBox;
    m_maximum_edit->setDecimals(7);
    m_maximum_edit->setMinimum(-1e27);
    m_maximum_edit->setMaximum(1e27);
    m_maximum_edit->setValue((m_boundary.upper_barrier));

    m_lower_limit = new QCheckBox;
    m_lower_limit->setChecked(m_boundary.limit_lower);
    connect(m_lower_limit, &QCheckBox::stateChanged, this, [this](int state) {
        m_minimum_edit->setEnabled(state);
        m_lower_barrier_beta->setEnabled(state);
        m_lower_barrier_wall->setEnabled(state);
    });

    m_upper_limit = new QCheckBox;
    m_upper_limit->setChecked(m_boundary.limit_upper);
    connect(m_upper_limit, &QCheckBox::stateChanged, this, [this](int state) {
        m_maximum_edit->setEnabled(state);
        m_upper_barrier_beta->setEnabled(state);
        m_upper_barrier_wall->setEnabled(state);
    });

    m_lower_barrier_beta = new QDoubleSpinBox;
    m_lower_barrier_beta->setValue((m_boundary.lower_barrier_beta));
    m_lower_barrier_beta->setDecimals(4);

    m_lower_barrier_wall = new QDoubleSpinBox;
    m_lower_barrier_wall->setValue((m_boundary.lower_barrier_wall));
    m_lower_barrier_wall->setRange(0, 1e10);

    m_upper_barrier_beta = new QDoubleSpinBox;
    m_upper_barrier_beta->setValue((m_boundary.upper_barrier_beta));
    m_upper_barrier_beta->setDecimals(4);

    m_upper_barrier_wall = new QDoubleSpinBox;
    m_upper_barrier_wall->setValue((m_boundary.upper_barrier_wall));
    m_upper_barrier_wall->setRange(0, 1e10);

    layout->addWidget(m_lower_limit, 0, 0);
    layout->addWidget(m_minimum_edit, 0, 1);
    layout->addWidget(m_value_edit, 0, 2);
    layout->addWidget(m_upper_limit, 0, 3);
    layout->addWidget(m_maximum_edit, 0, 4);
    layout->addWidget(m_lower_barrier_beta, 1, 0);
    layout->addWidget(m_lower_barrier_wall, 1, 1);
    layout->addWidget(m_upper_barrier_beta, 1, 2);
    layout->addWidget(m_upper_barrier_wall, 1, 3);
    layout->addWidget(m_buttonbox, 2, 0, 1, 4);

    m_minimum_edit->setEnabled(m_boundary.limit_lower);
    m_lower_barrier_beta->setEnabled(m_boundary.limit_lower);
    m_lower_barrier_wall->setEnabled(m_boundary.limit_lower);

    m_maximum_edit->setEnabled(m_boundary.limit_upper);
    m_upper_barrier_beta->setEnabled(m_boundary.limit_upper);
    m_upper_barrier_wall->setEnabled(m_boundary.limit_upper);
}

void ParameterDialog::accept()
{
    m_value = m_value_edit->value();

    m_boundary.limit_lower = m_lower_limit->isChecked();
    m_boundary.limit_upper = m_upper_limit->isChecked();

    m_boundary.lower_barrier = m_minimum_edit->value();
    m_boundary.upper_barrier = m_maximum_edit->value();

    m_boundary.lower_barrier_beta = m_lower_barrier_beta->value();
    m_boundary.upper_barrier_beta = m_upper_barrier_beta->value();

    m_boundary.lower_barrier_wall = m_lower_barrier_wall->value();
    m_boundary.upper_barrier_wall = m_upper_barrier_wall->value();

    QDialog::accept();
}
