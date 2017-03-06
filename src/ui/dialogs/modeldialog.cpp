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
#include <QtWidgets/QTabWidget>
#include "modeldialog.h"


ModalDialog::ModalDialog(QWidget* widget, const QString& str) : m_widget(widget)
{
    layout = new QGridLayout;
    setModal(false);
    m_tab = new QTabWidget;
    layout->addWidget(m_tab, 0, 0);
    setWindowTitle(str);
    setLayout(layout);
    resize(800, 400);
}


ModalDialog::~ModalDialog()
{
}

void ModalDialog::setWidget(QWidget* widget, const QString &str)
{
    if(windowTitle().isEmpty() || windowTitle().isNull())
        setWindowTitle(str);
    m_widget = widget;
    m_tab->addTab(m_widget, str);
}


#include "modeldialog.moc"
