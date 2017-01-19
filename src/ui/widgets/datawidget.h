/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifndef DATAWIDGET_H
#define DATAWIDGET_H
#include "src/core/dataclass.h"

#include <QtWidgets/QWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTableWidget>
#include <QtCore/QPointer>
class QPushButton;

class DataWidget : public QWidget
{
    Q_OBJECT

public:
    DataWidget();
    ~DataWidget();
    void setData(QWeakPointer<DataClass> dataclass);
    void clear();
public slots:
       
    void RowAdded();

private:
    QPointer<QTableView > m_concentrations, m_signals;
    QPointer<QPushButton > m_switch;
    QWeakPointer<DataClass > m_data;
private slots:
    void switchHG();
    
signals:
    void recalculate();
};

#endif // DATAWIDGET_H
