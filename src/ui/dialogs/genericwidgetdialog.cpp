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

#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>

#include "genericwidgetdialog.h"

GenericWidgetDialog::GenericWidgetDialog(const QString& title, QWidget* widget, QWidget* parent)
    : QDialog(parent)
    , m_widget(widget)
{
    setModal(true);
    resize(800, 600);
    setWindowTitle(title.toUtf8());
    setUi();
}

void GenericWidgetDialog::setUi()
{
    QGridLayout* mainlayout = new QGridLayout;
    setLayout(mainlayout);

    mainlayout->addWidget(m_widget, 0, 1, 1, 3);

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainlayout->addWidget(m_buttonbox, 1, 1, 1, 3);
}
