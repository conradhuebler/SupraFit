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

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTabWidget>

#include "modaldialog.h"

ModalDialog::ModalDialog(QWidget* widget, const QString& str)
    : QDialog(widget)
{
    layout = new QGridLayout;
    setModal(false);
    m_tab = new QTabWidget;
    m_tab->setTabsClosable(true);
    layout->addWidget(m_tab, 0, 0);
    setWindowTitle(str);
    setLayout(layout);
    resize(800, 400);
    connect(m_tab, SIGNAL(tabCloseRequested(int)), this, SLOT(RemoveTab(int)));
}

void ModalDialog::Attention()
{
    show();
    raise();
    activateWindow();
}

void ModalDialog::setWidget(QWidget* widget, QString str)
{
    m_widget = widget;
    if (!m_widget->objectName().isEmpty() && !m_widget->objectName().isNull())
        str = m_widget->objectName();
    if (windowTitle().isEmpty() || windowTitle().isNull())
        setWindowTitle(str);
    int i = m_tab->addTab(m_widget, str);
    m_tab->setCurrentIndex(i);
    m_tabNames << str;
}

void ModalDialog::RemoveTab(int tab)
{
    QWidget* model = qobject_cast<QWidget*>(m_tab->widget(tab));
    m_tab->removeTab(tab);
    m_tabNames.removeAt(tab);
    delete model;
}

#include "modaldialog.moc"
