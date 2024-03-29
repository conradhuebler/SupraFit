/*
 * <one line to give the library's name and an idea of what it does.>
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

#include "src/capabilities/datagenerator.h"

#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableWidget>

class GenerateDataDialog : public QDialog {

    Q_OBJECT
public:
    GenerateDataDialog();

    DataTable* Table() const { return m_table; }
    QJsonObject Data() const { return m_data; }

private:
    void setUi();
    void ReshapeTable();
    void Evaluate();

    QTableView* m_tableview;
    QSpinBox *m_independent, *m_datapoints;
    QLineEdit* m_equation;
    QStringList m_equations;
    QDialogButtonBox* m_buttonbox;

    DataGenerator* m_generator;

    QPointer<DataTable> m_table;
    QJsonObject m_data;

    int m_currentEquationIndex = 0;
};
