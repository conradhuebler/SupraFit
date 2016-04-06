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

#include "datawidget.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTableWidget>
#include <QStandardItemModel>

#include <QDebug>
DataWidget::DataWidget()
{
    QGridLayout *layout = new QGridLayout;
    
    setLayout(layout);
    
    
    m_concentrations = new QTableView;
        m_concentrations->setFixedWidth(250);
    m_signals = new QTableView;
    layout->addWidget(m_concentrations, 0, 0);
    layout->addWidget(m_signals, 0, 1);
}

DataWidget::~DataWidget()
{
    
}

void DataWidget::setData(DataClass* data)
{
    m_data = data;
    QStandardItemModel *concentration = new QStandardItemModel;
    QStandardItemModel *signal = new QStandardItemModel;
    
    for(int i = 0; i < m_data->Size(); i++)
    {
        QList<QStandardItem *> row;
        row.append(new QStandardItem(QString::number(m_data->operator[](i)->Conc1())));
        row.append(new QStandardItem(QString::number(m_data->operator[](i)->Conc2())));
        concentration->appendRow(row);
        QList<QStandardItem *> row2;
        QVector<qreal > datas = m_data->operator[](i)->Data();
        for(int i = 0; i < datas.size(); ++i)
            row2.append(new QStandardItem(QString::number(datas[i])));
        signal->appendRow(row2);
    }
    m_concentrations->setModel(concentration);
    m_signals->setModel(signal);
}

#include "datawidget.moc"
