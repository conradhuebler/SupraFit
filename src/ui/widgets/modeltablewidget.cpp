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


#include "src/core/jsonhandler.h"
#include "src/core/models.h"
#include "src/ui/widgets/optimizerwidget.h"

#include <QtCore/QJsonObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QCollator>
#include <QtCore/QFile>

#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFileDialog>

#include "modeltablewidget.h"

ModelTableWidget::ModelTableWidget()
{
    QGridLayout *layout = new QGridLayout;
    
    m_export = new QPushButton(tr("Export Models"));
    m_valid = new QCheckBox(tr("Invalid Models"));
    m_threshold = new ScientificBox;
    m_threshold->setValue(1);
    layout->addWidget(new QLabel(tr("Threshold SSE")), 0, 0);
    layout->addWidget(m_threshold, 0, 1);
    layout->addWidget(m_valid, 0, 2);
    layout->addWidget(m_export, 0, 3);
    
    m_table = new QTableView(this);
    m_table->setSortingEnabled(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, SIGNAL(clicked(QModelIndex)), this, SLOT(rowSelected(QModelIndex)));
    connect(m_table, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));
    connect(m_export, SIGNAL(clicked()), this, SLOT(ExportModels()));
    layout->addWidget(m_table, 1, 0, 1, 4);
    
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
    m_table->resizeColumnsToContents();
    resize(m_table->sizeHint());
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

void ModelTableWidget::ExportModels()
{
    qreal threshold = m_threshold->value();
    bool allow_invalid = m_valid->isChecked();
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), ".", tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(str.isEmpty())
        return;

    QJsonObject toplevel;
    int i = 0;
    for(const QJsonObject &obj: qAsConst(m_list))
    {
        QJsonObject constants = obj["data"].toObject()["constants"].toObject();
        QStringList keys = constants.keys();
        bool valid = true;
        for(const QString &str : qAsConst(keys))
        {
            double var = constants[str].toString().toDouble();
            valid = valid && (var > 0);
        }
        double error = obj["sse"].toDouble();
        if(error < threshold && (valid || allow_invalid))
            toplevel["model_" + QString::number(i++)] = obj;       
    }
    JsonHandler::WriteJsonFile(toplevel, str);
}

#include "modeltablewidget.moc"