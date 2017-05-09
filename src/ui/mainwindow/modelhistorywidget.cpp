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

#include "src/core/toolset.h"

#include <QtCore/QCollator>
#include <QtCore/QMap>
#include <QtCore/QPointer>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>

#include <QDebug>

#include <iostream>

#include "modelhistorywidget.h"

ModelHistoryWidget::ModelHistoryWidget(const QJsonObject *element, int active, int index, QWidget *parent) : QGroupBox(parent), m_json(element), m_index(index)
{
    QGridLayout *layout = new QGridLayout;
    QJsonObject constants = (*m_json)["data"].toObject()["constants"].toObject();
    QStringList keys = constants.keys();
    
    if(keys.size() > 10)
    {
        QCollator collator;
        collator.setNumericMode(true);
        std::sort(
            keys.begin(),
                  keys.end(),
                  [&collator](const QString &key1, const QString &key2)
                  {
                      return collator.compare(key1, key2) < 0;
                  });
    }
    
    QString consts = "<p>";
    for(const QString &str : qAsConst(keys))
    {
        QString element = constants[str].toString();
        if(!element.isNull() && !element.isEmpty())
        {
            consts += element;
            consts +="; ";
        }
    }
    consts.chop(2);
    consts += "</p><font color =\'red\'>Converged: " + ToolSet::bool2YesNo((*m_json)["converged"].toBool()) + "</font>";
    QLabel *constant_overview = new QLabel;
    constant_overview->setText(consts);
    constant_overview->setTextFormat(Qt::RichText);
    layout->addWidget(constant_overview, 1, 0, 1, 2);
    

    layout->addWidget(new QLabel(QString::number(active) + " used signals"), 2, 0, 1, 2);
    
    layout->addWidget(new QLabel("Error"), 3, 0);
    layout->addWidget(new QLabel(QString::number((*m_json)["sum_of_squares"].toDouble())), 3, 1);
    
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
    setFixedSize(200,250);
    setAlignment(Qt::AlignHCenter);
    setTitle(m_json->value("model").toString());
}

void ModelHistoryWidget::remove()
{
    QPointer<ModelHistoryWidget > element = qobject_cast<ModelHistoryWidget *>( this);
    emit Remove(m_index, element); 
}

ModelHistory::ModelHistory( QWidget *parent) : QWidget(parent), m_index(0)
{
    m_vlayout = new QVBoxLayout;
    m_vlayout->setAlignment(Qt::AlignTop);
    setLayout(m_vlayout);
    setBackgroundRole(QPalette::Midlight);
    setMaximumWidth(230);
}

ModelHistory::~ModelHistory()
{
}
void ModelHistory::InsertElement(const QJsonObject &model)
{
    int active;
    QStringList keys = model["pureShift"].toObject().keys();
    active = keys.size();
    InsertElement(model, active);    
}


void ModelHistory::InsertElement(const QJsonObject &model, int active)
{
    QJsonObject *object = new QJsonObject(model);
    m_history[m_index] = object;
    QPointer<ModelHistoryWidget > element = new ModelHistoryWidget(object, active, m_index); 
    m_vlayout->addWidget(element);
    connect(element, SIGNAL(AddJson(QJsonObject)), this, SIGNAL(AddJson(QJsonObject)));
    connect(element, SIGNAL(LoadJson(QJsonObject)), this, SIGNAL(LoadJson(QJsonObject)));
    connect(element, SIGNAL(Remove(int, QPointer<ModelHistoryWidget>)), this, SLOT(Remove(int, QPointer<ModelHistoryWidget>)));
    m_index++;
}

void ModelHistory::Remove(int index, QPointer<ModelHistoryWidget> element)
{
    if(element)
    {
        QLayoutItem * item= m_vlayout->itemAt(m_vlayout->indexOf(element));
        m_vlayout->removeItem(item);
        delete element;
        m_history.remove(index);
    }
}

#include "modelhistorywidget.moc"
