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
#include "core/data/dataclass.h"

#include <QtWidgets/QWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTableWidget>
#include <QtCore/QPointer>

class DataWidget : public QWidget
{
    Q_OBJECT

public:
    DataWidget();
    ~DataWidget();
    void setData(DataClass *data);
private:
    QPointer<QTableView > m_concentrations, m_signals;
    DataClass *m_data;
};

#endif // DATAWIDGET_H
