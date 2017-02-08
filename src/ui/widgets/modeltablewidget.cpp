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

#include "src/core/models.h"

#include <QtCore/QJsonObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QCollator>

#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>

#include "modeltablewidget.h"

ModelTableWidget::ModelTableWidget()
{
    QGridLayout *layout = new QGridLayout;
    m_table = new QTableView(this);
    m_table->setSortingEnabled(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, SIGNAL(clicked(QModelIndex)), this, SLOT(rowSelected(QModelIndex)));
    connect(m_table, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));
    layout->addWidget(m_table, 0, 0);
    
    setLayout(layout);
}

ModelTableWidget::~ModelTableWidget()
{
    m_list.clear();
}

void ModelTableWidget::setModelList(const QList<QJsonObject>& list)
{
    if(m_model.isNull())
        return;
    m_list = list;
    QStandardItemModel *model = new QStandardItemModel;
    QStringList header = QStringList() <<  "Sum of Squares";
    for(int i = 0; i < list.size(); ++i)
    {
        double error = list[i]["sse"].toDouble();
        qDebug() << list[i]["sse"];
        QStandardItem *item = new QStandardItem(QString::number(error));
        item->setData(i, Qt::UserRole);
        model->setItem(i, 0, item);
        
        QJsonObject constants = list[i]["data"].toObject()["constants"].toObject();
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
        
        QString consts;
        int j = 1;
        for(const QString &str : qAsConst(keys))
        {
            QString element = constants[str].toString();
            if(!element.isNull() && !element.isEmpty())
            {
                QStandardItem *item = new QStandardItem(element);
                item->setData(i, Qt::UserRole);
                model->setItem(i, j, item);
                j++;
            }
            
        }
    }
    
    for(const QString &str : m_model.data()->ConstantNames())
        header << str;
    
    model->setHorizontalHeaderLabels(header);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    m_table->setModel(proxyModel);
}

void ModelTableWidget::rowSelected(const QModelIndex &index)
{
    int i = index.data(Qt::UserRole).toInt();
    QJsonObject model = m_list[i];
    emit LoadModel(model);
}

void ModelTableWidget::ShowContextMenu(const QPoint& pos)
{
    Q_UNUSED(pos)
    QModelIndex index = m_table->currentIndex();
    int i = index.data(Qt::UserRole).toInt();
    QJsonObject model = m_list[i];
    emit AddModel(model);
}

#include "modeltablewidget.moc"
