/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QPointer>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTabWidget>

class QTabWidget;
class QGridLayout;

class ModalDialog : public QDialog {
    Q_OBJECT
public:
    ModalDialog(QWidget* widget = 0, const QString& str = QString(tr("no name")));
    void setWidget(QWidget* widget, QString str = QString(tr("no name")));
    QWidget* Widget() { return m_widget; }
    inline int Count() const { return m_tab->count(); }

    inline bool contains(const QString& str) { return m_tabNames.contains(str); }

public slots:
    void Attention();

private:
    QGridLayout* layout;
    QPointer<QWidget> m_widget;
    QTabWidget* m_tab;
    QStringList m_tabNames;

private slots:
    void RemoveTab(int i);
};
