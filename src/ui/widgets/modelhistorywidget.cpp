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
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtCore/QMap>
#include <QDebug>
#include "modelhistorywidget.h"

ModelHistoryWidget::ModelHistoryWidget(const ModelHistoryElement *element, QWidget *parent) : m_json(&element->model), QGroupBox(parent)
{
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel("Error"), 0, 0);
    QLineEdit *error = new QLineEdit;
    error->setReadOnly(true);
    error->setText(QString::number(element->error));
    layout->addWidget(error, 0, 1);
    qDebug() << "reached";
    m_add = new QPushButton(tr("Add Model"));
    m_load = new QPushButton(tr("Load Model"));
    connect(m_add, SIGNAL(clicked()), this, SLOT(AddModel()));
    connect(m_load, SIGNAL(clicked()), this, SLOT(LoadModel()));
    layout->addWidget(m_add, 1, 0);
    layout->addWidget(m_load, 1, 1);
    
    setLayout(layout);
//     setFixedSize(300,100);
}

ModelHistory::ModelHistory(QMap<int, ModelHistoryElement> *history, QWidget *parent) : m_history(history), QScrollArea(parent)
{
    m_mainwidget = new QWidget;
    m_vlayout = new QVBoxLayout;
    m_mainwidget->setLayout(m_vlayout);
    QHBoxLayout *layout = new QHBoxLayout;

    setWidget(m_mainwidget);
    setWidgetResizable(true);
    setAlignment(Qt::AlignTop);
    setBackgroundRole(QPalette::Midlight);
    layout->addWidget(m_mainwidget);
    setLayout(layout);
    
}

ModelHistory::~ModelHistory()
{
}

void ModelHistory::InsertElement(const ModelHistoryElement *elm)
{
    ModelHistoryWidget *element = new ModelHistoryWidget(elm);
        m_vlayout->addWidget(element);
}



#include "modelhistorywidget.moc"
