/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/guitools/chartwrapper.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QFile>
#include <QtCore/QMimeData>
#include <QtCore/QSharedPointer>

#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QWidget>

class QTreeView;

class ParameterTree : public QAbstractItemModel {
    Q_OBJECT
public:
    inline ParameterTree(QWeakPointer<MetaModel> model, QHash<QSharedPointer<AbstractModel>, QColor>* linked_list)
        : m_linked_list(linked_list)
    {
        m_model = model;
        m_null = &null;
    }

    inline ~ParameterTree() override
    {
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            if (section == 0)
                return tr("Parameter");
            else
                return tr("# Affected Models");
        } else
            return QAbstractItemModel::headerData(section, orientation, role);
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        if (!index.isValid())
            return Qt::ItemIsDropEnabled;

        Qt::ItemFlags flag;

        qreal* pos = static_cast<qreal*>(index.internalPointer());
        bool ok = false;
        index.data(Qt::DisplayRole).toString().toDouble(&ok);
        qDebug() << ok << index.data(Qt::DisplayRole).toString();
        if (pos == m_null && ok && index.column() == 0)
            flag = Qt::ItemIsEditable;

        return flag | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | QAbstractItemModel::flags(index);
    }

    virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::CopyAction | Qt::MoveAction;
    }

    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& index) override;

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

    virtual QModelIndex parent(const QModelIndex& child) const override;

    virtual bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& index) const override;

private:
    QWeakPointer<MetaModel> m_model;
    QHash<QSharedPointer<AbstractModel>, QColor>* m_linked_list;

    qreal null = -1;
    qreal* m_null;
};

class ModelParameterEntry : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ModelParameterEntry(QObject* parent = 0)
        : QStyledItemDelegate(parent)
    {
    }
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    //   void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

private:
};

class MetaModelParameter : public QWidget {
    Q_OBJECT
public:
    MetaModelParameter(QSharedPointer<AbstractModel> model, QHash<QSharedPointer<AbstractModel>, QColor>* linked_list);
    ~MetaModelParameter();

signals:

public slots:

private:
    QTreeView* m_tree;
    QWeakPointer<MetaModel> m_model;
    QGridLayout* m_layout;
    ParameterTree* m_treemodel;
    QHash<QSharedPointer<AbstractModel>, QColor>* m_linked_list;

    void setUi();

private slots:
    void SplitParameter();
};
