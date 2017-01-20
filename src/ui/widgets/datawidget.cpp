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

#include <QApplication>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTableWidget>
#include <QStandardItemModel>

#include <QDebug>
DataWidget::DataWidget() 
{
    QGridLayout *layout = new QGridLayout;
    m_switch = new QPushButton(tr("switch h/g"));
    
    connect(m_switch, SIGNAL(clicked()), this, SLOT(switchHG()));
    setLayout(layout);
    m_name = new QLineEdit();
    connect(m_name, SIGNAL(textChanged(QString)), this, SLOT(SetProjectName()));
    m_concentrations = new QTableView;
        m_concentrations->setMaximumWidth(200);
    m_signals = new QTableView;
        m_signals->setMaximumWidth(750);

    QHBoxLayout *hlayout = new QHBoxLayout;
    
    hlayout->addWidget(new QLabel(tr("Project Name")));
    hlayout->addWidget(m_name);
    hlayout->addSpacing(2*width()/3);
    hlayout->addWidget(m_switch);
    
    layout->addLayout(hlayout, 0, 0, 1, 2);
    layout->addWidget(m_concentrations, 1, 0);
    layout->addWidget(m_signals, 1, 1);
}

DataWidget::~DataWidget()
{
}
void DataWidget::clear()
{

}


void DataWidget::setData(QWeakPointer<DataClass> dataclass)
{
    m_data = dataclass;
    m_concentrations->setModel(m_data.data()->ConcentrationModel());
    m_signals->setModel(m_data.data()->SignalModel());
    m_concentrations->resizeColumnsToContents();
    m_signals->resizeColumnsToContents();
    m_name->setText(qApp->instance()->property("projectname").toString());
}

void DataWidget::switchHG()
{
    m_data.data()->SwitchConentrations();
    emit recalculate();
}

void DataWidget::RowAdded()
{
    
    QStandardItemModel *concentration = new QStandardItemModel;
    QStandardItemModel *signal = new QStandardItemModel;
 
    m_concentrations->setModel(concentration);
    m_signals->setModel(signal);
    m_concentrations->resizeColumnsToContents();
    m_signals->resizeColumnsToContents();
    
}

void DataWidget::SetProjectName()
{
    qApp->instance()->setProperty("projectname", m_name->text());
    emit NameChanged();
}


#include "datawidget.moc"
