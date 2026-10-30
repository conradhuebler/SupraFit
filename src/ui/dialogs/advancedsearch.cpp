/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "advancedsearch.h"

#include "src/ui/widgets/scanwidget.h"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>

AdvancedSearch::AdvancedSearch(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Scan"));
}

void AdvancedSearch::setModel(const QSharedPointer<AbstractModel> model)
{
    delete layout();
    m_scan = new ScanWidget(model);
    QScrollArea* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_scan);
    QDialogButtonBox* buttons = new QDialogButtonBox;
    QPushButton* run = buttons->addButton(tr("Scan"), QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Close);
    connect(run, &QPushButton::clicked, this, [this]() { emit RunCalculation(m_scan->Job()); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(scroll);
    layout->addWidget(buttons);
    setLayout(layout);
    resize(480, 520);
}
