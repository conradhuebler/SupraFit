/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
    inline ParameterTree(QWeakPointer<MetaModel> model)
    {
        m_model = model;
        m_null = &null;
    }

    inline ~ParameterTree()
    {
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        if (!index.isValid())
            return Qt::ItemIsDropEnabled;

        Qt::ItemFlags flag;

        qreal* pos = static_cast<qreal*>(index.internalPointer());
        bool ok = false;
        index.data(Qt::DisplayRole).toDouble(&ok);
        if (pos == m_null && ok)
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
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
};

class MetaModelParameter : public QWidget {
    Q_OBJECT
public:
    MetaModelParameter(QSharedPointer<AbstractModel> model);
    ~MetaModelParameter();

signals:

public slots:

private:
    QTreeView* m_tree;
    QWeakPointer<MetaModel> m_model;
    QGridLayout* m_layout;
    ParameterTree* m_treemodel;
    void setUi();

private slots:
    void SplitParameter();
};
