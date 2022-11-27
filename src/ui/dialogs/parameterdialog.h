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

#pragma once

#include "src/global.h"

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QLineEdit;
class QDoubleSpinBox;
class ParameterDialog : public QDialog {
public:
    ParameterDialog(const ParameterBoundary& boundary, double value, QWidget* parent = 0);

    void setUi();
    virtual void accept() override;
    inline ParameterBoundary Boundary() const { return m_boundary; }
    inline double Value() const { return m_value; }

private:
    void showParameter();
    void hideParameter();
    QDialogButtonBox* m_buttonbox;

    QDoubleSpinBox *m_value_edit, *m_maximum_edit, *m_minimum_edit, *m_lower_barrier_beta, *m_lower_barrier_wall, *m_upper_barrier_beta, *m_upper_barrier_wall;
    QCheckBox *m_lower_limit, *m_upper_limit;

    ParameterBoundary m_boundary;
    double m_value;
    int m_focus = 0;
};
