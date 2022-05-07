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

#include "src/capabilities/datagenerator.h"

#include <QtCore/QDebug>

#include <QJSEngine>
#include <QJSValue>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTableView>

#include "generatedatadialog.h"

GenerateDataDialog::GenerateDataDialog()
{
    setModal(true);
    m_generator = new DataGenerator(this);
    setUi();
    resize(800, 600);
    setWindowTitle(tr("Generate Data"));
}

void GenerateDataDialog::setUi()
{
    QGridLayout* mainlayout = new QGridLayout;
    setLayout(mainlayout);

    m_tableview = new QTableView;
    m_datapoints = new QSpinBox;
    m_datapoints->setMinimum(1);
    m_datapoints->setMaximum(1e25);
    m_datapoints->setValue(20);
    connect(m_datapoints, &QSpinBox::valueChanged, this, &GenerateDataDialog::ReshapeTable);

    m_independent = new QSpinBox;
    m_independent->setMinimum(1);
    m_independent->setMaximum(1e27);
    connect(m_independent, &QSpinBox::valueChanged, this, &GenerateDataDialog::ReshapeTable);

    mainlayout->addWidget(new QLabel(tr("Data Points:")), 0, 0);
    mainlayout->addWidget(m_datapoints, 0, 1);

    mainlayout->addWidget(new QLabel(tr("Independet Rows:")), 0, 2);
    mainlayout->addWidget(m_independent, 0, 3);

    m_equation = new QLineEdit;
    connect(m_equation, &QLineEdit::textEdited, this, [this]() {
        if (m_currentEquationIndex < m_equations.size() && m_currentEquationIndex != -1)
            m_equations[m_currentEquationIndex] = m_equation->text();
        Evaluate();
    });

    mainlayout->addWidget(new QLabel(tr("Equation")), 1, 0);
    mainlayout->addWidget(m_equation, 1, 1, 1, 3);
    mainlayout->addWidget(m_tableview, 2, 0, 1, 4);
    connect(m_tableview->horizontalHeader(), &QHeaderView::sectionDoubleClicked, this, [this](int i) {
        if (i >= m_equations.size() || i == -1)
            return;
        m_equation->setText(m_equations[i]);
        m_currentEquationIndex = i;
    });

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainlayout->addWidget(m_buttonbox, 3, 1, 1, 3);

    ReshapeTable();
}

void GenerateDataDialog::ReshapeTable()
{
    if (m_equations.size() < m_independent->value()) {
        while (m_equations.size() < m_independent->value())
            m_equations << "X";
    } else {
        while (m_equations.size() > m_independent->value())
            m_equations.removeLast();
    }
    Evaluate();
}

void GenerateDataDialog::Evaluate()
{
    QJsonObject data;
    data["independent"] = m_independent->value();
    data["datapoints"] = m_datapoints->value();
    data["equations"] = m_equations.join("|");
    m_generator->setJson(data);
    m_generator->Evaluate();
    m_table = m_generator->Table();
    m_tableview->setModel(m_table);
    m_data = data;
}
