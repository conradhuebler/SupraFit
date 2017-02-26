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

#include <QtWidgets/QGridLayout>

#include "modeldialog.h"



// ModalDialog::ModalDialog()
// {
//     layout = new QGridLayout;
//     m_widget = new QWidget;
//     layout->addWidget(m_widget, 0, 0);
//     setModal(false);
//     setLayout(layout);
// }

ModalDialog::ModalDialog(QWidget* widget, const QString& str) : m_widget(widget)
{
    layout = new QGridLayout;
    layout->addWidget(m_widget, 0, 0);
    setModal(false);
    setWindowTitle(str);
    setLayout(layout);
}


ModalDialog::~ModalDialog()
{
}

void ModalDialog::setWidget(QWidget* widget, const QString &str)
{
    setWindowTitle(str);
    delete m_widget;
    m_widget = widget;
    layout->addWidget(m_widget, 0, 0);
}


#include "modeldialog.moc"
