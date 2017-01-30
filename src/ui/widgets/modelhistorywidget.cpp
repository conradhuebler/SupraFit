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
#include <QtCore/QPointer>
#include <QDebug>

#include <iostream>

#include "modelhistorywidget.h"

ModelHistoryWidget::ModelHistoryWidget(const ModelHistoryElement *element, QWidget *parent) : QGroupBox(parent), m_json(&element->model)
{
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel("<h3>" + m_json->value("model").toString() + "</h3>"), 0, 0, 1, 2);
    int active = 0;
    for(int i = 0; i < element->active_signals.size(); ++i)
        active += element->active_signals[i];
    layout->addWidget(new QLabel(QString::number(active) + " used signals"), 2, 0, 1, 2);
    layout->addWidget(new QLabel("Error"), 3, 0);
    layout->addWidget(new QLabel(QString::number(element->error)), 3, 1);
    
    m_add = new QPushButton(tr("Duplicate\nModel"));
    m_add->setFlat(true);
    m_load = new QPushButton(tr("Load Model"));
    m_load->setFlat(true);
    m_remove = new QPushButton(tr("Remove"));
    m_remove->setFlat(true);
    
    connect(m_add, SIGNAL(clicked()), this, SLOT(AddModel()));
    connect(m_load, SIGNAL(clicked()), this, SLOT(LoadModel()));
    connect(m_remove, SIGNAL(clicked()), this, SLOT(remove()));
    layout->addWidget(m_add, 4, 0);
    layout->addWidget(m_load, 4, 1);
    layout->addWidget(m_remove, 5, 0, 1, 2);
    setLayout(layout);
    setFixedSize(200,180);
}

void ModelHistoryWidget::remove()
{
    QPointer<ModelHistoryWidget > element = qobject_cast<ModelHistoryWidget *>( this);
    emit Remove(m_json, element); 
}

ModelHistory::ModelHistory(QMap<int, ModelHistoryElement> *history, QWidget *parent) : m_history(history), QWidget(parent)
{
    m_vlayout = new QVBoxLayout;
    m_vlayout->setAlignment(Qt::AlignTop);
    setLayout(m_vlayout);
    setBackgroundRole(QPalette::Midlight);

    
}

ModelHistory::~ModelHistory()
{
}

void ModelHistory::InsertElement(const ModelHistoryElement *elm)
{
    QPointer<ModelHistoryWidget > element = new ModelHistoryWidget(elm);
    m_vlayout->addWidget(element);
    connect(element, SIGNAL(AddJson(QJsonObject)), this, SIGNAL(AddJson(QJsonObject)));
    connect(element, SIGNAL(LoadJson(QJsonObject)), this, SIGNAL(LoadJson(QJsonObject)));
    connect(element, SIGNAL(Remove(const QJsonObject *, QPointer<ModelHistoryWidget>)), this, SLOT(Remove(const QJsonObject *, QPointer<ModelHistoryWidget>)));
}

void ModelHistory::Remove(const QJsonObject *json, QPointer<ModelHistoryWidget> element)
{
    Q_UNUSED(json)
    if(element)
    {
        QLayoutItem * item= m_vlayout->itemAt(m_vlayout->indexOf(element));
        m_vlayout->removeItem(item);
        /*
         * I know, that we are only deleting the widget but not the element in the map, will be a FIXME
         */
        delete element;
    }
}

#include "modelhistorywidget.moc"
